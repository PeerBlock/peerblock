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

struct applist_ctx {
	configs &cfgs;

	applist_ctx(configs &cfgs) : cfgs(cfgs) {}
};

static void AppList_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDC_ADD:
		case IDC_EDIT:
			ShowAddAppDialog(hwnd);
			break;
		case IDC_REMOVE:
			break;
	}
}

static void AppList_OnDestroy(HWND hwnd) {
	applist_ctx *ctx = GetDialogStatePtr<applist_ctx>(hwnd);

	HWND list = GetDlgItem(hwnd, IDC_LIST);

	ctx->cfgs.ucfg.lists.apps.label = PixelsToPoints((unsigned int)ListView_GetColumnWidth(list, 0));
	ctx->cfgs.ucfg.lists.apps.application = PixelsToPoints((unsigned int)ListView_GetColumnWidth(list, 1));
	ctx->cfgs.ucfg.lists.apps.type = PixelsToPoints((unsigned int)ListView_GetColumnWidth(list, 2));

	delete ctx;
}

static BOOL AppList_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	configs &cfgs = *(configs*)((PROPSHEETPAGE*)lParam)->lParam;

	int taborder[] = { IDC_LIST, IDC_ADD, IDC_EDIT, IDC_REMOVE };
	SetTabOrder(hwnd, taborder);

	const std::wstring &label = LoadString(IDS_LABEL);
	const std::wstring &application = LoadString(IDS_APPLICATION);
	const std::wstring &type = LoadString(IDS_TYPE);

	const LVCOLUMN cols[] = {
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.apps.label),
			(LPTSTR)label.c_str(),
			0,
			0
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.apps.application),
			(LPTSTR)application.c_str(),
			0,
			1
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.apps.type),
			(LPTSTR)type.c_str(),
			0,
			2
		}
	};

	HWND list = GetDlgItem(hwnd, IDC_LIST);
	
	ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(list, L"Explorer", NULL);
	InsertListViewColumns(list, cols);
	
	applist_ctx *ctx = new applist_ctx(cfgs);
	SetDialogStatePtr(hwnd, ctx);

	return TRUE;
}

static LRESULT AppList_OnNotify(HWND hwnd, int, LPNMHDR pnmh) {
	return 0;
}

INT_PTR CALLBACK AppList_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_COMMAND, AppList_OnCommand);
		HANDLE_MSG(hwnd, WM_DESTROY, AppList_OnDestroy);
		HANDLE_MSG(hwnd, WM_INITDIALOG, AppList_OnInitDialog);
		HANDLE_MSG(hwnd, WM_NOTIFY, AppList_OnNotify);
		default: return FALSE;
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}
