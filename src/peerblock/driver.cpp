/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2011 PeerBlock, LLC
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

*/

#include "stdafx.h"
#include "win32_error.h"
#include "driver.h"

#include "tracelog.h"
extern TraceLog g_tlog;


driver::driver() : m_dev(INVALID_HANDLE_VALUE),m_loaded(false),m_started(false),removable(false)
{
}

driver::~driver() {
	this->close();
}

void driver::load(const std::wstring &name)
{
	wchar_t path[MAX_PATH + 1], *pathend;

	{
		tstring strBuf = boost::str(tformat(_T("[driver] [load(1)]    preparing to load driver - name: [%1%]")) % name.c_str() );
		TRACEBUFI(strBuf);
	}

	DWORD ret = SearchPath(NULL, name.c_str(), L".sys", MAX_PATH, path, &pathend);
	if(!ret) throw win32_error("SearchPath");

	load(name, path);
}

void driver::load(const std::wstring &name, const std::wstring &file)
{
	{
		tstring strBuf = boost::str(tformat(_T("[driver] [load(2)]    preparing to load driver - name: [%1%], file: [%2%]"))
			% name.c_str() % file.c_str() );
		TRACEBUFI(strBuf);
	}

	load(name, file, L"\\\\.\\" + name);
}

void driver::load(const std::wstring &name, const std::wstring &file, const std::wstring &devfile)
{
	TRACEV("[driver] [load(3)]  > Entering routine.");
	if(m_loaded) throw std::exception("driver already loaded", 0);

	{
		tstring strBuf = boost::str(tformat(_T("[driver] [load(3)]    loading driver - name: [%1%], file: [%2%], devfile: [%3%]"))
			% name.c_str() % file.c_str() % devfile.c_str() );
		TRACEBUFI(strBuf);
	}

	m_name = name;
	m_file = file;
	m_devfile = devfile;

	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!manager) throw win32_error("OpenSCManager");

	SC_HANDLE service = CreateService(manager, m_name.c_str(), m_name.c_str(),
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		m_file.c_str(), NULL, NULL, NULL, NULL, NULL);

	if(!service)
	{
		DWORD err = 0;

		TRACEERR("[driver] [load(3)]", L"driver not currently installed as service", err = GetLastError());

		err = 0;
		service = OpenService(manager, m_name.c_str(), SERVICE_ALL_ACCESS);
		if(!service)
		{
			TRACEERR("[driver] [load(3)]", L"opening service", err = GetLastError());
			CloseServiceHandle(manager);
			throw win32_error("OpenService", err);
		}

		// make sure it has the same path as m_file.

		DWORD bytes;
		BOOL ret = QueryServiceConfig(service, NULL, 0, &bytes);

		if(ret || (err = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
		{
			TRACEERR("[driver] [load(3)]", L"calling QueryServiceConfig", err);
			CloseServiceHandle(service);
			CloseServiceHandle(manager);
			throw win32_error("QueryServiceConfig", err);
		}
		err = 0; // reset error-code, so that nobody else accidentally trips over it.

		QUERY_SERVICE_CONFIG *qsc = (QUERY_SERVICE_CONFIG*)malloc(bytes);
		if(!qsc)
		{
			TRACEERR("[driver] [load(3)]", L"allocating memory for QueryServiceConfig", err = GetLastError());
			CloseServiceHandle(service);
			CloseServiceHandle(manager);
			throw std::bad_alloc();
		}

		ret = QueryServiceConfig(service, qsc, bytes, &bytes);
		if(!ret)
		{
			TRACEERR("[driver] [load(3)]", L"second call to QueryServiceConfig", err = GetLastError());
			CloseServiceHandle(service);
			CloseServiceHandle(manager);
			throw win32_error("QueryServiceConfig", err);
		}

		bool del = _wcsicmp(qsc->lpBinaryPathName, m_file.c_str()) != 0;
		if (del)
		{
			// check for e.g. \??\C:\WINDOWS\System32\DRIVERS\ipfltdrv.sys
			wchar_t * descr = _wgetenv(L"WINDIR");
			if (descr)
			{
				wstring strFullPath(L"\\??\\");
				strFullPath += descr;
				strFullPath += L"\\";
				strFullPath += m_file;

				tstring strBuf = boost::str(tformat(_T("[driver] [load(3)]    comparing against driver full-path:[%1%]")) % strFullPath );
				TRACEBUFI(strBuf);

				del = _wcsicmp(qsc->lpBinaryPathName, strFullPath.c_str()) != 0;
			}
			else
			{
				TRACEW("[driver] [load(3)]    WINDIR environment variable not found!");
			}
		}

		tstring strBuf = boost::str(tformat(_T("[driver] [load(3)]    service name: [%1%], file name: [%2%]"))
			% qsc->lpBinaryPathName % m_file.c_str() );
		TRACEBUFI(strBuf);

		free(qsc);

		// paths don't match, remove service and recreate.
		if(del)
		{
			TRACEW("[driver] [load(3)]    paths don't match, removing and recreating driver-service");
			TCHAR buf[128];

			// if it's not removable, bail out.
			if(!this->removable)
			{
				TRACEC("[driver] [load(3)]    ERROR trying to remove driver-service");
				CloseServiceHandle(service);
				CloseServiceHandle(manager);
				throw std::exception("unremovable service mismatch", 0);
			}

			// check if its running
			SERVICE_STATUS status;
			ret = QueryServiceStatus(service, &status);
			if(!ret)
			{
				TRACEERR("[driver] [load(3)]", L"ERROR calling QueryServiceStatus", err = GetLastError());
				CloseServiceHandle(service);
				CloseServiceHandle(manager);
				throw win32_error("QueryServiceStatus", err);
			}

			// and stop it if it is.
			switch(status.dwCurrentState)
			{
				case SERVICE_STOPPED:
					TRACEI("[driver] [load(3)]    service state: [SERVICE_STOPPED]");
					break;

				case SERVICE_START_PENDING:
					TRACEI("[driver] [load(3)]    service state: [SERVICE_START_PENDING]");
					break;

				case SERVICE_STOP_PENDING:
					TRACEI("[driver] [load(3)]    service state: [SERVICE_STOP_PENDING]");
					break;

				case SERVICE_RUNNING:
					TRACEI("[driver] [load(3)]    service state: [SERVICE_RUNNING]");
					break;

				case SERVICE_CONTINUE_PENDING:
					TRACEI("[driver] [load(3)]    service state: [SERVICE_CONTINUE_PENDING]");
					break;

				case SERVICE_PAUSE_PENDING:
					TRACEI("[driver] [load(3)]    service state: [SERVICE_PAUSE_PENDING]");
					break;

				case SERVICE_PAUSED:
					TRACEI("[driver] [load(3)]    service state: [SERVICE_PAUSED]");
					break;

				default:
					swprintf_s(buf, _countof(buf), L"[driver] [load(3)]  * ERROR: Unknown service state: [%d]", status.dwCurrentState);
					TRACEBUFE(buf);
					break;
			}
			if(status.dwCurrentState != SERVICE_STOPPED && status.dwCurrentState != SERVICE_STOP_PENDING)
			{
				ret = ControlService(service, SERVICE_CONTROL_STOP, &status);
				if(!ret)
				{
					TRACEERR("[driver] [load(3)]", L"ERROR stopping driver-service", err = GetLastError());
					CloseServiceHandle(service);
					CloseServiceHandle(manager);
					throw win32_error("ControlService", err);
				}
			}

			// now delete the service.
			ret = DeleteService(service);
			err = GetLastError();
			CloseServiceHandle(service);

			if(!ret && err != ERROR_SERVICE_MARKED_FOR_DELETE)
			{
				TRACEERR("[driver] [load(3)]", L"ERROR deleting driver-service", err);
				CloseServiceHandle(manager);
				throw win32_error("DeleteService", err);
			}

			// finally recreate it.
			service = CreateService(manager, m_name.c_str(), m_name.c_str(),
				SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
				m_file.c_str(), NULL, NULL, NULL, NULL, NULL);

			if(!service && (err = GetLastError()) == ERROR_SERVICE_MARKED_FOR_DELETE)
			{
				TRACEW("[driver] [load(3)]    Service marked for delete; trying closing/reopening SCM");
				CloseServiceHandle(manager);
				Sleep(10000);
				manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

				if (!manager)
				{
					TRACEERR("[driver] [load(3)]", L"ERROR while re-opening SCM", err = GetLastError());
					throw win32_error("OpenSCManager 2", err);
				}

				service = CreateService(manager, m_name.c_str(), m_name.c_str(),
					SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
					m_file.c_str(), NULL, NULL, NULL, NULL, NULL);
			}

			if(!service)
			{
				TRACEERR("[driver] [load(3)]", L"ERROR re-creating driver-service", err = GetLastError());
				CloseServiceHandle(manager);
				throw win32_error("CreateService", err);
			}

			TRACEI("[driver] [load(3)]    finished re-creating driver-service");
		}
	}

	SERVICE_STATUS status;
	if(QueryServiceStatus(service, &status))
	{
		if (status.dwCurrentState == SERVICE_RUNNING)
		{
			TRACES("[driver] [load(3)]    Driver-service is running");
			m_started = true;
		}
		else if (status.dwCurrentState == SERVICE_START_PENDING)
		{
			TRACEI("[driver] [load(3)]    Driver-service is starting");
		}
		else
		{
			TRACEI("[driver] [load(3)]    driver-service not running");
			TCHAR buf[128];
			swprintf_s(buf, _countof(buf), L"[driver] [load(3)]    - service state: [%d]", status.dwCurrentState);
			TRACEBUFW(buf);
		}
	}

	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	m_loaded = true;
	TRACEV("[driver] [load(3)]  < Leaving routine.");
}



void driver::close()
{
	TRACEI("[driver] [close]  > Entering routine.");

	tstring strBuf = boost::str(tformat(_T("[driver] [close]   closing driver - name: [%1%]")) % m_name.c_str() );
	TRACEBUFI(strBuf);

	if(!m_loaded)
	{
		TRACEW("[driver] [close]    Tried to close driver, but it wasn't loaded.");
		TRACEI("[driver] [close]  < Leaving routine (without doing anything).");
		return;
	}

	stop();

	if(this->removable)
	{
		TRACEV("[driver] [close]    driver is removable");
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

	TRACEI("[driver] [close]    unloaded driver");
	m_loaded = false;

	TRACEI("[driver] [close]  < Leaving routine.");

} // End of close()



bool driver::isrunning()
{
	TRACEV("[driver] [isrunning]  > Entering routine.");
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

	TRACEV("[driver] [isrunning]  < Leaving routine.");
	return status.dwCurrentState == SERVICE_RUNNING;

} // End of isrunning()



void driver::start(bool _gethandle)
{
	TRACEV("[driver] [start]  > Entering routine.");
	if(m_started)
	{
		TRACEI("[driver] [start]    driver already started");
		return;
	}

	TRACEI("[driver] [start]    starting driver");

	DWORD err = 0;

	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!manager)
	{
		TRACEERR("[driver] [start]", L"ERROR: OpenSCManager", err = GetLastError());
		throw win32_error("OpenSCManager");
	}

	SC_HANDLE service = OpenService(manager, m_name.c_str(), SERVICE_ALL_ACCESS);
	if(!service)
	{
		CloseServiceHandle(manager);
		TRACEERR("[driver] [start]", L"ERROR: OpenService", err = GetLastError());
		throw win32_error("OpenService", err);
	}

	if(!StartService(service, 0, NULL) && (err = GetLastError()) != ERROR_SERVICE_ALREADY_RUNNING)
	{
		bool startFailed = true;

		if (err == ERROR_SERVICE_DATABASE_LOCKED)
		{
			int numRetries = 0;

			do
			{
				TRACEW("[driver] [start]    experiencing ERROR_SERVICE_DATABASE_LOCKED condition; waiting 10 seconds and trying again");
				Sleep(10000); // 10 seconds
				err = 0;

				if (!StartService(service, 0, NULL) && (err = GetLastError()) != ERROR_SERVICE_ALREADY_RUNNING)
					// still having problems
					++numRetries;
				else
					// either success, or another error
					break;

			} while (err == ERROR_SERVICE_DATABASE_LOCKED && numRetries < 6);

			if (numRetries < 6 && (err == 0 || err == ERROR_SERVICE_ALREADY_RUNNING))
			{
				startFailed = false;
				TRACES("[driver] [start]    successfully recovered from ERROR_SERVICE_DATABASE_LOCKED condition");
			}
			else if (err == ERROR_SERVICE_DATABASE_LOCKED)
			{
				TRACEE("[driver] [start]    cannot recover from ERROR_SERVICE_DATABASE_LOCKED condition; giving up");
			}
			else
			{
				TRACEE("[driver] [start]    cannot recover from ERROR_SERVICE_DATABASE_LOCKED condition; another error surfaced; giving up");
			}
		}

		if (startFailed)
		{
			CloseServiceHandle(service);
			CloseServiceHandle(manager);

			TRACEERR("[driver] [start]", L"ERROR: StartService", err);
			throw win32_error("StartService", err);
		}
	}

	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	if (_gethandle)
	{
		tstring strBuf = boost::str(tformat(_T("[driver] [start]    getting handle to driver - devfile: [%1%]")) % m_devfile.c_str() );
		TRACEBUFI(strBuf);

		m_dev = CreateFile(m_devfile.c_str(), GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

		if(m_dev == INVALID_HANDLE_VALUE)
		{
			TRACEERR("[driver] [start]", L"ERROR: CreateFile", err = GetLastError());
			throw win32_error("CreateFile");
		}

		m_stoppable = true;
	}
	else
	{
		m_stoppable = false; // should only be false for MS IpFilterDriver
		TRACEI("[driver] [start]    flagging driver as not stoppable");
	}

	TRACEI("[driver] [start]    started driver");
	m_started = true;

	TRACEV("[driver] [start]  < Leaving routine.");

} // End of start()



void driver::stop()
{
	TRACEI("[driver] [stop]  > Entering routine.");

	if(!m_started)
	{
		TRACEW("[driver] [stop]    Tried to stop driver, but it is not started");
		TRACEI("[driver] [stop]  < Leaving routine (without doing anything).");
		return;
	}
	else if (!m_stoppable)
	{
		TRACEW("[driver] [stop]    Tried to stop driver, but it is not flagged as a stoppable driver");
		TRACEI("[driver] [stop]  < Leaving routine (without doing anything).");
		return;
	}

	TRACEI("[driver] [stop]    stopping driver");

	if(m_dev != INVALID_HANDLE_VALUE) {
		CloseHandle(m_dev);
		m_dev = INVALID_HANDLE_VALUE;
	}

	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!manager)
	{
		TRACEE("[driver] [stop]    ERROR:  Can't OpenSCManager");
		throw win32_error("OpenSCManager");
	}

	SC_HANDLE service=OpenService(manager, m_name.c_str(), SERVICE_ALL_ACCESS);
	if(!service) {
		DWORD err = GetLastError();

		CloseServiceHandle(manager);
		TRACEE("[driver] [stop]    ERROR:  Can't OpenService");
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

		TRACEE("[driver] [stop]    ERROR:  Can't ControlService");
		throw win32_error("ControlService", err);
	}

	CloseServiceHandle(service);
	CloseServiceHandle(manager);

	TRACEI("[driver] [stop]    driver has been stopped");
	m_started = false;

	TRACEI("[driver] [stop]  < Leaving routine.");

} // End of stop()



DWORD driver::rawio(DWORD ioctl, void *inbuf, DWORD insize, void *outbuf, DWORD outsize, OVERLAPPED *ovl)
{
	TRACEV("[driver] [rawio]  > Entering routine.");
	DWORD bytes;

	BOOL ret = DeviceIoControl(m_dev, ioctl, inbuf, insize, outbuf, outsize, &bytes, ovl);
	if(ret)
	{
		TRACEV("[driver] [rawio]    Successfully sent IOCTL to driver");
		TRACEV("[driver] [rawio]  < Leaving routine.");
		return ERROR_SUCCESS;
	}
	else
	{
		DWORD err = GetLastError();
		if (err == ERROR_IO_PENDING)
		{
			TRACEV("[driver] [rawio]    IO_PENDING while sending IOCTL to driver");
			err = ERROR_SUCCESS;
		}
		else
		{
			TRACEE("[driver] [rawio]    ERROR sending IOCTL to driver");
		}

		TRACEV("[driver] [rawio]  < Leaving routine.");
		return err;
	}

} // End of rawio()



DWORD driver::read(DWORD ioctl, void *buf, DWORD size, OVERLAPPED *ovl)
{
	TRACEV("[driver] [read]    Entering routine.");
	return rawio(ioctl, 0, 0, buf, size, ovl);

} // End of read()



DWORD driver::write(DWORD ioctl, void *buf, DWORD size, OVERLAPPED *ovl)
{
	TRACEV("[driver] [write]    Entering routine.");
	return rawio(ioctl, buf, size, 0, 0, ovl);

} // End of write()



DWORD driver::cancelio()
{
	TRACEV("[driver] [cancelio]    Entering routine.");
	BOOL ret = CancelIo(m_dev);
	return ret ? ERROR_SUCCESS : GetLastError();

} // End of cancelio()



DWORD driver::getresult(OVERLAPPED *ovl)
{
	TRACEV("[driver] [getresult]    Entering routine.");
	DWORD bytes;

	BOOL ret = GetOverlappedResult(m_dev, ovl, &bytes, TRUE);
	return ret ? ERROR_SUCCESS : GetLastError();

} // End of getresult()
