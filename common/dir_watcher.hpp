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
#include <vector>
#include <boost/function.hpp>
#include <windows.h>
#include "file.hpp"
#include "mutex.hpp"

class dir_watcher : private OVERLAPPED {
public:
	struct change {
		enum action_type { added, removed, modified, renamed };
		action_type action;
		std::wstring file;
		std::wstring oldfile; // valid on renamed only.
	};

	typedef boost::function<void(const std::vector<change>&)> function_type;

	dir_watcher() {}
	dir_watcher(const wchar_t *dir, bool recurse, DWORD filter, const function_type &func) { open(dir, recurse, filter, func); }
	~dir_watcher() { close(); }

	void open(const wchar_t *dir, bool recurse, DWORD filter, const function_type &func);
	void close();

private:
	file m_file;
	mutex m_lock;
	function_type m_func;
	bool m_recurse;
	DWORD m_filter;
	char m_buffer[65535];

	void on_iocp(DWORD err, DWORD transfered);
	static void WINAPI on_iocp_thunk(DWORD err, DWORD transfered, OVERLAPPED *ovl);
};
