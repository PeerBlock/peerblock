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
#include "appdb.hpp"
#include "helpers.hpp"
#include "procs.hpp"
#include "exception.hpp"
#include "resource.h"

static void AddApp_OnClose(HWND hwnd) {
	EndDialog(hwnd, 0);
}

static void AddApp_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
	}
}

static void AddApp_OnDestroy(HWND hwnd) {

}

static BOOL AddApp_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	int taborder[] = { IDC_LABEL, IDC_PATH, IDC_BROWSE, IDC_APPS, IDC_ALLOW, IDC_BLOCK, IDOK, IDCANCEL };
	SetTabOrder(hwnd, taborder);
	
	const std::wstring &name = LoadString(IDS_NAME);
	const std::wstring &path = LoadString(IDS_PATH);

	const LVCOLUMN cols[] = {
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(64u),
			(LPTSTR)name.c_str(),
			0,
			0
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(64u),
			(LPTSTR)path.c_str(),
			0,
			1
		}
	};

	HWND list = GetDlgItem(hwnd, IDC_LIST);
	
	ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(list, L"Explorer", NULL);
	InsertListViewColumns(list, cols);

	SendDlgItemMessage(hwnd, IDC_ALLOW, BM_SETCHECK, BST_CHECKED, 0);

	appdb db;
	db.open();

	int icox = GetSystemMetrics(SM_CXSMICON);
	int icoy = GetSystemMetrics(SM_CYSMICON);

	HIMAGELIST ilist = ImageList_Create(icox, icoy, ILC_COLOR32, 0, (int)db.map().size());
	if(ilist) {
		ListView_SetImageList(list, ilist, LVSIL_SMALL);
	}

	int i = 0;
	for(appdb::map_type::const_iterator iter = db.map().begin(), end = db.map().end(); iter != end; ++iter) {
		LVITEM lvi = {0};

		lvi.iItem = i++;

		lvi.mask = LVIF_TEXT | LVIF_IMAGE;
		lvi.iSubItem = 0;
		lvi.pszText = (LPWSTR)iter->second.name.c_str();
		lvi.iImage = I_IMAGECALLBACK;
		ListView_InsertItem(list, &lvi);

		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 1;
		lvi.pszText = (LPWSTR)iter->second.path.c_str();
		ListView_SetItem(list, &lvi);
	}

	return TRUE;
}

static LRESULT AddApp_OnNotify(HWND hwnd, int, LPNMHDR pnmh) {
	if(pnmh->idFrom == IDC_LIST && pnmh->code == LVN_GETDISPINFO) {
		NMLVDISPINFO &lvdi = *(NMLVDISPINFO*)pnmh;

		wchar_t path[MAX_PATH];
		ListView_GetItemText(pnmh->hwndFrom, lvdi.item.iItem, 1, path, MAX_PATH);

		SHFILEINFO info;

		int idx = -1;

		DWORD_PTR ret = SHGetFileInfo(path, 0, &info, sizeof(info), SHGFI_ICON | SHGFI_SMALLICON);
		if(ret) {
			HIMAGELIST ilist = ListView_GetImageList(pnmh->hwndFrom, LVSIL_SMALL);

			idx = ImageList_AddIcon(ilist, info.hIcon);

			DestroyIcon(info.hIcon);
		}

		if(idx != -1) {
			lvdi.item.iImage = idx;
		}
		else {
			lvdi.item.mask &= ~LVIF_IMAGE;
		}

		lvdi.item.mask |= LVIF_DI_SETITEM;
	}

	return 0;
}

static INT_PTR CALLBACK AddApp_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CLOSE, AddApp_OnClose);
		HANDLE_MSG(hwnd, WM_COMMAND, AddApp_OnCommand);
		HANDLE_MSG(hwnd, WM_DESTROY, AddApp_OnDestroy);
		HANDLE_MSG(hwnd, WM_INITDIALOG, AddApp_OnInitDialog);
		HANDLE_MSG(hwnd, WM_NOTIFY, AddApp_OnNotify);
		default: return FALSE;
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}

void ShowAddAppDialog(HWND parent) {
	SysFontDialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDAPP), parent, AddApp_DlgProc);
}
