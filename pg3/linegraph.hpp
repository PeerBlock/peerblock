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

#include <windows.h>

#define LINEGRAPH_CLASS L"LineGraph"

#define WM_LINEGRAPH_SETFORMAT		(WM_USER+1)
#define WM_LINEGRAPH_GETFORMAT		(WM_USER+2)
#define WM_LINEGRAPH_ADDLINE			(WM_USER+3)
#define WM_LINEGRAPH_SETLINE			(WM_USER+4)
#define WM_LINEGRAPH_GETLINE			(WM_USER+5)
#define WM_LINEGRAPH_GETLINECOUNT	(WM_USER+6)
#define WM_LINEGRAPH_REMLINE			(WM_USER+7)
#define WM_LINEGRAPH_CLEARLINES		(WM_USER+8)
#define WM_LINEGRAPH_ADDSTEP			(WM_USER+9)
#define WM_LINEGRAPH_SETSTEP			(WM_USER+10)
#define WM_LINEGRAPH_GETSTEP			(WM_USER+11)
#define WM_LINEGRAPH_GETSTEPCOUNT	(WM_USER+12)
#define WM_LINEGRAPH_REMSTEP			(WM_USER+13)
#define WM_LINEGRAPH_CLEARSTEPS		(WM_USER+14)

/* used in LINEGRAPH_FORMAT dwFlags */
#define LINEGRAPH_FLAG_ALIGN			(1)
#define LINEGRAPH_FLAG_BGCOLOR		(1<<1)
#define LINEGRAPH_FLAG_MAXSTEPS		(1<<2)
#define LINEGRAPH_FLAG_FONT			(1<<3)

/* used in LINEGRAPH_FORMAT dwAlign */
#define LINEGRAPH_ALIGN_LEFT			(0)
#define LINEGRAPH_ALIGN_RIGHT			(1)

/* used in LINEGRAPH_LINE dwFlags */
/* LINEGRAPH_FLAG_BGCOLOR can also be used */
#define LINEGRAPH_FLAG_WIDTH			(1<<4)
#define LINEGRAPH_FLAG_POINTSIZE		(1<<5)
#define LINEGRAPH_FLAG_LABEL			(1<<6)
#define LINEGRAPH_FLAG_COLOR			(1<<7)
#define LINEGRAPH_FLAG_POINTCOLOR	(1<<8)
#define LINEGRAPH_FLAG_TEXTCOLOR		(1<<9)
#define LINEGRAPH_FLAG_SMOOTH			(1<<10)
#define LINEGRAPH_FLAG_DRAWBG			(1<<11)
#define LINEGRAPH_FLAG_DRAWPOINTS	(1<<12)

/* used in LINEGRAPH_STEP dwFlags */
/* LINEGRAPH_FLAG_LABEL can also be used */
#define LINEGRAPH_FLAG_STEP			(1<<13)

typedef struct TAG_LINEGRAPH_FORMAT {
	DWORD dwFlags;
	DWORD dwAlign;
	HFONT hFont;
	UINT uMaxSteps;
	COLORREF crBackgroundColor;
} LINEGRAPH_FORMAT;

typedef struct TAG_LINEGRAPH_LINE {
	DWORD dwFlags;
	double dWidth; // in points
	double dPointSize; // in points
	LPWSTR pszLabel;
	DWORD dwLabelMax;
	COLORREF crColor;
	COLORREF crBackgroundColor;
	COLORREF crPointColor;
	COLORREF crTextColor;
	BOOL bSmooth;
	BOOL bDrawBackground;
	BOOL bDrawPoints;
} LINEGRAPH_LINE;

typedef struct TAG_LINEGRAPH_STEP {
	DWORD dwFlags;
	double dStep;
	LPWSTR pszLabel;
	DWORD dwLabelMax;
	UINT uStep;
} LINEGRAPH_STEP;

#define LineGraph_SetFormat(hwndCtrl, lpFmt) SendMessage((hwndCtrl), WM_LINEGRAPH_SETFORMAT, 0, (LPARAM)(const LINEGRAPH_FORMAT*)(lpFmt))
#define LineGraph_GetFormat(hwndCtrl, lpFmt) SendMessage((hwndCtrl), WM_LINEGRAPH_GETFORMAT, 0, (LPARAM)(LINEGRAPH_FORMAT*)(lpFmt))
#define LineGraph_AddLine(hwndCtrl, lpLine) ((UINT)SendMessage((hwndCtrl), WM_LINEGRAPH_ADDLINE, 0, (LPARAM)(const LINEGRAPH_LINE*)(lpLine)))
#define LineGraph_SetLine(hwndCtrl, uLine, lpLine) ((UINT)SendMessage((hwndCtrl), WM_LINEGRAPH_SETLINE, (UINT)(uLine), (LPARAM)(const LINEGRAPH_LINE*)(lpLine)))
#define LineGraph_GetLine(hwndCtrl, uLine, lpLine) SendMessage((hwndCtrl), WM_LINEGRAPH_GETLINE, (UINT)(uLine), (LINEGRAPH_LINE*)(lpLine))
#define LineGraph_GetLineCount(hwndCtrl) ((UINT)SendMessage((hwndCtrl), WM_LINEGRAPH_GETLINECOUNT, 0, 0))
#define LineGraph_RemoveLine(hwndCtrl, uLine) SendMessage((hwndCtrl), WM_LINEGRAPH_REMLINE, (WPARAM)(UINT)(uLine), 0)
#define LineGraph_ClearLines(hwndCtrl) SendMessage((hwndCtrl), WM_LINEGRAPH_CLEARLINES, 0, 0)
#define LineGraph_AddStep(hwndCtrl, uLine, lpStep) ((UINT)SendMessage((hwndCtrl), WM_LINEGRAPH_ADDSTEP, (WPARAM)(UINT)(uLine), (LPARAM)(const LINEGRAPH_STEP*)(lpStep)))
#define LineGraph_SetStep(hwndCtrl, uLine, lpStep) SendMessage((hwndCtrl), WM_LINEGRAPH_SETSTEP, (WPARAM)(UINT)(uLine), (LPARAM)(const LINEGRAPH_STEP*)(lpStep))
#define LineGraph_GetStep(hwndCtrl, uLine, lpStep) SendMessage((hwndCtrl), WM_LINEGRAPH_GETSTEP, (WPARAM)(UINT)(uLine), (LPARAM)(LINEGRAPH_STEP*)(lpStep))
#define LineGraph_GetStepCount(hwndCtrl, uLine) ((UINT)SendMessage((hwndCtrl), WM_LINEGRAPH_GETSTEPCOUNT, (WPARAM)(UINT)(uLine), 0))
#define LineGraph_RemoveStep(hwndCtrl, uLine, uStep) SendMessage((hwndCtrl), WM_LINEGRAPH_REMSTEP, (WPARAM)(UINT)(uLine), (LPARAM)(UINT)(uStep))
#define LineGraph_ClearSteps(hwndCtrl, uLine) SendMessage((hwndCtrl), WM_LINEGRAPH_CLEARSTEPS, (WPARAM)(UINT)(uLine), 0)

void RegisterLineGraph();
