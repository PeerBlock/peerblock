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

#include "stdafx.h"
#include "resource.h"
#include "tracelog.h"
using namespace std;

extern TraceLog g_tlog;

int MessageBox(HWND hwnd, const tstring &text, const tstring &caption, UINT type) {
	return MessageBox(hwnd, text.c_str(), caption.c_str(), type);
}

int MessageBox(HWND hwnd, const tstring &text, UINT captionid, UINT type) {
	return MessageBox(hwnd, text, LoadString(captionid), type);
}

int MessageBox(HWND hwnd, UINT textid, UINT captionid, UINT type) {
	return MessageBox(hwnd, LoadString(textid), LoadString(captionid), type);
}

#ifndef PB_REPORT_BUGS

#define ReportException(x,y,z)

#else

static const char *g_bughost="bugs.phoenixlabs.org";
static const char *g_bugport="50005";



// TODO:  Break this out into a wrapper around a base function that simply accepts a string input.
//        This way, we will be able to report whatever we want, regardless of whether it's a
//        complete exception or not.  Need to check on the code that's actually handling these
//        sorts of problem-reports, to see what it's expecting.
static void ReportException(const exception *ex, const char *file, int line)
{
/*	TRACEC("Reporting exception to bugs.phoenixlabs.org:50005");
	string packet;
	{
		ostringstream buf;

		buf
			<< g_build
			<< '\n'
			<< file
			<< '\n'
			<< line;

		if(ex) {
			buf
				<< '\n'
				<< typeid(*ex).name()
				<< '\n'
				<< MBS_UTF8(ex->what());
		}

		packet=buf.str();
	}

	SOCKET sock=INVALID_SOCKET;
	{
		addrinfo hints={0};
		hints.ai_socktype=SOCK_DGRAM;
		hints.ai_protocol=IPPROTO_UDP;

		addrinfo *info;
		if(getaddrinfo(g_bughost, g_bugport, &hints, &info)!=0) return;

		for(const addrinfo *iter=info; iter!=NULL; iter=iter->ai_next) {
			sock=socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
			if(sock==INVALID_SOCKET) continue;

			if(connect(sock, iter->ai_addr, (socklen_t)iter->ai_addrlen)!=SOCKET_ERROR) {
				break;
			}
			else {
				closesocket(sock);
				sock=INVALID_SOCKET;
			}
		}

		freeaddrinfo(info);
	}

	if(sock==INVALID_SOCKET) return;

	send(sock, packet.c_str(), (socklen_t)packet.size(), 0);
	closesocket(sock);*/
}

#endif



void PeerBlockExceptionBox(HWND _hwnd, const peerblock_error &_ex)
{
//	ReportException(&ex, file, line);

	tstring str=boost::str(tformat(LoadString(_ex.m_textId)));
	TRACEC("    vvvv  EXCEPTION!!  vvvv");
	TRACEBUFC(str.c_str());
	TRACEC("    ^^^^  EXCEPTION!!  ^^^^");
	MessageBox(_hwnd, str, _ex.m_codeId, MB_ICONERROR|MB_OK);
}



void ExceptionBox(HWND hwnd, const exception &ex, const char *file, int line) {
	ReportException(&ex, file, line);

	tstring str=boost::str(tformat(LoadString(IDS_EXCEPTIONTEXT))%g_build%file%line%typeid(ex).name()%ex.what());
	TRACEC("    vvvv  EXCEPTION!!  vvvv");
	TRACEBUFC(str.c_str());
	TRACEC("    ^^^^  EXCEPTION!!  ^^^^");
	MessageBox(hwnd, str, IDS_EXCEPTION, MB_ICONERROR|MB_OK);
}



void UncaughtExceptionBox(HWND hwnd, const char *file, int line) {
	ReportException(NULL, file, line);

	tstring str=boost::str(tformat(LoadString(IDS_CAUGHTUNKNOWNTEXT))%g_build%file%line);
	TRACEC("    vvvv  EXCEPTION!!  vvvv");
	TRACEBUFC(str.c_str());
	TRACEC("    ^^^^  EXCEPTION!!  ^^^^");
	MessageBox(hwnd, str, IDS_UNCAUGHT, MB_ICONERROR|MB_OK);
}



void UncaughtExceptionBox(HWND hwnd, const exception &ex, const char *file, int line) {
	ReportException(&ex, file, line);

	tstring str;

	if(const win32_error *err = dynamic_cast<const win32_error*>(&ex)) {
		str=boost::str(tformat(LoadString(IDS_UNCAUGHTWIN32TEXT)) % g_build % file % line % err->func() % err->error() % err->what());
	}
	else {
		str=boost::str(tformat(LoadString(IDS_UNCAUGHTTEXT)) % g_build % file % line % typeid(ex).name() % ex.what());
	}

	TRACEC("    vvvv  EXCEPTION!!  vvvv");
	TRACEBUFC(str.c_str());
	TRACEC("    ^^^^  EXCEPTION!!  ^^^^");
	MessageBox(hwnd, str, IDS_UNCAUGHT, MB_ICONERROR|MB_OK);
}
