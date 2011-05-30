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

HWND g_hCreateListDlg = NULL;

static void CreateList_OnClose(HWND hwnd) {
	EndDialog(hwnd, IDCANCEL);
}

static void CreateList_OnDestroy(HWND hwnd) {
	g_hCreateListDlg = NULL;
}

static void CreateList_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDC_FILE:
			if(codeNotify==EN_UPDATE)
				EnableWindow(GetDlgItem(hwnd, IDOK), GetWindowTextLength(hwndCtl)>0);
			break;
		case IDC_BROWSE: {
			TCHAR file[MAX_PATH]={0};

			OPENFILENAME ofn={0};
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner=hwnd;
			ofn.lpstrFilter=_T("PeerBlock Lists (*.p2p)\0*.p2p\0");
			ofn.lpstrFile=file;
			ofn.nMaxFile=MAX_PATH;
			ofn.Flags=OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;

			if(GetSaveFileName(&ofn)) {
				path p=path::relative_to(path::base_dir(), file);
				SetDlgItemText(hwnd, IDC_FILE, p.c_str());
			}
		} break;
		case IDOK: {
			StaticList **list=(StaticList**)(LONG_PTR)GetWindowLongPtr(hwnd, DWLP_USER);

			*list=new StaticList;

			(*list)->File=GetDlgItemText(hwnd, IDC_FILE);
			(*list)->Type=(IsDlgButtonChecked(hwnd, IDC_BLOCK)==BST_CHECKED)?List::Block:List::Allow;
			(*list)->Description=GetDlgItemText(hwnd, IDC_DESCRIPTION);

			EndDialog(hwnd, IDOK);
		} break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
	}
}

static BOOL CreateList_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	g_hCreateListDlg = hwnd;

#pragma warning(disable:4244) //not my fault!
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
#pragma warning(default:4244)

	CheckDlgButton(hwnd, IDC_BLOCK, BST_CHECKED);

	// Load peerblock icon in the dialogs titlebar
	RefreshDialogIcon(g_hCreateListDlg);

	return TRUE;
}

INT_PTR CALLBACK CreateList_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_COMMAND, CreateList_OnCommand);
		HANDLE_MSG(hwnd, WM_INITDIALOG, CreateList_OnInitDialog);
		HANDLE_MSG(hwnd, WM_CLOSE, CreateList_OnClose);
		HANDLE_MSG(hwnd, WM_DESTROY, CreateList_OnDestroy);
		case WM_DIALOG_ICON_REFRESH:
			RefreshDialogIcon(g_hCreateListDlg);
			return 1;
		default: return 0;
	}
}
