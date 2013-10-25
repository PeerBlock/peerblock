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

#include "tracelog.h"
extern TraceLog g_tlog;

static boost::shared_ptr<thread> g_thread;

class LoadingThread {
private:
	HWND hwnd, progress;
	LoadingData &data;
	bool aborted;

public:
	LoadingThread(HWND hwnd, HWND progress, LoadingData &data)
		: hwnd(hwnd), progress(progress), data(data),aborted(false) {}

	void operator()() {
		TRACEI("[LoadingThread] [operator()]  > Entering routine.");
		SendMessage(progress, PBM_SETRANGE32, 0, 1000);

		INT_PTR ret=0;

		try {
			double total=(double)data.InitFunc();

			for(unsigned int i=0; data.ProcessFunc(); i++) {
				if(aborted) {
					ret=1;
					break;
				}
				TRACEI("[LoadingThread] [operator()]    updating progress");
				SendMessage(progress, PBM_SETPOS, (WPARAM)(int)(i/total*1000.0), 0);
			}
		}
		catch(exception &ex) {
			UncaughtExceptionBox(hwnd, ex, __FILE__, __LINE__);
			EndDialog(hwnd, -1);
		}
		catch(...) {
			UncaughtExceptionBox(hwnd, __FILE__, __LINE__);
			EndDialog(hwnd, -1);
		}

		SendMessage(progress, PBM_SETPOS, 1000, 0);
		AnimateWindow(hwnd, 200, AW_BLEND|AW_HIDE);

		TRACEI("[LoadingThread] [operator()]  < Leaving routine.");
		EndDialog(hwnd, ret);
	}

	bool Abort() {
		TRACEW("[LoadingThread] [Abort]  Processing Abort!");
		if(!aborted) aborted=(data.AbortFunc && data.AbortFunc());
		return aborted;
	}
};

static boost::shared_ptr<LoadingThread> g_funcs;

static void Loading_OnClose(HWND hwnd) {
	g_funcs->Abort();
}

static void Loading_OnDestroy(HWND hwnd) {
	g_thread=boost::shared_ptr<thread>();
	g_funcs=boost::shared_ptr<LoadingThread>();
}

static BOOL Loading_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {

	TRACEI("[Loading_OnInitDialog]  > Entering routine.");

	LoadingData &data=*(LoadingData*)lParam;

	if(data.Title) SetWindowText(hwnd, data.Title);

	TRACEI("[Loading_OnInitDialog]    spawning thread to handle loading");
	g_funcs=boost::shared_ptr<LoadingThread>(new LoadingThread(hwnd, GetDlgItem(hwnd, IDC_PROGRESS), data));
	g_thread=boost::shared_ptr<thread>(new thread(boost::ref(*g_funcs)));

	TRACEI("[Loading_OnInitDialog]  < Leaving routine.");
	return TRUE;
}

INT_PTR CALLBACK Loading_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

//	TRACEI("[Loading_DlgProc]  Entering routine.");

	if (msg == WM_CLOSE)
	{
		TRACEI("[Loading_DlgProc]    msg: WM_CLOSE.");
	}
	else if (msg == WM_DESTROY)
	{
		TRACEI("[Loading_DlgProc]    msg: WM_DESTROY.");
	}
	else if (msg == WM_INITDIALOG)
	{
		TRACEI("[Loading_DlgProc]    msg: WM_INITDIALOG.");
	}

	try {
		switch(msg) {
			HANDLE_MSG(hwnd, WM_CLOSE, Loading_OnClose);
			HANDLE_MSG(hwnd, WM_DESTROY, Loading_OnDestroy);
			HANDLE_MSG(hwnd, WM_INITDIALOG, Loading_OnInitDialog);
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
