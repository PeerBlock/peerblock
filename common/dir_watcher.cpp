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

#include "stdafx.h"
#include "dir_watcher.hpp"

void dir_watcher::open(const wchar_t *dir, bool recurse, DWORD filter, const function_type &func) {
	m_func = func;

	m_recurse = recurse;
	m_filter = filter;

	m_file.open(dir, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, FILE_FLAG_OVERLAPPED);
	m_file.bind_iocp(on_iocp_thunk);

	m_file.readdirchanges(m_buffer, sizeof(m_buffer), recurse, filter, NULL, this);
}

void dir_watcher::close() {
	if(m_file.is_open()) {
		m_file.close();
		m_func = function_type();
	}
}

void dir_watcher::on_iocp(DWORD err, DWORD transfered) {
	if(err == ERROR_SUCCESS) {
		std::vector<change> changes;

		FILE_NOTIFY_INFORMATION *iter = (FILE_NOTIFY_INFORMATION*)m_buffer;
		while(iter) {
			//TODO: do parsing.

			if(iter->Action) {
				iter = (FILE_NOTIFY_INFORMATION*)((char*)iter + iter->Action);
			}
			else {
				iter = 0;
			}
		}

		m_func(changes);
		
		m_file.readdirchanges(m_buffer, sizeof(m_buffer), m_recurse, m_filter, NULL, this);
	}
	else if(err != ERROR_OPERATION_ABORTED) {
		// report error.
	}
}

void WINAPI dir_watcher::on_iocp_thunk(DWORD err, DWORD transfered, OVERLAPPED *ovl) {
	((dir_watcher*)ovl)->on_iocp(err, transfered);
}
