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

#include <windows.h>
#include "win32_error.hpp"

/*
	timer_queue is used to queue up expiring timers that are called from a thread
	pool.
*/
class timer_queue {
public:
	timer_queue() {
		m_queue = CreateTimerQueue();
		if(!m_queue) throw win32_error("CreateTimerQueue");
	}

	~timer_queue() {
		close();
	}

	// closes the timer queue.  this will block if expired timers are currently executing.
	void close() {
		if(m_queue) {
			BOOL ret = DeleteTimerQueueEx(m_queue, INVALID_HANDLE_VALUE);
			if(!ret) throw win32_error("DeleteTimerQueueEx");

			m_queue = NULL;
		}
	}

	// creates a new timer.
	// callback is called from a thread pool in duetime milliseconds.  after that, it is called every period milliseconds.
	// see CreateTimerQueueTimer for flags.
	HANDLE create(WAITORTIMERCALLBACK callback, void *arg, DWORD duetime, DWORD period, ULONG flags = WT_EXECUTEDEFAULT) {
		HANDLE timer;

		BOOL ret = CreateTimerQueueTimer(&timer, m_queue, callback, arg, duetime, period, flags);
		if(!ret) throw win32_error("CreateTimerQueueTimer");

		return timer;
	}

	// sets the duetime and period of a timer.
	void set(HANDLE timer, DWORD duetime, DWORD period) {
		BOOL ret = ChangeTimerQueueTimer(m_queue, timer, duetime, period);
		if(!ret) throw win32_error("ChangeTimerQueueTimer");
	}

	// destroys a timer.
	// if wait is true and the timer is expired and running, it blocks until it is finished.
	// if wait is false and the timer is expired and running, it will mark the timer for deletion and return false.
	bool destroy(HANDLE timer, bool wait = false) {
		BOOL ret = DeleteTimerQueueTimer(m_queue, timer, wait ? INVALID_HANDLE_VALUE : NULL);

		DWORD err;
		if(!ret && (err = GetLastError()) != ERROR_IO_PENDING) throw win32_error("DeleteTimerQueueTimer");

		return ret != FALSE;
	}

private:
	// noncopyable.
	timer_queue(const timer_queue&);
	void operator=(const timer_queue&);

	HANDLE m_queue;
};
