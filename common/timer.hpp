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
	timer is used to determine the CPU usages of a single-threaded operation.
*/
class timer {
public:
	void start() {
		BOOL ret = GetThreadTimes(GetCurrentThread(), (FILETIME*)&sc, (FILETIME*)&se, (FILETIME*)&sk, (FILETIME*)&su);
		if(!ret) throw win32_error("GetThreadTimes");

		sw = GetTickCount();
	}

	void stop() {
		BOOL ret = GetThreadTimes(GetCurrentThread(), (FILETIME*)&ec, (FILETIME*)&ee, (FILETIME*)&ek, (FILETIME*)&eu);
		if(!ret) throw win32_error("GetThreadTimes");

		ew = GetTickCount();
	}

	double user() const { return (eu - su) / 10000000.0; }
	double kernel() const { return (ek - sk) / 10000000.0; }
	double total() const { return ((ek + eu) - (sk + su)) / 10000000.0; }
	double wall() const { return (ew - sw) / 1000.0; }

private:
	unsigned __int64 sc, se, sk, su;
	unsigned __int64 ec, ee, ek, eu;
	DWORD sw, ew;
};
