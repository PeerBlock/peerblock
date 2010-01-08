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

#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <ctime>
#include <windows.h>
#include "tstring.h"
#include "pathx.hpp"

struct List {
	enum ListType { Block, Allow };

	tstring Description;
	ListType Type;
	bool Enabled;

	List() : Enabled(true) {}
	virtual ~List() {}

	virtual path Path() const=0;
};

struct StaticList : List {
	path File;

	path Path() const { return File; }
};

struct DynamicList : List {
	tstring Url;
	time_t LastUpdate;
	bool FailedUpdate;

	DynamicList() : LastUpdate(0),FailedUpdate(false) {}

	path File() const;
	path TempFile() const;

	path Path() const { return File(); }

	bool operator==(const DynamicList &right)
	{
		return Url.find(right.Url) != string::npos;
	}
	bool operator==(const tstring &url)
	{
		return Url.find(url) != string::npos;
	}

	bool operator<(const DynamicList &right)
	{
		return Url < right.Url;
	}

	void Dump(TRACELOG_LEVEL _lvl)
	{
		tstring strBuf = boost::str(tformat(_T("[DynamicList] [Dump]    desc:[%1%] url:[%2%] enabled:[%3%] type:[%4%] updated:[%5%]")) 
			% Description % Url % Enabled % Type % LastUpdate);

		g_tlog.LogMessage(strBuf, _lvl);
	}
};

struct Color { COLORREF Text, Background; };

enum NotifyType { Never, OnBlock, OnHttpBlock };
enum CleanType { None, Delete, ArchiveDelete };

struct PortProfile 
{
	tstring Name;	// name of profile
	bool Enabled;	// true to use this profile
	std::set<ULONG> Ports;	// ports in profile
};

struct PortSet 
{
	// allow common protocols
	bool AllowHttp;    // 80, 443
	bool AllowFtp;     // 21
	bool AllowSmtp;    // 25
	bool AllowPop3;    // 110

	// all ports combined
	std::set<ULONG> Ports;	// will use all profiles marked as enabled

	std::vector<PortProfile> Profiles;	// list of profiles

	// merges all the enabled profiles
	void Merge() 
	{
		Ports.clear();

		{
			if (AllowHttp) {
				Ports.insert(80);
				Ports.insert(443);
			}
			if (AllowFtp)
				Ports.insert(21);
			if (AllowFtp)
				Ports.insert(25);
			if (AllowFtp)
				Ports.insert(110);
		}

		for (vector<PortProfile>::const_iterator iter = Profiles.begin(); iter != Profiles.end(); iter++) {
			PortProfile pp = (PortProfile) *iter;

			if (pp.Enabled) {
				for (set<ULONG>::const_iterator iter2 = pp.Ports.begin(); iter2 != pp.Ports.end(); iter2++) {
					Ports.insert((ULONG) *iter2);
				}
			}
		}

	} // End of PortSet::Merge()

}; // End of PortSet struct

struct Configuration {
	path ArchivePath;
	int HistoryColumns[6];
	int LogColumns[6];
	int ListEditorColumns[3];
	int ListManagerColumns[3];
	int UpdateColumns[3];
	std::vector<StaticList> StaticLists;
	std::vector<DynamicList> DynamicLists;
	PortSet PortSet;
	bool ColorCode;
	Color BlockedColor, AllowedColor, HttpColor;
	time_t LastUpdate, LastArchived;
	unsigned int CacheCrc;
	bool Block, BlockHttp, AllowLocal;
	bool UpdatePeerBlock, UpdateLists, UpdateAtStartup;
	bool StartMinimized, ShowSplash, HideOnClose, StayHidden;
	bool LogAllowed, LogBlocked, ShowAllowed;
	bool FirstBlock, FirstHide;
	NotifyType BlinkOnBlock, NotifyOnBlock;
	unsigned short UpdateInterval, LogSize, CleanupInterval;
	short UpdateCountdown;
	CleanType CleanupType;
	tstring UpdateProxy;
	long UpdateProxyType;
	// 0 = never cleanup
	// >0 = size in bytes
	__int64 MaxHistorySize;

	bool TracelogEnabled;
	int TracelogLevel;
	int LastVersionRun;

	RECT WindowPos, UpdateWindowPos, ListManagerWindowPos, ListEditorWindowPos, HistoryWindowPos, PortSetWindowPos;
	bool WindowHidden, AlwaysOnTop, HideTrayIcon;

	Configuration();

	bool Load();
	void Save();
	bool LoadFile(const TCHAR *file, HANDLE *fp, HANDLE *map, const void **view);
};

template<size_t len>
static void SaveListColumns(int const (&src)[len], int (&dest)[len]) {
	if(std::count_if(src, src+len, std::bind2nd(std::greater<int>(), 0))>0)
		std::copy(src, src+len, dest);
}

template<size_t len>
static void SaveListColumns(HWND list, int (&dest)[len]) {
	int cols[len];

	for(size_t i=0; i<len; i++)
		cols[i]=ListView_GetColumnWidth(list, i);

	SaveListColumns(cols, dest);
}

// saves a window position to a RECT
// hwnd - the handle to the window
// rect - the RECT to save the values to. Usually g_config.*WindowPos.
static void SaveWindowPosition(HWND hwnd, RECT &rect)
{
	RECT rc;
	GetWindowRect(hwnd, &rc);

	if(rc.left >= 0 && rc.top >= 0 && rc.right >= 0 && rc.bottom >= 0)
	{
		rect = rc;
	}
}

extern Configuration g_config;
