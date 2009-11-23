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
#include "win32_error.hpp"

class file {
public:
	file() : m_file(INVALID_HANDLE_VALUE) {}
	file(LPCTSTR name, DWORD access, DWORD share, DWORD disposition, DWORD flags = 0) : m_file(INVALID_HANDLE_VALUE) { open(name, access, share, disposition, flags); }
	~file() { close(); }

	void open(LPCTSTR name, DWORD access, DWORD share, DWORD disposition, DWORD flags = 0) {
		if(m_file != INVALID_HANDLE_VALUE) throw std::runtime_error("file already open");

		m_file = CreateFile(name, access, share, NULL, disposition, flags, NULL);
		if(m_file == INVALID_HANDLE_VALUE) throw win32_error("CreateFile");
	}

	void close() {
		if(m_file != INVALID_HANDLE_VALUE) {
			CloseHandle(m_file);
			m_file = INVALID_HANDLE_VALUE;
		}
	}

	bool is_open() const { return (m_file != INVALID_HANDLE_VALUE); }

	void bind_iocp(LPOVERLAPPED_COMPLETION_ROUTINE func) {
		BOOL ret = BindIoCompletionCallback(m_file, func, 0);
		if(!ret) throw win32_error("BindIoCompletionCallback");
	}

	void ioctl(DWORD code, void *inbuf, DWORD inlen, void *outbuf, DWORD outlen, DWORD *bytes, OVERLAPPED *ovl) {
		BOOL ret = DeviceIoControl(m_file, code, inbuf, inlen, outbuf, outlen, bytes, ovl);
		if(!ret) {
			win32_error err("DeviceIoControl");

			if(ovl) {
				if(err.error() != ERROR_IO_PENDING) {
					throw err;
				}
			}
			else {
				throw err;
			}
		}
	}

	void readdirchanges(void *buf, DWORD buflen, bool recurse, DWORD filter, DWORD *bytes, OVERLAPPED *ovl) {
		BOOL ret = ReadDirectoryChangesW(m_file, buf, buflen, recurse ? TRUE : FALSE, filter, bytes, ovl, NULL);
		if(!ret) throw win32_error("ReadDirectoryChangesW");
	}

	void cancelio() {
		BOOL ret = CancelIo(m_file);
		if(!ret) throw win32_error("CancelIo");
	}

	DWORD getresult(OVERLAPPED *ovl) {
		DWORD bytes;

		BOOL ret = GetOverlappedResult(m_file, ovl, &bytes, TRUE);
		return ret ? ERROR_SUCCESS : GetLastError();
	}

	unsigned __int64 size() {
		LARGE_INTEGER li;
		if(!GetFileSizeEx(m_file, &li)) {
			throw win32_error("GetFileSizeEx");
		}

		return li.QuadPart;
	}

	HANDLE handle() const { return m_file; }

protected:
	HANDLE m_file;

private:
	file(const file&);
	void operator=(const file&);
};
