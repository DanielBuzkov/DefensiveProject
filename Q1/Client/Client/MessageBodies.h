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

// TODO : ADD MORE MESSAGE TYPES (dynamic ones)

// REQUESTS:

// Opcode 1000
typedef struct _RequestRegisterBody {
	name_t name;
	publicKey_t publicKey;

	static constexpr size_t GetSize() {
		return sizeof(RequestRegisterBody);
	}

} RequestRegisterBody;

// Opcode 1002
typedef struct _RequestPKBody {
	uuid_t uuid;

	static constexpr size_t GetSize() {
		return sizeof(RequestPKBody);
	}
} RequestPKBody;

// Opcode 1003
template <MessageType _type, typename Content>
struct MessageToClient {
	MessageToClient() : messageType((uint8_t)_type), contentSize(Content::GetSize()) {}

	uuid_t uuid;
	uint8_t messageType;
	uint32_t contentSize;
	Content content;

	static constexpr size_t GetSize() {
		return sizeof(uuid) + sizeof(messageType) + sizeof(contentSize) + Content::GetSize();
	}
};

// Type 1
typedef struct _EmptyMessage {
	static constexpr size_t GetSize() {
		return 0;
	}
} EmptyMessage;

// Type 2
typedef struct _SendSymKeyMessage {
	symkey_t symKey;

	static constexpr size_t GetSize() {
		return sizeof(symKey);
	}
} SendSymKeyMessage;

typedef MessageToClient<MessageType::GetSymKey, EmptyMessage> RequestGetSymKeyBody;
typedef MessageToClient<MessageType::SendSymKey, SendSymKeyMessage> RequestSendSymKeyBody;
//MORE
//MORE

// Opcodes 1001, 1004, 9000
typedef struct _EmptyBody {
	static constexpr size_t GetSize() {
		return 0;
	}
} EmptyBody;

// RESPONSES:

// Opcode 2000
typedef struct _ResponseRegisterBody {
	uuid_t uuid;

	static constexpr size_t GetSize() {
		return sizeof(ResponseRegisterBody);
	}

} ResponseRegisterBody;

// Opcode 2001
typedef struct _ResponseUsersListNode {
	uuid_t uuid;
	name_t name;

	static constexpr size_t GetSize() {
		return sizeof(uuid) + sizeof(name);
	}

} ResponseUsersListNode;

// Opcode 2002
typedef struct _ResponsePKBody {
	uuid_t uuid;
	publicKey_t publicKey;

	static constexpr size_t GetSize() {
		return sizeof(ResponsePKBody);
	}

} ResponsePKBody;

// Opcode 2003
typedef struct _ResponseSendMessageBody {
	uuid_t uuid;
	uint32_t messageId;

	static constexpr size_t GetSize() {
		return sizeof(ResponseSendMessageBody);
	}

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
