#include "Friend.h"

Friend::~Friend() {
	if (m_publicKey) {
		delete m_publicKey;
	}

	if (m_symkey) {
		delete m_symkey;
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

bool Friend::IsNameEqual(const std::string& otherName) const {
	return m_name.IsEqual(otherName);
}

bool Friend::HasSym() {
	return m_symkey != nullptr;
}

bool Friend::GetUuid(uuid_t o_uuidBuff) { 
	return m_uuid.Serialize(o_uuidBuff, sizeof(uuid_t));
}

bool Friend::GetName(name_t o_nameBuff) { 
	return m_name.Serialize(o_nameBuff, sizeof(name_t));
}

RSAPublicWrapper* Friend::GetPublicKey() { return m_publicKey; }
AESWrapper* Friend::GetSymKey() { return m_symkey; }

void Friend::SetPublicKey(const publicKey_t key) {
	if (m_publicKey != nullptr) {
		return;
	}

	m_publicKey = new RSAPublicWrapper((char*)key, sizeof(publicKey_t));
}

void Friend::SetSymKey(const uint8_t* key, size_t keyLen) {
	if (m_symkey != nullptr) {
		return;
	}

	m_symkey = new AESWrapper((unsigned char*)key, keyLen);
}
