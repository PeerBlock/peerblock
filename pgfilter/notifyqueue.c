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

typedef struct __pg_notifynode {
	LIST_ENTRY entry;
	PGNOTIFICATION notification;
} PGNOTIFYNODE;

typedef struct __pg_irpnode {
	LIST_ENTRY entry;
	PIRP irp;
} PGIRPNODE;

typedef union TAG_PGLISTNODE {
	PGNOTIFYNODE nn;
	PGIRPNODE in;
} PGLISTNODE;

void InitNotificationQueue(NOTIFICATION_QUEUE *queue) {
	InitializeListHead(&queue->irp_list);
	InitializeListHead(&queue->notification_list);
	ExInitializeNPagedLookasideList(&queue->lookaside, NULL, NULL, 0, sizeof(PGLISTNODE), '02GP', 0);
	KeInitializeSpinLock(&queue->lock);
	queue->queued = 0;
}

void DestroyNotificationQueue(NOTIFICATION_QUEUE *queue) {
	while(!IsListEmpty(&queue->notification_list)) {
		PGNOTIFYNODE *notifynode = (PGNOTIFYNODE*)RemoveHeadList(&queue->notification_list);
		ExFreeToNPagedLookasideList(&queue->lookaside, notifynode);
	}

	while(!IsListEmpty(&queue->irp_list)) {
		PGIRPNODE *irpnode = (PGIRPNODE*)RemoveHeadList(&queue->irp_list);
		
		irpnode->irp->IoStatus.Status = STATUS_CANCELLED;
		irpnode->irp->IoStatus.Information = 0;

		IoCompleteRequest(irpnode->irp, IO_NO_INCREMENT);

		ExFreeToNPagedLookasideList(&queue->lookaside, irpnode);
	}

	ExDeleteNPagedLookasideList(&queue->lookaside);
}

void Notification_Send(NOTIFICATION_QUEUE *queue, const PGNOTIFICATION *notification) {
	KIRQL irq;

	KeAcquireSpinLock(&queue->lock, &irq);

	if(IsListEmpty(&queue->irp_list)) {
		if(queue->queued < 64) {
			PGNOTIFYNODE *notifynode;

			notifynode = ExAllocateFromNPagedLookasideList(&queue->lookaside);
			
			InitializeListHead(&notifynode->entry);
			RtlCopyMemory(&notifynode->notification, notification, sizeof(PGNOTIFICATION));

			InsertTailList(&queue->notification_list, &notifynode->entry);
			++queue->queued;
		}
		
		KeReleaseSpinLock(&queue->lock, irq);
	}
	else {
		PGIRPNODE *irpnode = (PGIRPNODE*)RemoveHeadList(&queue->irp_list);
		PGNOTIFICATION *irpnotification = irpnode->irp->AssociatedIrp.SystemBuffer;
		PIRP irp;

		RtlCopyMemory(irpnotification, notification, sizeof(PGNOTIFICATION));

		irp = irpnode->irp;
		ExFreeToNPagedLookasideList(&queue->lookaside, irpnode);

		KeReleaseSpinLock(&queue->lock, irq);

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = sizeof(PGNOTIFICATION);

		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}
}

static void Notification_OnCancel(PDEVICE_OBJECT device, PIRP irp) {
	KIRQL irq;
	NOTIFICATION_QUEUE *queue = &((PGINTERNAL*)device->DeviceExtension)->queue;
	LIST_ENTRY *iter;
	int found = 0;

	DbgPrint("Canceling IRP...\n");

	KeAcquireSpinLock(&queue->lock, &irq);
	for(iter = queue->irp_list.Flink; iter != &queue->irp_list; iter = iter->Flink) {
		PGIRPNODE *irpnode = (PGIRPNODE*)iter;
		if(irpnode->irp == irp) {
			RemoveEntryList(iter);
			ExFreeToNPagedLookasideList(&queue->lookaside, irpnode);
			found = 1;
			break;
		}
	}
	KeReleaseSpinLock(&queue->lock, irq);

	// if it wasn't found, it has already been dequeued and handled.
	if(found) {
		DbgPrint("IRP found, completing.\n");

		irp->IoStatus.Status = STATUS_CANCELLED;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}
	
	IoReleaseCancelSpinLock(irp->CancelIrql);
}

NTSTATUS Notification_Recieve(NOTIFICATION_QUEUE *queue, PIRP irp) {
	PIO_STACK_LOCATION irpstack = IoGetCurrentIrpStackLocation(irp);
	KIRQL irq;

	if(irpstack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(PGNOTIFICATION)) {
		irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return STATUS_BUFFER_TOO_SMALL;
	}
	
	KeAcquireSpinLock(&queue->lock, &irq);

	if(IsListEmpty(&queue->notification_list)) {
		PGIRPNODE *irpnode;
		KIRQL crirq;

		irpnode = ExAllocateFromNPagedLookasideList(&queue->lookaside);

		InitializeListHead(&irpnode->entry);

		irpnode->irp = irp;
		//irp->Tail.Overlay.DriverContext[0] = irpnode;

		InsertTailList(&queue->irp_list, &irpnode->entry);

		IoMarkIrpPending(irp);

		IoAcquireCancelSpinLock(&crirq);

#pragma warning(push)
#pragma warning(disable:4311 4312)
		//IoSetCancelRoutine generates warnings in 32-bit due to silly macroisms.
		IoSetCancelRoutine(irp, Notification_OnCancel);
#pragma warning(pop)

		IoReleaseCancelSpinLock(crirq);
		
		KeReleaseSpinLock(&queue->lock, irq);

		return STATUS_PENDING;
	}
	else {
		PGNOTIFYNODE *notifynode = (PGNOTIFYNODE*)RemoveHeadList(&queue->notification_list);
		PGNOTIFICATION *notification = irp->AssociatedIrp.SystemBuffer;

		RtlCopyMemory(notification, &notifynode->notification, sizeof(PGNOTIFICATION));
		ExFreeToNPagedLookasideList(&queue->lookaside, notifynode);
		--queue->queued;
		
		KeReleaseSpinLock(&queue->lock, irq);

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = sizeof(PGNOTIFICATION);
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return STATUS_SUCCESS;
	}
}
