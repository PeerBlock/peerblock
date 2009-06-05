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
		$Revision: 1.30 $
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
};

struct Color { COLORREF Text, Background; };

enum NotifyType { Never, OnBlock, OnHttpBlock };
enum CleanType { None, Delete, ArchiveDelete };

struct Configuration {
	path ArchivePath;
	int HistoryColumns[6];
	int LogColumns[6];
	int ListEditorColumns[3];
	int ListManagerColumns[3];
	int UpdateColumns[3];
	std::vector<StaticList> StaticLists;
	std::vector<DynamicList> DynamicLists;
	bool ColorCode;
	Color BlockedColor, AllowedColor, HttpColor;
	time_t LastUpdate;
	unsigned int CacheCrc;
	bool Block, BlockHttp, AllowLocal;
	bool UpdatePeerGuardian, UpdateLists;
	bool StartMinimized, ShowSplash, HideOnClose, StayHidden;
	bool LogAllowed, LogBlocked, ShowAllowed;
	bool FirstBlock, FirstHide;
	NotifyType BlinkOnBlock, NotifyOnBlock;
	unsigned short UpdateInterval, LogSize, CleanupInterval;
	short UpdateCountdown;
	CleanType CleanupType;
	tstring UpdateProxy;
	long UpdateProxyType;

	RECT WindowPos, UpdateWindowPos, ListManagerWindowPos, ListEditorWindowPos, HistoryWindowPos;
	bool WindowHidden, AlwaysOnTop, HideTrayIcon;

	Configuration();

	bool Load();
	void Save();
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

extern Configuration g_config;
