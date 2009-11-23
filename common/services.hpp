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

#include <boost/shared_ptr.hpp>
#include <windows.h>
#include "win32_error.hpp"

namespace services {

// base class for other services handles.
class handle {
public:
	void close() {
		if(m_handle) {
			BOOL ret = CloseServiceHandle(m_handle);
			if(!ret) throw win32_error("CloseServiceHandle");

			m_handle = 0;
		}
	}

protected:
	handle() : m_handle(0) {}
	~handle() { close(); }

	SC_HANDLE m_handle;
};

class service;

// manager keeps track of other services.
class manager : private handle {
public:
	manager(const wchar_t *machine = 0, const wchar_t *database = 0, DWORD access = SC_MANAGER_ALL_ACCESS) {
		m_handle = OpenSCManager(machine, database, access);
		if(!m_handle) throw win32_error("OpenSCManager");
	}

	friend service;
};

// service operates on services.
class service : public handle {
public:
	service() {}

	// creates a service.  returns true if created, false if it already exists.
	bool create(manager &m, const wchar_t *name, const wchar_t *displayname, DWORD servtype, DWORD starttype, DWORD errctrl, const wchar_t *path, DWORD access = SERVICE_ALL_ACCESS) {
		m_handle = CreateService(m.m_handle, name, displayname, access, servtype, starttype, errctrl, path, NULL, NULL, NULL, NULL, NULL);
		if(!m_handle) {
			DWORD err = GetLastError();
			if(err == ERROR_SERVICE_EXISTS) {
				return false;
			}
			else {
				throw win32_error("CreateService", err);
			}
		}

		return true;
	}

	// opens a service.
	void open(manager &m, const wchar_t *name, DWORD access = SERVICE_ALL_ACCESS) {
		m_handle = OpenService(m.m_handle, name, access);
		if(!m_handle) throw win32_error("OpenService");
	}

	// retrieves the current service config.
	boost::shared_ptr<QUERY_SERVICE_CONFIG> query_config() {
		boost::shared_ptr<QUERY_SERVICE_CONFIG> qsc;
		DWORD bytes = 0;
		BOOL ret;

		do {
			ret = QueryServiceConfig(m_handle, qsc.get(), bytes, &bytes);
			if(!ret) {
				DWORD err = GetLastError();
				if(err == ERROR_INSUFFICIENT_BUFFER) {
					QUERY_SERVICE_CONFIG *p = (QUERY_SERVICE_CONFIG*)GlobalAlloc(GPTR, bytes);
					if(!p) throw win32_error("GlobalAlloc");

					qsc.reset(p, &GlobalFree);
				}
				else {
					throw win32_error("QueryServiceConfig", err);
				}
			}
		} while(!ret);

		return qsc;
	}

	// retrieves the current service status.
	SERVICE_STATUS query_status() {
		SERVICE_STATUS status;

		BOOL ret = QueryServiceStatus(m_handle, &status);
		if(!ret) throw win32_error("QueryServiceStatus");

		return status;
	}

	// sends a control message to the service.
	SERVICE_STATUS control(DWORD ctrl) {
		SERVICE_STATUS status;

		BOOL ret = ControlService(m_handle, ctrl, &status);
		if(!ret) throw win32_error("ControlService");

		return status;
	}

	// removes the service.  returns true if removed, false if it was marked for removal.
	bool remove() {
		BOOL ret = DeleteService(m_handle);
		if(ret) return true;
		else {
			DWORD err = GetLastError();
			if(err == ERROR_SERVICE_MARKED_FOR_DELETE) return false;
			else throw win32_error("DeleteService", err);
		}
	}

	// starts a service.  returns true if started, false if it was already running.
	bool start() {
		BOOL ret = StartService(m_handle, 0, 0);
		if(ret) return true;
		else {
			DWORD err = GetLastError();
			if(err == ERROR_SERVICE_ALREADY_RUNNING) return false;
			else throw win32_error("StartService", err);
		}
	}
};

};
