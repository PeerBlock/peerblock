/*
	Copyright (C) 2004-2005 Cory Nelson

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
		$Date: 2005/04/20 03:30:47 $
		$Revision: 1.16 $
*/

#include "stdafx.h"
#include "resource.h"
using namespace std;

static const LPCTSTR IPFILTER_NAME=_T("IpFilterDriver");
static const LPCTSTR IPFILTER_PATH=_T("System32\\Drivers\\IpFltDrv.sys");

static const wchar_t* PGFILTER_NAME = L"pbfilter";
static const wchar_t* PGFILTER_PATH = L"pbfilter.sys";

pgfilter::pgfilter() {
	m_ipfltdrv.removable = false;
	m_ipfltdrv.load(IPFILTER_NAME, IPFILTER_PATH);
	m_ipfltdrv.start();
	
	wstring p = L"\\??\\" + (path::base_dir() / PGFILTER_PATH).file_str();

	m_filter.removable = true; // so it can be removed if there's a path mismatch.
	m_filter.load(PGFILTER_NAME);
	if(m_filter.isrunning()) {
		m_filter.stop();
	}
	m_filter.start();
	m_filter.removable = false; // don't remove it afterward though.

	start_thread();
}

pgfilter::~pgfilter() {
	stop_thread();
	
	m_filter.close();
	m_ipfltdrv.close();
}
