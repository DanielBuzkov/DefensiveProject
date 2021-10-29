#pragma once

#include <stdint.h>
#include <string>

/*
	The purpose of the following classes is to handle basic data,
	validate given input for parsing and enabling a safe interface
	to read and write the relevant data as declared by the project manager.
*/

static constexpr size_t MAX_NAME_BUFFER_SIZE = 255;
static constexpr size_t UUID_LENGTH = 16;

typedef uint8_t name_t[MAX_NAME_BUFFER_SIZE];
typedef uint8_t uuid_t[UUID_LENGTH];

class Name {
public:
	Name() : m_isInit(false), m_data{ 0 } {}

	/**
		This function initializes the instance with a given string.
		The function makes sure the string stands by the protocol's
		rules, and keeping a valid length and values.

		Only then the function copies the data given to the m_data array
		and keeping the null termintator at place.

		@param	data	-	The given string which is wanted to be inserted.
		
		@return	bool	-	True if initialized successfuly, false otherwise.
	*/
	bool Deserialize(const std::string data);

	/**
		Used to fill the given buffer by the content of the instance.

		@param	buffer	-	The buffer to which copy the data.
		@param	buffLen	-	The length of given buffer.

		@return	bool	-	True is serialized succesfuly, false otherwise.
	*/
	bool Serialize(uint8_t *buffer, const size_t buffLen) const;

	const bool IsEqual(const std::string& otherName) const;

	void Reset();

private:
	bool m_isInit;
	name_t m_data;
};


class UUID {
public:
	UUID() : m_isInit(false), m_data{ 0 } {}

	/**
		This function initializes the instance with a given string,
		assuming it is read from a file as ascii.

		Only then the function copies the data given to the m_data array.

		This function takes into account that each byte from is represented
		as two ascii charecters.

		@param	data	-	The given string which is wanted to be inserted.

		@return	bool	-	True if initialized successfuly, false otherwise.
	*/
	bool FromFile(const std::string data);

	/**
		Used to fill the given buffer by the content of the instance.

		@param	buffer	-	The buffer to which copy the data.
		@param	buffLen	-	The length of given buffer.

		@return	bool	-	True is serialized succesfuly, false otherwise.
	*/
	bool ToFile(uint8_t* buffer, const size_t buffLen) const;

	bool Deserialize(const char* buffer, const size_t buffLen);

	bool Serialize(uint8_t* buffer, const size_t buffLen) const;

private:
	bool m_isInit;
	uuid_t m_data;
};

