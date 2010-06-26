/*
	Original code copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2010 PeerBlock, LLC

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

static const UINT DESTROY_TIMER=1;

static HGLOBAL LoadResource(HMODULE hModule, LPCTSTR lpResName, LPCTSTR type) {
	HRSRC res=FindResource(hModule, lpResName, type);
	DWORD size=SizeofResource(hModule, res);
	HGLOBAL resdata=LoadResource(hModule, res);
	void *data=LockResource(resdata);

	HGLOBAL clone=GlobalAlloc(GMEM_MOVEABLE, size);
	memcpy(GlobalLock(clone), data, size);
	GlobalUnlock(clone);

	UnlockResource(resdata);
	FreeResource(resdata);

	return clone;
}

static IPicture* LoadImage(LPCTSTR image) {
	HGLOBAL data=LoadResource(GetModuleHandle(NULL), image, _T("BINARY"));

	IStream *stream;
	CreateStreamOnHGlobal(data, TRUE, &stream);

	IPicture *picture;
	OleLoadPicture(stream, 0, FALSE, IID_IPicture, (void**)&picture);

	stream->Release();

	return picture;
}

static IPicture *g_img;

static void Splash_OnDestroy(HWND hwnd) {
	g_img->Release();
}

static const int HIMETRIC_PER_INCH=2540;

static BOOL Splash_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	boost::mt19937 rgen;
	rgen.seed((boost::uint32_t)time(NULL));

	g_img=LoadImage(MAKEINTRESOURCE(IDR_SPLASH));

	HDC dc=GetDC(NULL);
	int ppix = GetDeviceCaps(dc, LOGPIXELSX);
	int ppiy = GetDeviceCaps(dc, LOGPIXELSY);
	ReleaseDC(NULL, dc);

	int width, height;
	g_img->get_Width((OLE_XSIZE_HIMETRIC*)&width);
	g_img->get_Height((OLE_YSIZE_HIMETRIC*)&height);

	width=(ppix*width + HIMETRIC_PER_INCH/2)/HIMETRIC_PER_INCH;
	height=(ppix*height + HIMETRIC_PER_INCH/2)/HIMETRIC_PER_INCH;

	RECT rc;
	GetClientRect(GetDesktopWindow(), &rc);

	MoveWindow(hwnd, rc.right/2-width/2, rc.bottom/2-height/2, width, height, TRUE);
	MoveWindow(GetDlgItem(hwnd, IDC_PICTURE), 0, 0, width, height, TRUE);

	HANDLE bitmap;
	g_img->get_Handle((OLE_HANDLE*)&bitmap);

	SendDlgItemMessage(hwnd, IDC_PICTURE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);

#ifdef _WIN32_WINNT
	SetTimer(hwnd, DESTROY_TIMER, 1000, NULL);
#else
	SetTimer(hwnd, DESTROY_TIMER, 1500, NULL);
#endif

	return FALSE;
}

static void Splash_OnTimer(HWND hwnd, UINT id) {
	if(id==DESTROY_TIMER) {
#ifdef _WIN32_WINNT
		AnimateWindow(hwnd, 500, AW_HIDE|AW_BLEND);
#endif
		EndDialog(hwnd, 0);
	}
}

INT_PTR CALLBACK Splash_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_DESTROY, Splash_OnDestroy);
			HANDLE_MSG(hwnd, WM_INITDIALOG, Splash_OnInitDialog);
			HANDLE_MSG(hwnd, WM_TIMER, Splash_OnTimer);
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
