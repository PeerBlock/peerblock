/*
	Copyright (C) 2009-2010 PeerBlock, LLC

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

#include <boost\lockfree\fifo.hpp>
#include <string>
#include <windows.h>

using namespace std;

typedef enum TRACELOG_LEVEL {
	TRACELOG_LEVEL_NONE = 0,
	TRACELOG_LEVEL_CRITICAL,
	TRACELOG_LEVEL_ERROR,
	TRACELOG_LEVEL_WARNING,
	TRACELOG_LEVEL_SUCCESS,
	TRACELOG_LEVEL_INFO,
	TRACELOG_LEVEL_VERBOSE,
	TRACELOG_LEVEL_DEBUG,
	TRACELOG_LEVEL_MAX
} TracelogLevel;

typedef struct TRACELOG_ENTRY {
	tstring Message;
	TRACELOG_LEVEL Level;
	DWORD Tid;
} TracelogEntry;

#ifdef _NDEBUG
	static const TRACELOG_LEVEL TRACELOG_LEVEL_DEFAULT = TRACELOG_LEVEL_VERBOSE;
#else
	static const TRACELOG_LEVEL TRACELOG_LEVEL_DEFAULT = TRACELOG_LEVEL_INFO;
#endif

static const int TRACELOG_QUEUE_LENGTH = 20;


class TraceLog {

private:
	bool HaveLogfile;
	TracelogLevel LoggingLevel;
	tstring LogfileName;

	// TODO:  switch to wofstream or ofstream depending on unicode or not
	wofstream LogFile;

	boost::lockfree::fifo<TRACELOG_ENTRY *> MsgQueue;
	boost::lockfree::fifo<TRACELOG_ENTRY *> MsgFreelist;


public:
	HANDLE LoggingReady;
	HANDLE LoggingMutex; // TODO:  Change this to PG2 mutex-object

	void LogMessage(tstring messsage, TracelogLevel level);
	void LogErrorMessage(tstring location, tstring buf, DWORD err);
	void SetLogfile(tstring filename = _T("peerblock.log"));
	void SetLoglevel(TRACELOG_LEVEL level);
	void StartLogging();
	void StopLogging();

	void ProcessMessages();

	TraceLog();
	~TraceLog();
};


#define TRACEC(msg)		g_tlog.LogMessage(_T(msg), TRACELOG_LEVEL_CRITICAL);
#define TRACEE(msg)		g_tlog.LogMessage(_T(msg), TRACELOG_LEVEL_ERROR);
#define TRACEW(msg)		g_tlog.LogMessage(_T(msg), TRACELOG_LEVEL_WARNING);
#define TRACES(msg)		g_tlog.LogMessage(_T(msg), TRACELOG_LEVEL_SUCCESS);
#define TRACEI(msg)		g_tlog.LogMessage(_T(msg), TRACELOG_LEVEL_INFO);
#define TRACEV(msg)		g_tlog.LogMessage(_T(msg), TRACELOG_LEVEL_VERBOSE);
#define TRACED(msg)		g_tlog.LogMessage(_T(msg), TRACELOG_LEVEL_DEBUG);

#define TRACEBUFC(msg)	g_tlog.LogMessage(msg, TRACELOG_LEVEL_CRITICAL);
#define TRACEBUFE(msg)	g_tlog.LogMessage(msg, TRACELOG_LEVEL_ERROR);
#define TRACEBUFW(msg)	g_tlog.LogMessage(msg, TRACELOG_LEVEL_WARNING);
#define TRACEBUFS(msg)	g_tlog.LogMessage(msg, TRACELOG_LEVEL_SUCCESS);
#define TRACEBUFI(msg)	g_tlog.LogMessage(msg, TRACELOG_LEVEL_INFO);
#define TRACEBUFV(msg)	g_tlog.LogMessage(msg, TRACELOG_LEVEL_VERBOSE);
#define TRACEBUFD(msg)	g_tlog.LogMessage(msg, TRACELOG_LEVEL_DEBUG);

#define TRACEERR(loc, buf, err)		g_tlog.LogErrorMessage(_T(loc), buf, err);
