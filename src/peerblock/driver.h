/*
	Copyright (C) 2004-2005 Cory Nelson
	Based on the original work by Tim Leonard
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

#include <string>
#include <windows.h>

class driver {
public:
	driver();
	~driver();

	void load(const std::wstring &name);
	void load(const std::wstring &name, const std::wstring &file);
	void load(const std::wstring &name, const std::wstring &file, const std::wstring &devfile);
	void close();

	bool isrunning();
	void start(bool gethandle = true);
	void stop();

	DWORD rawio(DWORD ioctl, void *inbuf, DWORD insize, void *outbuf, DWORD outsize, OVERLAPPED *ovl = 0);
	DWORD read(DWORD ioctl, void *buf, DWORD size, OVERLAPPED *ovl = 0);
	DWORD write(DWORD ioctl, void *buf, DWORD size, OVERLAPPED *ovl = 0);

	DWORD cancelio();
	DWORD getresult(OVERLAPPED *ovl);

	bool removable;

private:
	// noncopyable.
	driver(const driver&);
	driver& operator=(const driver&);

	HANDLE m_dev;
	std::wstring m_name, m_file, m_devfile;
	bool m_loaded, m_started, m_stoppable;
};
