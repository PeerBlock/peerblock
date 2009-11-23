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

class async_state {
public:
	async_state() : m_state(0) {}

	// called to start the operation.  returns true if this call gets to start it.
	bool start(bool restart = false) {
		long exch, cmp, oldcmp = m_state;

		do {
			if(oldcmp == 0) exch = 1;
			else if(oldcmp == 1) {
				if(!restart) return false;
				exch = 3;
			}
			else {
				return false;
			}

			cmp = oldcmp;
			oldcmp = InterlockedCompareExchange(&m_state, exch, oldcmp);
		} while(oldcmp != cmp);

		return (exch == 1);
	}

	// stops the operation.  clears any pending restart flags.
	void stop() {
		long cmp, oldcmp = m_state;

		do {
			if(oldcmp != 1 && oldcmp != 3) return;

			cmp = oldcmp;
			oldcmp = InterlockedCompareExchange(&m_state, 2, oldcmp);
		} while(oldcmp != cmp);
	}

	// call once done with the operation.  clears any state and returns true if the operation should restart.
	bool reset() {
		long exch, cmp, oldcmp = m_state;

		do {
			exch = (oldcmp == 2) ? 0 : 1;

			cmp = oldcmp;
			oldcmp = InterlockedCompareExchange(&m_state, exch, oldcmp);
		} while(oldcmp != cmp);

		return (exch == 1);
	}

	bool needs_cancel() const { return (m_state & 2) != 0; }

private:
	// 0: not running.
	// 1: running.
	// 2: cancel
	// 3: cancel+restart (set if start(true) is called and it is already running)
	volatile long m_state;
};
