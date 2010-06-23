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

#include <windows.h>

#define HFM_GETSEARCH (WM_USER+1)

#define HFN_SEARCH 1

#define HFM_RANGE		(1)
#define HFM_SOURCE		(1<<1)
#define HFM_DEST		(1<<2)
#define HFM_FROM		(1<<3)
#define HFM_TO			(1<<4)
#define HFM_PROTOCOL	(1<<5)
#define HFM_ACTION		(1<<6)

struct HFM_SEARCHINFO {
	int iMask;

	LPTSTR lpRange;
	int cRangeMax;

	unsigned int uSource, uDest;
	SYSTEMTIME stFrom, stTo;
	int iProtocol;
	int iAction;
};

struct HFNM_SEARCH {
	NMHDR hdr;
	HFM_SEARCHINFO info;
};

INT_PTR CALLBACK HistoryFind_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern HWND g_hHistoryFindDlg;
