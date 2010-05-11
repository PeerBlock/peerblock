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

*/

#include "stdafx.h"
using namespace std;

static BOOL ColorPicker_OnCreate(HWND hwnd, LPCREATESTRUCT) {
	SetWindowLong(hwnd, 0, (LONG)RGB(255,255,255));
	return TRUE;
}

static void ColorPicker_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags) {
	COLORREF presets[16];
	fill(presets, presets+16, RGB(255,255,255));

	CHOOSECOLOR cc={0};
	cc.lStructSize=sizeof(cc);
	cc.hwndOwner=hwnd;
	cc.rgbResult=(COLORREF)GetWindowLong(hwnd, 0);
	cc.lpCustColors=presets;
	cc.Flags=CC_FULLOPEN|CC_RGBINIT|CC_SOLIDCOLOR;

	if(ChooseColor(&cc)) {
		SetWindowLong(hwnd, 0, (LONG)cc.rgbResult);
		InvalidateRect(hwnd, NULL, FALSE);

		CPNM_SETCOLOR sc;
		sc.nmh.code=CPN_SETCOLOR;
		sc.nmh.hwndFrom=hwnd;
		sc.nmh.idFrom=GetDlgCtrlID(hwnd);
		sc.color=cc.rgbResult;

		SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)sc.nmh.idFrom, (LPARAM)&sc);
	}
}

static void ColorPicker_OnPaint(HWND hwnd) {
	RECT rc;
	GetClientRect(hwnd, &rc);

	HBRUSH brush=CreateSolidBrush((COLORREF)GetWindowLong(hwnd, 0));

	PAINTSTRUCT ps={0};
	HDC hdc=BeginPaint(hwnd, &ps);

	FillRect(hdc, &rc, brush);

	EndPaint(hwnd, &ps);

	DeleteObject((HGDIOBJ)brush);
}

static COLORREF ColorPicker_OnGetColor(HWND hwnd) {
	return (COLORREF)GetWindowLong(hwnd, 0);
}

static COLORREF ColorPicker_OnSetColor(HWND hwnd, COLORREF color) {
	COLORREF cr=(COLORREF)SetWindowLong(hwnd, 0, (LONG)color);

	InvalidateRect(hwnd, NULL, FALSE);

	return cr;
}

static LRESULT CALLBACK ColorPicker_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CREATE, ColorPicker_OnCreate);
		HANDLE_MSG(hwnd, WM_LBUTTONUP, ColorPicker_OnLButtonUp);
		HANDLE_MSG(hwnd, WM_PAINT, ColorPicker_OnPaint);
		case CPM_GETCOLOR: return (LRESULT)ColorPicker_OnGetColor(hwnd);
		case CPM_SETCOLOR: return (LRESULT)ColorPicker_OnSetColor(hwnd, (COLORREF)wParam);
		default: return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

bool RegisterColorPicker(HINSTANCE hInstance) {
	WNDCLASS wc={0};
	wc.lpszClassName=PB_COLORPICKER_CLASS;
	wc.hInstance=hInstance;
	wc.lpfnWndProc=ColorPicker_WndProc;
	wc.hCursor=LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra=sizeof(COLORREF);

	return (RegisterClass(&wc)!=0);
}
