/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2014 PeerBlock, LLC

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

static void SettingsProc_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	HWND save=GetDlgItem(hwnd, IDC_SAVE);

	RECT rc;
	GetWindowRect(save, &rc);

	HDWP dwp = BeginDeferWindowPos(3);

	DeferWindowPos(dwp, save, NULL, 7, cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	EndDeferWindowPos(dwp);
}

INT_PTR CALLBACK SettingsFirst_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			case WM_COMMAND:
				switch(LOWORD(wParam)) {
					case IDC_LOGSIZE:
						g_config.LogSize=GetDlgItemInt(hwnd, IDC_LOGSIZE, NULL, FALSE);
						break;
					case IDC_SHOWALLOWED:
						g_config.ShowAllowed=(IsDlgButtonChecked(hwnd, IDC_SHOWALLOWED)==BST_CHECKED);
						break;
					case IDC_COLORCODE:
						g_config.ColorCode=(IsDlgButtonChecked(hwnd, IDC_COLORCODE)==BST_CHECKED);
						break;
					case IDC_LOGALLOWED:
						if(HIWORD(wParam)==CBN_SELCHANGE) {
							int i = ComboBox_GetCurSel((HWND)lParam);

							if(i == 0) {
								g_config.LogAllowed = false;
								g_config.LogBlocked = false;
							}
							else if(i == 1) {
								g_config.LogAllowed = true;
								g_config.LogBlocked = false;
							}
							else if(i == 2) {
								g_config.LogAllowed = false;
								g_config.LogBlocked = true;
							}
							else {
								g_config.LogAllowed = true;
								g_config.LogBlocked = true;
							}
						}
						break;
					case IDC_CLEANUP:
						if(HIWORD(wParam)==CBN_SELCHANGE) {
							int i=ComboBox_GetCurSel((HWND)lParam);

							BOOL e=(i!=0);
							BOOL e2=(i==2);

							EnableWindow(GetDlgItem(hwnd, IDC_CLEANUPTIMESPIN), e);
							EnableWindow(GetDlgItem(hwnd, IDC_CLEANUPTIME), e);
							EnableWindow(GetDlgItem(hwnd, IDC_ARCHIVETO), e2);
							EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), e2);
							EnableWindow(GetDlgItem(hwnd, IDC_MAXHISTORYSIZE), e);
							EnableWindow(GetDlgItem(hwnd, IDC_MAXHISTORYSIZESPIN), e);

							if(i==0) g_config.CleanupType=None;
							else if(i==1) g_config.CleanupType=Delete;
							else if(i==2) g_config.CleanupType=ArchiveDelete;
						}
						break;
					case IDC_ARCHIVETO:
						if(HIWORD(wParam)==EN_CHANGE)
							g_config.ArchivePath=GetDlgItemText(hwnd, IDC_ARCHIVETO);
						break;
					case IDC_CLEANUPTIME:
						g_config.CleanupInterval=(unsigned short)GetDlgItemInt(hwnd, IDC_CLEANUPTIME, NULL, FALSE);
						break;
					case IDC_BROWSE: {
						BROWSEINFO bi={0};
						bi.hwndOwner=hwnd;
						bi.ulFlags=BIF_USENEWUI;

						LPITEMIDLIST id=SHBrowseForFolder(&bi);

						if(id) {
							TCHAR buf[MAX_PATH];

							if(SHGetPathFromIDList(id, buf)) {
								path p=path::relative_to(path::base_dir(), buf);
								SetDlgItemText(hwnd, IDC_ARCHIVETO, p.c_str());
							}

							ILFree(id);
						}
					} break;
					case IDC_MAXHISTORYSIZE:
						g_config.MaxHistorySize=((unsigned short)GetDlgItemInt(hwnd, IDC_MAXHISTORYSIZE, NULL, FALSE)) * 1000000;
						break;
					case IDC_SAVE:
						TRACEI("[settingsproc] [SettingsFirst_DlgProc]    saving configuration, at user request");
						g_config.Save();
						break;
					case IDC_IBLUSER:
						g_config.IblUsername = GetDlgItemText(hwnd, IDC_IBLUSER);
						break;
					case IDC_IBLPIN:
						g_config.IblPin = GetDlgItemText(hwnd, IDC_IBLPIN);
						break;
				}
				break;
			case WM_INITDIALOG: {
				SendDlgItemMessage(hwnd, IDC_LOGSIZE, EM_SETLIMITTEXT, 4, 0);
				SendDlgItemMessage(hwnd, IDC_LOGSIZESPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwnd, IDC_LOGSIZE), 0);
				SendDlgItemMessage(hwnd, IDC_LOGSIZESPIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));

				SendDlgItemMessage(hwnd, IDC_CLEANUPTIME, EM_SETLIMITTEXT, 2, 0);
				SendDlgItemMessage(hwnd, IDC_CLEANUPTIMESPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwnd, IDC_CLEANUPTIME), 0);
				SendDlgItemMessage(hwnd, IDC_CLEANUPTIMESPIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));

				SendDlgItemMessage(hwnd, IDC_MAXHISTORYSIZE, EM_SETLIMITTEXT, 4, 0);
				SendDlgItemMessage(hwnd, IDC_MAXHISTORYSIZESPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwnd, IDC_MAXHISTORYSIZE), 0);
				SendDlgItemMessage(hwnd, IDC_MAXHISTORYSIZESPIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 0));

				HWND logallowed = GetDlgItem(hwnd, IDC_LOGALLOWED);

				tstring str = LoadString(IDS_LOGNONE);
				ComboBox_AddString(logallowed, str.c_str());

				str = LoadString(IDS_LOGALLOWED);
				ComboBox_AddString(logallowed, str.c_str());

				str = LoadString(IDS_LOGBLOCKED);
				ComboBox_AddString(logallowed, str.c_str());

				str = LoadString(IDS_LOGBOTH);
				ComboBox_AddString(logallowed, str.c_str());

				int idx;
				if(!g_config.LogAllowed && !g_config.LogBlocked) {
					idx = 0;
				}
				else if(g_config.LogAllowed && !g_config.LogBlocked) {
					idx = 1;
				}
				else if(!g_config.LogAllowed && g_config.LogBlocked) {
					idx = 2;
				}
				else {
					idx = 3;
				}

				ComboBox_SetCurSel(logallowed, idx);

				HWND cleanup=GetDlgItem(hwnd, IDC_CLEANUP);

				str=LoadString(IDS_DONOTHING);
				ComboBox_AddString(cleanup, str.c_str());

				str=LoadString(IDS_REMOVE);
				ComboBox_AddString(cleanup, str.c_str());

				str=LoadString(IDS_ARCHIVEREMOVE);
				ComboBox_AddString(cleanup, str.c_str());

				ComboBox_SetCurSel(cleanup, (int)g_config.CleanupType);

				if(g_config.CleanupType!=None) {
					EnableWindow(GetDlgItem(hwnd, IDC_CLEANUPTIMESPIN), TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_CLEANUPTIME), TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_MAXHISTORYSIZE), TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_MAXHISTORYSIZESPIN), TRUE);

					if(g_config.CleanupType==ArchiveDelete) {
						EnableWindow(GetDlgItem(hwnd, IDC_ARCHIVETO), TRUE);
						EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
					}
				}

				SetDlgItemInt(hwnd, IDC_LOGSIZE, g_config.LogSize, FALSE);
				if(g_config.ShowAllowed) CheckDlgButton(hwnd, IDC_SHOWALLOWED, BST_CHECKED);
				if(g_config.ColorCode) CheckDlgButton(hwnd, IDC_COLORCODE, BST_CHECKED);

				SetDlgItemInt(hwnd, IDC_CLEANUPTIME, g_config.CleanupInterval, FALSE);

				if(!g_config.ArchivePath.empty())
					SetDlgItemText(hwnd, IDC_ARCHIVETO, g_config.ArchivePath.directory_str().c_str());

				SetDlgItemInt(hwnd, IDC_MAXHISTORYSIZE, (UINT)(g_config.MaxHistorySize / 1000000), FALSE);

				ColorPicker_SetColor(GetDlgItem(hwnd, IDC_ATEXT), g_config.AllowedColor.Text);
				ColorPicker_SetColor(GetDlgItem(hwnd, IDC_ABG), g_config.AllowedColor.Background);
				ColorPicker_SetColor(GetDlgItem(hwnd, IDC_BTEXT), g_config.BlockedColor.Text);
				ColorPicker_SetColor(GetDlgItem(hwnd, IDC_BBG), g_config.BlockedColor.Background);
				ColorPicker_SetColor(GetDlgItem(hwnd, IDC_HTTPTEXT), g_config.HttpColor.Text);
				ColorPicker_SetColor(GetDlgItem(hwnd, IDC_HTTPBG), g_config.HttpColor.Background);

				// I-Blocklist Subscription info
				if(!g_config.IblUsername.empty())
					SetDlgItemText(hwnd, IDC_IBLUSER, g_config.IblUsername.c_str());
				if(!g_config.IblPin.empty())
					SetDlgItemText(hwnd, IDC_IBLPIN, g_config.IblPin.c_str());

				RECT rc;
				GetClientRect(hwnd, &rc);
				SendMessage(hwnd, WM_SIZE, 0, MAKELONG(rc.right-rc.left, rc.bottom-rc.top));
			} break;
			case WM_NOTIFY: {
				const CPNM_SETCOLOR *sc=(const CPNM_SETCOLOR*)lParam;
				switch(sc->nmh.idFrom) {
					case IDC_ATEXT:
						g_config.AllowedColor.Text=sc->color;
						break;
					case IDC_ABG:
						g_config.AllowedColor.Background=sc->color;
						break;
					case IDC_BTEXT:
						g_config.BlockedColor.Text=sc->color;
						break;
					case IDC_BBG:
						g_config.BlockedColor.Background=sc->color;
						break;
					case IDC_HTTPTEXT:
						g_config.HttpColor.Text=sc->color;
						break;
					case IDC_HTTPBG:
						g_config.HttpColor.Background=sc->color;
						break;
				}
			}
			break;
		}
		return 0;
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

INT_PTR CALLBACK SettingsSecond_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			case WM_COMMAND:
				switch(LOWORD(wParam)) {
					case IDC_STARTWITHWINDOWS: {
						HKEY key;
						if(RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_WRITE, &key)==ERROR_SUCCESS) {
							if(IsDlgButtonChecked(hwnd, IDC_STARTWITHWINDOWS)==BST_CHECKED) {
								TCHAR dir[MAX_PATH];
								DWORD len=GetModuleFileName(NULL, dir, MAX_PATH);

								if(len) RegSetValueEx(key, _T("PeerBlock"), 0, REG_SZ, (LPBYTE)dir, (len+1)*sizeof(TCHAR));
							}
							else RegDeleteValue(key, _T("PeerBlock"));

							RegCloseKey(key);
						}
					} break;
					case IDC_STARTMINIMIZED: {
						g_config.StartMinimized=(IsDlgButtonChecked(hwnd, IDC_STARTMINIMIZED)==BST_CHECKED);

						EnableWindow(GetDlgItem(hwnd, IDC_SHOWSPLASH), !g_config.StartMinimized);
					} break;
					case IDC_SHOWSPLASH:
						g_config.ShowSplash=(IsDlgButtonChecked(hwnd, IDC_SHOWSPLASH)==BST_CHECKED);
						break;
					case IDC_USEPROXY: {
						const BOOL b=(IsDlgButtonChecked(hwnd, IDC_USEPROXY)==BST_CHECKED);

						EnableWindow(GetDlgItem(hwnd, IDC_PROXYHOST), b);
						EnableWindow(GetDlgItem(hwnd, IDC_HTTP), b);
						EnableWindow(GetDlgItem(hwnd, IDC_SOCKS5), b);

						if(b) g_config.UpdateProxy=GetDlgItemText(hwnd, IDC_PROXYHOST);
						else g_config.UpdateProxy.clear();
					} break;
					case IDC_PROXYHOST:
						if(HIWORD(wParam)==EN_CHANGE)
							g_config.UpdateProxy=GetDlgItemText(hwnd, IDC_PROXYHOST);
						break;
					case IDC_HTTP:
					case IDC_SOCKS5:
						g_config.UpdateProxyType=(IsDlgButtonChecked(hwnd, IDC_HTTP)==BST_CHECKED)?CURLPROXY_HTTP:CURLPROXY_SOCKS5;
						break;
					case IDC_AUTOUPDATE: {
						const BOOL b=(IsDlgButtonChecked(hwnd, IDC_AUTOUPDATE)==BST_CHECKED);

						EnableWindow(GetDlgItem(hwnd, IDC_AUTOUPDATETIMESPIN), b);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOUPDATETIME), b);

						if(b) {
							if(GetWindowTextLength(GetDlgItem(hwnd, IDC_AUTOUPDATETIME))>0)
								g_config.UpdateInterval=(unsigned short)GetDlgItemInt(hwnd, IDC_AUTOUPDATETIME, NULL, FALSE);
							else SetDlgItemInt(hwnd, IDC_AUTOUPDATETIME, 2, FALSE);
						}
						else g_config.UpdateInterval=0;
					} break;
					case IDC_AUTOUPDATETIME:
						if(HIWORD(wParam)==EN_CHANGE)
							g_config.UpdateInterval=(unsigned short)GetDlgItemInt(hwnd, IDC_AUTOUPDATETIME, NULL, FALSE);
						break;
					case IDC_AUTOCLOSE: {
						const BOOL b=(IsDlgButtonChecked(hwnd, IDC_AUTOCLOSE)==BST_CHECKED);

						EnableWindow(GetDlgItem(hwnd, IDC_AUTOCLOSETIMESPIN), b);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOCLOSETIME), b);

						if(b) {
							if(GetWindowTextLength(GetDlgItem(hwnd, IDC_AUTOCLOSETIME))>0)
								g_config.UpdateCountdown=(short)GetDlgItemInt(hwnd, IDC_AUTOCLOSETIME, NULL, FALSE);
							else SetDlgItemInt(hwnd, IDC_AUTOCLOSETIME, 5, FALSE);
						}
						else g_config.UpdateCountdown=-1;
					} break;
					case IDC_AUTOCLOSETIME:
						if(HIWORD(wParam)==EN_CHANGE)
							g_config.UpdateCountdown=(unsigned short)GetDlgItemInt(hwnd, IDC_AUTOCLOSETIME, NULL, FALSE);
						break;
					case IDC_CHECKPB:
					case IDC_CHECKLISTS: {
						const bool ba=g_config.UpdatePeerBlock=(IsDlgButtonChecked(hwnd, IDC_CHECKPB)==BST_CHECKED);
						const bool bb=g_config.UpdateLists=(IsDlgButtonChecked(hwnd, IDC_CHECKLISTS)==BST_CHECKED);

						BOOL b=ba||bb;

						EnableWindow(GetDlgItem(hwnd, IDC_AUTOUPDATE), b);
						EnableWindow(GetDlgItem(hwnd, IDC_USEPROXY), b);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOCLOSE), b);

						b=(ba||bb)&&(IsDlgButtonChecked(hwnd, IDC_AUTOUPDATE)==BST_CHECKED);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOUPDATETIMESPIN), b);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOUPDATETIME), b);

						b=(ba||bb)&&(IsDlgButtonChecked(hwnd, IDC_USEPROXY)==BST_CHECKED);
						EnableWindow(GetDlgItem(hwnd, IDC_PROXYHOST), b);
						EnableWindow(GetDlgItem(hwnd, IDC_HTTP), b);
						EnableWindow(GetDlgItem(hwnd, IDC_SOCKS5), b);

						b=(ba||bb)&&(IsDlgButtonChecked(hwnd, IDC_AUTOCLOSE)==BST_CHECKED);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOCLOSETIMESPIN), b);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOCLOSETIME), b);
					} break;
					case IDC_HIDETRAY:
						g_config.StayHidden=(IsDlgButtonChecked(hwnd, IDC_HIDETRAY)==BST_CHECKED);
						if(g_config.StayHidden && g_trayactive) {
							g_trayactive=false;
							g_config.HideTrayIcon=true;
							Shell_NotifyIcon(NIM_DELETE, &g_nid);
						}
						else if(!g_config.StayHidden && !g_trayactive) {
							g_trayactive=true;
							g_config.HideTrayIcon=false;
							Shell_NotifyIcon(NIM_ADD, &g_nid);
						}
						break;
					case IDC_HIDEONCLOSE:
						g_config.HideOnClose=(IsDlgButtonChecked(hwnd, IDC_HIDEONCLOSE)==BST_CHECKED);
						break;
					case IDC_ONTOP:
						g_config.AlwaysOnTop=(IsDlgButtonChecked(hwnd, IDC_ONTOP)==BST_CHECKED);
						SetWindowPos(g_main, g_config.AlwaysOnTop?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
						break;
					case IDC_UPDATE_AT_STARTUP:
						g_config.UpdateAtStartup=(IsDlgButtonChecked(hwnd, IDC_UPDATE_AT_STARTUP) == BST_CHECKED);
						break;
					case IDC_NOTIFY: {
						BOOL e=(IsDlgButtonChecked(hwnd, IDC_NOTIFY)==BST_CHECKED);

						EnableWindow(GetDlgItem(hwnd, IDC_NOTIFYON), e);
						EnableWindow(GetDlgItem(hwnd, IDC_BLINKTRAY), e);
						EnableWindow(GetDlgItem(hwnd, IDC_NOTIFYWINDOW), e);

						if(e) {
							const int i=ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_NOTIFYON));
							const NotifyType t=(i==0)?OnHttpBlock:OnBlock;

							if(IsDlgButtonChecked(hwnd, IDC_BLINKTRAY)==BST_CHECKED)
								g_config.BlinkOnBlock=t;
							else g_config.BlinkOnBlock=Never;

							if(IsDlgButtonChecked(hwnd, IDC_NOTIFYWINDOW)==BST_CHECKED)
								g_config.NotifyOnBlock=t;
							else g_config.NotifyOnBlock=Never;
						}
						else {
							g_config.BlinkOnBlock=Never;
							g_config.NotifyOnBlock=Never;
						}
					} break;
					case IDC_NOTIFYON:
						if(HIWORD(wParam)==CBN_SELCHANGE) {
							const int i=ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_NOTIFYON));
							const NotifyType nt=(i==0)?OnHttpBlock:OnBlock;

							if(IsDlgButtonChecked(hwnd, IDC_BLINKTRAY)==BST_CHECKED)
								g_config.BlinkOnBlock=nt;

							if(IsDlgButtonChecked(hwnd, IDC_NOTIFYWINDOW)==BST_CHECKED)
								g_config.NotifyOnBlock=nt;
						}
						break;
					case IDC_BLINKTRAY:
						if(IsDlgButtonChecked(hwnd, IDC_BLINKTRAY)==BST_CHECKED) {
							const int i=ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_NOTIFYON));
							g_config.BlinkOnBlock=(i==0)?OnHttpBlock:OnBlock;
						}
						else g_config.BlinkOnBlock=Never;
						break;
					case IDC_NOTIFYWINDOW:
						if(IsDlgButtonChecked(hwnd, IDC_NOTIFYWINDOW)==BST_CHECKED) {
							const int i=ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_NOTIFYON));
							g_config.NotifyOnBlock=(i==0)?OnHttpBlock:OnBlock;
						}
						else g_config.NotifyOnBlock=Never;
						break;
					case IDC_SAVE:
						TRACEI("[settingsproc] [SettingsSecond_DlgProc]    saving configuration, at user request");
						g_config.Save();
						break;
				}
				break;
			case WM_INITDIALOG: {
				SendDlgItemMessage(hwnd, IDC_AUTOUPDATETIME, EM_SETLIMITTEXT, 2, 0);
				SendDlgItemMessage(hwnd, IDC_AUTOUPDATETIMESPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwnd, IDC_AUTOUPDATETIME), 0);
				SendDlgItemMessage(hwnd, IDC_AUTOUPDATETIMESPIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(99, 1));

				SendDlgItemMessage(hwnd, IDC_AUTOCLOSETIME, EM_SETLIMITTEXT, 2, 0);
				SendDlgItemMessage(hwnd, IDC_AUTOCLOSETIMESPIN, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwnd, IDC_AUTOCLOSETIME), 0);
				SendDlgItemMessage(hwnd, IDC_AUTOCLOSETIMESPIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(99, 0));

				if(g_config.StartMinimized) {
					CheckDlgButton(hwnd, IDC_STARTMINIMIZED, BST_CHECKED);
					EnableWindow(GetDlgItem(hwnd, IDC_SHOWSPLASH), FALSE);
				}

				if(g_config.ShowSplash) CheckDlgButton(hwnd, IDC_SHOWSPLASH, BST_CHECKED);
				if(g_config.StayHidden) CheckDlgButton(hwnd, IDC_HIDETRAY, BST_CHECKED);
				if(g_config.HideOnClose) CheckDlgButton(hwnd, IDC_HIDEONCLOSE, BST_CHECKED);
				if(g_config.AlwaysOnTop) CheckDlgButton(hwnd, IDC_ONTOP, BST_CHECKED);
				if(g_config.UpdateAtStartup) CheckDlgButton(hwnd, IDC_UPDATE_AT_STARTUP, BST_CHECKED);

				if(g_config.UpdateInterval>0) {
					CheckDlgButton(hwnd, IDC_AUTOUPDATE, BST_CHECKED);
					SetDlgItemInt(hwnd, IDC_AUTOUPDATETIME, g_config.UpdateInterval, FALSE);
				}

				if(g_config.UpdateProxy.length()>0) {
					CheckDlgButton(hwnd, IDC_USEPROXY, BST_CHECKED);
					SetDlgItemText(hwnd, IDC_PROXYHOST, g_config.UpdateProxy.c_str());
					CheckDlgButton(hwnd, (g_config.UpdateProxyType==CURLPROXY_HTTP)?IDC_HTTP:IDC_SOCKS5, BST_CHECKED);
				}

				if(g_config.UpdateCountdown>=0) {
					CheckDlgButton(hwnd, IDC_AUTOCLOSE, BST_CHECKED);
					SetDlgItemInt(hwnd, IDC_AUTOCLOSETIME, g_config.UpdateCountdown, FALSE);
				}

				if(g_config.UpdatePeerBlock || g_config.UpdateLists) {
					if(g_config.UpdatePeerBlock) CheckDlgButton(hwnd, IDC_CHECKPB, BST_CHECKED);
					if(g_config.UpdateLists) CheckDlgButton(hwnd, IDC_CHECKLISTS, BST_CHECKED);

					EnableWindow(GetDlgItem(hwnd, IDC_AUTOUPDATE), TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_USEPROXY), TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_AUTOCLOSE), TRUE);

					if(g_config.UpdateInterval>0) {
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOUPDATETIMESPIN), TRUE);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOUPDATETIME), TRUE);
					}

					if(g_config.UpdateProxy.length()>0) {
						EnableWindow(GetDlgItem(hwnd, IDC_PROXYHOST), TRUE);
						EnableWindow(GetDlgItem(hwnd, IDC_HTTP), TRUE);
						EnableWindow(GetDlgItem(hwnd, IDC_SOCKS5), TRUE);
					}

					if(g_config.UpdateCountdown>=0) {
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOCLOSETIMESPIN), TRUE);
						EnableWindow(GetDlgItem(hwnd, IDC_AUTOCLOSETIME), TRUE);
					}
				}

				HKEY key;
				if(RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_READ, &key)==ERROR_SUCCESS) {
					TCHAR dir[MAX_PATH];
					GetModuleFileName(NULL, dir, MAX_PATH);

					TCHAR data[MAX_PATH];
					DWORD type=REG_SZ, datalen=sizeof(data);

					if(RegQueryValueEx(key, _T("PeerBlock"), NULL, &type, (LPBYTE)data, &datalen)==ERROR_SUCCESS && type==REG_SZ && !_tcscmp(data, dir))
						CheckDlgButton(hwnd, IDC_STARTWITHWINDOWS, BST_CHECKED);

					RegCloseKey(key);
				}

				HWND notifyon=GetDlgItem(hwnd, IDC_NOTIFYON);

				tstring str=LoadString(IDS_HTTPBLOCKS);
				ComboBox_AddString(notifyon, str.c_str());

				str=LoadString(IDS_ALLBLOCKS);
				ComboBox_AddString(notifyon, str.c_str());

				ComboBox_SetCurSel(notifyon, 0);

				if(g_config.BlinkOnBlock!=Never || g_config.NotifyOnBlock!=Never) {
					CheckDlgButton(hwnd, IDC_NOTIFY, BST_CHECKED);

					EnableWindow(notifyon, TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_BLINKTRAY), TRUE);
					EnableWindow(GetDlgItem(hwnd, IDC_NOTIFYWINDOW), TRUE);

					if(g_config.BlinkOnBlock==OnHttpBlock || g_config.NotifyOnBlock==OnHttpBlock)
						ComboBox_SetCurSel(notifyon, 0);
					else ComboBox_SetCurSel(notifyon, 1);

					if(g_config.BlinkOnBlock!=Never)
						CheckDlgButton(hwnd, IDC_BLINKTRAY, BST_CHECKED);
					if(g_config.NotifyOnBlock!=Never)
						CheckDlgButton(hwnd, IDC_NOTIFYWINDOW, BST_CHECKED);
				}

				RECT rc;
				GetClientRect(hwnd, &rc);
				SendMessage(hwnd, WM_SIZE, 0, MAKELONG(rc.right-rc.left, rc.bottom-rc.top));
			} break;

            case WM_REFRESH_AUTOUPDATE: {
                SetDlgItemInt(hwnd, IDC_AUTOUPDATETIME, g_config.UpdateInterval, FALSE);
            } break;

			HANDLE_MSG(hwnd, WM_SIZE, SettingsProc_OnSize);
		}
		return 0;
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
