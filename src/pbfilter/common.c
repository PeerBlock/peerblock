/*
	Original code copyright (C) 2004-2005 Cory Nelson
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

#pragma warning(push)
#pragma warning(disable:4103)
#include <wdm.h>
#pragma warning(pop)

#include <ntddk.h>
#include <stdlib.h>
#include "internal.h"

PBINTERNAL *g_internal;

const PBIPRANGE* inranges(const PBIPRANGE *ranges, int count, ULONG ip) {
	const PBIPRANGE *iter = ranges;
	const PBIPRANGE *last = ranges + count;

	while(0 < count) {
		int count2 = count >> 1;
		const PBIPRANGE *mid = iter + count2;

		if(mid->start < ip) {
			iter = mid + 1;
			count -= count2 + 1;
		}
		else {
			count = count2;
		}
	}

	if(iter != last) {
		if(iter->start != ip) --iter;
	}
	else {
		--iter;
	}

	return (iter >= ranges && iter->start <= ip && ip <= iter->end) ? iter : NULL;
}

void SetRanges(const PBRANGES *ranges, int block) 
{
	PBIPRANGE *nranges, *oldranges;
	ULONG ncount, labelsid;
	KIRQL irq;

	DbgPrint("pbfilter:  > SetRanges\n");
	if(ranges && ranges->count > 0) 
	{
		DbgPrint("pbfilter:    found some ranges\n");
		ncount = ranges->count;
		labelsid = ranges->labelsid;

		DbgPrint("pbfilter:    allocating memory from nonpaged pool");
		nranges = ExAllocatePoolWithTag(NonPagedPool, ranges->count * sizeof(PBIPRANGE), '02GP');
		if (nranges == NULL)
		{
			DbgPrint("pbfilter:    ERROR: SetRanges() can't allocate nranges memory from NonPagedPool!!");
			DbgPrint("  count:[%d], size:[%d]\n", ranges->count, sizeof(PBIPRANGE));
			return;
		}

		DbgPrint("pbfilter:    copying ranges into driver\n");
		RtlCopyMemory(nranges, ranges->ranges, ranges->count * sizeof(PBIPRANGE));
		DbgPrint("pbfilter:    done setting up new ranges\n");
	}
	else 
	{
		DbgPrint("pbfilter:    no ranges specified\n");
		ncount = 0;
		labelsid = 0xFFFFFFFF;
		nranges = NULL;
	}

	DbgPrint("pbfilter:    acquiring rangeslock...\n");
	KeAcquireSpinLock(&g_internal->rangeslock, &irq);
	DbgPrint("pbfilter:    ...rangeslock acquired\n");

	if(block) 
	{
		DbgPrint("pbfilter:    block list\n");
		oldranges = g_internal->blockedcount ? g_internal->blockedranges : NULL;

		g_internal->blockedcount = ncount;
		g_internal->blockedranges = nranges;
		g_internal->blockedlabelsid = labelsid;
	}
	else 
	{
		DbgPrint("pbfilter:    allow list\n");
		oldranges = g_internal->allowedcount ? g_internal->allowedranges : NULL;

		g_internal->allowedcount = ncount;
		g_internal->allowedranges = nranges;
		g_internal->allowedlabelsid = labelsid;
	}

	DbgPrint("pbfilter:    releasing rangeslock...\n");
	KeReleaseSpinLock(&g_internal->rangeslock, irq);
	DbgPrint("pbfilter:    ...rangeslock released\n");

	if(oldranges) {
		DbgPrint("pbfilter:    freeing oldranges\n");
		ExFreePoolWithTag(oldranges, '02GP');
	}
	DbgPrint("pbfilter:  < SetRanges\n");
}

void SetDestinationPorts(const USHORT *ports, USHORT count) 
{
	USHORT *oldports = NULL;
	USHORT *nports = NULL;
	KIRQL irq;

	if (ports && count > 0) {
		oldports = g_internal->destinationports;

		nports = (USHORT*) ExAllocatePoolWithTag(NonPagedPool, sizeof(USHORT) * count, 'PDBP');
		if (nports == NULL)
		{
			DbgPrint("pbfilter:    ERROR: SetDestinationPorts() can't allocate nports memory from NonPagedPool!!\n");
			return;
		}
		RtlCopyMemory(nports, ports, sizeof(USHORT) * count);
	}
	else {
		nports = NULL;
		g_internal->destinationportcount = 0;
	}

	KeAcquireSpinLock(&g_internal->destinationportslock, &irq);

	g_internal->destinationports = nports;
	g_internal->destinationportcount = count;

	KeReleaseSpinLock(&g_internal->destinationportslock, irq);

	if (oldports) {
		ExFreePoolWithTag(oldports, 'PDBP');
	}
}

void SetSourcePorts(const USHORT *ports, USHORT count) 
{
	USHORT *oldports = NULL;
	USHORT *nports = NULL;
	KIRQL irq;

	if (ports && count > 0) {
		oldports = g_internal->sourceports;

		nports = (USHORT*) ExAllocatePoolWithTag(NonPagedPool, sizeof(USHORT) * count, 'PSBP');
		if (nports == NULL)
		{
			DbgPrint("pbfilter:    ERROR: SetSourcePorts() can't allocate nports memory from NonPagedPool!!\n");
			return;
		}
		RtlCopyMemory(nports, ports, sizeof(USHORT) * count);
	}
	else {
		nports = NULL;
		g_internal->sourceportcount = 0;
	}

	KeAcquireSpinLock(&g_internal->sourceportslock, &irq);

	g_internal->sourceports = nports;
	g_internal->sourceportcount = count;

	KeReleaseSpinLock(&g_internal->sourceportslock, irq);

	if (oldports) {
		ExFreePoolWithTag(oldports, 'PSBP');
	}
}

int __cdecl CompareUShort(const void * a, const void * b) 
{
	return ( *(USHORT*)a - *(USHORT*)b );
}

int SourcePortAllowed(USHORT port) {
	return (int) bsearch(&port, g_internal->sourceports, g_internal->sourceportcount, sizeof(USHORT), CompareUShort);
}

int DestinationPortAllowed(USHORT port) {
	return (int) bsearch(&port, g_internal->destinationports, g_internal->destinationportcount, sizeof(USHORT), CompareUShort);
}
