#pragma warning(push)
#pragma warning(disable:4103)
#include <wdm.h>
#pragma warning(pop)

#include <ntddk.h>
#include "internal.h"

PGINTERNAL *g_internal;

const PGIPRANGE* inranges(const PGIPRANGE *ranges, int count, ULONG ip) {
	const PGIPRANGE *iter = ranges;
	const PGIPRANGE *last = ranges + count;

	while(0 < count) {
		//night_stalker_z: bit shifting
		//int count2 = count / 2;
		int count2 = count >> 1;
		const PGIPRANGE *mid = iter + count2;
		
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

void SetRanges(const PGRANGES *ranges, int block) {
	PGIPRANGE *nranges, *oldranges;
	ULONG ncount, labelsid;
	KIRQL irq;

	if(ranges && ranges->count > 0) {
		ncount = ranges->count;
		labelsid = ranges->labelsid;
		nranges = ExAllocatePoolWithTag(NonPagedPool, ranges->count * sizeof(PGIPRANGE), '02GP');
		RtlCopyMemory(nranges, ranges->ranges, ranges->count * sizeof(PGIPRANGE));
	}
	else {
		ncount = 0;
		labelsid = 0xFFFFFFFF;
		nranges = NULL;
	}

	KeAcquireSpinLock(&g_internal->rangeslock, &irq);

	if(block) {
		oldranges = g_internal->blockedcount ? g_internal->blockedranges : NULL;

		g_internal->blockedcount = ncount;
		g_internal->blockedranges = nranges;
		g_internal->blockedlabelsid = labelsid;
	}
	else {
		oldranges = g_internal->allowedcount ? g_internal->allowedranges : NULL;

		g_internal->allowedcount = ncount;
		g_internal->allowedranges = nranges;
		g_internal->allowedlabelsid = labelsid;
	}

	KeReleaseSpinLock(&g_internal->rangeslock, irq);

	if(oldranges) {
		ExFreePoolWithTag(oldranges, '02GP');
	}
}
