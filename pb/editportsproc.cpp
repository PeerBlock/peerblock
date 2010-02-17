/*
	Copyright (C) 2009-2010 PeerBlock, LLC

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

HWND g_hEditPortsDlg = NULL;

static void EditPorts_OnClose(HWND hwnd)
{
	HWND list = GetDlgItem(hwnd, IDC_PORTS);
	for (vector<PortProfile>::size_type i = 0; i < g_config.PortSet.Profiles.size(); i++) {
		g_config.PortSet.Profiles[i].Enabled = ListView_GetCheckState(list, i) ? true : false;
	}

	g_config.PortSet.Merge();
	g_filter->setdestinationports(g_config.PortSet.DestinationPorts);
	g_filter->setsourceports(g_config.PortSet.SourcePorts);

	EndDialog(hwnd, NULL);
}

static void About_OnDestroy(HWND hwnd) {
	g_hEditPortsDlg = NULL;
}

static void EditPorts_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
		case IDC_HTTPPORT:
			g_config.PortSet.AllowHttp = (IsDlgButtonChecked(hwnd, IDC_HTTPPORT) == BST_CHECKED);
			break;
		case IDC_FTPPORT:
			g_config.PortSet.AllowFtp = (IsDlgButtonChecked(hwnd, IDC_FTPPORT) == BST_CHECKED);
			break;
		case IDC_SMTPPORT:
			g_config.PortSet.AllowSmtp = (IsDlgButtonChecked(hwnd, IDC_SMTPPORT) == BST_CHECKED);
			break;
		case IDC_POP3PORT:
			g_config.PortSet.AllowPop3 = (IsDlgButtonChecked(hwnd, IDC_POP3PORT) == BST_CHECKED);
			break;
		case IDC_ADD: {
			PortProfile p;

			if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PORTPROFILE), hwnd, PortProfile_DlgProc, (LPARAM) &p)==IDOK) {
				g_config.PortSet.Profiles.push_back(p);

				HWND list = GetDlgItem(hwnd, IDC_PORTS);
				int idx = ListView_GetItemCount(list);

				LVITEM lvi = {0};
				lvi.mask = LVIF_TEXT|LVIF_PARAM;
				lvi.iItem = idx;

				lvi.iSubItem = 0;

				lvi.mask = LVIF_TEXT;
				lvi.pszText = (LPTSTR) p.Name.c_str();

				ListView_InsertItem(list, &lvi);

				ListView_SetCheckState(list, idx, true);
			}
		} break;
		case IDC_REMOVE:
			if (MessageBox(hwnd, _T("Are you sure you want to delete the selected profiles?"), _T("Delete profiles"), MB_YESNO) == IDYES) {
				HWND list = GetDlgItem(hwnd, IDC_PORTS);

				stack<int> items;
				for(int idx = ListView_GetNextItem(list, -1, LVNI_SELECTED); idx != -1; idx = ListView_GetNextItem(list, idx, LVNI_SELECTED))
					items.push(idx);

				while (items.size() > 0) {
					int idx = items.top();
					items.pop();

					g_config.PortSet.Profiles.erase(g_config.PortSet.Profiles.begin() + idx);
					ListView_DeleteItem(list, idx);
				}
			}
			break;
		case IDC_EDIT: {
			HWND list = GetDlgItem(hwnd, IDC_PORTS);
			int idx = ListView_GetNextItem(list, -1, LVNI_SELECTED);

			if (idx > -1) {
				PortProfile p = g_config.PortSet.Profiles[idx];

				if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PORTPROFILE), hwnd, PortProfile_DlgProc, (LPARAM) &p)==IDOK) {
					g_config.PortSet.Profiles[idx] = p;
					ListView_SetItemText(list, idx, 0, (LPWSTR) p.Name.c_str());
				}
			}
		} break;
	}
}


static void EditPorts_OnDestroy(HWND hwnd)
{
	SaveWindowPosition(hwnd, g_config.PortSetWindowPos);
}


static void EditPorts_OnSize(HWND hwnd, UINT state, int cx, int cy) 
{
	HWND add=GetDlgItem(hwnd, IDC_ADD);
	HWND edit=GetDlgItem(hwnd, IDC_EDIT);
	HWND remove=GetDlgItem(hwnd, IDC_REMOVE);

	HWND ports=GetDlgItem(hwnd, IDC_PORTS);
	HWND defgroup=GetDlgItem(hwnd, IDC_DEFGROUP);

	RECT rc;
	GetWindowRect(add, &rc);

	HDWP dwp=BeginDeferWindowPos(8);

	DeferWindowPos(dwp, defgroup, NULL, 7, 2, cx-14, 120, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, ports, NULL, 7, 130, cx-14, cy-21-140-(rc.bottom-rc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, add, NULL, cx-((rc.right-rc.left+7)*3), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, edit, NULL, cx-((rc.right-rc.left+7)*2), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, remove, NULL, cx-(rc.right-rc.left+7), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	EndDeferWindowPos(dwp);

	if (state == SIZE_RESTORED) 
	{
		SaveWindowPosition(hwnd, g_config.PortSetWindowPos);
	}
}


static BOOL EditPorts_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	g_hEditPortsDlg = hwnd;

	CheckDlgButton(hwnd, IDC_HTTPPORT, g_config.PortSet.AllowHttp);
	CheckDlgButton(hwnd, IDC_FTPPORT, g_config.PortSet.AllowFtp);
	CheckDlgButton(hwnd, IDC_SMTPPORT, g_config.PortSet.AllowSmtp);
	CheckDlgButton(hwnd, IDC_POP3PORT, g_config.PortSet.AllowPop3);

	HWND list = GetDlgItem(hwnd, IDC_PORTS);
	ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);

	LVCOLUMN lvc = {0};
	
	lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 300;
	lvc.iSubItem = 0;
	lvc.pszText = _T("Name");

	ListView_InsertColumn(list, 0, &lvc);

	int idx = 0;
	for (vector<StaticList>::size_type i = 0; i < g_config.PortSet.Profiles.size(); i++) {
		PortProfile profile = g_config.PortSet.Profiles[i];

		LVITEM lvi = {0};
		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.iItem = idx;

		lvi.iSubItem = 0;
		lvi.lParam = (LPARAM) &profile;
		lvi.pszText = (LPTSTR) profile.Name.c_str();

		ListView_InsertItem(list, &lvi);

		lvi.mask = LVIF_TEXT;

		ListView_SetCheckState(list, idx, profile.Enabled);

		idx++;
	}

	if(	g_config.PortSetWindowPos.left!=0 || g_config.PortSetWindowPos.top!=0 ||
		g_config.PortSetWindowPos.right!=0 || g_config.PortSetWindowPos.bottom!=0 ) 
	{
		SetWindowPos(hwnd, NULL,
			g_config.PortSetWindowPos.left,
			g_config.PortSetWindowPos.top,
			g_config.PortSetWindowPos.right-g_config.PortSetWindowPos.left,
			g_config.PortSetWindowPos.bottom-g_config.PortSetWindowPos.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER );
	}

	RECT rc;
	GetClientRect(hwnd, &rc);
	EditPorts_OnSize(hwnd, 0, rc.right, rc.bottom);

	// Load peerblock icon in the windows titlebar
	RefreshDialogIcon(hwnd);

	return TRUE;
}


static INT_PTR EditPorts_OnNotify(HWND hwnd, int idCtrl, NMHDR *nmh) {
	if (nmh->idFrom == IDC_PORTS) 
	{
		switch(nmh->code) 
		{
			case LVN_ITEMCHANGED: 
			{
				unsigned int num = ListView_GetSelectedCount(nmh->hwndFrom);

				BOOL edit, remove;

				switch(num) 
				{
					case 0:
						edit=FALSE;
						remove=FALSE;
						break;
					case 1:
						edit=TRUE;
						remove=TRUE;
					break;
					default:
						edit=FALSE;
						remove=TRUE;
				}

				EnableWindow(GetDlgItem(hwnd, IDC_EDIT), edit);
				EnableWindow(GetDlgItem(hwnd, IDC_REMOVE), remove);
			} break;
			case NM_DBLCLK: {
				if (ListView_GetSelectedCount(nmh->hwndFrom) == 1) {
					SendMessage(GetDlgItem(hwnd, IDC_EDIT), BM_CLICK, 0, 0);
				}
			} break;
		}
	}

	return 0;
}


static void EditPorts_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) 
{
	RECT rc={0};
	rc.right=349;
	rc.bottom=219;

	MapDialogRect(hwnd, &rc);

	lpMinMaxInfo->ptMinTrackSize.x=rc.right;
	lpMinMaxInfo->ptMinTrackSize.y=rc.bottom;
}


INT_PTR CALLBACK EditPorts_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
	try 
	{
		switch(msg) 
		{
			HANDLE_MSG(hwnd, WM_GETMINMAXINFO, EditPorts_OnGetMinMaxInfo);
			HANDLE_MSG(hwnd, WM_COMMAND, EditPorts_OnCommand);
			HANDLE_MSG(hwnd, WM_INITDIALOG, EditPorts_OnInitDialog);
			HANDLE_MSG(hwnd, WM_NOTIFY, EditPorts_OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, EditPorts_OnSize);
			HANDLE_MSG(hwnd, WM_CLOSE, EditPorts_OnClose);
			HANDLE_MSG(hwnd, WM_DESTROY, EditPorts_OnDestroy);
			case WM_DIALOG_ICON_REFRESH:
				RefreshDialogIcon(hwnd);
				return 1;
			default: return 0;
		}
	}
	catch(exception &ex) 
	{
		UncaughtExceptionBox(hwnd, ex, __FILE__, __LINE__);
		return 0;
	}
	catch(...) 
	{
		UncaughtExceptionBox(hwnd, __FILE__, __LINE__);
		return 0;
	}

}