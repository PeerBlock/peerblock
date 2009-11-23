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
#include "config.hpp"
#include "backend.hpp"
#include "win32_error.hpp"
#include "exception.hpp"
#include "helpers.hpp"
#include "procs.hpp"
#include "vistabar.hpp"
#include "resource.h"

enum {
	IDC_STATUS = 200,
	IDC_REBAR,
	IDC_TOOLBAR,
	IDC_TOOLBAR_ENABLE,
	IDC_TOOLBAR_LISTMANAGER,
	IDC_TOOLBAR_CHECKUPDATES,
	IDC_RTOOLBAR,
	IDC_RTOOLBAR_OPTIONS,
	IDC_RTOOLBAR_ABOUT
};

enum {
	TIMER_ID_REBARFADE
};

struct main_ctx {
	user_config usercfg;
	boost::scoped_ptr<backend> backend;
	VistaBar tbback;

	// for rebar fade.
	VistaBar endbar;
	double starthue;
	DWORD fadestarttime, fadeendtime;
};

static void Main_OnClose(HWND hwnd) {
	DestroyWindow(hwnd);
}

static void Main_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	main_ctx &ctx = *GetWindowStatePtr<main_ctx>(hwnd);

	switch(id) {
		case IDC_TOOLBAR_ENABLE: {
			if(ctx.backend->m_cfg.block.enabled) {
				id = ID_CONTEXT_DISABLE;
			}
			else {
				id = ctx.backend->m_cfg.block.checkhttp ? ID_CONTEXT_ENABLE : ID_CONTEXT_ENABLEHTTP;
			}

			Main_OnCommand(hwnd, id, 0, 0);
		} break;
		case ID_CONTEXT_ENABLE:
			ctx.backend->setblock(true);
			ctx.backend->setblockhttp(true);
			break;
		case ID_CONTEXT_ENABLEHTTP:
			ctx.backend->setblock(true);
			ctx.backend->setblockhttp(false);
			break;
		case ID_CONTEXT_DISABLE:
			ctx.backend->setblock(false);
			break;
		case ID_HELP_ABOUTPEERGUARDIAN:
			ShowAboutDialog(hwnd);
			break;
		case ID_HELP_PEERGUARDIANHOMEPAGE:
			ShellExecute(NULL, NULL, L"http://peerguardian.sf.net/pg2.html", NULL, NULL, SW_SHOW);
			break;
		case ID_HELP_PHOENIXLABSHOMEPAGE:
			ShellExecute(NULL, NULL, L"http://phoenixlabs.org", NULL, NULL, SW_SHOW);
			break;
		case IDC_TOOLBAR_LISTMANAGER:
			ShowListManagerDialog(hwnd, configs(ctx.backend->m_cfg, ctx.usercfg));
			break;
		case IDC_RTOOLBAR_OPTIONS:
			throw win32_error("ReadFile", ERROR_IO_PENDING);
			break;
	}
}

static void Main_OnConfigChange(HWND hwnd) {
	main_ctx &ctx = *GetWindowStatePtr<main_ctx>(hwnd);

	int idx;

	if(ctx.backend->m_cfg.block.enabled) {
		idx = ctx.backend->m_cfg.block.checkhttp ? 0 : 1;
	}
	else {
		idx = 2;
	}

	HWND toolbar = GetDlgItem(GetDlgItem(hwnd, IDC_REBAR), IDC_TOOLBAR);

	TBBUTTONINFO tbbi;
	tbbi.cbSize = sizeof(tbbi);
	tbbi.dwMask = TBIF_LPARAM;
	SendMessage(toolbar, TB_GETBUTTONINFO, IDC_TOOLBAR_ENABLE, (LPARAM)&tbbi);

	if(tbbi.lParam != idx) {
		const UINT strs[] = { IDS_ENABLED, IDS_ENABLEDHTTP, IDS_DISABLED };
		static double hues[] = { 0.575, 0.166, 0.025 };
		
		const std::wstring &str = LoadString(strs[idx]);

		tbbi.dwMask = TBIF_LPARAM | TBIF_TEXT;
		tbbi.lParam = idx;
		tbbi.pszText = (LPWSTR)str.c_str();
		SendMessage(toolbar, TB_SETBUTTONINFO, IDC_TOOLBAR_ENABLE, (LPARAM)&tbbi);

		ctx.endbar.SetHue(hues[idx]);
		ctx.starthue = ctx.tbback.Hue();
		ctx.fadestarttime = GetTickCount();
		ctx.fadeendtime = ctx.fadestarttime + 250;
		SetTimer(hwnd, TIMER_ID_REBARFADE, USER_TIMER_MINIMUM, NULL);

		for(int i = 0; i < 3; ++i) {
			MENUITEMINFO mii;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STATE;
			mii.fState = (i == idx) ? MFS_CHECKED : MFS_UNCHECKED;

			SetMenuItemInfo(GetSubMenu(GetMenu(hwnd), 0), i, TRUE, &mii);
		}

		DrawMenuBar(hwnd);
	}
}

static void Main_OnSize(HWND hwnd, UINT state, int cx, int cy);

static BOOL Main_OnCreate(HWND hwnd, LPCREATESTRUCT) {
	std::auto_ptr<main_ctx> ctx(new main_ctx);

	ctx->tbback.SetHue(0.575);

	ctx->backend.reset(new dummy_backend);
	ctx->backend->setconfigfunc(boost::bind(Main_OnConfigChange, hwnd));

	// setup status bar

	HWND status = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, (HMENU)IDC_STATUS, NULL, NULL);
	if(!status) return FALSE;

	SendMessage(status, SB_SIMPLE, TRUE, 0);
	SendMessage(status, SB_SETTEXT, SB_SIMPLEID, (LPARAM)L"Blocking 14,907 IPs");

	// setup imagelist

	int sizex = GetSystemMetrics(SM_CXSMICON);
	int sizey = GetSystemMetrics(SM_CYSMICON);

	HIMAGELIST ilist = ImageList_Create(sizex, sizey, ILC_COLOR32, 0, 4);

	HICON icon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LISTCONFIG), IMAGE_ICON, sizex, sizey, 0);
	ImageList_AddIcon(ilist, icon);
	DestroyIcon(icon);

	icon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_REFRESH), IMAGE_ICON, sizex, sizey, 0);
	ImageList_AddIcon(ilist, icon);
	DestroyIcon(icon);

	icon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_PREFS), IMAGE_ICON, sizex, sizey, 0);
	ImageList_AddIcon(ilist, icon);
	DestroyIcon(icon);

	icon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_INFO), IMAGE_ICON, sizex, sizey, 0);
	ImageList_AddIcon(ilist, icon);
	DestroyIcon(icon);

	// setup toolbar

	HWND toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT, 0, 0, 0, 0, hwnd, (HMENU)IDC_TOOLBAR, NULL, NULL);
	if(!toolbar) return FALSE;

	SendMessage(toolbar, TB_SETWINDOWTHEME, 0, (LPARAM)L"Alternate");
	SendMessage(toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(toolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);

	TBMETRICS tbm = {0};
	tbm.cbSize = sizeof(tbm);
	tbm.dwMask = TBMF_PAD | TBMF_BARPAD | TBMF_BUTTONSPACING;
	tbm.cyPad = PointsToPixels(2u);
	tbm.cxButtonSpacing = PointsToPixels(3u);

	SendMessage(toolbar, TB_SETMETRICS, 0, (LPARAM)&tbm);
	SendMessage(toolbar, TB_SETIMAGELIST, 0, (LPARAM)ilist);

	const std::wstring &enabled = LoadString(IDS_ENABLED);
	const std::wstring &listmanager = LoadString(IDS_LISTMANAGER);
	const std::wstring &checkupdates = LoadString(IDS_CHECKUPDATES);

	TBBUTTON btns[3] = {0};

	btns[0].iBitmap = I_IMAGENONE;
	btns[0].idCommand = IDC_TOOLBAR_ENABLE;
	btns[0].fsState = TBSTATE_ENABLED;
	btns[0].fsStyle = BTNS_BUTTON | BTNS_DROPDOWN | BTNS_SHOWTEXT;
	btns[0].dwData = 0; // 0 = enabled.
	btns[0].iString = (INT_PTR)enabled.c_str();
	
	btns[1].iBitmap = 0;
	btns[1].idCommand = IDC_TOOLBAR_LISTMANAGER;
	btns[1].fsState = TBSTATE_ENABLED;
	btns[1].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	btns[1].iString = (INT_PTR)listmanager.c_str();
	
	btns[2].iBitmap = 1;
	btns[2].idCommand = IDC_TOOLBAR_CHECKUPDATES;
	btns[2].fsState = TBSTATE_ENABLED;
	btns[2].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	btns[2].iString = (INT_PTR)checkupdates.c_str();

	SendMessage(toolbar, TB_ADDBUTTONS, 3, (LPARAM)btns);

	// setup right toolbar

	HWND rtoolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT, 0, 0, 0, 0, hwnd, (HMENU)IDC_RTOOLBAR, NULL, NULL);
	if(!rtoolbar) return FALSE;

	SendMessage(rtoolbar, TB_SETWINDOWTHEME, 0, (LPARAM)L"Alternate");
	SendMessage(rtoolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(rtoolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
	SendMessage(rtoolbar, TB_SETMETRICS, 0, (LPARAM)&tbm);
	SendMessage(rtoolbar, TB_SETIMAGELIST, 0, (LPARAM)ilist);

	const std::wstring &options = LoadString(IDS_OPTIONS);
	const std::wstring &aboutpg = LoadString(IDS_ABOUTPEERGUARDIAN);

	btns[0].iBitmap = 2;
	btns[0].idCommand = IDC_RTOOLBAR_OPTIONS;
	btns[0].fsState = TBSTATE_ENABLED;
	btns[0].fsStyle = BTNS_BUTTON;
	btns[0].iString = (INT_PTR)options.c_str();

	btns[1].iBitmap = 3;
	btns[1].idCommand = IDC_RTOOLBAR_ABOUT;
	btns[1].fsState = TBSTATE_ENABLED;
	btns[1].fsStyle = BTNS_BUTTON | BTNS_DROPDOWN | BTNS_WHOLEDROPDOWN;
	btns[1].iString = (INT_PTR)aboutpg.c_str();

	SendMessage(rtoolbar, TB_ADDBUTTONS, 2, (LPARAM)btns);

	DWORD btnsize = (DWORD)SendMessage(toolbar, TB_GETBUTTONSIZE, 0, 0);

	SendMessage(rtoolbar, TB_SETBUTTONSIZE, 0, MAKELONG(0, HIWORD(btnsize)));

	// setup rebar

	HWND rebar = CreateWindowEx(WS_EX_COMPOSITED, REBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NORESIZE, 0, 0, 0, 0, hwnd, (HMENU)IDC_REBAR, NULL, NULL);
	if(!rebar) return FALSE;

	REBARINFO rbi = {0};
	rbi.cbSize = sizeof(rbi);

	SendMessage(rebar, RB_SETBARINFO, 0, (LPARAM)&rbi);

	REBARBANDINFO rbbi = {0};

	rbbi.cbSize = sizeof(rbbi);
	rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
	rbbi.fStyle = RBBS_CHILDEDGE | RBBS_NOGRIPPER;
	
	rbbi.hwndChild = toolbar;
	rbbi.cxMinChild = 0;
	rbbi.cyMinChild = HIWORD(btnsize);
	rbbi.cx = 100;

	SendMessage(rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi);

	RECT b1rc;
	SendMessage(rtoolbar, TB_GETRECT, IDC_RTOOLBAR_OPTIONS, (LPARAM)&b1rc);

	RECT b2rc;
	SendMessage(rtoolbar, TB_GETRECT, IDC_RTOOLBAR_ABOUT, (LPARAM)&b2rc);
	
	rbbi.hwndChild = rtoolbar;
	rbbi.cx = (b1rc.right - b1rc.left) + (b2rc.right - b2rc.left) + tbm.cxButtonSpacing * 3;
	SendMessage(rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi);

	// setup listview

	HWND list = CreateWindowEx(0, WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, 0, 0, 0, 0, hwnd, (HMENU)IDC_LIST, NULL, NULL);
	if(!list) return FALSE;

	ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(list, L"Explorer", NULL);

	const std::wstring &timestr = LoadString(IDS_TIME);
	const std::wstring &addrstr = LoadString(IDS_ADDRESS);
	const std::wstring &labelstr = LoadString(IDS_LABEL);
	const std::wstring &countstr = LoadString(IDS_COUNT);

	const LVCOLUMN cols[] = {
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(ctx->usercfg.main.columns.time),
			(LPTSTR)timestr.c_str(),
			0,
			0
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(ctx->usercfg.main.columns.address),
			(LPTSTR)addrstr.c_str(),
			0,
			1
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(ctx->usercfg.main.columns.label),
			(LPTSTR)labelstr.c_str(),
			0,
			2
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(ctx->usercfg.main.columns.count),
			(LPTSTR)countstr.c_str(),
			0,
			3
		}
	};

	InsertListViewColumns(list, cols);

	// setup context

	SetWindowStatePtr(hwnd, ctx.release());

	RECT rc;
	GetClientRect(hwnd, &rc);
	Main_OnSize(hwnd, 0, rc.right, rc.bottom);

	return TRUE;
}

static void Main_OnDestroy(HWND hwnd) {
	boost::scoped_ptr<main_ctx> ctx(GetWindowStatePtr<main_ctx>(hwnd));

	HWND lists = GetDlgItem(hwnd, IDC_LISTS);

	ctx->usercfg.main.columns.time = PixelsToPoints((unsigned int)ListView_GetColumnWidth(lists, 0));
	ctx->usercfg.main.columns.address = PixelsToPoints((unsigned int)ListView_GetColumnWidth(lists, 1));
	ctx->usercfg.main.columns.label = PixelsToPoints((unsigned int)ListView_GetColumnWidth(lists, 2));
	ctx->usercfg.main.columns.count = PixelsToPoints((unsigned int)ListView_GetColumnWidth(lists, 3));

	/*try {
		ctx->backend->m_cfg.save();
	}
	catch(std::exception &ex) {
		ReportException(hwnd, ex, __WFILE__, __LINE__);
	}*/

	PostQuitMessage(0);
}

static LRESULT Main_OnNotify(HWND hwnd, int, LPNMHDR pnmh) {
	main_ctx &ctx = *GetWindowStatePtr<main_ctx>(hwnd);

	if(pnmh->idFrom == IDC_TOOLBAR && pnmh->code == TBN_DROPDOWN) {
		NMTOOLBAR &nmtb = *(NMTOOLBAR*)pnmh;

		HMENU menu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ENABLE));
		HMENU submenu = GetSubMenu(menu, 0);

		TBBUTTONINFO tbbi;
		tbbi.cbSize = sizeof(tbbi);
		tbbi.dwMask = TBIF_LPARAM;

		SendMessage(pnmh->hwndFrom, TB_GETBUTTONINFO, IDC_TOOLBAR_ENABLE, (LPARAM)&tbbi);

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STATE;
		mii.fState = MFS_CHECKED;

		SetMenuItemInfo(submenu, (UINT)tbbi.lParam, TRUE, &mii);

		RECT rc;
		GetWindowRect(pnmh->hwndFrom, &rc);

		SetForegroundWindow(hwnd);
		TrackPopupMenuEx(submenu, TPM_LEFTALIGN | TPM_TOPALIGN, rc.left + nmtb.rcButton.left, rc.top + nmtb.rcButton.bottom, hwnd, NULL);

		DestroyMenu(menu);

		return TBDDRET_DEFAULT;
	}
	else if(pnmh->idFrom == IDC_RTOOLBAR && pnmh->code == TBN_DROPDOWN) {
		NMTOOLBAR &nmtb = *(NMTOOLBAR*)pnmh;

		HMENU menu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ABOUT));
		HMENU submenu = GetSubMenu(menu, 0);

		RECT rc;
		GetWindowRect(pnmh->hwndFrom, &rc);

		SetForegroundWindow(hwnd);
		TrackPopupMenuEx(submenu, TPM_RIGHTALIGN | TPM_TOPALIGN, rc.left + nmtb.rcButton.right, rc.top + nmtb.rcButton.bottom, hwnd, NULL);

		DestroyMenu(menu);
	}
	else if(pnmh->idFrom == IDC_REBAR && pnmh->code == NM_CUSTOMDRAW) {
		NMCUSTOMDRAW &nmcd = *(NMCUSTOMDRAW*)pnmh;

		if(nmcd.dwDrawStage == CDDS_PREERASE) {
			RECT rc;
			GetClientRect(nmcd.hdr.hwndFrom, &rc);

			ctx.tbback.Draw(nmcd.hdc, rc.right, rc.bottom);
			return CDRF_SKIPDEFAULT;
		}
	}
	else if(pnmh->idFrom == IDC_TOOLBAR && pnmh->code == NM_CUSTOMDRAW) {
		NMTBCUSTOMDRAW &tbcd = *(NMTBCUSTOMDRAW*)pnmh;

		if(tbcd.nmcd.dwDrawStage == CDDS_PREPAINT) {
			return CDRF_NOTIFYITEMDRAW;
		}
		else if(tbcd.nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
			//TODO: draw buttons for XP?
		}
	}

	return 0;
}

static void Main_OnSize(HWND hwnd, UINT state, int cx, int cy) {
	HWND list = GetDlgItem(hwnd, IDC_LIST);
	HWND rebar = GetDlgItem(hwnd, IDC_REBAR);
	HWND status = GetDlgItem(hwnd, IDC_STATUS);

	REBARBANDINFO rbbi = {0};
	rbbi.cbSize = sizeof(rbbi);
	rbbi.fMask = RBBIM_SIZE;

	SendMessage(rebar, RB_GETBANDINFO, 1, (LPARAM)&rbbi);

	rbbi.cx = cx - rbbi.cx;
	SendMessage(rebar, RB_SETBANDINFO, 0, (LPARAM)&rbbi);

	int rheight = (int)SendMessage(rebar, RB_GETBARHEIGHT, 0, 0);

	RECT src;
	GetWindowRect(status, &src);

	HDWP dwp = BeginDeferWindowPos(3);

	DeferWindowPos(dwp, rebar, NULL, 0, cy - rheight - (src.bottom - src.top), cx, rheight, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	DeferWindowPos(dwp, list, NULL, 0, 0, cx, cy - rheight - (src.bottom - src.top), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
	DeferWindowPos(dwp, status, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

	EndDeferWindowPos(dwp);
}

static void Main_OnTimer(HWND hwnd, UINT id) {
	if(id == TIMER_ID_REBARFADE) {
		main_ctx &ctx = *GetWindowStatePtr<main_ctx>(hwnd);

		DWORD curtime = GetTickCount();

		if(curtime < ctx.fadeendtime) {
			ctx.tbback.SetHue(ctx.starthue);
			ctx.tbback.Gradient(ctx.endbar, (curtime - ctx.fadestarttime) / 250.0);
		}
		else {
			ctx.tbback = ctx.endbar;
			KillTimer(hwnd, TIMER_ID_REBARFADE);
		}

		InvalidateRect(GetDlgItem(hwnd, IDC_REBAR), NULL, TRUE);
	}
}

static LRESULT CALLBACK Main_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CLOSE, Main_OnClose);
		HANDLE_MSG(hwnd, WM_COMMAND, Main_OnCommand);
		HANDLE_MSG(hwnd, WM_CREATE, Main_OnCreate);
		HANDLE_MSG(hwnd, WM_DESTROY, Main_OnDestroy);
		HANDLE_MSG(hwnd, WM_NOTIFY, Main_OnNotify);
		HANDLE_MSG(hwnd, WM_SIZE, Main_OnSize);
		HANDLE_MSG(hwnd, WM_TIMER, Main_OnTimer);
		default: return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}

void RegisterMainClass(HINSTANCE hInstance) {
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = Main_WndProc;
	wc.cbWndExtra = sizeof(main_ctx*);
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = L"Main";

	if(!RegisterClassEx(&wc)) throw win32_error("RegisterClassEx");
}

HWND CreateMainWindow(HINSTANCE hInstance) {
	HWND hwnd = CreateWindowEx(0, L"Main", L"PeerGuardian 3", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, 440, 240, NULL, NULL, hInstance, NULL);

	if(!hwnd) throw win32_error("CreateWindowEx");

	return hwnd;
}
