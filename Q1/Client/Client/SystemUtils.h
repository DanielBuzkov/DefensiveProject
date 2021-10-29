#pragma once

#include <string>
#include "Protocol.h"

namespace SystemUtils {

	size_t GetFileSize(const std::string& path);

	bool IsFileExists(const std::string& path);
};
