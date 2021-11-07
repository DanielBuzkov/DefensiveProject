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

static constexpr size_t MAX_MESSAGE_LENGTH = 8192;

Client::Client() : m_isInit(false), m_port(0), m_privateKey(nullptr) {}

Client::~Client() {
	if (m_privateKey != nullptr) {
		delete m_privateKey;
	}

	for (auto& currTuple : m_data) {
		delete currTuple.second;
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
		else if (ret == Client::ReturnStatus::GeneralError) {
			std::cout << "Error while handling request" << std::endl;
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
		std::cin >> userInput; //*!

		// Handling failed input and clearing the error flag and buffer.
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

			std::cout << "Bad entry, please enter a number from the list." << std::endl;
			continue;
		}

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

	m_port = (uint16_t)tmp;

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

std::string Client::GetNameFromUuid(const uuid_t &uuid) {
	for (const auto& currTuple : m_data) {
		Friend* currFirend = currTuple.second;

		if (currFirend->IsUuidEqual(uuid) == true) {
			return currFirend->GetName();
		}
	}

	return "";
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
		std::cout << "Invalid name. Name should contain only alphabetic charecters." << std::endl;
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
	m_privateKey->getPublicKey((char*)request.body.publicKey, sizeof(request.body.publicKey));

	// Sending request and waiting for response.
	ReturnStatus ret = Exchange(request, response);

	if (ret == ReturnStatus::Success) {
		// Update me.info with all new values.
		if (UpdateMeInfo(response.body.uuid) == false) {
			// Making sure no traces left after failure.
			delete m_privateKey;
			m_privateKey = nullptr;

			m_name.Reset();

			std::cout << "Failed updating UUID from server" << std::endl;
			return ReturnStatus::GeneralError;
		}

		m_isInit = true;
	}
	else {
		// Making sure no traces left after failure.
		delete m_privateKey;
		m_privateKey = nullptr;

		m_name.Reset();
	}

	return ret;
}

Client::ReturnStatus Client::HandleList() {
	RequestList request(m_uuid);
	std::vector<uint8_t> responseVec;

	// Sending request and waiting for response.
	Client::ReturnStatus ret = Exchange(request, responseVec);
	if (ret != Client::ReturnStatus::Success) {
		return ret;
	}

	// The exchange function has aleady validated the data is deserializeable and the lengths match.
	auto payloadSize = responseVec.size() - sizeof(BaseResponseHeader);

	if (payloadSize % ResponseUsersListNode::GetSize() != 0) {
		return Client::ReturnStatus::GeneralError;
	}

	auto numerOfNodes = payloadSize / ResponseUsersListNode::GetSize();

	for (auto i = 0; i < numerOfNodes; i++) {
		ResponseUsersListNode currNode;

		memcpy(&currNode, responseVec.data() + sizeof(BaseResponseHeader) + (i * ResponseUsersListNode::GetSize()), ResponseUsersListNode::GetSize());
		std::string currName((char*)currNode.name);

		std::cout << currName << std::endl;

		// No need handling this friend, since names are also unique.
		if (m_data.find(currName) != m_data.end()) {
			continue;
		}

		// Adding the new friend to the list.
		Friend* currFriend = new Friend();

		// Keep getting the other clients.
		if (currFriend->Init(currName, currNode.uuid) != true) {
			delete currFriend;
			continue;
		}

		m_data[currName] = currFriend;
	}
	
	return Client::ReturnStatus::Success;
}

Client::ReturnStatus Client::HandlePublicKey() {
	RequestPK request(m_uuid);
	ResponsePK response;
	
	std::string name;

	std::cout << "Insert destenation name: ";
	std::cin >> name;

	if (m_data.find(name) == m_data.end()) {
		std::cout << "Name not found" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	if (m_data[name]->GetUuid(request.body.uuid) == false) {

		std::cout << "Failed getting UUID for user" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	ReturnStatus ret = Exchange(request, response);

	if (ret == ReturnStatus::Success) {
		m_data[name]->SetPublicKey(response.body.publicKey);
	}

	return ret;
}

Client::ReturnStatus Client::HandleWaitingMessages() {

	RequestGetMessages request(m_uuid);
	std::vector<uint8_t> responseVec;

	// Sending request and waiting for response.
	Client::ReturnStatus ret = Exchange(request, responseVec);

	if (ret != Client::ReturnStatus::Success) {
		return ret;
	}

	// The header has been validated at the exchange function.
	std::vector<uint8_t> payload(responseVec.begin() + sizeof(BaseResponseHeader), responseVec.end());

	// The exchange function has aleady validated the data is deserializeable and the lengths match.
	auto bytesLeft = payload.size();
	auto bytesRead = 0;

	while (bytesLeft > 0) {

		if (bytesLeft < MessageHeader::GetSize()) {
			std::cout << "Reached an invalid tail length" << std::endl;
			return Client::ReturnStatus::GeneralError;
		}

		MessageHeader currHeader;
		std::vector<uint8_t> consume(payload.begin() + bytesRead, payload.end());

		if (currHeader.Deserialize(consume) != true) {
			std::cout << "Read invalid header from server" << std::endl;
			return Client::ReturnStatus::GeneralError;
		}

		bytesLeft -= MessageHeader::GetSize();
		bytesRead += MessageHeader::GetSize();

		// Get friend name from UUID
		std::string clientName = GetNameFromUuid(currHeader.uuid);

		if (clientName == "" ||
			m_data.find(clientName) == m_data.end()) {
			std::cout << "Failed getting client's name" << std::endl;
			return Client::ReturnStatus::GeneralError;
		}

		std::cout << "From : " << clientName << std::endl;
		std::cout << "Content : " << std::endl;

		switch (currHeader.GetMessageType())
		{
		case MessageType::GetSymKey: {
			std::cout << "Request for symmetric key";
			break;
		}

		case MessageType::SendSymKey: {
			std::vector<uint8_t> content(consume.begin() + MessageHeader::GetSize(), consume.end());

			std::string symKey = m_privateKey->decrypt((char*)content.data(), 128);
			m_data[clientName]->SetSymKey((unsigned char*)symKey.c_str(), symKey.size());

			std::cout << "symmetric key received";
			break;
		}

		case MessageType::SendText: {
			std::vector<uint8_t> content(consume.begin() + MessageHeader::GetSize(), consume.end());
			
			std::string plain = m_data[clientName]->GetSymKey()->decrypt((char*)content.data(), currHeader.contentSize);
			
			std::cout << plain;
			break;
		}

		default:
			break;
		}

		bytesLeft -= currHeader.contentSize;
		bytesRead += currHeader.contentSize;
		std::cout << std::endl;
	}

	return Client::ReturnStatus::Success;
}

Client::ReturnStatus Client::HandleSendMessage() {

	ResponseSendMessage response;

	std::string name;
	std::string message;

	std::cout << "Insert destenation name: ";
	std::cin >> name;

	if (std::cin.fail()) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << "Bad entry" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	if (m_data.find(name) == m_data.end()) {
		std::cout << "Username not found" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	std::cout << "Insert message to send: ";
	std::cin >> message;

	if (std::cin.fail()) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cout << "Bad entry" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	if (m_data[name]->HasSym() != true) {
		std::cout << "Friend has no sym key set" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	std::string cipher = m_data[name]->GetSymKey()->encrypt(message.c_str(), message.size());
	std::vector<uint8_t> vec(cipher.begin(), cipher.end());

	DynamicRequest request(m_uuid, (uint16_t)Opcode::RequestSendMessage, vec);

	std::vector<uint8_t> requestBuff;
	request.Serialize(requestBuff);

	std::cout << "about to send " << requestBuff.size() << " bytes!" << std::endl;

	return Exchange(requestBuff, response);
}

Client::ReturnStatus Client::HandleRequestSymKey() {

	RequestGetSymKey request(m_uuid);
	ResponseSendMessage response;

	std::string name;

	std::cout << "Insert destenation name: ";
	std::cin >> name;

	if (m_data.find(name) == m_data.end()) {
		std::cout << "Username not found" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}
	
	if (m_data[name]->GetUuid(request.body.messageHeader.uuid) == false) {

		std::cout << "Failed getting UUID for user" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	return Exchange(request, response);
}

Client::ReturnStatus Client::HandleSendSymKey() {

	RequestSendSymKey request(m_uuid);
	ResponseSendMessage response;

	std::string name;

	std::cout << "Insert destenation name: ";
	std::cin >> name;

	if (m_data.find(name) == m_data.end()) {
		std::cout << "Username not found" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	if (m_data[name]->GetUuid(request.body.messageHeader.uuid) != true) {
		std::cout << "Failed getting UUID for user" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	if (m_data[name]->HasPuiblic() != true) {
		std::cout << "Ask for public key first!" << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	AESWrapper* symKey = m_data[name]->GetSymKey();
	memcpy((char*)&request.body.content, symKey->getKey(), 16);

	std::string cipher = m_data[name]->GetPublicKey()->encrypt((char*)&request.body.content, 16);
	memcpy((char*)&request.body.content, cipher.c_str(), request.body.messageHeader.contentSize);

	return Exchange(request, response);
}

Client::ReturnStatus Client::Exchange(const std::vector<uint8_t>& requestVec, std::vector<uint8_t>& responseVec) {

	BaseResponseHeader tempHeader;
	responseVec.clear();

	try {
		boost::asio::io_context io_context;
		boost::asio::ip::tcp::socket socket(io_context);

		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(m_ipAddr), m_port);

		socket.connect(endpoint);

		boost::asio::write(socket, boost::asio::buffer(requestVec.data(), requestVec.size()));

		// Reading header.
		boost::asio::streambuf header;
		boost::asio::read(socket, header, boost::asio::transfer_exactly(sizeof(tempHeader)));
		
		const unsigned char* header_data = boost::asio::buffer_cast<const unsigned char*>(header.data());
		responseVec.insert(responseVec.end(), header_data, header_data + sizeof(tempHeader));

		if (tempHeader.Deserialize(responseVec) != true) {
			throw std::runtime_error("Failed deserialize");
		}

		// Reading payload.
		boost::asio::streambuf payload;
		boost::asio::read(socket, payload, boost::asio::transfer_exactly(tempHeader.GetPayloadSize()));

		const unsigned char* payload_data = boost::asio::buffer_cast<const unsigned char*>(payload.data());
		responseVec.insert(responseVec.end(), payload_data, payload_data + tempHeader.GetPayloadSize());
		
		socket.close();
	}
	catch (std::exception& e) {

		std::cerr << "[ERROR] " << e.what() << std::endl;
		return Client::ReturnStatus::GeneralError;
	}

	// Make sure the server hasn't responded with an error.
	if (tempHeader.GetCode() == Opcode::ResponseFailure) {
		return Client::ReturnStatus::ServerError;
	}

	return Client::ReturnStatus::Success;
}
