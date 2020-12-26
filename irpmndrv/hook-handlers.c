
#include <ntifs.h>
#include <Ntddstor.h>
#include <fltkernel.h>
#include "preprocessor.h"
#include "allocator.h"
#include "kernel-shared.h"
#include "utils.h"
#include "hook.h"
#include "req-queue.h"
#include "data-loggers.h"
#include "fo-context-table.h"
#include "request.h"
#include "devext-hooks.h"
#include "hook-handlers.h"


#undef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE_ENABLED 0

/************************************************************************/
/*                       GLOBAL VARIABLES                               */
/************************************************************************/

static IO_REMOVE_LOCK _rundownLock;
static FO_CONTEXT_TABLE _foTable;
static PDRIVER_OBJECT _gDriverObject = NULL;


/************************************************************************/
/*                        HELPER ROUTINES                               */
/************************************************************************/


static PREQUEST_FASTIO _CreateFastIoRequest(EFastIoOperationType FastIoType, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PVOID FileObject, PVOID Arg1, PVOID Arg2, PVOID Arg3, PVOID Arg4, PVOID Arg5, PVOID Arg6, PVOID Arg7)
{
	PREQUEST_FASTIO ret = NULL;
	BASIC_CLIENT_INFO clientInfo;
	PFILE_OBJECT_CONTEXT foc = NULL;

	ret = (PREQUEST_FASTIO)RequestMemoryAlloc(sizeof(REQUEST_FASTIO));
	if (ret != NULL) {
		RequestHeaderInit(&ret->Header, DriverObject, DeviceObject, ertFastIo);
		ret->FastIoType = FastIoType;
		ret->FileObject = FileObject;
		ret->PreviousMode = ExGetPreviousMode();
		ret->Arg1 = Arg1;
		ret->Arg2 = Arg2;
		ret->Arg3 = Arg3;
		ret->Arg4 = Arg4;
		ret->Arg5 = Arg5;
		ret->Arg6 = Arg6;
		ret->Arg7 = Arg7;
		ret->IOSBInformation = 0;
		ret->IOSBStatus = STATUS_UNSUCCESSFUL;
		foc = FoTableGet(&_foTable, FileObject);
		if (foc != NULL) {
			clientInfo = *(PBASIC_CLIENT_INFO)(foc + 1);
			_SetRequestFlags(&ret->Header, &clientInfo);
			FoContextDereference(foc);
		}
	}

	return ret;
}


static BOOLEAN _CatchRequest(PDRIVER_HOOK_RECORD DriverHookRecord, PDEVICE_HOOK_RECORD DeviceHookRecord, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (DriverHookRecord->MonitoringEnabled) {
		if (DriverHookRecord->MonitorNewDevices && DeviceHookRecord == NULL) {
			if (DeviceObject != NULL) {
				status = DriverHookRecordAddDevice(DriverHookRecord, DeviceObject, NULL, NULL, TRUE, &deviceRecord);
				if (NT_SUCCESS(status)) {
					PREQUEST_HEADER rq = NULL;

					status = RequestXXXDetectedCreate(ertDeviceDetected, DeviceObject->DriverObject, DeviceObject, &rq);
					if (NT_SUCCESS(status))
						RequestQueueInsert(rq);

					DeviceHookRecordDereference(deviceRecord);
				}

				ret = TRUE;
			}
		} else ret = (DeviceHookRecord != NULL && DeviceHookRecord->MonitoringEnabled);
	}

	return ret;
}


static void _GetHookRecords(PDEVICE_OBJECT *DeviceObject, PDRIVER_HOOK_RECORD *DriverRecord, PDEVICE_HOOK_RECORD *DeviceRecord)
{
	PDRIVER_HOOK_RECORD tmpDriverRecord = NULL;
	PDEVICE_HOOK_RECORD tmpDeviceHookRecord = NULL;
	PPROXY_DEVICE_EXTENSION ext = NULL;
	PDRIVER_OBJECT tmpDriverObject = (*DeviceObject)->DriverObject;
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; DriverRecord=0x%p; DeviceRecord=0x%p", DeviceObject, DriverRecord, DeviceRecord);

	if (tmpDriverObject == _gDriverObject && *DeviceObject != NULL) {
		ext = (PPROXY_DEVICE_EXTENSION)((*DeviceObject)->DeviceExtension);
		if (ext->Type == detProxy) {
			tmpDriverObject = ext->TargetDevice->DriverObject;
			*DeviceObject = ext->TargetDevice;
		}
	}

	tmpDriverRecord = DriverHookRecordGet(tmpDriverObject);
	if (tmpDriverRecord != NULL)
		tmpDeviceHookRecord = DriverHookRecordGetDevice(tmpDriverRecord, *DeviceObject);

	*DriverRecord = tmpDriverRecord;
	*DeviceRecord = tmpDeviceHookRecord;

	DEBUG_EXIT_FUNCTION("void, *DriverRecord=0x%p, *DeviceRecord=0x%p", *DriverRecord, *DeviceRecord);
	return;
}


static void _GetOriginalFastIo(PDRIVER_HOOK_RECORD DriverRecord, PFAST_IO_DISPATCH *FastIo)
{
	if (DriverRecord->DeviceExtensionHook)
		*FastIo = DriverRecord->DriverObject->FastIoDispatch;
	else *FastIo = &DriverRecord->OldFastIoDisptach;

	return;
}



#define GetDeviceObject(aFileObject)			\
	(((aFileObject)->Vpb != NULL) ? (aFileObject)->Vpb->DeviceObject : (aFileObject)->DeviceObject)		\


/************************************************************************/
/*                        FAST IO ROUTINES                              */
/************************************************************************/

BOOLEAN HookHandlerFastIoCheckIfPossible(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait, ULONG LockKey, BOOLEAN CheckForReadOperation, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoCheckIfPossible, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)CheckForReadOperation, (PVOID)Wait, (PVOID)LockKey, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoCheckIfPossible != NULL)
			ret = fastIo->FastIoCheckIfPossible(FileObject, FileOffset, Length, Wait, LockKey, CheckForReadOperation, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				request->IOSBStatus = IoStatusBlock->Status;
				request->IOSBInformation = IoStatusBlock->Information;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


VOID HookHandlerFastIoDetachDevice(PDEVICE_OBJECT SourceDevice, PDEVICE_OBJECT TargetDevice)
{
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&SourceDevice, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, SourceDevice))
			request = _CreateFastIoRequest(FastIoDetachDevice, SourceDevice->DriverObject, SourceDevice, NULL, SourceDevice, TargetDevice, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoDetachDevice != NULL)
			fastIo->FastIoDetachDevice(SourceDevice, TargetDevice);
		
		if (request != NULL)
			RequestQueueInsert(&request->Header);

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", SourceDevice->DriverObject);
	}

	return;
}


BOOLEAN HookHandlerFastIoDeviceControl(PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG ControlCode, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoDeviceControl, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)OutputBufferLength, (PVOID)InputBufferLength, (PVOID)ControlCode, (PVOID)Wait, InputBuffer, OutputBuffer, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoDeviceControl != NULL)
			ret = fastIo->FastIoDeviceControl(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ControlCode, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoLock(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PLARGE_INTEGER Length, PEPROCESS ProcessId, ULONG Key, BOOLEAN FailImmediately, BOOLEAN Exclusive, PIO_STATUS_BLOCK StatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoLock, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length->LowPart, (PVOID)Length->HighPart, (PVOID)(((FailImmediately != 0) << 1) + (Exclusive != 0)), ProcessId, (PVOID)Key);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoLock != NULL)
			ret = fastIo->FastIoLock(FileObject, FileOffset, Length, ProcessId, Key, FailImmediately, Exclusive, StatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && StatusBlock != NULL) {
				request->IOSBInformation = StatusBlock->Information;
				request->IOSBStatus = StatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoQueryBasicInfo(PFILE_OBJECT FileObject, BOOLEAN Wait, PFILE_BASIC_INFORMATION Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoQueryBasicInfo, DeviceObject->DriverObject, DeviceObject, FileObject, Buffer, (PVOID)Wait, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoQueryBasicInfo != NULL)
			ret = fastIo->FastIoQueryBasicInfo(FileObject, Wait, Buffer, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				if (NT_SUCCESS(IoStatusBlock->Status)) {
					request->Arg1 = (PVOID)Buffer->CreationTime.LowPart;
					request->Arg2 = (PVOID)Buffer->CreationTime.HighPart;
					request->Arg3 = (PVOID)Buffer->LastAccessTime.LowPart;
					request->Arg4 = (PVOID)Buffer->LastAccessTime.HighPart;
					request->Arg5 = (PVOID)Buffer->LastWriteTime.LowPart;
					request->Arg6 = (PVOID)Buffer->LastWriteTime.HighPart;
					request->Arg7 = (PVOID)Buffer->FileAttributes;
					/* TODO: Add more members */
				}

				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoQueryNetworkOpenInfo(PFILE_OBJECT FileObject, BOOLEAN Wait, PFILE_NETWORK_OPEN_INFORMATION Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoQueryNetworkOpenInfo, DeviceObject->DriverObject, DeviceObject, FileObject, Buffer, (PVOID)Wait, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoQueryNetworkOpenInfo != NULL)
			ret = fastIo->FastIoQueryNetworkOpenInfo(FileObject, Wait, Buffer, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				if (NT_SUCCESS(IoStatusBlock->Status)) {
					request->Arg1 = (PVOID)Buffer->CreationTime.LowPart;
					request->Arg2 = (PVOID)Buffer->CreationTime.HighPart;
					request->Arg3 = (PVOID)Buffer->LastAccessTime.LowPart;
					request->Arg4 = (PVOID)Buffer->LastAccessTime.HighPart;
					request->Arg5 = (PVOID)Buffer->LastWriteTime.LowPart;
					request->Arg6 = (PVOID)Buffer->LastWriteTime.HighPart;
					request->Arg7 = (PVOID)Buffer->FileAttributes;
					/* TODO: Add more members */
				}

				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoQueryOpenInfo(PIRP Irp, PFILE_NETWORK_OPEN_INFORMATION Buffer, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoQueryOpen, DeviceObject->DriverObject, DeviceObject, irpStack->FileObject, Irp, Buffer, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (driverRecord->DeviceExtensionHook)
			irpStack->DeviceObject = DeviceObject;
		
		if (fastIo->FastIoQueryOpen != NULL)
			ret = fastIo->FastIoQueryOpen(Irp, Buffer, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret) {
				request->Arg1 = (PVOID)Buffer->CreationTime.LowPart;
				request->Arg2 = (PVOID)Buffer->CreationTime.HighPart;
				request->Arg3 = (PVOID)Buffer->LastAccessTime.LowPart;
				request->Arg4 = (PVOID)Buffer->LastAccessTime.HighPart;
				request->Arg5 = (PVOID)Buffer->LastWriteTime.LowPart;
				request->Arg6 = (PVOID)Buffer->LastWriteTime.HighPart;
				request->Arg7 = (PVOID)Buffer->FileAttributes;
				/* TODO: Add more members */
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoQueryStandardInfo(PFILE_OBJECT FileObject, BOOLEAN Wait, PFILE_STANDARD_INFORMATION Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoQueryStandardInfo, DeviceObject->DriverObject, DeviceObject, FileObject, Buffer, (PVOID)Wait, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoQueryStandardInfo != NULL)
			ret = fastIo->FastIoQueryStandardInfo(FileObject, Wait, Buffer, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				if (NT_SUCCESS(IoStatusBlock->Status)) {
					request->Arg1 = (PVOID)Buffer->AllocationSize.LowPart;
					request->Arg2 = (PVOID)Buffer->AllocationSize.HighPart;
					request->Arg3 = (PVOID)Buffer->EndOfFile.LowPart;
					request->Arg4 = (PVOID)Buffer->EndOfFile.HighPart;
					request->Arg5 = (PVOID)Buffer->NumberOfLinks;
					request->Arg6 = (PVOID)Buffer->Directory;
					request->Arg7 = (PVOID)Buffer->DeletePending;
					/* TODO: Add more members (Buffer,Wait) */
				}

				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoRead(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait, ULONG LockKey, PVOID Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoRead, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, (PVOID)Wait, Buffer, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoRead != NULL)
			ret = fastIo->FastIoRead(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoUnlockAll(PFILE_OBJECT FileObject, PEPROCESS ProcessId, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoUnlockAll, DeviceObject->DriverObject, DeviceObject, FileObject, ProcessId, NULL, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoUnlockAll != NULL)
			ret = fastIo->FastIoUnlockAll(FileObject, ProcessId, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoUnlockByKey(PFILE_OBJECT FileObject, PVOID ProcessId, ULONG Key, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoUnlockAllByKey, DeviceObject->DriverObject, DeviceObject, FileObject, ProcessId, (PVOID)Key, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoUnlockAllByKey != NULL)
			ret = fastIo->FastIoUnlockAllByKey(FileObject, ProcessId, Key, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoUnlockSingle(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PLARGE_INTEGER Length, PEPROCESS ProcessId, ULONG Key, PIO_STATUS_BLOCK StatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoUnlockSingle, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length->LowPart, (PVOID)Length->HighPart, ProcessId, (PVOID)Key, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoUnlockSingle != NULL)
			ret = fastIo->FastIoUnlockSingle(FileObject, FileOffset, Length, ProcessId, Key, StatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && StatusBlock != NULL) {
				request->IOSBInformation = StatusBlock->Information;
				request->IOSBStatus = StatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoWrite(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait, ULONG LockKey, PVOID Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoWrite, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, (PVOID)Wait, Buffer, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoWrite != NULL)
			ret = fastIo->FastIoWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p, DeviceObject 0x%p", DeviceObject->DriverObject, DeviceObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->FastIoWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatusBlock, DeviceObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoMdlRead(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, ULONG LockKey, PMDL *MdlChain, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(MdlRead, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, MdlChain, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->MdlRead != NULL)
			ret = fastIo->MdlRead(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->MdlRead(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatusBlock, DeviceObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoMdlWrite(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, ULONG LockKey, PMDL *MdlChain, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(PrepareMdlWrite, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, MdlChain, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->PrepareMdlWrite != NULL)
			ret = fastIo->PrepareMdlWrite(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatusBlock, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->PrepareMdlWrite(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatusBlock, DeviceObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoMdlReadComplete(PFILE_OBJECT FileObject, PMDL MdlChain, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(MdlReadComplete, DeviceObject->DriverObject, DeviceObject, FileObject, MdlChain, NULL, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->MdlReadComplete != NULL)
			ret = fastIo->MdlReadComplete(FileObject, MdlChain, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->MdlReadComplete(FileObject, MdlChain, DeviceObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoMdlWriteComplete(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PMDL MdlChain, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(MdlWriteComplete, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, MdlChain, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->MdlWriteComplete != NULL)
			ret = fastIo->MdlWriteComplete(FileObject, FileOffset, MdlChain, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->MdlWriteComplete(FileObject, FileOffset, MdlChain, DeviceObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoReadCompressed(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, ULONG LockKey, PVOID Buffer, PMDL *MdlChain, PIO_STATUS_BLOCK IoStatusBlock, PCOMPRESSED_DATA_INFO CompressedInfo, ULONG CompressedInfoLength, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoReadCompressed, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, (PVOID)Buffer, (PVOID)CompressedInfoLength, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoReadCompressed != NULL)
			ret = fastIo->FastIoReadCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatusBlock, CompressedInfo, CompressedInfoLength, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				if (NT_SUCCESS(IoStatusBlock->Status)) {
					if (MdlChain != NULL)
						request->Arg7 = *MdlChain;				
				}

				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->FastIoReadCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatusBlock, CompressedInfo, CompressedInfoLength, DeviceObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoWriteCompressed(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, ULONG LockKey, PVOID Buffer, PMDL *MdlChain, PIO_STATUS_BLOCK IoStatusBlock, PCOMPRESSED_DATA_INFO CompressedInfo, ULONG CompressedInfoLength, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(FastIoWriteCompressed, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, Buffer, (PVOID)CompressedInfoLength, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->FastIoWriteCompressed != NULL)
			ret = fastIo->FastIoWriteCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatusBlock, CompressedInfo, CompressedInfoLength, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			if (ret && IoStatusBlock != NULL) {
				if (NT_SUCCESS(IoStatusBlock->Status)) {
					if (MdlChain != NULL)
						request->Arg7 = *MdlChain;				
				}

				request->IOSBInformation = IoStatusBlock->Information;
				request->IOSBStatus = IoStatusBlock->Status;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->FastIoWriteCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatusBlock, CompressedInfo, CompressedInfoLength, DeviceObject);
	}

	return ret;
}


NTSTATUS HookHandlerFastIoAcquireForModWrite(PFILE_OBJECT FileObject, PLARGE_INTEGER EndingOffset, PERESOURCE *ResourceToRelease, PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	LARGE_INTEGER offset;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject)) {
			offset.QuadPart = -1;
			if (EndingOffset != NULL)
				offset = *EndingOffset;

			request = _CreateFastIoRequest(AcquireForModWrite, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)offset.LowPart, (PVOID)offset.HighPart, NULL, NULL, NULL, NULL, NULL);
		}

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->AcquireForModWrite != NULL)
			status = fastIo->AcquireForModWrite(FileObject, EndingOffset, ResourceToRelease, DeviceObject);
		else status = STATUS_SUCCESS;

		if (request != NULL) {
			RequestHeaderSetResult(request->Header, NTSTATUS, status);
			if (NT_SUCCESS(status) && ResourceToRelease != NULL)
				request->Arg3 = *ResourceToRelease;

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		status = DeviceObject->DriverObject->FastIoDispatch->AcquireForModWrite(FileObject, EndingOffset, ResourceToRelease, DeviceObject);
	}

	return status;
}


NTSTATUS HookHandlerFastIoReleaseForModWrite(PFILE_OBJECT FileObject, PERESOURCE ResourceToRelease, PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(ReleaseForModWrite, DeviceObject->DriverObject, DeviceObject, FileObject, ResourceToRelease, NULL, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->ReleaseForModWrite != NULL)
			status = fastIo->ReleaseForModWrite(FileObject, ResourceToRelease, DeviceObject);
		else status = STATUS_SUCCESS;

		if (request != NULL) {
			RequestHeaderSetResult(request->Header, NTSTATUS, status);
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		status = DeviceObject->DriverObject->FastIoDispatch->ReleaseForModWrite(FileObject, ResourceToRelease, DeviceObject);
	}

	return status;
}


NTSTATUS HookHandlerFastIoAcquireForCcFlush(PFILE_OBJECT FileObject, PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(AcquireForCcFlush, DeviceObject->DriverObject, DeviceObject, FileObject, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->AcquireForCcFlush != NULL)
			status = fastIo->AcquireForCcFlush(FileObject, DeviceObject);
		else status = STATUS_SUCCESS;

		if (request != NULL) {
			RequestHeaderSetResult(request->Header, NTSTATUS, status);
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		status = DeviceObject->DriverObject->FastIoDispatch->AcquireForCcFlush(FileObject, DeviceObject);
	}

	return status;
}


NTSTATUS HookHandlerFastIoReleaseForCcFlush(PFILE_OBJECT FileObject, PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(ReleaseForCcFlush, DeviceObject->DriverObject, DeviceObject, FileObject, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->ReleaseForCcFlush != NULL)
			status = fastIo->ReleaseForCcFlush(FileObject, DeviceObject);
		else status = STATUS_SUCCESS;

		if (request != NULL) {
			RequestHeaderSetResult(request->Header, NTSTATUS, status);
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		status = DeviceObject->DriverObject->FastIoDispatch->ReleaseForCcFlush(FileObject, DeviceObject);
	}

	return status;
}


BOOLEAN HookHandlerFastIoMdlReadCompleteCompressed(PFILE_OBJECT FileObject, PMDL MdlChain, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(MdlReadCompleteCompressed, DeviceObject->DriverObject, DeviceObject, FileObject, MdlChain, NULL, NULL, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->MdlReadCompleteCompressed != NULL)
			ret = fastIo->MdlReadCompleteCompressed(FileObject, MdlChain, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->MdlReadCompleteCompressed(FileObject, MdlChain, DeviceObject);
	}

	return ret;
}


BOOLEAN HookHandlerFastIoMdlWriteCompleteCompressed(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PMDL MdlChain,  PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PFAST_IO_DISPATCH fastIo = NULL;

	_GetHookRecords(&DeviceObject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (_CatchRequest(driverRecord, deviceRecord, DeviceObject))
			request = _CreateFastIoRequest(MdlWriteCompleteCompressed, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, MdlChain, NULL, NULL, NULL, NULL);

		_GetOriginalFastIo(driverRecord, &fastIo);
		if (fastIo->MdlWriteCompleteCompressed != NULL)
			ret = fastIo->MdlWriteCompleteCompressed(FileObject, FileOffset, MdlChain, DeviceObject);
		
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, BOOLEAN, ret);
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		ret = DeviceObject->DriverObject->FastIoDispatch->MdlWriteCompleteCompressed(FileObject, FileOffset, MdlChain, DeviceObject);
	}

	return ret;
}

/************************************************************************/
/*                  NON-FAST IO HOOKS                                   */
/************************************************************************/

VOID HookHandlerStartIoDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PREQUEST_STARTIO request = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	PIO_STATUS_BLOCK iosb = Irp->UserIosb;
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", DeviceObject, Irp);

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (driverRecord->MonitorStartIo && _CatchRequest(driverRecord, deviceRecord, DeviceObject)) {
			DATA_LOGGER_RESULT loggedData;

			memset(&loggedData, 0, sizeof(loggedData));
			if (driverRecord->MonitorData)
				IRPDataLogger(DeviceObject, Irp, IrpStack, FALSE, &loggedData);
			
			request = (PREQUEST_STARTIO)RequestMemoryAlloc(sizeof(REQUEST_STARTIO) + loggedData.BufferSize);
			if (request != NULL) {
				RequestHeaderInit(&request->Header, DeviceObject->DriverObject, DeviceObject, ertStartIo);
				request->IRPAddress = Irp;
				request->MajorFunction = IrpStack->MajorFunction;
				request->MinorFunction = IrpStack->MinorFunction;
				request->IrpFlags = Irp->Flags;
				request->FileObject = IrpStack->FileObject;
				request->Status = STATUS_UNSUCCESSFUL;
				request->Information = 0;
				if (loggedData.Stripped)
					request->Header.Flags |= REQUEST_FLAG_DATA_STRIPPED;
				
				if (loggedData.Buffer != NULL && loggedData.BufferSize > 0) {
					request->DataSize = loggedData.BufferSize;
					__try {
						memcpy(request + 1, loggedData.Buffer, request->DataSize);
					} __except (EXCEPTION_EXECUTE_HANDLER) {

					}
				}
			}

			if (driverRecord->MonitorData)
				DataLoggerResultRelease(&loggedData);
		}

		driverRecord->OldStartIo(DeviceObject, Irp);
		if (request != NULL) {
			if (iosb != NULL) {
				request->Status = iosb->Status;
				request->Information = iosb->Information;
			}

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

typedef struct _IRP_COMPLETION_CONTEXT {
	volatile LONG ReferenceCount;
	PVOID OriginalContext;
	PIO_COMPLETION_ROUTINE OriginalRoutine;
	ULONG OriginalControl;
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;
	volatile PREQUEST_IRP_COMPLETION CompRequest;
	IO_STACK_LOCATION StackLocation;
	BASIC_CLIENT_INFO ClientInfo;
	HANDLE RequestorProcessId;
	KPROCESSOR_MODE PreviousMode;
	KPROCESSOR_MODE RequestorMode;
} IRP_COMPLETION_CONTEXT, *PIRP_COMPLETION_CONTEXT;


static NTSTATUS _HookHandlerIRPCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	DATA_LOGGER_RESULT loggedData;
	PIO_STACK_LOCATION nextStack = NULL;
	NTSTATUS status = STATUS_CONTINUE_COMPLETION;
	NTSTATUS irpStatus = Irp->IoStatus.Status;
	PREQUEST_IRP_COMPLETION completionRequest = NULL;
	PIRP_COMPLETION_CONTEXT cc = (PIRP_COMPLETION_CONTEXT)Context;
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p; Context=0x%p", DeviceObject, Irp, Context);

	memset(&loggedData, 0, sizeof(loggedData));
	driverRecord = DriverHookRecordGet(cc->DriverObject);
	if (driverRecord != NULL) {
		if (driverRecord->MonitorData)
			IRPDataLogger(cc->DeviceObject, Irp, &cc->StackLocation, TRUE, &loggedData);	
	}

	if (cc->StackLocation.MajorFunction == IRP_MJ_CREATE &&
		NT_SUCCESS(Irp->IoStatus.Status)) {
		FoTableInsert(&_foTable, cc->StackLocation.FileObject, &cc->ClientInfo, sizeof(cc->ClientInfo));
	}

	completionRequest = (PREQUEST_IRP_COMPLETION)RequestMemoryAlloc(sizeof(REQUEST_IRP_COMPLETION) + loggedData.BufferSize);
	if (completionRequest != NULL) {
		RequestHeaderInit(&completionRequest->Header, cc->DriverObject, cc->DeviceObject, ertIRPCompletion);
		completionRequest->IRPAddress = Irp;
		completionRequest->CompletionInformation = Irp->IoStatus.Information;
		completionRequest->CompletionStatus = Irp->IoStatus.Status;
		completionRequest->FileObject = cc->StackLocation.FileObject;
		memcpy(completionRequest->Arguments, &cc->StackLocation.Parameters.Others, sizeof(completionRequest->Arguments));
		completionRequest->MajorFunction = cc->StackLocation.MajorFunction;
		completionRequest->MinorFunction = cc->StackLocation.MinorFunction;
		completionRequest->PreviousMode = cc->PreviousMode;
		completionRequest->RequestorMode = cc->RequestorMode;
		completionRequest->RequestorProcessId = (ULONG_PTR)cc->RequestorProcessId;
		cc->CompRequest = completionRequest;
		_SetRequestFlags(&completionRequest->Header, &cc->ClientInfo);
		IRPDataLoggerSetRequestFlags(&completionRequest->Header, &loggedData);
		if (loggedData.BufferSize > 0 && loggedData.Buffer != NULL) {
			completionRequest->DataSize = loggedData.BufferSize;
			__try {
				memcpy(completionRequest + 1, loggedData.Buffer, completionRequest->DataSize);
			} __except (EXCEPTION_EXECUTE_HANDLER) {

			}
		}
	}

	if (driverRecord != NULL) {
		if (driverRecord->MonitorData)
			DataLoggerResultRelease(&loggedData);
	
		DriverHookRecordDereference(driverRecord);
	}

	// Change the next (well, its the previous one in the completion path)
	// location Context and CompletionRoutine to the original data specified
	// by the higher driver.
	nextStack = IoGetNextIrpStackLocation(Irp);
	if (nextStack->Context == cc)
		nextStack->Context = cc->OriginalContext;
	
	if (nextStack->CompletionRoutine == _HookHandlerIRPCompletion)
		nextStack->CompletionRoutine = cc->OriginalRoutine;

	if (cc->OriginalRoutine != NULL &&
		// Inspired by IoCompleteRequest
		((Irp->Cancel && (cc->OriginalControl & SL_INVOKE_ON_CANCEL)) ||
		(!NT_SUCCESS(irpStatus) && (cc->OriginalControl & SL_INVOKE_ON_ERROR)) ||
		(NT_SUCCESS(irpStatus) && (cc->OriginalControl & SL_INVOKE_ON_SUCCESS))))
		status = cc->OriginalRoutine(DeviceObject, Irp, cc->OriginalContext);
	else if (Irp->PendingReturned && Irp->CurrentLocation < Irp->StackCount)
		// Inspired by IoCompleteRequest
		IoMarkIrpPending(Irp);

	if (completionRequest != NULL)
		RequestHeaderSetResult(completionRequest->Header, NTSTATUS, status);

	if (InterlockedDecrement(&cc->ReferenceCount) == 0) {
		HeapMemoryFree(cc);
		if (completionRequest != NULL)
			RequestQueueInsert(&completionRequest->Header);
	}

	IoReleaseRemoveLock(&_rundownLock, Irp);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static PIRP_COMPLETION_CONTEXT _HookIRPCompletionRoutine(PIRP Irp, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject)
{
	PIO_STACK_LOCATION irpStack = NULL;
	PIRP_COMPLETION_CONTEXT ret = NULL;
	DEBUG_ENTER_FUNCTION("Irp=0x%p; DriverObject=0x%p; DeviceObject=0x%p", Irp, DriverObject, DeviceObject);

	if (NT_SUCCESS(IoAcquireRemoveLock(&_rundownLock, Irp))) {
		ret = HeapMemoryAllocNonPaged(sizeof(IRP_COMPLETION_CONTEXT));
		if (ret != NULL) {
			RtlSecureZeroMemory(ret, sizeof(IRP_COMPLETION_CONTEXT));
			ret->ReferenceCount = 1;
			ret->DriverObject = DriverObject;
			ret->DeviceObject = DeviceObject;
			ret->PreviousMode = ExGetPreviousMode();
			ret->RequestorMode = Irp->RequestorMode;
			ret->RequestorProcessId = (HANDLE)IoGetRequestorProcessId(Irp);
			irpStack = IoGetCurrentIrpStackLocation(Irp);
			ret->StackLocation = *irpStack;
			if (irpStack->CompletionRoutine != NULL) {
				ret->OriginalContext = irpStack->Context;
				ret->OriginalRoutine = irpStack->CompletionRoutine;
				ret->OriginalControl = irpStack->Control;
			}

			IoSkipCurrentIrpStackLocation(Irp);
			IoSetCompletionRoutine(Irp, _HookHandlerIRPCompletion, ret, TRUE, TRUE, TRUE);
			IoSetNextIrpStackLocation(Irp);
		}

		if (ret == NULL)
			IoReleaseRemoveLock(&_rundownLock, Irp);
	}

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


NTSTATUS HookHandlerIRPDisptach(PDEVICE_OBJECT Deviceobject, PIRP Irp)
{
	UNICODE_STRING uPreCreateFileName;
	BOOLEAN catchRequest = FALSE;
	BASIC_CLIENT_INFO clientInfo;
	BOOLEAN isCleanup = FALSE;
	PFILE_OBJECT cleanupFileObject = NULL;
	BOOLEAN isCreate = FALSE;
	BOOLEAN isCreatePipe = FALSE;
	BOOLEAN isCreateMailslot = FALSE;
	PFILE_OBJECT createFileObject = NULL;
	PIRP_COMPLETION_CONTEXT compContext = NULL;
	PREQUEST_IRP request = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PREQUEST_FILE_OBJECT_NAME_DELETED rfond = NULL;
	BOOLEAN openByFileId = FALSE;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", Deviceobject, Irp);

	_GetHookRecords(&Deviceobject, &driverRecord, &deviceRecord);
	if (driverRecord != NULL) {
		if (driverRecord->DeviceExtensionHook)
			irpStack->DeviceObject = Deviceobject;
		
		memset(&clientInfo, 0, sizeof(clientInfo));
		memset(&uPreCreateFileName, 0, sizeof(uPreCreateFileName));
		catchRequest = (_CatchRequest(driverRecord, deviceRecord, Deviceobject) && (deviceRecord == NULL || deviceRecord->IRPMonitorSettings[irpStack->MajorFunction]));
		if (catchRequest) {
			isCreate = (irpStack->MajorFunction == IRP_MJ_CREATE);
			isCreatePipe = (irpStack->MajorFunction == IRP_MJ_CREATE_NAMED_PIPE);
			isCreateMailslot = (irpStack->MajorFunction == IRP_MJ_CREATE_MAILSLOT);
			if (isCreate || isCreatePipe || isCreateMailslot) {
				if (isCreate)
					openByFileId = (irpStack->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID) != 0;
				
				createFileObject = irpStack->FileObject;
				QueryClientBasicInformation(&clientInfo);
				if (createFileObject != NULL && KeGetCurrentIrql() < DISPATCH_LEVEL && !openByFileId)
					FileNameFromFileObject(createFileObject, &uPreCreateFileName);
			} else if (irpStack->FileObject != NULL) {
				PFILE_OBJECT_CONTEXT foc = NULL;

				foc = FoTableGet(&_foTable, irpStack->FileObject);
				if (foc != NULL) {
					clientInfo = *(PBASIC_CLIENT_INFO)(foc + 1);
					FoContextDereference(foc);
				}
			}

			if (driverRecord->MonitorIRP) {
				DATA_LOGGER_RESULT loggedData;

				memset(&loggedData, 0, sizeof(loggedData));
				if (driverRecord->MonitorData)
					IRPDataLogger(Deviceobject, Irp, irpStack, FALSE, &loggedData);

				request = (PREQUEST_IRP)RequestMemoryAlloc(sizeof(REQUEST_IRP) + loggedData.BufferSize);
				if (request != NULL) {
					RequestHeaderInit(&request->Header, Deviceobject->DriverObject, Deviceobject, ertIRP);
					RequestHeaderSetResult(request->Header, NTSTATUS, STATUS_PENDING);
					request->IRPAddress = Irp;
					request->MajorFunction = irpStack->MajorFunction;
					request->MinorFunction = irpStack->MinorFunction;
					request->PreviousMode = ExGetPreviousMode();
					request->RequestorMode = Irp->RequestorMode;
					request->Arg1 = irpStack->Parameters.Others.Argument1;
					request->Arg2 = irpStack->Parameters.Others.Argument2;
					request->Arg3 = irpStack->Parameters.Others.Argument3;
					request->Arg4 = irpStack->Parameters.Others.Argument4;
					request->IrpFlags = Irp->Flags;
					request->FileObject = irpStack->FileObject;
					request->IOSBStatus = Irp->IoStatus.Status;
					request->IOSBInformation = Irp->IoStatus.Information;
					request->RequestorProcessId = IoGetRequestorProcessId(Irp);
					_SetRequestFlags(&request->Header, &clientInfo);
					IRPDataLoggerSetRequestFlags(&request->Header, &loggedData);
					if (loggedData.BufferSize > 0 && loggedData.Buffer != NULL) {
						request->DataSize = loggedData.BufferSize;
						__try {
							memcpy(request + 1, loggedData.Buffer, request->DataSize);
						} __except (EXCEPTION_EXECUTE_HANDLER) {
						}
					}
				}

				if (driverRecord->MonitorData)
					DataLoggerResultRelease(&loggedData);
			}

			if (driverRecord->MonitorIRPCompletion) {
				compContext = _HookIRPCompletionRoutine(Irp, Deviceobject->DriverObject, Deviceobject);
				if (compContext != NULL) {
					compContext->ClientInfo = clientInfo;
					if (request != NULL)
						InterlockedIncrement(&compContext->ReferenceCount);
				}
			}

			isCleanup = (irpStack->MajorFunction == IRP_MJ_CLEANUP);
			if (isCleanup) {
				cleanupFileObject = irpStack->FileObject;
				if (cleanupFileObject != NULL) {
					if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
						rfond = (PREQUEST_FILE_OBJECT_NAME_DELETED)RequestMemoryAlloc(sizeof(REQUEST_FILE_OBJECT_NAME_DELETED));
						if (rfond != NULL) {
							RequestHeaderInit(&rfond->Header, Deviceobject->DriverObject, Deviceobject, ertFileObjectNameDeleted);
							RequestHeaderSetResult(rfond->Header, NTSTATUS, STATUS_SUCCESS);
							rfond->FileObject = cleanupFileObject;
							_SetRequestFlags(&rfond->Header, &clientInfo);
						}
					}

					FoTableDeleteNoReturn(&_foTable, cleanupFileObject);
				}
			}
		}

		status = (driverRecord->DeviceExtensionHook) ? 
			driverRecord->DriverObject->MajorFunction[irpStack->MajorFunction](Deviceobject, Irp) :
			driverRecord->OldMajorFunction[irpStack->MajorFunction](Deviceobject, Irp);
		
		if (catchRequest && (isCreate || isCreateMailslot || isCreatePipe) && createFileObject != NULL) {
			PREQUEST_FILE_OBJECT_NAME_ASSIGNED ar = NULL;

			if (NT_SUCCESS(status) && isCreate && KeGetCurrentIrql() < DISPATCH_LEVEL && status != STATUS_PENDING) {
				PFLT_FILE_NAME_INFORMATION fi = NULL;
				NTSTATUS tmpStatus = STATUS_UNSUCCESSFUL;

				tmpStatus = FltGetFileNameInformationUnsafe(createFileObject, NULL, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &fi);
				if (NT_SUCCESS(tmpStatus)) {
					tmpStatus = FltParseFileNameInformation(fi);
					if (NT_SUCCESS(tmpStatus)) {
						UNICODE_STRING uFileName;

						uFileName = fi->Name;
						if (fi->Volume.Length > 0 && fi->Volume.Length <= fi->Name.Length) {
							uFileName.Length -= fi->Volume.Length;
							uFileName.MaximumLength = uFileName.Length;
							uFileName.Buffer += fi->Volume.Length / sizeof(wchar_t);
						}

						ar = (PREQUEST_FILE_OBJECT_NAME_ASSIGNED)RequestMemoryAlloc(sizeof(REQUEST_FILE_OBJECT_NAME_ASSIGNED) + uFileName.Length);
						if (ar != NULL) {
							RequestHeaderInitNoId(&ar->Header, Deviceobject->DriverObject, Deviceobject, ertFileObjectNameAssigned);
							RequestHeaderSetResult(ar->Header, NTSTATUS, STATUS_SUCCESS);
							ar->FileObject = createFileObject;
							ar->NameLength = uFileName.Length;
							memcpy(ar + 1, uFileName.Buffer, uFileName.Length);
							_SetRequestFlags(&ar->Header, &clientInfo);
							RequestQueueInsert(&ar->Header);
						} else tmpStatus = STATUS_INSUFFICIENT_RESOURCES;
					}

					FltReleaseFileNameInformation(fi);
				}

				if (NT_SUCCESS(status) && status != STATUS_PENDING)
					FoTableInsert(&_foTable, createFileObject, &clientInfo, sizeof(clientInfo));
			} else if (uPreCreateFileName.Length > 0) {
				ar = (PREQUEST_FILE_OBJECT_NAME_ASSIGNED)RequestMemoryAlloc(sizeof(REQUEST_FILE_OBJECT_NAME_ASSIGNED) + uPreCreateFileName.Length);
				if (ar != NULL) {
					RequestHeaderInitNoId(&ar->Header, Deviceobject->DriverObject, Deviceobject, ertFileObjectNameAssigned);
					RequestHeaderSetResult(ar->Header, NTSTATUS, STATUS_SUCCESS);
					ar->FileObject = createFileObject;
					ar->NameLength = uPreCreateFileName.Length;
					memcpy(ar + 1, uPreCreateFileName.Buffer, uPreCreateFileName.Length);
					_SetRequestFlags(&ar->Header, &clientInfo);
					RequestQueueInsert(&ar->Header);
				}
			}
		}

		if (request != NULL) {
			RequestHeaderSetResult(request->Header, NTSTATUS, status);
			RequestQueueInsert(&request->Header);
		}

		if (compContext != NULL && InterlockedDecrement(&compContext->ReferenceCount) == 0) {
			RequestQueueInsert(&compContext->CompRequest->Header);
			HeapMemoryFree(compContext);
		}

		if (rfond != NULL)
			RequestQueueInsert(&rfond->Header);

		if (uPreCreateFileName.Buffer != NULL)
			HeapMemoryFree(uPreCreateFileName.Buffer);

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", Deviceobject->DriverObject);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS HookHandlerAddDeviceDispatch(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject)
{
	PDEVICE_OBJECT detectedDevice = NULL;
	PREQUEST_ADDDEVICE request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; PhysicalDeviceObject=0x%p", DriverObject, PhysicalDeviceObject);

	driverRecord = DriverHookRecordGet(DriverObject);
	if (driverRecord != NULL) {
		if (driverRecord->MonitoringEnabled && driverRecord->MonitorAddDevice) {
			request = (PREQUEST_ADDDEVICE)RequestMemoryAlloc(sizeof(REQUEST_ADDDEVICE));
			if (request != NULL)
				RequestHeaderInit(&request->Header, DriverObject, PhysicalDeviceObject, ertAddDevice);
		}

		status = driverRecord->OldAddDevice(DriverObject, PhysicalDeviceObject);
		if (request != NULL) {
			RequestHeaderSetResult(request->Header, NTSTATUS, status);
			RequestQueueInsert(&request->Header);
		}

		detectedDevice = PhysicalDeviceObject;
		while (detectedDevice != NULL) {
			PREQUEST_HEADER rq = NULL;

			if (NT_SUCCESS(RequestXXXDetectedCreate(ertDriverDetected, detectedDevice->DriverObject, NULL, &rq)))
				RequestQueueInsert(rq);

			if (NT_SUCCESS(RequestXXXDetectedCreate(ertDeviceDetected, detectedDevice->DriverObject, detectedDevice, &rq)))
				RequestQueueInsert(rq);

			detectedDevice = detectedDevice->AttachedDevice;
		}

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DriverObject);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID HookHandlerDriverUnloadDisptach(PDRIVER_OBJECT DriverObject)
{
	PREQUEST_UNLOAD request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	driverRecord = DriverHookRecordGet(DriverObject);
	if (driverRecord != NULL) {
		if (driverRecord->MonitoringEnabled && driverRecord->MonitorDriverUnload) {
			request = (PREQUEST_UNLOAD)RequestMemoryAlloc(sizeof(REQUEST_UNLOAD));
			if (request != NULL)
				RequestHeaderInit(&request->Header, DriverObject, NULL, ertDriverUnload);
		}

		ObReferenceObject(DriverObject);
		driverRecord->OldDriverUnload(DriverObject);
		UnhookDriverObject(driverRecord);
		DriverHookRecordDereference(driverRecord);
		ObDereferenceObject(DriverObject);
		if (request != NULL)
			RequestQueueInsert(&request->Header);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DriverObject);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*              INITIALIZATION AND FINALIZATION                         */
/************************************************************************/

NTSTATUS HookHandlerModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	ObReferenceObject(DriverObject);
	_gDriverObject = DriverObject;
	FoTableInit(&_foTable, NULL);
	IoInitializeRemoveLock(&_rundownLock, 'LRHH', 0x7FFFFFFF, 0x7FFFFFFF);
	status = IoAcquireRemoveLock(&_rundownLock, DriverObject);
	if (!NT_SUCCESS(status)) {
		ObDereferenceObject(_gDriverObject);
		_gDriverObject = NULL;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void HookHandlerModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	IoReleaseRemoveLockAndWait(&_rundownLock, DriverObject);
	FoTableFinit(&_foTable);
	ObDereferenceObject(_gDriverObject);
	_gDriverObject = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
