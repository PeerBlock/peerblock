/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2010 PeerBlock, LLC

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

HWND g_hHistoryFindDlg = NULL;

static void HistoryFind_OnClose(HWND hwnd) {
	DestroyWindow(hwnd);
}

static void HistoryFind_OnDestroy(HWND hwnd) {
	g_hHistoryFindDlg = NULL;
}

static void HistoryFind_FillSearchInfo(HWND hwnd, HFM_SEARCHINFO &hf) {
	hf.iMask=0;

	if(IsDlgButtonChecked(hwnd, IDC_RANGE)==BST_CHECKED) {
		hf.iMask|=HFM_RANGE;
		hf.cRangeMax=GetDlgItemText(hwnd, IDC_RANGENAME, hf.lpRange, hf.cRangeMax);
	}

	if(IsDlgButtonChecked(hwnd, IDC_SOURCE)==BST_CHECKED) {
		hf.iMask|=HFM_SOURCE;
		SendDlgItemMessage(hwnd, IDC_SOURCEIP, IPM_GETADDRESS, 0, (LPARAM)&hf.uSource);
	}

	if(IsDlgButtonChecked(hwnd, IDC_DEST)==BST_CHECKED) {
		hf.iMask|=HFM_DEST;
		SendDlgItemMessage(hwnd, IDC_DESTIP, IPM_GETADDRESS, 0, (LPARAM)&hf.uDest);
	}

	if(IsDlgButtonChecked(hwnd, IDC_FROM)==BST_CHECKED) {
		hf.iMask|=HFM_FROM;
		MonthCal_GetCurSel(GetDlgItem(hwnd, IDC_FROMDATE), &hf.stFrom);
	}

	if(IsDlgButtonChecked(hwnd, IDC_TO)==BST_CHECKED) {
		hf.iMask|=HFM_TO;
		MonthCal_GetCurSel(GetDlgItem(hwnd, IDC_TODATE), &hf.stTo);
	}

	if(IsDlgButtonChecked(hwnd, IDC_PROTOCOL)==BST_CHECKED) {
		hf.iMask|=HFM_PROTOCOL;

		switch(ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_PROTOCOLLIST))) {
			case 0: hf.iProtocol=IPPROTO_ICMP; break;
			case 1: hf.iProtocol=IPPROTO_IGMP; break;
			case 2: hf.iProtocol=IPPROTO_GGP; break;
			case 3: hf.iProtocol=IPPROTO_TCP; break;
			case 4: hf.iProtocol=IPPROTO_PUP; break;
			case 5: hf.iProtocol=IPPROTO_UDP; break;
			case 6: hf.iProtocol=IPPROTO_IDP; break;
			case 7: hf.iProtocol=IPPROTO_ND; break;
			default: __assume(0);
		}
	}

	if(IsDlgButtonChecked(hwnd, IDC_ACTION)==BST_CHECKED) {
		hf.iMask|=HFM_ACTION;
		hf.iAction=ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_ACTIONLIST))==0;
	}
}

static void HistoryFind_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDC_FIND: {
			UINT id=(UINT)GetDlgCtrlID(hwnd);

			TCHAR buf[256];

			HFNM_SEARCH hf;

			hf.hdr.code=HFN_SEARCH;
			hf.hdr.hwndFrom=hwnd;
			hf.hdr.idFrom=id;

			hf.info.lpRange=buf;
			hf.info.cRangeMax=256;
			HistoryFind_FillSearchInfo(hwnd, hf.info);

			SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)id, (LPARAM)&hf);
		} break;
		case IDC_RANGE:
			EnableWindow(GetDlgItem(hwnd, IDC_RANGENAME), IsDlgButtonChecked(hwnd, IDC_RANGE)==BST_CHECKED);
			break;
		case IDC_SOURCE:
			EnableWindow(GetDlgItem(hwnd, IDC_SOURCEIP), IsDlgButtonChecked(hwnd, IDC_SOURCE)==BST_CHECKED);
			break;
		case IDC_DEST:
			EnableWindow(GetDlgItem(hwnd, IDC_DESTIP), IsDlgButtonChecked(hwnd, IDC_DEST)==BST_CHECKED);
			break;
		case IDC_FROM:
			EnableWindow(GetDlgItem(hwnd, IDC_FROMDATE), IsDlgButtonChecked(hwnd, IDC_FROM)==BST_CHECKED);
			break;
		case IDC_TO:
			EnableWindow(GetDlgItem(hwnd, IDC_TODATE), IsDlgButtonChecked(hwnd, IDC_TO)==BST_CHECKED);
			break;
		case IDC_PROTOCOL:
			EnableWindow(GetDlgItem(hwnd, IDC_PROTOCOLLIST), IsDlgButtonChecked(hwnd, IDC_PROTOCOL)==BST_CHECKED);
			break;
		case IDC_ACTION:
			EnableWindow(GetDlgItem(hwnd, IDC_ACTIONLIST), IsDlgButtonChecked(hwnd, IDC_ACTION)==BST_CHECKED);
			break;
	}
}

static LPCTSTR const g_protocols[]={
	_T("ICMP"),
	_T("IGMP"),
	_T("Gateway^2"),
	_T("TCP"),
	_T("PUP"),
	_T("UDP"),
	_T("XNS IDP"),
	_T("NetDisk")
};
static const size_t g_numprotocols=sizeof(g_protocols)/sizeof(LPCTSTR);

static BOOL HistoryFind_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	g_hHistoryFindDlg = hwnd;

	HWND protocol=GetDlgItem(hwnd, IDC_PROTOCOLLIST);
	for(size_t i=0; i<g_numprotocols; i++)
		ComboBox_AddString(protocol, g_protocols[i]);
	ComboBox_SetCurSel(protocol, 3);

	HWND action=GetDlgItem(hwnd, IDC_ACTIONLIST);
	ComboBox_AddString(action, LoadString(IDS_BLOCKED).c_str());
	ComboBox_AddString(action, LoadString(IDS_ALLOWED).c_str());
	ComboBox_SetCurSel(action, 0);

	HWND srcip=GetDlgItem(hwnd, IDC_SOURCEIP);
	HWND destip=GetDlgItem(hwnd, IDC_DESTIP);
	
	EnableWindow(srcip, FALSE);
	InvalidateRect(srcip, NULL, TRUE);

	EnableWindow(destip, FALSE);
	InvalidateRect(destip, NULL, TRUE);

	// Load peerblock icon in the windows titlebar
	RefreshDialogIcon(hwnd);

	return TRUE;
}

INT_PTR CALLBACK HistoryFind_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_COMMAND, HistoryFind_OnCommand);
			HANDLE_MSG(hwnd, WM_INITDIALOG, HistoryFind_OnInitDialog);
			HANDLE_MSG(hwnd, WM_CLOSE, HistoryFind_OnClose);
			HANDLE_MSG(hwnd, WM_DESTROY, HistoryFind_OnDestroy);
			case WM_DIALOG_ICON_REFRESH:
				RefreshDialogIcon(hwnd);
				return 1;
			case HFM_GETSEARCH: HistoryFind_FillSearchInfo(hwnd, *((HFM_SEARCHINFO*)lParam));
			default: return 0;
		}
	}
	catch(exception &ex) {
		UncaughtExceptionBox(hwnd, ex, __FILE__, __LINE__);
		return 0;
	}
	catch(...) {
		UncaughtExceptionBox(hwnd, __FILE__, __LINE__);
		return 0;
	}
}
