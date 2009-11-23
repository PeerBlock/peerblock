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

#include "stdafx.h"
#include "helpers.hpp"

HBITMAP LoadImage(HINSTANCE hInstance, LPCWSTR name) {
	CComPtr<IPicture> picture;
	{
		HRSRC res = FindResource(hInstance, name, L"BINARY");
		DWORD size = SizeofResource(hInstance, res);
		HGLOBAL resdata = LoadResource(hInstance, res);
		void *data = LockResource(resdata);

		CComPtr<IStream> stream(SHCreateMemStream((const BYTE*)data, size));
		OleLoadPicture(stream, 0, FALSE, IID_IPicture, (void**)&picture);

		UnlockResource(resdata);
		FreeResource(resdata);
	}

	HBITMAP bmp;
	picture->get_Handle((OLE_HANDLE*)&bmp);

	return bmp;
}

#pragma pack(push, 1)

typedef struct {
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
	WCHAR end[1];
} DLGTEMPLATEEX;

typedef struct {
	WORD pointsize;
	WORD weight;
	BYTE italic;
	BYTE charset;
	WCHAR typeface[1];
} DLGTEMPLATEEX2;

#pragma pack(pop)

static DLGTEMPLATE* RewriteDialogFont(HINSTANCE hInstance, LPCTSTR lpTemplate, LPCWSTR font, UINT points) {
	size_t fontlen = (wcslen(font) + 1) * sizeof(wchar_t);

	DLGTEMPLATE *ret;
	DWORD size;

	HRSRC res = FindResource(hInstance, lpTemplate, RT_DIALOG);
	if(!res) throw win32_error("FindResource");

	HGLOBAL resdata = LoadResource(hInstance, res);
	if(!resdata) throw win32_error("LoadResource");

	void *data = LockResource(resdata);

	size = SizeofResource(hInstance, res);

	ret = (DLGTEMPLATE*)GlobalAlloc(GPTR, size + fontlen);
	if(!ret) {
		win32_error err("GlobalAlloc");
		
		UnlockResource(resdata);
		FreeResource(resdata);

		throw err;
	}

	memcpy(ret, data, size);

	UnlockResource(resdata);
	FreeResource(resdata);

	DLGTEMPLATEEX *dlg = (DLGTEMPLATEEX*)ret;

	wchar_t *iter = dlg->end;

	// skip menu.
	if(*iter == 0xFFFF) iter += 2;
	else while(*iter++);
	
	// skip class.
	if(*iter == 0xFFFF) iter += 2;
	else while(*iter++);

	// skip title
	while(*iter++);

	DLGTEMPLATEEX2 *dlg2 = (DLGTEMPLATEEX2*)iter;

	// skip typeface.
	iter = dlg2->typeface;
	while(*iter++);

	// first item is 4-byte aligned after typeface.
	char *items = (char*)(((uintptr_t)iter + 3) & ~3);
	char *out = (char*)(((uintptr_t)dlg2->typeface + fontlen + 3) & ~3);

	// move stuff out of the way for new font.
	memmove(out, items, size - (items - (char*)dlg));

	// move new settings in.
	dlg2->pointsize = points;
	wcscpy(dlg2->typeface, font);

	return ret;
}

boost::shared_ptr<DLGTEMPLATE> MakeSysFontDialog(HINSTANCE hInstance, LPCTSTR lpTemplate) {
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);

	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);

	UINT size = PixelsToPoints((unsigned int)-ncm.lfMessageFont.lfHeight);

	DLGTEMPLATE *dlg = RewriteDialogFont(hInstance, lpTemplate, ncm.lfMessageFont.lfFaceName, size);
	return boost::shared_ptr<DLGTEMPLATE>(dlg, &GlobalFree);
}

INT_PTR SysFontDialogBox(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc) {
	return SysFontDialogBoxParam(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0);
}

INT_PTR SysFontDialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
	boost::shared_ptr<DLGTEMPLATE> dlg = MakeSysFontDialog(hInstance, lpTemplate);

	INT_PTR ret = DialogBoxIndirectParam(hInstance, dlg.get(), hWndParent, lpDialogFunc, dwInitParam);
	if(ret == -1) throw win32_error("DialogBoxIndirectParam");

	return ret;
}
