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
#include "path.hpp"
#include "config.hpp"
#include "win32_error.hpp"
#include "appdb.hpp"
using namespace sqlite3x;

appdb::appdb() {}
appdb::~appdb() { close(); }

void appdb::open() {
	path p = global_config::appdb_path();
	m_con.open(p.c_str());

	m_con.executenonquery("PRAGMA page_size = 4096");

	m_con.executenonquery(
		"CREATE TABLE IF NOT EXISTS t_apps("
		"  path TEXT NOT NULL UNIQUE,"
		"  realpath TEXT NOT NULL,"
		"  name TEXT)");

	m_icmd.prepare(m_con, "INSERT INTO t_apps(path,realpath,name) VALUES(?,?,?)");
	m_scmd.prepare(m_con, "SELECT realpath,name FROM t_apps WHERE path=?");

	m_windows.clear();
	EnumWindowStations(enum_wstations_thunk, (LPARAM)this);

	// for now.
	m_cache = m_windows;
}

void appdb::close() {
	if(m_con.is_open()) {
		m_icmd.close();
		m_scmd.close();

		m_con.close();
	}
}

void appdb::insert(const std::wstring &path) {
	const std::wstring upper = path::toupper(path);

	map_type::iterator iter = m_cache.find(upper);

	if(iter == m_cache.end()) {
		sqlite3_reader reader(m_scmd);

		m_scmd.bind(1, upper);
		if(m_scmd.step() == sqlite3x::row) {
			map_type::mapped_type v;

			v.path = m_scmd.getstring16(0);
			v.name = m_scmd.getstring16(1);

			iter = m_cache.insert(map_type::value_type(upper, v)).first;
		}
	}

	if(iter == m_cache.end()) {
		map_type::mapped_type &cached = m_cache[upper];

		map_type::iterator witer = m_windows.find(upper);
		if(witer == m_windows.end()) {
			m_windows.clear();
			EnumWindowStations(enum_wstations_thunk, (LPARAM)this);

			witer = m_windows.find(upper);
		}

		if(witer != m_windows.end()) {
			cached = witer->second;
		}

		m_queue.push_back(std::make_pair(upper, cached));

		sqlite3_transaction trans(m_con);
		sqlite3_reader reader(m_icmd);
		sqlite3_result res;

		for(queue_type::size_type i = 0; i < m_queue.size(); ++i) {
			m_icmd.bind(1, m_queue[i].first);
			m_icmd.bind(2, m_queue[i].second.path);
			m_icmd.bind(3, m_queue[i].second.name);

			res = m_icmd.executenonquery();
			if(res == sqlite3x::busy) break;
		}

		if(res != sqlite3x::busy) {
			m_queue.clear();
			trans.commit();
		}
	}
}

void appdb::enum_proc(HWND hwnd) {
	if(GetWindow(hwnd, GW_OWNER)/* || !IsWindowVisible(hwnd)*/) {
		return;
	}

	DWORD procid;
	GetWindowThreadProcessId(hwnd, &procid);

	HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procid);
	if(proc) {
		wchar_t path[MAX_PATH];
		DWORD len = GetModuleFileNameEx(proc, NULL, path, MAX_PATH);
		if(len) {
			const std::wstring pathx(path, len);
			const std::wstring upper = path::toupper(pathx);

			map_type::iterator iter = m_windows.find(upper);
			if(iter == m_windows.end()) {
				map_type::mapped_type v;
				v.path = pathx;

				iter = m_windows.insert(map_type::value_type(upper, v)).first;
			}

			if(iter->second.name.empty()) {
				int len = GetWindowTextLength(hwnd);

				if(len) {
					boost::scoped_array<wchar_t> buf(new wchar_t[++len]);
					
					len = GetWindowText(hwnd, buf.get(), len);
					if(len) iter->second.name.assign(buf.get(), len);
				}
			}
		}

		CloseHandle(proc);
	}
}

BOOL CALLBACK appdb::enum_windows_thunk(HWND hwnd, LPARAM lParam) {
	((appdb*)lParam)->enum_proc(hwnd);
	return TRUE;
}

BOOL CALLBACK appdb::enum_desktops_thunk(LPWSTR desktop, LPARAM lParam) {
	HDESK d = OpenDesktop(desktop, 0, FALSE, DESKTOP_ENUMERATE);

	if(d) {
		EnumDesktopWindows(d, enum_windows_thunk, lParam);
		CloseDesktop(d);
	}

	return TRUE;
}

BOOL CALLBACK appdb::enum_wstations_thunk(LPWSTR station, LPARAM lParam) {
	HWINSTA ws = OpenWindowStation(station, FALSE, WINSTA_ENUMDESKTOPS);
	
	if(ws) {
		EnumDesktops(ws, enum_desktops_thunk, lParam);
		CloseWindowStation(ws);
	}

	return TRUE;
}
