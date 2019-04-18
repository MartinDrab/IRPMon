
#ifdef _KERNEL_MODE
#include <ntifs.h>
#else
#include <windows.h>
#endif
#include "general-types.h"
#include "request.h"



size_t RequestGetSize(const REQUEST_HEADER *Header)
{
	size_t ret = 0;
	const REQUEST_IRP *irp = NULL;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	const REQUEST_STARTIO *startIo = NULL;
	const REQUEST_DRIVER_DETECTED *drr = CONTAINING_RECORD(Header, REQUEST_DRIVER_DETECTED, Header);
	const REQUEST_DEVICE_DETECTED *der = CONTAINING_RECORD(Header, REQUEST_DEVICE_DETECTED, Header);
	const REQUEST_FILE_OBJECT_NAME_ASSIGNED *ar = NULL;
	const REQUEST_FILE_OBJECT_NAME_DELETED *dr = NULL;

	switch (Header->Type) {
		case ertIRP:
			ret = sizeof(REQUEST_IRP);
			irp = CONTAINING_RECORD(Header, REQUEST_IRP, Header);
			ret += irp->DataSize;
			break;
		case ertIRPCompletion:
			ret = sizeof(REQUEST_IRP_COMPLETION);
			irpComp = CONTAINING_RECORD(Header, REQUEST_IRP_COMPLETION, Header);
			ret += irpComp->DataSize;
			break;
		case ertFastIo:
			ret = sizeof(REQUEST_FASTIO);
			break;
		case ertAddDevice:
			ret = sizeof(REQUEST_ADDDEVICE);
			break;
		case ertDriverUnload:
			ret = sizeof(REQUEST_UNLOAD);
			break;
		case ertStartIo:
			ret = sizeof(REQUEST_STARTIO);
			startIo = CONTAINING_RECORD(Header, REQUEST_STARTIO, Header);
			ret += startIo->DataSize;
			break;
		case ertDriverDetected:
			ret = sizeof(REQUEST_DRIVER_DETECTED) + drr->DriverNameLength;
			break;
		case ertDeviceDetected:
			ret = sizeof(REQUEST_DEVICE_DETECTED) + der->DeviceNameLength;
			break;
		case ertFileObjectNameAssigned:
			ar = CONTAINING_RECORD(Header, REQUEST_FILE_OBJECT_NAME_ASSIGNED, Header);
			ret = sizeof(REQUEST_FILE_OBJECT_NAME_ASSIGNED) + ar->NameLength;
			break;
		case ertFileObjectNameDeleted:
			dr = CONTAINING_RECORD(Header, REQUEST_FILE_OBJECT_NAME_DELETED, Header);
			ret = sizeof(REQUEST_FILE_OBJECT_NAME_DELETED);
			break;
	}

	return ret;
}
