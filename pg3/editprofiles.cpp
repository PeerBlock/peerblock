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

static void EditProfiles_OnClose(HWND hwnd) {
	EndDialog(hwnd, IDCLOSE);
}

static void EditProfiles_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	global_config &cfg = *GetDialogStatePtr<global_config>(hwnd);

	switch(id) {
		case IDC_REMOVE: {
			HWND profiles = GetDlgItem(hwnd, IDC_PROFILES);

			int idx = ListView_GetSelectionMark(profiles);
			if(idx != -1) {
				cfg.lists.profiles.erase(cfg.lists.profiles.begin() + idx);
				ListView_DeleteItem(profiles, idx);

				if(cfg.lists.profiles.size()) {
					if(cfg.lists.curprofile == idx) cfg.lists.curprofile = 0;
				}
				else {
					pg3_profile p;
					p.label = LoadString(IDS_DEFAULT);
					p.lists.resize(cfg.lists.lists.size());
					p.presets.p2p = true;

					cfg.lists.profiles.push_back(p);
					cfg.lists.curprofile = 0;

					LVITEM lvi = {0};
					lvi.mask = LVIF_TEXT;
					lvi.pszText = (LPWSTR)p.label.c_str();

					ListView_InsertItem(profiles, &lvi);
				}
				
				ListView_SetItemState(profiles, 0, LVIS_SELECTED, LVIS_SELECTED);
				ListView_SetSelectionMark(profiles, 0);
			}
		} break;
		case IDC_RENAME: {
			HWND profiles = GetDlgItem(hwnd, IDC_PROFILES);

			int idx = ListView_GetSelectionMark(profiles);
			if(idx != -1) {
				SetFocus(profiles);
				ListView_EditLabel(profiles, idx);
			}
		} break;
		case IDCLOSE:
			EndDialog(hwnd, IDCLOSE);
			break;
	}
}

static BOOL EditProfiles_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	global_config &cfg = *(global_config*)lParam;

	int taborder[] = { IDC_PROFILES, IDC_REMOVE, IDC_RENAME, IDCLOSE };
	SetTabOrder(hwnd, taborder);

	HWND profiles = GetDlgItem(hwnd, IDC_PROFILES);

	ListView_SetExtendedListViewStyle(profiles, LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(profiles, L"Explorer", NULL);

	LVITEM lvi = {0};
	lvi.mask = LVIF_TEXT;

	for(std::vector<pg3_profile>::const_iterator iter = cfg.lists.profiles.begin(), end = cfg.lists.profiles.end(); iter != end; ++iter) {
		lvi.pszText = (LPWSTR)iter->label.c_str();
		ListView_InsertItem(profiles, &lvi);

		++lvi.iItem;
	}

	ListView_SetItemState(profiles, 0, LVIS_SELECTED, LVIS_SELECTED);
	ListView_SetSelectionMark(profiles, 0);

	SetDialogStatePtr(hwnd, &cfg);
	return TRUE;
}

static LRESULT EditProfiles_OnNotify(HWND hwnd, int, LPNMHDR pnmh) {
	if(pnmh->idFrom == IDC_PROFILES && pnmh->code == LVN_ENDLABELEDIT) {
		NMLVDISPINFO &lvdi = *(NMLVDISPINFO*)pnmh;

		BOOL res;
		if(lvdi.item.pszText) {
			global_config &cfg = *GetDialogStatePtr<global_config>(hwnd);
			cfg.lists.profiles[lvdi.item.iItem].label = lvdi.item.pszText;

			res = TRUE;
		}
		else {
			res = FALSE;
		}

		SetWindowLongPtr(hwnd, DWLP_MSGRESULT, res);
		return res;
	}

	return 0;
}

static INT_PTR CALLBACK EditProfiles_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CLOSE, EditProfiles_OnClose);
		HANDLE_MSG(hwnd, WM_COMMAND, EditProfiles_OnCommand);
		HANDLE_MSG(hwnd, WM_INITDIALOG, EditProfiles_OnInitDialog);
		HANDLE_MSG(hwnd, WM_NOTIFY, EditProfiles_OnNotify);
		default: return FALSE;
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}

void ShowEditProfilesDialog(HWND parent, global_config &cfg) {
	SysFontDialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EDITPROFILES), parent, EditProfiles_DlgProc, (LPARAM)&cfg);
}
