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
#include <stdexcept>
#include "utf8.h"

class mbs_error : public std::runtime_error {
public:
	mbs_error(const char *msg) : runtime_error(msg) {}
};

std::wstring mbstowcs(const std::string &str);
std::string wcstombs(const std::wstring &str);

#define TSTRING_UTF8(wide) wchar_utf8(wide)
#define UTF8_TSTRING(narrow) utf8_wchar(narrow)

#define TSTRING_WCHAR(wide) (wide)
#define WCHAR_TSTRING(wide) (wide)

#define TSTRING_MBS(wide) wcstombs(wide)
#define MBS_TSTRING(narrow) mbstowcs(narrow)

#define MBS_UTF8(narrow) wchar_utf8(mbstowcs(narrow))
#define UTF8_MBS(narrow) wcstombs(utf8_wchar(narrow))
