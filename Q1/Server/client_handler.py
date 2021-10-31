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
        self.handlers = {
            Opcodes.RegisterReq: ClientHandler.handle_register,
            Opcodes.UserListReq: ClientHandler.handle_user_list,
            Opcodes.GetPKReq: ClientHandler.handle_get_public_key,
            Opcodes.SendMessageReq: ClientHandler.handle_send_message,
            Opcodes.GetMessagesReq: ClientHandler.handle_get_messages,
        }
        self.users = {}

    def is_registered(self, client_id):
        return client_id in self.users

    def username_exists(self, name):
        for user in self.users:
            if self.users[user].name == name:
                return True

        return False

    def is_header_valid(self, header):
        if not Opcodes.contains(header.code):
            return False

        if header.code != Opcodes.RegisterReq and self.is_registered(UUID(bytes=header.client_id)) is False:
            return False

        return True

    def send_error(self, client_sock):
        response = ResponseHeader()
        client_sock.send(response.raw)

    def handle_register(self, payload):
        body = RegisterReqBody(payload)

        # if there is already a user with this name
        if self.username_exists(body.name):
            return None, None

        new_id = uuid.uuid1()
        while self.is_registered(new_id):
            new_id = uuid.uuid1()

        self.users[new_id] = ClientData(body.name, body.pk)
        response = RegisterResBody(new_id)

        return response.raw, Opcodes.RegisterRes

    def handle_user_list(self, payload):
        # This handler doesn't need body data, thus no parsing is needed.

        send_buffer = bytes()

        for uuid in self.users:
            send_buffer += UserListResNode(uuid, self.users[uuid].name).raw

        return send_buffer, Opcodes.UserListRes

    ############################ NEEDS WORK: ###################################
    def handle_get_public_key(self, payload):
        body = GetPKReqBody(payload)
        response = GetPKResBody(body.client_id, self.users[body.client_id].public_key)

        return response.raw, Opcodes.GetPKRes

    def handle_send_message(self, payload):
        return None

    def handle_get_messages(self, payload):
        return None

    def handle_client(self, client_socket):
        data = client_socket.recv(REQ_HEADER_LEN)
        header = RequestHeader(data)

        payload = client_socket.recv(header.payload_size)

        if len(payload) != header.payload_size:
            self.send_error(client_socket)
            print("Invalid len ({} != {})".format(len(payload), header.payload_size))
            return

        if self.is_header_valid(header) is False:
            self.send_error(client_socket)
            print("Header is invalid")
            return

        try:
            func = self.handlers[header.code]
            response_body, response_opcode = func(self, payload)
        except ValueError:
            # Short cut for sending error to client
            response_body = None

        if response_body is None:
            self.send_error(client_socket)
        else:
            response_header = ResponseHeader(code=response_opcode, payload_size=len(response_body))
            client_socket.send(response_header.raw + bytes(response_body))
