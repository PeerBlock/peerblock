/*
	Copyright (C) 2004-2005 Cory Nelson
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

#ifndef __NOTIFICATION_QUEUE_H__
#define __NOTIFICATION_QUEUE_H__

#include <ntddk.h>

typedef struct _irp_queue {
	KSPIN_LOCK lock;
	LIST_ENTRY irp_list;
	LIST_ENTRY notification_list;
	NPAGED_LOOKASIDE_LIST lookaside;
	unsigned int queued;
} NOTIFICATION_QUEUE;

void InitNotificationQueue(NOTIFICATION_QUEUE *queue);
void DestroyNotificationQueue(NOTIFICATION_QUEUE *queue);

void Notification_Send(NOTIFICATION_QUEUE *queue, const PBNOTIFICATION *notification);
NTSTATUS Notification_Recieve(NOTIFICATION_QUEUE *queue, PIRP irp);

#endif
