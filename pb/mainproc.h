/*
	Copyright (C) 2004-2005 Cory Nelson
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

#pragma once

#include <boost/shared_ptr.hpp>
#include <windows.h>

#ifdef _WIN32_WINNT
#if _WIN32_WINNT >= 0x0600
#include "pbfilter_wfp.h"
#else
#include "pbfilter_nt.h"
#endif
#endif

#define WM_MAIN_VISIBLE	(WM_APP+1)

struct TabData {
	UINT Title;
	LPCTSTR Template;
	DLGPROC Proc;
	HWND Tab;
};

INT_PTR CALLBACK Main_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void SetBlock(bool block);
void SendDialogIconRefreshMessage();
void SetBlockHttp(bool block);
void Shutdown();


extern TabData g_tabs[];
extern boost::shared_ptr<pbfilter> g_filter;
extern DWORD g_blinkstart;
extern HWND g_main;

extern bool g_trayactive;
extern NOTIFYICONDATA g_nid;
extern mutex g_lastblocklock;
extern DWORD g_lastblocktime;
extern mutex g_lastupdatelock;
