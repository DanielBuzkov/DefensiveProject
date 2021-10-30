#include "Client.h"
#include <iostream>
#include <fstream>
#include <string>

#include <boost/asio.hpp>

#include "SystemUtils.h"

static constexpr const char* ME_INFO_PATH = "me.info";
static constexpr const char* SERVER_INFO_PATH = "server.info";

// Four three digit numbers, and four dots for seperation.
static constexpr size_t MAX_IP_STR_LENGTH = (4 * 3) + 3;
static constexpr size_t MIN_IP_STR_LENGTH = (4 * 1) + 3;

// At most 2**16 which is 5 digits long.
static constexpr size_t MAX_PORT_STR_LENGTH = 5;
static constexpr size_t MIN_PORT_STR_LENGTH = 1;

Client::~Client() {
	if (m_privateKey != nullptr) {
		delete m_privateKey;
	}

	if (m_publicKey != nullptr) {
		delete m_publicKey;
	}
}

bool Client::Init() {

	// Parsing server info
	if (ParseServerInfo() == false) {
		return false;
	}

	if (SystemUtils::IsFileExists(ME_INFO_PATH) == false) {
		// If the personal information doesn't exists, the user hasn't
		// registered yet, so the m_init field won't be set to true.

		// The file's content shall be updated once the UUID is received from the server.
		// At that stage all the file's content is available and both the server
		// and the client are aware of him being registered.

		m_isInit = false;
		return true;
	}

	if (ParseMeInfo() == false) {
		return false;
	}

	m_isInit = true;
	return true;
}

void Client::Run(){
	
	// Keep track of menu choise handling success.
	Client::ReturnStatus ret = Client::ReturnStatus::GeneralError;

	while (true) {

		switch (GetMenuChoise())
		{
		case Client::MenuOptions::Register:
			ret = HandleRegister();
			break;

		case Client::MenuOptions::ClientList:
			ret = HandleList();
			break;

		case Client::MenuOptions::PublicKey:
			ret = HandlePublicKey();
			break;

		case Client::MenuOptions::GetMessages:
			ret = HandleWaitingMessages();
			break;

		case Client::MenuOptions::SendMessageToFriend:
			ret = HandleSendMessage();
			break;

		case Client::MenuOptions::GetSymKey:
			ret = HandleRequestSymKey();
			break;

		case Client::MenuOptions::SendSymKey:
			ret = HandleSendSymKey();
			break;

		case Client::MenuOptions::Exit:
			// Close socket and free shit
			return;

		default:
			ret = Client::ReturnStatus::GeneralError;
			break;
		}

		if (ret == Client::ReturnStatus::ServerError) {
			std::cout << "Server responded with an error" << std::endl;
		}
	}
}

//-------------------------------------------- UTILITIES --------------------------------------------

void Client::PrintOption() {
	std::cout << "10) Register" << std::endl;
	std::cout << "20) Request for client list" << std::endl;
	std::cout << "30) Request for public key" << std::endl;
	std::cout << "40) Request for waiting messages" << std::endl;
	std::cout << "50) Send a text message" << std::endl;
	std::cout << "51) Send a request for symmetric key" << std::endl;
	std::cout << "52) Send your symmetric key" << std::endl;
	std::cout << " 0) Exit client" << std::endl;
}

Client::MenuOptions Client::GetMenuChoise() {

	uint16_t userInput = 0;

	std::cout << "--------------------------------" << std::endl;
	std::cout << "MessageU client at your service." << std::endl;

	while(true) {
		PrintOption();
		std::cin >> userInput;

		// Making sure the choise is in the whitelist.
		if (userInput != (uint16_t)Client::MenuOptions::Register &&
			userInput != (uint16_t)Client::MenuOptions::ClientList &&
			userInput != (uint16_t)Client::MenuOptions::PublicKey &&
			userInput != (uint16_t)Client::MenuOptions::GetMessages &&
			userInput != (uint16_t)Client::MenuOptions::SendMessageToFriend &&
			userInput != (uint16_t)Client::MenuOptions::GetSymKey &&
			userInput != (uint16_t)Client::MenuOptions::SendSymKey &&
			userInput != (uint16_t)Client::MenuOptions::Exit) {
			// (I hate c++ and its un-iterable enums.)
			std::cout << "Invalid option, please try choosing from the menu again." << std::endl;
			continue;
		}

		// Making sure the user has made a choise which is available for his state.
		if (m_isInit == false &&
			userInput != (uint16_t)Client::MenuOptions::Register &&
			userInput != (uint16_t)Client::MenuOptions::Exit) {
			std::cout << "Must register before making this action." << std::endl;
			continue;
		}

		if (m_isInit == true &&
			userInput == (uint16_t)Client::MenuOptions::Register) {
			std::cout << "Unable to register again" << std::endl;
			continue;
		}

		break;
	}

	return static_cast<Client::MenuOptions>(userInput);
}

bool Client::ParseServerInfo() {

	if (SystemUtils::IsFileExists(SERVER_INFO_PATH) == false) {
		std::cout << "Server info file doesn't exists" << std::endl;
		return false;
	}

	auto fileSize = SystemUtils::GetFileSize(SERVER_INFO_PATH);

	// Adding one for the semicolon
	if (fileSize < (MIN_PORT_STR_LENGTH) + (MIN_IP_STR_LENGTH) + 1 ||
		fileSize > (MAX_PORT_STR_LENGTH) + (MAX_IP_STR_LENGTH) + 1) {
		std::cout << "Invalid file content length" << std::endl;
		return false;
	}

	std::fstream infoFile;
	std::string line;

	// Openning file for reading.
	infoFile.open(SERVER_INFO_PATH);
	if (infoFile.is_open() == false) {
		std::cout << "Failed openning " << SERVER_INFO_PATH << std::endl;
		return false;
	}

	if (!std::getline(infoFile, line)) {
		std::cout << "Failed reading line from " << SERVER_INFO_PATH << std::endl;
		infoFile.close();
		return false;
	}

	// The file is not needed anymore.
	infoFile.close();

	uint64_t tmp = 0;
	size_t delimiterIndex = line.find(':');

	if (delimiterIndex == std::string::npos ||
		line.length() < fileSize) {
		std::cout << "Invalid file format" << std::endl;
		return false;
	}

	// Trying to cast the values read from the file in case 'stoi' fails or more.
	try {
		m_ipAddr = line.substr(0, delimiterIndex);
		boost::asio::ip::address::from_string(m_ipAddr);

		tmp = stoi(line.substr(delimiterIndex + 1));
	}
	catch (...) {
		std::cout << "Failed to cast the content" << std::endl;
		return false;
	}

	/*
		uint64_t can handle numbers up to 20 digits long.
		The validations before made sure the file has less then 20
		charecters overall, meaning no integer overflow should
		accure here, leading to valid opperation.
	*/
	if (tmp > UINT16_MAX) {
		std::cout << "Invalid port value" << std::endl;
		return false;
	}

	m_port = tmp;

	return true;
}

bool Client::ParseMeInfo() {
	std::fstream infoFile;

	std::string inputName;
	std::string inputUuid;
	std::string inputPrivateKey;
	std::string temp;

	if (m_isInit == true) {
		std::cout << "User is already init" << std::endl;
		return false;
	}

	// Openning file for reading.
	infoFile.open(ME_INFO_PATH);
	if (infoFile.is_open() == false) {
		std::cout << "Failed openning " << ME_INFO_PATH << std::endl;
		return false;
	}

	if (!std::getline(infoFile, inputName)) {
		std::cout << "Failed reading name from " << ME_INFO_PATH << std::endl;
		infoFile.close();
		return false;
	}

	if (!std::getline(infoFile, inputUuid)) {
		std::cout << "Failed reading uuid from " << ME_INFO_PATH << std::endl;
		infoFile.close();
		return false;
	}

	if (!std::getline(infoFile, inputPrivateKey)) {
		std::cout << "Failed reading private key from " << ME_INFO_PATH << std::endl;
		infoFile.close();
		return false;
	}

	if (std::getline(infoFile, temp)) {
		std::cout << "Invalid file format" << std::endl;
		infoFile.close();
		return false;
	}

	// The file is not needed anymore.
	infoFile.close();

	if (m_name.Deserialize(inputName) == false ||
		m_uuid.FromFile(inputUuid) == false) {
		std::cout << "Failed reading UUID or name" << std::endl;
		return false;
	}

	try {
		// This badly decomanted function will throw an exception upon invalid key.
		m_privateKey = new RSAPrivateWrapper(Base64Wrapper::decode(inputPrivateKey));
	}
	catch (const std::exception&) {
		std::cout << "Invalid key" << std::endl;
		return false;
	}

	m_isInit = true;

	return true;
}

bool Client::UpdateMeInfo(uuid_t newUuid) {
	
	if (m_isInit == true) {
		std::cout << "User is already init" << std::endl;
		return false;
	}

	// Making sure the given UUID is valid and updating the field.
	if (m_uuid.Deserialize((const char*)newUuid, sizeof(uuid_t)) == false) {
		std::cout << "Invalid UUID" << std::endl;
		return false;
	}

	std::string uuidString;
	
	if (m_uuid.ToFile(uuidString) == false) {
		std::cout << "Unable to get UUID" << std::endl;
		return false;
	}

	name_t nameBuffer = { 0 };

	if (m_name.Serialize(nameBuffer, sizeof(nameBuffer)) == false) {
		std::cout << "Failed updating file" << std::endl;
		return false;
	}

	// Remove newlines from string.
	std::string encodedPrivateKey = Base64Wrapper::encode(m_privateKey->getPrivateKey());
	encodedPrivateKey.erase(std::remove(encodedPrivateKey.begin(), encodedPrivateKey.end(), '\n'), encodedPrivateKey.end());

	m_isInit = true;

	std::ofstream infoFile(ME_INFO_PATH);

	infoFile << nameBuffer << std::endl;
	infoFile << uuidString << std::endl;
	infoFile << encodedPrivateKey;

	infoFile.close();

	return true;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ DONE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv NEEDS WORK vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
bool Client::SendToServer(uint8_t* buffer, size_t buffSize) {
	return false;
}

bool Client::ChooseClientFromList(Friend* o_otherClient) {
	if (m_friends.size() == 0) {
		std::cout << "No friends found in list. Request client list from server." << std::endl;
		return false;
	}
	
	std::string name;

	std::cout << "Enter friends name : ";
	std::cin >> name;

	for (Friend currFriend : m_friends) {
		if (currFriend.IsNameEqual(name) == true) {
			o_otherClient = &currFriend;
			return true;
		}
	}

	return false;
}

//--------------------------------------------- HANDLERS ---------------------------------------------

Client::ReturnStatus Client::HandleRegister() {
	
	// Making sure a registered user is not registering again.
	if (m_isInit == true) {
		std::cout << "Error: Client is already registered." << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	std::string name;

	std::cout << "Insert your name: ";
	std::cin >> name;

	if (m_name.Deserialize(name) == false) {
		std::cout << "Invalid name" << std::endl;
		return ReturnStatus::GeneralError;
	}

	RequestRegister request;
	ResponseRegister response;

	if (m_name.Serialize(request.body.name, sizeof(request.body.name)) == false) {
		std::cout << "Failed serializing name to message" << std::endl;
		m_name.Reset();
		return ReturnStatus::GeneralError;
	}

	// Generating the key pair for RSA
	m_privateKey = new RSAPrivateWrapper();
	m_publicKey = new RSAPublicWrapper(m_privateKey->getPublicKey());

	m_publicKey->getPublicKey((char*)request.body.publicKey, sizeof(request.body.publicKey));
	
	// Sending request and waiting for response.
	ReturnStatus ret = Exchange(request, response);

	if (ret == ReturnStatus::Success) {
		// Update me.info with all new values.
		if (UpdateMeInfo(response.body.uuid) == false) {
			// Making sure no traces left after failure.
			delete m_publicKey;
			delete m_privateKey;

			m_publicKey = nullptr;
			m_privateKey = nullptr;

			m_name.Reset();

			std::cout << "Failed updating UUID from server" << std::endl;
			return ReturnStatus::GeneralError;
		}

		m_isInit = true;
	}
	else {
		// Making sure no traces left after failure.
		delete m_publicKey;
		delete m_privateKey;

		m_publicKey = nullptr;
		m_privateKey = nullptr;

		m_name.Reset();
	}

	return ret;
}

Client::ReturnStatus Client::HandleList() {
	RequestList request(m_uuid);

	return Client::ReturnStatus::GeneralError;
}

Client::ReturnStatus Client::HandlePublicKey() {
	Friend *otherClient = nullptr;
	
	// Determine which client's public key is wanted.
	if (ChooseClientFromList(otherClient) == false) {
		return Client::ReturnStatus::GeneralError;
	}

	RequestPK request;
	ResponsePK response;
	
	if (otherClient->GetUuid(request.body.uuid) == false) {
		return Client::ReturnStatus::GeneralError;
	}

	ReturnStatus ret = Exchange(request, response);

	if (ret == ReturnStatus::Success) {
		otherClient->SetPublicKey(response.body.publicKey, sizeof(response.body.publicKey));
	}

	return ret;
}

Client::ReturnStatus Client::HandleRequestSymKey() {
	Friend* otherClient = nullptr;

	// Determine from which client request a sym key.
	if (ChooseClientFromList(otherClient) == false) {
		return Client::ReturnStatus::GeneralError;
	}

	RequestGetSymKey request(m_uuid);
	ResponseSendMessage response;

	if (otherClient->GetUuid(request.body.uuid) == false) {
		return Client::ReturnStatus::GeneralError;
	}

	return Exchange(request, response);
}

Client::ReturnStatus Client::HandleSendSymKey() {
	Friend* otherClient = nullptr;

	// Determine to which client send a sym key.
	if (ChooseClientFromList(otherClient) == false) {
		return Client::ReturnStatus::GeneralError;
	}

	if (otherClient->HasSym() == false) {
		std::cout << "No symmetric key for this user" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	RequestSendSymKey request(m_uuid);
	ResponseSendMessage response;

	if (otherClient->GetUuid(request.body.uuid) == false) {
		return Client::ReturnStatus::GeneralError;
	}

	memcpy(request.body.content.symKey, otherClient->GetSymKey()->getKey(), sizeof(request.body.content.symKey));

	return Exchange(request, response);
}

Client::ReturnStatus Client::HandleSendMessage() {
	return Client::ReturnStatus::GeneralError;
}

Client::ReturnStatus Client::HandleWaitingMessages() {
	RequestGetMessages request(m_uuid);
	uint8_t buffer[sizeof(request)];

	request.Serialize(buffer, sizeof(buffer));

	if (SendToServer(buffer, sizeof(buffer)) == false) {
		return Client::ReturnStatus::GeneralError;
	}

	return Client::ReturnStatus::Success;
}
