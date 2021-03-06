#include "Validators.h"

#include <iomanip>

// Every two chars in ASCII represent a single byte of data.
static constexpr size_t CHARS_PER_BYTE = 2;
static constexpr size_t HEX_BASE = 16;

static constexpr uint8_t LOW_NIBBLE = 0x0f;
static constexpr size_t NIBBLE_LEN_IN_BITS = 4;

// Leaving space for null terminator
static constexpr size_t MAX_NAME_STR_SIZE = MAX_NAME_BUFFER_SIZE - 1;

bool Name::Deserialize(const std::string data) {
	
	// Invalid size for given data.
	// Make sure to leave the '>=' to leave space fot the null terminator.
	if (data.length() >= sizeof(this->m_data)) {
		return false;
	}
	
	// Won't allow rewrite existing data.
	if (this->m_isInit == true) {
		return false;
	}

	// Making sure name is valid.
	// Allowing only alphabetic charecters and space.
	for (auto c : data) {
		if (isalpha(c) == false &&
			c != ' ') {
			return false;
		}
	}

	memcpy(this->m_data, data.c_str(), data.length());
	
	// m_data is already set as zeros, but to make sure.
	this->m_data[data.length()] = 0;

	this->m_isInit = true;

	return true;
}

bool Name::Serialize(uint8_t* buffer, const size_t buffSize) const {

	// Invalid size for given data.
	// Allowing only the exact size for buffers.
	if (buffSize != sizeof(this->m_data)) {
		return false;
	}

	// Unable to copy data, since the instance has no data.
	if (this->m_isInit != true) {
		return false;
	}

	memset(buffer, 0, buffSize);
	memcpy(buffer, this->m_data, buffSize);

	return true;
}

void Name::Reset() {
	this->m_isInit = false;
	memset(this->m_data, 0, sizeof(this->m_data));
}

bool UUID::FromFile(const std::string data) {

	// Invalid size for given data.
	if (data.length() != CHARS_PER_BYTE * sizeof(this->m_data)) {
		return false;
	}

	// Won't allow rewrite existing data.
	if (this->m_isInit == true) {
		return false;
	}

	// Making sure uuid is valid.
	for (auto c : data) {
		if (isxdigit(c) == false) {
			return false;
		}
	}

	// Converting each to charecters to a single value in m_data.
	for (size_t i = 0; i < sizeof(this->m_data); i++) {
		long temp = std::stoul(data.substr(CHARS_PER_BYTE * i, CHARS_PER_BYTE), nullptr, HEX_BASE);

		// Should not happen, since the substring is only two characters long.
		if (temp > UINT8_MAX) {
			return false;
		}

		this->m_data[i] = (uint8_t)temp;
	}

	this->m_isInit = true;

	return true;
}

bool UUID::ToFile(std::string &o_string) const {

	// Unable to copy data, since the instance has no data.
	if (this->m_isInit != true) {
		return false;
	}

	const char* digits = "0123456789abcdef";

	// Converting each to charecters to a single value in m_data.
	for (size_t i = 0; i < sizeof(this->m_data) ; i++) {
		auto high = (this->m_data[i] >> NIBBLE_LEN_IN_BITS) & LOW_NIBBLE;
		auto low = this->m_data[i] & LOW_NIBBLE;

		o_string += digits[high];
		o_string += digits[low];
	}

	return true;
}

bool UUID::Deserialize(const char* buffer, const size_t buffLen) {
	
	if (buffLen != sizeof(uuid_t)) {
		return false;
	}

	// Now allowing rewriting the current data.
	if (this->m_isInit == true) {
		return false;
	}

	memcpy(this->m_data, buffer, buffLen);
	this->m_isInit = true;

	return true;
}

bool UUID::Serialize(uint8_t* buffer, const size_t buffLen) const{

	if (buffLen != sizeof(uuid_t)) {
		return false;
	}

	// Making sure that the instace is initalized.
	if (this->m_isInit != true) {
		return false;
	}

	memset(buffer, 0, buffLen);
	memcpy(buffer, this->m_data, sizeof(this->m_data));

	return true;
}

const bool UUID::IsEqual(const uuid_t& otherUuid) const {
	return memcmp(this->m_data, otherUuid, sizeof(uuid_t)) == 0;
}
