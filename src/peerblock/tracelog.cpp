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

#include "stdafx.h"
#include "tracelog.h"


//////////---------------------------------------------------------------------
// Globals

//static TracelogThread g_tt;
static boost::shared_ptr<thread> g_tracelogthread;



//================================================================================================
//
//  ProcessMessages()
//
//    - Called by ThreadProc whenever we receive notice that new messages have arrived
//
/// <summary>
///   Takes all queued-up tracelog messages, formats them, and writes them out to the configured
///   logfile.
/// </summary>
//
void TraceLog::ProcessMessages()
{
	// reset "wake up" event


	//////////-----------------------------------------------------------------
	// Process all messages

	TracelogEntry * tlEnt = NULL;

	while (!MsgQueue.empty())
	{
		MsgQueue.dequeue(&tlEnt);

		// sanity-check
		if (NULL == tlEnt)
			continue;

		// re-format message, as needed

		// do we need to log this to-file?
		if (tlEnt->Level <= LoggingLevel && HaveLogfile)
		{
			// write it out to file
			tstring strLine = boost::str(tformat(_T("%1%\n")) % tlEnt->Message );
			LogFile << strLine;
		}

		// reset the tracelog-entry and stick it onto the free-list
		//tlEnt->Level = TRACELOG_LEVEL_MAX;
		//tlEnt->Message = _T("");
		MsgFreelist.enqueue(tlEnt);
	}

	LogFile.flush();


	//////////-----------------------------------------------------------------
	// Done!

	return;

} // End of ProcessMessages()



//================================================================================================
//
//  LogMessage()
//
//    - Called by everyone
//
/// <summary>
///   Adds a message to our internal ringbuffer, to be handled later by ProcessMessages().
/// </summary>
//
void TraceLog::LogMessage(tstring _msg, TracelogLevel _lvl)
{
	if (_lvl > LoggingLevel)
		return;

	if (WaitForSingleObject(LoggingReady, 0) != WAIT_OBJECT_0)
	{
		return;
	}

	DWORD waitResult = WaitForSingleObject(LoggingMutex, 1000);
	switch (waitResult)
	{
		case WAIT_OBJECT_0:

			// insert message into ringbuf at position indicated
			TRACELOG_ENTRY * tlEnt;

			if (MsgFreelist.empty())
			{
				// TODO:  log a message warning that we had to grow the queue-length
				// TODO:  grow queue by some number of items at a time
				tlEnt = new TRACELOG_ENTRY;
			}
			else
			{
				MsgFreelist.dequeue(&tlEnt);
			}

			SYSTEMTIME st;
			GetLocalTime(&st);

			tlEnt->Message = boost::str(tformat(_T("[%1%/%2%/%3%] [%|4$0+2|:%|5$0+2|:%|6$0+2|.%|7$0+3|] [%|8$0+5|]  %9%")) 
				% st.wMonth % st.wDay % st.wYear % st.wHour % st.wMinute % st.wSecond % st.wMilliseconds % GetCurrentThreadId() % _msg );
			tlEnt->Level = _lvl;
			tlEnt->Tid = GetCurrentThreadId();

			MsgQueue.enqueue(tlEnt);

			// signal Logging Thread that it has something to do
			ProcessMessages();	// TODO:  This should be its own real thread...

			// done!
			if (!ReleaseMutex(LoggingMutex)) 
			{ 
				return;
			} 

			break;

		case WAIT_TIMEOUT:
			// TODO:  handle case where we timeout waiting for logging mutex
			return;
			break;

		case WAIT_FAILED:
			// TODO:  handle case where waiting for logging mutex fails
			return;
			break;

		case WAIT_ABANDONED:
			// TODO:  handle case where the logging-mutex owner abandoned it
			return;
			break;
	}


} // End of LogMessage()



//================================================================================================
//
//  LogErrorMessage()
//
//    - Called from wherever when we encounter an error
//
/// <summary>
///   Logs an error message to the tracelog, including a second line with error number and 
///   description.
/// </summary>
//
void TraceLog::LogErrorMessage(tstring _location, tstring _buf, DWORD _err)
{
	LogMessage(_location + L"  * Error:  " + _buf, TRACELOG_LEVEL_ERROR);

	TCHAR chBuf[1024];
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		_err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );
	swprintf_s(chBuf, sizeof(chBuf)/2, L"%s    - err:[%d], desc:[%s]", _location.c_str(), _err, (LPTSTR)lpMsgBuf);
	LogMessage(chBuf, TRACELOG_LEVEL_ERROR);
	LocalFree(lpMsgBuf);

} // End of LogErrorMessage()



//================================================================================================
//
//  SetLogfile()
//
//    - Should be only instantiated from pb.cpp
//
/// <summary>
///   Default constructor.
/// </summary>
//
void TraceLog::SetLogfile(tstring _fname)
{
	bool success = false;

	LogFile.open(_fname.c_str(), ios::app);
	if (LogFile.good())
	{
		LogFile << _T("\n\n--------------------------------------------------------------------------------\n\nLogging Started...\n");
		success = true;
	}
	else
	{
		success = false;
	}

	if (success)
	{
		LogfileName = _fname;
		HaveLogfile = true;
		if (LoggingMutex != NULL)
			SetEvent(LoggingReady);
	}
	else
	{
		HaveLogfile = false;
	}

} // End of SetLogfile()



//================================================================================================
//
//  SetLoglevel()
//
//    - Called by ???
//
/// <summary>
///   Sets the severity at which we'll log to that specified.
/// </summary>
//
void TraceLog::SetLoglevel(TRACELOG_LEVEL _lvl)
{
	LoggingLevel = _lvl;

} // End of SetLoglevel()



//================================================================================================
//
//  StartLogging()
//
//    - Called by ???
//
/// <summary>
///   Starts logging after we've stopped it for whatever reason, reopening the file if necessary.
/// </summary>
//
void TraceLog::StartLogging()
{
	SetLogfile(LogfileName);
	TRACEI("[TraceLog] [StartLogging]    starting logging");

} // End of StartLogging()



//================================================================================================
//
//  StopLogging()
//
//    - Called by ???
//
/// <summary>
///   Stops logging, and closes the file we were using.
/// </summary>
/// <remarks>
///   This routine is generally called when we need to relinquish control of a file, for example
///   so that it can be deleted out from under us.  A call to TraceLog::StartLogging() will be
///   required to start logging back up again.  No messages will be logged while we're in Stopped
///   mode, though a future version of TraceLog (i.e. a dedicated log-writer thread) should be
///   able to better handle queueing of messages while we're stopped...
/// </remarks>
//
void TraceLog::StopLogging()
{
	TRACEI("[TraceLog] [StopLogging]    stopping logging");
	ResetEvent(LoggingReady); // Sets signalled to FALSE, so noone else will try logging	
	if (LogFile.is_open())
	{
//		TRACEI("[TraceLog] [StopLogging]    closing file");
		LogFile.close();
	}

} // End of StopLogging()



//================================================================================================
//
//  TraceLog()
//
//    - Should be only instantiated from pb.cpp
//
/// <summary>
///   Default constructor.
/// </summary>
//
TraceLog::TraceLog()
{
	LoggingLevel = TRACELOG_LEVEL_DEFAULT;
	HaveLogfile = false;
	LoggingReady = CreateEvent(NULL, TRUE, FALSE, _T("PB LoggingReady Event"));
	LoggingMutex = CreateMutex(NULL, FALSE, _T("PB TraceLogging Mutex"));

	for (int i=0; i<TRACELOG_QUEUE_LENGTH; ++i)
	{
		TRACELOG_ENTRY * tlEnt = new TRACELOG_ENTRY;
		tlEnt->Level = TRACELOG_LEVEL_DEFAULT;
		tlEnt->Tid = 0;
		tlEnt->Message = _T("(no message)");	// fix problem while exiting program
		MsgFreelist.enqueue(tlEnt);
	}

	// start logging thread

} // End of TraceLog() constructor



//================================================================================================
//
//  ~TraceLog()
//
//    - Should be only destroyed from pb.cpp
//
/// <summary>
///   Destructor.
/// </summary>
//
TraceLog::~TraceLog()
{
	ResetEvent(LoggingReady); // Sets signalled to FALSE, so noone else will try logging


	// Looks like the boost::lockfree stuff still isn't quite ready for primetime...
	//TRACELOG_ENTRY * tlEnt = NULL;
	//while (!MsgQueue.empty())
	//{
	//	// TODO: Flush out these messages to-disk!
	//	MsgQueue.dequeue(&tlEnt);
	//	delete tlEnt;
	//	tlEnt = NULL;
	//}
	//while (!MsgFreelist.empty())
	//{
	//	try
	//	{
	//		MsgFreelist.dequeue(&tlEnt);
	//		tlEnt->Tid = 0;
	//		tlEnt->Message = _T("no message");
	//		delete tlEnt;
	//		tlEnt = NULL;
	//	}
	//	catch (...)
	//	{
	//		// TODO:  Why do we occasionally hit an exception here??
	//		int debugbreak=1;
	//	}
	//}

	LogFile.close(); // Close file

} // End of ~TraceLog() destructor
