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

#include <cstddef>
#include <string>
#include <boost/shared_ptr.hpp>
#include <windows.h>
#include <commctrl.h>
#include "win32_error.hpp"

#pragma warning(push)
#pragma warning(disable:4244)

template<typename T>
static T* GetWindowStatePtr(HWND hwnd) {
	return (T*)(LONG_PTR)GetWindowLongPtr(hwnd, 0);
}

template<typename T>
static T* SetWindowStatePtr(HWND hwnd, T *ptr) {
	return (T*)(LONG_PTR)SetWindowLongPtr(hwnd, 0, (LONG_PTR)ptr);
}

template<typename T>
static T* GetDialogStatePtr(HWND hwnd) {
	return (T*)(LONG_PTR)GetWindowLongPtr(hwnd, DWLP_USER);
}

template<typename T>
static T* SetDialogStatePtr(HWND hwnd, T *ptr) {
	return (T*)(LONG_PTR)SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)ptr);
}

#pragma warning(pop)

static std::wstring LoadString(UINT id) {
	LPCTSTR buf = 0;

	int len = LoadString(GetModuleHandle(NULL), id, (LPTSTR)&buf, 0);
	if(!len) throw win32_error("LoadString");

	return std::wstring(buf, len);
}

static std::wstring GetWindowText(HWND hwnd) {
	std::wstring ret;
	
	int len = GetWindowTextLength(hwnd);
	if(len) {
		boost::scoped_array<wchar_t> buf(new wchar_t[++len]);
		len = GetWindowText(hwnd, buf.get(), len);

		ret.assign(buf.get(), len);
	}

	return ret;
}

static unsigned int LogPixelsX() {
	HDC hdc = GetDC(NULL);
	int res = GetDeviceCaps(hdc, LOGPIXELSX);
	ReleaseDC(NULL, hdc);

	return res;
}

static unsigned int PointsToPixels(unsigned int points) {
	return (unsigned int)((unsigned __int64)points * LogPixelsX() / 72u);
}

static unsigned int PixelsToPoints(unsigned int pixels) {
	return (unsigned int)(72ull * pixels / LogPixelsX());
}

static double PointsToPixels(double points) {
	return points * LogPixelsX() / 72.0;
}

static double PixelsToPoints(double pixels) {
	return 72.0 * pixels / LogPixelsX();
}

template<std::size_t Count>
static void InsertListViewColumns(HWND listview, const LVCOLUMN (&cols)[Count]) {
	for(std::size_t i = 0; i < Count; ++i) {
		ListView_InsertColumn(listview, (int)i, &cols[i]);
	}
}

template<std::size_t Count>
static void SetTabOrder(HWND hwnd, const int (&ctrls)[Count]) {
	HWND prev = GetDlgItem(hwnd, ctrls[0]);
	for(std::size_t i = 1; i < Count; ++i) {
		HWND ctrl = GetDlgItem(hwnd, ctrls[i]);

		SetWindowPos(ctrl, prev, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		prev = ctrl;
	}
}

HBITMAP LoadImage(HINSTANCE hInstance, LPCWSTR name);

/*
	The following is a hack that rewrites dialog resources to use the shell
	font, because "MS Shell Dlg" does not actually give the shell font on Vista!

	This allows correctly themed fonts everywhere.
*/

boost::shared_ptr<DLGTEMPLATE> MakeSysFontDialog(HINSTANCE hInstance, LPCTSTR lpTemplate);

INT_PTR SysFontDialogBox(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc);
INT_PTR SysFontDialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
