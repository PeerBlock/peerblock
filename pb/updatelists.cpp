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

#include "stdafx.h"
#include "resource.h"
using namespace std;

#include "tracelog.h"
extern TraceLog g_tlog;

#define DO_STRINGIFY(x) #x
#define STRINGIFY(x) DO_STRINGIFY(x)

#ifdef _WIN32_WINNT
#ifdef _WIN64
// nt x64 build
#define BUILDTYPE 4
#else
// nt build
#define BUILDTYPE 3
#endif
#else
#ifdef _WIN64
// 9x x64 build
#define BUILDTYPE 2
#else
// 9x build
#define BUILDTYPE 1
#endif
#endif

#define BUILDNUM (BUILDTYPE*(unsigned long long)10000000000+BUILDDATE)
#define BUILDSTR STRINGIFY(BUILDTYPE) STRINGIFY(BUILDDATE)

// TODO:  make a special page to display for update-found purposes; pass in build-string?
static const char *g_agent="PeerBlock/" MAKE_STR(PB_VER_MAJOR) "." MAKE_STR(PB_VER_MINOR) "." MAKE_STR(PB_VER_BUGFIX) "." MAKE_STR(PB_VER_BUILDNUM);
static const LPCTSTR g_updateserver=_T("http://www.peerblock.com");	// displayed in Update UI
const unsigned long long g_build=BUILDNUM;

#ifdef PB_RELTYPE_STABLE
static const char *g_updateurl="http://update.peerblock.com/pb_update.php?build="BUILDSTR;
static const LPCTSTR g_homepage=_T("http://update.peerblock.com/latest-release");		// displayed in web-browser if new program version is found
#endif
#ifdef PB_RELTYPE_BETA
static const char *g_updateurl="http://update.peerblock.com/pb_update_ir.php?build="BUILDSTR;
static const LPCTSTR g_homepage=_T("http://update.peerblock.com/latest-interim-release");
#endif
#ifdef PB_RELTYPE_TEST
static const char *g_updateurl="http://update.peerblock.com/pb_update_test.php?build="BUILDSTR;
static const LPCTSTR g_homepage=_T("http://update.peerblock.com/latest-test-release");
#endif
#ifdef PB_RELTYPE_DEV
static const char *g_updateurl="http://update.peerblock.com/pb_update_dev.php?build="BUILDSTR;
static const LPCTSTR g_homepage=_T("http://www.peerblock.com");
#endif

static const UINT TIMER_COUNTDOWN=1;
static unsigned short g_countdown;
static HWND g_updater=NULL;

class UpdateThread {
private:
	struct HandleData {
		char errbuf[CURL_ERROR_SIZE];
		UpdateThread *ut;
		string url;
		path tempfile;
		FILE *fp;
		DynamicList *list;
		int index;
		bool finished;
		double progress;

		HandleData(UpdateThread *ut) : ut(ut),fp(NULL),list(NULL),finished(false),progress(0.0) {
			errbuf[0]='\0';
		}
	};

	set<unsigned int> allowed;

	vector<HandleData*> hdata;
	double progressmod, maxprogress;

	HWND hwnd, list, progress;
	bool autoupdate;



	//============================================================================================
	//
	//  preconnect_func()
	//
	//    - Called by list::save()
	//
	/// <summary>
	///   Writes our list out to binary "p2b" file.
	/// </summary>
	//
	static void preconnect_func(void *clientp, sockaddr *addr, int addrlen) {

		TRACEV("[UpdateThread] [preconnect_func]  > Entering routine.");

		if(addrlen >= sizeof(sockaddr_in) && addr->sa_family == AF_INET) {
			unsigned int ip = htonl(((sockaddr_in*)addr)->sin_addr.s_addr);

			set<unsigned int> &allowed = *(set<unsigned int>*)clientp;

			set<unsigned int>::iterator iter = allowed.lower_bound(ip);
			if(iter == allowed.end() || *iter != ip) {
				allowed.insert(iter, ip);

				if(g_filter) {
					p2p::list allow;

					for(iter = allowed.begin(); iter != allowed.end(); ++iter) {
						allow.insert(p2p::range(L"Auto-allow for updating", *iter, *iter));
					}

					allow.optimize(true);

					if(g_filter) {
						g_filter->setranges(allow, false);
					}
				}
			}
		}

		TRACEV("[UpdateThread] [preconnect_func]  < Leaving routine.");

	} // End of preconnect_func()



	static size_t append_func(void *data, size_t size, size_t count, void *arg) {
		size_t ret=size*count;

		((string*)arg)->append((const char*)data, ret);

		return ret;
	}

	static int progress_func(void *arg, double total, double pos, double, double) {

		TRACED("[UpdateThread] [progress_func]  > Entering routine.");

		HandleData *data=(HandleData*)arg;

		if(total>0.0) {
			data->progress=min(pos/total*100.0, 100.0);
			if(data->ut->progress) data->ut->UpdateProgress();
		}

		if(data->ut->list) {
			tstring str;

			if(total>0.0) {
				int pcnt=(int)data->progress;
				str=boost::str(tformat(LoadString(IDS_DOWNLOADINGPCNT))%pcnt);
			}
			else str=LoadString(IDS_DOWNLOADING);

			LVITEM lvi={0};
			lvi.mask=LVIF_TEXT;
			lvi.iItem=data->index;
			lvi.iSubItem=2;
			lvi.pszText=(LPTSTR)str.c_str();
			ListView_SetItem(data->ut->list, &lvi);
		}

		TRACED("[UpdateThread] [progress_func]  < Leaving routine.");

		return 0;
	}

	void UpdateProgress() 
	{
		// TODO:  Remove, or at least reduce severity of, these tracelog statements, as they're for BETA DEBUG PURPOSES ONLY!

		TRACED("[UpdateThread] [UpdateProgress]  > Entering routine.");

		double total=this->progressmod;

		TRACED("[UpdateThread] [UpdateProgress]    about to spin through vector");
		for(vector<HandleData*>::size_type i=0; i<hdata.size(); i++)
			total+=hdata[i]->progress;
		
		TRACED("[UpdateThread] [UpdateProgress]    computing temporary Total value");
		total=min(total, maxprogress);

		TRACED("[UpdateThread] [UpdateProgress]    sending message to progress-bar");
		SendMessage(progress, PBM_SETPOS, (WPARAM)(int)total, 0);

		TRACED("[UpdateThread] [UpdateProgress]  < Leaving routine.");
	}

	static DWORD GetFileSize(LPCTSTR file) {

		TRACEV("[UpdateThread] [GetFileSize]  > Entering routine.");

		TCHAR chBuf[512];
		_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[UpdateThread] [GetFileSize]    file: [%s]"), file);
		g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_INFO);

		HANDLE fp=CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if(fp==INVALID_HANDLE_VALUE) throw win32_error("CreateFile");

		DWORD ret=::GetFileSize(fp, NULL);
		if(ret==INVALID_FILE_SIZE) throw win32_error("GetFileSize");

		CloseHandle(fp);

		TRACEV("[UpdateThread] [GetFileSize]  < Leaving routine.");
		return ret;
	}

public:
	int changes;
	bool aborted;

	UpdateThread() 
	{
		TRACEI("[UpdateThread] [UpdateThread]    created thread (base constructor)");
	}

	UpdateThread(HWND hwnd, HWND list, HWND progress, bool autoupdate)
		: hwnd(hwnd), list(list), progress(progress), autoupdate(autoupdate),changes(0),aborted(false),progressmod(0.0) 
	{
		TRACEI("[UpdateThread] [UpdateThread]    created thread");
	}

	~UpdateThread() 
	{
		if(allowed.size() > 0 && g_filter) {
			g_filter->setranges(p2p::list(), false);
		}
//		TRACEI("[UpdateThread] [UpdateThread]    destroyed thread");
	}



	int _Process() 
	{
		TRACEV("[UpdateThread] [_Process]  > Entering routine.");
		unsigned short total=0;
		bool updatepb=false;
		bool updatelists=false;

		if(g_config.UpdatePeerBlock) total+=1;
		if(g_config.UpdateLists) 
		{
			for(vector<DynamicList>::size_type i=0; i<g_config.DynamicLists.size(); i++) {
				if(g_config.DynamicLists[i].Enabled) {
					total++;
					updatelists=true;
				}
			}
		}

		if(total>0) 
		{
			const string proxy=TSTRING_UTF8(g_config.UpdateProxy);

			string build;
			vector<CURL*> handles;

			LVITEM lvi={0};
			lvi.mask=LVIF_TEXT;

			if(progress) 
			{
				SendMessage(progress, PBM_SETPOS, 0, 0);
				SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, total*100));
				maxprogress=total*100.0;
			}

			curl_global_init(CURL_GLOBAL_WIN32);

			CURLM *multi=curl_multi_init();

			///////////////////////////////////////////////
			/// Update PeerBlock

			if(g_config.UpdatePeerBlock) 
			{
				TRACEI("[UpdateThread] [_Process]    updating peerblock");
				TCHAR buf[128];
				swprintf_s(buf, sizeof(buf)/2, L"[UpdateThread] [_Process]    update url:[%S]", g_updateurl);
				TRACEBUFI(buf);
				swprintf_s(buf, sizeof(buf)/2, L"[UpdateThread] [_Process]    homepage url:[%s]", g_homepage);
				TRACEBUFI(buf);
				swprintf_s(buf, sizeof(buf)/2, L"[UpdateThread] [_Process]    agent string:[%S]", g_agent);
				TRACEBUFI(buf);

				HandleData *data=new HandleData(this);

				CURL *site=curl_easy_init();
				if(site) 
				{
					TRACEV("[UpdateThread] [_Process]    curl returned site, now setting options");
					curl_easy_setopt(site, CURLOPT_PRECONNECT, preconnect_func);
					curl_easy_setopt(site, CURLOPT_PRECONNECTDATA, &allowed);
					curl_easy_setopt(site, CURLOPT_FOLLOWLOCATION, 1);
					curl_easy_setopt(site, CURLOPT_PROGRESSFUNCTION, progress_func);
					curl_easy_setopt(site, CURLOPT_PROGRESSDATA, data);
					curl_easy_setopt(site, CURLOPT_NOPROGRESS, 0);
					curl_easy_setopt(site, CURLOPT_USERAGENT, g_agent);
					curl_easy_setopt(site, CURLOPT_URL, g_updateurl);
					curl_easy_setopt(site, CURLOPT_WRITEFUNCTION, append_func);
					curl_easy_setopt(site, CURLOPT_WRITEDATA, &build);
					curl_easy_setopt(site, CURLOPT_FAILONERROR, 1);
					curl_easy_setopt(site, CURLOPT_PRIVATE, data);
					curl_easy_setopt(site, CURLOPT_ERRORBUFFER, data->errbuf);
					if(proxy.length()>0) 
					{
						curl_easy_setopt(site, CURLOPT_PROXY, proxy.c_str());
						curl_easy_setopt(site, CURLOPT_PROXYTYPE, g_config.UpdateProxyType);
					}

					hdata.push_back(data);
					handles.push_back(site);
					curl_multi_add_handle(multi, site);
					TRACEV("[UpdateThread] [_Process]    added site-handle to curl handle-list");
				}
				else 
				{
					TRACEV("[UpdateThread] [_Process]    curl did not return site");
					delete data;
					data=NULL;
				}

				if(list) 
				{
					TRACEV("[UpdateThread] [_Process]    list found");
					tstring text=LoadString(IDS_UPDATEPB);

					lvi.pszText=(LPTSTR)text.c_str();
					ListView_InsertItem(list, &lvi);

					lvi.iSubItem=1;
					lvi.pszText=(LPTSTR)g_updateserver;
					ListView_SetItem(list, &lvi);

					lvi.iSubItem=2;
					text=LoadString(data?IDS_CONNECTING:IDS_ERRCURL);
					lvi.pszText=(LPTSTR)text.c_str();
					ListView_SetItem(list, &lvi);

					if(data) data->index=lvi.iItem;

					lvi.iItem++;
				}
			}
			else
			{
				TRACEI("[UpdateThread] [_Process]    not updating peerblock, as per config");
			}

			///////////////////////////////////////////////
			/// Update dynamic lists

			if(g_config.UpdateLists && updatelists) 
			{
				TRACEI("[UpdateThread] [_Process]    updating dynamic lists");
				time_t curtime=time(NULL);

				for(vector<DynamicList>::size_type i=0; i<g_config.DynamicLists.size(); i++) 
				{
					if(!g_config.DynamicLists[i].Enabled) continue;

					const path file=g_config.DynamicLists[i].File();

					TCHAR chBuf[256];
					_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[UpdateThread] [_Process]    updating file: [%s]"), file.c_str());
					TRACEBUFI(chBuf);

					time_t elapsedTime = curtime-g_config.DynamicLists[i].LastUpdate;
					bool fileExists = path::exists(file);
					DWORD fileSize = 0;

					_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[UpdateThread] [_Process]    + %d (of 43200) seconds have passed since last update"), elapsedTime);
					TRACEBUFI(chBuf);

					if (fileExists)
					{
						TRACEI("[UpdateThread] [_Process]    + file exists");

						fileSize = GetFileSize(file.c_str());
						_stprintf_s(chBuf, sizeof(chBuf)/2, _T("[UpdateThread] [_Process]    + fileSize = %d"), fileSize);
						TRACEBUFI(chBuf);
					}
					else
					{
						TRACEI("[UpdateThread] [_Process]    + file does not exist");
					}

					if(elapsedTime >= 43200 || !fileExists || fileSize==0) 
					{
						HandleData *data=new HandleData(this);

						data->list=&g_config.DynamicLists[i];
						data->url=TSTRING_UTF8(g_config.DynamicLists[i].Url);
						data->tempfile=g_config.DynamicLists[i].TempFile();

						if(!path::exists(data->tempfile.without_leaf())) path::create_directory(data->tempfile.without_leaf());

						data->fp=_tfopen(data->tempfile.c_str(), _T("wbT"));

						if(data->fp) 
						{
							CURL *site=curl_easy_init();
							if(site) 
							{
								curl_easy_setopt(site, CURLOPT_PRECONNECT, preconnect_func);
								curl_easy_setopt(site, CURLOPT_PRECONNECTDATA, &allowed);
								curl_easy_setopt(site, CURLOPT_FOLLOWLOCATION, 1);
								curl_easy_setopt(site, CURLOPT_PROGRESSFUNCTION, progress_func);
								curl_easy_setopt(site, CURLOPT_PROGRESSDATA, data);
								curl_easy_setopt(site, CURLOPT_NOPROGRESS, 0);
								curl_easy_setopt(site, CURLOPT_USERAGENT, g_agent);
								curl_easy_setopt(site, CURLOPT_URL, data->url.c_str());
								curl_easy_setopt(site, CURLOPT_WRITEFUNCTION, NULL);
								curl_easy_setopt(site, CURLOPT_WRITEDATA, data->fp);
								curl_easy_setopt(site, CURLOPT_PRIVATE, data);
								curl_easy_setopt(site, CURLOPT_ERRORBUFFER, data->errbuf);
								if(proxy.length()>0) 
								{
									curl_easy_setopt(site, CURLOPT_PROXY, proxy.c_str());
									curl_easy_setopt(site, CURLOPT_PROXYTYPE, g_config.UpdateProxyType);
								}

								if(g_config.DynamicLists[i].LastUpdate && path::exists(g_config.DynamicLists[i].File())) 
								{
									curl_easy_setopt(site, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
									curl_easy_setopt(site, CURLOPT_TIMEVALUE, g_config.DynamicLists[i].LastUpdate);
								}
								else curl_easy_setopt(site, CURLOPT_TIMECONDITION, CURL_TIMECOND_NONE);
								
								hdata.push_back(data);
								handles.push_back(site);
								curl_multi_add_handle(multi, site);
								TRACEV("[UpdateThread] [_Process]    added dynamic-list site to curl handle-list");
							}
							else 
							{
								TRACEW("[UpdateThread] [_Process]    *  Dynamic-list site not found by curl!");
								fclose(data->fp);
								{
									try 
									{
										path::remove(data->tempfile);
									}
									catch(exception &ex) 
									{
										ExceptionBox(hwnd, ex, __FILE__, __LINE__);
									}
								}

								delete data;
								data=NULL;
								g_config.DynamicLists[i].FailedUpdate=true;
							}
						}
						else 
						{
							TRACEW("[UpdateThread] [_Process]    *  Couldn't open data->fp!");
							delete data;
							data=NULL;

							g_config.DynamicLists[i].FailedUpdate=true;
						}

						if(list) 
						{
							TRACEI("[UpdateThread] [_Process]    found list, adding stuff to list-view");

							lvi.iSubItem=0;
							lvi.pszText=(LPTSTR)g_config.DynamicLists[i].Description.c_str();
							ListView_InsertItem(list, &lvi);

							lvi.iSubItem=1;
							lvi.pszText=(LPTSTR)g_config.DynamicLists[i].Url.c_str();
							ListView_SetItem(list, &lvi);

							const tstring str=LoadString(data?IDS_CONNECTING:IDS_ERRFILEOPEN);
							lvi.iSubItem=2;
							lvi.pszText=(LPTSTR)str.c_str();
							ListView_SetItem(list, &lvi);

							if(data) data->index=lvi.iItem;
							else this->progressmod+=100.0;

							lvi.iItem++;
						}
					}
					else 
					{
						TRACEI("[UpdateThread] [_Process]    not updating list; setting progress-bar to 100");
						this->progressmod+=100.0;

						if(list) 
						{
							TRACEV("[UpdateThread] [_Process]    setting list-items, \"no update needed\"");

							lvi.iSubItem=0;
							lvi.pszText=(LPTSTR)g_config.DynamicLists[i].Description.c_str();
							ListView_InsertItem(list, &lvi);

							lvi.iSubItem=1;
							lvi.pszText=(LPTSTR)g_config.DynamicLists[i].Url.c_str();
							ListView_SetItem(list, &lvi);

							const tstring str=LoadString(IDS_NOUPDATENEEDED);
							lvi.iSubItem=2;
							lvi.pszText=(LPTSTR)str.c_str();
							ListView_SetItem(list, &lvi);

							lvi.iItem++;
						}

						TRACEV("[UpdateThread] [_Process]    finished updating list");
						g_config.DynamicLists[i].FailedUpdate=false;
					}
				}
			}
			else
			{
				TRACEI("[UpdateThread] [_Process]    not updating dynamic list(s)");
			}

			if(progress) this->UpdateProgress();

			if(handles.size()>0) 
			{
				///////////////////////////////////////////////
				/// Perform Updates

				int max;
				fd_set read, write, error;

				int running;
				TRACEI("[UpdateThread] [_Process]    performing updates");
				while(curl_multi_perform(multi, &running)==CURLM_CALL_MULTI_PERFORM);

				TRACEV("[UpdateThread] [_Process]    called initial curl_multi_perform");
				while(!aborted && running) 
				{
					FD_ZERO(&read); FD_ZERO(&write); FD_ZERO(&error);
					curl_multi_fdset(multi, &read, &write, &error, &max);

					timeval tv={2,0};

					if(select(FD_SETSIZE, &read, &write, &error, &tv)!=-1) 
					{
						while(curl_multi_perform(multi, &running)==CURLM_CALL_MULTI_PERFORM);
						TRACED("[UpdateThread] [_Process]    called curl_multi_perform");

						int msgs;
						while(CURLMsg *msg=curl_multi_info_read(multi, &msgs)) 
						{
							TRACEV("[UpdateThread] [_Process]    called curl_multi_info_read");
							if(msg->msg==CURLMSG_DONE) 
							{
								TRACEV("[UpdateThread] [_Process]    CURLMSG_DONE");
								HandleData *data;
								curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &data);

								data->finished=true;
								data->progress=100.0;
								if(this->progress) this->UpdateProgress();

								if(data->fp) 
								{
									fclose(data->fp);
									data->fp=NULL;
								}

								if(msg->data.result==CURLE_OK) 
								{
									TRACEV("[UpdateThread] [_Process]    CURLE_OK");
									long code;
									TCHAR chBuf[256];
									curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &code);
									_stprintf_s(chBuf, sizeof(chBuf)/2, 
										_T("[UpdateThread] [_Process]    response code: [%ld]"), code);
									g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_SUCCESS);

									if(code==200) 
									{
										TRACEI("[UpdateThread] [_Process]    successfully contacted URL; code:[200]");
										if(data->list) 
										{
											TRACEV("[UpdateThread] [_Process]    data is a list");
											try 
											{
												// Verify that list is useful
												p2p::list testlist;
												LoadList(data->tempfile, testlist);
												if (testlist.size() > 0)
												{
													// Overwrite old list-file
													TRACES("[UpdateThread] [_Process]    Verified newly downloaded list, so overwriting old list");
													path::move(data->tempfile, data->list->File(), true);
													data->list->FailedUpdate=false;

													time(&data->list->LastUpdate);
													changes++;
												}
												else
												{
													// Failed to verify!
													TRACEW("[UpdateThread] [_Process]    Newly downloaded list failed verification: has 0 elements!!");

													if (list)
													{
														const tstring str = LoadString(IDS_DNLISTFAILEDVERIFY);
														lvi.iItem = data->index;
														lvi.iSubItem = 2;
														lvi.pszText = (LPTSTR)str.c_str();
														ListView_SetItem(list, &lvi);
														data->list->FailedUpdate = true;
													}
												}
											}
											catch(exception &ex) 
											{
												ExceptionBox(hwnd, ex, __FILE__, __LINE__);
												data->list->FailedUpdate=true;
											}
											

											if(list && !data->list->FailedUpdate) 
											{
												TRACEV("[UpdateThread] [_Process]    updating list entry to IDS_FINISHED");
												const tstring str=LoadString(IDS_FINISHED);

												lvi.iItem=data->index;
												lvi.iSubItem=2;
												lvi.pszText=(LPTSTR)str.c_str();
												ListView_SetItem(list, &lvi);
											}
										}
										else 
										{
											TRACEV("[UpdateThread] [_Process]    data is not a list, must be program update");
											try 
											{
												unsigned long long b=boost::lexical_cast<unsigned long long>(build);

												if(list) 
												{
													if (b > g_build)
													{
														TRACES("[UpdateThread] [_Process]    found list var; new program version found");
														updatepb = true;
													}
													else
													{
														TRACEI("[UpdateThread] [_Process]    found list var; no new program version found");
														updatepb = false;
													}
													const tstring str=LoadString((b>g_build)?IDS_UPDATEAVAILABLE:IDS_NONEAVAILABLE);

													lvi.iItem=data->index;
													lvi.iSubItem=2;
													lvi.pszText=(LPTSTR)str.c_str();
													ListView_SetItem(list, &lvi);
												}
												else 
												{
													if (b > g_build)
													{
														TRACES("[UpdateThread] [_Process]    no list var; new program version found");
														updatepb = true;
													}
													else
													{
														TRACEI("[UpdateThread] [_Process]    no list var; no new program version found");
														updatepb = false;
													}
												}
											}
											catch(...) 
											{
												TRACEW("[UpdateThread] [_Process]    *  caught exception; ignoring");
												// keep going...
											}
										}
									}
									else if(code==304) 
									{
										TRACEI("[UpdateThread] [_Process]    NO UPDATE AVAILABLE; code:[304]");
										if(data->list) 
										{
											TRACEV("[UpdateThread] [_Process]    found data->list; NO UPDATE AVAILABLE");
											time(&data->list->LastUpdate);
											data->list->FailedUpdate=false;
										}

										if(list) 
										{
											TRACEV("[UpdateThread] [_Process]    found list; NO UPDATE AVAILABLE");
											const tstring str=LoadString(IDS_NOUPDATEAVAIL);

											lvi.iItem=data->index;
											lvi.iSubItem=2;
											lvi.pszText=(LPTSTR)str.c_str();
											ListView_SetItem(list, &lvi);
										}
									}
									else if(code>=300) 
									{
										tstring strBuf = boost::str(tformat(
											_T("[UpdateThread] [_Process]    code >= 300; code:[%1%]")) % code );
										TRACEBUFE(strBuf);
										if(data->list) data->list->FailedUpdate=true;

										if(list) 
										{
											tstring strBuf = boost::str(tformat(
												_T("[UpdateThread] [_Process]    found list; ERROR CONTACTING URL, code:[%1%]")) % code );
											TRACEBUFE(strBuf);
											const tstring str=boost::str(tformat(LoadString(IDS_ERRORCONTACTINGWHY)) % code);

											lvi.iItem=data->index;
											lvi.iSubItem=2;
											lvi.pszText=(LPTSTR)str.c_str();
											ListView_SetItem(list, &lvi);
										}
									}
									else if(code>200) 
									{
										tstring strBuf = boost::str(tformat(
											_T("[UpdateThread] [_Process]    code > 200, code < 300; code:[%1%]")) % code );
										TRACEBUFE(strBuf);
										if(data->list) data->list->FailedUpdate=true;

										if(list) 
										{
											tstring strBuf = boost::str(tformat(
												_T("[UpdateThread] [_Process]    found list; UNEXPECTED RESPONSE contacting url, code:[%1%]")) % code );
											TRACEBUFE(strBuf);
											const tstring str=boost::str(tformat(LoadString(IDS_UNEXPCONTACTINGWHY)) % code);

											lvi.iItem=data->index;
											lvi.iSubItem=2;
											lvi.pszText=(LPTSTR)str.c_str();
											ListView_SetItem(list, &lvi);
										}
									}
								} // end if CURLE_OK
								else if(data->list) 
								{
									tstring strBuf = boost::str(tformat(_T("[UpdateThread] [_Process]    *  failed update, !CURLE_OK, result:[%1%]")) % msg->data.result );
									TRACEBUFE(strBuf);
									data->list->FailedUpdate=true;

									if(list) {
										tstring str;
										
										if(data->errbuf[0])
										{
											tstring strBuf = boost::str(tformat(
												_T("[UpdateThread] [_Process]    ERROR CONTACTING URL, result:[%1%] err:[%2%]")) 
												% msg->data.result % data->errbuf );
											TRACEBUFE(strBuf);
											str=boost::str(tformat(LoadString(IDS_ERRORCONTACTINGWHY))%data->errbuf);
										}
										else 
										{
											tstring strBuf = boost::str(tformat(_T("[UpdateThread] [_Process]    ERROR CONTACTING URL, result:[%1%]")) % msg->data.result );
											TRACEBUFE(strBuf);
											str=boost::str(tformat(LoadString(IDS_ERRORCONTACTINGWHY))%msg->data.result);
										}

										lvi.iItem=data->index;
										lvi.iSubItem=2;
										lvi.pszText=(LPTSTR)str.c_str();
										ListView_SetItem(list, &lvi);
									}
								}

								if(data->list) 
								{
									TRACEI("[UpdateThread] [_Process]    removing tempfile");
									try 
									{
										if(path::exists(data->tempfile)) path::remove(data->tempfile);
									}
									catch(exception &ex) 
									{
										ExceptionBox(hwnd, ex, __FILE__, __LINE__);
									}
								}
							}
						}
					}
				}

				///////////////////////////////////////////////
				/// Cleanup

				TRACEI("[UpdateThread] [_Process]    performing cleanup");
				for(vector<CURL*>::size_type i=0; i<handles.size(); i++) {
					curl_multi_remove_handle(multi, handles[i]);

					HandleData *data;
					curl_easy_getinfo(handles[i], CURLINFO_PRIVATE, &data);

					if(list && aborted && !data->finished) {
						TRACEW("[UpdateThread] [_Process]    processing aborted before finished");
						const tstring str=LoadString(IDS_ABORTED);
						lvi.iItem=data->index;
						lvi.iSubItem=2;
						lvi.pszText=(LPTSTR)str.c_str();
						ListView_SetItem(list, &lvi);
					}

					if(data->fp) fclose(data->fp);

					if(data->list) {
						try {
							if(path::exists(data->tempfile)) path::remove(data->tempfile);
						}
						catch(exception &ex) {
							ExceptionBox(hwnd, ex, __FILE__, __LINE__);
						}
					}

					delete data;

					curl_easy_cleanup(handles[i]);
				}

				TRACES("[UpdateThread] [_Process]    Done updating.");
			}

			if(progress) SendMessage(progress, PBM_SETPOS, (int)(total*100), 0);

			curl_multi_cleanup(multi);
			curl_global_cleanup();

			if(!aborted && updatepb && MessageBox(hwnd, IDS_PBUPDATETEXT, IDS_PBUPDATE, MB_ICONQUESTION|MB_YESNO)==IDYES)
			{
				TRACEI("[UpdateThread] [_Process]    program update found; showing homepage");
				ShellExecute(NULL, NULL, g_homepage, NULL, NULL, SW_SHOWNORMAL);
			}
		}

		HWND abort=GetDlgItem(hwnd, IDC_ABORT);
		HWND close=GetDlgItem(hwnd, IDC_CLOSE);

		SendMessage(hwnd, DM_SETDEFID, IDC_CLOSE, 0);
		SetFocus(close);

		EnableWindow(abort, FALSE);
		EnableWindow(close, TRUE);

		TRACEI("[UpdateThread] [_Process]    finished downloading updates");

		if(IsWindowVisible(hwnd)) 
		{
			TRACEV("[UpdateThread] [_Process]    waiting for update-window to close");
			if(g_config.UpdateCountdown==0) {
				TRACEI("[UpdateThread] [_Process]    closing window now, since UpdateCountdown == 0");
				if(g_updater==(HWND)-1) EndDialog(hwnd, changes);
				else DestroyWindow(hwnd);
			}
			if(g_config.UpdateCountdown>0) {
				TRACEI("[UpdateThread] [_Process]    starting countdown, since UpdateCountdown > 0");
				g_countdown=(unsigned short)g_config.UpdateCountdown;
				tstring s=boost::str(tformat(LoadString(IDS_CLOSEX))%g_countdown);
				SetDlgItemText(hwnd, IDC_CLOSE, s.c_str());

				SetTimer(hwnd, TIMER_COUNTDOWN, 1000, NULL);
			}
		}
		else if(g_updater==(HWND)-1) 
		{
			TRACEV("[UpdateThread] [_Process]    window not visible, g_updater:[FFFFFFFF]:  calling EndDialog()");
			if (!EndDialog(hwnd, changes))
			{
				DWORD err = GetLastError();
				if (err == 5) // access denied
				{
					SendMessage(hwnd, WM_CLOSE, 0, 0);
				}
				else
				{
					TRACEERR("[UpdateThread] [_Process]", L"ERROR calling EndDialog(), trying WM_CLOSE", GetLastError());
					SendMessage(hwnd, WM_CLOSE, 0, 0);
				}
			}
		}
		else 
		{
			TRACEV("[UpdateThread] [_Process]    closing invisible auto-update window");
			if (!DestroyWindow(hwnd))
			{
				DWORD err = GetLastError();
				if (err == 5) // access denied
				{
					SendMessage(hwnd, WM_CLOSE, 0, 0);
				}
				else
				{
					TRACEERR("[UpdateThread] [_Process]", L"ERROR calling DestroyWindow(), trying WM_CLOSE", GetLastError());
					SendMessage(hwnd, WM_CLOSE, 0, 0);
				}
			}
		}

		TRACEV("[UpdateThread] [_Process]  < Leaving routine.");
		return changes;

	} // End of _Process()



	int Process() {
		try {
			return this->_Process();
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

	void operator()() {
		this->Process();
	}
};


class UpdateThreadDummy {
private:
	struct HandleData {
		char errbuf[CURL_ERROR_SIZE];
		UpdateThreadDummy *ut;
		string url;
		path tempfile;
		FILE *fp;
		DynamicList *list;
		int index;
		bool finished;
		double progress;

		HandleData(UpdateThreadDummy *ut) : ut(ut),fp(NULL),list(NULL),finished(false),progress(0.0) {
			errbuf[0]='\0';
		}
	};

	set<unsigned int> allowed;

	vector<HandleData*> hdata;
	double progressmod, maxprogress;

	HWND hwnd, list, progress;
	bool autoupdate;

	static void preconnect_func(void *clientp, sockaddr *addr, int addrlen) {
		if(addrlen >= sizeof(sockaddr_in) && addr->sa_family == AF_INET) {
			unsigned int ip = htonl(((sockaddr_in*)addr)->sin_addr.s_addr);

			set<unsigned int> &allowed = *(set<unsigned int>*)clientp;

			set<unsigned int>::iterator iter = allowed.lower_bound(ip);
			if(iter == allowed.end() || *iter != ip) {
				allowed.insert(iter, ip);

				if(g_filter) {
					p2p::list allow;

					for(iter = allowed.begin(); iter != allowed.end(); ++iter) {
						allow.insert(p2p::range(L"Auto-allow for updating", *iter, *iter));
					}

					allow.optimize(true);

					if(g_filter) {
						g_filter->setranges(allow, false);
					}
				}
			}
		}
	}

	static size_t append_func(void *data, size_t size, size_t count, void *arg) {
		size_t ret=size*count;

		((string*)arg)->append((const char*)data, ret);

		return ret;
	}

	static int progress_func(void *arg, double total, double pos, double, double) {
		HandleData *data=(HandleData*)arg;

		if(total>0.0) {
			data->progress=min(pos/total*100.0, 100.0);
			if(data->ut->progress) data->ut->UpdateProgress();
		}

		if(data->ut->list) {
			tstring str;

			if(total>0.0) {
				int pcnt=(int)data->progress;
				str=boost::str(tformat(LoadString(IDS_DOWNLOADINGPCNT))%pcnt);
			}
			else str=LoadString(IDS_DOWNLOADING);

			LVITEM lvi={0};
			lvi.mask=LVIF_TEXT;
			lvi.iItem=data->index;
			lvi.iSubItem=2;
			lvi.pszText=(LPTSTR)str.c_str();
			ListView_SetItem(data->ut->list, &lvi);
		}

		return 0;
	}

	void UpdateProgress() {
		double total=this->progressmod;

		for(vector<HandleData*>::size_type i=0; i<hdata.size(); i++)
			total+=hdata[i]->progress;
		
		total=min(total, maxprogress);

		SendMessage(progress, PBM_SETPOS, (WPARAM)(int)total, 0);
	}

	static DWORD GetFileSize(LPCTSTR file) {
		HANDLE fp=CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if(fp==INVALID_HANDLE_VALUE) throw win32_error("CreateFile");

		DWORD ret=::GetFileSize(fp, NULL);
		if(ret==INVALID_FILE_SIZE) throw win32_error("GetFileSize");

		CloseHandle(fp);

		return ret;
	}

public:
	int changes;
	bool aborted;

	UpdateThreadDummy() {}
	UpdateThreadDummy(HWND hwnd, HWND list, HWND progress, bool autoupdate)
		: hwnd(hwnd), list(list), progress(progress), autoupdate(autoupdate),changes(0),aborted(false),progressmod(0.0) {}

	~UpdateThreadDummy() {
		if(allowed.size() > 0 && g_filter) {
			g_filter->setranges(p2p::list(), false);
		}
	}

	void boo() {
		g_filter->setranges(p2p::list(), false);
	}

	int _Process() {
		unsigned short total=0;
		bool updatepb=false;
		bool updatelists=false;

		if(g_config.UpdatePeerBlock) total+=1;
		if(g_config.UpdateLists) {
			for(vector<DynamicList>::size_type i=0; i<g_config.DynamicLists.size(); i++) {
				if(g_config.DynamicLists[i].Enabled) {
					total++;
					updatelists=true;
				}
			}
		}

		if(total>0) {
			const string proxy=TSTRING_UTF8(g_config.UpdateProxy);

			string build;
			vector<CURL*> handles;

			LVITEM lvi={0};
			lvi.mask=LVIF_TEXT;

			if(progress) {
				SendMessage(progress, PBM_SETPOS, 0, 0);
				SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, total*100));
				maxprogress=total*100.0;
			}

			curl_global_init(CURL_GLOBAL_WIN32);

			CURLM *multi=curl_multi_init();

			///////////////////////////////////////////////
			/// Update PeerBlock

			if(g_config.UpdatePeerBlock) {
				HandleData *data=new HandleData(this);

				CURL *site=curl_easy_init();
				if(site) {
					curl_easy_setopt(site, CURLOPT_PRECONNECT, preconnect_func);
					curl_easy_setopt(site, CURLOPT_PRECONNECTDATA, &allowed);
					curl_easy_setopt(site, CURLOPT_FOLLOWLOCATION, 1);
					curl_easy_setopt(site, CURLOPT_PROGRESSFUNCTION, progress_func);
					curl_easy_setopt(site, CURLOPT_PROGRESSDATA, data);
					curl_easy_setopt(site, CURLOPT_NOPROGRESS, 0);
					curl_easy_setopt(site, CURLOPT_USERAGENT, g_agent);
					curl_easy_setopt(site, CURLOPT_URL, g_updateurl);
					curl_easy_setopt(site, CURLOPT_WRITEFUNCTION, append_func);
					curl_easy_setopt(site, CURLOPT_WRITEDATA, &build);
					curl_easy_setopt(site, CURLOPT_FAILONERROR, 1);
					curl_easy_setopt(site, CURLOPT_PRIVATE, data);
					curl_easy_setopt(site, CURLOPT_ERRORBUFFER, data->errbuf);
					if(proxy.length()>0) {
						curl_easy_setopt(site, CURLOPT_PROXY, proxy.c_str());
						curl_easy_setopt(site, CURLOPT_PROXYTYPE, g_config.UpdateProxyType);
					}

					hdata.push_back(data);
					handles.push_back(site);
					curl_multi_add_handle(multi, site);
				}
				else {
					delete data;
					data=NULL;
				}

				if(list) {
					tstring text=LoadString(IDS_UPDATEPB);

					lvi.pszText=(LPTSTR)text.c_str();
					ListView_InsertItem(list, &lvi);

					lvi.iSubItem=1;
					lvi.pszText=(LPTSTR)g_updateserver;
					ListView_SetItem(list, &lvi);

					lvi.iSubItem=2;
					text=LoadString(data?IDS_CONNECTING:IDS_ERRCURL);
					lvi.pszText=(LPTSTR)text.c_str();
					ListView_SetItem(list, &lvi);

					if(data) data->index=lvi.iItem;

					lvi.iItem++;
				}
			}

			///////////////////////////////////////////////
			/// Update dynamic lists

			if(g_config.UpdateLists && updatelists) {
				time_t curtime=time(NULL);

				for(vector<DynamicList>::size_type i=0; i<g_config.DynamicLists.size(); i++) {
					if(!g_config.DynamicLists[i].Enabled) continue;

					const path file=g_config.DynamicLists[i].File();
					if(curtime-g_config.DynamicLists[i].LastUpdate >= 43200 || !path::exists(file) || GetFileSize(file.c_str())==0) {
						HandleData *data=new HandleData(this);

						data->list=&g_config.DynamicLists[i];
						data->url=TSTRING_UTF8(g_config.DynamicLists[i].Url);
						data->tempfile=g_config.DynamicLists[i].TempFile();

						if(!path::exists(data->tempfile.without_leaf())) path::create_directory(data->tempfile.without_leaf());

						data->fp=_tfopen(data->tempfile.c_str(), _T("wbT"));

						if(data->fp) {
							CURL *site=curl_easy_init();
							if(site) {
								curl_easy_setopt(site, CURLOPT_PRECONNECT, preconnect_func);
								curl_easy_setopt(site, CURLOPT_PRECONNECTDATA, &allowed);
								curl_easy_setopt(site, CURLOPT_FOLLOWLOCATION, 1);
								curl_easy_setopt(site, CURLOPT_PROGRESSFUNCTION, progress_func);
								curl_easy_setopt(site, CURLOPT_PROGRESSDATA, data);
								curl_easy_setopt(site, CURLOPT_NOPROGRESS, 0);
								curl_easy_setopt(site, CURLOPT_USERAGENT, g_agent);
								curl_easy_setopt(site, CURLOPT_URL, data->url.c_str());
								curl_easy_setopt(site, CURLOPT_WRITEFUNCTION, NULL);
								curl_easy_setopt(site, CURLOPT_WRITEDATA, data->fp);
								curl_easy_setopt(site, CURLOPT_PRIVATE, data);
								curl_easy_setopt(site, CURLOPT_ERRORBUFFER, data->errbuf);
								if(proxy.length()>0) {
									curl_easy_setopt(site, CURLOPT_PROXY, proxy.c_str());
									curl_easy_setopt(site, CURLOPT_PROXYTYPE, g_config.UpdateProxyType);
								}

								if(g_config.DynamicLists[i].LastUpdate && path::exists(g_config.DynamicLists[i].File())) {
									curl_easy_setopt(site, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
									curl_easy_setopt(site, CURLOPT_TIMEVALUE, g_config.DynamicLists[i].LastUpdate);
								}
								else curl_easy_setopt(site, CURLOPT_TIMECONDITION, CURL_TIMECOND_NONE);
								
								hdata.push_back(data);
								handles.push_back(site);
								curl_multi_add_handle(multi, site);
							}
							else {
								fclose(data->fp);
								{
									try {
										path::remove(data->tempfile);
									}
									catch(exception &ex) {
										ExceptionBox(hwnd, ex, __FILE__, __LINE__);
									}
								}

								delete data;
								data=NULL;
								g_config.DynamicLists[i].FailedUpdate=true;
							}
						}
						else {
							delete data;
							data=NULL;

							g_config.DynamicLists[i].FailedUpdate=true;
						}

						if(list) {
							lvi.iSubItem=0;
							lvi.pszText=(LPTSTR)g_config.DynamicLists[i].Description.c_str();
							ListView_InsertItem(list, &lvi);

							lvi.iSubItem=1;
							lvi.pszText=(LPTSTR)g_config.DynamicLists[i].Url.c_str();
							ListView_SetItem(list, &lvi);

							const tstring str=LoadString(data?IDS_CONNECTING:IDS_ERRFILEOPEN);
							lvi.iSubItem=2;
							lvi.pszText=(LPTSTR)str.c_str();
							ListView_SetItem(list, &lvi);

							if(data) data->index=lvi.iItem;
							else this->progressmod+=100.0;

							lvi.iItem++;
						}
					}
					else {
						this->progressmod+=100.0;

						if(list) {
							lvi.iSubItem=0;
							lvi.pszText=(LPTSTR)g_config.DynamicLists[i].Description.c_str();
							ListView_InsertItem(list, &lvi);

							lvi.iSubItem=1;
							lvi.pszText=(LPTSTR)g_config.DynamicLists[i].Url.c_str();
							ListView_SetItem(list, &lvi);

							const tstring str=LoadString(IDS_NOUPDATENEEDED);
							lvi.iSubItem=2;
							lvi.pszText=(LPTSTR)str.c_str();
							ListView_SetItem(list, &lvi);

							lvi.iItem++;
						}

						g_config.DynamicLists[i].FailedUpdate=false;
					}
				}
			}

			if(progress) this->UpdateProgress();

			if(handles.size()>0) {
				///////////////////////////////////////////////
				/// Perform Updates

				int max;
				fd_set read, write, error;

				int running;
				while(curl_multi_perform(multi, &running)==CURLM_CALL_MULTI_PERFORM);

				while(!aborted && running) {
					FD_ZERO(&read); FD_ZERO(&write); FD_ZERO(&error);
					curl_multi_fdset(multi, &read, &write, &error, &max);

					timeval tv={2,0};

					if(select(FD_SETSIZE, &read, &write, &error, &tv)!=-1) {
						while(curl_multi_perform(multi, &running)==CURLM_CALL_MULTI_PERFORM);

						int msgs;
						while(CURLMsg *msg=curl_multi_info_read(multi, &msgs)) {
							if(msg->msg==CURLMSG_DONE) {
								HandleData *data;
								curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &data);

								data->finished=true;
								data->progress=100.0;
								if(this->progress) this->UpdateProgress();

								if(data->fp) {
									fclose(data->fp);
									data->fp=NULL;
								}

								if(msg->data.result==CURLE_OK) {
									long code;
									curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &code);

									if(code==200) {
										if(data->list) {
											try {
												path::move(data->tempfile, data->list->File(), true);
											}
											catch(exception &ex) {
												ExceptionBox(hwnd, ex, __FILE__, __LINE__);
												data->list->FailedUpdate=true;
											}
											
											data->list->FailedUpdate=false;

											time(&data->list->LastUpdate);
											changes++;

											if(list) {
												const tstring str=LoadString(IDS_FINISHED);

												lvi.iItem=data->index;
												lvi.iSubItem=2;
												lvi.pszText=(LPTSTR)str.c_str();
												ListView_SetItem(list, &lvi);
											}
										}
										else {
											try {
												unsigned int b=boost::lexical_cast<unsigned int>(build);

												if(list) {
													const tstring str=LoadString((b>g_build)?IDS_UPDATEAVAILABLE:IDS_NONEAVAILABLE);

													lvi.iItem=data->index;
													lvi.iSubItem=2;
													lvi.pszText=(LPTSTR)str.c_str();
													ListView_SetItem(list, &lvi);
												}
												else updatepb=(b>g_build);
											}
											catch(...) {
												// keep going...
											}
										}
									}
									else if(code==304) {
										if(data->list) {
											time(&data->list->LastUpdate);
											data->list->FailedUpdate=false;
										}

										if(list) {
											const tstring str=LoadString(IDS_NOUPDATEAVAIL);

											lvi.iItem=data->index;
											lvi.iSubItem=2;
											lvi.pszText=(LPTSTR)str.c_str();
											ListView_SetItem(list, &lvi);
										}
									}
									else if(code>=300) {
										if(data->list) data->list->FailedUpdate=true;

										if(list) {
											const tstring str=LoadString(IDS_ERRORCONTACTING);

											lvi.iItem=data->index;
											lvi.iSubItem=2;
											lvi.pszText=(LPTSTR)str.c_str();
											ListView_SetItem(list, &lvi);
										}
									}
								}
								else if(data->list) {
									data->list->FailedUpdate=true;

									if(list) {
										tstring str;
										
										if(data->errbuf[0])
											str=boost::str(tformat(LoadString(IDS_ERRORCONTACTINGWHY))%data->errbuf);
										else str=LoadString(IDS_ERRORCONTACTING);

										lvi.iItem=data->index;
										lvi.iSubItem=2;
										lvi.pszText=(LPTSTR)str.c_str();
										ListView_SetItem(list, &lvi);
									}
								}

								if(data->list) {
									try {
										if(path::exists(data->tempfile)) path::remove(data->tempfile);
									}
									catch(exception &ex) {
										ExceptionBox(hwnd, ex, __FILE__, __LINE__);
									}
								}
							}
						}
					}
				}

				///////////////////////////////////////////////
				/// Cleanup

				for(vector<CURL*>::size_type i=0; i<handles.size(); i++) {
					curl_multi_remove_handle(multi, handles[i]);

					HandleData *data;
					curl_easy_getinfo(handles[i], CURLINFO_PRIVATE, &data);

					if(list && aborted && !data->finished) {
						const tstring str=LoadString(IDS_ABORTED);
						lvi.iItem=data->index;
						lvi.iSubItem=2;
						lvi.pszText=(LPTSTR)str.c_str();
						ListView_SetItem(list, &lvi);
					}

					if(data->fp) fclose(data->fp);

					if(data->list) {
						try {
							if(path::exists(data->tempfile)) path::remove(data->tempfile);
						}
						catch(exception &ex) {
							ExceptionBox(hwnd, ex, __FILE__, __LINE__);
						}
					}

					delete data;

					curl_easy_cleanup(handles[i]);
				}
			}

			if(progress) SendMessage(progress, PBM_SETPOS, (int)(total*100), 0);

			curl_multi_cleanup(multi);
			curl_global_cleanup();

			if(!aborted && updatepb && MessageBox(hwnd, IDS_PBUPDATETEXT, IDS_PBUPDATE, MB_ICONQUESTION|MB_YESNO)==IDYES)
				ShellExecute(NULL, NULL, g_homepage, NULL, NULL, SW_SHOWNORMAL);
		}

		HWND abort=GetDlgItem(hwnd, IDC_ABORT);
		HWND close=GetDlgItem(hwnd, IDC_CLOSE);

		SendMessage(hwnd, DM_SETDEFID, IDC_CLOSE, 0);
		SetFocus(close);

		EnableWindow(abort, FALSE);
		EnableWindow(close, TRUE);

		if(IsWindowVisible(hwnd)) {
			if(g_config.UpdateCountdown==0) {
				if(g_updater==(HWND)-1) EndDialog(hwnd, changes);
				else DestroyWindow(hwnd);
			}
			if(g_config.UpdateCountdown>0) {
				g_countdown=(unsigned short)g_config.UpdateCountdown;
				tstring s=boost::str(tformat(LoadString(IDS_CLOSEX))%g_countdown);
				SetDlgItemText(hwnd, IDC_CLOSE, s.c_str());

				SetTimer(hwnd, TIMER_COUNTDOWN, 1000, NULL);
			}
		}
		else if(g_updater==(HWND)-1) EndDialog(hwnd, changes);
		else DestroyWindow(hwnd);

		return changes;
	}

	int Process() {
		try {
			return this->_Process();
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

	void operator()() {
		this->Process();
	}
};


static UpdateThread g_ut;
static boost::shared_ptr<thread> g_updatethread;



static void UpdateLists_OnClose(HWND hwnd)
{
	g_ut.aborted=true;

	if(g_updater==(HWND)-1)
		EndDialog(hwnd, g_ut.changes);
	else
		DestroyWindow(hwnd);

} // End of UpdateLists_OnClose()



static void UpdateLists_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) 
{
	if(id==IDC_ABORT) 
	{
		TRACEI("[UpdateLists_OnCommand]    IDC_ABORT");
		EnableWindow(GetDlgItem(hwnd, IDC_ABORT), FALSE);
		g_ut.aborted=true;
	}
	else if(id==IDC_CLOSE) {
		TRACEI("[UpdateLists_OnCommand]    IDC_CLOSE");
		if(g_updater==(HWND)-1) EndDialog(hwnd, g_ut.changes);
		else DestroyWindow(hwnd);
	}

} // End of UpdateLists_OnCommand()



static void UpdateLists_OnDestroy(HWND hwnd) 
{
	TRACEI("[UpdateLists_OnDestroy]  > Entering routine.");

	TRACEI("[UpdateLists_OnDestroy]    loading lists");
	if(g_ut.changes>0) LoadLists(GetParent(hwnd));

	TRACEI("[UpdateLists_OnDestroy]    resetting update-thread");
	g_updatethread.reset();

	TRACEI("[UpdateLists_OnDestroy]    updating list columns");
	HWND list=GetDlgItem(hwnd, IDC_LIST);
	SaveListColumns(list, g_config.UpdateColumns);

	SaveWindowPosition(hwnd, g_config.UpdateWindowPos);

	g_updater=NULL;
	TRACEI("[UpdateLists_OnDestroy]  < Leaving routine.");

} // End of UpdateLists_OnDestroy()



static void UpdateLists_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) {
	RECT rc={0};
	rc.right=268;
	rc.bottom=185;

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



static void UpdateLists_OnSize(HWND hwnd, UINT state, int cx, int cy);
static BOOL UpdateLists_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) 
{
	TRACEI("[UpdateLists_OnInitDialog]  > Entering routine.");
	HWND list=GetDlgItem(hwnd, IDC_LIST);
	ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP);

	InsertColumn(list, 0, g_config.UpdateColumns[0], IDS_DESCRIPTION);
	InsertColumn(list, 1, g_config.UpdateColumns[1], IDS_TASK);
	InsertColumn(list, 2, g_config.UpdateColumns[2], IDS_STATUS);

	if(
		g_config.UpdateWindowPos.left!=0 || g_config.UpdateWindowPos.top!=0 ||
		g_config.UpdateWindowPos.right!=0 || g_config.UpdateWindowPos.bottom!=0
	) {
		SetWindowPos(hwnd, NULL,
			g_config.UpdateWindowPos.left,
			g_config.UpdateWindowPos.top,
			g_config.UpdateWindowPos.right-g_config.UpdateWindowPos.left,
			g_config.UpdateWindowPos.bottom-g_config.UpdateWindowPos.top,
			SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER
		);
	}

	RECT rc;
	GetClientRect(hwnd, &rc);
	UpdateLists_OnSize(hwnd, 0, rc.right, rc.bottom);

	TRACEI("[UpdateLists_OnInitDialog]    spawning updatethread");
	g_ut=UpdateThread(hwnd, list, GetDlgItem(hwnd, IDC_PROGRESS), false);
	g_updatethread=boost::shared_ptr<thread>(new thread(boost::ref(g_ut)));

	TRACEI("[UpdateLists_OnInitDialog]  < Leaving routine.");
	return TRUE;

} // End of UpdateLists_OnInitDialog()



static void UpdateLists_OnSize(HWND hwnd, UINT state, int cx, int cy) {
	HWND list=GetDlgItem(hwnd, IDC_LIST);
	HWND progress=GetDlgItem(hwnd, IDC_PROGRESS);
	HWND close=GetDlgItem(hwnd, IDC_CLOSE);
	HWND abort=GetDlgItem(hwnd, IDC_ABORT);

	RECT rc, prc;
	GetWindowRect(close, &rc);
	GetWindowRect(progress, &prc);

	HDWP dwp=BeginDeferWindowPos(4);

	int mid=cx/2-((rc.right-rc.left)*2+7)/2;

	DeferWindowPos(dwp, list, NULL, 7, 7, cx-14, cy-28-(rc.bottom-rc.top)-(prc.bottom-prc.top), SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, progress, NULL, 7, cy-14-(rc.bottom-rc.top)-(prc.bottom-prc.top), cx-14, prc.bottom-prc.top, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	DeferWindowPos(dwp, close, NULL, mid, cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);
	DeferWindowPos(dwp, abort, NULL, mid+7+(rc.right-rc.left), cy-7-(rc.bottom-rc.top), 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOSIZE);

	EndDeferWindowPos(dwp);

	if(state==SIZE_RESTORED) {
		SaveWindowPosition(hwnd, g_config.UpdateWindowPos);
	}
}



static void UpdateLists_OnTimer(HWND hwnd, UINT id) 
{
	TRACEI("[UpdateLists_OnTimer]  > Entering routine.");
	if(id==TIMER_COUNTDOWN) {
		if(!--g_countdown) {
			if(g_updater==(HWND)-1) EndDialog(hwnd, g_ut.changes);
			else DestroyWindow(hwnd);
			g_updater=NULL;
		}
		else {
			tstring s=boost::str(tformat(LoadString(IDS_CLOSEX))%g_countdown);
			SetDlgItemText(hwnd, IDC_CLOSE, s.c_str());
		}
	}

	TRACEI("[UpdateLists_OnTimer]  < Leaving routine.");

} // End of UpdateLists_OnTimer()



static INT_PTR CALLBACK UpdateLists_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_CLOSE, UpdateLists_OnClose);
			HANDLE_MSG(hwnd, WM_COMMAND, UpdateLists_OnCommand);
			HANDLE_MSG(hwnd, WM_DESTROY, UpdateLists_OnDestroy);
			HANDLE_MSG(hwnd, WM_GETMINMAXINFO, UpdateLists_OnGetMinMaxInfo);
			HANDLE_MSG(hwnd, WM_INITDIALOG, UpdateLists_OnInitDialog);
			HANDLE_MSG(hwnd, WM_SIZE, UpdateLists_OnSize);
			HANDLE_MSG(hwnd, WM_TIMER, UpdateLists_OnTimer);
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



int UpdateLists(HWND parent) 
{
	TRACEI("[UpdateLists]  > Entering routine.");

	int ret=0;

	if(g_updater==NULL) 
	{
		if(parent) 
		{
			// Either user-initiated, or else "update at startup"
			g_updater=(HWND)-1;
			TRACEI("[UpdateLists]    g_updater null, found parent, creating dialog-box");
			ret=(int)DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_UPDATELISTS), parent, UpdateLists_DlgProc);
			TRACEI("[UpdateLists]    dialog box finished running");
		}
		else 
		{
			// auto-updating
			TRACEI("[UpdateLists]    g_updater null, no parent, creating dialog-box");
			g_updater=CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_UPDATELISTS), parent, UpdateLists_DlgProc);
		}

		time(&g_config.LastUpdate);
	}
	else if(parent) 
	{	
		// restoring update-window?
		TRACEI("[UpdateLists]    g_updater not null, found parent, creating dialog-box");
		ShowWindow(g_updater, SW_SHOW);
		ShowWindow(g_updater, SW_RESTORE);
		SetForegroundWindow(g_updater);
	}
	else
	{
		// update-window stuck in "waiting for user to close" state, while we re-enter update routine?
		TRACEE("[UpdateLists]    g_updater not null, no parent, WTF??!?");
	}

	TRACEI("[UpdateLists]  < Leaving routine.");
	return ret;

} // End of UpdateLists()
