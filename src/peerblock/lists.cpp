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

#include <mutex>
using namespace std;

#include "tracelog.h"
extern TraceLog g_tlog;

p2p::list g_tempallow, g_tempblock;

enum FileType { File_Unknown, File_Zip, File_Gzip, File_7zip };

static FileType GetType(const path &file) {
	FILE *fp=_tfopen(file.file_str().c_str(), _T("rb"));
	if(!fp) throw runtime_error("file does not exist");

	unsigned char buf[6];
	size_t read=fread(buf, sizeof(unsigned char), 6, fp);
	fclose(fp);

	if(read>=2) {
		if(buf[0]==0x1F && buf[1]==0x8B) return File_Gzip;
		if(read>=4) {
			if(!memcmp(buf, "PK\x03\x04", 4)) return File_Zip;
			if(read==6 && !memcmp(buf, "7z\xBC\xAF\x27\x1C", 6)) return File_7zip;
		}
	}

	return File_Unknown;
}

static pair<boost::shared_array<char>,size_t> UngzipFile(const path &file) {
	int fd=_topen(file.file_str().c_str(), _O_RDONLY|_O_BINARY);
	if(fd==-1) throw zip_error("unable to open file");

	gzFile fp=gzdopen(fd, "rb");
	if(fp==NULL) {
		_close(fd);
		throw zip_error("gzopen");
	}

	vector<char> buf;

	char tmp[4096];
	int read;

	while((read=gzread(fp, tmp, sizeof(tmp)))) {
		if(read<0) {
			int e;
			zip_error err("gzread", gzerror(fp, &e));
			gzclose(fp);

			throw err;
		}
		copy(tmp, tmp+read, back_inserter(buf));
	}

	gzclose(fp);

	boost::shared_array<char> ret(new char[buf.size()]);
	copy(buf.begin(), buf.end(), ret.get());

	return make_pair(ret, buf.size());
}

#define kBufferSize 4096
static Byte g_Buffer[kBufferSize];

bool LoadList(path file, p2p::list &list)
{
	TRACEV("[LoadList]  > Entering routine.");
	if(!file.has_root()) file=path::base_dir()/file;
	if(!path::exists(file))
	{
		TRACEE("[LoadList]    ERROR: specified path doesn't exist!!");
		TRACEV("[LoadList]  < Leaving routine.");
		return false;
	}

	switch(GetType(file))
	{
		case File_Zip:
		{
			TRACEV("[LoadList]    found zip file");
			ZipFile zip(file);

			if(zip.GoToFirstFile()) {
				do {
					zip.OpenCurrentFile();

					pair<boost::shared_array<char>,size_t> buf=zip.ReadCurrentFile();

					zip.CloseCurrentFile();

					list.load(istrstream((const char*)buf.first.get(), (streamsize)buf.second));
				} while(zip.GoToNextFile());
			}
			TRACEV("[LoadList]    done with zip file");
		} break;

		case File_Gzip:
		{
			TRACEV("[LoadList]    found gzip file");
			pair<boost::shared_array<char>,size_t> buf=UngzipFile(file);

			list.load(istrstream((const char*)buf.first.get(), (streamsize)buf.second));
			TRACEV("[LoadList]    done with gzip file");
		} break;

		case File_7zip:
		{
			TRACEV("[LoadList]    found 7z file");
			CFileInStream is;
			CLookToRead lookStream;

			if(InFile_OpenW(&is.file, file.c_str()))
			{
				errno_t err = 0;
			    _get_errno(&err);
	   			tstring strBuf = boost::str(tformat(_T("[LoadList]  * ERROR:  [%1%] on _tfopen 7z file [%2%]"))
					% err % file.c_str());
				TRACEBUFE(strBuf);
				throw zip_error("unable to open file");
			}

			FileInStream_CreateVTable(&is);
			LookToRead_CreateVTable(&lookStream, False);

			lookStream.realStream = &is.s;
			LookToRead_Init(&lookStream);

			ISzAlloc ai;
			ai.Alloc=SzAlloc;
			ai.Free=SzFree;

			ISzAlloc aitemp;
			aitemp.Alloc=SzAllocTemp;
			aitemp.Free=SzFreeTemp;

			CSzArEx db;

			CrcGenerateTable();
			SzArEx_Init(&db);

			SRes res=SzArEx_Open(&db, &lookStream.s, &ai, &aitemp);
			if(res!=SZ_OK) {
				File_Close(&is.file);
	   			tstring strBuf = boost::str(tformat(_T("[LoadList]  * ERROR:  [%1%] on SzArchiveOpen 7z file [%2%]"))
					% res % file.c_str());
				TRACEBUFE(strBuf);
				throw zip_error("SzArchiveOpen");
			}

			UInt16 *filename = NULL;

			for(unsigned int i=0; i<db.db.NumFiles; i++) {
				char *outbuf = NULL;
				size_t bufsize = 0;
				size_t offset = 0, processed = 0;
				unsigned int index = 0xFFFFFFFF;

				const CSzFileItem *f = db.db.Files + i;
				if(f->IsDir) continue;

				size_t len = SzArEx_GetFileNameUtf16(&db, i, NULL);
				SzFree(NULL, filename);
				filename = (UInt16 *)SzAlloc(NULL, len * sizeof(filename[0]));
				SzArEx_GetFileNameUtf16(&db, i, filename);

				res=SzArEx_Extract(&db, &lookStream.s, i, &index, (unsigned char**)&outbuf, &bufsize, &offset, &processed, &ai, &aitemp);
				if(res!=SZ_OK) {
					SzArEx_Free(&db, &ai);
					SzFree(NULL, filename);
					File_Close(&is.file);
	   				tstring strBuf = boost::str(tformat(_T("[LoadList]  * ERROR:  [%1%] on SzExtract 7z file [%2%]"))
						% res % file.c_str());
					TRACEBUFE(strBuf);
					throw zip_error("SzExtract");
				}

				try {
					list.load(istrstream((const char*)outbuf + offset, (streamsize)processed));
				}
				catch(...) {
					IAlloc_Free(&ai, outbuf);
					TRACEV("[LoadList]  * ERROR:  Exception caught while performing 7z list.load()");
					throw;
				}

				IAlloc_Free(&ai,outbuf);
			}

			SzArEx_Free(&db, &ai);
			SzFree(NULL, filename);

			File_Close(&is.file);
			TRACEV("[LoadList]    done with 7z file");
		} break;

		case File_Unknown:
		{
			TRACEV("[LoadList]    found p2p/p2b file");
			HANDLE h=CreateFile(file.file_str().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if(h==INVALID_HANDLE_VALUE) throw win32_error("CreateFile");

			DWORD size=GetFileSize(h, NULL);
			if(size>0) {
				HANDLE m=CreateFileMapping(h, NULL, PAGE_READONLY, 0, 0, NULL);
				if(m==NULL) {
					win32_error ex("CreateFileMapping");

					CloseHandle(h);
					throw ex;
				}

				void *v=MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
				if(v==NULL) {
					win32_error ex("MapViewOfFile");

					CloseHandle(m);
					CloseHandle(h);
					throw ex;
				}

				try {
					list.load(istrstream((const char*)v, (streamsize)size));
				}
				catch(...) {
					UnmapViewOfFile(v);
					CloseHandle(m);
					CloseHandle(h);
					throw;
				}

				UnmapViewOfFile(v);
				CloseHandle(m);
			}
			CloseHandle(h);
			TRACEV("[LoadList]    done with found p2p/p2b file");
		} break;
		default: __assume(0);
	}

	TRACEV("[LoadList]  < Leaving routine.");
	return true;
}



//================================================================================================
//
//  FindUrl()
//
//    - Called by ???
//
/// <summary>
///   Checks for a list-url in the specified container.  Returns true if found, false if not.
/// </summary>
//
vector<DynamicList>::size_type FindUrl(tstring _url, vector<DynamicList> &_list)
{
	tstring strBuf = boost::str(tformat(_T("[lists] [FindUrl]    finding url:[%1%]")) % _url );
	TRACEBUFV(strBuf);

	for(vector<DynamicList>::size_type i=0; i<_list.size(); i++)
	{
		strBuf = boost::str(tformat(_T("[lists] [FindUrl]    checking listnum:[%1%] url:[%2%]")) % i % _list[i].Url);
		TRACEBUFV(strBuf);
		if (_list[i].Url.compare(_url) == 0)
		{
			strBuf = boost::str(tformat(_T("[lists] [FindUrl]    found listnum:[%1%] url:[%2%]")) % i % _list[i].Url);
			TRACEBUFV(strBuf);
			return i;
		}
	}

	return -1;

} // End of FindUrl()



class GenCacheFuncs {
private:
	HWND hwnd;
	vector<StaticList>::size_type i;
	bool stat, dyn, opt, save, needupdate;
	p2p::list allow;
	p2p::list &block;

	static void ListProblem(HWND hwnd, const path &p, const exception &ex) {
		const tstring text=boost::str(tformat(LoadString(IDS_LISTERRTEXT))%p.c_str()%typeid(ex).name()%ex.what());
		MessageBox(hwnd, text, IDS_LISTERR, MB_ICONERROR|MB_OK);
	}

public:
	GenCacheFuncs(HWND hwnd, p2p::list &work)
		: hwnd(hwnd),block(work),stat(false),dyn(false),opt(true),save(true),needupdate(false),i(0) {}

	int Init() {
		TRACEI("[GenCacheFuncs] [Init]  > Entering routine.");
		int len=2;

		TCHAR chBuf[128];
		_stprintf_s(chBuf, _countof(chBuf), _T("[GenCacheFuncs] [Init]    num static lists: [%Iu]"), g_config.StaticLists.size());
		g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_CRITICAL);
		_stprintf_s(chBuf, _countof(chBuf), _T("[GenCacheFuncs] [Init]    num dynamic lists: [%Iu]"), g_config.DynamicLists.size());
		g_tlog.LogMessage(chBuf, TRACELOG_LEVEL_CRITICAL);

		if(g_config.StaticLists.size()>0) {
			stat=true;
			for(vector<StaticList>::size_type i=0; i<g_config.StaticLists.size(); i++)
				if(g_config.StaticLists[i].Enabled) len++;
		}

		if(g_config.DynamicLists.size()>0) {
			dyn=true;
			for(vector<DynamicList>::size_type i=0; i<g_config.DynamicLists.size(); i++)
				if(g_config.DynamicLists[i].Enabled) len++;
		}

		TRACEI("[GenCacheFuncs] [Init]  < Leaving routine.");
		return len;
	}



	//============================================================================================
	//
	//  Process()
	//
	//    - Called by ???
	//
	/// <summary>
	///   Handles the (re)generation of the cache.
	/// </summary>
	//
	bool Process() {

		TRACEI("[GenCacheFuncs] [Process]  > Entering routine.");
		if(stat) {
			TRACEI("[GenCacheFuncs] [Process]    static list");
			if(g_config.StaticLists[i].Enabled) {
				TRACEI("[GenCacheFuncs] [Process]    list is enabled");
				p2p::list &l=(g_config.StaticLists[i].Type==List::Allow)?allow:block;

				try {
					TRACEI("[GenCacheFuncs] [Process]    loading list");
					if(!LoadList(g_config.StaticLists[i].File, l)) {
						TRACEE("[GenCacheFuncs] [Process]  ERROR loading list!");
						tstring str=boost::str(tformat(LoadString(IDS_FILENOTFOUNDTEXT))%g_config.StaticLists[i].File.file_str());
						MessageBox(hwnd, str, IDS_FILENOTFOUND, MB_ICONWARNING|MB_OK);
					}
					TRACEI("[GenCacheFuncs] [Process]    list loaded");
				}
				catch(exception &ex) {
					TRACEE("[GenCacheFuncs] [Process]    EXCEPTION caught while loading list!");
					ListProblem(hwnd, g_config.StaticLists[i].File, ex);

					if(!(stat=(++i < g_config.StaticLists.size()))) i=0;
					return true;
				}
			}
			if(!(stat=(++i < g_config.StaticLists.size())))
			{
				TRACEI("[GenCacheFuncs] [Process]    stat funkiness");
				i=0;
			}
		}
		else if(dyn) {
			TRACEI("[GenCacheFuncs] [Process]    dynamic list");
			if(g_config.DynamicLists[i].Enabled) {
				TRACEI("[GenCacheFuncs] [Process]    list is enabled");
				p2p::list &l=(g_config.DynamicLists[i].Type==List::Allow)?allow:block;

				try {
					TRACEI("[GenCacheFuncs] [Process]    loading list");
					if(!LoadList(g_config.DynamicLists[i].File(), l))
					{
						TRACEI("[GenCacheFuncs] [Process]    failure to load list; needs update");
						needupdate=true;
					}
					TRACEI("[GenCacheFuncs] [Process]    list loaded");
				}
				catch(exception &ex) {
					TRACEE("[GenCacheFuncs] [Process]    EXCEPTION caught while loading list!");
					ListProblem(hwnd, g_config.DynamicLists[i].Url, ex);

					if(!(dyn=(++i < g_config.DynamicLists.size()))) i=0;
					return true;
				}
			}
			if(!(dyn=(++i < g_config.DynamicLists.size()))) i=0;
		}
		else if(opt) {
			TRACEI("[GenCacheFuncs] [Process]    opt");
			if(block.size()>0) {
				if(allow.size()>0) block.erase(allow);
				if(block.size()>0) block.optimize();
			}
			opt=false;
		}
		else if(save) {
			TRACEI("[GenCacheFuncs] [Process]    save");
			block.save(TSTRING_MBS((path::base_dir()/_T("cache.p2b")).c_str()), p2p::list::file_p2b);
			save=false;
		}
		else {
			TRACEI("[GenCacheFuncs] [Process]    default");
			if(needupdate && hwnd)
			{
				TRACEI("[GenCacheFuncs] [Process]    displaying needs-update window");
				MessageBox(hwnd, IDS_NEEDUPDATETEXT, IDS_NEEDUPDATE, MB_ICONWARNING|MB_OK);
			}
			TRACEI("[GenCacheFuncs] [Process]  < Leaving routine.");
			return false;
		}

		TRACEI("[GenCacheFuncs] [Process]  < Leaving routine.");
		return true;
	}
};

static bool IsCacheValid() {
	boost::crc_32_type crc;

	for(vector<StaticList>::size_type i=0; i<g_config.StaticLists.size(); i++) {
		if(g_config.StaticLists[i].Enabled) {
			path file=g_config.StaticLists[i].File;
			if(!file.has_root()) file=path::base_dir()/file;

			HANDLE fp=CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if(fp!=INVALID_HANDLE_VALUE) {
				tstring str=file.c_str();
				crc.process_bytes(str.data(), str.size()*sizeof(TCHAR));
				crc.process_bytes(&g_config.StaticLists[i].Type, sizeof(List::ListType));

				FILETIME ft;
				if(GetFileTime(fp, NULL, NULL, &ft))
					crc.process_bytes(&ft, sizeof(ft));

				CloseHandle(fp);
			}
		}
	}

	for(vector<DynamicList>::size_type i=0; i<g_config.DynamicLists.size(); i++) {
		if(g_config.DynamicLists[i].Enabled) {
			HANDLE fp=CreateFile(g_config.DynamicLists[i].File().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if(fp!=INVALID_HANDLE_VALUE) {
				crc.process_bytes(g_config.DynamicLists[i].Url.c_str(), g_config.DynamicLists[i].Url.size()*sizeof(TCHAR));
				crc.process_bytes(&g_config.DynamicLists[i].Type, sizeof(List::ListType));

				FILETIME ft;
				if(GetFileTime(fp, NULL, NULL, &ft))
					crc.process_bytes(&ft, sizeof(ft));

				CloseHandle(fp);
			}
		}
	}

	unsigned int res=(unsigned int)crc.checksum();

	if(res!=g_config.CacheCrc) {
		g_config.CacheCrc=res;
		return false;
	}

	return path::exists(path::base_dir()/_T("cache.p2b"));
}

static bool GenCache(HWND hwnd, p2p::list &work) {

	TRACEI("[GenCache]  > Entering routine.");

	if(!IsCacheValid()) {

		TRACEI("[GenCache]    cache not valid");

		GenCacheFuncs funcs(hwnd, work);

		if(hwnd) {

			TRACEI("[GenCache]    no window, creating dialog box");

			tstring title=LoadString(IDS_GENCACHE);

			// setting up data to be used by Loading Thread
			LoadingData data;
			data.Title=title.c_str();
			data.InitFunc=boost::bind(&GenCacheFuncs::Init, &funcs);
			data.ProcessFunc=boost::bind(&GenCacheFuncs::Process, &funcs);

			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LOADING), hwnd, Loading_DlgProc, (LPARAM)&data);
		}
		else {
			TRACEI("[GenCache]    window found, spawning thread");
			funcs.Init();
			while(funcs.Process());
		}

		TRACEI("[GenCache]  < Leaving routine (true).");
		return true;
	}
	else
	{
		TRACEI("[GenCache]  < Leaving routine (false).");
		return false;
	}
}

void LoadLists(HWND parent) {

	TRACEI("[LoadLists]  > Entering routine.");

	p2p::list block;
    static std::mutex cacheLock;

	TRACEI("[LoadLists]    generating cache");
    {
        std::lock_guard<std::mutex> lock(cacheLock);
	    if(!GenCache(parent, block))
	    {
		    TRACEI("[LoadLists]    loading lists");
            try {
    		    LoadList(_T("cache.p2b"), block);
            } catch (p2p::p2p_error ex) {
        	    tstring str=boost::str(tformat(_T("[LoadLists]    error detected while parsing list-cache, will blow away cache.p2b and try again; ex:%1%")) 
                    % ex.what());
                TRACEBUFW(str.c_str());
                path::remove(path::base_dir()/_T("cache.p2b"));
    		    LoadList(_T("cache.p2b"), block);
            }
	    }
    }

	TRACEI("[LoadLists]    performing random setup");
	if(block.size()>0) {
		p2p::list allow;

		if(g_config.AllowLocal) {
			set<unsigned int> locals;
			GetLocalIps(locals, LOCALIP_ADAPTER|LOCALIP_GATEWAY|LOCALIP_DHCP|LOCALIP_DNS);

			for(set<unsigned int>::const_iterator iter=locals.begin(); iter!=locals.end(); ++iter)
				allow.insert(p2p::range(L"", *iter, *iter));
		}

		if(g_tempallow.size()>0) allow.insert(g_tempallow);

		if(allow.size()>0) block.erase(allow);
		if(block.size()>0) block.optimize();
	}

	TRACEI("[LoadLists]    setting up filter-driver ranges");
	g_filter->setranges(block, true);
	TRACEI("[LoadLists]    driver ranges set");

	SendMessage(g_tabs[0].Tab, WM_LOG_RANGES, 0, (UINT)g_filter->blockcount());

	TRACEI("[LoadLists]  < Exiting routine.");
}

path DynamicList::File() const {
	boost::crc_32_type crc;
	crc.process_bytes(this->Url.c_str(), this->Url.length()*sizeof(TCHAR));

	TCHAR buf[32];
	return path::base_dir()/_T("lists")/(_ultot((unsigned long)crc.checksum(), buf, 10)+tstring(_T(".list")));
}

path DynamicList::TempFile() const {
	boost::crc_32_type crc;
	crc.process_bytes(this->Url.c_str(), this->Url.length()*sizeof(TCHAR));

	TCHAR buf[32];
	return path::base_dir()/_T("lists")/(_ultot((unsigned long)crc.checksum(), buf, 10)+tstring(_T(".list.tmp")));
}
