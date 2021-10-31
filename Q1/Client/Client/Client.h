#pragma once

#include <list>
#include <string>
#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "Protocol.h"
#include "Friend.h"
#include "RSAWrapper.h"
#include "AESWrapper.h"
#include "Base64Wrapper.h"

class Client {
public:
	Client() : m_isInit(false), m_port(0), m_privateKey(nullptr), m_publicKey(nullptr) {}
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

	void Run();

private:
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

	enum class ReturnStatus {
		Success,
		ServerError,
		GeneralError
	};

private:
	/**
		The function prints the menu to the user and gets a choise from him.
		A valid choise is guarenteed since the function won't return until
		a valid option is entered.

		It also keeps in mind the state of the client, meaning an unregister user
		won't be able to choose any option other than register.

		@return MenuOptions	-	The coresposing enumerate to keep handling the request.
	*/
	MenuOptions GetMenuChoise();

	/**
		Prints the entire menu for the user.
	*/
	static void PrintOption();

	/**
		The function parses the 'server.info' file by the format:
			ip_address:port
		e.g. "127.0.0.1:1234"

		@return bool	-	True upon success, false otherwise (file not found / wrong format / etc)
	*/
	bool ParseServerInfo();

	bool ParseMeInfo();

	template<Opcode _reqCode, typename ReqBody, Opcode _resCode, typename ResBody>
	ReturnStatus Exchange(
		const StaticRequest<_reqCode, ReqBody> request, 
		StaticResponse<_resCode, ResBody>& response);

	bool UpdateMeInfo(uuid_t newUuid);

	bool ChooseClientFromList(Friend* o_otherClient);

	bool FriendExists(std::string name);

	ReturnStatus HandleRegister();
	ReturnStatus HandleList();
	ReturnStatus HandlePublicKey();

	ReturnStatus HandleRequestSymKey();
	ReturnStatus HandleSendSymKey();
	ReturnStatus HandleSendMessage();

	ReturnStatus HandleWaitingMessages();

private:
	// To keep track if the client has been refistered or not.
	bool m_isInit;

	// Connection to server
	std::string m_ipAddr;
	uint16_t m_port;

	// Save all clients to communicate with
	std::list<Friend> m_friends;

	RSAPrivateWrapper *m_privateKey;
	RSAPublicWrapper *m_publicKey;

	// Client's name, UUID and given public key
	Name m_name;
	UUID m_uuid;
};

template<Opcode _reqCode, typename ReqBody, Opcode _resCode, typename ResBody>
Client::ReturnStatus Client::Exchange(
	const StaticRequest<_reqCode, ReqBody> request, 
	StaticResponse<_resCode, ResBody>& response
) {
	uint8_t requestBuf[sizeof(request)] = { 0 };
	uint8_t responseBuf[sizeof(response)] = { 0 };

	if (request.Serialize(requestBuf, sizeof(requestBuf)) == false) {
		return Client::ReturnStatus::GeneralError;
	}

	try {
		boost::asio::io_context io_context;
		boost::asio::ip::tcp::socket socket(io_context);

		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_ipAddr), m_port);

		socket.connect(endpoint);

		socket.write_some(boost::asio::buffer(requestBuf, sizeof(requestBuf)));
		socket.read_some(boost::asio::buffer(responseBuf, sizeof(responseBuf)));
		
		socket.close();

		// Deserialize only header to check opcode
		if (response.header.Deserialize(responseBuf, sizeof(response.header)) == false) {
			return Client::ReturnStatus::GeneralError;
		}

		if (response.header.code == (uint16_t)Opcode::ResponseFailure) {
			return Client::ReturnStatus::ServerError;
		}

		// Message is not failure, try to get full message.
		if (response.Desrialize(responseBuf, sizeof(responseBuf)) == false) {
			return Client::ReturnStatus::GeneralError;
		}
	}
	catch (std::exception& e) {

		std::cerr << "[ERROR] " << e.what() << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	return Client::ReturnStatus::Success;
}

