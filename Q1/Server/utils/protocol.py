from enum import IntEnum, unique
import struct
from uuid import UUID
'''
This file contains all known opcodes and message structures.

The entire system's protocol is defined by the structs and 
classes shown below.

A message which doesn't meet the given protocol will not be 
handled and discarded away.

Opcodes with no matching body struct have an empty body.

The format for the packing and unpacking is given in each 
message class as described in python's official 'struct' 
documentation.
'''

SERVER_VERSION = 2

NAME_LEN = 255
PUBLIC_KEY_LEN = 160
SYM_KEY_LENGTH = 16
UUID_LEN = 16

# Header = UUID, version, code, size
REQ_HEADER_LEN = UUID_LEN + 1 + 2 + 4


@unique
class Opcodes(IntEnum):
    RegisterReq = 1000
    UserListReq = 1001
    GetPKReq = 1002
    SendMessageReq = 1003
    GetMessagesReq = 1004

    RegisterRes = 2000
    UserListRes = 2001
    GetPKRes = 2002
    SendMessageRes = 2003
    GetMessagesRes = 2004

    CommunicationError = 9000

    # Implementing an easy search function for enums.
    @classmethod
    def contains(cls, value):
        return value in cls._value2member_map_

    @classmethod
    def is_request(cls, value):
        return (value == cls.RegisterReq or
            value == cls.UserListReq or
            value == cls.GetPKReq or
            value == cls.SendMessageReq or
            value == cls.GetMessagesReq)


# Used for messages between users
@unique
class MessageType(IntEnum):
    GetSK = 1
    SendSK = 2
    Text = 3
    File = 4

    # Implementing an easy search function for enums.
    @classmethod
    def contains(cls, value):
        return value in cls._value2member_map_


# ############################################ REQUESTS ############################################ #
class RequestHeader:
    format = f"<{UUID_LEN}sBHL"

    def __init__(self, bytestream):
        (self.client_id,
         self.version,
         self.code,
         self.payload_size) = struct.unpack(self.format, bytestream)

    def is_valid(self):
        if self.version > SERVER_VERSION:
            return False

        if not Opcodes.is_request(self.code):
            return False

        ''' 
            match-case was only added on later python versions.
            
            Register request and public key request have a defined
            request body and expected sizes which can be validated.
            
            User list and get messages requests have no body, thus
            a zero is expected as the payload size.
            
            Send message can have any possible length, thus
            no validation is possible.
        '''
        if self.code == Opcodes.RegisterReq:
            return self.payload_size == RegisterReqBody.get_size()

        elif self.code == Opcodes.UserListReq:
            return self.payload_size == 0

        elif self.code == Opcodes.GetPKReq:
            return self.payload_size == GetPKReqBody.get_size()

        elif self.code == Opcodes.SendMessageReq:
            return True

        elif self.code == Opcodes.GetMessagesReq:
            return self.payload_size == 0

        else:
            return False


#  OPCODE 1000
class RegisterReqBody:
    format = f"<{NAME_LEN}s{PUBLIC_KEY_LEN}s"

    def __init__(self, bytestream):
        (self.name,
         self.pk) = struct.unpack(self.format, bytestream)

    @staticmethod
    def get_size():
        return NAME_LEN + PUBLIC_KEY_LEN


#  OPCODE 1002
class GetPKReqBody:
    format = f"<{UUID_LEN}s"

    def __init__(self, bytestream):
        (self.client_id, ) = struct.unpack(self.format, bytestream)

    @staticmethod
    def get_size():
        return UUID_LEN


#  OPCODE 1003
class SendMessageReqBody:
    format = f"<{UUID_LEN}sBL"

    def __init__(self, bytestream):
        (self.client_id,
         self.message_type,
         self.content_size) = struct.unpack(self.format, bytestream[:21])
        self.content = bytestream[21:]
        self.raw = bytestream

    @staticmethod
    def get_sub_header_size():
        return UUID_LEN + 1 + 4


# ############################################ RESPONSES ############################################ #
class ResponseHeader:
    format = "<BHL"

    def __init__(self, version = SERVER_VERSION, code = Opcodes.CommunicationError, payload_size = 0):
        self.raw = struct.pack(self.format, version, code, payload_size)


#  OPCODE 2000
class RegisterResBody:
    format = f"<{UUID_LEN}s"

    def __init__(self, client_id : UUID):
        self.raw = struct.pack(self.format, client_id.bytes)


#  OPCODE 2001
class UserListResNode:
    format = f"<{UUID_LEN}s{NAME_LEN}s"

    def __init__(self, client_id : UUID, client_name = ""):
        self.raw = struct.pack(self.format, client_id.bytes, client_name)


#  OPCODE 2002
class GetPKResBody:
    format = f"<{UUID_LEN}s{PUBLIC_KEY_LEN}s"

    def __init__(self, client_id, pk):
        self.raw = struct.pack(self.format, client_id, pk)


#  OPCODE 2003
class SendMessageResBody:
    format = f"<{UUID_LEN}sL"

    def __init__(self, client_id, message_id):
        self.raw = struct.pack(self.format, client_id, message_id)


#  OPCODE 2004
class GetMessagesResBody:
    format = f"<{UUID_LEN}sLBL"

    def __init__(self, bytestream):
        (self.client_id,
         self.message_id,
         self.message_type,
         self.message_size) = struct.unpack(self.format, bytestream)

        metadata_size = len(self.client_id) + len(self.message_id) + len(self.message_type) + len(self.message_size)
        self.content = bytestream[metadata_size: metadata_size + self.message_size]
