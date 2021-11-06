#pragma once

#include <string>

#include "Defines.h"
#include "Validators.h"

#include "RSAWrapper.h"
#include "AESWrapper.h"

class Friend {
public:
	Friend() : m_isInit(false), m_publicKey(nullptr), m_symkey(nullptr) {}
	~Friend();

	bool Init(std::string name, uint8_t* uuidBuffer, size_t buffLen);

	bool HasSym();
	bool HasPuiblic();
	bool IsNameEqual(const std::string& otherName) const;

	bool GetUuid(uuid_t o_uuidBuff);
	bool GetName(name_t o_nameBuff);
	RSAPublicWrapper* GetPublicKey();
	AESWrapper* GetSymKey();
	
	void SetPublicKey(const publicKey_t key);
	void SetSymKey(const uint8_t* key, size_t keyLen);

private:
	bool m_isInit;

	UUID m_uuid;
	Name m_name;

	RSAPublicWrapper *m_publicKey;
	AESWrapper *m_symkey;
};
