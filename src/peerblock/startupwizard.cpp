/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2010 PeerBlock, LLC

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
#include "resource.h"
using namespace std;

static HFONT g_font;
static bool g_p2p=true, g_ads=false, g_spy=false, g_gov=false, g_edu=false, g_custom=false;

static INT_PTR CALLBACK First_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		case WM_INITDIALOG:
			SendDlgItemMessage(hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)g_font, TRUE);
			break;
		case WM_NOTIFY: {
			NMHDR *nmh=(NMHDR*)lParam;
			switch(nmh->code) {
				case PSN_SETACTIVE:
					PropSheet_SetWizButtons(nmh->hwndFrom, PSWIZB_NEXT);
					break;
			}
		} break;
	}

	return FALSE;
}

static INT_PTR CALLBACK PickLists_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		case WM_INITDIALOG: {
			if(g_p2p) CheckDlgButton(hwnd, IDC_P2P, BST_CHECKED);
			if(g_ads) CheckDlgButton(hwnd, IDC_ADS, BST_CHECKED);
			if(g_spy) CheckDlgButton(hwnd, IDC_SPYWARE, BST_CHECKED);
			if(g_edu) CheckDlgButton(hwnd, IDC_EDUCATIONAL, BST_CHECKED);
			if(g_custom) CheckDlgButton(hwnd, IDC_CUSTOM, BST_CHECKED);
			if(!g_config.PortSet.IsHttpBlocked()) CheckDlgButton(hwnd, IDC_BLOCKHTTP, BST_CHECKED);

			tstring s=LoadString(IDS_STARTUP_LISTS);
			SetWindowText(GetDlgItem(hwnd, IDC_RECOMMEND), s.c_str());
		} break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_P2P:
					g_p2p=(IsDlgButtonChecked(hwnd, IDC_P2P)==BST_CHECKED);
					break;
				case IDC_ADS:
					g_ads=(IsDlgButtonChecked(hwnd, IDC_ADS)==BST_CHECKED);
					break;
				case IDC_SPYWARE:
					g_spy=(IsDlgButtonChecked(hwnd, IDC_SPYWARE)==BST_CHECKED);
					break;
				case IDC_EDUCATIONAL:
					g_edu=(IsDlgButtonChecked(hwnd, IDC_EDUCATIONAL)==BST_CHECKED);
					break;
				case IDC_CUSTOM:
					g_custom=(IsDlgButtonChecked(hwnd, IDC_CUSTOM)==BST_CHECKED);
					break;
				case IDC_BLOCKHTTP:
					g_config.PortSet.AllowHttp=(IsDlgButtonChecked(hwnd, IDC_BLOCKHTTP)==BST_CHECKED);
					break;
			}
			break;
		case WM_NOTIFY: {
			NMHDR *nmh=(NMHDR*)lParam;
			switch(nmh->code) {
				case PSN_SETACTIVE:
					PropSheet_SetWizButtons(nmh->hwndFrom, PSWIZB_BACK|PSWIZB_NEXT);
					break;
				case PSN_WIZNEXT:
					if(!g_custom) {
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_STARTUP_UPDATES);
						return TRUE;
					}
					break;
			}
		} break;
	}

	return FALSE;
}

static INT_PTR CALLBACK Updates_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		case WM_INITDIALOG: {
			int radio;
			switch(g_config.UpdateInterval) {
				case 1: radio=IDC_EVERYDAY; break;
				case 2: radio=IDC_EVERYOTHERDAY; break;
				case 7: radio=IDC_EVERYWEEK; break;
				default: radio=IDC_EVERYXDAYS; break;
			}
			CheckRadioButton(hwnd, IDC_EVERYDAY, IDC_EVERYXDAYS, radio);

			SetDlgItemInt(hwnd, IDC_CUSTOM, g_config.UpdateInterval, FALSE);
			SendDlgItemMessage(hwnd, IDC_CUSTOM, EM_SETLIMITTEXT, 2, 0);
			SendDlgItemMessage(hwnd, IDC_CUSTOMSPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwnd, IDC_CUSTOM), 0);
			SendDlgItemMessage(hwnd, IDC_CUSTOMSPIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(30, 1));

			if(radio!=IDC_EVERYXDAYS) {
				EnableWindow(GetDlgItem(hwnd, IDC_CUSTOM), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_CUSTOMSPIN), FALSE);
			}

			if(g_config.UpdatePeerBlock) CheckDlgButton(hwnd, IDC_PEERBLOCK, BST_CHECKED);
			if(g_config.UpdateLists) CheckDlgButton(hwnd, IDC_LISTS, BST_CHECKED);

			tstring s=LoadString(IDS_STARTUP_UPDATES);
			SetWindowText(GetDlgItem(hwnd, IDC_RECOMMEND), s.c_str());
		} break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_EVERYDAY:
					g_config.UpdateInterval=1;
					EnableWindow(GetDlgItem(hwnd, IDC_CUSTOM), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CUSTOMSPIN), FALSE);
					break;
				case IDC_EVERYOTHERDAY:
					g_config.UpdateInterval=2;
					EnableWindow(GetDlgItem(hwnd, IDC_CUSTOM), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CUSTOMSPIN), FALSE);
					break;
				case IDC_EVERYWEEK:
					g_config.UpdateInterval=7;
					EnableWindow(GetDlgItem(hwnd, IDC_CUSTOM), FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CUSTOMSPIN), FALSE);
					break;
				case IDC_EVERYXDAYS:
					g_config.UpdateInterval=GetDlgItemInt(hwnd, IDC_EVERYXDAYS, NULL, FALSE);
					EnableWindow(GetDlgItem(hwnd, IDC_CUSTOM), TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_CUSTOMSPIN), TRUE);
					break;
				case IDC_CUSTOM:
					g_config.UpdateInterval=GetDlgItemInt(hwnd, IDC_CUSTOM, NULL, FALSE);
					break;
				case IDC_PEERBLOCK:
					g_config.UpdatePeerBlock=(IsDlgButtonChecked(hwnd, IDC_PEERBLOCK)==BST_CHECKED);
					break;
				case IDC_LISTS:
					g_config.UpdateLists=(IsDlgButtonChecked(hwnd, IDC_LISTS)==BST_CHECKED);
					break;
			}
			break;
		case WM_NOTIFY: {
			NMHDR *nmh=(NMHDR*)lParam;
			switch(nmh->code) {
				case PSN_SETACTIVE:
					PropSheet_SetWizButtons(nmh->hwndFrom, PSWIZB_BACK|PSWIZB_NEXT);
					break;
				case PSN_WIZBACK:
					if(!g_custom) {
						SetWindowLongPtr(hwnd, DWLP_MSGRESULT, IDD_STARTUP_LISTS);
						return TRUE;
					}
					break;
			}
		} break;
	}

	return FALSE;
}

static INT_PTR CALLBACK Last_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		case WM_INITDIALOG: {
			SendDlgItemMessage(hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)g_font, TRUE);

			tstring s=LoadString(IDS_STARTUP_LAST);
			SetWindowText(GetDlgItem(hwnd, IDC_RECOMMEND), s.c_str());
		} break;
		case WM_NOTIFY: {
			NMHDR *nmh=(NMHDR*)lParam;
			switch(nmh->code) {
				case PSN_SETACTIVE:
					PropSheet_SetWizButtons(nmh->hwndFrom, PSWIZB_BACK|PSWIZB_FINISH);
					break;
			}
		} break;
	}

	return FALSE;
}

static HFONT MakeFont(LPCTSTR name, int size, bool bold) {
	NONCLIENTMETRICS ncm={0};
	ncm.cbSize=sizeof(ncm);

	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);

	LOGFONT lf=ncm.lfMessageFont;
	StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), name);

	if(bold) lf.lfWeight=FW_BOLD;

	HDC hdc = GetDC(NULL);
	lf.lfHeight = -MulDiv(size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(NULL, hdc);

	return CreateFontIndirect(&lf);
}

void DisplayStartupWizard(HWND parent) {
	g_font=MakeFont(_T("Verdana Bold"), 12, true);

	PROPSHEETPAGE psp[5]={0};
	HPROPSHEETPAGE hpsp[5]={0};

	psp[0].dwSize=sizeof(PROPSHEETPAGE);
	psp[0].dwFlags=PSP_DEFAULT|PSP_USETITLE|PSP_HIDEHEADER;
	psp[0].hInstance=GetModuleHandle(NULL);
	psp[0].pszTitle=MAKEINTRESOURCE(IDS_STARTUPWIZ);
	psp[0].pszTemplate=MAKEINTRESOURCE(IDD_STARTUP_FIRST);
	psp[0].pfnDlgProc=First_DlgProc;
	hpsp[0]=CreatePropertySheetPage(&psp[0]);

	psp[1].dwSize=sizeof(PROPSHEETPAGE);
	psp[1].dwFlags=PSP_DEFAULT|PSP_USETITLE|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	psp[1].hInstance=GetModuleHandle(NULL);
	psp[1].pszTitle=MAKEINTRESOURCE(IDS_STARTUPWIZ);
	psp[1].pszHeaderTitle=MAKEINTRESOURCE(IDS_SELECTLISTS);
	psp[1].pszHeaderSubTitle=MAKEINTRESOURCE(IDS_SELECTLISTSSUB);
	psp[1].pszTemplate=MAKEINTRESOURCE(IDD_STARTUP_LISTS);
	psp[1].pfnDlgProc=PickLists_DlgProc;
	hpsp[1]=CreatePropertySheetPage(&psp[1]);

	psp[2].dwSize=sizeof(PROPSHEETPAGE);
	psp[2].dwFlags=PSP_DEFAULT|PSP_USETITLE|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	psp[2].hInstance=GetModuleHandle(NULL);
	psp[2].pszTitle=MAKEINTRESOURCE(IDS_STARTUPWIZ);
	psp[2].pszHeaderTitle=MAKEINTRESOURCE(IDS_CUSTOMIZELISTS);
	psp[2].pszHeaderSubTitle=MAKEINTRESOURCE(IDS_CUSTOMIZELISTSSUB);
	psp[2].pszTemplate=MAKEINTRESOURCE(IDD_LISTS);
	psp[2].pfnDlgProc=Lists_DlgProc;
	hpsp[2]=CreatePropertySheetPage(&psp[2]);

	psp[3].dwSize=sizeof(PROPSHEETPAGE);
	psp[3].dwFlags=PSP_DEFAULT|PSP_USETITLE|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	psp[3].hInstance=GetModuleHandle(NULL);
	psp[3].pszTitle=MAKEINTRESOURCE(IDS_STARTUPWIZ);
	psp[3].pszHeaderTitle=MAKEINTRESOURCE(IDS_AUTOUPDATES);
	psp[3].pszHeaderSubTitle=MAKEINTRESOURCE(IDS_AUTOUPDATESSUB);
	psp[3].pszTemplate=MAKEINTRESOURCE(IDD_STARTUP_UPDATES);
	psp[3].pfnDlgProc=Updates_DlgProc;
	hpsp[3]=CreatePropertySheetPage(&psp[3]);

	psp[4].dwSize=sizeof(PROPSHEETPAGE);
	psp[4].dwFlags=PSP_DEFAULT|PSP_USETITLE|PSP_HIDEHEADER;
	psp[4].hInstance=GetModuleHandle(NULL);
	psp[4].pszTitle=MAKEINTRESOURCE(IDS_STARTUPWIZ);
	psp[4].pszTemplate=MAKEINTRESOURCE(IDD_STARTUP_LAST);
	psp[4].pfnDlgProc=Last_DlgProc;
	hpsp[4]=CreatePropertySheetPage(&psp[4]);

	PROPSHEETHEADER psh={0};
	psh.dwSize=sizeof(PROPSHEETHEADER);
	psh.dwFlags=PSH_WIZARD97 | PSH_USEHICON;
	psh.hInstance=GetModuleHandle(NULL);
	psh.hIcon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MAIN)); // Loaded main icon since this scren is only seen if the config file could not be loaded
	psh.phpage=hpsp;                                                       // This implies that there is no way to retrieve the state of the system tray icon
	psh.nStartPage=0;
	psh.nPages=sizeof(hpsp)/sizeof(HPROPSHEETPAGE);
	psh.hwndParent=parent;

	if(PropertySheet(&psh)) {
		DynamicList dl;
		dl.Type=List::Block;
		vector<DynamicList>::size_type idx=0;

		idx = FindUrl(g_presets[0], g_config.DynamicLists);
		if(g_ads && idx == -1) {
			dl.Url=g_presets[0];
			dl.Description=LoadString(IDS_ADS);
			g_config.DynamicLists.push_back(dl);
		}

		idx = FindUrl(g_presets[1], g_config.DynamicLists);
		if(g_edu && idx == -1) {
			dl.Url=g_presets[1];
			dl.Description=LoadString(IDS_EDU);
			g_config.DynamicLists.push_back(dl);
		}

		idx = FindUrl(g_presets[2], g_config.DynamicLists);
		if(g_p2p && idx == -1) {
			dl.Url=g_presets[2];
			dl.Description=LoadString(IDS_P2P);
			g_config.DynamicLists.push_back(dl);
		}

		idx = FindUrl(g_presets[3], g_config.DynamicLists);
		if(g_spy && idx == -1) {
			dl.Url=g_presets[3];
			dl.Description=LoadString(IDS_SPY);
			g_config.DynamicLists.push_back(dl);
		}
	}

	DeleteObject(g_font);
}
