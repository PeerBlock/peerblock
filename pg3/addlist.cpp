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
#include "path.hpp"
#include "helpers.hpp"
#include "procs.hpp"
#include "exception.hpp"
#include "resource.h"

static void AddList_OnClose(HWND hwnd) {
	EndDialog(hwnd, 0);
}

static void append_str(std::vector<wchar_t> &vec, const wchar_t *str) {
	std::copy(str, wcschr(str, '\0') + 1, std::back_inserter(vec));
}

static void AddList_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDC_BROWSE: {
			wchar_t file[512] = {0};
			GetDlgItemText(hwnd, IDC_LOCATION, file, 512);

			if(file[0] && (path::is_url(file) || !path::exists(file))) {
				file[0] = L'\0';
			}

			std::vector<wchar_t> filter;
			{
				append_str(filter, LoadString(IDS_OFN_ALLTYPES).c_str());
				append_str(filter, L"*.txt;*.p2p;*.dat;*.p2b");
				append_str(filter, LoadString(IDS_OFN_TEXTTYPES).c_str());
				append_str(filter, L"*.txt;*.p2p");
				append_str(filter, LoadString(IDS_OFN_EMULETYPE).c_str());
				append_str(filter, L"*.dat");
				append_str(filter, LoadString(IDS_OFN_BINARYTYPE).c_str());
				append_str(filter, L"*.p2b");
				append_str(filter, LoadString(IDS_OFN_ALLFILES).c_str());
				append_str(filter, L"*.*");

				filter.push_back(L'\0');
			}

			OPENFILENAME ofn = {0};
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFilter = &filter[0];
			ofn.lpstrFile = file;
			ofn.nMaxFile = 512;
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

			if(GetOpenFileName(&ofn)) {
				SetDlgItemText(hwnd, IDC_LOCATION, file);
			}
			else {
				DWORD err = CommDlgExtendedError();
				if(err) throw commdlg_error("GetOpenFileName", err);
			}
		} break;
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
	}
}

static void AddList_OnDestroy(HWND hwnd) {

}

static BOOL AddList_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	int taborder[] = { IDC_DESCRIPTION, IDC_LOCATION, IDC_BROWSE, IDC_PROXY_DEFAULT, IDC_PROXY_HTTP, IDC_PROXY_SOCKS4, IDC_PROXY_SOCKS5, IDC_PROXY, IDC_ALLOW, IDC_BLOCK, IDOK, IDCANCEL };
	SetTabOrder(hwnd, taborder);

	SendDlgItemMessage(hwnd, IDC_PROXY_DEFAULT, BM_SETCHECK, BST_CHECKED, 0);
	SendDlgItemMessage(hwnd, IDC_ALLOW, BM_SETCHECK, BST_CHECKED, 0);

	return TRUE;
}

static INT_PTR CALLBACK AddList_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
try {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CLOSE, AddList_OnClose);
		HANDLE_MSG(hwnd, WM_COMMAND, AddList_OnCommand);
		HANDLE_MSG(hwnd, WM_DESTROY, AddList_OnDestroy);
		HANDLE_MSG(hwnd, WM_INITDIALOG, AddList_OnInitDialog);
		default: return FALSE;
	}
}
catch(std::exception &ex) {
	ReportException(hwnd, ex, __WFILE__, __LINE__);
	return 0;
}
}

void ShowAddListDialog(HWND parent) {
	SysFontDialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDLIST), parent, AddList_DlgProc);
}
