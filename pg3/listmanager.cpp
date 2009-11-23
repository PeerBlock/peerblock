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
#include "helpers.hpp"
#include "procs.hpp"
#include "config.hpp"
#include "resource.h"

void ShowListManagerDialog(HWND parent, configs &cfgs) {
	HINSTANCE hinst = GetModuleHandle(NULL);

	boost::shared_ptr<DLGTEMPLATE> lists = MakeSysFontDialog(hinst, MAKEINTRESOURCE(IDD_LISTS));
	boost::shared_ptr<DLGTEMPLATE> quicklist = MakeSysFontDialog(hinst, MAKEINTRESOURCE(IDD_QUICKLIST));
	boost::shared_ptr<DLGTEMPLATE> applist = MakeSysFontDialog(hinst, MAKEINTRESOURCE(IDD_APPLIST));

	PROPSHEETPAGE psp[3] = {0};

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE | PSP_DLGINDIRECT;
	psp[0].hInstance = GetModuleHandle(NULL);
	psp[0].pResource = lists.get();
	psp[0].pszTitle = MAKEINTRESOURCE(IDS_LISTS);
	psp[0].pfnDlgProc = Lists_DlgProc;
	psp[0].lParam = (LPARAM)&cfgs;

	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_USETITLE | PSP_DLGINDIRECT;
	psp[1].hInstance = psp[0].hInstance;
	psp[1].pResource = quicklist.get();
	psp[1].pszTitle = MAKEINTRESOURCE(IDS_QUICKLIST);
	psp[1].pfnDlgProc = QuickList_DlgProc;
	psp[1].lParam = (LPARAM)&cfgs;

#if _WIN32_WINNT >= 0x0600
	psp[2].dwSize = sizeof(PROPSHEETPAGE);
	psp[2].dwFlags = PSP_USETITLE | PSP_DLGINDIRECT;
	psp[2].hInstance = psp[0].hInstance;
	psp[2].pResource = applist.get();
	psp[2].pszTitle = MAKEINTRESOURCE(IDS_APPLICATIONS);
	psp[2].pfnDlgProc = AppList_DlgProc;
	psp[2].lParam = (LPARAM)&cfgs;
#endif

	PROPSHEETHEADER psh = {0};
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP;
	psh.hwndParent = parent;
	psh.pszCaption = MAKEINTRESOURCE(IDS_LISTMANAGER);
	psh.ppsp = psp;

#if _WIN32_WINNT >= 0x0600
	psh.nPages = 3;
#else
	// no support for application-wide allow on non-Vista.
	psh.nPages = 2;
#endif

	PropertySheet(&psh);
}
