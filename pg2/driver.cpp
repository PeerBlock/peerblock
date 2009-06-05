/*
	Copyright (C) 2004-2005 Cory Nelson
	Based on the original work by Tim Leonard

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
	
	CVS Info :
		$Author: phrostbyte $
		$Date: 2005/02/26 05:31:35 $
		$Revision: 1.2 $
*/

#include "stdafx.h"
#include "win32_error.h"
#include "driver.h"

driver::driver() : m_dev(INVALID_HANDLE_VALUE),m_loaded(false),m_started(false),removable(false)
{
}

driver::~driver() {
	this->close(); 
}

void driver::load(const std::wstring &name) {
	wchar_t path[MAX_PATH + 1], *pathend;

	DWORD ret = SearchPath(NULL, name.c_str(), L".sys", MAX_PATH, path, &pathend);
	if(!ret) throw win32_error("SearchPath");

	load(name, path);
}

void driver::load(const std::wstring &name, const std::wstring &file) {
	load(name, file, L"\\\\.\\" + name);
}

void driver::load(const std::wstring &name, const std::wstring &file, const std::wstring &devfile) {
	if(m_loaded) throw std::exception("driver already loaded", 0);

	m_name = name;
	m_file = file;
	m_devfile = devfile;

	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!manager) throw win32_error("OpenSCManager");

	SC_HANDLE service = CreateService(manager, m_name.c_str(), m_name.c_str(),
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		m_file.c_str(), NULL, NULL, NULL, NULL, NULL);

	if(!service) {
		service = OpenService(manager, m_name.c_str(), SERVICE_ALL_ACCESS);

		if(!service) {
			DWORD err = GetLastError();

			CloseServiceHandle(manager);
			throw win32_error("OpenService", err);
		}

		// make sure it has the same path as m_file.

		DWORD bytes;
		BOOL ret = QueryServiceConfig(service, NULL, 0, &bytes);

		DWORD err;
		if(ret || (err = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
			CloseServiceHandle(service);
			CloseServiceHandle(manager);
			throw win32_error("QueryServiceConfig");
		}

		QUERY_SERVICE_CONFIG *qsc = (QUERY_SERVICE_CONFIG*)malloc(bytes);
		if(!qsc) {
			CloseServiceHandle(service);
			CloseServiceHandle(manager);
			throw std::bad_alloc("unable to allocate memory for QueryServiceConfig");
		}

		ret = QueryServiceConfig(service, qsc, bytes, &bytes);
		if(!ret) {
			CloseServiceHandle(service);
			CloseServiceHandle(manager);
			throw win32_error("QueryServiceConfig");
		}

		bool del = _wcsicmp(qsc->lpBinaryPathName, m_file.c_str()) != 0;

		free(qsc);

		// paths don't match, remove service and recreate.
		if(del) {
			// if it's not removable, bail out.
			if(!this->removable) {
				CloseServiceHandle(service);
				CloseServiceHandle(manager);
				throw std::exception("unremovable service mismatch", 0);
			}

			// check if its running
			SERVICE_STATUS status;
			ret = QueryServiceStatus(service, &status);
			if(!ret) {
				DWORD err = GetLastError();

				CloseServiceHandle(service);
				CloseServiceHandle(manager);
				throw win32_error("QueryServiceStatus", err);
			}

			// and stop it if it is.
			if(status.dwCurrentState != SERVICE_STOPPED && status.dwCurrentState != SERVICE_STOP_PENDING) {
				ret = ControlService(service, SERVICE_CONTROL_STOP, &status);
				if(!ret) {
					DWORD err = GetLastError();

					CloseServiceHandle(service);
					CloseServiceHandle(manager);
					throw win32_error("ControlService", err);
				}
			}

			// now delete the service.
			ret = DeleteService(service);
			CloseServiceHandle(service);

			if(!ret && (err = GetLastError()) != ERROR_SERVICE_MARKED_FOR_DELETE) {
				CloseServiceHandle(manager);
				throw win32_error("DeleteService", err);
			}

			// finally recreate it.
			service = CreateService(manager, m_name.c_str(), m_name.c_str(),
				SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
				m_file.c_str(), NULL, NULL, NULL, NULL, NULL);

			if(!service) {
				CloseServiceHandle(manager);
				throw win32_error("CreateService", err);
			}
		}
	}

	SERVICE_STATUS status;
	if(QueryServiceStatus(service, &status)) {
		m_started = (status.dwCurrentState == SERVICE_RUNNING);
	}

	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	m_loaded = true;
}

void driver::close() {
	if(!m_loaded) return;

	stop();

	if(this->removable) {
		SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if(!manager) throw win32_error("OpenSCManager");

		SC_HANDLE service = OpenService(manager, m_name.c_str(), SERVICE_ALL_ACCESS);
		if(!service) {
			DWORD err = GetLastError();

			CloseServiceHandle(manager);
			throw win32_error("OpenService", err);
		}

		DeleteService(service);

		CloseServiceHandle(service);
		CloseServiceHandle(manager);
	}

	m_loaded = false;
}

bool driver::isrunning() {
	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!manager) throw win32_error("OpenSCManager");

	SC_HANDLE service = OpenService(manager, m_name.c_str(), SERVICE_ALL_ACCESS);
	if(!service) {
		DWORD err = GetLastError();

		CloseServiceHandle(manager);
		throw win32_error("OpenService", err);
	}

	SERVICE_STATUS status;

	BOOL ret = QueryServiceStatus(service, &status);
	if(!ret) {
		DWORD err = GetLastError();

		CloseServiceHandle(service);
		CloseServiceHandle(manager);
		throw win32_error("QueryServiceStatus", err);
	}
	
	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	return status.dwCurrentState == SERVICE_RUNNING;
}

void driver::start() {
	if(m_started) return;

	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!manager) throw win32_error("OpenSCManager");

	SC_HANDLE service = OpenService(manager, m_name.c_str(), SERVICE_ALL_ACCESS);
	if(!service) {
		DWORD err = GetLastError();

		CloseServiceHandle(manager);
		throw win32_error("OpenService", err);
	}

	if(!StartService(service, 0, NULL) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
		DWORD err = GetLastError();

		CloseServiceHandle(service);
		CloseServiceHandle(manager);

		throw win32_error("StartService", err);
	}

	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	m_dev = CreateFile(m_devfile.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

	if(m_dev == INVALID_HANDLE_VALUE) {
		throw win32_error("CreateFile");
	}

	m_started = true;
}

void driver::stop() {
	if(!m_started) return;

	if(m_dev != INVALID_HANDLE_VALUE) {
		CloseHandle(m_dev);
		m_dev = INVALID_HANDLE_VALUE;
	}

	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!manager) throw win32_error("OpenSCManager");

	SC_HANDLE service=OpenService(manager, m_name.c_str(), SERVICE_ALL_ACCESS);
	if(!service) {
		DWORD err = GetLastError();

		CloseServiceHandle(manager);
		throw win32_error("OpenService", err);
	}

	SERVICE_STATUS status;
	if(
		(
			!QueryServiceStatus(service, &status) ||
			(
				status.dwCurrentState != SERVICE_STOPPED && 
				status.dwCurrentState != SERVICE_STOP_PENDING
			)
		) &&
		!ControlService(service, SERVICE_CONTROL_STOP, &status))
	{
		DWORD err = GetLastError();

		CloseServiceHandle(service);
		CloseServiceHandle(manager);

		throw win32_error("ControlService", err);
	}

	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	m_started = false;
}

DWORD driver::rawio(DWORD ioctl, void *inbuf, DWORD insize, void *outbuf, DWORD outsize, OVERLAPPED *ovl) {
	DWORD bytes;

	BOOL ret = DeviceIoControl(m_dev, ioctl, inbuf, insize, outbuf, outsize, &bytes, ovl);
	if(ret) return ERROR_SUCCESS;
	else {
		DWORD err = GetLastError();
		return (err == ERROR_IO_PENDING) ? ERROR_SUCCESS : err;
	}
}

DWORD driver::read(DWORD ioctl, void *buf, DWORD size, OVERLAPPED *ovl) {
	return rawio(ioctl, 0, 0, buf, size, ovl);
}

DWORD driver::write(DWORD ioctl, void *buf, DWORD size, OVERLAPPED *ovl) {
	return rawio(ioctl, buf, size, 0, 0, ovl);
}

DWORD driver::cancelio() {
	BOOL ret = CancelIo(m_dev);
	return ret ? ERROR_SUCCESS : GetLastError();
}

DWORD driver::getresult(OVERLAPPED *ovl) {
	DWORD bytes;

	BOOL ret = GetOverlappedResult(m_dev, ovl, &bytes, TRUE);
	return ret ? ERROR_SUCCESS : GetLastError();
}
