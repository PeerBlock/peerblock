/*
	Copyright (C) 2004-2005 Cory Nelson

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.

*/

#pragma once

#include <string>
#include <utility>
#include <stdexcept>
#include <cstddef>
#include <boost/utility.hpp>
#include <boost/shared_array.hpp>
#include <unzip.h>
#include "pathx.hpp"

class ZipFile : boost::noncopyable {
private:
	unzFile fp;

public:
	ZipFile();
	ZipFile(const path &file);
	~ZipFile();

	void Open(const path &file);
	void Close();

	bool IsOpen() const;

	bool GoToFirstFile();
	bool GoToNextFile();

	void OpenCurrentFile();
	void CloseCurrentFile();

	std::string GetCurrentFileName();
	std::pair<boost::shared_array<char>,size_t> ReadCurrentFile();
};

class zip_error : public std::runtime_error {
private:
	static std::string errmsg(int e);

public:
	zip_error(const std::string &func) : runtime_error(func) {}
	zip_error(const std::string &func, int msg) : runtime_error(func+": "+errmsg(msg)) {}
	zip_error(const std::string &func, const std::string &msg) : runtime_error(func+": "+msg) {}
};
