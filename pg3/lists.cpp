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

struct lists_ctx {
	configs &cfgs;
	global_config cfg;
	int profileidx;

	lists_ctx(configs &cfgs) : cfgs(cfgs),profileidx(0) {
		global_config::scoped_lock lock(cfgs.cfg);
		cfg = cfgs.cfg;
	}

	void fill_profiles(HWND profiles) const {
		ComboBox_ResetContent(profiles);

		for(std::vector<pg3_profile>::const_iterator iter = cfg.lists.profiles.begin(), end = cfg.lists.profiles.end(); iter != end; ++iter) {
			ComboBox_AddString(profiles, iter->label.c_str());
		}

		const std::wstring &newprofile = LoadString(IDS_NEWPROFILE);
		const std::wstring &editprofile = LoadString(IDS_EDITPROFILE);

		ComboBox_AddString(profiles, newprofile.c_str());
		ComboBox_AddString(profiles, editprofile.c_str());
	}
};

static void Lists_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	lists_ctx &ctx = *GetDialogStatePtr<lists_ctx>(hwnd);

	switch(id) {
		case IDC_PROFILES:
			if(codeNotify == CBN_SELCHANGE) {
				int idx = ComboBox_GetCurSel(hwndCtl);
				int count = ComboBox_GetCount(hwndCtl);

				if(idx == (count - 2)) {
					pg3_profile p;
					if(ShowAddProfileDialog(hwnd, ctx.cfg, p)) {
						ctx.cfg.lists.profiles.push_back(p);
						std::sort(ctx.cfg.lists.profiles.begin(), ctx.cfg.lists.profiles.end());

						idx = 0;
						for(std::vector<pg3_profile>::iterator iter = ctx.cfg.lists.profiles.begin(), end = ctx.cfg.lists.profiles.end(); iter != end; ++iter) {
							if(iter->label == p.label) break;
							else ++idx;
						}

						ComboBox_InsertString(hwndCtl, idx, p.label.c_str());
						ComboBox_SetCurSel(hwndCtl, idx);
					}
					else {
						ComboBox_SetCurSel(hwndCtl, ctx.profileidx);
					}
				}
				else if(idx == (count - 1)) {
					std::vector<pg3_profile>::size_type lastsize = ctx.cfg.lists.profiles.size();
					
					ShowEditProfilesDialog(hwnd, ctx.cfg);

					int idx = (ctx.cfg.lists.profiles.size() == lastsize) ? ctx.profileidx : 0;

					ctx.fill_profiles(hwndCtl);
					ComboBox_SetCurSel(hwndCtl, idx);

					//TODO: fill lists.
				}
				else {
					ctx.profileidx = ComboBox_GetCurSel(hwndCtl);
					//TODO: fill lists.
				}
			}
			break;
		case IDC_ADD:
		case IDC_EDIT:
			ShowAddListDialog(hwnd);
			break;
		case IDC_REMOVE:
			break;
	}
}

static void Lists_OnDestroy(HWND hwnd) {
	lists_ctx *ctx = GetDialogStatePtr<lists_ctx>(hwnd);

	HWND presets = GetDlgItem(hwnd, IDC_PRESETS);
	HWND lists = GetDlgItem(hwnd, IDC_LISTS);

	ctx->cfgs.ucfg.lists.presets.label = PixelsToPoints((unsigned int)ListView_GetColumnWidth(presets, 0));
	ctx->cfgs.ucfg.lists.presets.type = PixelsToPoints((unsigned int)ListView_GetColumnWidth(presets, 1));
	ctx->cfgs.ucfg.lists.presets.status = PixelsToPoints((unsigned int)ListView_GetColumnWidth(presets, 2));
	
	ctx->cfgs.ucfg.lists.custom.label = PixelsToPoints((unsigned int)ListView_GetColumnWidth(lists, 0));
	ctx->cfgs.ucfg.lists.custom.location = PixelsToPoints((unsigned int)ListView_GetColumnWidth(lists, 1));
	ctx->cfgs.ucfg.lists.custom.type = PixelsToPoints((unsigned int)ListView_GetColumnWidth(lists, 2));
	ctx->cfgs.ucfg.lists.custom.status = PixelsToPoints((unsigned int)ListView_GetColumnWidth(lists, 3));

	delete ctx;
}

typedef pg3_profile::tag_presets presets_t;

struct preset {
	const wchar_t *publisher;
	UINT label;
	UINT type;
	bool presets_t::* member;
};

static const preset g_presets[] = {
	{ L"Phoenix Labs", IDS_ANTIP2P, IDS_BLOCK, &presets_t::p2p },
	{ L"Phoenix Labs", IDS_ADS, IDS_BLOCK, &presets_t::ads },
	{ L"Phoenix Labs", IDS_EDU, IDS_BLOCK, &presets_t::edu },
	{ L"Phoenix Labs", IDS_SPYWARE, IDS_BLOCK, &presets_t::spy },
	{ L"Phoenix Labs", IDS_GAMING, IDS_ALLOW, &presets_t::gaming },
	{ L"Bluetack", IDS_LEVEL1, IDS_BLOCK, &presets_t::bt_level1 },
	{ L"Bluetack", IDS_LEVEL2, IDS_BLOCK, &presets_t::bt_level2 },
	{ L"Bluetack", IDS_LEVEL3, IDS_BLOCK, &presets_t::bt_level3 },
	{ L"Bluetack", IDS_BOGON, IDS_BLOCK, &presets_t::bt_bogon },
	{ L"Bluetack", IDS_DSHIELD, IDS_BLOCK, &presets_t::bt_dshield },
	{ L"Bluetack", IDS_EDU, IDS_BLOCK, &presets_t::bt_edu },
	{ L"Bluetack", IDS_HIJACKED, IDS_BLOCK, &presets_t::bt_hijacked },
	{ L"Bluetack", IDS_MULTICAST, IDS_BLOCK, &presets_t::bt_multicast },
	{ L"Bluetack", IDS_PRIVATE, IDS_BLOCK, &presets_t::bt_private },
	{ L"Bluetack", IDS_RESERVED, IDS_BLOCK, &presets_t::bt_reserved },
	{ L"Bluetack", IDS_BTALLOWS, IDS_ALLOW, &presets_t::bt_allows },
	{ L"Bluetack", IDS_LANRANGES, IDS_BLOCK, &presets_t::bt_lan },
	{ L"Bluetack", IDS_SPIDERS, IDS_BLOCK, &presets_t::bt_spiders },
	{ L"Bluetack", IDS_SPYWARE, IDS_BLOCK, &presets_t::bt_spyware },
	{ L"Bluetack", IDS_TROJANS, IDS_BLOCK, &presets_t::bt_trojans }
};

static const std::size_t g_presetcount = sizeof(g_presets) / sizeof(preset);

static BOOL Lists_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	configs &cfgs = *(configs*)((PROPSHEETPAGE*)lParam)->lParam;
	
	std::auto_ptr<lists_ctx> ctx(new lists_ctx(cfgs));

	int taborder[] = { IDC_PROFILES, IDC_PRESETS, IDC_LISTS, IDC_ADD, IDC_EDIT, IDC_REMOVE };
	SetTabOrder(hwnd, taborder);

	// setup profiles.

	if(ctx->cfg.lists.profiles.empty()) {
		pg3_profile p;
		p.label = LoadString(IDS_DEFAULT);
		p.lists.resize(ctx->cfg.lists.lists.size());
		p.presets.p2p = true;

		ctx->cfg.lists.profiles.push_back(p);
		ctx->cfg.lists.curprofile = 0;
	}

	HWND profiles = GetDlgItem(hwnd, IDC_PROFILES);

	ctx->fill_profiles(profiles);
	ComboBox_SetCurSel(profiles, 0);
	//TODO: fill lists.

	// setup preset lists.

	const std::wstring &publisher = LoadString(IDS_PUBLISHER);
	const std::wstring &label = LoadString(IDS_LABEL);
	const std::wstring &location = LoadString(IDS_LOCATION);
	const std::wstring &type = LoadString(IDS_TYPE);
	const std::wstring &status = LoadString(IDS_STATUS);

	const LVCOLUMN presetcols[] = {
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.presets.label),
			(LPTSTR)label.c_str(),
			0,
			0
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.presets.publisher),
			(LPTSTR)publisher.c_str(),
			0,
			1
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.presets.type),
			(LPTSTR)type.c_str(),
			0,
			2
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.presets.status),
			(LPTSTR)status.c_str(),
			0,
			3
		}
	};

	HWND presets = GetDlgItem(hwnd, IDC_PRESETS);

	ListView_SetExtendedListViewStyle(presets, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(presets, L"Explorer", NULL);
	InsertListViewColumns(presets, presetcols);

	for(std::size_t i = 0; i < g_presetcount; ++i) {
		const std::wstring &label = LoadString(g_presets[i].label);
		const std::wstring &type = LoadString(g_presets[i].type);

		LVITEM lvi = {0};

		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.iItem = (int)i;
		lvi.lParam = (LPARAM)&g_presets[i];

		lvi.pszText = (LPWSTR)label.c_str();
		ListView_InsertItem(presets, &lvi);

		
		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 1;
		lvi.pszText = (LPWSTR)g_presets[i].publisher;
		ListView_SetItem(presets, &lvi);

		lvi.iSubItem = 2;
		lvi.pszText = (LPWSTR)type.c_str();
		ListView_SetItem(presets, &lvi);
	}

	// setup custom lists.

	const LVCOLUMN customcols[] = {
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.custom.label),
			(LPTSTR)label.c_str(),
			0,
			0
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.custom.location),
			(LPTSTR)location.c_str(),
			0,
			1
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.custom.type),
			(LPTSTR)type.c_str(),
			0,
			2
		},
		{
			LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			LVCFMT_LEFT,
			PointsToPixels(cfgs.ucfg.lists.custom.status),
			(LPTSTR)status.c_str(),
			0,
			3
		}
	};

	HWND lists = GetDlgItem(hwnd, IDC_LISTS);
	
	ListView_SetExtendedListViewStyle(lists, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_DOUBLEBUFFER);
	SetWindowTheme(lists, L"Explorer", NULL);
	InsertListViewColumns(lists, customcols);
	
	SetDialogStatePtr(hwnd, ctx.release());

	return TRUE;
}

static LRESULT Lists_OnNotify(HWND hwnd, int, LPNMHDR pnmh) {
	return 0;
}

INT_PTR CALLBACK Lists_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_COMMAND, Lists_OnCommand);
		HANDLE_MSG(hwnd, WM_DESTROY, Lists_OnDestroy);
		HANDLE_MSG(hwnd, WM_INITDIALOG, Lists_OnInitDialog);
		HANDLE_MSG(hwnd, WM_NOTIFY, Lists_OnNotify);
		default: return FALSE;
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}
