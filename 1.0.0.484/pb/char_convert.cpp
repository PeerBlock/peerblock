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

#include "stdafx.h"
using namespace std;

std::wstring mbstowcs(const std::string &str) {
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.length(), NULL, 0);
	if(len == -1) throw win32_error("MultiByteToWideChar");

	boost::scoped_array<wchar_t> buf(new wchar_t[len]);

	len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.length(), buf.get(), len);
	if(len == -1) throw win32_error("MultiByteToWideChar");

	return wstring(buf.get(), len);
}

std::string wcstombs(const std::wstring &str) {
	int len = WideCharToMultiByte(CP_ACP, 0, str.c_str(), (int)str.length(), NULL, 0, NULL, NULL);
	if(len == -1) throw win32_error("WideCharToMultiByte");
	
	boost::scoped_array<char> buf(new char[len]);

	len = WideCharToMultiByte(CP_ACP, 0, str.c_str(), (int)str.length(), buf.get(), len, NULL, NULL);
	if(len == -1) throw win32_error("WideCharToMultiByte");

	return string(buf.get(), len);
}
