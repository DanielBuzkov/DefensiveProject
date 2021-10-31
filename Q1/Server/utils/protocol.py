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


# Used for messages between users
@unique
class MessageType(IntEnum):
    GetSK = 1
    SendSK = 2
    Text = 3
    File = 4


# ############################################ REQUESTS ############################################ #
class RequestHeader:
    format = f"<{UUID_LEN}sBHL"

    def __init__(self, bytestream):
        (self.client_id,
         self.version,
         self.code,
         self.payload_size) = struct.unpack(self.format, bytestream)


#  OPCODE 1000
class RegisterReqBody:
    format = f"<{NAME_LEN}s{PUBLIC_KEY_LEN}s"

    def __init__(self, bytestream):
        (self.name,
         self.pk) = struct.unpack(self.format, bytestream)


#  OPCODE 1002
class GetPKReqBody:
    format = f"<{UUID_LEN}s"

    def __init__(self, bytestream):
        (self.client_id) = struct.unpack(self.format, bytestream)


#  OPCODE 1003
class SendMessageReqBody:
    format = f"<{UUID_LEN}sBL"

    def __init__(self, bytestream):
        (self.client_id,
         self.message_type,
         self.content_size) = struct.unpack(self.format, bytestream)
        self.content = bytestream[-self.content_size:]


# ############################################ RESPONSES ############################################ #
class ResponseHeader:
    format = "<BHL"

    def __init__(self, version = SERVER_VERSION, code = Opcodes.CommunicationError, payload_size = 0):
        self.raw = struct.pack(self.format, version, code, payload_size)


#  OPCODE 2000
class RegisterResBody:
    format = "<16s"

    def __init__(self, client_id : UUID):
        self.raw = struct.pack(self.format, client_id.bytes)


#  OPCODE 2001
class UserListResNode:
    format = "<16s255s"

    def __init__(self, client_id : UUID, client_name = ""):
        self.raw = struct.pack(self.format, client_id.bytes, client_name)


#  OPCODE 2002
class GetPKResBody:
    format = "<16s160s"

    def __init__(self, client_id, pk):
        self.raw = struct.pack(self.format, client_id, pk)


#  OPCODE 2003
# class SendMessageResBody:
#     format = "<16sL"
#
#     def __init__(self, bytestream):
#         self.raw = struct.pack(self.format, client_id, pk)
#         (self.client_id,
#          self.message_id) = struct.unpack(self.format, bytestream)


#  OPCODE 2004
class GetMessagesResBody:
    format = "<16sLBL"

    def __init__(self, bytestream):
        (self.client_id,
         self.message_id,
         self.message_type,
         self.message_size) = struct.unpack(self.format, bytestream)

        metadata_size = len(self.client_id) + len(self.message_id) + len(self.message_type) + len(self.message_size)
        self.content = bytestream[metadata_size: metadata_size + self.message_size]
