//================================================================================================
//  addlistproc.cpp
//
//  Implements the window that pops up when you click the Add button in the List Manager.
//================================================================================================

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


//////////---------------------------------------------------------------------
// Includes

#include "stdafx.h"
#include "resource.h"

#include "listurls.h"

using namespace std;


//////////---------------------------------------------------------------------
// Globals

HWND g_hAddListDlg = NULL;
ListUrls * g_pListUrls = NULL;


static void AddList_OnClose(HWND hwnd) {
	EndDialog(hwnd, IDCANCEL);
}

static void AddList_OnDestroy(HWND hwnd) {
	g_hAddListDlg = NULL;
	delete g_pListUrls;
}



//================================================================================================
//
//  AddList_SanityCheckUrl()
//
//    - Called by AddList_OnCommand(), when processing IDOK
//
/// <summary>
///   Returns a to-be-added URL, or empty string if none is to be.
/// </summary>
/// <param name="url">
///   The URL which the caller wants to sanity-check.
/// </param>
//
static tstring AddList_SanityCheckUrl(tstring _url) 
{
	tstring url = _url;
	int result = 0;

	LISTNAME listId = g_pListUrls->FindListNum(_url);	// get list-name num
	LISTFLAGS listFlags = g_pListUrls->CheckUrl(_url, listId, GetParent(g_hAddListDlg));	// sanity-check url


	// Handle sanity-checking results

	// User entered a "known wrong" URL, such as a list-description URL from iblocklist.com instead of a 
	// list-update URL
	if (listFlags.test(LISTFLAG_WRONG))
	{
		TRACEW("[addlistproc] [AddList_SanityCheckUrl]    LISTFLAG_WRONG");
		result = MessageBox(g_hAddListDlg, IDS_LISTSAN_WRONG, IDS_LISTSAN_TITLE, MB_ICONWARNING|MB_OK);
		TRACEI("[addlistproc] [AddList_SanityCheckUrl]    changing url to friendly-named url, and re-running sanity check");
		if (result == IDOK)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked OK");
			url = g_pListUrls->GetBestUrl(listId);
			url = AddList_SanityCheckUrl(url);
		}
	}

	// User entered a URL that's one of the Default Lists
	else if (listFlags.test(LISTFLAG_DEFAULT) && !listFlags.test(LISTFLAG_DIFFDUPE) && !listFlags.test(LISTFLAG_EXACTDUPE))
	{
		if (listFlags.test(LISTFLAG_NOT_IBL))
		{
			TRACEW("[addlistproc] [AddList_SanityCheckUrl]    LISTFLAG_DEFAULT & !DUPE & LISTFLAG_NOT_IBL");
			tstring text=boost::str(tformat(LoadString(IDS_LISTSAN_DEFAULT_NIBL))%g_pListUrls->GetListDesc(listId));
			result = MessageBox(g_hAddListDlg, text, IDS_LISTSAN_TITLE, MB_ICONWARNING|MB_YESNO);
			if (result == IDYES)
			{
				TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked Yes");
				url = g_pListUrls->GetBestUrl(listId);
			}
			else if (result == IDNO)
			{
				TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked No");
			}
		}
		else
		{
			TRACEW("[addlistproc] [AddList_SanityCheckUrl]    LISTFLAG_DEFAULT & !DUPE & !LISTFLAG_NOT_IBL");
			tstring text=boost::str(tformat(LoadString(IDS_LISTSAN_DEFAULT))%g_pListUrls->GetListDesc(listId));
			result = MessageBox(g_hAddListDlg, text, IDS_LISTSAN_TITLE, MB_ICONWARNING|MB_OK);
			if (result == IDOK)
			{
				TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked OK");
				url = g_pListUrls->GetBestUrl(listId);
			}
		}
	}

	// User entered a URL that's a Default List URL, but not one previously configured
	else if (listFlags.test(LISTFLAG_DIFFDUPE) && listFlags.test(LISTFLAG_DEFAULT))
	{
		TRACEW("[addlistproc] [AddList_SanityCheckUrl]    LISTFLAG_DEFAULT & LISTFLAG_DIFFDUPE");
		tstring text=boost::str(tformat(LoadString(IDS_LISTSAN_DEFDUPE))%g_pListUrls->GetListDesc(listId));
		result = MessageBox(g_hAddListDlg, text, IDS_LISTSAN_TITLE, MB_ICONWARNING|MB_YESNO);
		if (result == IDYES)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked Yes");
			url = g_pListUrls->GetBestUrl(listId);
		}
		else if (result == IDNO)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked No");
		}
	}

	// User entered a URL that's an *exact* match for a URL that's already been added
	else if (listFlags.test(LISTFLAG_EXACTDUPE))
	{
		TRACEW("[addlistproc] [AddList_SanityCheckUrl]    LISTFLAG_EXACTDUPE");
		result = MessageBox(g_hAddListDlg, IDS_LISTSAN_EXACTDUPE, IDS_LISTSAN_TITLE, MB_ICONWARNING|MB_OK);
		if (result == IDOK)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked OK");
		}
	}

	// User entered a URL that's for a list that's already been added, but with a different URL.
	// For example, adding http://list.iblocklist.com/?list=bt_level2 when we already have
	// http://list.iblocklist.com/lists/bluetack/level-2 added.
	else if (listFlags.test(LISTFLAG_DIFFDUPE))
	{
		TRACEW("[addlistproc] [AddList_SanityCheckUrl]    LISTFLAG_DIFFDUPE");
		tstring text=boost::str(tformat(LoadString(IDS_LISTSAN_DIFFDUPE))%g_pListUrls->GetListDesc(listId));
		result = MessageBox(g_hAddListDlg, text, IDS_LISTSAN_TITLE, MB_ICONWARNING|MB_YESNO);
		if (result == IDYES)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked Yes");
			url = tstring(_T(""));
			//url = g_pListUrls->GetBestUrl(listId);
		}
		else if (result == IDNO)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked No");
		}
	}

	// User entered an "unfriendly URL", for instance one of the ones iblocklist.com 
	// publicly displays such as http://list.iblocklist.com/lists/bluetack/level-2
	else if (listFlags.test(LISTFLAG_UNFRIENDLY))
	{
		TRACEW("[addlistproc] [AddList_SanityCheckUrl]    LISTFLAG_UNFRIENDLY");
		result = MessageBox(g_hAddListDlg, IDS_LISTSAN_UNFRIENDLY, IDS_LISTSAN_TITLE, MB_ICONWARNING|MB_YESNO);
		if (result == IDYES)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked Yes");
			url = g_pListUrls->GetBestUrl(listId);
		}
		else if (result == IDNO)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked No");
		}
	}

	// User entered a non-iblocklist.com URL even though one exists, such as the 
	// bluetack.co.uk-hosted version of Level2
	else if (listFlags.test(LISTFLAG_NOT_IBL))
	{
		TRACEW("[addlistproc] [AddList_SanityCheckUrl]    LISTFLAG_NOT_IBL");
		result = MessageBox(g_hAddListDlg, IDS_LISTSAN_NOTIBL, IDS_LISTSAN_TITLE, MB_ICONWARNING|MB_YESNO);
		if (result == IDYES)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked Yes");
			url = g_pListUrls->GetBestUrl(listId);
		}
		else if (result == IDNO)
		{
			TRACEI("[addlistproc] [AddList_SanityCheckUrl]    user clicked No");
		}
	}

	return url;

} // End of AddList_SanityCheckUrl()



static void AddList_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
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

		case IDOK: 
		{
			List **list=(List**)(LONG_PTR)GetWindowLongPtr(hwnd, DWLP_USER);
			bool addedList = false;

			if(IsDlgButtonChecked(hwnd, IDC_ADDFILE)==BST_CHECKED) 
			{
				// File-based list
				const path file=GetDlgItemText(hwnd, IDC_FILE);

				if(!path::exists(file.has_root()?file:path::base_dir()/file)) 
				{
					MessageBox(hwnd, IDS_INVALIDFILETEXT, IDS_INVALIDFILE, MB_ICONERROR|MB_OK);
					break;
				}

				StaticList *l=new StaticList;
				l->File=file;
				
				*list=l;
				addedList = true;
			}
			else 
			{
				TRACEI("[addlistproc] [AddList_OnCommand]    sanity-checking URL-based list");

				// URL-based list
				tstring url=GetDlgItemText(hwnd, IDC_URL);

				if(!(path(url).is_url())) 
				{
					TRACEE("[addlistproc] [AddList_OnCommand]  * ERROR:  Not a valid URL!");
					MessageBox(hwnd, IDS_INVALIDURLTEXT, IDS_INVALIDURL, MB_ICONERROR|MB_OK);
					break;
				}

				// sanity-check
				url = AddList_SanityCheckUrl(url);

				if (!url.empty())
				{
					// Finally, add this new list!
					TRACEI("[addlistproc] [AddList_OnCommand]    adding new list");
					DynamicList *l=new DynamicList;
					l->Url=url;

					*list=l;
					addedList = true;
				}
				else
				{
					TRACEI("[addlistproc] [AddList_OnCommand]    not adding new list");
					DynamicList *l=new DynamicList;
					l->Url=url;

					*list=l;
				}
			}

			if (addedList)
			{
				TRACEI("[addlistproc] [AddList_OnCommand]    finishing list-add");
				(*list)->Type=(IsDlgButtonChecked(hwnd, IDC_BLOCK)==BST_CHECKED)?List::Block:List::Allow;
				(*list)->Description=GetDlgItemText(hwnd, IDC_DESCRIPTION);
			}
			EndDialog(hwnd, IDOK);

		} break; // End of IDOK


		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
	}

} // End of AddList_OnCommand()



static BOOL AddList_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) 
{
	g_hAddListDlg = hwnd;
	g_pListUrls = new ListUrls();
	g_pListUrls->Init();

#pragma warning(disable:4244) //not my fault!
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
#pragma warning(default:4244)

	HWND url=GetDlgItem(hwnd, IDC_URL);

	for(size_t i=0; i<g_presetcount; i++)
		ComboBox_AddString(url, g_presets[i]);
	ComboBox_SetCurSel(url, 2);

	CheckDlgButton(hwnd, IDC_ADDURL, BST_CHECKED);
	CheckDlgButton(hwnd, IDC_BLOCK, BST_CHECKED);
	EnableWindow(GetDlgItem(hwnd, IDC_FILE), FALSE);
	EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);
	EnableWindow(GetDlgItem(hwnd, IDC_URL), TRUE);
	EnableWindow(GetDlgItem(hwnd, IDOK), GetWindowTextLength(GetDlgItem(hwnd, IDC_URL))>0);

	// Load peerblock icon in the windows titlebar
	RefreshDialogIcon(g_hAddListDlg);

	return TRUE;
}

INT_PTR CALLBACK AddList_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_CLOSE, AddList_OnClose);
			HANDLE_MSG(hwnd, WM_COMMAND, AddList_OnCommand);
			HANDLE_MSG(hwnd, WM_INITDIALOG, AddList_OnInitDialog);
			HANDLE_MSG(hwnd, WM_DESTROY, AddList_OnDestroy);
			case WM_DIALOG_ICON_REFRESH:
				RefreshDialogIcon(g_hAddListDlg);
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
