#pragma once

#include <string>

/**
* This file holds implementation for system utilities.
* Once the application would want to support a new OS, this namespace
* shall be re-written.
*/

namespace SystemUtils {

	/**
	* This functino returns the size of a file in a given path.
	* 
	* @param path	-	The path to the file which we want its' size.
	* 
	* @return size_t	-	The size of the file.
	*/
	size_t GetFileSize(const std::string& path);

	/**
	* This functino returns if a file in a path exists.
	*
	* @param path	-	The path to the file which we want to check.
	*
	* @return bool	-	True if exists, false otherwise.
	*/
	bool IsFileExists(const std::string& path);
};
