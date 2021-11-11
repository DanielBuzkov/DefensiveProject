#include "Friend.h"

Friend::Friend() : m_isInit(false), m_publicKey(nullptr), m_symkey(nullptr) {}

Friend::~Friend() {
	if (this->m_publicKey) {
		delete this->m_publicKey;
		this->m_publicKey = nullptr;
	}

	if (this->m_symkey) {
		delete this->m_symkey;
		this->m_symkey = nullptr;
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

bool Friend::HasSym() const {
	return this->m_symkey != nullptr;
}

bool Friend::IsUuidEqual(const uuid_t &otherUuid) const {
	return this->m_uuid.IsEqual(otherUuid);
}

bool Friend::HasPublic() const {
	return this->m_publicKey != nullptr;
}

bool Friend::GetUuid(uuid_t o_uuidBuff) { 
	return this->m_uuid.Serialize(o_uuidBuff, sizeof(uuid_t));
}

std::string Friend::GetName() const { 
	uint8_t tmpNameBuffer[MAX_NAME_BUFFER_SIZE] = { 0 };
	this->m_name.Serialize(tmpNameBuffer, sizeof(name_t));

	std::string ret((char*)tmpNameBuffer);
	return ret;
}

RSAPublicWrapper* Friend::GetPublicKey() { 

	return this->m_publicKey;
}

AESWrapper* Friend::GetSymKey() { 
	
	if (this->m_symkey != nullptr) {
		return this->m_symkey;
	}

	this->m_symkey = new AESWrapper();
	
	return this->m_symkey;
}

void Friend::SetPublicKey(const publicKey_t key) {
	if (this->m_publicKey != nullptr) {
		return;
	}

	this->m_publicKey = new RSAPublicWrapper((char*)key, sizeof(publicKey_t));
}

void Friend::SetSymKey(const unsigned char* key, size_t keyLen) {
	if (this->m_symkey != nullptr) {
		return;
	}

	this->m_symkey = new AESWrapper(key, keyLen);
}
