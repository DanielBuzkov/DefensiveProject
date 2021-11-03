from threading import Thread
from utils.protocol import *
import uuid


class ClientData:
    def __init__(self, name, public_key):
        self.name = name
        self.public_key = public_key
        self.recv_messages = []


class ClientHandler:
    def __init__(self):
        self.users = {}

    #------------------------------------------- UTILS -------------------------------------------

    def is_registered(self, client_id):
        return client_id in self.users

    def username_exists(self, name):
        for user in self.users:
            if self.users[user].name == name:
                return True

        return False

    def is_sender_valid(self, header):
        if header.code != Opcodes.RegisterReq and self.is_registered(UUID(bytes=header.client_id)) is False:
            return False

        return True

    def send_error(self, client_sock):
        print("SENDING ERROR!")
        response = ResponseHeader()
        client_sock.send(response.raw)

    #------------------------------------------- HANDLERS -------------------------------------------

    def handle_register(self, payload):
        body = RegisterReqBody(payload)

        # if there is already a user with this name
        if self.username_exists(body.name):
            return None

        new_id = uuid.uuid1()
        while self.is_registered(new_id):
            new_id = uuid.uuid1()

        self.users[new_id] = ClientData(body.name, body.pk)
        response = RegisterResBody(new_id)

        return response.raw

    def handle_user_list(self):
        # This handler doesn't need body data, thus no parsing is needed.
        send_buffer = bytes()

        for uuid in self.users:
            send_buffer += UserListResNode(uuid, self.users[uuid].name).raw

        return send_buffer

    def handle_get_public_key(self, payload):
        body = GetPKReqBody(payload)
        response = GetPKResBody(body.client_id, self.users[UUID(bytes=body.client_id)].public_key)

        return response.raw

    def handle_send_message(self, payload):
        body = SendMessageReqBody(payload)
        # TODO : Generate Message ID
        response = SendMessageResBody(body.client_id, 69)

        if body.content_size != (len(payload) - body.get_sub_header_size()):
            return None

        # Saving only valid messages.
        if not MessageType.contains(body.message_type):
            return None

        # Validating logical relation of the size and the content.
        if (body.message_type == MessageType.GetSK and
            body.content_size != 0):
            return None

        elif (body.message_type == MessageType.SendSK and
            body.content_size != SYM_KEY_LENGTH):
            return None

        self.users[UUID(bytes=body.client_id)].recv_messages.append(body)

        return response.raw

    def handle_get_messages(self, uuid):
        # This handler doesn't need body data, thus no parsing is needed.
        send_buffer = bytes()

        for message in self.users[UUID(bytes=uuid)].recv_messages:
            send_buffer += message.raw

        return send_buffer

    def handle_client(self, client_socket):
        # Validate header:
        data = client_socket.recv(REQ_HEADER_LEN)

        if len(data) != REQ_HEADER_LEN:
            self.send_error(client_socket)
            print("Invalid structure")
            return

        header = RequestHeader(data)

        if header.is_valid() is False:
            self.send_error(client_socket)
            print("Invalid header")
            return

        # Validate body:
        payload = client_socket.recv(header.payload_size)

        if len(payload) != header.payload_size:
            self.send_error(client_socket)
            print("Invalid len ({} != {})".format(len(payload), header.payload_size))
            return

        if self.is_sender_valid(header) is False:
            self.send_error(client_socket)
            print("Received message from invalid source")
            return

        # Let's go
        try:
            if header.code == Opcodes.RegisterReq:
                response_body = self.handle_register(payload)
                response_opcode = Opcodes.RegisterRes

            elif header.code == Opcodes.UserListReq:
                response_body = self.handle_user_list()
                response_opcode = Opcodes.UserListRes

            elif header.code == Opcodes.GetPKReq:
                response_body = self.handle_get_public_key(payload)
                response_opcode = Opcodes.GetPKRes

            elif header.code == Opcodes.SendMessageReq:
                response_body = self.handle_send_message(payload)
                response_opcode = Opcodes.SendMessageRes

            elif header.code == Opcodes.GetMessagesReq:
                response_body = self.handle_get_messages(header.client_id)
                response_opcode = Opcodes.GetMessagesRes

            else:
                self.send_error(client_socket)
                return

            if response_body is None:
                self.send_error(client_socket)
            else:
                response_header = ResponseHeader(code=response_opcode, payload_size=len(response_body))
                client_socket.send(response_header.raw + bytes(response_body))

        except ValueError as e:
            '''
                Short cut for sending error to client.
                Expecting to catch any invalid UUID accesses and more.
            '''
            self.send_error(client_socket)
