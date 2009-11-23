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

#include "stdafx.h"
#include "path.hpp"
#include "helpers.hpp"
#include "procs.hpp"
#include "exception.hpp"
#include "resource.h"

static void AddTemp_OnClose(HWND hwnd) {
	EndDialog(hwnd, 0);
}

static void AddTemp_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
	}
}

static void AddTemp_OnDestroy(HWND hwnd) {

}

static BOOL AddTemp_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	int taborder[] = { IDC_ADDRESS, IDC_EXPIREDATE, IDC_EXPIRETIME, IDC_ALLOW, IDC_BLOCK, IDOK, IDCANCEL };
	SetTabOrder(hwnd, taborder);
	
	SendDlgItemMessage(hwnd, IDC_ALLOW, BM_SETCHECK, BST_CHECKED, 0);

	return TRUE;
}

static LRESULT AddTemp_OnNotify(HWND hwnd, int, LPNMHDR pnmh) {
	if(pnmh->idFrom == IDC_EXPIREDATE && pnmh->code == DTN_DATETIMECHANGE) {
		NMDATETIMECHANGE &dtc = *(NMDATETIMECHANGE*)pnmh;
		EnableWindow(GetDlgItem(hwnd, IDC_EXPIRETIME), dtc.dwFlags == GDT_VALID);
	}

	return 0;
}

static INT_PTR CALLBACK AddTemp_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CLOSE, AddTemp_OnClose);
		HANDLE_MSG(hwnd, WM_COMMAND, AddTemp_OnCommand);
		HANDLE_MSG(hwnd, WM_DESTROY, AddTemp_OnDestroy);
		HANDLE_MSG(hwnd, WM_INITDIALOG, AddTemp_OnInitDialog);
		HANDLE_MSG(hwnd, WM_NOTIFY, AddTemp_OnNotify);
		default: return FALSE;
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}

void ShowAddTempDialog(HWND parent) {
	SysFontDialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDTEMP), parent, AddTemp_DlgProc);
}
