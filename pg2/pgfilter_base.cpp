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
#include "../pgfilter/filter.h"
#include "win32_error.h"

#include "tracelog.h"
extern TraceLog g_tlog;

static const wchar_t* PGFILTER_NAME = L"pbfilter";
static const wchar_t* PGFILTER_PATH = L"pbfilter.sys";

pgfilter_base::pgfilter_base() : m_block(false),m_blockhttp(true),m_blockcount(0),m_allowcount(0) {}



void pgfilter_base::start_thread() 
{
	TRACEV("[pgfilter_base] [start_thread]  > Entering routine.");
	m_runthread = true;

	m_exitevt = CreateEvent(0, TRUE, FALSE, 0);
	if(!m_exitevt) throw win32_error("CreateEvent", 0);

	TRACEI("[pgfilter_base] [start_thread]    creating thread_thunk");
	m_thread = CreateThread(0, 0, thread_thunk, this, 0, 0);
	if(!m_thread) {
		DWORD err = GetLastError();

		CloseHandle(m_exitevt);

		TRACEE("[pgfilter_base] [start_thread]    ERROR creating thread_thunk!!");
		throw win32_error("CreateThread", err);
	}

	TCHAR chBuf[256];
	_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[pgfilter_base] [start_thread]    thread_thunk created with handle:[%p]"), m_thread);
	g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_VERBOSE);

	TRACEV("[pgfilter_base] [start_thread]  > Entering routine.");

} // End of start_thread()



void pgfilter_base::stop_thread() 
{
	TRACEV("[pgfilter_base] [stop_thread]  > Entering routine.");
	m_runthread = false;
	SetEvent(m_exitevt);

	TRACEV("[pgfilter_base] [stop_thread]    waiting for thread to terminate");
	WaitForSingleObject(m_thread, INFINITE);

	CloseHandle(m_thread);
	CloseHandle(m_exitevt);
	TRACEV("[pgfilter_base] [stop_thread]  < Leaving routine.");

} // End of stop_thread()



void pgfilter_base::setblock(bool block) 
{
	TRACEV("[pgfilter_base] [setblock]  > Entering routine.");
	TRACEV("[pgfilter_base] [setblock]    acquiring lock");
	mutex::scoped_lock lock(m_lock);

	if(block != m_block) 
	{
		if (block) 
			TRACEV("[pgfilter_base] [setblock]    resetting m_block to: [true]")
		else 
			TRACEV("[pgfilter_base] [setblock]    resetting m_block to: [false]");

		m_block = block;
		int data = block ? 1 : 0;

		DWORD ret = m_filter.write(IOCTL_PEERGUARDIAN_HOOK, &data, sizeof(data));
		if(ret != ERROR_SUCCESS) throw win32_error("DeviceIoControl", ret);
	}

	TRACEV("[pgfilter_base] [setblock]    mutex leaving scope; releasing lock");
	TRACEV("[pgfilter_base] [setblock]  < Leaving routine");

} // End of setblock()



void pgfilter_base::setblockhttp(bool block) 
{
	TRACEV("[pgfilter_base] [setblockhttp]  > Entering routine.");
	TRACEV("[pgfilter_base] [setblockhttp]    acquiring lock");
	mutex::scoped_lock lock(m_lock);

	if(block != m_blockhttp) 
	{
		if (block)
			TRACEV("[pgfilter_base] [setblockhttp]    resetting m_blockhttp to: [true]")
		else
			TRACEV("[pgfilter_base] [setblockhttp]    resetting m_blockhttp to: [false]");

		m_blockhttp = block;
		int data = block ? 1 : 0;

		DWORD ret = m_filter.write(IOCTL_PEERGUARDIAN_HTTP, &data, sizeof(data));
		if(ret != ERROR_SUCCESS) throw win32_error("DeviceIoControl", ret);
	}

	TRACEV("[pgfilter_base] [setblockhttp]    mutex leaving scope; releasing lock");
	TRACEV("[pgfilter_base] [setblockhttp]  < Leaving routine");

} // End of setblockhttp()



void pgfilter_base::setranges(const p2p::list &ranges, bool block) 
{
	typedef stdext::hash_map<std::wstring, const wchar_t*> hmap_type;

	TRACEV("[pgfilter_base] [setranges]  > Entering routine.");

	hmap_type labels;
	std::vector<wchar_t> labelsbuf;
	unsigned int ipcount = 0;

	TRACEV("[pgfilter_base] [setranges]    initial for-loop");

	for(p2p::list::const_iterator iter = ranges.begin(); iter != ranges.end(); ++iter) 
	{
		const wchar_t* &label = labels[iter->name];

		if(!label) {
			label = (const wchar_t*)labelsbuf.size();

			labelsbuf.insert(labelsbuf.end(), iter->name.begin(), iter->name.end());
			labelsbuf.push_back(L'\0');
		}
	}

	TRACEV("[pgfilter_base] [setranges]    second for-loop");

	for(hmap_type::iterator iter = labels.begin(); iter != labels.end(); ++iter) {
		iter->second = (&labelsbuf.front()) + (std::vector<wchar_t>::size_type)iter->second;
	}

	DWORD pgrsize = (DWORD)offsetof(PGRANGES, ranges[ranges.size()]);

	PGRANGES *pgr = (PGRANGES*)malloc(pgrsize);
	if(!pgr) throw std::bad_alloc("unable to allocate memory for IP ranges");

	pgr->block = block ? 1 : 0;
	pgr->count = (ULONG)ranges.size();

	TRACEV("[pgfilter_base] [setranges]    third for-loop");

	unsigned int i = 0;
	for(p2p::list::const_iterator iter = ranges.begin(); iter != ranges.end(); ++iter) {
		pgr->ranges[i].label = labels[iter->name];
		pgr->ranges[i].start = iter->start.ipl;
		pgr->ranges[i++].end = iter->end.ipl;

		ipcount += iter->end.ipl - iter->start.ipl + 1;
	}

	DWORD ret;
	{
		// TODO: this mutex doesn't appear necessary, and removing it fixes the "hangs at startup" bug; but is it safe?
		//TRACEV("[pgfilter_base] [setranges]    about to acquire mutex");
		//mutex::scoped_lock lock(m_lock);
		//TRACEV("[pgfilter_base] [setranges]    acquired mutex");
		
		pgr->labelsid = block ? (m_blocklabelsid + 1) : (m_allowlabelsid + 1);

		ret = m_filter.write(IOCTL_PEERGUARDIAN_SETRANGES, pgr, pgrsize);
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
		//TRACEV("[pgfilter_base] [setranges]    mutex leaving scope");
	}

	free(pgr);
	
	TRACEV("[pgfilter_base] [setranges]  < Leaving routine.");

	if(ret != ERROR_SUCCESS) {
		throw win32_error("DeviceIoControl", ret);
	}

} // End of setranges()



void pgfilter_base::setactionfunc(const action_function &func) 
{
	TRACEV("[pgfilter_base] [setactionfunc]  > Entering routine.");
	mutex::scoped_lock lock(m_lock);
	m_onaction = func;
	TRACEV("[pgfilter_base] [setactionfunc]  < Entering routine.");

} // End of setactionfunc()



void pgfilter_base::thread_func() 
{
	TRACEV("[pgfilter_base] [thread_func]  > Entering routine.");
	HANDLE evts[2];

	evts[0] = CreateEvent(0, TRUE, FALSE, 0);
	evts[1] = m_exitevt;

	while(m_runthread) {
		OVERLAPPED ovl = {0};
		ovl.hEvent = evts[0];

		PGNOTIFICATION pgn;

		TRACEV("[pgfilter_base] [thread_func]    sending IOCTL_PEERGUARDIAN_GETNOTIFICATION to driver");
		DWORD ret = m_filter.read(IOCTL_PEERGUARDIAN_GETNOTIFICATION, &pgn, sizeof(pgn), &ovl);
		if(ret != ERROR_SUCCESS) {
			TRACEW("[pgfilter_base] [thread_func]    ERROR reading from filter driver");
			if(ret == ERROR_OPERATION_ABORTED) break;
			else {
				std::wcout << L"error: read failed." << std::endl;
			}
		}

		// waiting for overlapped-event (0), or exit-event (1)
		TRACEV("[pgfilter_base] [thread_func]    waiting for notification from driver");
		ret = WaitForMultipleObjects(2, evts, FALSE, INFINITE);
		if(ret < WAIT_OBJECT_0 || ret > (WAIT_OBJECT_0 + 1)) {
			std::wcout << L"error: WaitForMultipleObjects failed." << std::endl;
		}

		TRACEV("[pgfilter_base] [thread_func]    received driver notification");
		if(!m_runthread) {
			TRACEI("[pgfilter_base] [thread_func]    thread terminating");
			m_filter.cancelio();
			m_filter.getresult(&ovl);
			break;
		}

		ret = m_filter.getresult(&ovl);
		if(ret == ERROR_SUCCESS) 
		{
			TRACEV("[pgfilter_base] [thread_func]    m_filter.getresult() succeeded");
			action a;

			if(pgn.action == 0) 
			{
				TRACEV("[pgfilter_base] [thread_func]    action:[blocked]");
				a.type = action::blocked;
			}
			else if(pgn.action == 1) 
			{
				TRACEV("[pgfilter_base] [thread_func]    action:[allowed]");
				a.type = action::allowed;
			}
			else 
			{
				TRACEV("[pgfilter_base] [thread_func]    action:[none]");
				a.type = action::none;
			}

			a.protocol = pgn.protocol;

			if(pgn.source.addr.sa_family == AF_INET) {
				a.src.addr4 = pgn.source.addr4;
				a.dest.addr4 = pgn.dest.addr4;
			}
			else {
				a.src.addr6 = pgn.source.addr6;
				a.dest.addr6 = pgn.dest.addr6;
			}

			{
				TRACEV("[pgfilter_base] [thread_func]    acquiring mutex");
				mutex::scoped_lock lock(m_lock);

				if(pgn.label && ((a.type == action::blocked && pgn.labelsid == m_blocklabelsid) || (a.type == action::allowed && pgn.labelsid == m_allowlabelsid))) {
					a.label = pgn.label;
				}

				if(m_onaction) 
				{
					TRACEV("[pgfilter_base] [thread_func]    performing on_action routine");
					m_onaction(a);
				}

				TRACEV("[pgfilter_base] [thread_func]    mutex going out of scope, releasing");
			}
		}
		else if(ret == ERROR_OPERATION_ABORTED) 
		{
			TRACEE("[pgfilter_base] [thread_func]    ERROR OPERATION_ABORTED");
			break;
		}
		else 
		{
			TRACEW("[pgfilter_base] [thread_func]    ERROR:  getresult failed");
			std::wcout << L"error: getresult failed." << std::endl;
		}

		TRACEV("[pgfilter_base] [thread_func]    end of main loop, resetting driver-wait event");
		ResetEvent(evts[0]);
	}

	CloseHandle(evts[0]);
	TRACEV("[pgfilter_base] [thread_func]  < Leaving routine.");
}



DWORD WINAPI pgfilter_base::thread_thunk(void *arg) 
{
	TRACEV("[pgfilter_base] [thread_thunk]  > Entering routine.");
	reinterpret_cast<pgfilter_base*>(arg)->thread_func();
	TRACEV("[pgfilter_base] [thread_thunk]  < Leaving routine.");
	return 0;

} // End of thread_thunk()
