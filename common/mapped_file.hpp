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

#include <windows.h>
#include "win32_error.hpp"
#include "file.hpp"

/*
	memory maps a file for reading.
	opening 0-length files is safe.
*/
class mapped_file {
public:
	typedef const char* const_iterator;

	mapped_file() {}
	mapped_file(const wchar_t *file) { open(file); }
	~mapped_file() { close(); }

	// opens file for reading.
	void open(const wchar_t *name) {
		m_file.open(name, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, 0);

		m_size = (size_t)m_file.size();

		if(m_size) {
			m_map = CreateFileMapping(m_file.handle(), NULL, PAGE_READONLY, 0, 0, NULL);
			if(!m_map) {
				throw win32_error("CreateFileMapping");
			}

			m_view = MapViewOfFile(m_map, FILE_MAP_READ, 0, 0, 0);
			if(!m_view) {
				win32_error err("MapViewOfFile");

				CloseHandle(m_map);

				throw err;
			}

			m_end = (char*)m_view + m_size;
		}
		else {
			m_view = NULL;
			m_end = NULL;
		}
	}

	// closes the file.
	void close() {
		if(m_file.is_open()) {
			if(m_view != NULL) {
				UnmapViewOfFile(m_view);
				CloseHandle(m_map);

				m_view = NULL;
			}

			m_file.close();
		}
	}

	// iterators.
	const_iterator begin() const { return (char*)m_view; }
	const_iterator end() const { return (char*)m_end; }

	// constant-time size.
	std::size_t size() const { return m_size; }

private:
	// noncopyable.
	mapped_file(const mapped_file&);
	void operator=(const mapped_file&);

	file m_file;

	HANDLE m_map;
	void *m_view, *m_end;
	std::size_t m_size;
};
