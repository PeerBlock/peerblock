/*
	Copyright (C) 2009 Mark Bulas

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


//////////---------------------------------------------------------------------
// Includes

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
///	  logfile.
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
			tstring strLine = boost::str(tformat(_T("[%1%] %2%\n")) % tlEnt->Tid % tlEnt->Message );
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

	if (WaitForSingleObject(LoggingReady, 0) != WAIT_OBJECT_0)
	{
		return;
	}

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

	tlEnt->Level = _lvl;
	tlEnt->Message = _msg;
	tlEnt->Tid = GetCurrentThreadId();
	MsgQueue.enqueue(tlEnt);

	// signal Logging Thread that it has something to do
	ProcessMessages();	// TODO:  This should be its own real thread...

	// done!

} // End of LogMessage()



//================================================================================================
//
//  SetLogfile()
//
//    - Should be only instantiated from pg2.cpp
//
/// <summary>
///   Default constructor.
/// </summary>
//
void TraceLog::SetLogfile(tstring _fname)
{
	bool success = false;

	// open file
 //   hFile = CreateFile (_fname.c_str (), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 
	//	FILE_ATTRIBUTE_NORMAL, 0);
 //   if (hFile == INVALID_HANDLE_VALUE) 
	//	success = false;
	//else
	//	success = true;
	
	LogFile.open(_fname.c_str());
	if (LogFile.good())
	{
		LogFile << _T("Logging Started...\n");
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
//  TraceLog()
//
//    - Should be only instantiated from pg2.cpp
//
/// <summary>
///   Default constructor.
/// </summary>
//
TraceLog::TraceLog()
{
	LoggingLevel = TRACELOG_LEVEL_DEFAULT;
	HaveLogfile = false;
	LoggingReady = CreateEvent (NULL, TRUE, FALSE, _T("PB LoggingReady Event"));

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
//    - Should be only destroyed from pg2.cpp
//
/// <summary>
///   Destructor.
/// </summary>
//
TraceLog::~TraceLog()
{
	ResetEvent(LoggingReady);	// Sets signalled to FALSE, so noone else will try logging



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

	LogFile.close();            // Close file

} // End of ~TraceLog() destructor

