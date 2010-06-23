/*
	Copyright (C) 2004-2005 Cory Nelson
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

static INT_PTR g_ret;
HWND g_hEditListDlg = NULL;

static void EditList_OnClose(HWND hwnd) {
	EndDialog(hwnd, IDCANCEL);
}

static void EditList_OnDestroy(HWND hwnd) {
	g_hEditListDlg = NULL;
}

static void EditList_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDC_FILE:
		case IDC_URL:
			if(codeNotify==EN_UPDATE || codeNotify==CBN_EDITUPDATE || codeNotify==CBN_SELCHANGE)
				EnableWindow(GetDlgItem(hwnd, IDOK), GetWindowTextLength(hwndCtl)>0);
			break;
		case IDC_ADDFILE:
			EnableWindow(GetDlgItem(hwnd, IDC_URL), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_FILE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDOK), GetWindowTextLength(GetDlgItem(hwnd, IDC_FILE))>0);
			break;
		case IDC_ADDURL:
			EnableWindow(GetDlgItem(hwnd, IDC_FILE), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_URL), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDOK), GetWindowTextLength(GetDlgItem(hwnd, IDC_URL))>0);
			break;
		case IDC_BROWSE: {
			TCHAR file[MAX_PATH]={0};

			OPENFILENAME ofn={0};
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner=hwnd;
			ofn.lpstrFilter=_T("PeerBlock Lists (*.p2p; *.p2b; *.txt)\0*.p2p;*.p2p;*.p2b;*.txt\0")_T("All Files (*.*)\0*.*\0");
			ofn.lpstrFile=file;
			ofn.nMaxFile=MAX_PATH;
			ofn.Flags=OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;

			if(GetOpenFileName(&ofn)) {
				path p=path::relative_to(path::base_dir(), file);
				SetDlgItemText(hwnd, IDC_FILE, p.c_str());
			}
		} break;
		case IDOK: {
			List **list=(List**)(LONG_PTR)GetWindowLongPtr(hwnd, DWLP_USER);

			if(IsDlgButtonChecked(hwnd, IDC_ADDFILE)==BST_CHECKED) {
				const path file=GetDlgItemText(hwnd, IDC_FILE);

				if(!path::exists(file.has_root()?file:path::base_dir()/file)) {
					MessageBox(hwnd, IDS_INVALIDFILETEXT, IDS_INVALIDFILE, MB_ICONERROR|MB_OK);
					break;
				}

				StaticList *l;

				if(l=dynamic_cast<StaticList*>(*list))
					if(l->File!=file) {
						l->File=file;
						g_ret|=LISTS_NEEDRELOAD;
					}
				else {
					delete *list;
					*list=l=new StaticList;
					l->File=file;

					g_ret|=LISTS_NEEDRELOAD;
				}
			}
			else {
				tstring url=GetDlgItemText(hwnd, IDC_URL);

				if(!(path(url).is_url())) {
					MessageBox(hwnd, IDS_INVALIDURLTEXT, IDS_INVALIDURL, MB_ICONERROR|MB_OK);
					break;
				}

				DynamicList *l;

				if(l=dynamic_cast<DynamicList*>(*list)) {
					if(l->Url!=url) {
						l->Url=url;
						g_ret|=LISTS_NEEDRELOAD;
					}
				}
				else {
					delete *list;
					*list=l=new DynamicList;
					l->Url=url;

					g_ret|=LISTS_NEEDRELOAD;
				}

			}

			List::ListType type=(IsDlgButtonChecked(hwnd, IDC_BLOCK)==BST_CHECKED)?List::Block:List::Allow;

			if((*list)->Type!=type) {
				(*list)->Type=type;
				g_ret|=LISTS_NEEDRELOAD;
			}

			(*list)->Description=GetDlgItemText(hwnd, IDC_DESCRIPTION);

			EndDialog(hwnd, g_ret);
		} break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
	}
}

static BOOL EditList_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	g_hEditListDlg = hwnd;

	List **list=(List**)lParam;

#pragma warning(disable:4244)
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)list);
#pragma warning(default:4244)

	HWND url=GetDlgItem(hwnd, IDC_URL);

	for(size_t i=0; i<g_presetcount; i++)
		ComboBox_AddString(url, g_presets[i]);
	ComboBox_SetCurSel(url, 3);

	CheckDlgButton(hwnd, ((*list)->Type==List::Allow)?IDC_ALLOW:IDC_BLOCK, BST_CHECKED);
	SetDlgItemText(hwnd, IDC_DESCRIPTION, (*list)->Description.c_str());

	if(StaticList *l=dynamic_cast<StaticList*>(*list)) {
		CheckDlgButton(hwnd, IDC_ADDFILE, BST_CHECKED);

		EnableWindow(GetDlgItem(hwnd, IDC_URL), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_FILE), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);

		SetDlgItemText(hwnd, IDC_FILE, l->File.c_str());
	}
	else if(DynamicList *l=dynamic_cast<DynamicList*>(*list)) {
		CheckDlgButton(hwnd, IDC_ADDURL, BST_CHECKED);

		EnableWindow(GetDlgItem(hwnd, IDC_FILE), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_URL), TRUE);

		SetDlgItemText(hwnd, IDC_URL, l->Url.c_str());
	}

	g_ret=0;

	// Load peerblock icon in the windows titlebar
	RefreshDialogIcon(hwnd);

	return TRUE;
}

INT_PTR CALLBACK EditList_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_COMMAND, EditList_OnCommand);
			HANDLE_MSG(hwnd, WM_INITDIALOG, EditList_OnInitDialog);
			HANDLE_MSG(hwnd, WM_CLOSE, EditList_OnClose);
			HANDLE_MSG(hwnd, WM_DESTROY, EditList_OnDestroy);
			case WM_DIALOG_ICON_REFRESH:
				RefreshDialogIcon(hwnd);
				return 1;
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
