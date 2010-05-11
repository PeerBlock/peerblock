/*
	Copyright (C) 2009 PeerBlock, LLC

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
#include "exceptionfilter.h"


#if defined _M_X64 || defined _M_IX86
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI 
  MyDummySetUnhandledExceptionFilter(
  LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
  return NULL;
}
#else
#error "This code works only for x86 and x64!"
#endif



//================================================================================================
//
//  PreventSetUnhandledExceptionFilter()
//
//    - Called by Windows as part of setting up exception-handling.
//
/// <summary>
///   This is a hack to resolve some issues setting up unhandled exception filters on VS 2005+.
/// </summary>
/// <remarks>
///   See (http://blog.kalmbach-software.de/2008/04/02/unhandled-exceptions-in-vc8-and-above-for-x86-and-x64/) 
///   for more details as to why this hack is required for certain cases.
/// </remarks>
//
BOOL PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if (hKernel32 == NULL) return FALSE;
	void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if(pOrgEntry == NULL) return FALSE;
 
	DWORD dwOldProtect = 0;
	SIZE_T jmpSize = 5;
#ifdef _M_X64
	jmpSize = 13;
#endif
	BOOL bProt = VirtualProtect(pOrgEntry, jmpSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	BYTE newJump[20];
	void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
#ifdef _M_IX86
	DWORD dwOrgEntryAddr = (DWORD) pOrgEntry;
	dwOrgEntryAddr += jmpSize; // add 5 for 5 op-codes for jmp rel32
	DWORD dwNewEntryAddr = (DWORD) pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
	// JMP rel32: Jump near, relative, displacement relative to next instruction.
	newJump[0] = 0xE9;  // JMP rel32
	memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
#elif _M_X64
	// We must use R10 or R11, because these are "scratch" registers 
	// which need not to be preserved accross function calls
	// For more info see: Register Usage for x64 64-Bit
	// http://msdn.microsoft.com/en-us/library/ms794547.aspx
	// Thanks to Matthew Smith!!!
	newJump[0] = 0x49;  // MOV R11, ...
	newJump[1] = 0xBB;  // ...
	memcpy(&newJump[2], &pNewFunc, sizeof (pNewFunc));
	//pCur += sizeof (ULONG_PTR);
	newJump[10] = 0x41;  // JMP R11, ...
	newJump[11] = 0xFF;  // ...
	newJump[12] = 0xE3;  // ...
#endif
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(), pOrgEntry, newJump, jmpSize, &bytesWritten);

	if (bProt != FALSE)
	{
		DWORD dwBuf;
		VirtualProtect(pOrgEntry, jmpSize, dwOldProtect, &dwBuf);
	}
	return bRet;

} // End of PreventSetUnhandledExceptionFilter()



//================================================================================================
//
//  PeerblockExceptionFilter()
//
//    - Called by Windows if the app throws an unhandled exception.
//
/// <summary>
///   Writes out a mini-dump file "peerblock.dmp" into the program's current working directory.
/// </summary>
/// <remarks>
///   See (http://www.codeproject.com/KB/debug/postmortemdebug_standalone1.aspx) for more details 
///	  on this implementation.
/// </remarks>
//
LONG WINAPI PeerblockExceptionFilter(struct _EXCEPTION_POINTERS *pExceptionInfo) 
{
	HMODULE hDll = NULL;

	hDll = LoadLibrary(_T("DBGHELP.DLL"));
	if (!hDll)
	{
		//TRACEE("[PeerblockExceptionFilter]    ERROR:  Can't load dbghelp.dll!!");
		return -1;
	}

	MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
	if (!pDump)
	{
		//TRACEE("[PeerblockExceptionFilter]    ERROR:  Can't find MiniDumpWriteDump() routine!!");
		return -1;
	}

	HANDLE hFile = CreateFile( _T("peerblock.dmp"), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL, NULL );
	if (hFile == INVALID_HANDLE_VALUE)
	{
		//TRACEE("[PeerblockExceptionFilter]    ERROR:  Can't create dumpfile!!");
		return -1;
	}

	_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

	ExInfo.ThreadId = GetCurrentThreadId();
	ExInfo.ExceptionPointers = pExceptionInfo;
	ExInfo.ClientPointers = NULL;

	// write the dump
	BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
	if (bOK)
	{
		//TRACES("[PeerblockExceptionFilter]    Saved dumpfile");
	}
	else
	{
		//TRACEE("[PeerblockExceptionFilter]    ERROR:  Can't save dump to file!!");
		return -1;
	}
	CloseHandle(hFile);

	return EXCEPTION_EXECUTE_HANDLER;

}; // End of UnhandledExceptionFilter()



