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

#pragma once

#include <vector>
#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <p2p/range.hpp>
#include <windows.h>
#include "tstring.h"
#include "threadx.hpp"

class pbfilter : boost::noncopyable {
public:
	struct action {
		std::string label;
		union {
			sockaddr addr;
			sockaddr_in addr4;
			sockaddr_in6 addr6;
		} src, dest;
		int protocol;
		enum { allowed, blocked, none } type;
	};
	typedef boost::function<void(const action&)> action_function;

	pbfilter();
	~pbfilter();

	void setblock(bool block);
	void setblockhttp(bool block);

	void setranges(const p2p::list &ranges, bool block);

	void setactionfunc(const action_function &func = action_function());

	unsigned int blockcount() const { return m_blockcount; }
	unsigned int allowcount() const { return m_allowcount; }

private:
	struct range {
		const char *label;
		unsigned int start, end;
	};

	static const range* inranges(const range *ranges, int count, unsigned int ip);

	void thread_func();
	static DWORD WINAPI thread_thunk(void *arg);

	HANDLE m_thread;

	std::vector<range> m_blockranges, m_allowranges;
	std::vector<char> m_blocklabels, m_allowlabels;
	unsigned int m_blockcount, m_allowcount;

	action_function m_onaction;

	volatile bool m_blockhttp;
	volatile bool m_runthread;

	mutex m_lock, m_threadlock;
};
