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
// Type 1
typedef struct _EmptyMessage {
	static constexpr size_t GetSize() {
		return 0;
	}
} EmptyMessage;

// Type 2
typedef struct s {
	uint8_t encSymKey[128];

	static constexpr size_t GetSize() {
		return sizeof(encSymKey);
	}
} SendSymKeyMessage;

typedef struct _SendTextMessage {
	std::vector<uint8_t> data;

	size_t GetSize() {
		return data.size();
	}
} SendTextMessage;


class MessageHeader {
public:
	MessageHeader() : uuid{ 0 }, messageType(0), contentSize(0) { };
	MessageHeader(uint8_t type, uint32_t size) : messageType(type), contentSize(size) {};

	static constexpr size_t GetSize() {
		return sizeof(uuid) + sizeof(messageType) + sizeof(contentSize);
	}

	bool Deserialize(const std::vector<uint8_t>& inVector) {

		if (inVector.size() < MessageHeader::GetSize()) {
			return false;
		}

		memcpy((uint8_t*)&uuid, inVector.data(), sizeof(uuid));
		memcpy((uint8_t*)&messageType, inVector.data() + sizeof(uuid), sizeof(messageType));
		memcpy((uint8_t*)&contentSize, inVector.data() + sizeof(uuid) + sizeof(messageType), sizeof(contentSize));

		switch (messageType)
		{
		case (uint8_t)MessageType::GetSymKey:
			if (contentSize != EmptyMessage::GetSize()) {
				memset(uuid, 0, sizeof(uuid));
				messageType = 0;
				contentSize = 0;

				return false;
			}
			break;

		case (uint8_t)MessageType::SendSymKey:
			if (contentSize != SendSymKeyMessage::GetSize()) {
				memset(uuid, 0, sizeof(uuid));
				messageType = 0;
				contentSize = 0;

				return false;
			}
			break;

		case (uint8_t)MessageType::SendText:
			break;

		case (uint8_t)MessageType::SendFile:
			break;

		default:
			memset(uuid, 0, sizeof(uuid));
			messageType = 0;
			contentSize = 0;

			return false;
		}

		return true;
	}

	MessageType GetMessageType() {
		return (MessageType)messageType;
	}

	uuid_t uuid;
	uint8_t messageType;
	uint32_t contentSize;
};

template <MessageType _type, typename Content>
struct MessageToClient {
	MessageToClient() : messageHeader((uint8_t)_type, Content::GetSize()) {}

	MessageHeader messageHeader;
	Content content;

	static constexpr size_t GetSize() {
		return MessageHeader::GetSize() + Content::GetSize();
	}
};

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
