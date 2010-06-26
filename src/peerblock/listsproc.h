/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2010 PeerBlock, LLC

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

#ifndef __LISTSPROC_H__
#define __LISTSPROC_H__

#include <windows.h>

#define LISTS_NEEDRELOAD 1
#define LISTS_NEEDUPDATE 2

INT_PTR CALLBACK Lists_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern HWND g_hListsDlg;

#endif
