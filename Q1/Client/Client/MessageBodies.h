#pragma once

#include "Defines.h"
#include "Validators.h"

enum class MessageType : uint8_t {
	GetSymKey = 1,
	SendSymKey = 2,
	SendText = 3,
	SendFile = 4
};

#pragma pack(push, 1)

// MESSAGES BETWEEN CLIENTS:
// Type 1
typedef struct {
} EmptyMessage;

// Type 2
typedef struct {
	symkey_t symKey;
} SendSymKeyMessage;

// TODO : ADD MORE MESSAGE TYPES (dynamic ones)

// REQUESTS:

// Opcode 1000
typedef struct {
	name_t name;
	publicKey_t publicKey;

} RequestRegisterBody;

// Opcode 1002
typedef struct {
	uuid_t uuid;
} RequestPKBody;

// Opcode 1003
template <MessageType _type, size_t _size, typename Content>
struct MessageToClient {
	MessageToClient() : messageType((uint8_t)_type), contentSize(_size) {}

	uuid_t uuid;
	uint8_t messageType;
	uint32_t contentSize;
	Content content;
};

typedef MessageToClient<MessageType::GetSymKey, 0, EmptyMessage> RequestGetSymKeyBody;
typedef MessageToClient<MessageType::SendSymKey, sizeof(SendSymKeyMessage), SendSymKeyMessage> RequestSendSymKeyBody;
//MORE
//MORE

// Opcodes 1001, 1004, 9000
typedef struct {
} EmptyBody;

// RESPONSES:

// Opcode 2000
typedef struct {
	uuid_t uuid;
} ResponseRegisterBody;

// Opcode 2001
/*typedef struct {
	uuid_t uuid;
	name_t name;
} ;*/

// Opcode 2002
typedef struct {
	uuid_t uuid;
	publicKey_t publicKey;
} ResponsePKBody;

// Opcode 2003
typedef struct {
	uuid_t uuid;
	uint32_t messageId;
} ResponseSendMessageBody;

// Opcode 2004
/*typedef struct {
	uuid_t uuid;
	uint32_t messageId;
	uint8_t messageType;
	uint32_t messageSize;
	//content itself
} ;*/

#pragma pack(pop)
