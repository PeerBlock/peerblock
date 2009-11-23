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
#include "helpers.hpp"
#include "procs.hpp"
#include "resource.h"

static void About_OnClose(HWND hwnd) {
	EndDialog(hwnd, 0);
}

static void About_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	if(id == IDCLOSE) {
		EndDialog(hwnd, 0);
	}
}

static BOOL About_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	SetDlgItemText(hwnd, IDC_CONTENT, LoadString(IDS_ABOUTCONTENT).c_str());

	HICON ico = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DOPEFISH), IMAGE_ICON, 48, 48, 0);

	HWND dope = GetDlgItem(hwnd, IDC_DOPE);

	ico = (HICON)SendMessage(dope, STM_SETICON, (WPARAM)ico, 0);
	SetWindowPos(dope, NULL, 0, 0, 48, 48, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_NOZORDER);

	if(ico) {
		DestroyIcon(ico);
	}

	return TRUE;
}

static INT_PTR CALLBACK About_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CLOSE, About_OnClose);
		HANDLE_MSG(hwnd, WM_COMMAND, About_OnCommand);
		HANDLE_MSG(hwnd, WM_INITDIALOG, About_OnInitDialog);
		default: return FALSE;
	}
}

void ShowAboutDialog(HWND parent) {
#if _WIN32_WINNT >= 0x0600
	TASKDIALOGCONFIG cfg = {0};

	cfg.cbSize = sizeof(cfg);
	cfg.hwndParent = parent;
	cfg.hInstance = GetModuleHandle(NULL);
	cfg.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
	cfg.dwCommonButtons = TDCBF_CLOSE_BUTTON;
	cfg.pszWindowTitle = MAKEINTRESOURCE(IDS_ABOUTPEERGUARDIAN);
	cfg.pszMainIcon = MAKEINTRESOURCE(IDI_DOPEFISH);
	cfg.pszMainInstruction = MAKEINTRESOURCE(IDS_ABOUTPEERGUARDIAN);
	cfg.pszContent = MAKEINTRESOURCE(IDS_ABOUTCONTENT);

	TaskDialogIndirect(&cfg, NULL, NULL, NULL);
#else
	SysFontDialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), parent, About_DlgProc);
#endif
}
