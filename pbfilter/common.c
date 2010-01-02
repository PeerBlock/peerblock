#pragma warning(push)
#pragma warning(disable:4103)
#include <wdm.h>
#pragma warning(pop)

#include <ntddk.h>
#include "internal.h"

#include <stdlib.h>

PBINTERNAL *g_internal;

const PBIPRANGE* inranges(const PBIPRANGE *ranges, int count, ULONG ip) {
	const PBIPRANGE *iter = ranges;
	const PBIPRANGE *last = ranges + count;

	while(0 < count) {
		//night_stalker_z: bit shifting
		//int count2 = count / 2;
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

	DbgPrint("pbfilter:  > SetRanges");
	if(ranges && ranges->count > 0) 
	{
		DbgPrint("pbfilter:    found some ranges");
		ncount = ranges->count;
		labelsid = ranges->labelsid;
		DbgPrint("pbfilter:    allocating memory from nonpaged pool");
		nranges = ExAllocatePoolWithTag(NonPagedPool, ranges->count * sizeof(PBIPRANGE), '02GP');
		DbgPrint("pbfilter:    copying ranges into driver");
		RtlCopyMemory(nranges, ranges->ranges, ranges->count * sizeof(PBIPRANGE));
		DbgPrint("pbfilter:    done setting up new ranges");
	}
	else 
	{
		DbgPrint("pbfilter:    no ranges specified");
		ncount = 0;
		labelsid = 0xFFFFFFFF;
		nranges = NULL;
	}

	DbgPrint("pbfilter:    acquiring rangeslock...");
	KeAcquireSpinLock(&g_internal->rangeslock, &irq);
	DbgPrint("pbfilter:    ...rangeslock acquired");

	if(block) 
	{
		DbgPrint("pbfilter:    block list");
		oldranges = g_internal->blockedcount ? g_internal->blockedranges : NULL;

		g_internal->blockedcount = ncount;
		g_internal->blockedranges = nranges;
		g_internal->blockedlabelsid = labelsid;
	}
	else 
	{
		DbgPrint("pbfilter:    allow list");
		oldranges = g_internal->allowedcount ? g_internal->allowedranges : NULL;

		g_internal->allowedcount = ncount;
		g_internal->allowedranges = nranges;
		g_internal->allowedlabelsid = labelsid;
	}

	DbgPrint("pbfilter:    releasing rangeslock...");
	KeReleaseSpinLock(&g_internal->rangeslock, irq);
	DbgPrint("pbfilter:    ...rangeslock released");

	if(oldranges) {
		DbgPrint("pbfilter:    freeing oldranges");
		ExFreePoolWithTag(oldranges, '02GP');
	}
	DbgPrint("pbfilter:  < SetRanges");
}

void SetPorts(const ULONG *ports, ULONG count) 
{
	ULONG *oldports = NULL;
	ULONG *nports;
	KIRQL irq;

	if (ports && count > 0) {
		oldports = g_internal->ports;

		nports = (ULONG*) ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) * count, 'tPBP');
		RtlCopyMemory(nports, ports, sizeof(ULONG) * count);
	}
	else {
		nports = NULL;
		g_internal->portcount = 0;
	}

	KeAcquireSpinLock(&g_internal->portslock, &irq);

	g_internal->ports = nports;
	g_internal->portcount = count;

	KeReleaseSpinLock(&g_internal->portslock, irq);

	if (oldports) {
		ExFreePoolWithTag(oldports, 'tPBP');
	}
}

int __cdecl CompareULong(const void * a, const void * b) {
	return ( *(ULONG*)a - *(ULONG*)b );
}

int PortAllowed(ULONG port) {
	return (int) bsearch(&port, g_internal->ports, g_internal->portcount, sizeof(ULONG), CompareULong);
}
