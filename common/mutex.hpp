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
	simple RAII wrapper around a Win32 critical section.
*/
class mutex : private CRITICAL_SECTION {
public:
	mutex() {
		// on pre-Vista, InitializeCriticalSection can throw a STATUS_NO_MEMORY exception.
#if _WIN32_WINNT < 0x0600
		__try {
#endif
			InitializeCriticalSection(this);
#if _WIN32_WINNT < 0x0600
		}
		__except(GetExceptionCode() == STATUS_NO_MEMORY) {
			throw win32_error("InitializeCriticalSection", ERROR_NOT_ENOUGH_MEMORY);
		}
#endif
	}

	~mutex() { DeleteCriticalSection(this); }

	// use in scope for exception-safe unlocking.
	class scoped_lock {
	public:
		scoped_lock(mutex &m, bool lock = true) : m_mutex(m),m_locked(false) {
			if(lock) enter();
		}
		~scoped_lock() {
			if(m_locked) m_mutex.leave();
		}

		void enter() {
			m_locked = true;
			m_mutex.enter();
		}

		void leave() {
			m_locked = false;
			m_mutex.leave();
		}

	private:
		mutex &m_mutex;
		bool m_locked;
	};

private:
	// noncopyable.
	mutex(const mutex&);
	void operator=(const mutex&);

	void enter() { EnterCriticalSection(this); }
	void leave() { LeaveCriticalSection(this); }
};
