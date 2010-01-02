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

*/

#include "stdafx.h"
using namespace std;

// Increasing loop_count will slightly lower cpu usage.
// A mutex will be re-locked every loop_count loops.
// SetRanges will block for up to around loop_count*sleep_ms milliseconds.
// ~pbfilter will block for up to around sleep_ms milliseconds.
static const DWORD loop_count=20;
static const DWORD sleep_ms=25;

pbfilter::pbfilter() : m_thread(NULL),m_blockhttp(true) {
	//this->setblock(true);
}

pbfilter::~pbfilter() {
	this->setblock(false);
}

void pbfilter::thread_func() {
	while(m_runthread) {
		mutex::scoped_lock lock(m_lock);

		for(DWORD i = 0; m_runthread && i < loop_count; ++i) {
			MIB_TCPTABLE *table = NULL;
			DWORD tablelen = 0;

			GetTcpTable(table, &tablelen, FALSE);

			table = (MIB_TCPTABLE*)malloc(tablelen);
			if(GetTcpTable(table, &tablelen, FALSE) != NO_ERROR) {
				free(table);
				continue;
			}

			for(DWORD i = 0; m_runthread && i < table->dwNumEntries; ++i) {
				unsigned short remoteport = (unsigned short)ntohs((unsigned short)table->table[i].dwRemotePort);
				if(!m_blockhttp && PortAllowed(remoteport)) continue;

				unsigned int remote = ntohl(table->table[i].dwRemoteAddr);

				const range *r;

				if(m_blockranges.size()) {
					r = inranges(&m_blockranges.front(), (int)m_blockranges.size(), remote);
					if(r && m_allowranges.size()) {
						if(inranges(&m_allowranges.front(), (int)m_allowranges.size(), remote)) {
							r = NULL;
						}
					}
				}

				if(r) {
					table->table[i].dwState = MIB_TCP_STATE_DELETE_TCB;
					SetTcpEntry(&table->table[i]);

					if(m_onaction) {
						action a;

						a.label = r->label;

						a.src.addr4.sin_family = AF_INET;
						a.src.addr4.sin_addr.s_addr = table->table[i].dwLocalAddr;
						a.src.addr4.sin_port = (u_short)table->table[i].dwLocalPort;

						a.dest.addr4.sin_family = AF_INET;
						a.dest.addr4.sin_addr.s_addr = table->table[i].dwRemoteAddr;
						a.dest.addr4.sin_port = (u_short)table->table[i].dwRemotePort;

						a.protocol = IPPROTO_TCP;

						a.type = action::blocked;

						m_onaction(a);
					}
				}
			}

			free(table);
			Sleep(sleep_ms);
		}
	}
}

DWORD WINAPI pbfilter::thread_thunk(void *arg) {
	reinterpret_cast<pbfilter*>(arg)->thread_func();
	return 0;
}

const pbfilter::range *pbfilter::inranges(const range *ranges, int count, unsigned int ip) {
	const range *iter = ranges;
	const range *last = ranges + count;

	while(0 < count) {
		//night_stalker_z: bit shifting
		//int count2 = count / 2;
		int count2 = count >> 1;
		const range *mid = iter + count2;
		
		if(mid->start < ip) {
			iter = mid + 1;
			count -= count2 + 1;
		}
		else {
			count = count2;
		}
	}

	if(iter != last) {
		if(iter->start != ip) --iter;
	}
	else {
		--iter;
	}

	return (iter >= ranges && iter->start <= ip && ip <= iter->end) ? iter : NULL;
}

void pbfilter::setranges(const p2p::list &ranges, bool block) {
	typedef stdext::hash_map<std::wstring, const char*> hmap_type;

	hmap_type labels;
	std::vector<char> labelsbuf;

	for(p2p::list::const_iterator iter = ranges.begin(); iter != ranges.end(); ++iter) {
		const char* &label = labels[iter->name];

		if(!label) {
			label = (const char*)labelsbuf.size();

			std::string str = wcstombs(iter->name);

			labelsbuf.insert(labelsbuf.end(), str.begin(), str.end());
			labelsbuf.push_back('\0');
		}
	}

	for(hmap_type::iterator iter = labels.begin(); iter != labels.end(); ++iter) {
		iter->second = (&labelsbuf.front()) + (std::vector<char>::size_type)iter->second;
	}

	std::vector<range> rvec;
	unsigned int ipcount = 0;

	unsigned int i = 0;
	for(p2p::list::const_iterator iter = ranges.begin(); iter != ranges.end(); ++iter) {
		range r;
		r.label = labels[iter->name];
		r.start = iter->start.ipl;
		r.end = iter->end.ipl;

		ipcount += iter->end.ipl - iter->start.ipl + 1;

		rvec.push_back(r);
	}

	{
		mutex::scoped_lock lock(m_lock);

		if(block) {
			m_blockranges.swap(rvec);
			m_blocklabels.swap(labelsbuf);
			m_blockcount = ipcount;
		}
		else {
			m_allowranges.swap(rvec);
			m_allowlabels.swap(labelsbuf);
			m_allowcount = ipcount;
		}
	}
}

void pbfilter::setactionfunc(const action_function &func) {
	mutex::scoped_lock lock(m_lock);
	m_onaction = func;
}

void pbfilter::setblock(bool block) {
	mutex::scoped_lock lock(m_threadlock);

	if(block && !m_thread) {
		m_runthread = true;

		DWORD threadid;
		m_thread = CreateThread(0, 0, thread_thunk, this, 0, &threadid);
		if(!m_thread) {
			throw win32_error("CreateThread");
		}
	}
	else if(!block && m_thread) {
		m_runthread = false;

		WaitForSingleObject(m_thread, INFINITE);
		CloseHandle(m_thread);

		m_thread = NULL;
	}
}

void pbfilter::setblockhttp(bool block) {
	m_blockhttp = block;
}
