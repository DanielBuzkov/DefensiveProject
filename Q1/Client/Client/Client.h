#pragma once

#include <string>
#include <unordered_map>

#include <boost/asio.hpp>

#include "Protocol.h"
#include "Friend.h"
#include "RSAWrapper.h"
#include "AESWrapper.h"
#include "Base64Wrapper.h"

class Client {
public:
	Client();
	~Client();

	/**
		This function parses the user's info file to find the name, UUID and PK.
		In case no file exists, the function won't fail, but will set the instance as uninitialized.
		
		Since the file's content is given by the server, assuming it is valid.
		In case any value is invalid, no connection would be possible to establish,
		and any threat stays localy on the endpoint (similarly to CA cretificate saved localy).
		
		@return	bool	-	True if client info exists and valid, or not set yet. False otherwise.
	*/
	bool Init();

	/**
		This function is the main handler of the client.
		It has an infint loop which reads the user's choise and communicates with the server
		as requested.
	*/
	void Run();

private:
	// All possible choises from the menu.
	enum class MenuOptions {
		Register = 10,
		ClientList = 20,
		PublicKey = 30,
		GetMessages = 40,
		SendMessageToFriend = 50,
		GetSymKey = 51,
		SendSymKey = 52,
		Exit = 0,
	};

	// Inner enum to keep track of the handling status.
	enum class ReturnStatus {
		Success,
		ServerError,
		GeneralError
	};

private:
	/**
		Prints the entire menu for the user.
	*/
	static void PrintOption();

	/**
		The function prints the menu to the user and gets a choise from him.
		A valid choise is guarenteed since the function won't return until
		a valid option is entered.

		It also keeps in mind the state of the client, meaning an unregister user
		won't be able to choose any option other than register.

		@return MenuOptions	-	The coresposing enumerate to keep handling the request.
	*/
	MenuOptions GetMenuChoise() const;

	/**
		The function parses the 'server.info' file by the format:
			ip_address:port
		e.g. "127.0.0.1:1234"

		@return bool	-	True upon success, false otherwise (file not found / wrong format / etc)
	*/
	bool ParseServerInfo();

	/**
		This function parses the content of `me.info` file and updates the instances fields.

		@return	bool	-	True if file's content is valid and the fields successfully update, false otherwise.
	*/
	bool ParseMeInfo();

	/**
		This function updates the me.info from the current fields.
		It should be called after the registration request.

		@return	bool	-	True if file updated, false otherwise.
	*/
	bool UpdateMeInfo(uuid_t newUuid);

	/**
		A template function for sending a request and receiving a response from the server.
		At this implementation the request and the response are pre-defined at compile time.

		@param	request		-	A static request to be sent.
		@param	response	-	A static response which will be filled by the received data.

		@return	ReturnStatus	-	ServerError if the response is a valid error message from the server.
								-	Success if the request has been handled successfuly.
								-	GeneralError otherwise.
	*/
	template<Opcode _reqCode, typename ReqBody, Opcode _resCode, typename ResBody>
	ReturnStatus Exchange(const StaticRequest<_reqCode, ReqBody> request, StaticResponse<_resCode, ResBody>& response) const;

	/**
		A template function for sending a request and receiving a response with unknown length from the server.
		At this implementation the request is pre-defined at compile time, but the response shall be
		interpreted as it is being read.

		@param	request		-	A static request to be sent.
		@param	responseVec	-	A vector of the received data from the server.

		@return	ReturnStatus	-	ServerError if the response is a valid error message from the server.
								-	Success if the request has been handled successfuly.
								-	GeneralError otherwise.
	*/
	template<Opcode _reqCode, typename ReqBody>
	ReturnStatus Exchange(const StaticRequest<_reqCode, ReqBody> request, std::vector<uint8_t>& responseVec) const;

	/**
		A template function for sending a request of unknown length and receiving a response from the server.
		At this implementation the response is pre-defined at compile time, but the request shall be
		interpreted as it is being written.

		@param	requestVec	-	A vector of the sent data to the server.
		@param	response	-	A static response which will be filled by the received data.

		@return	ReturnStatus	-	ServerError if the response is a valid error message from the server.
								-	Success if the request has been handled successfuly.
								-	GeneralError otherwise.
	*/
	template<Opcode _resCode, typename ResBody>
	ReturnStatus Exchange(const std::vector<uint8_t>& requestVec, StaticResponse<_resCode, ResBody>& response) const;

	/**
		A template function for sending a request of unknown length and receiving a response 
		with unknown length from the server.
		At this implementation the request and the response shall be interpreted at run time.

		@param	requestVec	-	A vector of the sent data to the server.
		@param	responseVec	-	A vector of the received data from the server.

		@return	ReturnStatus	-	ServerError if the response is a valid error message from the server.
								-	Success if the request has been handled successfuly.
								-	GeneralError otherwise.
	*/
	ReturnStatus Exchange(const std::vector<uint8_t>& requestVec, std::vector<uint8_t>& responseVec) const;

	/**
		The map which holds all the other clients' relevant data is an unordered map
		with the name as the key.
		Thus this funtcion helps for the otherway around when needed to get the name 
		from a given UUID.

		@param	uuid	-	The UUID of the user whose name we want.

		@return	string	-	The name if succesful, empty string otherwise.
	*/
	std::string GetNameFromUuid(const uuid_t &uuid);

	/*
		Each of these functions implements a single option from the menu.
		Each of them returns Client::ReturnStatus :
			
			Client::ReturnStatus::Success		-	Successfuly handled.
			Client::ReturnStatus::ServerError	-	Received the error response from the server (Opcode::ResponseFailure).
			Client::ReturnStatus::GeneralError	-	Some error accured while sending or receiving.
	*/
	ReturnStatus HandleRegister();
	ReturnStatus HandleList();
	ReturnStatus HandlePublicKey();
	ReturnStatus HandleWaitingMessages();
	ReturnStatus HandleSendMessage();
	ReturnStatus HandleRequestSymKey();
	ReturnStatus HandleSendSymKey();

private:
	// To keep track if the client has been refistered or not.
	bool m_isInit;

	// Connection to server info.
	std::string m_ipAddr;
	uint16_t m_port;

	// The client uses names as identifiers.
	std::unordered_map<std::string, Friend*> m_data;

	RSAPrivateWrapper *m_privateKey;

	// Client's name, UUID and given public key.
	Name m_name;
	UUID m_uuid;
};

template<Opcode _reqCode, typename ReqBody, Opcode _resCode, typename ResBody>
Client::ReturnStatus Client ::Exchange(const StaticRequest<_reqCode, ReqBody> request, StaticResponse<_resCode, ResBody>& response) const {
	std::vector<uint8_t> requestVec;
	request.Serialize(requestVec);

	return Exchange(requestVec, response);
}

template<Opcode _reqCode, typename ReqBody>
Client::ReturnStatus Client::Exchange(const StaticRequest<_reqCode, ReqBody> request, std::vector<uint8_t>& responseVec) const {
	std::vector<uint8_t> requestVec;
	request.Serialize(requestVec);

	return Exchange(requestVec, responseVec);
}

template<Opcode _resCode, typename ResBody>
Client::ReturnStatus Client::Exchange(const std::vector<uint8_t>& requestVec, StaticResponse<_resCode, ResBody>& response) const {

	std::vector<uint8_t> responseVec;

	switch (Exchange(requestVec, responseVec)) {
	case Client::ReturnStatus::ServerError:
		return Client::ReturnStatus::ServerError;

	case Client::ReturnStatus::GeneralError:
		return Client::ReturnStatus::GeneralError;

	case Client::ReturnStatus::Success:
		// Message is not failure, try to get full message.
		if (response.Desrialize(responseVec) == false) {
			return Client::ReturnStatus::GeneralError;
		}

		return Client::ReturnStatus::Success;

	default:
		return Client::ReturnStatus::GeneralError;
	}

	return Client::ReturnStatus::Success;
}
