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
#include "../pgfilter/filter.h"
#include "win32_error.hpp"
#include "pgfilter.hpp"

pgfilter_base::pgfilter_base() : m_block(false),m_blockhttp(true),m_blockcount(0),m_allowcount(0) {}

void pgfilter_base::start_thread() {
	m_runthread = true;

	m_exitevt = CreateEvent(0, TRUE, FALSE, 0);
	if(!m_exitevt) throw win32_error("CreateEvent", 0);

	m_thread = CreateThread(0, 0, thread_thunk, this, 0, 0);
	if(!m_thread) {
		DWORD err = GetLastError();

		CloseHandle(m_exitevt);
		throw win32_error("CreateThread", err);
	}
}

void pgfilter_base::stop_thread() {
	m_runthread = false;
	SetEvent(m_exitevt);

	WaitForSingleObject(m_thread, INFINITE);

	CloseHandle(m_thread);
	CloseHandle(m_exitevt);
}

void pgfilter_base::setblock(bool block) {
	mutex::scoped_lock lock(m_lock);

	if(block != m_block) {
		m_block = block;

		int data = block ? 1 : 0;

		m_filter.write(IOCTL_PEERGUARDIAN_HOOK, &data, sizeof(data));
	}
}

void pgfilter_base::setblockhttp(bool block) {
	mutex::scoped_lock lock(m_lock);

	if(block != m_blockhttp) {
		m_blockhttp = block;

		int data = block ? 1 : 0;

		m_filter.write(IOCTL_PEERGUARDIAN_HTTP, &data, sizeof(data));
	}
}

void pgfilter_base::setranges(const p2p::list &ranges, bool block) {
	p2p::static_list list(ranges);
	unsigned int ipcount = 0;

	DWORD pgrsize = (DWORD)offsetof(PGRANGES, ranges[list.m_list4.size()]);
	DWORD pgr6size = (DWORD)offsetof(PGRANGES, ranges6[list.m_list6.size()]);

	boost::scoped_array<char> rbuf(new char[pgrsize + pgr6size]);

	PGRANGES *pgr = (PGRANGES*)rbuf.get();
	PGRANGES *pgr6 = (PGRANGES*)rbuf.get() + pgrsize;

	pgr->protocol = AF_INET;
	pgr6->protocol = AF_INET6;
	pgr->count = (ULONG)list.m_list4.size();
	pgr6->count = (ULONG)list.m_list6.size();
	pgr6->block = pgr->block = block ? 1 : 0;

	unsigned int i = 0;

	for(p2p::static_list4::const_iterator iter = list.m_list4.begin(), end = list.m_list4.end(); iter != end; ++iter) {
		pgr->ranges[i].label = iter->label;
		pgr->ranges[i].start = iter->start;
		pgr->ranges[i++].end = iter->end;

		ipcount += iter->end - iter->start + 1;
	}

	i = 0;

	for(p2p::static_list6::const_iterator iter = list.m_list6.begin(), end = list.m_list6.end(); iter != end; ++iter) {
		pgr6->ranges[i].label = iter->label;

		pgr6->ranges6[i].start.i64[0] = iter->start.i64[0];
		pgr6->ranges6[i].start.i64[1] = iter->start.i64[1];

		pgr6->ranges6[i].end.i64[0] = iter->end.i64[0];
		pgr6->ranges6[i++].end.i64[1] = iter->end.i64[1];

		//TODO: count ranges in 128-bit integers.
		//ipcount += iter->end - iter->start + 1;
	}

	{
		mutex::scoped_lock lock(m_lock);

		pgr6->labelsid = pgr->labelsid = (block ? m_blocklabelsid : m_allowlabelsid) + 1;

		m_filter.write(IOCTL_PEERGUARDIAN_SETRANGES, pgr, pgrsize);
		m_filter.write(IOCTL_PEERGUARDIAN_SETRANGES, pgr6, pgr6size);

		if(block) {
			++m_blocklabelsid;
			m_blockcount = ipcount;
			m_blocklist.swap(list);
		}
		else {
			++m_allowlabelsid;
			m_allowcount = ipcount;
			m_allowlist.swap(list);
		}
	}
}

void pgfilter_base::setactionfunc(const action_function &func) {
	mutex::scoped_lock lock(m_lock);
	m_onaction = func;
}

void pgfilter_base::thread_func() {
	HANDLE evts[2];

	evts[0] = CreateEvent(0, TRUE, FALSE, 0);
	evts[1] = m_exitevt;

	while(m_runthread) {
		OVERLAPPED ovl = {0};
		ovl.hEvent = evts[0];

		PGNOTIFICATION pgn;

		m_filter.read(IOCTL_PEERGUARDIAN_GETNOTIFICATION, &pgn, sizeof(pgn), &ovl);

		DWORD ret = WaitForMultipleObjects(2, evts, FALSE, INFINITE);
		if(ret < WAIT_OBJECT_0 || ret > (WAIT_OBJECT_0 + 1)) {
			std::wcout << L"error: WaitForMultipleObjects failed." << std::endl;
		}

		if(!m_runthread) {
			m_filter.cancelio();
			m_filter.getresult(&ovl);
			break;
		}

		ret = m_filter.getresult(&ovl);
		if(ret == ERROR_SUCCESS) {
			action a;

			if(pgn.action == 0) a.type = action::blocked;
			else if(pgn.action == 1) a.type = action::allowed;
			else a.type = action::none;

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
				mutex::scoped_lock lock(m_lock);

				if(pgn.label && ((a.type == action::blocked && pgn.labelsid == m_blocklabelsid) || (a.type == action::allowed && pgn.labelsid == m_allowlabelsid))) {
					a.label = pgn.label;
				}

				if(m_onaction) {
					m_onaction(a);
				}
			}
		}
		else if(ret == ERROR_OPERATION_ABORTED) break;
		else {
			std::wcout << L"error: getresult failed." << std::endl;
		}

		ResetEvent(evts[0]);
	}

	CloseHandle(evts[0]);
}

DWORD WINAPI pgfilter_base::thread_thunk(void *arg) {
	reinterpret_cast<pgfilter_base*>(arg)->thread_func();
	return 0;
}
