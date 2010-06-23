/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009 PeerBlock, LLC

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
#include "resource.h"
using namespace std;

static const LPCTSTR IPFILTER_NAME=_T("IpFilterDriver");
static const LPCTSTR IPFILTER_PATH=_T("System32\\Drivers\\IpFltDrv.sys");

static const wchar_t* PBFILTER_NAME = L"pbfilter";
static const wchar_t* PBFILTER_PATH = L"pbfilter.sys";

pbfilter::pbfilter() 
{
	TRACEI("[pbfilter_nt] [pbfilter]  > Entering routine.");
	m_ipfltdrv.removable = false;

	TRACEI("[pbfilter_nt] [pbfilter]    loading microsoft IpFilterDriver");
	m_ipfltdrv.load(IPFILTER_NAME, IPFILTER_PATH);
	TRACEI("[pbfilter_nt] [pbfilter]    starting IpFilterDriver");
	m_ipfltdrv.start(false);

	wstring p = L"\\??\\" + (path::base_dir() / PBFILTER_PATH).file_str();

	m_filter.removable = true; // so it can be removed if there's a path mismatch.
	TRACEI("[pbfilter_nt] [pbfilter]    loading pbfilter.sys");
	m_filter.load(PBFILTER_NAME);
	if(m_filter.isrunning()) 
	{
		TRACEI("[pbfilter_nt] [pbfilter]    pbfilter.sys already running, now stopping it");
		m_filter.stop();
		TRACEI("[pbfilter_nt] [pbfilter]    stopped pbfilter.sys");
	}

	TRACEI("[pbfilter_nt] [pbfilter]    starting pbfilter.sys");
	m_filter.start();
	m_filter.removable = false; // don't remove it afterward though.

	TRACEI("[pbfilter_nt] [pbfilter]    starting thread");
	start_thread();

	TRACEI("[pbfilter_nt] [pbfilter]  < Leaving routine.");
}

pbfilter::~pbfilter() {
	stop_thread();

	m_filter.removable = true;	// let's delete the driver-service wrapper
	m_filter.close();

	// Don't close the Microsoft IpFilterDriver, because 1) we never acquired a handle to it in 
	// the first place, and it's been known to hang in a STOP_PENDING state if we try to stop
//	m_ipfltdrv.close();
}
