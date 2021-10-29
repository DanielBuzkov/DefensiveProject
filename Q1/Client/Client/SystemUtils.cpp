
#include "SystemUtils.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace SystemUtils {
	size_t GetFileSize(const std::string& path) {
		struct stat stat_buf;
		int rc = stat(path.c_str(), &stat_buf);

		// In the context of the relevant usage, getting a negative value is enough.
		return rc == 0 ? stat_buf.st_size : -1;
	}

	bool IsFileExists(const std::string& path) {
		std::ifstream file;
		file.open(path);

		bool ret = file ? true : false;

		file.close();

		return ret;
	}
};
