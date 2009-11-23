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
#include "driver.hpp"
#include "services.hpp"
#include "win32_error.hpp"

driver::driver() : m_loaded(false),m_started(false),removable(false)
{
}

driver::~driver() {
	close(); 
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
	if(m_loaded) throw std::runtime_error("driver already loaded");

	m_name = name;
	m_file = file;
	m_devfile = devfile;

	services::manager manager;

	services::service service;

	while(!service.create(manager, m_name.c_str(), m_name.c_str(),
								 SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
								 m_file.c_str()))
	{
		service.open(manager, m_name.c_str());

		boost::shared_ptr<QUERY_SERVICE_CONFIG> qsc = service.query_config();

		std::wstring fupper = path::toupper(m_file);
		std::wstring supper = path::toupper(qsc->lpBinaryPathName);

		// if paths don't match, remove service and recreate.
		if(fupper != supper) {
			// if it's not removable, bail out.
			if(!this->removable) {
				throw std::runtime_error("unremovable service mismatch");
			}

			// check if its running
			SERVICE_STATUS status = service.query_status();

			// and stop it if it is.
			if(status.dwCurrentState != SERVICE_STOPPED && status.dwCurrentState != SERVICE_STOP_PENDING) {
				status = service.control(SERVICE_CONTROL_STOP);
			}

			// now delete the service.
			service.remove();
			service.close();
		}
	}

	m_started = service.query_status().dwCurrentState == SERVICE_RUNNING;
	m_loaded = true;
}

void driver::close() {
	if(!m_loaded) return;

	stop();

	if(this->removable) {
		services::manager manager;
		services::service service;

		service.open(manager, m_name.c_str());
		service.remove();
	}

	m_loaded = false;
}

bool driver::isrunning() {
	services::manager manager;
	services::service service;

	service.open(manager, m_name.c_str());
	return service.query_status().dwCurrentState == SERVICE_RUNNING;
}

void driver::start() {
	if(m_started) return;

	services::manager manager;
	services::service service;

	service.open(manager, m_name.c_str());
	service.start();

	m_dev.open(m_devfile.c_str(), GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED);
	m_started = true;
}

void driver::stop() {
	if(!m_started) return;

	m_dev.close();

	services::manager manager;
	services::service service;

	service.open(manager, m_name.c_str());

	SERVICE_STATUS status = service.query_status();

	if(status.dwCurrentState != SERVICE_STOPPED && status.dwCurrentState != SERVICE_STOP_PENDING) {
		service.control(SERVICE_CONTROL_STOP);
	}

	m_started = false;
}
