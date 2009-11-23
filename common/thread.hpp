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

#include <boost/scoped_ptr.hpp>
#include <windows.h>
#include "win32_error.hpp"

/*
	simple wrapper for win32 threads.
*/
class thread {
public:
	thread() : m_thread(NULL) {}
	~thread() { destroy(); }

	template<typename Functor>
	void create(Functor func) {
		Functor *f = new Functor(func);

		m_thread = CreateThread(NULL, 0, thread_proc<Functor>, (void*)f, 0, NULL);
		if(!m_thread) {
			win32_error err("CreateThread");
			delete f;
			throw err;
		}
	}

	void destroy() {
		if(m_thread) {
			CloseHandle(m_thread);
			m_thread = NULL;
		}
	}

	void join() {
		DWORD ret = WaitForSingleObject(m_thread, INFINITE);
		if(ret != WAIT_OBJECT_0) throw win32_error("WaitForSingleObject");
	}

	operator bool() const { return m_thread != NULL; }
	bool operator!() const { return m_thread == NULL; }

private:
	// noncopyable.
	thread(const thread&);
	void operator=(const thread&);

	template<typename Functor>
	static DWORD WINAPI thread_proc(void *arg) {
		boost::scoped_ptr<Functor> func((Functor*)arg);

		try { (*func)(); }
		catch(...) {}

		return 0;
	}

	HANDLE m_thread;
};
