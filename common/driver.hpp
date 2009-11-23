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

#include <string>
#include <windows.h>
#include "file.hpp"

/*
	handles installing, starting, and communicating with a driver.
*/
class driver {
public:
	driver();
	~driver();

	// loads a driver by name.  file and device file are inferred.
	void load(const std::wstring &name);

	// loads a driver by name and file.  device file is inferred.
	void load(const std::wstring &name, const std::wstring &file);

	// loads a driver by name, file, and device file.
	void load(const std::wstring &name, const std::wstring &file, const std::wstring &devfile);

	// stops and closes the driver.
	void close();

	// checks if the driver is running.
	bool isrunning();

	// starts the driver.
	void start();

	// stops the driver.
	void stop();

	// sends an IOCTL to the driver.
	void rawio(DWORD ioctl, void *inbuf, DWORD insize, void *outbuf, DWORD outsize, OVERLAPPED *ovl = 0) {
		DWORD bytes;
		m_dev.ioctl(ioctl, inbuf, insize, outbuf, outsize, &bytes, ovl);
	}

	// sends an IOCTL to the driver, with the output buffers NULL.
	void read(DWORD ioctl, void *buf, DWORD size, OVERLAPPED *ovl = 0) {
		rawio(ioctl, 0, 0, buf, size, ovl);
	}

	// sends an IOCTL to the driver, with the input buffers NULL.
	void write(DWORD ioctl, void *buf, DWORD size, OVERLAPPED *ovl = 0) {
		rawio(ioctl, buf, size, 0, 0, ovl);
	}

	// cancels all overlapped I/O for the current thread.
	// results of the I/O is ERROR_OPERATION_ABORTED.
	void cancelio() { m_dev.cancelio(); }

	// gets the result of an overlapped operation.
	DWORD getresult(OVERLAPPED *ovl) { return m_dev.getresult(ovl); }

	// sets if the driver can be removed after it is stopped.
	bool removable;

private:
	// noncopyable.
	driver(const driver&);
	void operator=(const driver&);

	file m_dev;
	std::wstring m_name, m_file, m_devfile;
	bool m_loaded, m_started;
};
