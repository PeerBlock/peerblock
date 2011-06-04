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

#include "tracelog.h"
extern TraceLog g_tlog;


static const UINT TIMER_TEMPALLOW=2;
static const UINT TIMER_COMMITLOG=3;
static const UINT ID_LIST_ALLOWFOR15MIN=201;
static const UINT ID_LIST_ALLOWFORONEHOUR=202;
static const UINT ID_LIST_ALLOWPERM=203;
static const UINT ID_LIST_BLOCKFOR15MIN=204;
static const UINT ID_LIST_BLOCKFORONEHOUR=205;
static const UINT ID_LIST_BLOCKPERM=206;
static const UINT ID_LIST_COPYIP=207;

static const int MAX_SIZE_IP = 32;

allowlist_t g_allowlist, g_blocklist;
threaded_sqlite3_connection g_con;
int g_numlogged = 0;	// used to make sure we log at least a few allowed-packets before stopping, so that the user sees some activity
DWORD g_lastblocktime;	// tracks time we last blocked something, for exit warning purposes, in msec resolution
mutex g_lastblocklock;	// lock to protect g_lastblocktime accesses
mutex g_lastupdatelock;	// lock to protect against checking g_config.LastUpdate while we're in the process of updating


class LogFilterAction {
private:
	struct Action {
		time_t Time;
		std::string Name;
		unsigned int SourceIp, DestIp;
		unsigned short SourcePort, DestPort;
		int Protocol;
		bool Blocked;
	};

	tstring allowed, blocked;

	HWND hwnd,log;
	mutex mutex;
	bool usedb;

	static tstring format_ipport(const sockaddr &addr) {
		TRACEV("[LogFilterAction] [format_ipport]  > Entering routine.");
		TCHAR buf[256];
		DWORD buflen = 256;

		WSAAddressToString((sockaddr*)&addr, (addr.sa_family == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6), 0, buf, &buflen);

		return buf;
	}

	// should really put this in a utility class instead of copy/paste
	static string format_ipport(unsigned int ip, unsigned short port) {
		const unsigned char *bytes=reinterpret_cast<const unsigned char*>(&ip);

		char buf[24];
		StringCbPrintfA(buf, sizeof(buf), port?"%u.%u.%u.%u:%u":"%u.%u.%u.%u", bytes[3], bytes[2], bytes[1], bytes[0], port);

		return buf;
	}

	static tstring current_time() {
		TRACEV("[LogFilterAction] [current_time]  > Entering routine.");
		time_t t=time(NULL);
		TCHAR buf[9];

		_tcsftime(buf, 9, _T("%H:%M:%S"), localtime(&t));

		return buf;
	}

	static tstring current_datetime() {
		TRACEV("[LogFilterAction] [current_datetime]  > Entering routine.");
		time_t t=time(NULL);
		TCHAR buf[21];

		_tcsftime(buf, 21, _T("%m/%d/%Y %H:%M:%S"), localtime(&t));

		return buf;
	}

	std::queue<Action> dbqueue;

public:
	LogFilterAction(HWND hwnd, HWND l, bool db=true) : hwnd(hwnd),log(l),usedb(db)
	{
		TRACEV("[LogFilterAction] [LogFilterAction]  > Entering routine.");

		TCHAR chBuf[256];
		_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[LogFilterAction] [LogFilterAction]    hwnd:[%p] log:[%p] usedb:[%d]"), hwnd, log, usedb);
		g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_VERBOSE);

		allowed=LoadString(IDS_ALLOWED);
		blocked=LoadString(IDS_BLOCKED);

		TRACEV("[LogFilterAction] [LogFilterAction]  < Leaving routine.");
	}

	~LogFilterAction()
	{
		TRACEV("[LogFilterAction] [~LogFilterAction]  > Entering routine.");
		this->Commit(true);
		TRACEV("[LogFilterAction] [~LogFilterAction]  < Leaving routine.");
	}



	void operator()(const pbfilter::action &action)
	{
		TRACEV("[LogFilterAction] [operator()]  > Entering routine.");
		unsigned int sourceip, destip;
		unsigned int destport;

		if(action.src.addr.sa_family == AF_INET) {
			TRACEV("[LogFilterAction] [operator()]    src AF_INET");
			sourceip = htonl(action.src.addr4.sin_addr.s_addr);
		}
		else {
			TRACEV("[LogFilterAction] [operator()]    src NOT AF_INET");
			sourceip = 0;
		}

		if(action.dest.addr.sa_family == AF_INET) {
			TRACEV("[LogFilterAction] [operator()]    dest AF_INET");
			destip = htonl(action.dest.addr4.sin_addr.s_addr);
			destport = htons(action.dest.addr4.sin_port);
		}
		else {
			TRACEV("[LogFilterAction] [operator()]    dest NOT AF_INET");
			destip = 0;
			destport = htons(action.dest.addr6.sin6_port);
		}

		if((action.type == pbfilter::action::blocked || g_config.LogAllowed || g_config.ShowAllowed || g_numlogged < 10) && (sourceip!=INADDR_LOOPBACK || destip!=INADDR_LOOPBACK))
		{
			TRACEV("[LogFilterAction] [operator()]    allowed, not loopbacks");
			if(action.type == pbfilter::action::blocked && g_config.BlinkOnBlock!=Never && (g_config.BlinkOnBlock==OnBlock || ((destport==80 || destport==443) && action.protocol==IPPROTO_TCP)))
			{
				TRACEV("[LogFilterAction] [operator()]    start blinking");
				g_blinkstart=GetTickCount();
			}

			if(g_config.ShowAllowed || action.type == pbfilter::action::blocked || g_numlogged < 10)
			{
				TCHAR chBuf[256];
				_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[LogFilterAction] [operator()]    updating list for window log:[%p]"), log);
				g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_VERBOSE);

				int count=ListView_GetItemCount(log);

				_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[LogFilterAction] [operator()]    log:[%p], cnt:[%d], lsz:[%d]"), log, count, g_config.LogSize);
				g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_VERBOSE);

				while(count-- >= (int)g_config.LogSize) ListView_DeleteItem(log, g_config.LogSize-1);

				if(g_config.LogSize>0) {
					TRACEV("[LogFilterAction] [operator()]    logsize > 0");
					const tstring name=(action.label!=_T("n/a"))?action.label:_T("");
					const tstring source=format_ipport(action.src.addr);
					const tstring dest=format_ipport(action.dest.addr);
					const tstring time=current_time();
					const tstring &actionstr=(action.type == pbfilter::action::blocked) ? blocked : allowed;

					LVITEM lvi={0};
					lvi.mask=LVIF_TEXT|LVIF_PARAM;

					lvi.iSubItem=0;
					lvi.pszText=(LPTSTR)time.c_str();

					if(action.type == pbfilter::action::blocked)
					{
						TRACEV("[LogFilterAction] [operator()]    logging blocked packet");

						if(action.protocol==IPPROTO_TCP && (destport==80 || destport==443))
						{
							lvi.lParam=(LPARAM)2;	// HTTP block
						}
						else
						{
							// non-HTTP block
							lvi.lParam=(LPARAM)1;

							// Update "last block time", so we can warn the user when exiting.
							mutex::scoped_lock lock(g_lastblocklock);
							g_lastblocktime = GetTickCount();
						}
					}
					else
					{
						TRACEV("[LogFilterAction] [operator()]    logging allowed packet");
						if (g_numlogged < 20) ++g_numlogged;
						lvi.lParam=(LPARAM)0;
					}

					ListView_InsertItem(log, &lvi);

					lvi.mask=LVIF_TEXT;
					lvi.iSubItem=1;
					lvi.pszText=(LPTSTR)name.c_str();
					ListView_SetItem(log, &lvi);

					lvi.iSubItem=2;
					lvi.pszText=(LPTSTR)source.c_str();
					ListView_SetItem(log, &lvi);

					lvi.iSubItem=3;
					lvi.pszText=(LPTSTR)dest.c_str();
					ListView_SetItem(log, &lvi);

					lvi.iSubItem=4;
					switch(action.protocol) {
						case IPPROTO_ICMP: lvi.pszText=_T("ICMP"); break;
						case IPPROTO_IGMP: lvi.pszText=_T("IGMP"); break;
						case IPPROTO_GGP: lvi.pszText=_T("Gateway^2"); break;
						case IPPROTO_TCP: lvi.pszText=_T("TCP"); break;
						case IPPROTO_PUP: lvi.pszText=_T("PUP"); break;
						case IPPROTO_UDP: lvi.pszText=_T("UDP"); break;
						case IPPROTO_IDP: lvi.pszText=_T("XNS IDP"); break;
						case IPPROTO_ND: lvi.pszText=_T("NetDisk"); break;
						default: lvi.pszText=_T("Unknown"); break;
					}
					ListView_SetItem(log, &lvi);

					lvi.iSubItem=5;
					lvi.pszText=(LPTSTR)actionstr.c_str();
					ListView_SetItem(log, &lvi);

					// We should log at least a few Allowed messages even if ShowAllowed is false, so that the user
					// sees at least some activity and doesn't think that we're broken.
					if (g_numlogged == 9 && !g_config.ShowAllowed)
					{
						TRACEI("[LogFilterAction] [operator()]    Stopping default Allowed Packet logging");
						++g_numlogged;

						LVITEM lvi2={0};
						lvi2.lParam=(LPARAM)1;
						lvi2.mask=LVIF_TEXT|LVIF_PARAM;
						lvi2.iSubItem=0;
						lvi2.pszText=(LPTSTR)time.c_str();
						ListView_InsertItem(log, &lvi2);

						lvi2.mask=LVIF_TEXT;
						lvi2.iSubItem=1;
						lvi2.pszText=_T("Stopped displaying allowed packets, for performance reasons.  Re-enable by selecting 'Show allowed connections' option to override.");;
						ListView_SetItem(log, &lvi2);
					}
				}
			}

			if(usedb && action.src.addr.sa_family == AF_INET && action.dest.addr.sa_family == AF_INET)
			{
				TRACEV("[LogFilterAction] [operator()]    using db, src and dest not af_inet");
				if((action.type != pbfilter::action::blocked && g_config.LogAllowed) || (action.type == pbfilter::action::blocked && g_config.LogBlocked))
				{
					TRACEV("[LogFilterAction] [operator()]    logging to dbqueue");
					LogFilterAction::Action a;
					::time(&a.Time);
					a.Name=TSTRING_UTF8(action.label);
					a.SourceIp=htonl(action.src.addr4.sin_addr.s_addr);
					a.SourcePort=htons(action.src.addr4.sin_port);
					a.DestIp=htonl(action.dest.addr4.sin_addr.s_addr);
					a.DestPort=htons(action.dest.addr4.sin_port);
					a.Protocol=action.protocol;
					a.Blocked=action.type == pbfilter::action::blocked;

					mutex::scoped_lock lock(this->mutex);
					this->dbqueue.push(a);
				}
			}
		}
		TRACEV("[LogFilterAction] [operator()]  < Leaving routine.");

	} // End of operator()


private:
	void _Commit(bool force) {
		mutex::scoped_lock lock(this->mutex);
		static unsigned int currentinserted = g_config.HistoryCheckInterval;
		bool needvacuum = false;

		if(!this->dbqueue.empty()) {
			sqlite3_try_lock lock(g_con, true);
			if(force && !lock.is_locked()) lock.enter();

			if(lock.is_locked()) {
				sqlite3_command cmd_getnameid(g_con, "select id from t_names where name=?;");
				sqlite3_command cmd_setnameid(g_con, "insert into t_names(name) values(?);");
				sqlite3_command cmd_history(g_con, "insert into t_history values(julianday(?, 'unixepoch'),?,?,?,?,?,?,?);");

				for(; !this->dbqueue.empty(); this->dbqueue.pop()) {
					const Action &a=this->dbqueue.front();

					try {
						cmd_getnameid.bind(1, a.Name);
						sqlite3_reader reader=cmd_getnameid.executereader();

						long long nameid;

						if(reader.read()) {
							nameid=reader.getint64(0);
							reader.close();
						}
						else {
							reader.close();

							cmd_setnameid.bind(1, a.Name);
							cmd_setnameid.executenonquery();

							nameid=g_con.insertid();
						}

						cmd_history.bind(1, (int)a.Time);
						cmd_history.bind(2, nameid);
						cmd_history.bind(3, (int)a.SourceIp);
						cmd_history.bind(4, (int)a.SourcePort);
						cmd_history.bind(5, (int)a.DestIp);
						cmd_history.bind(6, (int)a.DestPort);
						cmd_history.bind(7, a.Protocol);
						cmd_history.bind(8, a.Blocked?1:0);
						cmd_history.executenonquery();

						currentinserted++;
					}
					catch(exception &ex) {
						ExceptionBox(NULL, ex, __FILE__, __LINE__);
						return;
					}
				}

				// limit the historydb size
				if ((g_config.LogBlocked || g_config.LogAllowed) && g_config.CleanupType != None && g_config.MaxHistorySize > 0)
				{
					if (currentinserted >= g_config.HistoryCheckInterval)
					{
						TRACEI("checking _tstat64");

						// http://msdn.microsoft.com/en-us/library/14h5k7ff(VS.71).aspx
						struct _stat64 fileStat;
						int ret = _tstat64( (path::base_dir()/_T("history.db")).c_str(), &fileStat );

						if (ret == 0)
						{
							TRACEI("checking filesize against user");

							if (fileStat.st_size > g_config.MaxHistorySize)
							{
								TRACEI("filesize is bigger than user specified");

								// centre record to start truncating from
								__int64 midrowid = 0;
								{
									ostringstream s;
									s << "select max(ROWID) - ((max(ROWID) - min(ROWID)) / 2) from t_history;";
									sqlite3_command cmd(g_con, s.str());
									sqlite3_reader reader = cmd.executereader();

									if (reader.read())
									{
										midrowid = reader.getint64(0);
									}
								}

								if (g_config.CleanupType == ArchiveDelete)
								{
									try
									{
										queue<string> dates;

										{
											ostringstream ss;
											ss << "select distinct date(time, 'localtime') from t_history where ROWID < " << midrowid << ";";

											sqlite3_command cmd(g_con, ss.str());
											sqlite3_reader reader=cmd.executereader();
											while(reader.read())
												dates.push(reader.getstring(0));
										}

										for(; !dates.empty(); dates.pop())
										{
											TRACEI("getting dates");
											path p=g_config.ArchivePath;

											if(!p.has_root()) p=path::base_dir()/p;
											if(!path::exists(p)) path::create_directory(p);

											p/=(UTF8_TSTRING(dates.front())+_T(".log"));

											FILE *fp=_tfopen(p.file_str().c_str(), _T("a"));

											if(fp) {
												boost::shared_ptr<FILE> safefp(fp, fclose);

												ostringstream sel;
												sel << "select time(time, 'localtime'),name,source,sourceport,destination,destport,protocol,action from t_history,t_names where time>=julianday('"+dates.front()+"', 'utc') and time<julianday('"+dates.front()+"', '+1 days', 'utc') and t_history.ROWID<" << midrowid << " and t_names.id=nameid;";
												sqlite3_command cmd(g_con, sel.str());

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
										} // for(!dates.empty())

										TRACEI("Cleaned up db");
									}
									catch(exception &ex) {
										ExceptionBox(NULL, ex, __FILE__, __LINE__);
									}
								} // g_config.CleanupType == ArchiveDelete

								if (g_config.CleanupType == Delete || g_config.CleanupType == ArchiveDelete)
								{
									try
									{
										ostringstream ss;

										ss << "delete from t_history where ROWID < " << midrowid << ";";
										g_con.executenonquery(ss.str());

										TRACEI("removed from history");

										// stop it growing over computing size
										// normal is 1000 = 1k
										// computing is 1024 = 1k
										if (fileStat.st_size > g_config.MaxHistorySize * 1.024)
										{
											TRACEI("need vacuum");
											needvacuum = true;
										}
									}
									catch(database_error &ex)
									{
										TRACEE("[LogFilterAction] [_Commit]  * ERROR:  Caught database_error exception while deleting files!!");
										ExceptionBox(NULL, ex, __FILE__, __LINE__);
									}
									catch(exception &ex) {
										TRACEE("[LogFilterAction] [_Commit]  * ERROR:  Caught exception while deleting files!!");
										ExceptionBox(NULL, ex, __FILE__, __LINE__);
									}
								} // g_config.CleanupType == Delete || g_config.CleanupType == ArchiveDelete
							} // fileStat.st_size > g_config.MaxHistoryDBSize
						}
						else
						{
							TCHAR buf[255];
							swprintf_s(buf, sizeof(buf), L"_tstat64 failed: %d", GetLastError());
							TRACEBUFI(buf);
						}

						currentinserted = 0;
					} // currentinserted >= 100
				}

				lock.commit();

				// crashes at the place where needvacuum = true; is called with or without g_con.commit() so put down here
				if (needvacuum)
				{
					g_con.executenonquery(_T("vacuum;"));
					TRACEI("vacuumed history.db as it was too big");
				}
			}
		}
	}

public:
	void Commit(bool force=false) {
		try {
			_Commit(force);
		}
		catch(exception &ex) {
			UncaughtExceptionBox(NULL, ex, __FILE__, __LINE__);
		}
		catch(...) {
			UncaughtExceptionBox(NULL, __FILE__, __LINE__);
		}
	}
};

static boost::shared_ptr<LogFilterAction> g_log;

UINT CreateListViewPopUpMenu(HWND hwnd, NMHDR *nmh, NMITEMACTIVATE *nmia, LVITEM &lvi);



//================================================================================================
//
//  UpdateStatus()
//
//    - Called by Log_OnCommand() after user clicks Update button and lists get updated
//    - Called by Log_OnInitDialog() near the end of setting everything up
//    - Called by Log_OnTimer() on timer expiry, after updating lists
//    - Called by Log_DlgProc() upon receipt of WM_LOG_HOOK or WM_LOG_RANGES messages
//
/// <summary>
///   Updates the main PeerBlock window's "metrics", with number blocks, time of last update, etc.
/// </summary>
/// <param name="hwnd">
///   Parent's hwnd, if any.
/// </param>
//
static void UpdateStatus(HWND hwnd)
{
	TRACEV("[LogProc] [UpdateStatus]  > Entering routine.");
	tstring enable, http, blocking, httpstatus, update, lastupdate;

	enable=LoadString(g_config.Block?IDS_DISABLE:IDS_ENABLE);
	http=LoadString(g_config.PortSet.IsHttpBlocked()?IDS_ALLOWHTTP:IDS_BLOCKHTTP);

	if(g_config.Block)
	{
		// Use locale-specific number grouping
		std::ostringstream numblocked;
		numblocked.imbue(std::locale(""));
		numblocked << g_filter->blockcount();
		blocking=boost::str(tformat(LoadString(IDS_PBACTIVE)) % numblocked.str().c_str());
	}
	else
	{
		blocking=LoadString(IDS_PBDISABLED);
	}

	httpstatus=boost::str(tformat(LoadString(IDS_HTTPIS)) % LoadString(g_config.PortSet.IsHttpBlocked()?IDS_BLOCKED:IDS_ALLOWED));

	{
		unsigned int lists=(unsigned int)(g_config.StaticLists.size()+g_config.DynamicLists.size());
		unsigned int uptodate=(unsigned int)g_config.StaticLists.size();
		unsigned int failed=0;
		unsigned int disabled=0;

		TRACEV("[LogProc] [UpdateStatus]    counting num disabled static lists");
		for(vector<StaticList>::size_type i=0; i<g_config.StaticLists.size(); i++)
			if(!g_config.StaticLists[i].Enabled) disabled++;

		TRACEV("[LogProc] [UpdateStatus]    counting dynamic lists");
		for(vector<DynamicList>::size_type i=0; i<g_config.DynamicLists.size(); i++) {
			bool exists=path::exists(g_config.DynamicLists[i].File());

			if(exists && time(NULL)-g_config.DynamicLists[i].LastUpdate < 604800)
				uptodate++;
			if(!exists || !g_config.DynamicLists[i].Enabled) disabled++;
			if(g_config.DynamicLists[i].FailedUpdate) failed++;
		}

		update=boost::str(tformat(LoadString(IDS_UPDATESTATUS)) % lists % uptodate % failed % disabled);
		TRACEV("[LogProc] [UpdateStatus]    done generating list metrics");
	}

	if(g_config.LastUpdate)
	{
		TRACEV("[LogProc] [UpdateStatus]    g_config.LastUpdate: [true]");
		mutex::scoped_lock lock(g_lastupdatelock);
		time_t dur=time(NULL)-g_config.LastUpdate;

		if(dur<604800)
		{
			TRACEV("[LogProc] [UpdateStatus]    dur < 604800");
			TCHAR buf[64];
			_tcsftime(buf, 64, _T("%#x"), localtime(&g_config.LastUpdate));

			lastupdate=boost::str(tformat(LoadString(IDS_LISTSUPTODATE))%buf);
		}
		else {
			TRACEV("[LogProc] [UpdateStatus]    didn't update lists");
			lastupdate=boost::str(tformat(LoadString(IDS_LISTSNOTUPTODATE))%(dur/86400));
		}
	}
	else {
		TRACEI("[LogProc] [UpdateStatus]    g_config.LastUpdate: [false]");
		lastupdate=LoadString(IDS_LISTSNOTUPDATED);
	}

	SetWindowText(GetDlgItem(hwnd, IDC_ENABLE), enable.c_str());
	SetWindowText(GetDlgItem(hwnd, IDC_HTTP), http.c_str());
	SetWindowText(GetDlgItem(hwnd, IDC_ENABLED_STATUS), blocking.c_str());
	SetWindowText(GetDlgItem(hwnd, IDC_HTTP_STATUS), httpstatus.c_str());
	SetWindowText(GetDlgItem(hwnd, IDC_UPDATE_STATUS), update.c_str());
	SetWindowText(GetDlgItem(hwnd, IDC_LAST_UPDATE), lastupdate.c_str());

	TRACEV("[LogProc] [UpdateStatus]  < Leaving routine.");

} // End of UpdateStatus()



static void Log_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) {

		case IDC_LISTS:
		{
			TRACEI("[LogProc] [Log_OnCommand]    user clicked List Manager button");
			INT_PTR ret=DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LISTS), hwnd, Lists_DlgProc);
			if(ret&LISTS_NEEDUPDATE)
			{
				{
					TRACEI("[LogProc] [Log_OnCommand]    lists need to be updated");
					mutex::scoped_lock lock(g_lastupdatelock);
					UpdateLists(hwnd);
				}

				SendMessage(hwnd, WM_TIMER, TIMER_UPDATE, 0);
				g_config.Save();
			}
			if(g_filter.get() && ret&LISTS_NEEDRELOAD) LoadLists(hwnd);
		} break;

		case IDC_UPDATE:
		{
			TRACEI("[LogProc] [Log_OnCommand]    user clicked Update button");
			int ret = 0;

			{
				mutex::scoped_lock lock(g_lastupdatelock);
				ret=UpdateLists(hwnd);
			}

			g_config.Save();

			if(g_filter.get()) {
				if(ret>0) LoadLists(hwnd);
				UpdateStatus(hwnd);
			}
		} break;

		case IDC_HISTORY:
			TRACEI("[LogProc] [Log_OnCommand]    user clicked View History button");
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY), hwnd, History_DlgProc);
			break;

		case IDC_CLEAR:
			TRACEI("[LogProc] [Log_OnCommand]    user clicked Clear Log button");
			ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_LIST));
			break;

		case IDC_ENABLE:
			TRACEI("[LogProc] [Log_OnCommand]    user clicked Enable/Disable button");
			SetBlock(!g_config.Block);
			break;

		case IDC_HTTP:
			TRACEI("[LogProc] [Log_OnCommand]    Block/Enable HTTP button");
			SetBlockHttp(!g_config.PortSet.IsHttpBlocked());
			break;
	}
}



static void Log_OnDestroy(HWND hwnd)
{
	TRACEI("[LogProc] [Log_OnDestroy]  > Entering routine.");
	HWND list=GetDlgItem(hwnd, IDC_LIST);

	SaveListColumns(list, g_config.LogColumns);

	TRACEI("[LogProc] [Log_OnDestroy]    saving config");
	g_config.Save();

	TRACEI("[LogProc] [Log_OnDestroy]    setting filter action-function to nothing");
	g_filter->setactionfunc();
	TRACEI("[LogProc] [Log_OnInitDialog]    setting g_log to empty LogFilterAction");
	g_log=boost::shared_ptr<LogFilterAction>();

	TRACEI("[LogProc] [Log_OnDestroy]  < Leaving routine.");

} // End of Log_OnDestroy()



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



static BOOL Log_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	TRACEV("[LogProc] [Log_OnInitDialog]  > Entering routine.");

	HWND list=GetDlgItem(hwnd, IDC_LIST);
	ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);

	InsertColumn(list, 0, g_config.LogColumns[0], IDS_TIME);
	InsertColumn(list, 1, g_config.LogColumns[1], IDS_RANGE);
	InsertColumn(list, 2, g_config.LogColumns[2], IDS_SOURCE);
	InsertColumn(list, 3, g_config.LogColumns[3], IDS_DESTINATION);
	InsertColumn(list, 4, g_config.LogColumns[4], IDS_PROTOCOL);
	InsertColumn(list, 5, g_config.LogColumns[5], IDS_ACTION);

	{
		try {
			g_con.open((path::base_dir()/_T("history.db")).c_str());
			g_con.setbusytimeout(5000);
			g_con.executenonquery("pragma page_size=4096;");

			{
				TRACEV("[LogProc] [Log_OnInitDialog]    acquiring sqlite3 lock");
				sqlite3_lock lock(g_con, true);
				TRACEV("[LogProc] [Log_OnInitDialog]    acquired sqlite3 lock");

				if(g_con.executeint("select count(*) from sqlite_master where name='t_names';")==0)
					g_con.executenonquery("create table t_names(id integer primary key, name text unique);");

				if(g_con.executeint("select count(*) from sqlite_master where name='t_history';")==0)
					g_con.executenonquery("create table t_history(time real, nameid integer, source integer, sourceport integer, destination integer, destport integer, protocol integer, action integer);");

				if(g_con.executeint("select count(*) from sqlite_master where name='i_time';")==0)
					g_con.executenonquery("create index i_time on t_history(time);");

				if(g_con.executeint("select count(*) from sqlite_master where name='i_actiontime';")==0)
					g_con.executenonquery("create index i_actiontime on t_history(action,time);");

				TRACEV("[LogProc] [Log_OnInitDialog]    committing sqlite3 lock");
				lock.commit();
				TRACEV("[LogProc] [Log_OnInitDialog]    committed sqlite3 lock");
			}
			TRACEV("[LogProc] [Log_OnInitDialog]    setting g_log to new LogFilterAction");
			g_log=boost::shared_ptr<LogFilterAction>(new LogFilterAction(hwnd, list));
			SetTimer(hwnd, TIMER_COMMITLOG, 15000, NULL);
		}
		catch(database_error&) {
			TRACEE("[LogProc] [Log_OnInitDialog]    ERROR:  Caught database_error");
			MessageBox(hwnd, IDS_HISTORYOPEN, IDS_HISTORYERR, MB_ICONERROR|MB_OK);
			EnableWindow(GetDlgItem(hwnd, IDC_HISTORY), FALSE);
			g_log=boost::shared_ptr<LogFilterAction>(new LogFilterAction(hwnd, list, false));
		}
	}

	g_filter->setactionfunc(boost::ref(*g_log.get()));

	TRACEI("[LogProc] [Log_OnInitDialog]    updating status");
	UpdateStatus(hwnd);

	SetTimer(hwnd, TIMER_UPDATE, 30000, NULL);
	SetTimer(hwnd, TIMER_TEMPALLOW, 30000, NULL);

	TRACEV("[LogProc] [Log_OnInitDialog]  < Leaving routine.");
	return TRUE;

} // End of Log_OnInitDialog()



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



static INT_PTR Log_OnNotify(HWND hwnd, int idCtrl, NMHDR *nmh)
{
	TRACED("[LogProc] [Log_OnNotify]  > Entering routine.");
	if(nmh->code==NM_CUSTOMDRAW && nmh->idFrom==IDC_LIST && g_config.ColorCode)
	{
		TRACED("[LogProc] [Log_OnNotify]    custom draw list colorcode");
		NMLVCUSTOMDRAW *cd=(NMLVCUSTOMDRAW*)nmh;
		switch(cd->nmcd.dwDrawStage) {
			case CDDS_PREPAINT:
				SetWindowLongPtr(hwnd, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
				break;
			case CDDS_ITEMPREPAINT: {
				Color c;

				switch((int)cd->nmcd.lItemlParam) {
					case 0:
						c=g_config.AllowedColor;
						break;
					case 1:
						c=g_config.BlockedColor;
						break;
					case 2:
						c=g_config.HttpColor;
						break;
					default: __assume(0);
				}

				cd->clrText=c.Text;
				cd->clrTextBk=c.Background;
				SetWindowLongPtr(hwnd, DWLP_MSGRESULT, CDRF_NEWFONT);
			} break;
			default:
				SetWindowLongPtr(hwnd, DWLP_MSGRESULT, CDRF_DODEFAULT);
		}
		TRACED("[LogProc] [Log_OnNotify]  < Leaving routine (TRUE).");
		return TRUE;
	}
	else if(nmh->code==NM_RCLICK && nmh->idFrom==IDC_LIST)
	{
		TRACED("[LogProc] [Log_OnNotify]    right-click on list");
		NMITEMACTIVATE *nmia=(NMITEMACTIVATE*)nmh;

		if(nmia->iItem!=-1) {
			set<unsigned int> ips;
			GetLocalIps(ips, LOCALIP_ADAPTER);

			LVITEM lvi={0};
			lvi.mask=LVIF_PARAM;
			lvi.iItem=nmia->iItem;
			ListView_GetItem(nmh->hwndFrom, &lvi);

			UINT ret = CreateListViewPopUpMenu(hwnd, nmh, nmia, lvi);
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
	else
	{
		TRACED("[LogProc] [Log_OnNotify]    doing nothing");
	}

	TRACED("[LogProc] [Log_OnNotify]  < Leaving routine (0).");

	return 0;
}



static void Log_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	TRACEV("[LogProc] [Log_OnSize]    Entering routine.");
	HWND enable=GetDlgItem(hwnd, IDC_ENABLE);
	HWND lists=GetDlgItem(hwnd, IDC_LISTS);
	HWND update=GetDlgItem(hwnd, IDC_UPDATE);
	HWND http=GetDlgItem(hwnd, IDC_HTTP);
	HWND history=GetDlgItem(hwnd, IDC_HISTORY);
	HWND clear=GetDlgItem(hwnd, IDC_CLEAR);
	HWND enabledstatus=GetDlgItem(hwnd, IDC_ENABLED_STATUS);
	HWND httpstatus=GetDlgItem(hwnd, IDC_HTTP_STATUS);
	HWND updatestatus=GetDlgItem(hwnd, IDC_UPDATE_STATUS);
	HWND lastupdate=GetDlgItem(hwnd, IDC_LAST_UPDATE);
	HWND log=GetDlgItem(hwnd, IDC_LIST);

	RECT rc;
	GetWindowRect(enable, &rc);

	int midwidth=cx-20-(rc.right-rc.left)*2;
	int halfwidth=(midwidth-5)/2;

	HDWP dwp=BeginDeferWindowPos(11);

	DeferWindowPos(dwp, enable, NULL, 5, 5, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, lists, NULL, 5, 10+(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, update, NULL, 5, 15+(rc.bottom-rc.top)*2, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	DeferWindowPos(dwp, http, NULL, cx-5-(rc.right-rc.left), 5, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, history, NULL, cx-5-(rc.right-rc.left), 10+(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, clear, NULL, cx-5-(rc.right-rc.left), 15+(rc.bottom-rc.top)*2, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	DeferWindowPos(dwp, enabledstatus, NULL, 10+(rc.right-rc.left), 5, halfwidth, (rc.bottom-rc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, httpstatus, NULL, 15+(rc.right-rc.left)+halfwidth, 5, halfwidth, (rc.bottom-rc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, updatestatus, NULL, 10+(rc.right-rc.left), 10+(rc.bottom-rc.top), midwidth, (rc.bottom-rc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, lastupdate, NULL, 10+(rc.right-rc.left), 15+(rc.bottom-rc.top)*2, midwidth, (rc.bottom-rc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);

	DeferWindowPos(dwp, log, NULL, 5, 20+(rc.bottom-rc.top)*3, cx-10, cy-25-(rc.bottom-rc.top)*3, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);

	EndDeferWindowPos(dwp);
}

class TempAllowRemovePred {
private:
	p2p::list &l;
	time_t current;
	bool changed;

public:
	TempAllowRemovePred(p2p::list &list) : l(list),current(time(NULL)),changed(false) {}

	~TempAllowRemovePred() {
		if(changed) LoadLists(NULL);
	}

	bool operator()(const allowlist_t::value_type &v) {
		if((v.first-current)<0) {
			changed=true;
			l.erase(v.second);
			return true;
		}
		else return false;
	}
};



static void Log_OnTimer(HWND hwnd, UINT id)
{
	TRACEV("[LogProc] [Log_OnTimer]  > Entering routine.");
	switch(id)
	{
		case TIMER_UPDATE:
		{
			bool hasupdate = false;
			TRACEV("[LogProc] [Log_OnTimer]    TIMER_UPDATE");

			{
				mutex::scoped_lock lock(g_lastupdatelock);
				if(g_config.UpdateInterval>0 && (time(NULL)-g_config.LastUpdate >= ((time_t)g_config.UpdateInterval)*86400))
				{
					TRACEI("[LogProc] [Log_OnTimer]    performing automated update of program/lists");
					UpdateLists(NULL);

					// prevent deadlock if UpdateStatus is called here
					hasupdate = true;
				}
			}

			if (hasupdate)
				UpdateStatus(hwnd);
		} break;

		case TIMER_TEMPALLOW:
			TRACEV("[LogProc] [Log_OnTimer]    TIMER_TEMPALLOW");
			g_allowlist.remove_if(TempAllowRemovePred(g_tempallow));
			g_blocklist.remove_if(TempAllowRemovePred(g_tempblock));
			break;

		case TIMER_COMMITLOG:
			TRACEV("[LogProc] [Log_OnTimer]    TIMER_COMMITLOG");
			g_log->Commit();
			break;
	}

	TRACEV("[LogProc] [Log_OnTimer]  < Leaving routine.");

} // End of Log_OnTimer()



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
UINT CreateListViewPopUpMenu(HWND hwnd, NMHDR *nmh, NMITEMACTIVATE *nmia, LVITEM &lvi)
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

		tstring str;
		if(lvi.lParam) {
			str=boost::str(tformat(LoadString(IDS_ALLOWXFOR15MIN))%text);
			AppendMenu(menu, MF_STRING, ID_LIST_ALLOWFOR15MIN, str.c_str());

			str=boost::str(tformat(LoadString(IDS_ALLOWXFORONEHOUR))%text);
			AppendMenu(menu, MF_STRING, ID_LIST_ALLOWFORONEHOUR, str.c_str());

			str=boost::str(tformat(LoadString(IDS_ALLOWXPERM))%text);
			AppendMenu(menu, MF_STRING, ID_LIST_ALLOWPERM, str.c_str());
		}
		else {
			str=boost::str(tformat(LoadString(IDS_BLOCKXFOR15MIN))%text);
			AppendMenu(menu, MF_STRING, ID_LIST_BLOCKFOR15MIN, str.c_str());

			str=boost::str(tformat(LoadString(IDS_BLOCKXFORONEHOUR))%text);
			AppendMenu(menu, MF_STRING, ID_LIST_BLOCKFORONEHOUR, str.c_str());

			str=boost::str(tformat(LoadString(IDS_BLOCKXPERM))%text);
			AppendMenu(menu, MF_STRING, ID_LIST_BLOCKPERM, str.c_str());
		}

		AppendMenu(menu, MF_SEPARATOR, 0, NULL);

		str=boost::str(tformat(LoadString(IDS_COPYXTOCLIPBOARD))%text);
		AppendMenu(menu, MF_STRING, ID_LIST_COPYIP, str.c_str());

		RECT rect;
		GetWindowRect(nmh->hwndFrom, &rect);

		SetForegroundWindow(hwnd);
		ret=TrackPopupMenuEx(menu, TPM_TOPALIGN|TPM_RETURNCMD, rect.left+nmia->ptAction.x, rect.top+nmia->ptAction.y, hwnd, NULL);

		DestroyMenu(menu);
	}

	return ret;
}

INT_PTR CALLBACK Log_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		TCHAR chBuf[256];
		_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[LogProc] [Log_DlgProc]    processing hwnd:[%p] msg:[%d]"), hwnd, msg);
		g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_DEBUG);

		switch(msg) {
			HANDLE_MSG(hwnd, WM_COMMAND, Log_OnCommand);
			HANDLE_MSG(hwnd, WM_DESTROY, Log_OnDestroy);
			HANDLE_MSG(hwnd, WM_INITDIALOG, Log_OnInitDialog);
			HANDLE_MSG(hwnd, WM_NOTIFY, Log_OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, Log_OnSize);
			HANDLE_MSG(hwnd, WM_TIMER, Log_OnTimer);
			case WM_LOG_HOOK:
			case WM_LOG_RANGES:
				TRACEV("[LogProc] [Log_DlgProc]    WM_LOG_HOOK or WM_LOG_RANGES");
				UpdateStatus(hwnd);
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
