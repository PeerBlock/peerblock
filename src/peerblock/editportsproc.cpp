/*
	Copyright (C) 2009-2013 PeerBlock, LLC

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

#include "mainproc.h"

// flag so that populating the list wont trigger any unnecessary notifications
// which may cause the checkboxes to be in an incorrect state
static bool finishedloading = false;

static TCHAR* GetPortTypeString(PortType type)
{
	TCHAR* ret;

	if (type == Destination)
		ret = _T("Outgoing");
	else if (type == Source)
		ret = _T("Incoming");
	else
		ret = _T("Both");

	return ret;
}

// Saves and merges ports
// hwnd - handle to the window
static void SavePorts(HWND hwnd)
{
	HWND list = GetDlgItem(hwnd, IDC_PORTS);
	for (vector<PortProfile>::size_type i = 0; i < g_config.PortSet.Profiles.size(); i++) {
		g_config.PortSet.Profiles[i].Enabled = ListView_GetCheckState(list, i) ? true : false;
	}

	g_config.PortSet.Merge();
	g_filter->setdestinationports(g_config.PortSet.DestinationPorts);
	g_filter->setsourceports(g_config.PortSet.SourcePorts);
}


static void EditPorts_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
		case IDC_HTTPPORT:
			SetBlockHttp((IsDlgButtonChecked(hwnd, IDC_HTTPPORT) != BST_CHECKED));
			break;
		case IDC_FTPPORT:
			g_config.PortSet.AllowFtp = (IsDlgButtonChecked(hwnd, IDC_FTPPORT) == BST_CHECKED);
			SavePorts(hwnd);
			break;
		case IDC_SMTPPORT:
			g_config.PortSet.AllowSmtp = (IsDlgButtonChecked(hwnd, IDC_SMTPPORT) == BST_CHECKED);
			SavePorts(hwnd);
			break;
		case IDC_POP3PORT:
			g_config.PortSet.AllowPop3 = (IsDlgButtonChecked(hwnd, IDC_POP3PORT) == BST_CHECKED);
			SavePorts(hwnd);
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

				lvi.iSubItem = 1;
				lvi.pszText = GetPortTypeString(p.Type);
				ListView_SetItem(list, &lvi);

				ListView_SetCheckState(list, idx, true);

				SavePorts(hwnd);
			}
		} break;
		case IDC_REMOVE:
			if (MessageBox(hwnd, _T("Are you sure you want to delete the selected profiles?"), _T("Delete profiles"), MB_YESNO) == IDYES) {
				HWND list = GetDlgItem(hwnd, IDC_PORTS);

				stack<int> items;
				for(int idx = ListView_GetNextItem(list, -1, LVNI_SELECTED); idx != -1; idx = ListView_GetNextItem(list, idx, LVNI_SELECTED))
					items.push(idx);

				while (!items.empty()) {
					int idx = items.top();
					items.pop();

					g_config.PortSet.Profiles.erase(g_config.PortSet.Profiles.begin() + idx);
					ListView_DeleteItem(list, idx);
				}

				SavePorts(hwnd);
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
					ListView_SetItemText(list, idx, 1, GetPortTypeString(p.Type));

					SavePorts(hwnd);
				}
			}
		} break;
	}
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

	DeferWindowPos(dwp, defgroup, NULL, 7, 5, cx-14, 120, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, ports, NULL, 7, 133, cx-14, cy-21-140-(rc.bottom-rc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, add, NULL, cx-((rc.right-rc.left+7)*3), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, edit, NULL, cx-((rc.right-rc.left+7)*2), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, remove, NULL, cx-(rc.right-rc.left+7), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	EndDeferWindowPos(dwp);
}


static BOOL EditPorts_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
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

	lvc.pszText = _T("Allow from");
	lvc.cx = 100;

	ListView_InsertColumn(list, 1, &lvc);

	int idx = 0;
	for (vector<PortProfile>::size_type i = 0; i < g_config.PortSet.Profiles.size(); i++) {
		PortProfile profile = g_config.PortSet.Profiles[i];

		LVITEM lvi = {0};
		lvi.mask = LVIF_TEXT;
		lvi.iItem = idx++;

		lvi.iSubItem = 0;
		lvi.pszText = (LPTSTR) profile.Name.c_str();

		ListView_InsertItem(list, &lvi);

		lvi.iSubItem = 1;
		lvi.pszText = GetPortTypeString(profile.Type);
		ListView_SetItem(list, &lvi);

		ListView_SetCheckState(list, i, profile.Enabled);
	}

	finishedloading = true;

	return TRUE;
}


static INT_PTR EditPorts_OnNotify(HWND hwnd, int idCtrl, NMHDR *nmh) {
	if (finishedloading && (nmh->idFrom == IDC_PORTS))
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

				SavePorts(hwnd);
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


INT_PTR CALLBACK EditPorts_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	try
	{
		switch(msg)
		{
			HANDLE_MSG(hwnd, WM_COMMAND, EditPorts_OnCommand);
			HANDLE_MSG(hwnd, WM_INITDIALOG, EditPorts_OnInitDialog);
			HANDLE_MSG(hwnd, WM_NOTIFY, EditPorts_OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, EditPorts_OnSize);
			case WM_PORTSETTINGSCHANGE:
				CheckDlgButton(hwnd, IDC_HTTPPORT, g_config.PortSet.AllowHttp);
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
