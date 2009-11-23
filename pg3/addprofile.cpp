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
#include "procs.hpp"
#include "helpers.hpp"
#include "exception.hpp"
#include "resource.h"

struct addprofile_ctx {
	const global_config &cfg;
	pg3_profile &p;

	addprofile_ctx(const global_config &cfg, pg3_profile &p) : cfg(cfg),p(p) {}
};

static void AddProfile_OnClose(HWND hwnd) {
	EndDialog(hwnd, IDCANCEL);
}

static void AddProfile_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	const addprofile_ctx &ctx = *GetDialogStatePtr<addprofile_ctx>(hwnd);

	switch(id) {
		case IDOK: {
			int idx = ComboBox_GetCurSel(hwndCtl);

			ctx.p = ctx.cfg.lists.profiles[idx];
			ctx.p.label = GetWindowText(GetDlgItem(hwnd, IDC_LABEL));

			EndDialog(hwnd, IDOK);
		} break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
	}
}

static BOOL AddProfile_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	const addprofile_ctx &ctx = *(const addprofile_ctx*)lParam;

	int taborder[] = { IDC_LABEL, IDC_PROFILES, IDOK, IDCANCEL };
	SetTabOrder(hwnd, taborder);

	HWND profiles = GetDlgItem(hwnd, IDC_PROFILES);

	for(std::vector<pg3_profile>::const_iterator iter = ctx.cfg.lists.profiles.begin(), end = ctx.cfg.lists.profiles.end(); iter != end; ++iter) {
		ComboBox_AddString(profiles, iter->label.c_str());
	}

	ComboBox_SetCurSel(profiles, 0);

	SetDialogStatePtr(hwnd, &ctx);
	return TRUE;
}

static INT_PTR CALLBACK AddProfile_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CLOSE, AddProfile_OnClose);
		HANDLE_MSG(hwnd, WM_COMMAND, AddProfile_OnCommand);
		HANDLE_MSG(hwnd, WM_INITDIALOG, AddProfile_OnInitDialog);
		default: return FALSE;
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}

bool ShowAddProfileDialog(HWND parent, const global_config &cfg, pg3_profile &p) {
	const addprofile_ctx ctx(cfg, p);

	INT_PTR res = SysFontDialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDPROFILE), parent, AddProfile_DlgProc, (LPARAM)&ctx);
	return (res == IDOK);
}
