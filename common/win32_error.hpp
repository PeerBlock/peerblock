/*
	Copyright (C) 2007 Cory Nelson

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

#include <stdexcept>
#include <windows.h>

/*
	win32_error wraps an error given by GetLastError().
*/
class win32_error : public std::exception {
public:
	win32_error(DWORD err = GetLastError()) : m_err(err),m_func(0),m_what(0),m_wcwhat(0) {}
	win32_error(const char *func, DWORD err = GetLastError()) : m_err(err),m_func(func),m_what(0),m_wcwhat(0) {}
	win32_error(const win32_error &err) : m_err(err.m_err),m_func(err.m_func),m_what(0),m_wcwhat(0) {}
	~win32_error() {
		if(m_what) LocalFree((HLOCAL)m_what);
		if(m_wcwhat) LocalFree((HLOCAL)m_wcwhat);
	}

	// gets an error description in the current codepage.
	const char* what() const {
		if(!m_what) {
			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, m_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&m_what,
				0, NULL);
		}

		return m_what ? m_what : "unknown";
	}

	// gets a wide-char error description.
	const wchar_t *wcwhat() const {
		if(!m_wcwhat) {
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, m_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPWSTR)&m_wcwhat,
				0, NULL);
		}

		return m_wcwhat ? m_wcwhat : L"unknown";
	}

	// gets the win32 function which caused the exception.
	const char* func() const { return m_func; }

	// gets the win32 error code.
	DWORD error() const { return m_err; }

private:
	win32_error& operator=(const win32_error&);

	const char *m_func;
	mutable const char *m_what;
	mutable const wchar_t *m_wcwhat;
	DWORD m_err;
};
