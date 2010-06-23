/*
	Copyright (C) 2010 PeerBlock, LLC

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

#include "resource.h"

// Message sent al all dialogs when the system tray status icon changes states
#define WM_DIALOG_ICON_REFRESH (WM_APP + 3)



//================================================================================================
//
//  RefreshDialogIcon(HWND hDialog)
//
//    - Called by each dialog when the status bar icon changes status (colors)
//
/// <summary>
///   Sends a message to the dialog to refresh the titlebar icon, based on the 
///   state of the status bar icon
/// </summary>
//
static void RefreshDialogIcon(HWND hDialog)
{
	// Load peerblock icon in the dialog titlebar
	HICON hIcon = NULL;

	if (g_config.Block == false)
		hIcon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_DISABLED));
	else if(g_config.Block == true && g_config.PortSet.IsHttpBlocked() == false)
		hIcon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_HTTPDISABLED));
	else
		hIcon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MAIN));

	if (hIcon != NULL)
	{
		::SendMessage(hDialog, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		::SendMessage(hDialog, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	}

} // End of RefreshDialogIcon()



//================================================================================================
//
//  GetDlgItemText()
//
//    - Called by a dialog when it needs to retrieve text from a control
//
/// <summary>
///   Returns the text (if any) from a control in a dialog.
/// </summary>
//
static tstring GetDlgItemText(HWND hDlg, int nIDDlgItem) 
{
	HWND ctrl=GetDlgItem(hDlg, nIDDlgItem);

	if(ctrl != NULL)
	{
		int len=GetWindowTextLength(ctrl)+1;
		boost::scoped_array<TCHAR> buf(new TCHAR[len]);

		return tstring(buf.get(), GetWindowText(ctrl, buf.get(), len));
	}

	return tstring(_T(""));

} // End of GetDlgItemText()
