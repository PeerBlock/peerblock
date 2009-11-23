/*
	Copyright (C) 2007 Cory Nelson

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

#include <cstddef>
#include <cstring>
#include <cwchar>

#include <string>
#include <stdexcept>
#include <boost/scoped_array.hpp>

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>

#include "win32_error.hpp"

/*
	path performs path manipulation.
	such as L"dir" / L"file.txt" will build L"dir\\file.txt"
*/
class path {
public:
	path() { m_buf[0] = L'\0'; }

	path(const wchar_t *p) {
		*this = p;
	}

	path(const std::wstring &p) {
		*this = p;
	}

	path& operator=(const wchar_t *p) {
		std::size_t len = wcslen(p) + 1;

		if(len <= MAX_PATH) {
			wmemcpy(m_buf, p, len);
		}
		else {
			throw std::length_error("path length larger than MAX_PATH");
		}

		return *this;
	}

	path& operator=(const std::wstring &p) {
		std::size_t len = p.length() + 1;

		if(len <= MAX_PATH) {
			wmemcpy(m_buf, p.c_str(), len);
		}
		else {
			throw std::length_error("path length larger than MAX_PATH");
		}

		return *this;
	}

	const wchar_t* c_str() const { return m_buf; }

	bool empty() const { return (m_buf[0] == 0); }

	bool is_root() const { return PathIsRoot(m_buf) == TRUE; }
	bool is_relative() const { return PathIsRelative(m_buf) == TRUE; }

	bool is_url() const { return is_url(m_buf); }
	static bool is_url(const wchar_t *path) { return PathIsURL(path) == TRUE; }

	bool has_root() const { return !is_relative(); }

	void make_pretty() { PathMakePretty(m_buf); }

	void set_extension(const wchar_t *ext) { PathRenameExtension(m_buf, ext); }
	void remove_extension() { PathRemoveExtension(m_buf); }

	void normalize() {
		wchar_t buf[MAX_PATH];
		PathCanonicalize(buf, m_buf);

		wcscpy(m_buf, buf);
	}

	path& operator/=(const path &p) {
		PathAppend(m_buf, p.m_buf);
		return *this;
	}

	path operator/(const path &p) const {
		path n(*this);
		return n /= p;
	}

	bool operator==(const path &p) const {
		return !_wcsicmp(m_buf, p.m_buf);
	}

	bool operator!=(const path &p) const {
		return _wcsicmp(m_buf, p.m_buf) != 0;
	}

	std::wstring file_str() const {
		wchar_t buf[MAX_PATH];
		wcscpy(buf, m_buf);

		PathRemoveBackslash(buf);
		return buf;
	}

	std::wstring directory_str() const {
		wchar_t buf[MAX_PATH];
		wcscpy(buf, m_buf);

		PathAddBackslash(buf);
		return buf;
	}

	path root() const {
		wchar_t buf[MAX_PATH];
		wcscpy(buf, m_buf);

		PathStripToRoot(buf);
		return buf;
	}

	path without_root() const { return PathSkipRoot(m_buf); }

	path leaf() const {
		wchar_t buf[MAX_PATH];
		wcscpy(buf, m_buf);

		PathStripPath(buf);
		return buf;
	}

	path without_leaf() const { return (*this) / L".."; }

	static path relative_to(const path &from, const path &to, bool fromdir=true) {
		if(PathIsSameRoot(from.m_buf, to.m_buf)) {
			path p;

			if(!PathRelativePathTo(p.m_buf, from.m_buf, fromdir ? FILE_ATTRIBUTE_DIRECTORY : 0, to.m_buf, 0))
				throw win32_error("PathRelativePathTo");

			return p;
		}
		else if(to.has_root()) return to;
		else throw std::runtime_error("paths have different roots");
	}

	static bool exists(const path &p) { return PathFileExists(p.m_buf) == TRUE; }

	static void remove(const path &p) {
		if(!DeleteFile(p.m_buf)) {
			throw win32_error("DeleteFile");
		}
	}

	static void move(const path &from, const path &to, bool replace = false) {
		if(!MoveFileEx(from.m_buf, to.m_buf, replace ? MOVEFILE_REPLACE_EXISTING : 0))
			throw win32_error("MoveFileEx");
	}

	static void create_directory(const path &p) {
		if(!CreateDirectory(p.directory_str().c_str(), NULL))
			throw win32_error("CreateDirectory");
	}

	static path base() {
		wchar_t buf[MAX_PATH];
		GetModuleFileName(NULL, buf, MAX_PATH);

		return buf;
	}

	static path base_dir() { return base().without_leaf(); }
	static path appdata_dir() { return special_dir(CSIDL_APPDATA); }
	static path common_appdata_dir() { return special_dir(CSIDL_COMMON_APPDATA); }

	static path temp_dir() {
		wchar_t buf[MAX_PATH];

		DWORD ret = GetTempPath(MAX_PATH, buf);
		if(!ret) throw win32_error("GetTempPath");

		return buf;
	}

	// generates a random 8.3 ascii filename.
	static path temp_file(const wchar_t *prefix, path dir = temp_dir()) {
		wchar_t buf[MAX_PATH];

		UINT ret = GetTempFileName(dir.c_str(), prefix, 0, buf);
		if(!ret) throw win32_error("GetTempFileName");

		return buf;
	}

	static std::wstring toupper(const std::wstring &p) {
		int len = LCMapString(LOCALE_INVARIANT, LCMAP_UPPERCASE, p.c_str(), (int)p.length(), NULL, 0);
		if(!len) throw win32_error("LCMapString");

		boost::scoped_array<wchar_t> buf(new wchar_t[len]);

		len = LCMapString(LOCALE_INVARIANT, LCMAP_UPPERCASE, p.c_str(), (int)p.length(), buf.get(), len);
		if(!len) throw win32_error("LCMapString");

		return std::wstring(buf.get(), len);
	}

private:
	static path special_dir(int dir) {
		wchar_t buf[MAX_PATH];
		SHGetFolderPath(NULL, dir, NULL, SHGFP_TYPE_CURRENT, buf);

		return buf;
	}

	wchar_t m_buf[MAX_PATH];
};
