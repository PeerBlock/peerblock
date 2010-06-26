/*
	Original code copyright (C) 2004-2005 Cory Nelson
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

#include "stdafx.h"
#include "resource.h"
using namespace std;
using namespace sqlite3x;

static ostream& operator<<(ostream &os, const SYSTEMTIME &st) {
	os << setfill('0')
		<< setw(4) << st.wYear << '-'
		<< setw(2) << st.wMonth << '-'
		<< setw(2) << st.wDay;

	return os;
}

static INT_PTR HistoryCalendar_OnNotify(HWND hwnd, int idCtrl, NMHDR *nmh) {
	if(nmh->code==MCN_SELECT && nmh->idFrom==IDC_CALENDAR) {
		UINT id=(UINT)GetDlgCtrlID(hwnd);

		HCNM_SELCHANGE hc;

		hc.hdr.code=HCN_SELCHANGE;
		hc.hdr.hwndFrom=hwnd;
		hc.hdr.idFrom=id;
		hc.stSelectedTime=((NMSELCHANGE*)nmh)->stSelStart;

		SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)id, (LPARAM)&hc);
	}
	else if(nmh->code==MCN_GETDAYSTATE && nmh->idFrom==IDC_CALENDAR) {
		static MONTHDAYSTATE mds[3];
		mds[0]=0;
		mds[1]=0;
		mds[2]=0;

		NMDAYSTATE *ds=(NMDAYSTATE*)nmh;
		ds->prgDayState=mds;

		sqlite3_try_lock lock(g_con);

		if(lock.is_locked()) {
			ostringstream ss;
			ss << "select distinct strftime('%m', time, 'localtime')-" << ds->stStart.wMonth << ",strftime('%d', time, 'localtime')-1 from t_history where action=1 and time >= julianday('" << ds->stStart << "', 'utc') and time < julianday('" << ds->stStart << "', '+3 months', 'utc');";

			const string query=ss.str();
			{
				sqlite3_command cmd(g_con, query.c_str());
				sqlite3_reader reader=cmd.executereader();

				while(reader.read()) {
					int m=reader.getint(0);
					int d=reader.getint(1);

					if(m>=0 && m<3 && d>=0 && d<31) mds[m]|=1<<d;
				}
			}
		}
	}

	return 0;
}

static void HistoryCalendar_OnSize(HWND hwnd, UINT state, int cx, int cy) {
	HWND calendar=GetDlgItem(hwnd, IDC_CALENDAR);

	RECT rc;
	MonthCal_GetMinReqRect(calendar, &rc);

	MoveWindow(calendar, 0, 0, cx, rc.bottom-rc.top, TRUE);
}

static void HistoryCalendar_OnGetCurSel(HWND hwnd, SYSTEMTIME *st) {
	HWND calendar=GetDlgItem(hwnd, IDC_CALENDAR);
	MonthCal_GetCurSel(calendar, st);
}

static void HistoryCalendar_OnSetCurSel(HWND hwnd, const SYSTEMTIME *st) {
	HWND calendar=GetDlgItem(hwnd, IDC_CALENDAR);
	MonthCal_SetCurSel(calendar, st);

	UINT id=(UINT)(UINT_PTR)GetMenu(hwnd);

	HCNM_SELCHANGE hc;

	hc.hdr.code=HCN_SELCHANGE;
	hc.hdr.hwndFrom=hwnd;
	hc.hdr.idFrom=id;
	hc.stSelectedTime=*st;

	SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)id, (LPARAM)&hc);
}

INT_PTR CALLBACK HistoryCalendar_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_NOTIFY, HistoryCalendar_OnNotify);
			HANDLE_MSG(hwnd, WM_SIZE, HistoryCalendar_OnSize);
			case HCM_GETCURSEL:
				HistoryCalendar_OnGetCurSel(hwnd, (SYSTEMTIME*)wParam);
				return 0;
			case HCM_SETCURSEL:
				HistoryCalendar_OnSetCurSel(hwnd, (const SYSTEMTIME*)wParam);
				return 0;
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
