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
		int count2 = count / 2;
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

static int ip6cmp(const PGIP6 *left, const PGIP6 *right) {
#ifdef _M_X64
	if(left->i64[0] < right->i64[0]) return -1;
	if(left->i64[0] > right->i64[0]) return 1;
	
	if(left->i64[1] < right->i64[1]) return -1;
	if(left->i64[1] > right->i64[1]) return 1;
#else
	if(left->i32[0] < right->i32[0]) return -1;
	if(left->i32[0] > right->i32[0]) return 1;
	
	if(left->i32[1] < right->i32[1]) return -1;
	if(left->i32[1] > right->i32[1]) return 1;
	
	if(left->i32[2] < right->i32[2]) return -1;
	if(left->i32[2] > right->i32[2]) return 1;
	
	if(left->i32[3] < right->i32[3]) return -1;
	if(left->i32[3] > right->i32[3]) return 1;
#endif

	return 0;
}

const PGIP6RANGE* in6ranges(const PGIP6RANGE *ranges, int count, const PGIP6 *ip) {
	const PGIP6RANGE *iter = ranges;
	const PGIP6RANGE *last = ranges + count;

	while(0 < count) {
		int count2 = count / 2;
		const PGIP6RANGE *mid = iter + count2;
		
		if(ip6cmp(&mid->start, ip) < 0) {
			iter = mid + 1;
			count -= count2 + 1;
		}
		else {
			count = count2;
		}
	}

	if(iter != last) {
		if(ip6cmp(&iter->start, ip)) --iter;
	}
	else {
		--iter;
	}

	return (iter >= ranges && ip6cmp(&iter->start, ip) <= 0 && ip6cmp(ip, &iter->end) <= 0) ? iter : NULL;
}

void SetRanges(const PGRANGES *ranges, int protocol, int block) {
	PGIPRANGE *nranges = NULL, *oldranges = NULL;
	PGIP6RANGE *n6ranges = NULL, *old6ranges = NULL;
	ULONG ncount = 0, labelsid = 0xFFFFFFFF;
	KIRQL irq;

	if(ranges && ranges->count > 0) {
		ncount = ranges->count;
		labelsid = ranges->labelsid;

		if(protocol == AF_INET) {
			nranges = ExAllocatePoolWithTag(NonPagedPool, ranges->count * sizeof(PGIPRANGE), '02GP');
			RtlCopyMemory(nranges, ranges->ranges, ranges->count * sizeof(PGIPRANGE));
		}
		else {
			n6ranges = ExAllocatePoolWithTag(NonPagedPool, ranges->count * sizeof(PGIP6RANGE), '02GP');
			RtlCopyMemory(n6ranges, ranges->ranges6, ranges->count * sizeof(PGIP6RANGE));
		}
	}

	KeAcquireSpinLock(&g_internal->rangeslock, &irq);

	if(block) {
		if(protocol == AF_INET) {
			oldranges = g_internal->blockedcount ? g_internal->blockedranges : NULL;

			g_internal->blockedcount = ncount;
			g_internal->blockedranges = nranges;
			g_internal->blockedlabelsid = labelsid;
		}
		else {
			old6ranges = g_internal->blocked6count ? g_internal->blocked6ranges : NULL;

			g_internal->blocked6count = ncount;
			g_internal->blocked6ranges = n6ranges;
			g_internal->blocked6labelsid = labelsid;
		}
	}
	else {
		if(protocol == AF_INET) {
			oldranges = g_internal->allowedcount ? g_internal->allowedranges : NULL;

			g_internal->allowedcount = ncount;
			g_internal->allowedranges = nranges;
			g_internal->allowedlabelsid = labelsid;
		}
		else {
			old6ranges = g_internal->allowed6count ? g_internal->allowed6ranges : NULL;

			g_internal->allowed6count = ncount;
			g_internal->allowed6ranges = n6ranges;
			g_internal->allowed6labelsid = labelsid;
		}
	}

	KeReleaseSpinLock(&g_internal->rangeslock, irq);

	if(oldranges) {
		ExFreePoolWithTag(oldranges, '02GP');
	}
	else if(old6ranges) {
		ExFreePoolWithTag(old6ranges, '02GP');
	}
}
