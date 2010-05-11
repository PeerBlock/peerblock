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

#define NDIS60 1

#include <stddef.h>

#pragma warning(push)
#pragma warning(disable:4103)
#include <wdm.h>
#pragma warning(pop)

#include <ntddk.h>
#include <ntstrsafe.h>

#include <fwpmk.h>
#include <fwpsk.h>

#include "internal.h"

static ULONG CheckRanges(PBNOTIFICATION *pbn, ULONG ip) {
	const PBIPRANGE *range;
	KIRQL irq;
	ULONG action = 2;
		
	KeAcquireSpinLock(&g_internal->rangeslock, &irq);
	
	if(g_internal->allowedcount) {
		range = inranges(g_internal->allowedranges, g_internal->allowedcount, ip);
		if(range) {
			pbn->label = range->label;
			pbn->labelsid = g_internal->allowedlabelsid;
			action = 1;
		}
	}

	if(!range) {
		if(g_internal->blockedcount) {
			range = inranges(g_internal->blockedranges, g_internal->blockedcount, ip);
			if(range) {
				pbn->label = range->label;
				pbn->labelsid = g_internal->blockedlabelsid;
				action = 0;
			}
		}
		else {
			range = NULL;
		}
	}

	KeReleaseSpinLock(&g_internal->rangeslock, irq);

	return action;
}

static void FillAddrs(PBNOTIFICATION *pbn, ULONG srcAddr, const IN6_ADDR *srcAddr6, USHORT srcPort, ULONG destAddr, const IN6_ADDR *destAddr6, USHORT destPort) {
	if(!srcAddr6) {
		pbn->source.addr4.sin_family = AF_INET;
		pbn->source.addr4.sin_addr.s_addr = NTOHL(srcAddr);
		pbn->source.addr4.sin_port = NTOHS(srcPort);
	}
	else {
		pbn->source.addr6.sin6_family = AF_INET6;
		pbn->source.addr6.sin6_addr = *srcAddr6;
		pbn->source.addr6.sin6_port = NTOHS(srcPort);
	}

	if(!destAddr6) {
		pbn->dest.addr4.sin_family = AF_INET;
		pbn->dest.addr4.sin_addr.s_addr = NTOHL(destAddr);
		pbn->dest.addr4.sin_port = NTOHS(destPort);
	}
	else {
		pbn->dest.addr6.sin6_family = AF_INET6;
		pbn->dest.addr6.sin6_addr = *destAddr6;
		pbn->dest.addr6.sin6_port = NTOHS(destPort);
	}
}

static ULONG RealClassifyV4Connect(ULONG protocol, ULONG localAddr, const IN6_ADDR *localAddr6, USHORT localPort, ULONG remoteAddr, const IN6_ADDR *remoteAddr6, USHORT remotePort) {
	PBNOTIFICATION pbn = {0};

	if(protocol == IPPROTO_TCP && !g_internal->blockhttp && (DestinationPortAllowed(remotePort) || SourcePortAllowed(localPort))) {
		pbn.action = 2;
	}
	else {
		pbn.action = CheckRanges(&pbn, remoteAddr);
	}

	pbn.protocol = protocol;
	FillAddrs(&pbn, localAddr, localAddr6, localPort, remoteAddr, remoteAddr6, remotePort);

	Notification_Send(&g_internal->queue, &pbn);
	return pbn.action;
}

static NTSTATUS ClassifyV4Connect(const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
											 VOID* packet, const FWPS_FILTER0* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut)
{
	ULONG protocol, localAddr, remoteAddr;
	USHORT localPort, remotePort;

	if(!g_internal->block) return STATUS_SUCCESS;

	protocol = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL].value.uint16;
	localAddr = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS].value.uint32;
	localPort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT].value.uint16;
	remoteAddr = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS].value.uint32;
	remotePort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT].value.uint16;

	if(!RealClassifyV4Connect(protocol, localAddr, NULL, localPort, remoteAddr, NULL, remotePort)) {
		// Action is block, clear the write flag of the rights member of the FWPS_CLASSIFY_OUT0 structure.
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
		classifyOut->actionType = FWP_ACTION_BLOCK;

		return STATUS_SUCCESS;
	}

	// Action is allow, clear the write flag of the rights member of the FWPS_CLASSIFY_OUT0 structure,
	// IF the FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT of the flags member in the FWPS_FILTER0 structure is set.
	if((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
	}
	classifyOut->actionType = FWP_ACTION_CONTINUE;

	return STATUS_SUCCESS;
}

static ULONG RealClassifyV4Accept(ULONG protocol, ULONG localAddr, const IN6_ADDR *localAddr6, USHORT localPort, ULONG remoteAddr, const IN6_ADDR *remoteAddr6, USHORT remotePort) {
	PBNOTIFICATION pbn = {0};

	pbn.action = CheckRanges(&pbn, remoteAddr);
	pbn.protocol = protocol;
	
	FillAddrs(&pbn, remoteAddr, remoteAddr6, remotePort, localAddr, localAddr6, localPort);

	Notification_Send(&g_internal->queue, &pbn);
	return pbn.action;
}

static NTSTATUS ClassifyV4Accept(const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
											 VOID* packet, const FWPS_FILTER0* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut)
{
	ULONG protocol, localAddr, remoteAddr;
	USHORT localPort, remotePort;

	if(!g_internal->block) return STATUS_SUCCESS;

	protocol = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL].value.uint16;
	localAddr = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS].value.uint32;
	localPort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT].value.uint16;
	remoteAddr = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS].value.uint32;
	remotePort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT].value.uint16;

	if(!RealClassifyV4Accept(protocol, localAddr, NULL, localPort, remoteAddr, NULL, remotePort)) {
		// Action is block, clear the write flag of the rights member of the FWPS_CLASSIFY_OUT0 structure.
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
		classifyOut->actionType = FWP_ACTION_BLOCK;

		return STATUS_SUCCESS;
	}

	// Action is allow, clear the write flag of the rights member of the FWPS_CLASSIFY_OUT0 structure,
	// IF the FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT of the flags member in the FWPS_FILTER0 structure is set.
	if((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
	}
	classifyOut->actionType = FWP_ACTION_CONTINUE;

	return STATUS_SUCCESS;
}

static NTSTATUS ClassifyV6Connect(const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
											 VOID* packet, const FWPS_FILTER0* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut)
{
	const IN6_ADDR *localAddr, *remoteAddr;
	const FAKEV6ADDR *fakeremoteAddr;
	ULONG protocol;
	USHORT localPort, remotePort;
	int action;

	if(!g_internal->block) return STATUS_SUCCESS;

	protocol = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_PROTOCOL].value.uint16;
	localAddr = (const IN6_ADDR *)inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16;
	remoteAddr = (const IN6_ADDR *)inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16;
	localPort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_PORT].value.uint16;
	remotePort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT].value.uint16;
	fakeremoteAddr = (const FAKEV6ADDR*)remoteAddr;

	if(fakeremoteAddr->teredo.prefix == 0x0000120) {
		ULONG realRemoteAddr = ~NTOHL(fakeremoteAddr->teredo.clientip);
		//TODO: get local address.
		action = RealClassifyV4Connect(protocol, 0, localAddr, localPort, realRemoteAddr, remoteAddr, remotePort);
	}
	else if(fakeremoteAddr->sixtofour.prefix == 0x0220) {
		ULONG realRemoteAddr = NTOHL(fakeremoteAddr->sixtofour.clientip);
		action = RealClassifyV4Connect(protocol, 0, localAddr, localPort, realRemoteAddr, remoteAddr, remotePort);
	}
	else {
		fakeremoteAddr = NULL;
		action = 2;
	}

	if(!action) {
		// Action is block, clear the write flag of the rights member of the FWPS_CLASSIFY_OUT0 structure.
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
		classifyOut->actionType = FWP_ACTION_BLOCK;

		return STATUS_SUCCESS;
	}
	else if(!fakeremoteAddr) {
		PBNOTIFICATION pbn = {0};

		pbn.protocol = protocol;

		pbn.source.addr6.sin6_family = AF_INET6;
		pbn.source.addr6.sin6_addr = *localAddr;
		pbn.source.addr6.sin6_port = (USHORT)NTOHS((USHORT)localPort);

		pbn.dest.addr6.sin6_family = AF_INET6;
		pbn.dest.addr6.sin6_addr = *remoteAddr;
		pbn.dest.addr6.sin6_port = (USHORT)NTOHS((USHORT)remotePort);

		pbn.action = 2;
		Notification_Send(&g_internal->queue, &pbn);
	}

	// Action is allow, clear the write flag of the rights member of the FWPS_CLASSIFY_OUT0 structure,
	// IF the FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT of the flags member in the FWPS_FILTER0 structure is set.
	if ((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
	}
	classifyOut->actionType = FWP_ACTION_CONTINUE;

	return STATUS_SUCCESS;
}

static NTSTATUS ClassifyV6Accept(const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
											VOID* packet, const FWPS_FILTER0* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut)
{
	const IN6_ADDR *localAddr, *remoteAddr;
	const FAKEV6ADDR *fakeremoteAddr;
	ULONG protocol;
	USHORT localPort, remotePort;
	int action;

	if(!g_internal->block) return STATUS_SUCCESS;

	protocol = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_PROTOCOL].value.uint16;
	localAddr = (const IN6_ADDR *)inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16;
	remoteAddr = (const IN6_ADDR *)inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16;
	localPort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_PORT].value.uint16;
	remotePort = inFixedValues->incomingValue[FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_PORT].value.uint16;
	fakeremoteAddr = (const FAKEV6ADDR*)remoteAddr;

	if(fakeremoteAddr->teredo.prefix == 0x0000120) {
		ULONG realRemoteAddr = ~NTOHL(fakeremoteAddr->teredo.clientip);
		//TODO: get local address.
		action = RealClassifyV4Accept(protocol, 0, localAddr, localPort, realRemoteAddr, remoteAddr, remotePort);
	}
	else if(fakeremoteAddr->sixtofour.prefix == 0x0220) {
		ULONG realRemoteAddr = NTOHL(fakeremoteAddr->sixtofour.clientip);
		action = RealClassifyV4Accept(protocol, 0, localAddr, localPort, realRemoteAddr, remoteAddr, remotePort);
	}
	else {
		fakeremoteAddr = NULL;
		action = 2;
	}

	if(!action) {
		// Action is block, clear the write flag of the rights member of the FWPS_CLASSIFY_OUT0 structure.
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
		classifyOut->actionType = FWP_ACTION_BLOCK;

		return STATUS_SUCCESS;
	}
	else if(!fakeremoteAddr) {
		PBNOTIFICATION pbn = {0};

		pbn.protocol = protocol;

		pbn.source.addr6.sin6_family = AF_INET6;
		pbn.source.addr6.sin6_addr = *remoteAddr;
		pbn.source.addr6.sin6_port = (USHORT)NTOHS((USHORT)remotePort);

		pbn.dest.addr6.sin6_family = AF_INET6;
		pbn.dest.addr6.sin6_addr = *localAddr;
		pbn.dest.addr6.sin6_port = (USHORT)NTOHS((USHORT)localPort);

		pbn.action = 2;
		Notification_Send(&g_internal->queue, &pbn);
	}

	// Action is allow, clear the write flag of the rights member of the FWPS_CLASSIFY_OUT0 structure,
	// IF the FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT of the flags member in the FWPS_FILTER0 structure is set.
	if((filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)) {
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
	}
	classifyOut->actionType = FWP_ACTION_CONTINUE;

	return STATUS_SUCCESS;
}

static NTSTATUS NTAPI NullNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID *filterKey, const FWPS_FILTER0 *filter) {
	return STATUS_SUCCESS;
}

static NTSTATUS InstallCallouts(PDEVICE_OBJECT device) 
{
	FWPS_CALLOUT0 c = {0};
	NTSTATUS ret;

	DbgPrint("pbfilter:  > Entering InstallCallouts()\n");
	c.notifyFn = NullNotify;

	// IPv4 connect filter.
	
	c.calloutKey = PBWFP_CONNECT_CALLOUT_V4;
	c.classifyFn = ClassifyV4Connect;

	ret = FwpsCalloutRegister0(device, &c, &g_internal->connect4);
	if(!NT_SUCCESS(ret)) return ret;

	DbgPrint("Installed V4 connect callout: %d\n", ret);

	// IPv4 accept filter.
	
	c.calloutKey = PBWFP_ACCEPT_CALLOUT_V4;
	c.classifyFn = ClassifyV4Accept;

	ret = FwpsCalloutRegister0(device, &c, &g_internal->accept4);
	if(!NT_SUCCESS(ret)) return ret;

	DbgPrint("Installed V4 accept callout: %d\n", ret);

	// IPv6 connect filter.
	
	c.calloutKey = PBWFP_CONNECT_CALLOUT_V6;
	c.classifyFn = ClassifyV6Connect;

	ret = FwpsCalloutRegister0(device, &c, &g_internal->connect6);
	if(!NT_SUCCESS(ret)) return ret;

	DbgPrint("Installed V6 connect callout: %d\n", ret);

	// IPv6 accept filter.
	
	c.calloutKey = PBWFP_ACCEPT_CALLOUT_V6;
	c.classifyFn = ClassifyV6Accept;

	ret = FwpsCalloutRegister0(device, &c, &g_internal->accept6);
	if(!NT_SUCCESS(ret)) return ret;

	DbgPrint("Installed V6 accept callout: %d\n", ret);

	DbgPrint("pbfilter:  < Leaving InstallCallouts()\n");
	return ret;
}



static NTSTATUS Driver_OnCreate(PDEVICE_OBJECT device, PIRP irp) 
{
	DbgPrint("pbfilter:  > Entering Driver_OnCreate()\n");
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	DbgPrint("pbfilter:    completing IRP\n");
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	DbgPrint("pbfilter:  < Leaving Driver_OnCreate()\n");
	return STATUS_SUCCESS;
}

static NTSTATUS Driver_OnCleanup(PDEVICE_OBJECT device, PIRP irp) 
{
	DbgPrint("pbfilter:  > Entering Driver_OnCleanup()\n");

	DbgPrint("pbfilter:    resetting internal block/allow lists\n");
	g_internal->block = 0;
	SetRanges(NULL, 0);
	SetRanges(NULL, 1);

	DbgPrint("pbfilter:    removing notificationqueue\n");
	DestroyNotificationQueue(&g_internal->queue);

	DbgPrint("pbfilter:    completing IRP\n");
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	DbgPrint("pbfilter:  < Leaving Driver_OnCleanup()\n");
	return STATUS_SUCCESS;
}

static NTSTATUS Driver_OnDeviceControl(PDEVICE_OBJECT device, PIRP irp) 
{
	PIO_STACK_LOCATION irpstack;
	ULONG controlcode;
	NTSTATUS status;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	irpstack = IoGetCurrentIrpStackLocation(irp);
	controlcode = irpstack->Parameters.DeviceIoControl.IoControlCode;

	switch(controlcode) 
	{
		case IOCTL_PEERBLOCK_HOOK:
			DbgPrint("pbfilter:  > IOCTL_PEERBLOCK_HOOK\n");
			if(irp->AssociatedIrp.SystemBuffer != NULL && irpstack->Parameters.DeviceIoControl.InputBufferLength == sizeof(int)) 
			{
				DbgPrint("pbfilter:    setting block\n");
				g_internal->block = *(int*)irp->AssociatedIrp.SystemBuffer;
			}
			else 
			{
				DbgPrint("pbfilter:  * ERROR: invalid parameter\n");
				irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}
			DbgPrint("pbfilter:  < IOCTL_PEERBLOCK_HOOK\n");
			break;

		case IOCTL_PEERBLOCK_HTTP:
			DbgPrint("pbfilter:  > IOCTL_PEERBLOCK_HTTP\n");
			if(irp->AssociatedIrp.SystemBuffer != NULL && irpstack->Parameters.DeviceIoControl.InputBufferLength == sizeof(int)) 
			{
				DbgPrint("pbfilter:    setting blockhttp\n");
				g_internal->blockhttp = *(int*)irp->AssociatedIrp.SystemBuffer;
			}
			else  
			{
				DbgPrint("pbfilter:  * ERROR: invalid parameter\n");
				irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}
			DbgPrint("pbfilter:  < IOCTL_PEERBLOCK_HTTP\n");
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
				DbgPrint("pbfilter:    setting ranges\n");
				SetRanges(ranges, ranges->block);
			}
			else 
			{
				DbgPrint("pbfilter:  * ERROR: invalid parameter\n");
				irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			}
			DbgPrint("pbfilter:  < IOCTL_PEERBLOCK_SETRANGES\n");
		} break;

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

		} break;

		case IOCTL_PEERBLOCK_SETSOURCEPORTS:
		{
			USHORT *ports;
			ULONG count;

			DbgPrint("pbfilter:    > IOCTL_PEERBLOCK_SETSOURCEPORTS\n");
			ports = irp->AssociatedIrp.SystemBuffer;
			count = irpstack->Parameters.DeviceIoControl.InputBufferLength;

			SetSourcePorts(ports, (USHORT) (count / sizeof(USHORT)));
			DbgPrint("pbfilter:    < IOCTL_PEERBLOCK_SETSOURCEPORTS\n");

		} break;

		default:
			irp->IoStatus.Status=STATUS_INVALID_PARAMETER;
	}

	status = irp->IoStatus.Status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

static void Driver_OnUnload(PDRIVER_OBJECT driver) 
{
	UNICODE_STRING devlink;
	NTSTATUS status;

	DbgPrint("pbfilter:  > Entering Driver_OnUnload()\n");

	DbgPrint("pbfilter:    unregistering callouts\n");

	if(g_internal->connect4) {
		status = FwpsCalloutUnregisterById0(g_internal->connect4);
		DbgPrint("Unregistered V4 connect: %d\n", status);
	}

	if(g_internal->accept4) {
		status = FwpsCalloutUnregisterById0(g_internal->accept4);
		DbgPrint("Unregistered V4 accept: %d\n", status);
	}

	if(g_internal->connect6) {
		status = FwpsCalloutUnregisterById0(g_internal->connect6);
		DbgPrint("Unregistered V6 connect: %d\n", status);
	}

	if(g_internal->accept6) {
		status = FwpsCalloutUnregisterById0(g_internal->accept6);
		DbgPrint("Unregistered V6 accept: %d\n", status);
	}

	DbgPrint("pbfilter:    deleting devobj\n");

	RtlInitUnicodeString(&devlink, DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&devlink);
	IoDeleteDevice(driver->DeviceObject);

	DbgPrint("pbfilter:  < Leaving Driver_OnUnload()\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registrypath) 
{
	UNICODE_STRING devicename;
	PDEVICE_OBJECT device = NULL;
	NTSTATUS status;

	//DbgBreakPoint();

	DbgPrint("pbfilter:  > Entering DriverEntry()\n");
	DbgPrintEx(DPFLTR_IHVNETWORK_ID, DPFLTR_WARNING_LEVEL, "pbfilter:  > Entering DriverEntry()\n");
	RtlInitUnicodeString(&devicename, NT_DEVICE_NAME);

	status = IoCreateDevice(driver, sizeof(PBINTERNAL), &devicename, FILE_DEVICE_PEERBLOCK, 0, FALSE, &device);

	if(NT_SUCCESS(status)) 
	{
		UNICODE_STRING devicelink;

		DbgPrint("pbfilter:    created driver\n");
		RtlInitUnicodeString(&devicelink, DOS_DEVICE_NAME);
		status=IoCreateSymbolicLink(&devicelink, &devicename);

		DbgPrint("pbfilter:    setting up functions\n");
		driver->MajorFunction[IRP_MJ_CREATE] =
		driver->MajorFunction[IRP_MJ_CLOSE] = Driver_OnCreate;
		driver->MajorFunction[IRP_MJ_CLEANUP] = Driver_OnCleanup;
		driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Driver_OnDeviceControl;
		driver->DriverUnload = Driver_OnUnload;

		DbgPrint("pbfilter:    initializing internal data\n");

		device->Flags |= DO_BUFFERED_IO;

		g_internal = device->DeviceExtension;

		KeInitializeSpinLock(&g_internal->rangeslock);
		KeInitializeSpinLock(&g_internal->portslock);
		InitNotificationQueue(&g_internal->queue);

		g_internal->blockedcount = 0;
		g_internal->allowedcount = 0;

		g_internal->destinationportcount = 0;
		g_internal->sourceportcount = 0;

		g_internal->block = 0;
		g_internal->blockhttp = 1;

		g_internal->connect4 = 0;
		g_internal->accept4 = 0;
		g_internal->connect6 = 0;
		g_internal->accept6 = 0;

		DbgPrint("pbfilter:    installing callouts...\n");
		status = InstallCallouts(device);
		DbgPrint("pbfilter:    ...callouts installed\n");
	}

	if(!NT_SUCCESS(status)) 
	{
		DbgPrint("pbfilter:  * ERROR encountered - unloading driver\n");
		Driver_OnUnload(driver);
	}

	DbgPrint("pbfilter:  < Leaving DriverEntry()\n");
	return status;
}
