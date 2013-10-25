/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2011 PeerBlock, LLC

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

#include <stdexcept>
#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <process.h>
#include <windows.h>
#include "messagebox.h"
#include "win32_error.h"

#include "tracelog.h"
extern TraceLog g_tlog;


class thread : boost::noncopyable {
public:
	typedef boost::function<void()> function_type;

private:
	HANDLE t;
	function_type fn;

	static unsigned int __stdcall func(void *arg) {
		try {
			((thread*)arg)->fn();
		}
		catch(std::exception &ex) {
			UncaughtExceptionBox(NULL, ex, __FILE__, __LINE__);
			return -1;
		}
		catch(...) {
			UncaughtExceptionBox(NULL, __FILE__, __LINE__);
			return -1;
		}

		return 0;
	}

public:
	thread(const function_type &f, int priority=THREAD_PRIORITY_NORMAL) : fn(f) {
		t=(HANDLE)_beginthreadex(NULL, 0, &func, (void*)this, (priority!=THREAD_PRIORITY_NORMAL)?CREATE_SUSPENDED:0, NULL);
		if(!t) throw win32_error("_beginthreadex");

		if(priority!=THREAD_PRIORITY_NORMAL) {
			SetThreadPriority(t, priority);
			ResumeThread(t);
		}
	}
	~thread() { CloseHandle(t); }

	void join() { WaitForSingleObject(t, INFINITE); }
};

class mutex {
private:
	CRITICAL_SECTION cs;

public:
	void enter()
	{
		//TCHAR chBuf[256];
		//_stprintf_s(chBuf, _countof(chBuf), _T("***  > Entering Mutex:[%p]"), this);
		//g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_VERBOSE);

		EnterCriticalSection(&cs);
	}

	bool tryenter() { return TryEnterCriticalSection(&cs)!=0; }

	void leave()
	{
		//TCHAR chBuf[256];
		//_stprintf_s(chBuf, _countof(chBuf), _T("***  < Leaving Mutex:[%p]"), this);
		//g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_VERBOSE);

		LeaveCriticalSection(&cs);
	}

private:
	class lock {
	private:
		mutex &m;
		bool locked;

	protected:
		lock(mutex &m) : m(m),locked(false) {}

	public:
		~lock() { if(locked) m.leave(); }

		void enter() {
			m.enter();
			locked=true;
		}

		bool tryenter() {
			return (locked=m.tryenter());
		}

		void leave() {
			m.leave();
			locked=false;
		}

		bool is_locked() const { return locked; }
	};

public:
	mutex() { InitializeCriticalSection(&cs); }
	~mutex() { DeleteCriticalSection(&cs); }

	class scoped_lock : public lock {
	public:
		scoped_lock(mutex &m, bool locked=true) : lock(m) {
			if(locked) this->enter();
		}
	};

	class scoped_try_lock : public lock {
	public:
		scoped_try_lock(mutex &m, bool locked=true) : lock(m) {
			if(locked) this->tryenter();
		}
	};
};

class spinlock {
private:
	volatile long locked;

public:
	void enter() { while(InterlockedCompareExchange(&locked, 1, 0)); }
	bool tryenter() { return InterlockedCompareExchange(&locked, 1, 0)==0; }

	bool tryenter(unsigned int tries) {
		while(tries--) if(InterlockedCompareExchange(&locked, 1, 0)==0) return true;
		return false;
	}

	void leave() { locked = 0; }

private:
	class lock {
	private:
		spinlock &sl;
		bool locked;

	protected:
		lock(spinlock &l) : sl(l),locked(false) {}

	public:
		~lock() { if(locked) sl.leave(); }

		void enter() {
			sl.enter();
			locked=true;
		}

		bool tryenter() {
			return (locked=sl.tryenter());
		}

		void leave() {
			sl.leave();
			locked=false;
		}

		bool is_locked() const { return locked; }
	};

public:
	spinlock() : locked(0) {}

	class scoped_lock : public lock {
	public:
		scoped_lock(spinlock &l, bool locked=true) : lock(l) {
			if(locked) this->enter();
		}
	};

	class scoped_try_lock : public lock {
	public:
		scoped_try_lock(spinlock &l, bool locked=true) : lock(l) {
			if(locked) this->tryenter();
		}
	};
};
