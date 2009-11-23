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

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-0x7FFF(32767), and 0x8000(32768)-0xFFFF(65535)
// are reserved for use by customers.
//
#define FILE_DEVICE_PEERGUARDIAN  0xBEEF

//
// Macro definition for defining IOCTL and FSCTL function control codes. Note
// that function codes 0-0x7FF(2047) are reserved for Microsoft Corporation,
// and 0x800(2048)-0xFFF(4095) are reserved for customers.
//
#define PEERGUARDIAN_IOCTL_BASE  0xDED

//
// The device driver IOCTLs
//
#define CTL_CODE_PEERGUARDIAN(i,a) CTL_CODE(FILE_DEVICE_PEERGUARDIAN, PEERGUARDIAN_IOCTL_BASE+(i), METHOD_BUFFERED, (a))

#define IOCTL_PEERGUARDIAN_HOOK						CTL_CODE_PEERGUARDIAN(0,FILE_WRITE_DATA)
#define IOCTL_PEERGUARDIAN_SETRANGES				CTL_CODE_PEERGUARDIAN(1,FILE_WRITE_DATA)
#define IOCTL_PEERGUARDIAN_HTTP						CTL_CODE_PEERGUARDIAN(2,FILE_WRITE_DATA)
#define IOCTL_PEERGUARDIAN_GETNOTIFICATION		CTL_CODE_PEERGUARDIAN(3,FILE_READ_DATA)

//
// Name that Win32 front end will use to open the PeerDefender device
//
#define PEERGUARDIAN_DEVICE_NAME_WIN32	_T("\\\\.\\pgfilter")

// {5464FE75-7BB9-43c0-81C8-E52BCC71C1AB}
DEFINE_GUID(PGWFP_CONNECT_CALLOUT_V4, 
0x5464fe75, 0x7bb9, 0x43c0, 0x81, 0xc8, 0xe5, 0x2b, 0xcc, 0x71, 0xc1, 0xab);

// {D51FD4B6-0E6A-4a5d-83B1-728E6D74B384}
DEFINE_GUID(PGWFP_ACCEPT_CALLOUT_V4, 
0xd51fd4b6, 0xe6a, 0x4a5d, 0x83, 0xb1, 0x72, 0x8e, 0x6d, 0x74, 0xb3, 0x84);

// {22731651-58CF-4e9b-989A-4F0EDA0141C7}
DEFINE_GUID(PGWFP_CONNECT_CALLOUT_V6, 
0x22731651, 0x58cf, 0x4e9b, 0x98, 0x9a, 0x4f, 0xe, 0xda, 0x1, 0x41, 0xc7);

// {E69A6A0B-0CF1-4e39-B613-B4E1FE830DA4}
DEFINE_GUID(PGWFP_ACCEPT_CALLOUT_V6, 
0xe69a6a0b, 0xcf1, 0x4e39, 0xb6, 0x13, 0xb4, 0xe1, 0xfe, 0x83, 0xd, 0xa4);

// {EF6723D8-79A9-466d-B436-E6BB2BEA8EA2}
DEFINE_GUID(PG2_MONITOR_SUBLAYER, 
0xef6723d8, 0x79a9, 0x466d, 0xb4, 0x36, 0xe6, 0xbb, 0x2b, 0xea, 0x8e, 0xa2);

// {2F7A255E-C1AB-47f0-BB7A-6B9E8309AB03}
DEFINE_GUID(PGWFP_CONNECT_FILTER_V4, 
0x2f7a255e, 0xc1ab, 0x47f0, 0xbb, 0x7a, 0x6b, 0x9e, 0x83, 0x9, 0xab, 0x3);

// {EDCFC963-527A-483e-A2AF-A0BF74F475D8}
DEFINE_GUID(PGWFP_ACCEPT_FILTER_V4, 
0xedcfc963, 0x527a, 0x483e, 0xa2, 0xaf, 0xa0, 0xbf, 0x74, 0xf4, 0x75, 0xd8);

// {ED4F8E37-E054-403e-A570-132ED67CEF46}
DEFINE_GUID(PGWFP_CONNECT_FILTER_V6, 
0xed4f8e37, 0xe054, 0x403e, 0xa5, 0x70, 0x13, 0x2e, 0xd6, 0x7c, 0xef, 0x46);

// {204418B9-9DD0-428b-9008-3433BD94C8C3}
DEFINE_GUID(PGWFP_ACCEPT_FILTER_V6, 
0x204418b9, 0x9dd0, 0x428b, 0x90, 0x8, 0x34, 0x33, 0xbd, 0x94, 0xc8, 0xc3);

typedef struct TAG_PGIPRANGE {
	LPCWSTR label;	// not stored in driver, make sure pointer is constant
	ULONG start;
	ULONG end;
} PGIPRANGE;

typedef union TAG_PGIP6 {
	ULONGLONG i64[2];
	ULONG i32[4];
} PGIP6;

typedef struct TAG_PGIP6RANGE {
	LPCWSTR label;
	PGIP6 start;
	PGIP6 end;
} PGIP6RANGE;

typedef struct TAG_PGRANGES {
	ULONG protocol; // AF_INET or AF_INET6
	ULONG block; // 1 = block, 0 = allow.
	ULONG labelsid;
	ULONG count;
	union {
		PGIPRANGE ranges[1];
		PGIP6RANGE ranges6[1];
	};
} PGRANGES;

typedef union {
	SOCKADDR addr;
	SOCKADDR_IN addr4;
	SOCKADDR_IN6 addr6;
} PGADDR;

typedef struct TAG_PGNOTIFICATION {
	LPCWSTR label;
	ULONG labelsid;
	ULONG protocol;
	PGADDR source, dest;
	ULONG action; // 0 = block, 1 = explicit allow (ie allow list), 2 = allow (not blocked)
} PGNOTIFICATION;
