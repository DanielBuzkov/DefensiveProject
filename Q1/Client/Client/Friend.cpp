#include "Friend.h"

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

bool Friend::Init(std::string name, uint8_t* uuidBuffer, size_t buffLen) {
	
	// Won't init twice
	if (m_isInit == true) {
		return false;
	}
	
	// Initialize the fields which each friend MUST have.
	if (m_name.Deserialize(name) == false ||
		m_uuid.Deserialize((char*)uuidBuffer, buffLen) == false) {
		return false;
	}

	m_isInit = true;

	return true;
}

bool Friend::IsUuidEqual(const uuid_t &otherUuid) const {
	return m_uuid.IsEqual(otherUuid);
}

bool Friend::HasSym() {
	return m_symkey != nullptr;
}

bool Friend::HasPuiblic() {
	return m_publicKey != nullptr;
}

bool Friend::GetUuid(uuid_t o_uuidBuff) { 
	return m_uuid.Serialize(o_uuidBuff, sizeof(uuid_t));
}

std::string Friend::GetName() { 
	uint8_t tmpNameBuffer[MAX_NAME_BUFFER_SIZE] = { 0 };
	m_name.Serialize(tmpNameBuffer, sizeof(name_t));

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
