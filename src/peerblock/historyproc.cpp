/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2011 PeerBlock, LLC

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
using namespace sqlite3x;

#ifdef IDC_LIST
#undef IDC_LIST
#endif
#define IDC_LIST 100

static const UINT ID_LIST_ALLOWFOR15MIN=201;
static const UINT ID_LIST_ALLOWFORONEHOUR=202;
static const UINT ID_LIST_ALLOWPERM=203;
static const UINT ID_LIST_BLOCKFOR15MIN=204;
static const UINT ID_LIST_BLOCKFORONEHOUR=205;
static const UINT ID_LIST_BLOCKPERM=206;
static const UINT ID_LIST_COPYIP=207;

static const int MAX_SIZE_IP = 32;

struct HistoryRow {
	long long id;
	bool full;

	tstring time, name, source, dest;
	LPCTSTR protocol, action;
	enum { allowed, blocked, http } type;

	HistoryRow() : full(false) {}
};

static HWND g_list, g_calendar, g_search;
static WNDPROC g_tabproc;
static tstring g_allowed, g_blocked;

static boost::shared_array<HistoryRow> g_rows;
static size_t g_rowcount;

HWND g_hHistoryDlg = NULL;

// Function Prototypes
static INT_PTR History_OnNotify(HWND hwnd, int idCtrl, NMHDR *nmh);
static void History_OnSize(HWND hwnd, UINT state, int cx, int cy);
UINT CreateListViewPopUpMenu(HWND hwnd, NMHDR *nmh, NMITEMACTIVATE *nmia);

static void History_OnClose(HWND hwnd) {
	EndDialog(hwnd, 0);
}

static tstring FormatIp(unsigned int ip, unsigned short port) {
	const unsigned char *bytes=reinterpret_cast<const unsigned char*>(&ip);

	tstringstream ss;

	ss << (int)bytes[3] << _T('.') << (int)bytes[2] << _T('.') << (int)bytes[1] << _T('.') << (int)bytes[0];
	if(port!=0) ss << _T(':') << (int)port;

	return ss.str();
}

static LRESULT CALLBACK Tabs_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	NMHDR *nmh=(NMHDR*)lParam;
	try {
		if(msg==WM_NOTIFY && nmh->idFrom==IDC_LIST) {
			if(nmh->code==LVN_GETDISPINFO) {
				NMLVDISPINFO *di=(NMLVDISPINFO*)nmh;

				HistoryRow &r=g_rows[di->item.iItem];
				LPCTSTR str;

				if(!r.full) {
					sqlite3_lock lock(g_con);

					sqlite3_command cmd(g_con, "select time(time, 'localtime'),name,source,sourceport,destination,destport,protocol,action from t_history,t_names where t_history.ROWID=? and id=nameid order by t_history.ROWID desc;");
					cmd.bind(1, r.id);

					sqlite3_reader reader=cmd.executereader();
					if(reader.read()) {
						unsigned short srcport=(unsigned short)reader.getint(3);
						unsigned short destport=(unsigned short)reader.getint(5);

#ifdef _UNICODE
						r.time=reader.getstring16(0);
						r.name=reader.getstring16(1);
#else
						r.time=UTF8_MBS(reader.getstring(0));
						r.name=UTF8_MBS(reader.getstring(1));
#endif

						r.source=FormatIp((unsigned int)reader.getint(2), srcport);
						r.dest=FormatIp((unsigned int)reader.getint(4), destport);

						if(reader.getint(7)==1) {
							if(reader.getint(6)==IPPROTO_TCP && (destport==80 || destport==443))
								r.type=HistoryRow::http;
							else r.type=HistoryRow::blocked;
						}
						else r.type=HistoryRow::allowed;

						r.action=(r.type!=HistoryRow::allowed)?g_blocked.c_str():g_allowed.c_str();

						if(r.name==_T("n/a")) r.name.clear();

						switch(reader.getint(6)) {
							case IPPROTO_ICMP: r.protocol=_T("ICMP"); break;
							case IPPROTO_IGMP: r.protocol=_T("IGMP"); break;
							case IPPROTO_GGP: r.protocol=_T("Gateway^2"); break;
							case IPPROTO_TCP: r.protocol=_T("TCP"); break;
							case IPPROTO_PUP: r.protocol=_T("PUP"); break;
							case IPPROTO_UDP: r.protocol=_T("UDP"); break;
							case IPPROTO_IDP: r.protocol=_T("XNS IDP"); break;
							case IPPROTO_ND: r.protocol=_T("NetDisk"); break;
							default: r.protocol=_T("Unknown"); break;
						}

						r.full=true;
					}
				}

				switch(di->item.iSubItem) {
					case 0: str=r.time.c_str(); break;
					case 1: str=r.name.c_str(); break;
					case 2: str=r.source.c_str(); break;
					case 3: str=r.dest.c_str(); break;
					case 4: str=r.protocol; break;
					case 5: str=r.action; break;
					default: __assume(0);
				}

				di->item.pszText=(LPTSTR)str;
			}
			else if(nmh->code==LVN_ODCACHEHINT) {
				const NMLVCACHEHINT &ch=*((const NMLVCACHEHINT*)nmh);

				sqlite3_lock lock(g_con);

				sqlite3_command cmd(g_con, "select time(time, 'localtime'),name,source,sourceport,destination,destport,protocol,action from t_history,t_names where t_history.ROWID=? and id=nameid order by t_history.ROWID desc;");

				for(int i=ch.iFrom; i<=ch.iTo; i++) {
					HistoryRow &r=g_rows[i];
					if(r.full) continue;

					cmd.bind(1, r.id);

					sqlite3_reader reader=cmd.executereader();

					if(reader.read()) {
						unsigned short srcport=(unsigned short)reader.getint(3);
						unsigned short destport=(unsigned short)reader.getint(5);

#ifdef _UNICODE
						r.time=reader.getstring16(0);
						r.name=reader.getstring16(1);
#else
						r.time=UTF8_MBS(reader.getstring(0));
						r.name=UTF8_MBS(reader.getstring(1));
#endif

						r.source=FormatIp((unsigned int)reader.getint(2), srcport);
						r.dest=FormatIp((unsigned int)reader.getint(4), destport);

						if(reader.getint(7)==1) {
							if(reader.getint(6)==IPPROTO_TCP && (destport==80 || destport==443))
								r.type=HistoryRow::http;
							else r.type=HistoryRow::blocked;
						}
						else r.type=HistoryRow::allowed;

						r.action=(r.type!=HistoryRow::allowed)?g_blocked.c_str():g_allowed.c_str();

						if(r.name==_T("n/a")) r.name.clear();

						switch(reader.getint(6)) {
							case IPPROTO_ICMP: r.protocol=_T("ICMP"); break;
							case IPPROTO_IGMP: r.protocol=_T("IGMP"); break;
							case IPPROTO_GGP: r.protocol=_T("Gateway^2"); break;
							case IPPROTO_TCP: r.protocol=_T("TCP"); break;
							case IPPROTO_PUP: r.protocol=_T("PUP"); break;
							case IPPROTO_UDP: r.protocol=_T("UDP"); break;
							case IPPROTO_IDP: r.protocol=_T("XNS IDP"); break;
							case IPPROTO_ND: r.protocol=_T("NetDisk"); break;
							default: r.protocol=_T("Unknown"); break;
						}

						r.full=true;
					}
				}
			}
			else if(nmh->code==NM_CUSTOMDRAW && g_config.ColorCode) {
				NMLVCUSTOMDRAW *cd=(NMLVCUSTOMDRAW*)nmh;
				switch(cd->nmcd.dwDrawStage) {
					case CDDS_PREPAINT:
						return CDRF_NOTIFYITEMDRAW;
					case CDDS_ITEMPREPAINT: {
						const HistoryRow &row=g_rows[cd->nmcd.dwItemSpec];

						Color c;

						switch(row.type) {
							case HistoryRow::allowed:
								c=g_config.AllowedColor;
								break;
							case HistoryRow::blocked:
								c=g_config.BlockedColor;
								break;
							case HistoryRow::http:
								c=g_config.HttpColor;
								break;
							default: __assume(0);
						}

						cd->clrText=c.Text;
						cd->clrTextBk=c.Background;
					} return CDRF_NEWFONT;
					default:
						return CDRF_DODEFAULT;
				}
			}
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

	return CallWindowProc(g_tabproc, hwnd, msg, wParam, lParam);
}

static ostream& operator<<(ostream &os, const SYSTEMTIME &st) {
	os << setfill('0')
		<< setw(4) << st.wYear << '-'
		<< setw(2) << st.wMonth << '-'
		<< setw(2) << st.wDay;

	return os;
}

static void History_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
	switch(id) {
		case IDC_TODAY: {
			SYSTEMTIME st;
			GetLocalTime(&st);

			HWND caltabs=GetDlgItem(hwnd, IDC_CALTABS);

			if(TabCtrl_GetCurSel(caltabs)!=0) {
				TabCtrl_SetCurSel(caltabs, 0);

				SetWindowPos(g_calendar, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
				ShowWindow(g_search, SW_HIDE);
			}

			HistoryCalendar_SetCurSel(g_calendar, &st);
		} break;
		case IDCLOSE:
			EndDialog(hwnd, 0);
			break;
		case ID_FILE_EXPORTTO:
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY_EXPORT), hwnd, ExportHistory_DlgProc);
			break;
		case ID_FILE_CLEARDATABASE:
			if(MessageBox(hwnd, IDS_CLEARDBTEXT, IDS_CLEARDB, MB_ICONWARNING|MB_YESNO|MB_DEFBUTTON2)==IDYES) {
				ListView_DeleteAllItems(g_list);
				g_rows=boost::shared_array<HistoryRow>();

				try {
					sqlite3_lock lock(g_con, true);

					g_con.executenonquery("drop index i_actiontime;");
					g_con.executenonquery("drop index i_time;");
					g_con.executenonquery("drop table t_history;");
					g_con.executenonquery("drop table t_names;");

					g_con.executenonquery("create table t_names(id integer primary key, name text unique);");
					g_con.executenonquery("create table t_history(time real, nameid integer, source integer, sourceport integer, destination integer, destport integer, protocol integer, action integer);");
					g_con.executenonquery("create index i_time on t_history(time);");
					g_con.executenonquery("create index i_actiontime on t_history(action,time);");

					lock.commit();

					g_con.executenonquery("vacuum;");
				}
				catch(exception &ex) {
					ExceptionBox(hwnd, ex, __FILE__, __LINE__);
				}
			}
			break;
		case ID_FILE_EXIT:
			EndDialog(hwnd, 0);
			break;
	}
}

static void History_OnDestroy(HWND hwnd) {
	g_rows=boost::shared_array<HistoryRow>();
	SaveListColumns(g_list, g_config.HistoryColumns);
	SaveWindowPosition(hwnd, g_config.HistoryWindowPos);

	g_hHistoryDlg = NULL;
}

static void History_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) {
	RECT rc={0};
	rc.right=450;
	rc.bottom=260;

	MapDialogRect(hwnd, &rc);

	lpMinMaxInfo->ptMinTrackSize.x=rc.right;
	lpMinMaxInfo->ptMinTrackSize.y=rc.bottom;
}

static void InsertColumn(HWND hList, INT iSubItem, INT iWidth, UINT idText) {
	LVCOLUMN lvc={0};

	lvc.mask=LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
	lvc.fmt=LVCFMT_LEFT;
	lvc.cx=iWidth;
	lvc.iSubItem=iSubItem;

	tstring buf=LoadString(idText);
	lvc.pszText=(LPTSTR)buf.c_str();

	ListView_InsertColumn(hList, iSubItem, &lvc);
}

static BOOL History_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	g_hHistoryDlg = hwnd;

	HWND tabs=GetDlgItem(hwnd, IDC_TABS);

#pragma warning(disable:4244)
	g_tabproc=(WNDPROC)(LONG_PTR)SetWindowLongPtr(tabs, GWLP_WNDPROC, (LONG_PTR)Tabs_WndProc);
#pragma warning(default:4244)

	TCITEM tci={0};
	tci.mask=TCIF_TEXT;

	tstring name=LoadString(IDS_ALL);
	tci.pszText=(LPTSTR)name.c_str();
	TabCtrl_InsertItem(tabs, 0, &tci);

	name=LoadString(IDS_BLOCKED);
	tci.pszText=(LPTSTR)name.c_str();
	TabCtrl_InsertItem(tabs, 1, &tci);

	name=LoadString(IDS_ALLOWED);
	tci.pszText=(LPTSTR)name.c_str();
	TabCtrl_InsertItem(tabs, 2, &tci);

	HWND caltabs=GetDlgItem(hwnd, IDC_CALTABS);

	name=LoadString(IDS_CALENDAR);
	tci.pszText=(LPTSTR)name.c_str();
	TabCtrl_InsertItem(caltabs, 0, &tci);

	name=LoadString(IDS_SEARCH);
	tci.pszText=(LPTSTR)name.c_str();
	TabCtrl_InsertItem(caltabs, 1, &tci);

	g_list=CreateWindow(WC_LISTVIEW, NULL, WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_NOSORTHEADER|LVS_OWNERDATA,
		0, 0, 0, 0, tabs, (HMENU)IDC_LIST, GetModuleHandle(NULL), NULL);
	ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);

#ifndef _UNICODE
	ListView_SetUnicodeFormat(g_list, FALSE);
#endif

	InsertColumn(g_list, 0, g_config.HistoryColumns[0], IDS_TIME);
	InsertColumn(g_list, 1, g_config.HistoryColumns[1], IDS_RANGE);
	InsertColumn(g_list, 2, g_config.HistoryColumns[2], IDS_SOURCE);
	InsertColumn(g_list, 3, g_config.HistoryColumns[3], IDS_DESTINATION);
	InsertColumn(g_list, 4, g_config.HistoryColumns[4], IDS_PROTOCOL);
	InsertColumn(g_list, 5, g_config.HistoryColumns[5], IDS_ACTION);

	g_calendar=CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY_CALENDAR), caltabs, HistoryCalendar_DlgProc);
	g_search=CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY_FIND), caltabs, HistoryFind_DlgProc);

	SYSTEMTIME st;
	GetLocalTime(&st);
	HistoryCalendar_SetCurSel(g_calendar, &st);

	if(
		g_config.HistoryWindowPos.left!=0 || g_config.HistoryWindowPos.top!=0 ||
		g_config.HistoryWindowPos.right!=0 || g_config.HistoryWindowPos.bottom!=0
	) {
		SetWindowPos(hwnd, NULL,
			g_config.HistoryWindowPos.left,
			g_config.HistoryWindowPos.top,
			g_config.HistoryWindowPos.right-g_config.HistoryWindowPos.left,
			g_config.HistoryWindowPos.bottom-g_config.HistoryWindowPos.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER
		);
	}

	RECT rc;
	GetClientRect(hwnd, &rc);
	History_OnSize(hwnd, 0, rc.right-rc.left, rc.bottom-rc.top);

	// Load peerblock icon in the windows titlebar
	RefreshDialogIcon(hwnd);

	return TRUE;
}

static unsigned int ParseIp(LPCTSTR str) {
	unsigned int ipa, ipb, ipc, ipd;

	if(_stscanf(str, _T("%u.%u.%u.%u"), &ipa, &ipb, &ipc, &ipd)==4) {
		union {
			unsigned int ip;
			unsigned char bytes[4];
		};

		bytes[0]=ipd;
		bytes[1]=ipc;
		bytes[2]=ipb;
		bytes[3]=ipa;

		return ip;
	}

	return 0;
}

static void History_LoadData(HWND hwnd, const SYSTEMTIME &st) {
	ostringstream ss;

	ss << "select ROWID from t_history where ";

	int cursel=TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_TABS));
	switch(cursel) {
		case 1:
			ss << "action=1 and ";
			break;
		case 2:
			ss << "action=0 and ";
			break;
	}
	ss << "time>=julianday('" << st << "', 'utc') and time<julianday('" << st << "', 'utc', '+1 day') order by t_history.ROWID desc;";

	g_allowed=LoadString(IDS_ALLOWED);
	g_blocked=LoadString(IDS_BLOCKED);

	SendMessage(g_list, WM_SETREDRAW, FALSE, 0);

	ListView_DeleteAllItems(g_list);
	g_rows=boost::shared_array<HistoryRow>();

	vector<long long> rows;

	try {
		sqlite3_lock lock(g_con);

		sqlite3_command cmd(g_con, ss.str());
		sqlite3_reader reader=cmd.executereader();

		while(reader.read()) rows.push_back(reader.getint64(0));
	}
	catch(database_error &ex) {
		ExceptionBox(hwnd, ex, __FILE__, __LINE__);
		SendMessage(g_list, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(g_list, NULL, FALSE);
		return;
	}

	boost::shared_array<HistoryRow> nrows(new HistoryRow[rows.size()]);

	for(vector<long long>::size_type i=0; i<rows.size(); ++i)
		nrows[i].id=rows[i];

	g_rows=nrows;
	g_rowcount=rows.size();
	rows.clear();

	ListView_SetItemCount(g_list, g_rowcount);

	SendMessage(g_list, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(g_list, NULL, FALSE);
}

static void History_PerformSearch(HWND hwnd) {
	TCHAR buf[256];

	HFM_SEARCHINFO info={0};
	info.lpRange=buf;
	info.cRangeMax=256;

	SendMessage(g_search, HFM_GETSEARCH, 0, (LPARAM)&info);

	ostringstream ss;
	ss << "select t_history.ROWID from t_history";

	bool first=true;

	if((info.iMask&HFM_RANGE)) {
		first=false;
		ss << ",t_names where t_names.id=nameid";
	}

	if((info.iMask&HFM_ACTION)) {
		if(first) {
			first=false;
			ss << " where";
		}
		else ss << " and";

		ss << " action=" << info.iAction;
	}

	if((info.iMask&HFM_FROM)) {
		if(first) {
			first=false;
			ss << " where";
		}
		else ss << " and";

		ss << " time>=julianday('" << info.stFrom << "', 'utc')";
	}

	if((info.iMask&HFM_TO)) {
		if(first) {
			first=false;
			ss << " where";
		}
		else ss << " and";

		ss << " time<julianday('" << info.stTo << "', 'utc', '+1 day')";
	}

	if((info.iMask&HFM_PROTOCOL)) {
		if(first) {
			first=false;
			ss << " where";
		}
		else ss << " and";

		ss << " protocol=" << info.iProtocol;
	}

	if((info.iMask&HFM_SOURCE)) {
		if(first) {
			first=false;
			ss << " where";
		}
		else ss << " and";

		ss << " source=" << (int)info.uSource;
	}

	if((info.iMask&HFM_DEST)) {
		if(first) {
			first=false;
			ss << " where";
		}
		else ss << " and";

		ss << " destination=" << (int)info.uDest;
	}

	tstring name;
	if((info.iMask&HFM_RANGE)) {
		ss << " and name like ?";

		vector<tstring> s;
		boost::split(s, tstring(buf, info.cRangeMax), boost::is_space(), boost::token_compress_on);

		tostringstream ss;
		ss << _T('%');
		copy(s.begin(), s.end(), tostream_iterator(ss, _T("%")));

		name=ss.str();
	}

	ss << " order by t_history.ROWID desc;";

	SendMessage(g_list, WM_SETREDRAW, FALSE, 0);

	ListView_DeleteAllItems(g_list);
	g_rows=boost::shared_array<HistoryRow>();

	vector<long long> rows;

	try {
		sqlite3_lock lock(g_con);

		sqlite3_command cmd(g_con, ss.str());

		if((info.iMask&HFM_RANGE)) cmd.bind(1, name);

		sqlite3_reader reader=cmd.executereader();

		while(reader.read()) rows.push_back(reader.getint64(0));
	}
	catch(exception &ex) {
		ExceptionBox(hwnd, ex, __FILE__, __LINE__);
		return;
	}

	boost::shared_array<HistoryRow> nrows(new HistoryRow[rows.size()]);

	for(vector<long long>::size_type i=0; i<rows.size(); ++i)
		nrows[i].id=rows[i];

	g_rows=nrows;
	g_rowcount=rows.size();
	rows.clear();

	ListView_SetItemCount(g_list, g_rowcount);

	SendMessage(g_list, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(g_list, NULL, FALSE);
}

static INT_PTR History_OnNotify(HWND hwnd, int idCtrl, NMHDR *nmh) {
	if(nmh->code==HCN_SELCHANGE && nmh->hwndFrom==g_calendar) {
		int i=TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_TABS));
		if(i<3) History_LoadData(hwnd, ((const HCNM_SELCHANGE*)nmh)->stSelectedTime);
	}
	else if(nmh->code==HFN_SEARCH && nmh->hwndFrom==g_search) {
		HWND tabs=GetDlgItem(hwnd, IDC_TABS);

		if(TabCtrl_GetItemCount(tabs)==3) {
			TCITEM tci={0};
			tci.mask=TCIF_TEXT;

			const tstring name=LoadString(IDS_SEARCH);
			tci.pszText=(LPTSTR)name.c_str();
			TabCtrl_InsertItem(tabs, 3, &tci);
		}
		TabCtrl_SetCurSel(tabs, 3);

		History_PerformSearch(hwnd);
	}
	else if(nmh->code==TCN_SELCHANGE) {
		if(nmh->idFrom==IDC_TABS) {
			int i=TabCtrl_GetCurSel(nmh->hwndFrom);

			if(i<3) {
				SYSTEMTIME st;
				HistoryCalendar_GetCurSel(g_calendar, &st);
				History_LoadData(hwnd, st);
			}
			else if(i==3) History_PerformSearch(hwnd);
		}
		else if(nmh->idFrom==IDC_CALTABS) {
			int sel=TabCtrl_GetCurSel(nmh->hwndFrom);

			if(sel==0) {
				SetWindowPos(g_calendar, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
				ShowWindow(g_search, SW_HIDE);
			}
			else if(sel==1) {
				SetWindowPos(g_search, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
				ShowWindow(g_calendar, SW_HIDE);
			}
		}
	}
	else if(nmh->code==NM_RCLICK && nmh->idFrom==IDC_LIST) {
		NMITEMACTIVATE *nmia=(NMITEMACTIVATE*)nmh;

		if(nmia->iItem!=-1) {
			set<unsigned int> ips;
			GetLocalIps(ips, LOCALIP_ADAPTER);

			UINT ret = CreateListViewPopUpMenu(hwnd, nmh, nmia);
			if(ret != 0)
			{
				tstring sClipBoardData;
				int nListItemsProcessedCounter = 0;

				int nListViewItemPosition = ListView_GetNextItem(nmh->hwndFrom, -1, LVNI_SELECTED);
				while(nListViewItemPosition != -1)
				{
					TCHAR text[MAX_SIZE_IP], name[256];

					ListView_GetItemText(nmh->hwndFrom, nListViewItemPosition, 2, text, MAX_SIZE_IP);

					TCHAR *end=_tcschr(text, _T(':'));
					if(end) *end=_T('\0');

					unsigned int allowip=ParseIp(text);

					if(ips.find(allowip)!=ips.end()) {
						ListView_GetItemText(nmh->hwndFrom, nListViewItemPosition, 3, text, MAX_SIZE_IP);

						end=_tcschr(text, _T(':'));
						if(end) *end=_T('\0');

						allowip=ParseIp(text);
					}

					ListView_GetItemText(nmh->hwndFrom, nListViewItemPosition, 1, name, 256);

					wstring name_wchar=TSTRING_WCHAR(name);
					p2p::range r(name_wchar, allowip, allowip);

					++nListItemsProcessedCounter;

					// The appropriate action will be taken depending on which menu item was selected
					switch(ret) {
						case ID_LIST_ALLOWFOR15MIN:
							g_tempallow.insert(r);
							g_allowlist.push_back(make_pair(time(NULL)+900, r));
							break;
						case ID_LIST_ALLOWFORONEHOUR:
							g_tempallow.insert(r);
							g_allowlist.push_back(make_pair(time(NULL)+3600, r));
							break;
						case ID_LIST_ALLOWPERM: {
							path dir=path::base_dir()/_T("lists");

							if(!path::exists(dir))
								path::create_directory(dir);

							path file=dir/_T("permallow.p2b");

							p2p::list list;
							LoadList(file, list);

							list.insert(p2p::range(name_wchar, allowip, allowip));
							list.optimize();

							list.save(TSTRING_MBS(file.file_str()), p2p::list::file_p2b);

							bool found=false;

							for(vector<StaticList>::size_type i=0; i<g_config.StaticLists.size(); i++) {
								if(g_config.StaticLists[i].File==_T("lists\\permallow.p2b")) {
									found=true;
									break;
								}
							}

							if(!found) {
								StaticList list;
								list.Type=List::Allow;
								list.Description=LoadString(IDS_PERMALLOWS);
								list.File=_T("lists\\permallow.p2b");

								g_config.StaticLists.push_back(list);
							}

						} break;
						case ID_LIST_BLOCKFOR15MIN:
							g_tempblock.insert(r);
							g_blocklist.push_back(make_pair(time(NULL)+900, r));
							break;
						case ID_LIST_BLOCKFORONEHOUR:
							g_tempblock.insert(r);
							g_blocklist.push_back(make_pair(time(NULL)+3600, r));
							break;
						case ID_LIST_BLOCKPERM: {
							path dir=path::base_dir()/_T("lists");

							if(!path::exists(dir))
								path::create_directory(dir);

							path file=dir/_T("permblock.p2b");

							p2p::list list;
							LoadList(file, list);

							list.insert(p2p::range(name_wchar, allowip, allowip));
							list.optimize();

							list.save(TSTRING_MBS(file.file_str()), p2p::list::file_p2b);

							bool found=false;

							for(vector<StaticList>::size_type i=0; i<g_config.StaticLists.size(); i++) {
								if(g_config.StaticLists[i].File==_T("lists\\permblock.p2b")) {
									found=true;
									break;
								}
							}

							if(!found) {
								StaticList list;
								list.Type=List::Block;
								list.Description=LoadString(IDS_PERMBLOCKS);
								list.File=_T("lists\\permblock.p2b");

								g_config.StaticLists.push_back(list);
							}
						} break;
						case ID_LIST_COPYIP:
							if(OpenClipboard(hwnd)) {

								tstring sTempClipboardData(text);
								sTempClipboardData += _T("\r\n");

								if(nListItemsProcessedCounter < (int)ListView_GetSelectedCount(nmh->hwndFrom))
								{
									sClipBoardData += sTempClipboardData;
								}
								else
								{
									sClipBoardData += sTempClipboardData;

									size_t len = sClipBoardData.size()*sizeof(TCHAR);

									HGLOBAL buf=GlobalAlloc(GMEM_MOVEABLE, len);

									CopyMemory(GlobalLock(buf), sClipBoardData.c_str(), len);
									GlobalUnlock(buf);

									EmptyClipboard();

									#ifdef _UNICODE
										SetClipboardData(CF_UNICODETEXT, buf);
									#else
										SetClipboardData(CF_TEXT, buf);
									#endif
								}

								CloseClipboard();
							}
							break;
					} // End Switch

					// Obtain next selected item in list
					nListViewItemPosition = ListView_GetNextItem(nmh->hwndFrom, nListViewItemPosition, LVNI_SELECTED);

				} // End While
			} // End If which checks if a menu item was selected

			// Optimize refreshing of list (only do it once at the end)
			UINT menuItemSelected = ID_LIST_COPYIP;
			if(ret != 0 && ret != ID_LIST_COPYIP)
				LoadLists(hwnd);
		}
	}

	return 0;
}

static void FitTabChild(HWND tabs, HWND child) {
	RECT rc;
	GetClientRect(tabs, &rc);
	TabCtrl_AdjustRect(tabs, FALSE, &rc);

	MoveWindow(child, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, TRUE);
}

static void FitTabChild_Tight(HWND tabs, HWND child) {
	RECT rc, wrc;
	GetClientRect(tabs, &rc);
	GetWindowRect(tabs, &wrc);

	TabCtrl_AdjustRect(tabs, FALSE, &rc);

	MoveWindow(child, 0, rc.top-2, wrc.right-wrc.left, rc.bottom-rc.top+6, TRUE);
}

static void History_OnSize(HWND hwnd, UINT state, int cx, int cy) {
	HWND tabs=GetDlgItem(hwnd, IDC_TABS);
	HWND caltabs=GetDlgItem(hwnd, IDC_CALTABS);
	HWND today=GetDlgItem(hwnd, IDC_TODAY);
	HWND close=GetDlgItem(hwnd, IDCLOSE);

	RECT rc, buttonrc;
	GetWindowRect(g_search, &rc);
	GetWindowRect(today, &buttonrc);

	HDWP dwp=BeginDeferWindowPos(4);

	DeferWindowPos(dwp, tabs, NULL, 7, 7, cx-21-(rc.right-rc.left), cy-14, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, caltabs, NULL, cx-7-(rc.right-rc.left), 7, rc.right-rc.left, cy-21-(buttonrc.bottom-buttonrc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);

	DeferWindowPos(dwp, today, NULL, (cx-7-(((rc.right-rc.left)/4)*3))-((buttonrc.right-buttonrc.left)/2), cy-7-(buttonrc.bottom-buttonrc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, close, NULL, (cx-7-((rc.right-rc.left)/4))-((buttonrc.right-buttonrc.left)/2), cy-7-(buttonrc.bottom-buttonrc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	EndDeferWindowPos(dwp);

	FitTabChild(tabs, g_list);
	FitTabChild_Tight(caltabs, g_calendar);
	FitTabChild_Tight(caltabs, g_search);

	if(state==SIZE_RESTORED) {
		SaveWindowPosition(hwnd, g_config.HistoryWindowPos);
	}
}

//================================================================================================
//
//  CreateListViewPopUpMenu(HWND hwnd, NMHDR *nmh, NMITEMACTIVATE *nmia))
//
//    - Creates a popup menu to display Allow/Block options
//
/// <summary>
///   Out = Obtain id of selected menu item
/// </summary>
//
UINT CreateListViewPopUpMenu(HWND hwnd, NMHDR *nmh, NMITEMACTIVATE *nmia)
{
	UINT ret = 0;

	// Obtain current ip
	set<unsigned int> ips;
	GetLocalIps(ips, LOCALIP_ADAPTER);

	// Obtain currently selected Cell
	int nListViewItemPosition = ListView_GetNextItem(nmh->hwndFrom, -1, LVNI_SELECTED);

	if(nListViewItemPosition != -1)
	{
		// Obtain the correct selected list item ip
		TCHAR text[MAX_SIZE_IP];

		// Display custom message if multiple items are selected
		if(ListView_GetSelectedCount(nmh->hwndFrom) == 1)
		{
			ListView_GetItemText(nmh->hwndFrom, nListViewItemPosition, 2, text, MAX_SIZE_IP);

			TCHAR *end=_tcschr(text, _T(':'));
			if(end) *end=_T('\0');

			unsigned int allowip=ParseIp(text);
			if(ips.find(allowip)!=ips.end()) {
				ListView_GetItemText(nmh->hwndFrom, nListViewItemPosition, 3, text, MAX_SIZE_IP);

				end=_tcschr(text, _T(':'));
				if(end) *end=_T('\0');

				allowip=ParseIp(text);
			}
		}
		else
		{
			TCHAR *pText = text;
			_tcscpy_s(pText, MAX_SIZE_IP, _T("multiple IPs\0"));
		}

		// Create Menu
		HMENU menu=CreatePopupMenu();

		tstring str=boost::str(tformat(LoadString(IDS_ALLOWXFOR15MIN))%text);
		AppendMenu(menu, MF_STRING, ID_LIST_ALLOWFOR15MIN, str.c_str());

		str=boost::str(tformat(LoadString(IDS_ALLOWXFORONEHOUR))%text);
		AppendMenu(menu, MF_STRING, ID_LIST_ALLOWFORONEHOUR, str.c_str());

		str=boost::str(tformat(LoadString(IDS_ALLOWXPERM))%text);
		AppendMenu(menu, MF_STRING, ID_LIST_ALLOWPERM, str.c_str());

		AppendMenu(menu, MF_SEPARATOR, 0, NULL);

		str=boost::str(tformat(LoadString(IDS_BLOCKXFOR15MIN))%text);
		AppendMenu(menu, MF_STRING, ID_LIST_BLOCKFOR15MIN, str.c_str());

		str=boost::str(tformat(LoadString(IDS_BLOCKXFORONEHOUR))%text);
		AppendMenu(menu, MF_STRING, ID_LIST_BLOCKFORONEHOUR, str.c_str());

		str=boost::str(tformat(LoadString(IDS_BLOCKXPERM))%text);
		AppendMenu(menu, MF_STRING, ID_LIST_BLOCKPERM, str.c_str());

		AppendMenu(menu, MF_SEPARATOR, 0, NULL);

		str=boost::str(tformat(LoadString(IDS_COPYXTOCLIPBOARD))%text);
		AppendMenu(menu, MF_STRING, ID_LIST_COPYIP, str.c_str());

		RECT rect;
		GetWindowRect(nmh->hwndFrom, &rect);

		SetForegroundWindow(hwnd);

		// Obtain menu item which was selected
		ret=TrackPopupMenuEx(menu, TPM_TOPALIGN|TPM_RETURNCMD, rect.left+nmia->ptAction.x, rect.top+nmia->ptAction.y, hwnd, NULL);

		DestroyMenu(menu);
	}

	return ret;
}

INT_PTR CALLBACK History_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_CLOSE, History_OnClose);
			HANDLE_MSG(hwnd, WM_COMMAND, History_OnCommand);
			HANDLE_MSG(hwnd, WM_DESTROY, History_OnDestroy);
			HANDLE_MSG(hwnd, WM_GETMINMAXINFO, History_OnGetMinMaxInfo);
			HANDLE_MSG(hwnd, WM_INITDIALOG, History_OnInitDialog);
			HANDLE_MSG(hwnd, WM_NOTIFY, History_OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, History_OnSize);
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
