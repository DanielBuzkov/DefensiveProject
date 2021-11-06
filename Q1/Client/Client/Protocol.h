#pragma once

#include <stdexcept>
#include <vector>

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

class BaseRequestHeader {

};


template<Opcode _code, size_t _size>
class RequestHeader {
public:
	RequestHeader() : clientId{ 0 }, version(CLIENT_VERSION), code((uint16_t)_code), payloadSize((uint32_t)_size) {}
	RequestHeader(UUID _uuid) : version(CLIENT_VERSION), code((uint16_t)_code), payloadSize((uint32_t)_size) {

		if (_uuid.Serialize(clientId, sizeof(clientId)) == false) {
			throw std::invalid_argument("Unable to handle currnt UUID for messages");
		}
	}

	const void Serialize(std::vector<uint8_t>& o_vector) const {
		o_vector.clear();
		o_vector.resize(sizeof(RequestHeader));

		memcpy(&o_vector[0], this, sizeof(RequestHeader));
	}

private:
	uuid_t clientId;
	uint8_t version;
	uint16_t code;
	uint32_t payloadSize;
};

class BaseResponseHeader {
public:

	BaseResponseHeader() : version(0), code(0), payloadSize(0) {}

	Opcode GetCode() { return (Opcode)code; }
	uint32_t GetPayloadSize() { return payloadSize; }

	bool Deserialize(const std::vector<uint8_t>& inVector) {
		memcpy((uint8_t*)&version, inVector.data(), sizeof(version));
		memcpy((uint8_t*)&code, inVector.data() + sizeof(version), sizeof(code));
		memcpy((uint8_t*)&payloadSize, inVector.data() + sizeof(version) + sizeof(code), sizeof(payloadSize));

		// Failure header shall still be accepted
		if (code == (uint16_t)Opcode::ResponseFailure) {
			return payloadSize == 0;
		}

		// Validating only opcode
		if (code != (uint16_t)Opcode::ResponseRegister &&
			code != (uint16_t)Opcode::ResponseList &&
			code != (uint16_t)Opcode::ResponsePK &&
			code != (uint16_t)Opcode::ResponseSendMessage &&
			code != (uint16_t)Opcode::ResponseGetMessage &&
			code != (uint16_t)Opcode::ResponseFailure) {

			version = 0;
			code = 0;
			payloadSize = 0;

			return false;
		}

		return true;
	}

protected:
	uint8_t version;
	uint16_t code;
	uint32_t payloadSize;
};

template<Opcode _code, size_t _size>
class ResponseHeader : public BaseResponseHeader {
public:

	bool Deserialize(const std::vector<uint8_t>& inVector) {

		if (inVector.size() < sizeof(version) + sizeof(code) + sizeof(payloadSize)) {
			return false;
		}

		memcpy((uint8_t*)&version, inVector.data(), sizeof(version));
		memcpy((uint8_t*)&code, inVector.data() + sizeof(version), sizeof(code));
		memcpy((uint8_t*)&payloadSize, inVector.data() + sizeof(version) + sizeof(code), sizeof(payloadSize));

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

	const void Serialize(std::vector<uint8_t>& o_vector) const {
		o_vector.resize(sizeof(header));
		header.Serialize(o_vector);

		if (Body::GetSize() > 0) {
			std::vector<uint8_t> payload(Body::GetSize());
			memcpy(&payload[0], &body, Body::GetSize());

			o_vector.insert(o_vector.end(), payload.begin(), payload.end());
		}
	}

};

template<Opcode _code, typename Body>
class StaticResponse {
public:
	ResponseHeader<_code, Body::GetSize()> header;
	Body body;

	bool Desrialize(const std::vector<uint8_t>& inVector) {

		if (header.Deserialize(inVector) != true) {
			return false;
		}

		if (header.GetCode() == Opcode::ResponseFailure) {
			return inVector.size() == sizeof(header);
		}

		if (inVector.size() != sizeof(header) + Body::GetSize()) {
			return false;
		}

		std::vector<uint8_t> payload(inVector.begin() + sizeof(header), inVector.end());
		memcpy((uint8_t*)&body, payload.data(), sizeof(body));

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
