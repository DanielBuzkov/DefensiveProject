#pragma once

#include <string>

#include "Defines.h"
#include "Validators.h"

#include "RSAWrapper.h"
#include "AESWrapper.h"

/**
	This class intented to help handling the other clients' data.
	It is a simple interface to get and update all relevant members
	such as the public and symetric key for each othe client
	and the name.
*/

class Friend {
public:
	
	Friend();
	~Friend();

	/**
		This function initializes the given client with a name and a uuid.
		If any of the fields is invalid, the function shall fail.

		It will also won't allow re-initialization of an instance, 
		since the name and the UUID are constant values for each client.

		@param	name	-	A string which contains the name of the client.
		@param	uuid	-	The UUID of the other client.

		@return	bool	-	True upon success, false otherwise.
	*/
	bool Init(const std::string name, const uuid_t uuid);

	/**
		@return	bool	-	True if the client has a symetric key with the user, false otherwise.
	*/
	bool HasSym();
	
	/**
		@return	bool	-	True if the client has a public key saved, false otherwise.
	*/
	bool HasPuiblic();

	/**
		This function compares a given UUID to the client's UUID. 
	
		@param	otheUuid	-	The uuid to which this client is compared.

		@return	bool	-	True if the UUIDs match-up, false otherwise.
	*/
	bool IsUuidEqual(const uuid_t &otherUuid) const;

	/**
		This function gets the client's UUUID

		@param	o_uuidBuff	-	Out parameter to which the uuid will be deserialized.

		@return	bool	-	True upon success, false otherwise.
	*/
	bool GetUuid(uuid_t o_uuidBuff);

	/**
		This function gets the client's name as a string.

		@return string	-	The client's name upon success, empty string upon failure.
	*/
	std::string GetName();

	/**
		@return RSAPublicWrapper	-	A pointer to the client's RSAPublicWrapper
	*/
	RSAPublicWrapper* GetPublicKey();

	/**
		@return AESWrapper	-	A pointer to the client's AESWrapper
	*/
	AESWrapper* GetSymKey();
	
	/**
		This function updates the client's public key.

		@param	key	-	The key to use as the public key.
	*/
	void SetPublicKey(const publicKey_t key);

	/**
		This function updates the client's symetric key with the user.

		@param	key		-	The key to use as the symetric key.
		@param	keyLen	-	The length of the given key.
	*/
	void SetSymKey(const unsigned char* key, size_t keyLen);

private:
	// An indicator to make sure no re-initializtion is made, and data is read only when initialized.
	bool m_isInit;

	// The client's UUID and name.
	UUID m_uuid;
	Name m_name;

	// The client's key wrappers.
	RSAPublicWrapper *m_publicKey;
	AESWrapper *m_symkey;
};
