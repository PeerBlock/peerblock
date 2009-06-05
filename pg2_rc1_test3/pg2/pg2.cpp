/*
	Copyright (C) 2004-2005 Cory Nelson

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
	
	CVS Info :
		$Author: phrostbyte $
		$Date: 2005/07/07 19:23:11 $
		$Revision: 1.27 $
*/

#include "stdafx.h"
#include "resource.h"
#include "updatelists.h"
using namespace std;

#if defined(_WIN32_WINNT) && _WIN32_WINNT>=0x0500
static const LPCTSTR g_mutex_name=_T("Global\\PeerGuardian2");
#else
static const LPCTSTR g_mutex_name=_T("PeerGuardian2");
#endif

// blocks ips without updating for vista
//void BlockWithoutUpdating(HWND hwnd);

static bool CheckOS() {
	OSVERSIONINFO osv = {0};
	osv.dwOSVersionInfoSize = sizeof(osv);

	if(!GetVersionEx(&osv)) return false;

	//TODO: check version.

	return true;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int nCmdShow) {
	if(!CheckOS()) {
		return -1;
	}

#ifdef _WIN32_WINNT
	if(!IsUserAnAdmin()) {
		MessageBox(NULL, IDS_NEEDADMINTEXT, IDS_NEEDADMIN, MB_ICONERROR|MB_OK);
		return -1;
	}
#endif

	HANDLE mutex=OpenMutex(MUTEX_ALL_ACCESS, FALSE, g_mutex_name);
	if(mutex) {
		UINT msg=RegisterWindowMessage(_T("PeerGuardian2SetVisible"));
		if(msg) SendMessage(HWND_BROADCAST, msg, 0, TRUE);
		return 0;
	}
	else mutex=CreateMutex(NULL, FALSE, g_mutex_name);

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	{
		INITCOMMONCONTROLSEX icx={0};
		icx.dwSize=sizeof(icx);
		icx.dwICC=ICC_DATE_CLASSES|ICC_INTERNET_CLASSES|ICC_LISTVIEW_CLASSES|ICC_PROGRESS_CLASS|ICC_TAB_CLASSES|ICC_UPDOWN_CLASS|ICC_USEREX_CLASSES|ICC_WIN95_CLASSES;

		InitCommonControlsEx(&icx);
	}

	RegisterColorPicker(hInstance);

	{
		WSADATA data;
		WSAStartup(WINSOCK_VERSION, &data);
	}

	try {
		HWND hwnd=CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAIN), NULL, Main_DlgProc);

		MSG msg;
		while(GetMessage(&msg, NULL, 0, 0)>0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	catch(exception &ex) {
		UncaughtExceptionBox(NULL, ex, __FILE__, __LINE__);
	}
	catch(...) {
		UncaughtExceptionBox(NULL, __FILE__, __LINE__);
	}

	if(g_filter) g_filter.reset();

	WSACleanup();

	return 0;
}


//void BlockWithoutUpdating(HWND hwnd)
//{
	//pgfilter *f = g_filter.get();
	//f->m_filter.load(L"pgfilter");
	//f->setblock(g_config.Block);
	//f->setblockhttp(g_config.BlockHttp);

	//UINT msg=RegisterWindowMessage(_T("PeerGuardian2LoadLists"));
	//SendMessage(HWND_BROADCAST, WM_PG2_LOADLISTS, 0, 0);

	
		

		

		
	


//}
