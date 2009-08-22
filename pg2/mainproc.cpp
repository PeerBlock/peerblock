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
		$Date: 2005/07/12 02:20:35 $
		$Revision: 1.50 $
*/

#include "stdafx.h"
#include "resource.h"
#include "tracelog.h"
using namespace std;
using namespace sqlite3x;

static UINT WM_TRAY_CREATED;
static UINT WM_PG2_VISIBLE;
static UINT WM_PG2_LOADLISTS;
static const UINT WM_PG2_TRAY=WM_APP+2;
static const UINT TRAY_ID=1;
static const UINT TIMER_BLINKTRAY=1;
static const UINT TIMER_PROCESSDB=2;

bool g_trayactive;
NOTIFYICONDATA g_nid={0};
static bool g_trayblink=false;
DWORD g_blinkstart=0;

extern TraceLog g_tlog;

HWND g_main;
boost::shared_ptr<pgfilter> g_filter;

TabData g_tabs[]={
	{ IDS_LOG, MAKEINTRESOURCE(IDD_LOG), Log_DlgProc },
	{ IDS_SETTINGS, MAKEINTRESOURCE(IDD_SETTINGS), Settings_DlgProc }
};
static const size_t g_tabcount=sizeof(g_tabs)/sizeof(TabData);

void SetBlock(bool block) {
	g_config.Block=block;
	g_filter->setblock(block);
	
	g_nid.hIcon=(HICON)LoadImage(GetModuleHandle(NULL), block?MAKEINTRESOURCE(IDI_MAIN):MAKEINTRESOURCE(IDI_DISABLED), IMAGE_ICON, 0, 0, LR_SHARED|LR_DEFAULTSIZE);
	if(g_trayactive) Shell_NotifyIcon(NIM_MODIFY, &g_nid);

	SendMessage(g_main, WM_SETICON, ICON_BIG, (LPARAM)g_nid.hIcon);
	SendMessage(g_main, WM_SETICON, ICON_SMALL, (LPARAM)g_nid.hIcon);

	SendMessage(g_tabs[0].Tab, WM_LOG_HOOK, 0, 0);
}

void SetBlockHttp(bool block) {
	g_config.BlockHttp=block;
	g_filter->setblockhttp(block);
	
	SendMessage(g_tabs[0].Tab, WM_LOG_HOOK, 0, 0);
}

static void Main_OnVisible(HWND hwnd, BOOL visible);
static void Main_OnClose(HWND hwnd) {
	if(g_config.HideOnClose) Main_OnVisible(hwnd, FALSE);
	else DestroyWindow(hwnd);
}

static void Main_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case ID_TRAY_PEERBLOCK:
			Main_OnVisible(hwnd, g_config.WindowHidden?TRUE:FALSE);
			break;
		case ID_TRAY_ENABLED:
			if(!g_config.Block) SetBlock(true);
			break;
		case ID_TRAY_DISABLED:
			if(g_config.Block) SetBlock(false);
			break;
		case ID_TRAY_BLOCKHTTP:
			SetBlockHttp(!g_config.BlockHttp);
			break;
		case ID_TRAY_ALWAYSONTOP:
			g_config.AlwaysOnTop=!g_config.AlwaysOnTop;
			SetWindowPos(hwnd, g_config.AlwaysOnTop?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

			// kinda hackish
			CheckDlgButton(g_pages[1].hwnd, IDC_ONTOP, g_config.AlwaysOnTop?BST_CHECKED:BST_UNCHECKED);
			break;
		case ID_TRAY_HIDETRAYICON:
			if(g_trayactive) {
				g_trayactive=false;
				Shell_NotifyIcon(NIM_DELETE, &g_nid);
				
				if(g_config.FirstHide) {
					g_config.FirstHide=false;
					MessageBox(hwnd, IDS_HIDINGTEXT, IDS_HIDING, MB_ICONINFORMATION|MB_OK);
				}
			}
			break;
		case ID_TRAY_ABOUT:
			if(!g_about) g_about=CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwnd, About_DlgProc);
			else SetForegroundWindow(g_about);
			break;
		case ID_TRAY_EXIT:
			DestroyWindow(hwnd);
			break;
	}
}

static void Main_OnDestroy(HWND hwnd) {
	{
		try {
			g_config.Save();
		}
		catch(exception &ex) {
			ExceptionBox(hwnd, ex, __FILE__, __LINE__);
		}
	}

	if(g_trayactive) Shell_NotifyIcon(NIM_DELETE, &g_nid);

	{
		try {
			for(size_t i=0; i<g_tabcount; i++)
				DestroyWindow(g_tabs[i].Tab);
		}
		catch(exception &ex) {
			ExceptionBox(hwnd, ex, __FILE__, __LINE__);
		}
	}

	PostQuitMessage(0);
}

static void Main_OnEndSession(HWND hwnd, BOOL fEnding) {
	if(fEnding) Main_OnDestroy(hwnd);
}

static void Main_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) {
	RECT rc={0};
	rc.right=363;
	rc.bottom=235;

	MapDialogRect(hwnd, &rc);

	lpMinMaxInfo->ptMinTrackSize.x=rc.right;
	lpMinMaxInfo->ptMinTrackSize.y=rc.bottom;
}

static void FitTabChild(HWND tabs, HWND child) {
	RECT rc;
	GetClientRect(tabs, &rc);
	TabCtrl_AdjustRect(tabs, FALSE, &rc);

	MoveWindow(child, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, TRUE);
}

static void Main_OnSize(HWND hwnd, UINT state, int cx, int cy);
static BOOL Main_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	g_main=hwnd;

	TRACEI("[Main_OnInitDialog]  > Entering routine.");

	TRACEI("[Main_OnInitDialog]    loading config");
	bool firsttime=false;
	try {
		if(!g_config.Load()) {
			TRACEI("[Main_OnInitDialog]    displaying first-time wizard");
			DisplayStartupWizard(hwnd);
			g_config.Save();
			firsttime=true;
		}
		else
		{
			TRACES("[Main_OnInitDialog]    Config loaded.");
		}

		// Setup tracelogging
		if (g_config.TracelogEnabled)
			g_tlog.SetLoglevel((TRACELOG_LEVEL)g_config.TracelogLevel);
		else
			g_tlog.SetLoglevel(TRACELOG_LEVEL_NONE);
	}
	catch(exception &ex) {
		TRACEC("[Main_OnInitDialog]    Exception trying to load config!");
		ExceptionBox(hwnd, ex, __FILE__, __LINE__);
		DestroyWindow(hwnd);
		return FALSE;
	}

	TRACEI("[Main_OnInitDialog]    resetting g_filter");
	try {
		g_filter.reset(new pgfilter());
		TRACES("[Main_OnInitDialog]    g_filter reset.");
	}
	catch(peerblock_error &ex) 
	{
		PeerBlockExceptionBox(NULL, ex);
		DestroyWindow(hwnd);
		return FALSE;
	}
	catch(win32_error &ex) {
		DWORD code = ex.error();
		if (code == 577)
		{
			const tstring text=boost::str(tformat(LoadString(IDS_UNSIGNEDDRIVERTEXT)) % ex.func() % ex.error() % ex.what());
			MessageBox(hwnd, text, IDS_DRIVERERR, MB_ICONERROR|MB_OK);
		}
		else
		{
			const tstring text=boost::str(tformat(LoadString(IDS_DRIVERERRWIN32TEXT)) % ex.func() % ex.error() % ex.what());
			MessageBox(hwnd, text, IDS_DRIVERERR, MB_ICONERROR|MB_OK);
		}

		DestroyWindow(hwnd);
		return FALSE;
	}
	catch(exception &ex) {
		const tstring text=boost::str(tformat(LoadString(IDS_DRIVERERRTEXT))%typeid(ex).name()%ex.what());
		MessageBox(hwnd, text, IDS_DRIVERERR, MB_ICONERROR|MB_OK);

		DestroyWindow(hwnd);
		return FALSE;
	}

	TRACEI("[Main_OnInitDialog]    getting tabs");
	HWND tabs=GetDlgItem(hwnd, IDC_TABS);

	for(size_t i=0; i<g_tabcount; i++) {
		tstring buf=LoadString(g_tabs[i].Title);

		TCITEM tci={0};
		tci.mask=TCIF_TEXT;
		tci.pszText=const_cast<LPTSTR>(buf.c_str());
		TabCtrl_InsertItem(tabs, i, &tci);

		g_tabs[i].Tab=CreateDialogParam(GetModuleHandle(NULL), g_tabs[i].Template, tabs, g_tabs[i].Proc, 0);
		FitTabChild(tabs, g_tabs[i].Tab);
	}

	if(firsttime) {
		TRACEI("[Main_OnInitDialog]    updating lists for firsttime");
		UpdateLists(g_tabs[0].Tab);
		SendMessage(g_tabs[0].Tab, WM_TIMER, TIMER_UPDATE, 0);
	}

	TRACEI("[Main_OnInitDialog]    setting block");
	g_filter->setblock(g_config.Block);

	TRACEI("[Main_OnInitDialog]    setting HTTP block");
	g_filter->setblockhttp(g_config.BlockHttp);

	TRACEI("[Main_OnInitDialog]    loading lists");
	LoadLists(hwnd);
	TRACES("[Main_OnInitDialog]    Lists loaded.");

	SendMessage(g_tabs[0].Tab, WM_LOG_HOOK, 0, g_config.Block?TRUE:FALSE);

	TRACEI("[Main_OnInitDialog]    doing other stuff");
	g_nid.cbSize=sizeof(NOTIFYICONDATA);
	g_nid.hWnd=hwnd;
	g_nid.uID=TRAY_ID;
	g_nid.uCallbackMessage=WM_PG2_TRAY;
	g_nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;	
	g_nid.hIcon=(HICON)LoadImage(GetModuleHandle(NULL), g_config.Block?MAKEINTRESOURCE(IDI_MAIN):MAKEINTRESOURCE(IDI_DISABLED), IMAGE_ICON, 0, 0, LR_SHARED|LR_DEFAULTSIZE);
	StringCbCopy(g_nid.szTip, sizeof(g_nid.szTip), _T("PeerBlock"));

	if((g_trayactive=(!g_config.HideTrayIcon && !g_config.StayHidden))) {
		Shell_NotifyIcon(NIM_ADD, &g_nid);
		
		g_nid.uFlags=NIF_ICON;
	}

	WM_TRAY_CREATED=RegisterWindowMessage(_T("TaskbarCreated"));

	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_nid.hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_nid.hIcon);

	if(!g_config.StartMinimized && !g_config.HideTrayIcon && !g_config.WindowHidden) {
		if(g_config.ShowSplash) DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SPLASH), hwnd, Splash_DlgProc);
		SendMessage(hwnd, WM_MAIN_VISIBLE, 0, TRUE);
	}
	else g_config.WindowHidden=true;

	UINT swpflags=0;
	if(
		g_config.WindowPos.left==0 && g_config.WindowPos.top==0 &&
		g_config.WindowPos.right==0 && g_config.WindowPos.bottom==0
	) swpflags=SWP_NOMOVE|SWP_NOSIZE;
	if(!g_config.AlwaysOnTop) swpflags|=SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER;

	SetWindowPos(hwnd, g_config.AlwaysOnTop?HWND_TOPMOST:NULL,
		g_config.WindowPos.left, g_config.WindowPos.top,
		g_config.WindowPos.right-g_config.WindowPos.left,
		g_config.WindowPos.bottom-g_config.WindowPos.top,
		swpflags);

	if((swpflags&SWP_NOSIZE) == 0) SendMessage(hwnd, DM_REPOSITION, 0, 0);

	RECT rc;
	GetClientRect(hwnd, &rc);
	Main_OnSize(hwnd, 0, rc.right, rc.bottom);

	WM_PG2_VISIBLE=RegisterWindowMessage(_T("PeerGuardian2SetVisible"));
	WM_PG2_LOADLISTS=RegisterWindowMessage(_T("PeerGuardian2LoadLists"));

	SetTimer(hwnd, TIMER_BLINKTRAY, 500, NULL);
	SetTimer(hwnd, TIMER_PROCESSDB, 600000, NULL);

	#ifdef _WIN32_WINNT
	if(g_config.WindowHidden)
		SetProcessWorkingSetSize(GetCurrentProcess(), (size_t)-1, (size_t)-1);
	#endif


	//night_stalker_z: Update lists at startup so it blocks
	// needs internet conenction
	if (g_config.UpdateAtStartup)
	{
		UpdateLists(hwnd);
		LoadLists(hwnd);
	}
	//BlockWithoutUpdate(hwnd);

	return FALSE;
}

static void Main_OnMove(HWND hwnd, int x, int y) {
	GetWindowRect(hwnd, &g_config.WindowPos);
}

static LRESULT Main_OnNotify(HWND hwnd, INT idCtrl, LPNMHDR pnmh) {
	if(pnmh->code==TCN_SELCHANGE && pnmh->idFrom==IDC_TABS) {
		int sel=TabCtrl_GetCurSel(pnmh->hwndFrom);

		if(sel!=-1) {
			SetWindowPos(g_tabs[sel].Tab, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
			for(size_t i=0; i<g_tabcount; i++)
				if(i!=sel) ShowWindow(g_tabs[i].Tab, SW_HIDE);
		}
	}

	return 0;
}

static void Main_OnSize(HWND hwnd, UINT state, int cx, int cy) {
	HWND tabs=GetDlgItem(hwnd, IDC_TABS);

	MoveWindow(tabs, 7, 7, cx-14, cy-14, TRUE);

	HDWP dwp=BeginDeferWindowPos(g_tabcount);

	RECT rc;
	GetClientRect(tabs, &rc);
	TabCtrl_AdjustRect(tabs, FALSE, &rc);

	for(size_t i=0; i<g_tabcount; i++)
		DeferWindowPos(dwp, g_tabs[i].Tab, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);

	EndDeferWindowPos(dwp);
	
	if(state==SIZE_RESTORED) {
		RECT rc;
		GetWindowRect(hwnd, &rc);

		if(rc.left>=0 && rc.top>=0 && rc.right>=0 && rc.bottom>=0)
			g_config.WindowPos=rc;
	}
}

static string format_ipport(unsigned int ip, unsigned short port) {
	const unsigned char *bytes=reinterpret_cast<const unsigned char*>(&ip);

	char buf[24];
	StringCbPrintfA(buf, sizeof(buf), port?"%u.%u.%u.%u:%u":"%u.%u.%u.%u", bytes[3], bytes[2], bytes[1], bytes[0], port);

	return buf;
}

static boost::shared_ptr<thread> g_dbthread;
static spinlock g_processlock;

static void Main_ProcessDb() {
	sqlite3_try_lock lock(g_con, true);

	if(lock.is_locked()) {
		if(g_config.CleanupType==ArchiveDelete) {
			try {
				queue<string> dates;
				{
					ostringstream ss;
					ss << "select distinct date(time, 'localtime') from t_history where time <= julianday('now', '-" << g_config.CleanupInterval << " days');";

					sqlite3_command cmd(g_con, ss.str());
					sqlite3_reader reader=cmd.executereader();
					while(reader.read())
						dates.push(reader.getstring(0));
				}

				for(; !dates.empty(); dates.pop()) {
					path p=g_config.ArchivePath;

					if(!p.has_root()) p=path::base_dir()/p;
					if(!path::exists(p)) path::create_directory(p);

					p/=(UTF8_TSTRING(dates.front())+_T(".log"));

					FILE *fp=_tfopen(p.file_str().c_str(), _T("a"));

					if(fp) {
						boost::shared_ptr<FILE> safefp(fp, fclose);

						sqlite3_command cmd(g_con, "select time(time, 'localtime'),name,source,sourceport,destination,destport,protocol,action from t_history,t_names where time>=julianday('"+dates.front()+"', 'utc') and time<julianday('"+dates.front()+"', '+1 days', 'utc') and t_names.id=nameid;");

						sqlite3_reader reader=cmd.executereader();
						while(reader.read()) {
							const string time=reader.getstring(0);
							string name=reader.getstring(1);

							const char *protocol;
							switch(reader.getint(6)) {
								case IPPROTO_ICMP: protocol="ICMP"; break;
								case IPPROTO_IGMP: protocol="IGMP"; break;
								case IPPROTO_GGP: protocol="Gateway^2"; break;
								case IPPROTO_TCP: protocol="TCP"; break;
								case IPPROTO_PUP: protocol="PUP"; break;
								case IPPROTO_UDP: protocol="UDP"; break;
								case IPPROTO_IDP: protocol="XNS IDP"; break;
								case IPPROTO_ND: protocol="NetDisk"; break;
								default: protocol="Unknown"; break;
							}

							fprintf(fp, "%s; %s; %s; %s; %s; %s\n",
								time.c_str(), name.c_str(),
								format_ipport((unsigned int)reader.getint(2), (unsigned short)reader.getint(3)).c_str(),
								format_ipport((unsigned int)reader.getint(4), (unsigned short)reader.getint(5)).c_str(),
								protocol, reader.getint(7)?"Blocked":"Allowed");
						}
					}
				}
			}
			catch(exception &ex) {
				ExceptionBox(NULL, ex, __FILE__, __LINE__);
			}
		}

		if(g_config.CleanupType==Delete || g_config.CleanupType==ArchiveDelete) 
		{
			try 
			{
				ostringstream ss;

				// first delete all data from the t_history table
				ss << "delete from t_history where time <= julianday('now', '-" << g_config.CleanupInterval << " days');";
				g_con.executenonquery(ss.str());
				lock.commit();

				// now delete all free btree page structures, as per 
				// http://web.utk.edu/~jplyon/sqlite/SQLite_optimization_FAQ.html#compact
				g_con.executenonquery(_T("vacuum;"));

				// HACK:  For some reason, vacuum doesn't do anything if we call it prior to commit(),
				//		  and commit() throws an exception if we call it twice, so we're doing it this way.
			}
			catch(database_error &ex) 
			{
				ExceptionBox(NULL, ex, __FILE__, __LINE__);
			}
		}
	}

	g_dbthread=boost::shared_ptr<thread>();
	g_processlock.leave();
}

static void Main_OnTimer(HWND hwnd, UINT id) {
	if(id==TIMER_BLINKTRAY && g_trayactive) {
		if(g_config.BlinkOnBlock!=Never && (GetTickCount()-6000) < g_blinkstart) {
			if((g_trayblink=!g_trayblink)) {
				HICON icon=g_nid.hIcon;

				g_nid.hIcon=NULL;
				Shell_NotifyIcon(NIM_MODIFY, &g_nid);
				g_nid.hIcon=icon;
			}
			else Shell_NotifyIcon(NIM_MODIFY, &g_nid);
		}
		else if(g_trayblink) {
			Shell_NotifyIcon(NIM_MODIFY, &g_nid);
			g_trayblink=false;
		}
	}
	else if(id==TIMER_PROCESSDB && g_config.CleanupType!=None && g_processlock.tryenter())
		g_dbthread=boost::shared_ptr<thread>(new thread(Main_ProcessDb, THREAD_PRIORITY_BELOW_NORMAL));
}

static void Main_OnTray(HWND hwnd, UINT id, UINT eventMsg) {
	if(eventMsg==WM_LBUTTONUP)
		SendMessage(hwnd, WM_MAIN_VISIBLE, 0, TRUE);
	else if(eventMsg==WM_RBUTTONUP) {
		POINT pt;
		GetCursorPos(&pt);

		HMENU menu=LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAYMENU));
		HMENU submenu=GetSubMenu(menu, 0);

		CheckMenuItem(submenu, ID_TRAY_PEERBLOCK, MF_BYCOMMAND|(g_config.WindowHidden?MF_UNCHECKED:MF_CHECKED));
		CheckMenuItem(submenu, ID_TRAY_ALWAYSONTOP, MF_BYCOMMAND|(g_config.AlwaysOnTop?MF_CHECKED:MF_UNCHECKED));
		CheckMenuItem(submenu, ID_TRAY_ENABLED, MF_BYCOMMAND|(g_config.Block?MF_CHECKED:MF_UNCHECKED));
		CheckMenuItem(submenu, ID_TRAY_DISABLED, MF_BYCOMMAND|(g_config.Block?MF_UNCHECKED:MF_CHECKED));
		CheckMenuItem(submenu, ID_TRAY_BLOCKHTTP, MF_BYCOMMAND|(g_config.BlockHttp?MF_CHECKED:MF_UNCHECKED));

		SetForegroundWindow(hwnd);
		TrackPopupMenuEx(submenu, TPM_BOTTOMALIGN, pt.x, pt.y, hwnd, NULL);

		DestroyMenu(menu);
	}
}

static void Main_OnVisible(HWND hwnd, BOOL visible) {
	int index=TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_TABS));

	if(visible) {
		if(index!=-1) ShowWindow(g_tabs[index].Tab, SW_SHOW);

		ShowWindow(hwnd, SW_SHOW);
		ShowWindow(hwnd, SW_RESTORE);
		SetForegroundWindow(hwnd);
		g_config.WindowHidden=false;

		if(!g_trayactive && !g_config.StayHidden) {
			g_trayactive=true;
			g_config.HideTrayIcon=false;
			Shell_NotifyIcon(NIM_ADD, &g_nid);
		}
	}
	else {
		if(index!=-1) ShowWindow(g_tabs[index].Tab, SW_HIDE);

		ShowWindow(hwnd, SW_HIDE);
		g_config.WindowHidden=true;

	#ifdef _WIN32_WINNT
		SetProcessWorkingSetSize(GetCurrentProcess(), (size_t)-1, (size_t)-1);
	#endif
	}
}

INT_PTR CALLBACK Main_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	TCHAR chBuf[128];
	if (msg != 289)
	{
		_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[Main_DlgProc]  Received MSG: [%d]"), msg);
		//g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_CRITICAL);
	}

	try {

		switch(msg) {
			HANDLE_MSG(hwnd, WM_CLOSE, Main_OnClose);
			HANDLE_MSG(hwnd, WM_COMMAND, Main_OnCommand);
			HANDLE_MSG(hwnd, WM_DESTROY, Main_OnDestroy);
			HANDLE_MSG(hwnd, WM_ENDSESSION, Main_OnEndSession);
			HANDLE_MSG(hwnd, WM_GETMINMAXINFO, Main_OnGetMinMaxInfo);
			HANDLE_MSG(hwnd, WM_INITDIALOG, Main_OnInitDialog);
			HANDLE_MSG(hwnd, WM_MOVE, Main_OnMove);
			HANDLE_MSG(hwnd, WM_NOTIFY, Main_OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, Main_OnSize);
			HANDLE_MSG(hwnd, WM_TIMER, Main_OnTimer);
			case WM_PG2_TRAY:
				Main_OnTray(hwnd, (UINT)wParam, (UINT)lParam);
				return 1;
			case WM_MAIN_VISIBLE:
				Main_OnVisible(hwnd, (BOOL)lParam);
				return 1;
			default:
				if(msg==WM_TRAY_CREATED && g_trayactive) Shell_NotifyIcon(NIM_ADD, &g_nid);
				else if(msg==WM_PG2_VISIBLE) {
					Main_OnVisible(hwnd, (BOOL)lParam);
					return 1;
				}
				else if(msg==WM_PG2_LOADLISTS) {
					LoadLists(hwnd);
					return 1;
				}
				return 0;
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
