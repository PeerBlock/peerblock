/*
	Copyright (C) 2013 PeerBlock, LLC

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
#include "tracelog.h"
#include "listupdateerrorwin.h"
using namespace std;

extern TraceLog g_tlog;

// globals
HWND g_hListUpdateErrorWinDlg = NULL;
set<int> g_errors;
mutex g_errorsLock;
mutex g_lifecycleLock;
int g_listUpdateError;

// forward declarations
INT_PTR CALLBACK ListUpdateErrorWin_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


void ListUpdateErrorWin(HWND hwnd, const std::set<int>& errors)
{
    TRACEV("[ListUpdateErrorWin] [ListUpdateErrorWin]  > Entering routine.");

    {
        // only one of these windows should be around at a time
        mutex::scoped_lock lock(g_lifecycleLock);
        if (g_hListUpdateErrorWinDlg) {
            TRACEW("[ListUpdateErrorWin] [ListUpdateErrorWin]    attempted to create list-update error-window while it was already present");
            // TODO:  can we raise this window to the foreground?
            return;
        }
    }

    {
        // set up the internal errors collection
        mutex::scoped_lock lock(g_errorsLock);
        g_errors = errors;
    }

    // now pop up the window
    if(DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LISTUPDATEERRORWIN), hwnd, ListUpdateErrorWin_DlgProc, 0)==IDOK)
    {
        TRACEV("[ListUpdateErrorWin] [ListUpdateErrorWin]    displayed error dialog");
    }

    // done!
    TRACEV("[ListUpdateErrorWin] [ListUpdateErrorWin]  < Leaving routine.");

} // End of ListUpdateErrorWin()


//================================================================================================
//
//  HideAllControls()
//
//    - Called by ListUpdateErrorWin_OnInitDialog()
//
/// <summary>
///   Hides all controls on this window, except for the OK button and the generic error text.  The dialog box instance
///   is expected to call this method, and then enable only the controls that are valid for this particular error.
/// </summary>
//
void HideAllControls()
{
    ShowWindow(GetDlgItem(g_hListUpdateErrorWinDlg, IDC_ATEXT), SW_HIDE);
    ShowWindow(GetDlgItem(g_hListUpdateErrorWinDlg, IDC_UPDATEPB7DAYS), SW_HIDE);
    ShowWindow(GetDlgItem(g_hListUpdateErrorWinDlg, IDC_IGNOREERR), SW_HIDE);
    ShowWindow(GetDlgItem(g_hListUpdateErrorWinDlg, IDC_MOREINFO), SW_HIDE);
    ShowWindow(GetDlgItem(g_hListUpdateErrorWinDlg, IDC_IBLSUBSCRIBE), SW_HIDE);
    ShowWindow(GetDlgItem(g_hListUpdateErrorWinDlg, IDC_IBLRENEW), SW_HIDE);

} // End of HideAllControls()


//================================================================================================
//
//  ShowControl()
//
//    - Called by ListUpdateErrorWin_OnInitDialog()
//
/// <summary>
///   Helper-method to make just a single control visible.
/// </summary>
//
void ShowControl(int _control)
{
    ShowWindow(GetDlgItem(g_hListUpdateErrorWinDlg, _control), SW_SHOW);

} // End of ShowControl()


//================================================================================================
//
//  GetMoreInfoUrl()
//
//    - Called by ListUpdateErrorWin_OnCommand()
//
/// <summary>
///   Returns a URL which can be visited to view additional information about this particular 
///   list-update error.
/// </summary>
//
tstring GetMoreInfoUrl(int _code)
{
    tstring ret;

    switch(_code) {

        case 401: {
            ret = _T("http://www.peerblock.com/list-update-error-info/401?src=pbw");
        } break;

        case 402: {
            ret = _T("http://www.peerblock.com/list-update-error-info/402?src=pbw");
        } break;

        case 404: {
            ret = _T("http://www.peerblock.com/list-update-error-info/404?src=pbw");
        } break;

        case 419: {
            ret = _T("http://www.peerblock.com/list-update-error-info/419?src=pbw");
        } break;

        case 420: {
            ret = _T("http://www.peerblock.com/list-update-error-info/420?src=pbw");
        } break;

        case 426: {
            ret = _T("http://www.peerblock.com/list-update-error-info/426?src=pbw");
        } break;

        case 429: {
            ret = _T("http://www.peerblock.com/list-update-error-info/429?src=pbw");
        } break;

        default: {
            ret = _T("http://www.peerblock.com/list-update-error-info/unknown?src=pbw");
        } break;
    }

    return ret;

} // End of GetMoreInfoUrlFromErrorCode()


//----------
// Dialog Box

static void ListUpdateErrorWin_OnClose(HWND hwnd) {
	EndDialog(hwnd, IDCANCEL);
}

static void ListUpdateErrorWin_OnDestroy(HWND hwnd) {
    mutex::scoped_lock lock(g_lifecycleLock);
    g_hListUpdateErrorWinDlg = NULL;
}

static void ListUpdateErrorWin_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {

		case IDC_MOREINFO: {
            tstring strUrl = GetMoreInfoUrl(g_listUpdateError);
            ShellExecute(NULL, NULL, strUrl.c_str(), NULL, NULL, SW_SHOW);
		} break;

		case IDC_IBLSUBSCRIBE: {
            ShellExecute(NULL, NULL, _T("https://www.iblocklist.com/peerblock_subscribe.php?src=pbw"), NULL, NULL, SW_SHOW);
		} break;

		case IDC_IBLRENEW: {
            tstring strUrl(_T("https://www.iblocklist.com/peerblock_renew?src=pbw"));
            if (!g_config.IblUsername.empty()) {
                // append username, so that iblocklist.com can prepopulate this field
                strUrl += (tformat(_T("&username=%1%")) % g_config.IblUsername).str();
            }
            ShellExecute(NULL, NULL, strUrl.c_str(), NULL, NULL, SW_SHOW);
		} break;

		case IDOK: {
            if (g_listUpdateError) {
                if (IsDlgButtonChecked(hwnd, IDC_IGNOREERR)==BST_CHECKED) {
                    // update config to not display this window for 429 errors in the future
                    g_config.IgnoreListUpdateLimit = true;
                    g_config.Save();
                }
                if (IsDlgButtonChecked(hwnd, IDC_UPDATEPB7DAYS)==BST_CHECKED) {
                    // update config to only check for updates once per 7 days
                    g_config.UpdateInterval = 7;
                    g_config.Save();
                	SendMessage(g_tabs[2].Tab, WM_REFRESH_AUTOUPDATE, 0, 0);
                }
            }
			EndDialog(hwnd, IDOK);
		} break;

	}
}

static BOOL ListUpdateErrorWin_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {

    {
        mutex::scoped_lock lock(g_lifecycleLock);
        g_hListUpdateErrorWinDlg = hwnd;
    }

    // make local copy of error list
    set<int> errors;
    {
        mutex::scoped_lock lock(g_errorsLock);
        errors = g_errors;
    }

    // make all controls invisible - we'll toggle their visibility later, based on the error we're displaying
    HideAllControls();

    // check for each error-code, in priority order
    if (errors.find(426) != errors.end()) {

        // user is using a no-longer-supported version of PeerBlock
        g_listUpdateError = 426;

        // set error text
        SetDlgItemText(hwnd, IDC_STATIC, 
            LoadString(IDS_LISTERR_426LONG).c_str()
            );

        // show all necessary controls
        ShowControl(IDC_MOREINFO);

    } else if (errors.find(420) != errors.end()) {

        // user has tried to auth too many times without success
        g_listUpdateError = 420;

        // set error text
        SetDlgItemText(hwnd, IDC_STATIC, 
            LoadString(IDS_LISTERR_420LONG).c_str()
            );

        // show all necessary controls
        ShowControl(IDC_MOREINFO);

    } else if (errors.find(401) != errors.end()) {

        // user entered incorrect username/pin
        g_listUpdateError = 401;

        // set error text
        SetDlgItemText(hwnd, IDC_STATIC, 
            LoadString(IDS_LISTERR_401LONG).c_str()
            );

        // show all necessary controls
        ShowControl(IDC_MOREINFO);

    } else if (errors.find(419) != errors.end()) {

        // subscription expired, need to renew
        g_listUpdateError = 419;

        // set error text
        SetDlgItemText(hwnd, IDC_STATIC, 
            LoadString(IDS_LISTERR_419LONG).c_str()
            );

        // show all necessary controls
        ShowControl(IDC_MOREINFO);
        ShowControl(IDC_IBLRENEW);

    } else if (errors.find(402) != errors.end()) {

        // this is a subscription-only list, and the user is not a subscriber
        g_listUpdateError = 402;

        // set error text
        SetDlgItemText(hwnd, IDC_STATIC, 
            LoadString(IDS_LISTERR_402LONG).c_str()
            );

        // show all necessary controls
        ShowControl(IDC_MOREINFO);
        ShowControl(IDC_IBLSUBSCRIBE);

    } else if (errors.find(429) != errors.end() && !g_config.IgnoreListUpdateLimit) {

        // list-update limit reached
        g_listUpdateError = 429;

        // set error text
        SetDlgItemText(hwnd, IDC_STATIC, 
            LoadString(IDS_LISTERR_429LONG).c_str()
            );

        if (g_config.UpdateInterval == 7) {
            // mark checkbox as checked
            CheckDlgButton(hwnd, IDC_UPDATEPB7DAYS, BST_CHECKED);
        }

        // show all necessary controls
        ShowControl(IDC_UPDATEPB7DAYS);
        ShowControl(IDC_IGNOREERR);
        ShowControl(IDC_MOREINFO);
        ShowControl(IDC_IBLSUBSCRIBE);

    } else if (errors.find(404) != errors.end()) {

        // this list-url doesn't exist
        g_listUpdateError = 404;

        // set error text
        SetDlgItemText(hwnd, IDC_STATIC, 
            LoadString(IDS_LISTERR_404LONG).c_str()
            );

        // show all necessary controls
        ShowControl(IDC_MOREINFO);

    } else {

        // can't find any expected errors, so just display something generic
        g_listUpdateError = 0;

        // set error text
        SetDlgItemText(hwnd, IDC_STATIC, 
            LoadString(IDS_LISTERR_UNKNOWNLONG).c_str()
            );

        // show all necessary controls
        ShowControl(IDC_MOREINFO);

        if (errors.find(429) != errors.end() && errors.size() == 1) {
            // if we get here but have only a 429 error in our list, the user wants to ignore it
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
    }

	// Load peerblock icon in the dialogs titlebar
	RefreshDialogIcon(g_hListUpdateErrorWinDlg);

	return TRUE;
}

INT_PTR CALLBACK ListUpdateErrorWin_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_COMMAND, ListUpdateErrorWin_OnCommand);
			HANDLE_MSG(hwnd, WM_INITDIALOG, ListUpdateErrorWin_OnInitDialog);
			HANDLE_MSG(hwnd, WM_CLOSE, ListUpdateErrorWin_OnClose);
			HANDLE_MSG(hwnd, WM_DESTROY, ListUpdateErrorWin_OnDestroy);
			case WM_DIALOG_ICON_REFRESH:
				RefreshDialogIcon(g_hListUpdateErrorWinDlg);
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
