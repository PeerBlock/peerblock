/*
	Copyright (C) 2004-2005 Cory Nelson
	PeerBlock modifications copyright (C) 2009-2010 PeerBlock, LLC
	Based on the original work by Tim Leonard

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

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-0x7FFF(32767), and 0x8000(32768)-0xFFFF(65535)
// are reserved for use by customers.
//
// Note that this is the same value as the old PeerGuardian code used, but it 
// really shouldn't cause any conflicts.
//
#define FILE_DEVICE_PEERBLOCK  0xBEEF

//
// Macro definition for defining IOCTL and FSCTL function control codes. Note
// that function codes 0-0x7FF(2047) are reserved for Microsoft Corporation,
// and 0x800(2048)-0xFFF(4095) are reserved for customers.
//
// Note that this is the same value as the old PeerGuardian code used, but it 
// really shouldn't cause any conflicts.
//
#define PEERBLOCK_IOCTL_BASE  0xDED

//
// The device driver IOCTLs
//
#define CTL_CODE_PEERBLOCK(i,a) CTL_CODE(FILE_DEVICE_PEERBLOCK, PEERBLOCK_IOCTL_BASE+(i), METHOD_BUFFERED, (a))

#define IOCTL_PEERBLOCK_HOOK				CTL_CODE_PEERBLOCK(0, FILE_WRITE_DATA)
#define IOCTL_PEERBLOCK_SETRANGES			CTL_CODE_PEERBLOCK(1, FILE_WRITE_DATA)
#define IOCTL_PEERBLOCK_HTTP				CTL_CODE_PEERBLOCK(2, FILE_WRITE_DATA)
#define IOCTL_PEERBLOCK_GETNOTIFICATION		CTL_CODE_PEERBLOCK(3, FILE_READ_DATA)
#define IOCTL_PEERBLOCK_SETDESTINATIONPORTS CTL_CODE_PEERBLOCK(4, FILE_WRITE_DATA)
#define IOCTL_PEERBLOCK_SETSOURCEPORTS      CTL_CODE_PEERBLOCK(5, FILE_WRITE_DATA)

//
// Name that Win32 front end will use to open the PeerBlock device
//
#define PEERBLOCK_DEVICE_NAME_WIN32	_T("\\\\.\\pbfilter")

//
// The following GUIDs were all regenerated for PeerBlock use
//

// {1DFC499B-50F8-45ae-80BE-628AF06E6DFB}
DEFINE_GUID(PBWFP_CONNECT_CALLOUT_V4, 
0x1dfc499b, 0x50f8, 0x45ae, 0x80, 0xbe, 0x62, 0x8a, 0xf0, 0x6e, 0x6d, 0xfb);

// {19E15977-B760-414e-A1C9-A5AC82DBEB73}
DEFINE_GUID(PBWFP_ACCEPT_CALLOUT_V4, 
0x19e15977, 0xb760, 0x414e, 0xa1, 0xc9, 0xa5, 0xac, 0x82, 0xdb, 0xeb, 0x73);

// {8F2BA47B-AA1A-4f71-A74D-7109383E6A0B}
DEFINE_GUID(PBWFP_CONNECT_CALLOUT_V6, 
0x8f2ba47b, 0xaa1a, 0x4f71, 0xa7, 0x4d, 0x71, 0x9, 0x38, 0x3e, 0x6a, 0xb);

// {EC263B43-013A-4dd9-A039-F648A030FB59}
DEFINE_GUID(PBWFP_ACCEPT_CALLOUT_V6, 
0xec263b43, 0x13a, 0x4dd9, 0xa0, 0x39, 0xf6, 0x48, 0xa0, 0x30, 0xfb, 0x59);

// {3096130E-DA25-469a-B56F-07770389EF38}
DEFINE_GUID(PB_MONITOR_SUBLAYER, 
0x3096130e, 0xda25, 0x469a, 0xb5, 0x6f, 0x7, 0x77, 0x3, 0x89, 0xef, 0x38);

// {908BDE64-C29A-4d7c-920A-F135C5B8A731}
DEFINE_GUID(PBWFP_CONNECT_FILTER_V4, 
0x908bde64, 0xc29a, 0x4d7c, 0x92, 0xa, 0xf1, 0x35, 0xc5, 0xb8, 0xa7, 0x31);

// {3A68D361-508F-4648-B820-437ED7B3E2F0}
DEFINE_GUID(PBWFP_ACCEPT_FILTER_V4, 
0x3a68d361, 0x508f, 0x4648, 0xb8, 0x20, 0x43, 0x7e, 0xd7, 0xb3, 0xe2, 0xf0);

// {393CAF3D-E6ED-4876-80C4-1A33CE66E71F}
DEFINE_GUID(PBWFP_CONNECT_FILTER_V6, 
0x393caf3d, 0xe6ed, 0x4876, 0x80, 0xc4, 0x1a, 0x33, 0xce, 0x66, 0xe7, 0x1f);

// {0CD2DEB3-57FA-4285-8B24-D9E93CF66ECE}
DEFINE_GUID(PBWFP_ACCEPT_FILTER_V6, 
0xcd2deb3, 0x57fa, 0x4285, 0x8b, 0x24, 0xd9, 0xe9, 0x3c, 0xf6, 0x6e, 0xce);

typedef struct TAG_PBIPRANGE {
	LPCWSTR label;	// not stored in driver, make sure pointer is constant
	ULONG start;
	ULONG end;
} PBIPRANGE;

typedef struct TAG_PBRANGES {
	ULONG block; // 1 = block, 0 = allow.
	ULONG labelsid;
	ULONG count;
	PBIPRANGE ranges[1];
} PBRANGES;

typedef union {
	SOCKADDR addr;
	SOCKADDR_IN addr4;
	SOCKADDR_IN6 addr6;
} PBADDR;

typedef struct TAG_PBNOTIFICATION {
	LPCWSTR label;
	ULONG labelsid;
	ULONG protocol;
	PBADDR source, dest;
	ULONG action; // 0 = block, 1 = explicit allow (ie allow list), 2 = allow (not blocked)
} PBNOTIFICATION;
