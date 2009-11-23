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

#pragma once

#include <string>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "p2p/list.hpp"
#include "p2p/static_list.hpp"

#include "mutex.hpp"
#include "driver.hpp"

/*
	pgfilter_base is a base class with code common to accessing both 2k and
	Vista drivers.
*/
class pgfilter_base {
public:
	// actions are sent through the callback action_function whenever the driver
	// blocks or allows a connection.
	struct action {
		std::wstring label;
		union {
			sockaddr addr;
			sockaddr_in addr4;
			SOCKADDR_IN6 addr6;
		} src, dest;
		int protocol;
		enum { allowed, blocked, none } type;
	};
	typedef boost::function<void(const action&)> action_function;

	// turns blocking on or off.  this is a blocking (but quick) call.
	void setblock(bool block);
	
	// turns http blocking on or off.  this is a blocking (but quick) call.
	void setblockhttp(bool block);

	// sets new ranges in the driver.  this is a blocking (but quick) call.
	void setranges(const p2p::list &ranges, bool block);

	// sets the function to be called when actions occur.
	void setactionfunc(const action_function &func = action_function());

	// gets the blocked IP count.
	unsigned int blockcount() const { return m_blockcount; }
	unsigned int allowcount() const { return m_allowcount; }

protected:
	pgfilter_base();

	// called by inheriting classes to start the notification thread.
	void start_thread();

	// called by inheriting classes to stop the notification thread.
	void stop_thread();
	
	driver m_filter;

private:
	// the notification thread function.
	void thread_func();
	static DWORD WINAPI thread_thunk(void *arg);

	HANDLE m_thread, m_exitevt;

	p2p::static_list m_blocklist, m_allowlist;
	unsigned int m_blocklabelsid, m_allowlabelsid;

	unsigned int m_blockcount, m_allowcount;

	action_function m_onaction;

	bool m_block, m_blockhttp;
	volatile bool m_runthread;

	mutex m_lock;
};

#if _WIN32_WINNT >= 0x0600

#include "wfp.hpp"

/*
	filter driver for Vista.
*/
class pgfilter : public pgfilter_base {
public:
	pgfilter();
	~pgfilter();

private:
	// installs the WFP callout driver.
	void install_callout();

	// removes the WFP callout driver.
	void remove_callout();

	wfp_session m_session;
};

#else

/*
	filter driver for 2k.
*/
class pgfilter : public pgfilter_base {
public:
	pgfilter();
	~pgfilter();

private:
	driver m_ipfltdrv;
};

#endif
