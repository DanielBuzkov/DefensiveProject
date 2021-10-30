#pragma once

#include <stdexcept>

#include "Defines.h"
#include "Validators.h"
#include "MessageBodies.h"

/*
	This file includes all relevant structs and types relevant for
	data handling for the client side.
	All messages which are sent and received through the socket
	shall be fit to the given protocl, otherwise it will be discarded.
*/

// Opcode 
enum class Opcode : uint16_t {
	RequestRegister = 1000,
	RequestList = 1001,
	RequestPK = 1002,
	RequestSendMessage = 1003,
	RequestGetMessages = 1004,

	ResponseRegister = 2000,
	ResponseList = 2001,
	ResponsePK = 2002,
	ResponseSendMessage = 2003,
	ResponseGetMessage = 2004,

	ResponseFailure = 9000
};

#pragma pack(push, 1)

/**
	All functions were implemented while being aware of the data usage.
	Meaning taking into account that requests won't be received on the client side,
	thus don't have to implement a deserialize function, or responses shall be 
	only received and thus only the default c'tor shall be used.
*/

template<Opcode _code, size_t _size>
class RequestHeader {
public:
	RequestHeader(UUID _uuid) : version(CLIENT_VERSION), code((uint16_t)_code), payloadSize((uint32_t)_size) {
		if (_uuid.Serialize(clientId, sizeof(clientId)) == false) {
			throw std::invalid_argument("Unable to handle currnt UUID for messages");
		}
	}
	
	RequestHeader() : clientId{ 0 }, version(CLIENT_VERSION), code((uint16_t)_code), payloadSize(_size) {}

	uuid_t clientId;
	uint8_t version;
	uint16_t code;
	uint32_t payloadSize;

	const bool Serialize(uint8_t* o_buffer, size_t buffSize) const {
		if (buffSize != sizeof(RequestHeader)) {
			return false;
		}

		memcpy(o_buffer, clientId, sizeof(clientId));
		memcpy(o_buffer + sizeof(clientId), &version, sizeof(version));
		memcpy(o_buffer + sizeof(clientId) + sizeof(version), &code, sizeof(code));
		memcpy(o_buffer + sizeof(clientId) + sizeof(version) + sizeof(code), &payloadSize, sizeof(payloadSize));

		return true;
	}
};

template<Opcode _code, size_t _size>
class ResponseHeader {
public:
	ResponseHeader() : version(0), code(0), payloadSize(0) {}

	uint8_t version;
	uint16_t code;
	uint32_t payloadSize;

	bool Deserialize(uint8_t* o_buffer, size_t buffSize) {
		if (buffSize != sizeof(ResponseHeader)) {
			return false;
		}

		memcpy((uint8_t*)&version, o_buffer, sizeof(version));
		memcpy((uint8_t*)&code, o_buffer + sizeof(version), sizeof(code));
		memcpy((uint8_t*)&payloadSize, o_buffer + sizeof(version) + sizeof(code), sizeof(payloadSize));

		// Failure header shall still be accepted
		if (code == (uint16_t)Opcode::ResponseFailure) {
			return true;
		}

		// Server's version should not bother the client
		if (code != (uint16_t)_code ||
			payloadSize != (uint32_t)_size) {
			
			version = 0;
			code = 0;
			payloadSize = 0;

			return false;
		}

		return true;
	}
};

/**
	StaticRequest and StaticResponse are made for messages with constant structure.
	Meaning no dynamic sizes, allowing compile time validation.
*/
template<Opcode _code, typename Body>
class StaticRequest {
public:
	StaticRequest(UUID _uuid) : header(_uuid) {}
	StaticRequest() : header() {}

	RequestHeader<_code, Body::GetSize()> header;
	Body body;

	const bool Serialize(uint8_t* o_buffer, size_t buffSize) const {
		if (buffSize != sizeof(header) + sizeof(Body)) {
			return false;
		}

		if (header.Serialize(o_buffer, sizeof(header)) == false) {
			return false;
		}

		memcpy(o_buffer + sizeof(header), &body, sizeof(body));

		return true;
	}

};

template<Opcode _code, typename Body>
class StaticResponse {
public:
	ResponseHeader<_code, Body::GetSize()> header;
	Body body;

	bool Desrialize(uint8_t* o_buffer, size_t buffSize) {
		if (buffSize != sizeof(header) + Body::GetSize()) {
			return false;
		}

		if (header.Deserialize(o_buffer, sizeof(header)) == false) {
			return false;
		}

		memcpy(&body, o_buffer + sizeof(header), Body::GetSize());

		return true;
	}
};

#pragma pack(pop)

typedef StaticRequest<Opcode::RequestRegister, RequestRegisterBody> RequestRegister;
typedef StaticRequest<Opcode::RequestList, EmptyBody> RequestList;
typedef StaticRequest<Opcode::RequestPK, RequestPKBody> RequestPK;
typedef StaticRequest<Opcode::RequestSendMessage, RequestGetSymKeyBody> RequestGetSymKey;
typedef StaticRequest<Opcode::RequestSendMessage, RequestSendSymKeyBody> RequestSendSymKey;
//MORE
//MORE
typedef StaticRequest<Opcode::RequestGetMessages, EmptyBody> RequestGetMessages;

typedef StaticResponse<Opcode::ResponseRegister, ResponseRegisterBody> ResponseRegister;
// typedef StaticResponse<Opcode::ResponseList, ResponseListBody> ResponseList;
typedef StaticResponse<Opcode::ResponsePK, ResponsePKBody> ResponsePK;
typedef StaticResponse<Opcode::ResponseSendMessage, ResponseSendMessageBody> ResponseSendMessage;
// typedef StaticResponse<Opcode::ResponseGetMessage, ResponseGetMessageBody> ResponseGetMessage;
typedef StaticResponse<Opcode::ResponseFailure, EmptyBody> ResponseFailure;
