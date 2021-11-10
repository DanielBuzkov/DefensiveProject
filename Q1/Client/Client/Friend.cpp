#include "Friend.h"

Friend::Friend() : m_isInit(false), m_publicKey(nullptr), m_symkey(nullptr) {}

Friend::~Friend() {
	if (m_publicKey) {
		delete m_publicKey;
		m_publicKey = nullptr;
	}

	if (m_symkey) {
		delete m_symkey;
		m_symkey = nullptr;
	}
}

bool Friend::Init(const std::string name, const uuid_t uuid) {
	
	// Won't init twice
	if (this->m_isInit == true) {
		return false;
	}
	
	// Initialize the fields which each friend MUST have.
	if (this->m_name.Deserialize(name) == false ||
		this->m_uuid.Deserialize((char*)uuid, sizeof(uuid_t)) == false) {
		return false;
	}

	this->m_isInit = true;

	return true;
}

bool Friend::IsUuidEqual(const uuid_t &otherUuid) const {
	return this->m_uuid.IsEqual(otherUuid);
}

bool Friend::HasSym() {
	return m_symkey != nullptr;
}

bool Friend::HasPuiblic() {
	return this->m_publicKey != nullptr;
}

bool Friend::GetUuid(uuid_t o_uuidBuff) { 
	return this->m_uuid.Serialize(o_uuidBuff, sizeof(uuid_t));
}

std::string Friend::GetName() { 
	uint8_t tmpNameBuffer[MAX_NAME_BUFFER_SIZE] = { 0 };
	this->m_name.Serialize(tmpNameBuffer, sizeof(name_t));

	std::string ret((char*)tmpNameBuffer);
	return ret;
}

RSAPublicWrapper* Friend::GetPublicKey() { 

	return m_publicKey; 
}

AESWrapper* Friend::GetSymKey() { 
	
	if (m_symkey != nullptr) {
		return m_symkey;
	}

	m_symkey = new AESWrapper();
	
	return m_symkey; 
}

void Friend::SetPublicKey(const publicKey_t key) {
	if (m_publicKey != nullptr) {
		return;
	}

	m_publicKey = new RSAPublicWrapper((char*)key, sizeof(publicKey_t));
}

void Friend::SetSymKey(const unsigned char* key, size_t keyLen) {
	if (m_symkey != nullptr) {
		return;
	}

	m_symkey = new AESWrapper(key, keyLen);
}
