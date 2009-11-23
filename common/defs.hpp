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

/*
	These are all standard defines used throughout the projects of this solution.
	#define PG2_VISTA if using Vista, otherwise XP or 2003 funcs will be used.
*/

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#define _SECURE_SCL 0

#define BOOST_USE_WINDOWS_H

#define STRICT
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#define INITGUID

#ifdef PG3_VISTA

// on Vista, use Longhorn defines.
#define _WIN32_WINNT		0x0600
#define _WIN32_IE			0x0700
#define NTDDI_VERSION	NTDDI_LONGHORN

#else

#ifdef _M_X64

// on 64-bit XP, use Windows 2003 SP1 defines.
#define _WIN32_WINNT		0x0502
#define _WIN32_IE			0x0600
#define NTDDI_VERSION	NTDDI_WS03SP1

#else

// on 32-bit XP, use Windows XP defines.
#define _WIN32_WINNT		0x0501
#define _WIN32_IE			0x0600
#define NTDDI_VERSION	NTDDI_WINXP

#endif

#endif

#if _WIN32_WINNT < 0x0600
/* commctrl.h wrongfully brings in task dialog APIs when not compiling for Vista. */
#define NOTASKDIALOG
#endif
