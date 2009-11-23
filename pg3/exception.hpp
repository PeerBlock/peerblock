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

#include <cstdio>
#include <string>
#include <exception>
#include <boost/scoped_array.hpp>
#include <windows.h>
#include <commdlg.h>

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)

class commdlg_error : public std::exception {
public:
	commdlg_error(const char *func, DWORD err = CommDlgExtendedError()) : m_func(func),m_err(err) {}
	commdlg_error(DWORD err = CommDlgExtendedError()) : m_func(0),m_err(err) {}
	commdlg_error(const commdlg_error &err) : m_func(err.m_func),m_err(err.m_err) {}

	const char* what() const;
	const wchar_t* wcwhat() const;

	const char* func() const { return m_func; }
	DWORD error() const { return m_err; }

private:
	mutable std::wstring m_wcwhat;
	mutable boost::scoped_array<char> m_what;
	const char *m_func;
	DWORD m_err;
};

void ReportException(HWND hwnd, std::exception &ex, const wchar_t *file, unsigned int line);
