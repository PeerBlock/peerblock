/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009 PeerBlock, LLC

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



//================================================================================================
//
//  win32_error
//
/// <summary>
///   Exception class that routines can catch() and specially-handle to display useful error
///	  windows for exception conditions.
/// </summary>
/// <remarks>
///	  Exceptions of this type are meant to provide better information on known error conditions.
///	  For example we know that if FwpmEngineOpen0() returns 1753 (in wfp.hpp) this generally
///	  means that the Windows "Base Filtering Engine" (and/or its dependent services" are not
///	  running, so if we see this error we can tell people to ensure that those services are not
///	  disabled.
/// </remarks>
//
class win32_error : public std::exception {
public:
	win32_error(DWORD err = GetLastError()) : m_err(err),m_func(0),m_what(0) {}
	win32_error(const char *func, DWORD err = GetLastError()) : m_err(err),m_func(func),m_what(0) {}
	win32_error(const win32_error &err) : m_err(err.m_err),m_func(err.m_func),m_what(0) {}
	~win32_error() { if(m_what) LocalFree((HLOCAL)m_what); }

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

	const char* func() const { return m_func; }
	DWORD error() const { return m_err; }

private:
	win32_error& operator=(const win32_error&);

	const char *m_func;
	mutable const char *m_what;
	DWORD m_err;

};	// End of class win32_error



//================================================================================================
//
//  peerblock_error
//
/// <summary>
///   Exception class that requires no special win32_error style handling.  Just display the
///	  appropriate messagebox, no need for printing GetLastError() et al.
/// </summary>
/// <remarks>
///	  Exceptions of this type are meant to provide better information on known error conditions.
///	  For example we know that if FwpmEngineOpen0() returns 1753 (in wfp.hpp) this generally
///	  means that the Windows "Base Filtering Engine" (and/or its dependent services" are not
///	  running, so if we see this error we can tell people to ensure that those services are not
///	  disabled.
/// </remarks>
//
class peerblock_error : public win32_error 
{
  public:
	peerblock_error(DWORD _codeId = 0, DWORD _textId = 0) : m_codeId(_codeId), m_textId(_textId) {}
    DWORD m_codeId;
    DWORD m_textId;

  private:

}; // End of class peerblock_error

