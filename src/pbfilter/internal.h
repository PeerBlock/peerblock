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

#define INITGUID

#include <wsk.h>
#include <ws2ipdef.h>
#include <guiddef.h>

#include "filter.h"
#include "notifyqueue.h"

#define NT_DEVICE_NAME L"\\Device\\pbfilter"
#define DOS_DEVICE_NAME L"\\DosDevices\\pbfilter"

/*#define HTONL(l) (\
	(((l) & 0xff000000) >> 24) | \
	(((l) & 0x00ff0000) >> 8) | \
	(((l) & 0x0000ff00) << 8) | \
	(((l) & 0x000000ff) << 24) \
)
#define NTOHL(l) HTONL(l)

#define HTONS(s) (\
	(((s) & 0xff00) >> 8) | \
	(((s) & 0x00ff) << 8) \
)
#define NTOHS(s) HTONS(s)*/

// these are intrisics for the BSWAP instruction, much faster than the above macros.
#define HTONL(l) _byteswap_ulong(l)
#define NTOHL(l) HTONL(l)
#define HTONS(s) _byteswap_ushort(s)
#define NTOHS(s) HTONS(s)

#pragma pack(push, 1)

typedef struct __ip_header {
	UCHAR		iphVerLen;		// Version and length 
	UCHAR		ipTOS;			// Type of service 
	USHORT	ipLength;		// Total datagram length 
	USHORT	ipID;				// Identification 
	USHORT	ipFlags;			// Flags
	UCHAR		ipTTL;			// Time to live 
	UCHAR		ipProtocol;		// Protocol 
	USHORT	ipChecksum;		// Header checksum 
	ULONG		ipSource;		// Source address 
	ULONG		ipDestination;	// Destination address 
} IP_HEADER;

typedef struct __tcp_header {
	USHORT	sourcePort;
	USHORT	destinationPort;
	ULONG		sequence;
	ULONG		ack;
} TCP_HEADER, UDP_HEADER;

typedef union TAG_FAKEV6ADDR {
	IN6_ADDR addr6;
	struct {
		unsigned int prefix; // 0x00000120
		unsigned int server;
		unsigned short flags;
		unsigned short clientport;
		unsigned int clientip;
	} teredo;
	struct {
		unsigned short prefix; // 0x0220
		unsigned int clientip;
		unsigned short subnet;
		unsigned __int64 address;
	} sixtofour;
} FAKEV6ADDR;

#pragma pack(pop)

typedef struct __pb_internal {
	NOTIFICATION_QUEUE queue;

	KSPIN_LOCK rangeslock;

	PBIPRANGE *blockedranges;
	ULONG blockedcount, blockedlabelsid;

	PBIPRANGE *allowedranges;
	ULONG allowedcount, allowedlabelsid;

	KSPIN_LOCK destinationportslock;
	KSPIN_LOCK sourceportslock;

	USHORT *destinationports;
	USHORT destinationportcount;
	USHORT *sourceports;
	USHORT sourceportcount;

	int block;
	
	UINT32 connect4;
	UINT32 accept4;
	UINT32 connect6;
	UINT32 accept6;
} PBINTERNAL;

extern PBINTERNAL *g_internal;

const PBIPRANGE* inranges(const PBIPRANGE *ranges, int count, ULONG ip);
void SetRanges(const PBRANGES *ranges, int block);
void SetDestinationPorts(const USHORT *ports, USHORT count);
void SetSourcePorts(const USHORT *ports, USHORT count);
int DestinationPortAllowed(USHORT port);
int SourcePortAllowed(USHORT port);
