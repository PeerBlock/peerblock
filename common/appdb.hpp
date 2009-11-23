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
#include <hash_map>
#include "sqlite3x/sqlite3x.hpp"
#include "platform.hpp"

/*
	appdb stores application names/paths that the WFP backend has encountered.
*/
class appdb {
public:
	struct path_info {
		std::wstring name;
		std::wstring path;
	};

	typedef stdext::hash_map<std::wstring, path_info> map_type;

	COMMON_EXPORT appdb();
	COMMON_EXPORT ~appdb();

	COMMON_EXPORT void open();
	COMMON_EXPORT void close();

	COMMON_EXPORT void insert(const std::wstring &path);

	const map_type& map() const { return m_cache; }

private:
	typedef std::vector<std::pair<std::wstring, path_info> > queue_type;

	map_type m_cache;
	queue_type m_queue;
	map_type m_windows;

	sqlite3x::sqlite3_connection m_con;
	sqlite3x::sqlite3_command m_icmd, m_scmd;

	void enum_proc(HWND hwnd);
	static BOOL CALLBACK enum_windows_thunk(HWND hwnd, LPARAM lParam);
	static BOOL CALLBACK enum_desktops_thunk(LPWSTR desktop, LPARAM lParam);
	static BOOL CALLBACK enum_wstations_thunk(LPWSTR station, LPARAM lParam);
};
