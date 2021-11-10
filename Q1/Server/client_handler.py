from threading import Thread, Lock
from utils.protocol import *
import uuid
import socket


class ClientData:
    def __init__(self, name, public_key):
        self.name = name
        self.public_key = public_key
        self.recv_messages = []


class ClientHandler:
    def __init__(self):
        self.mutex = Lock()
        self.users = {}

    def locker(func):
        def inner(self, *args, **kwargs):
            self.mutex.acquire()
            try:
                ret = func(self, *args, **kwargs)
                return ret
            finally:
                self.mutex.release()

        return inner

    #------------------------------------------- UTILS -------------------------------------------

    @locker
    def is_registered(self, client_id : UUID):
        return client_id in self.users

    @locker
    def username_exists(self, name):
        for user in self.users:
            if self.users[user].name == name:
                return True

        return False

    @locker
    def add_user(self, client : ClientData, uuid : UUID):
        if uuid not in self.users:
            self.users[uuid] = client

    @locker
    def get_pk_from_uuid(self, uuid : UUID):
        return self.users[uuid].public_key

    @locker
    def push_message(self, uuid : UUID, message : SendMessageReqBody):
        self.users[uuid].recv_messages.append(message)

    @locker
    def get_name_from_uuid(self, uuid : UUID):
        return self.users[uuid].name

    @locker
    def get_messages_from_uuid(self, uuid : UUID):
        send_buffer = bytes()

        for message in self.users[uuid].recv_messages:
            send_buffer += message.raw

        self.users[uuid].recv_messages.clear()

        return send_buffer

    def is_sender_valid(self, header : RequestHeader):
        if header.code != Opcodes.RegisterReq and self.is_registered(UUID(bytes=header.client_id)) is False:
            return False

        return True

    def send_error(self, client_sock : socket):
        response = ResponseHeader()
        client_sock.send(response.raw)

    #------------------------------------------- HANDLERS -------------------------------------------

    def handle_register(self, payload):
        body = RegisterReqBody(payload)

        # if there is already a user with this name
        if self.username_exists(body.name):
            print("User name exits")
            return None

        new_id = uuid.uuid1()
        while self.is_registered(new_id):
            new_id = uuid.uuid1()

        self.add_user(ClientData(body.name, body.pk), new_id)
        response = RegisterResBody(new_id)

        return response.raw

    def handle_user_list(self, client_uuid):
        # This handler doesn't need body data, thus no parsing is needed.
        send_buffer = bytes()

        for uuid in self.users:
            if uuid == UUID(bytes=client_uuid):
                continue
            send_buffer += UserListResNode(uuid, self.get_name_from_uuid(uuid)).raw

        return send_buffer

    def handle_get_public_key(self, payload):
        body = GetPKReqBody(payload)
        response = GetPKResBody(body.client_id, self.get_pk_from_uuid(UUID(bytes=body.client_id)))

        return response.raw

    def handle_send_message(self, payload, uuid):
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
            body.content_size != 128):
            return None

        # Switching id's, so the message itself will contain the sender's id
        dest_id = body.client_id
        body.client_id = uuid

        body.update()

        self.push_message(UUID(bytes=dest_id), body)

        return response.raw

    def handle_get_messages(self, uuid):
        return self.get_messages_from_uuid(UUID(bytes=uuid))

    def handle_client_thread(self, client_socket : socket):
        try:
            self.handle_client(client_socket)
        finally:
            client_socket.close()

    def handle_client(self, client_socket : socket):
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
                response_body = self.handle_user_list(header.client_id)
                response_opcode = Opcodes.UserListRes

            elif header.code == Opcodes.GetPKReq:
                response_body = self.handle_get_public_key(payload)
                response_opcode = Opcodes.GetPKRes

            elif header.code == Opcodes.SendMessageReq:
                response_body = self.handle_send_message(payload, header.client_id)
                response_opcode = Opcodes.SendMessageRes

            elif header.code == Opcodes.GetMessagesReq:
                response_body = self.handle_get_messages(header.client_id)
                response_opcode = Opcodes.GetMessagesRes

            else:
                self.send_error(client_socket)
                print("Invalid option")
                return

            if response_body is None:
                self.send_error(client_socket)
                print("Handling failed")
            else:
                print("All good in the hood")
                response_header = ResponseHeader(code=response_opcode, payload_size=len(response_body))
                client_socket.send(response_header.raw + bytes(response_body))

        except ValueError as e:
            '''
                Short cut for sending error to client.
                Expecting to catch any invalid UUID accesses and more.
            '''
            self.send_error(client_socket)
