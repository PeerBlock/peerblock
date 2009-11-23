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

#include <vector>
#include "win32_error.hpp"

/*
	simple RAII wrapper around a Win32 event.
*/
class win32_event {
public:
	win32_event() {
		m_evt = CreateEvent(NULL, FALSE, FALSE, NULL);
		if(!m_evt) throw win32_error("CreateEvent");
	}

	~win32_event() {
		CloseHandle(m_evt);
	}

	// sets the event.
	void set() { SetEvent(m_evt); }

	// blocks until the event is set.
	void wait() { WaitForSingleObject(m_evt, INFINITE); }

private:
	// noncopyable.
	win32_event(const win32_event&);
	void operator=(const win32_event&);

	HANDLE m_evt;
};
