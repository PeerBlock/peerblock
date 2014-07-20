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

#include <stddef.h>
#include <ntddk.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <ntddndis.h>
#include <pfhook.h>
#include "internal.h"

static void setfilter(PacketFilterExtensionPtr fn)
{
	UNICODE_STRING name;
	PDEVICE_OBJECT device=NULL;
	PFILE_OBJECT file=NULL;
	NTSTATUS status;

	DbgPrint("pbfilter:  > Entering setfilter()\n");
	RtlInitUnicodeString(&name, DD_IPFLTRDRVR_DEVICE_NAME);
	status=IoGetDeviceObjectPointer(&name, STANDARD_RIGHTS_ALL, &file, &device);

	if(NT_SUCCESS(status))
	{
		KEVENT event;
		IO_STATUS_BLOCK iostatus;
		PF_SET_EXTENSION_HOOK_INFO hookinfo;
		PIRP irp;

		DbgPrint("pbfilter:    got devobj\n");
		hookinfo.ExtensionPointer=fn;
		KeInitializeEvent(&event, NotificationEvent, FALSE);

		irp=IoBuildDeviceIoControlRequest(
				IOCTL_PF_SET_EXTENSION_POINTER, device, &hookinfo,
				sizeof(PF_SET_EXTENSION_HOOK_INFO), NULL, 0, FALSE, &event, &iostatus);

		DbgPrint("pbfilter:    calling into driver\n");
		if(irp && IoCallDriver(device, irp)==STATUS_PENDING)
		{
			DbgPrint("pbfilter:    waiting for IRP to complete\n");
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		}
		else
		{
			DbgPrint("pbfilter:    IRP not pending (or no IRP?)\n");
		}

		if(file)
		{
			DbgPrint("pbfilter:    Dereferencing file\n");
			ObDereferenceObject(file);
		}
		else
		{
			DbgPrint("pbfilter:    no file to dereference\n");
		}
	}
	else
	{
		DbgPrint("pbfilter:  * ERROR: unable to get IpFltDrv DevObj, status:[0x%lX]\n", status);
	}

	DbgPrint("pbfilter:  < Leaving setfilter()\n");
}

static PF_FORWARD_ACTION filter_cb(unsigned char *header, unsigned char *packet, unsigned int len, unsigned int recvindex, unsigned int sendindex, IPAddr nextrecvhop, IPAddr nextsendhop)
{
	PBNOTIFICATION pbn= {0};
	const IP_HEADER *iph=(IP_HEADER*)header;
	const PBIPRANGE *range=NULL;
	const ULONG src=NTOHL(iph->ipSource);
	const ULONG dest=NTOHL(iph->ipDestination);
	USHORT srcport=0;
	USHORT destport=0;
	int opening=1;
	int http = 0;

	// TCP = 6 (http://www.iana.org/assignments/protocol-numbers/)
	if(iph->ipProtocol == IPPROTO_TCP) {
		const TCP_HEADER *tcp=(TCP_HEADER*)packet;
		srcport=NTOHS(tcp->sourcePort);
		destport=NTOHS(tcp->destinationPort);

		opening = (tcp->ack==0);

		// XP needs this as the port which comes back is the same
		// as the port going out and therefore it will be blocked
		if (DestinationPortAllowed(destport) || DestinationPortAllowed(srcport) ||
				SourcePortAllowed(destport) || SourcePortAllowed(srcport)
		   ) {
			http = 1;
		}
	}

	if(!http) {
		KIRQL irq;

		KeAcquireSpinLock(&g_internal->rangeslock, &irq);

		//TODO: test allowed ranges.
		if(g_internal->allowedcount) {
			range = inranges(g_internal->allowedranges, g_internal->allowedcount, src);
			if(!range) range = inranges(g_internal->allowedranges, g_internal->allowedcount, dest);

			if(range) {
				pbn.label = range->label;
				pbn.labelsid = g_internal->allowedlabelsid;
				pbn.action = 1;
			}
		}

		if(!range && g_internal->blockedcount) {
			range=inranges(g_internal->blockedranges, g_internal->blockedcount, src);
			if(!range) range=inranges(g_internal->blockedranges, g_internal->blockedcount, dest);

			if(range) {
				pbn.label = range->label;
				pbn.labelsid = g_internal->blockedlabelsid;
				pbn.action = 0;
			}
		}

		KeReleaseSpinLock(&g_internal->rangeslock, irq);
	}

	if(!range) {
		pbn.action = 2;
	}

	if(range || opening) {
		pbn.protocol=iph->ipProtocol;

		pbn.source.addr4.sin_family = AF_INET;
		pbn.source.addr4.sin_addr.s_addr = iph->ipSource;
		pbn.source.addr4.sin_port = HTONS(srcport);

		pbn.dest.addr4.sin_family = AF_INET;
		pbn.dest.addr4.sin_addr.s_addr = iph->ipDestination;
		pbn.dest.addr4.sin_port = HTONS(destport);

		Notification_Send(&g_internal->queue, &pbn);

		return pbn.action ? PF_FORWARD : PF_DROP;
	}

	return PF_FORWARD;
}

static NTSTATUS drv_create(PDEVICE_OBJECT device, PIRP irp)
{
	DbgPrint("pbfilter:  > Entering drv_create()\n");
	irp->IoStatus.Status=STATUS_SUCCESS;
	irp->IoStatus.Information=0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	DbgPrint("pbfilter:  < Leaving drv_create()\n");
	return STATUS_SUCCESS;
}

static NTSTATUS drv_cleanup(PDEVICE_OBJECT device, PIRP irp)
{
	DbgPrint("pbfilter:  > Entering drv_cleanup()\n");

	setfilter(NULL);
	SetRanges(NULL, 0);
	SetRanges(NULL, 1);

	DestroyNotificationQueue(&g_internal->queue);

	irp->IoStatus.Status=STATUS_SUCCESS;
	irp->IoStatus.Information=0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	DbgPrint("pbfilter:  < Leaving drv_cleanup()\n");
	return STATUS_SUCCESS;
}

static NTSTATUS drv_control(PDEVICE_OBJECT device, PIRP irp)
{
	PIO_STACK_LOCATION irpstack;
	ULONG controlcode;
	NTSTATUS status;

	irp->IoStatus.Status=STATUS_SUCCESS;
	irp->IoStatus.Information=0;

	irpstack=IoGetCurrentIrpStackLocation(irp);
	controlcode=irpstack->Parameters.DeviceIoControl.IoControlCode;

	switch(controlcode)
	{
	case IOCTL_PEERBLOCK_HOOK:
		DbgPrint("pbfilter:  > IOCTL_PEERBLOCK_HOOK\n");
		if(irp->AssociatedIrp.SystemBuffer!=NULL && irpstack->Parameters.DeviceIoControl.InputBufferLength==sizeof(int))
		{
			int *hook=(int*)irp->AssociatedIrp.SystemBuffer;
			DbgPrint("pbfilter:    setting filter...\n");
			setfilter((*hook)?filter_cb:NULL);
			DbgPrint("pbfilter:    ...filter set\n");
		}
		else
		{
			DbgPrint("pbfilter:  * ERROR: invalid parameter\n");
			irp->IoStatus.Status=STATUS_INVALID_PARAMETER;
		}
		DbgPrint("pbfilter:  < IOCTL_PEERBLOCK_HOOK\n");
		break;

	case IOCTL_PEERBLOCK_SETRANGES:
	{
		PBRANGES *ranges;
		ULONG inputlen;

		DbgPrint("pbfilter:  > IOCTL_PEERBLOCK_SETRANGES\n");
		ranges = irp->AssociatedIrp.SystemBuffer;
		inputlen = irpstack->Parameters.DeviceIoControl.InputBufferLength;

		if(inputlen >= offsetof(PBRANGES, ranges[0]) && inputlen >= offsetof(PBRANGES, ranges[ranges->count]))
		{
			DbgPrint("pbfilter:    calling SetRanges()\n");
			SetRanges(ranges, ranges->block);
		}
		else
		{
			DbgPrint("pbfilter:  * Error: STATUS_INVALID_PARAMETER\n");
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		}
		DbgPrint("pbfilter:  < IOCTL_PEERBLOCK_SETRANGES\n");
	}
	break;

	case IOCTL_PEERBLOCK_GETNOTIFICATION:
		return Notification_Recieve(&g_internal->queue, irp);

	case IOCTL_PEERBLOCK_SETDESTINATIONPORTS:
	{
		USHORT *ports;
		ULONG count;

		DbgPrint("pbfilter:    > IOCTL_PEERBLOCK_SETDESTINATIONPORTS\n");
		ports = irp->AssociatedIrp.SystemBuffer;
		count = irpstack->Parameters.DeviceIoControl.InputBufferLength;

		SetDestinationPorts(ports, (USHORT) (count / sizeof(USHORT)));
		DbgPrint("pbfilter:    < IOCTL_PEERBLOCK_SETDESTINATIONPORTS\n");

	}
	break;

	case IOCTL_PEERBLOCK_SETSOURCEPORTS:
	{
		USHORT *ports;
		ULONG count;

		DbgPrint("pbfilter:    > IOCTL_PEERBLOCK_SETSOURCEPORTS\n");
		ports = irp->AssociatedIrp.SystemBuffer;
		count = irpstack->Parameters.DeviceIoControl.InputBufferLength;

		SetSourcePorts(ports, (USHORT) (count / sizeof(USHORT)));
		DbgPrint("pbfilter:    < IOCTL_PEERBLOCK_SETSOURCEPORTS\n");

	}
	break;

	default:
		DbgPrint("pbfilter:  * ERROR: invalid parameter for IOCTL!\n");
		irp->IoStatus.Status=STATUS_INVALID_PARAMETER;
	}

	status=irp->IoStatus.Status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

static void drv_unload(PDRIVER_OBJECT driver)
{
	UNICODE_STRING devlink;

	DbgPrint("pbfilter:  > Entering drv_unload()\n");
	RtlInitUnicodeString(&devlink, DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&devlink);

	IoDeleteDevice(driver->DeviceObject);

	DbgPrint("pbfilter:  < Leaving drv_unload()\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registrypath)
{
	UNICODE_STRING devicename;
	PDEVICE_OBJECT device=NULL;
	NTSTATUS status;

	DbgPrint("pbfilter:  > Entering DriverEntry()\n");

	DbgPrint("pbfilter:    setting up devicename\n");
	RtlInitUnicodeString(&devicename, NT_DEVICE_NAME);

	DbgPrint("pbfilter:    creating device\n");
	status=IoCreateDevice(driver, sizeof(PBINTERNAL), &devicename, FILE_DEVICE_PEERBLOCK, 0, FALSE, &device);

	if(NT_SUCCESS(status))
	{
		UNICODE_STRING devicelink;

		DbgPrint("pbfilter:    created device, initting internal data\n");

		DbgPrint("pbfilter:    ... creating symbolic link\n");
		RtlInitUnicodeString(&devicelink, DOS_DEVICE_NAME);
		status=IoCreateSymbolicLink(&devicelink, &devicename);

		DbgPrint("pbfilter:    ... setting up irp-handling functions\n");
		driver->MajorFunction[IRP_MJ_CREATE]=
			driver->MajorFunction[IRP_MJ_CLOSE]=drv_create;
		driver->MajorFunction[IRP_MJ_CLEANUP]=drv_cleanup;
		driver->MajorFunction[IRP_MJ_DEVICE_CONTROL]=drv_control;
		driver->DriverUnload=drv_unload;
		device->Flags|=DO_BUFFERED_IO;

		DbgPrint("pbfilter:    ... setting up device extension\n");
		g_internal=device->DeviceExtension;

		DbgPrint("pbfilter:    ... initializing lock and queue\n");
		KeInitializeSpinLock(&g_internal->rangeslock);
		KeInitializeSpinLock(&g_internal->destinationportslock);
		KeInitializeSpinLock(&g_internal->sourceportslock);
		InitNotificationQueue(&g_internal->queue);

		DbgPrint("pbfilter:    ... resetting counters\n");
		g_internal->blockedcount = 0;
		g_internal->allowedcount = 0;
		g_internal->destinationportcount = 0;
		g_internal->sourceportcount = 0;

		DbgPrint("pbfilter:    internal data initted\n");
	}

	if(!NT_SUCCESS(status))
	{
		DbgPrint("pbfilter:  * ERROR: couldn't create device, status:[0x%lX]    ... unloading\n", status);
		drv_unload(driver);
	}

	DbgPrint("pbfilter:  < Leaving DriverEntry(), status:[0x%lX]\n", status);

	return status;
}
