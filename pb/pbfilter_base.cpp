/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009 PeerBlock, LLC
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
#include "../pbfilter/filter.h"
#include "win32_error.h"

#include "tracelog.h"
extern TraceLog g_tlog;

static const wchar_t* PBFILTER_NAME = L"pbfilter";
static const wchar_t* PBFILTER_PATH = L"pbfilter.sys";

pbfilter_base::pbfilter_base() : m_block(false),m_blockhttp(true),m_blockcount(0),m_allowcount(0) {}



void pbfilter_base::start_thread() 
{
	TRACEV("[pbfilter_base] [start_thread]  > Entering routine.");
	m_runthread = true;

	m_exitevt = CreateEvent(0, TRUE, FALSE, 0);
	if(!m_exitevt) throw win32_error("CreateEvent", 0);

	TRACEI("[pbfilter_base] [start_thread]    creating thread_thunk");
	m_thread = CreateThread(0, 0, thread_thunk, this, 0, 0);
	if(!m_thread) {
		DWORD err = GetLastError();

		CloseHandle(m_exitevt);

		TRACEE("[pbfilter_base] [start_thread]    ERROR creating thread_thunk!!");
		throw win32_error("CreateThread", err);
	}

	TCHAR chBuf[256];
	_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[pbfilter_base] [start_thread]    thread_thunk created with handle:[%p]"), m_thread);
	g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_VERBOSE);

	TRACEV("[pbfilter_base] [start_thread]  < Leaving routine.");

} // End of start_thread()



void pbfilter_base::stop_thread() 
{
	TRACEV("[pbfilter_base] [stop_thread]  > Entering routine.");
	m_runthread = false;
	SetEvent(m_exitevt);

	TRACEV("[pbfilter_base] [stop_thread]    waiting for thread to terminate");
	WaitForSingleObject(m_thread, INFINITE);

	CloseHandle(m_thread);
	CloseHandle(m_exitevt);
	TRACEV("[pbfilter_base] [stop_thread]  < Leaving routine.");

} // End of stop_thread()



void pbfilter_base::setblock(bool block) 
{
	TRACEV("[pbfilter_base] [setblock]  > Entering routine.");
	TRACEI("[pbfilter_base] [setblock]    acquiring lock");
	mutex::scoped_lock lock(m_blocklock);

	if(block != m_block) 
	{
		if (block) 
			TRACEI("[pbfilter_base] [setblock]    resetting m_block to: [true]")
		else 
			TRACEI("[pbfilter_base] [setblock]    resetting m_block to: [false]");

		m_block = block;
		int data = block ? 1 : 0;

		TRACEI("[pbfilter_base] [setblock]    sending block request to driver...")
		DWORD ret = m_filter.write(IOCTL_PEERBLOCK_HOOK, &data, sizeof(data));
		TRACEI("[pbfilter_base] [setblock]    ...block request sent")
		if(ret != ERROR_SUCCESS) 
		{
			TRACEERR("[pbfilter_base] [setblock]", L"sending block request to driver", ret);
			throw win32_error("DeviceIoControl", ret);
		}
	}
	else
	{
		TRACEI("[pbfilter_base] [setblock]    block already set to that requested, ignoring request")
	}

	TRACEI("[pbfilter_base] [setblock]    mutex leaving scope; releasing lock");
	TRACEV("[pbfilter_base] [setblock]  < Leaving routine");

} // End of setblock()



void pbfilter_base::setblockhttp(bool block) 
{
	TRACEV("[pbfilter_base] [setblockhttp]  > Entering routine.");
	TRACEV("[pbfilter_base] [setblockhttp]    acquiring lock");
	mutex::scoped_lock lock(m_blocklock);

	if(block != m_blockhttp) 
	{
		if (block)
			TRACEV("[pbfilter_base] [setblockhttp]    resetting m_blockhttp to: [true]")
		else
			TRACEV("[pbfilter_base] [setblockhttp]    resetting m_blockhttp to: [false]");

		m_blockhttp = block;
		int data = block ? 1 : 0;

		DWORD ret = m_filter.write(IOCTL_PEERBLOCK_HTTP, &data, sizeof(data));
		if(ret != ERROR_SUCCESS) throw win32_error("DeviceIoControl", ret);
	}

	TRACEV("[pbfilter_base] [setblockhttp]    mutex leaving scope; releasing lock");
	TRACEV("[pbfilter_base] [setblockhttp]  < Leaving routine");

} // End of setblockhttp()



void pbfilter_base::setranges(const p2p::list &ranges, bool block) 
{
	typedef stdext::hash_map<std::wstring, const wchar_t*> hmap_type;

	TRACEV("[pbfilter_base] [setranges]  > Entering routine.");

	hmap_type labels;
	std::vector<wchar_t> labelsbuf;
	unsigned int ipcount = 0;

	TRACEV("[pbfilter_base] [setranges]    initial for-loop");

	for(p2p::list::const_iterator iter = ranges.begin(); iter != ranges.end(); ++iter) 
	{
		const wchar_t* &label = labels[iter->name];

		if(!label) {
			label = (const wchar_t*)labelsbuf.size();

			labelsbuf.insert(labelsbuf.end(), iter->name.begin(), iter->name.end());
			labelsbuf.push_back(L'\0');
		}
	}

	TRACEV("[pbfilter_base] [setranges]    second for-loop");

	for(hmap_type::iterator iter = labels.begin(); iter != labels.end(); ++iter) {
		iter->second = (&labelsbuf.front()) + (std::vector<wchar_t>::size_type)iter->second;
	}

	DWORD pbrsize = (DWORD)offsetof(PBRANGES, ranges[ranges.size()]);

	PBRANGES *pbr = (PBRANGES*)malloc(pbrsize);
	if(!pbr) throw std::bad_alloc("unable to allocate memory for IP ranges");

	pbr->block = block ? 1 : 0;
	pbr->count = (ULONG)ranges.size();

	TRACEV("[pbfilter_base] [setranges]    third for-loop");

	unsigned int i = 0;
	for(p2p::list::const_iterator iter = ranges.begin(); iter != ranges.end(); ++iter) {
		pbr->ranges[i].label = labels[iter->name];
		pbr->ranges[i].start = iter->start.ipl;
		pbr->ranges[i++].end = iter->end.ipl;

		ipcount += iter->end.ipl - iter->start.ipl + 1;
	}

	DWORD ret;
	{
		// TODO: this mutex doesn't appear necessary, and removing it fixes the "hangs at startup" bug; but is it safe?
		//TRACEV("[pbfilter_base] [setranges]    about to acquire mutex");
		//mutex::scoped_lock lock(m_lock);
		//TRACEV("[pbfilter_base] [setranges]    acquired mutex");
		
		pbr->labelsid = block ? (m_blocklabelsid + 1) : (m_allowlabelsid + 1);

		ret = m_filter.write(IOCTL_PEERBLOCK_SETRANGES, pbr, pbrsize);
		if(ret == ERROR_SUCCESS) {
			if(block) {
				++m_blocklabelsid;
				m_blockcount = ipcount;
				m_blocklabels.swap(labelsbuf);
			}
			else {
				++m_allowlabelsid;
				m_allowcount = ipcount;
				m_allowlabels.swap(labelsbuf);
			}
		}
		//TRACEV("[pbfilter_base] [setranges]    mutex leaving scope");
	}

	free(pbr);
	
	TRACEV("[pbfilter_base] [setranges]  < Leaving routine.");

	if(ret != ERROR_SUCCESS) {
		throw win32_error("DeviceIoControl", ret);
	}

} // End of setranges()



void pbfilter_base::setports(const std::set<ULONG> ports)
{
	ULONG *nports = (ULONG*) malloc(sizeof(ULONG) * ports.size());
	ULONG pcount = 0;

	TRACEI("[pbfilter_base] [setports]  > Entering routine.");
	for (set<ULONG>::const_iterator it = ports.begin(); it != ports.end(); it++)
	{
		nports[pcount++] = *it;
		TRACEI("[pbfilter_base] [setports]    ...iter...");
	}

	TRACEI("[pbfilter_base] [setports]    finished parsing ports");
	DWORD ret = m_filter.write(IOCTL_PEERBLOCK_SETPORTS, nports, (sizeof(ULONG) * ports.size()));

	if (ret != ERROR_SUCCESS)
	{
		TRACEERR("[pbfilter_base] [setports]", L"Problems talking to driver", ret);
		throw win32_error("DeviceIoControl", ret);
	}

	TRACEI("[pbfilter_base] [setports]  < Leaving Routine.");

} // End of setports()



void pbfilter_base::setactionfunc(const action_function &func) 
{
	TRACEV("[pbfilter_base] [setactionfunc]  > Entering routine.");
	mutex::scoped_lock lock(m_lock);
	m_onaction = func;
	TRACEV("[pbfilter_base] [setactionfunc]  < Entering routine.");

} // End of setactionfunc()



void pbfilter_base::thread_func() 
{
	TRACEV("[pbfilter_base] [thread_func]  > Entering routine.");
	HANDLE evts[2];

	evts[0] = CreateEvent(0, TRUE, FALSE, 0);
	evts[1] = m_exitevt;

	while(m_runthread) {
		OVERLAPPED ovl = {0};
		ovl.hEvent = evts[0];

		PBNOTIFICATION pbn;

		TRACEV("[pbfilter_base] [thread_func]    sending IOCTL_PEERBLOCK_GETNOTIFICATION to driver");
		DWORD ret = m_filter.read(IOCTL_PEERBLOCK_GETNOTIFICATION, &pbn, sizeof(pbn), &ovl);
		if(ret != ERROR_SUCCESS) {
			TRACEW("[pbfilter_base] [thread_func]    ERROR reading from filter driver");
			if(ret == ERROR_OPERATION_ABORTED) break;
			else {
				std::wcout << L"error: read failed." << std::endl;
			}
		}

		// waiting for overlapped-event (0), or exit-event (1)
		TRACEV("[pbfilter_base] [thread_func]    waiting for notification from driver");
		ret = WaitForMultipleObjects(2, evts, FALSE, INFINITE);
		if(ret < WAIT_OBJECT_0 || ret > (WAIT_OBJECT_0 + 1)) {
			std::wcout << L"error: WaitForMultipleObjects failed." << std::endl;
		}

		TRACEV("[pbfilter_base] [thread_func]    received driver notification");
		if(!m_runthread) {
			TRACEI("[pbfilter_base] [thread_func]    thread terminating");
			m_filter.cancelio();
			m_filter.getresult(&ovl);
			break;
		}

		ret = m_filter.getresult(&ovl);
		if(ret == ERROR_SUCCESS) 
		{
			TRACEV("[pbfilter_base] [thread_func]    m_filter.getresult() succeeded");
			action a;

			if(pbn.action == 0) 
			{
				TRACEV("[pbfilter_base] [thread_func]    action:[blocked]");
				a.type = action::blocked;
			}
			else if(pbn.action == 1) 
			{
				TRACEV("[pbfilter_base] [thread_func]    action:[allowed]");
				a.type = action::allowed;
			}
			else 
			{
				TRACEV("[pbfilter_base] [thread_func]    action:[none]");
				a.type = action::none;
			}

			a.protocol = pbn.protocol;

			if(pbn.source.addr.sa_family == AF_INET) {
				a.src.addr4 = pbn.source.addr4;
				a.dest.addr4 = pbn.dest.addr4;
			}
			else {
				a.src.addr6 = pbn.source.addr6;
				a.dest.addr6 = pbn.dest.addr6;
			}

			{
				TRACEV("[pbfilter_base] [thread_func]    acquiring mutex");
				mutex::scoped_lock lock(m_lock);

				if(pbn.label && ((a.type == action::blocked && pbn.labelsid == m_blocklabelsid) || (a.type == action::allowed && pbn.labelsid == m_allowlabelsid))) {
					a.label = pbn.label;
				}

				if(m_onaction) 
				{
					TRACEV("[pbfilter_base] [thread_func]    performing on_action routine");
					m_onaction(a);
				}

				TRACEV("[pbfilter_base] [thread_func]    mutex going out of scope, releasing");
			}
		}
		else if(ret == ERROR_OPERATION_ABORTED) 
		{
			TRACEE("[pbfilter_base] [thread_func]    ERROR OPERATION_ABORTED");
			break;
		}
		else 
		{
			TRACEW("[pbfilter_base] [thread_func]    ERROR:  getresult failed");
			std::wcout << L"error: getresult failed." << std::endl;
		}

		TRACEV("[pbfilter_base] [thread_func]    end of main loop, resetting driver-wait event");
		ResetEvent(evts[0]);
	}

	CloseHandle(evts[0]);
	TRACEV("[pbfilter_base] [thread_func]  < Leaving routine.");
}



DWORD WINAPI pbfilter_base::thread_thunk(void *arg) 
{
	TRACEV("[pbfilter_base] [thread_thunk]  > Entering routine.");
	reinterpret_cast<pbfilter_base*>(arg)->thread_func();
	TRACEV("[pbfilter_base] [thread_thunk]  < Leaving routine.");
	return 0;

} // End of thread_thunk()
