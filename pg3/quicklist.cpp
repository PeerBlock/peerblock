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
#include "exception.hpp"
#include "resource.h"

struct quicklist_ctx {
	configs &cfgs;

	quicklist_ctx(configs &cfgs) : cfgs(cfgs) {}
};

static void QuickList_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDC_ADD:
		case IDC_EDIT:
			ShowAddTempDialog(hwnd);
			break;
		case IDC_REMOVE:
			break;
	}
}

static void QuickList_OnDestroy(HWND hwnd) {
	quicklist_ctx *ctx = GetDialogStatePtr<quicklist_ctx>(hwnd);

	HWND list = GetDlgItem(hwnd, IDC_LIST);

	ctx->cfgs.ucfg.lists.quicklist.label = PixelsToPoints((unsigned int)ListView_GetColumnWidth(list, 0));
	ctx->cfgs.ucfg.lists.quicklist.address = PixelsToPoints((unsigned int)ListView_GetColumnWidth(list, 1));
	ctx->cfgs.ucfg.lists.quicklist.type = PixelsToPoints((unsigned int)ListView_GetColumnWidth(list, 2));
	ctx->cfgs.ucfg.lists.quicklist.expire = PixelsToPoints((unsigned int)ListView_GetColumnWidth(list, 3));

	delete ctx;
}

static BOOL QuickList_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	configs &cfgs = *(configs*)((PROPSHEETPAGE*)lParam)->lParam;

	int taborder[] = { IDC_LIST, IDC_ADD, IDC_EDIT, IDC_REMOVE };
	SetTabOrder(hwnd, taborder);

	const std::wstring &label = LoadString(IDS_LABEL);
	const std::wstring &address = LoadString(IDS_ADDRESS);
	const std::wstring &type = LoadString(IDS_TYPE);
	const std::wstring &expires = LoadString(IDS_EXPIRES);

	const LVCOLUMN cols[] = {
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.quicklist.label),
			(LPTSTR)label.c_str(),
			0,
			0
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.quicklist.address),
			(LPTSTR)address.c_str(),
			0,
			1
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.quicklist.type),
			(LPTSTR)type.c_str(),
			0,
			2
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.quicklist.expire),
			(LPTSTR)expires.c_str(),
			0,
			3
		}
	};

	HWND list = GetDlgItem(hwnd, IDC_LIST);
	
	ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(list, L"Explorer", NULL);
	InsertListViewColumns(list, cols);
	
	quicklist_ctx *ctx = new quicklist_ctx(cfgs);
	SetDialogStatePtr(hwnd, ctx);

	return TRUE;
}

static LRESULT QuickList_OnNotify(HWND hwnd, int, LPNMHDR pnmh) {
	return 0;
}

INT_PTR CALLBACK QuickList_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_COMMAND, QuickList_OnCommand);
		HANDLE_MSG(hwnd, WM_DESTROY, QuickList_OnDestroy);
		HANDLE_MSG(hwnd, WM_INITDIALOG, QuickList_OnInitDialog);
		HANDLE_MSG(hwnd, WM_NOTIFY, QuickList_OnNotify);
		default: return FALSE;
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}
