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

#pragma once

#include <cstring>
#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <strsafe.h>
#include "tstring.h"
#include "win32_error.h"

class path_error : public win32_error {
public:
	path_error(const char *func) : win32_error(func) {}
};

class path {
private:
	TCHAR buf[MAX_PATH];

public:
	path() { buf[0]=_T('\0'); }
	path(LPCTSTR p) { StringCbCopy(this->buf, sizeof(this->buf), p); }
	path(const tstring &p) { StringCbCopy(this->buf, sizeof(this->buf), p.c_str()); }

	LPCTSTR c_str() const { return this->buf; }

	bool empty() const { return (this->buf[0]==0); }

	bool is_root() const { return PathIsRoot(this->buf)==TRUE; }
	bool is_relative() const { return PathIsRelative(this->buf)==TRUE; }

	bool is_url() const { return PathIsURL(this->buf)==TRUE; }

	bool has_root() const { return !this->is_relative(); }

	void make_pretty() { PathMakePretty(this->buf); }
	void normalize() {
		TCHAR buf[MAX_PATH];
		PathCanonicalize(buf, this->buf);

		memcpy(this->buf, buf, sizeof(buf));
	}

	path& operator/=(const path &p) {
		PathAppend(this->buf, p.buf);
		return *this;
	}

	path operator/(const path &p) const {
		path n(*this);
		return n/=p;
	}

	bool operator==(const path &p) const {
		return !_tcsicmp(this->buf, p.buf);
	}

	bool operator!=(const path &p) const {
		return _tcsicmp(this->buf, p.buf)!=0;
	}

	tstring file_str() const {
		TCHAR buf[MAX_PATH];
		memcpy(buf, this->buf, sizeof(this->buf));

		PathRemoveBackslash(buf);
		return buf;
	}

	tstring directory_str() const {
		TCHAR buf[MAX_PATH];
		memcpy(buf, this->buf, sizeof(this->buf));

		PathAddBackslash(buf);
		return buf;
	}

	path root() const {
		TCHAR buf[MAX_PATH];
		memcpy(buf, this->buf, sizeof(this->buf));

		PathStripToRoot(buf);
		return buf;
	}

	path without_root() const { return PathSkipRoot(this->buf); }

	path leaf() const {
		TCHAR buf[MAX_PATH];
		memcpy(buf, this->buf, sizeof(this->buf));

		PathStripPath(buf);
		return buf;
	}

	path without_leaf() const { return (*this)/_T(".."); }

	static path relative_to(const path &from, const path &to, bool fromdir=true) {
		if(PathIsSameRoot(from.buf, to.buf)) {
			path p;

			if(!PathRelativePathTo(p.buf, from.buf, fromdir?FILE_ATTRIBUTE_DIRECTORY:0, to.buf, 0))
			{
				TCHAR chBuf[1024];
				swprintf_s(chBuf, sizeof(chBuf)/2, L"PathRelativePathTo() from:[%s] to:[%s] failed!!", from.buf, to.buf);
				TRACEERR("[path] [relative_to]", chBuf, GetLastError());
				throw path_error("PathRelativePathTo");
			}

			return p;
		}
		else if(to.has_root()) return to;
		else
		{
			TRACEE("[path] [relative_to]  * ERROR:  unable to create relative path: paths have different roots!!");
			throw std::runtime_error("unable to create relative path: paths have different roots");
		}
	}

	static bool exists(const path &p) { return PathFileExists(p.buf)==TRUE; }

	static void remove(const path &p) {
		if(exists(p) && !DeleteFile(p.buf))
		{
			TCHAR chBuf[1024];
			swprintf_s(chBuf, sizeof(chBuf)/2, L"Can't delete file [%s]!!", p.buf);
			TRACEERR("[path] [remove]", chBuf, GetLastError());
			throw path_error("DeleteFile");
		}
	}

	static void move(const path &from, const path &to, bool replace=false) {
#ifdef _WIN32_WINNT
		if(!MoveFileEx(from.buf, to.buf, replace?MOVEFILE_REPLACE_EXISTING:0))
		{
			TCHAR chBuf[1024];
			swprintf_s(chBuf, sizeof(chBuf)/2, L"Can't move file from:[%s] to:[%s]!!", from.buf, to.buf);
			TRACEERR("[path] [move]", chBuf, GetLastError());
			throw path_error("MoveFileEx");
		}
#else
		if(replace && exists(to)) remove(to);
		if(!MoveFile(from.buf, to.buf))
		{
			TCHAR chBuf[1024];
			swprintf_s(chBuf, sizeof(chBuf)/2, L"Can't move file from:[%s] to:[%s]!!", from.buf, to.buf);
			TRACEERR("[path] [move]", chBuf, GetLastError());
			throw path_error("MoveFile");
		}
#endif
	}

	static void copy(const path &from, const path &to, bool replace=false)
	{
		if(replace && exists(to)) remove(to);
		if(!CopyFile(from.buf, to.buf, true))
		{
			TCHAR chBuf[2048];
			swprintf_s(chBuf, sizeof(chBuf)/2, L"Can't copy file from:[%s] to:[%s]!!", from.buf, to.buf);
			TRACEERR("[path] [copy]", chBuf, GetLastError());
			throw path_error("CopyFile");
		}
	}

	static void create_directory(const path &p) {
		if(!CreateDirectory(p.directory_str().c_str(), NULL))
		{
			TCHAR chBuf[2048];
			swprintf_s(chBuf, sizeof(chBuf)/2, L"Can't create directory:[%s]!!", p.buf);
			TRACEERR("[path] [create_directory]", chBuf, GetLastError());
			throw path_error("CreateDirectory");
		}
	}

	static path base() {
		TCHAR buf[MAX_PATH];
		GetModuleFileName(NULL, buf, MAX_PATH);

		return buf;
	}

	static path base_dir() { return base().without_leaf(); }
};
