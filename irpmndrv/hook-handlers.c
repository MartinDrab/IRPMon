
#include <ntifs.h>
#include <Ntddstor.h>
#include <fltkernel.h>
#include "preprocessor.h"
#include "allocator.h"
#include "kernel-shared.h"
#include "utils.h"
#include "hook.h"
#include "req-queue.h"
#include "hook-handlers.h"


#undef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE_ENABLED 0

/************************************************************************/
/*                        HELPER ROUTINES                               */
/************************************************************************/

static PREQUEST_FASTIO _CreateFastIoRequest(EFastIoOperationType FastIoType, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PVOID FileObject, PVOID Arg1, PVOID Arg2, PVOID Arg3, PVOID Arg4, PVOID Arg5, PVOID Arg6, PVOID Arg7)
{
	PREQUEST_FASTIO ret = NULL;

	ret = (PREQUEST_FASTIO)HeapMemoryAllocNonPaged(sizeof(REQUEST_FASTIO));
	if (ret != NULL) {
		KeQuerySystemTime(&ret->Header.Time);
		ret->Header.Device = DeviceObject;
		ret->Header.Driver = DriverObject;
		ret->Header.Type = ertFastIo;
		ret->Header.Result = NULL;
		ret->FastIoType = FastIoType;
		ret->FileObject = FileObject;
		ret->ThreadId = PsGetCurrentThreadId();
		ret->ProcessId = PsGetCurrentProcessId();
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
	}

	return ret;
}

static BOOLEAN _CatchRequest(PDRIVER_HOOK_RECORD DriverHookRecord, PDEVICE_HOOK_RECORD DeviceHookRecord)
{
	return ((DriverHookRecord->MonitoringEnabled) &&
		((DriverHookRecord->MonitorNewDevices && DeviceHookRecord == NULL) ||
		 (DeviceHookRecord != NULL && DeviceHookRecord->MonitoringEnabled)));

}

/************************************************************************/
/*                        FAST IO ROUTINES                              */
/************************************************************************/

BOOLEAN HookHandlerFastIoCheckIfPossible(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait, ULONG LockKey, BOOLEAN CheckForReadOperation, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoCheckIfPossible, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)CheckForReadOperation, (PVOID)Wait, (PVOID)LockKey, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoCheckIfPossible(FileObject, FileOffset, Length, Wait, LockKey, CheckForReadOperation, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

VOID HookHandlerFastIoDetachDevice(PDEVICE_OBJECT SourceDevice, PDEVICE_OBJECT TargetDevice)
{
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(SourceDevice->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, SourceDevice);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoDetachDevice, SourceDevice->DriverObject, SourceDevice, NULL, SourceDevice, TargetDevice, NULL, NULL, NULL, NULL, NULL);

		driverRecord->OldFastIoDisptach.FastIoDetachDevice(SourceDevice, TargetDevice);
		if (request != NULL)
			RequestQueueInsert(&request->Header);

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", SourceDevice->DriverObject);
		__debugbreak();
	}

	return;
}

BOOLEAN HookHandlerFastIoDeviceControl(PFILE_OBJECT FileObject, BOOLEAN Wait, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, ULONG ControlCode, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoDeviceControl, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)ControlCode, (PVOID)InputBufferLength, (PVOID)OutputBufferLength, (PVOID)Wait, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoDeviceControl(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, ControlCode, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoLock(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PLARGE_INTEGER Length, PEPROCESS ProcessId, ULONG Key, BOOLEAN FailImmediately, BOOLEAN Exclusive, PIO_STATUS_BLOCK StatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoLock, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length->LowPart, (PVOID)Length->HighPart, (PVOID)(((FailImmediately != 0) << 1) + (Exclusive != 0)), ProcessId, (PVOID)Key);

		ret = driverRecord->OldFastIoDisptach.FastIoLock(FileObject, FileOffset, Length, ProcessId, Key, FailImmediately, Exclusive, StatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoQueryBasicInfo(PFILE_OBJECT FileObject, BOOLEAN Wait, PFILE_BASIC_INFORMATION Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoQueryBasicInfo, DeviceObject->DriverObject, DeviceObject, FileObject, Buffer, (PVOID)Wait, NULL, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoQueryBasicInfo(FileObject, Wait, Buffer, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoQueryNetworkOpenInfo(PFILE_OBJECT FileObject, BOOLEAN Wait, PFILE_NETWORK_OPEN_INFORMATION Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoQueryNetworkOpenInfo, DeviceObject->DriverObject, DeviceObject, FileObject, Buffer, (PVOID)Wait, NULL, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoQueryNetworkOpenInfo(FileObject, Wait, Buffer, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoQueryOpenInfo(PIRP Irp, PFILE_NETWORK_OPEN_INFORMATION Buffer, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord)) {
			PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
			request = _CreateFastIoRequest(FastIoQueryOpen, DeviceObject->DriverObject, DeviceObject, irpStack->FileObject, Irp, Buffer, NULL, NULL, NULL, NULL, NULL);
		}

		ret = driverRecord->OldFastIoDisptach.FastIoQueryOpen(Irp, Buffer, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoQueryStandardInfo(PFILE_OBJECT FileObject, BOOLEAN Wait, PFILE_STANDARD_INFORMATION Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoQueryStandardInfo, DeviceObject->DriverObject, DeviceObject, FileObject, Buffer, (PVOID)Wait, NULL, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoQueryStandardInfo(FileObject, Wait, Buffer, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoRead(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait, ULONG LockKey, PVOID Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoRead, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, (PVOID)Wait, Buffer, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoRead(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoUnlockAll(PFILE_OBJECT FileObject, PEPROCESS ProcessId, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoUnlockAll, DeviceObject->DriverObject, DeviceObject, FileObject, ProcessId, NULL, NULL, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoUnlockAll(FileObject, ProcessId, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoUnlockByKey(PFILE_OBJECT FileObject, PVOID ProcessId, ULONG Key, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoUnlockAllByKey, DeviceObject->DriverObject, DeviceObject, FileObject, ProcessId, (PVOID)Key, NULL, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoUnlockAllByKey(FileObject, ProcessId, Key, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoUnlockSingle(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PLARGE_INTEGER Length, PEPROCESS ProcessId, ULONG Key, PIO_STATUS_BLOCK StatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoUnlockSingle, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length->LowPart, (PVOID)Length->HighPart, ProcessId, (PVOID)Key, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoUnlockSingle(FileObject, FileOffset, Length, ProcessId, Key, StatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoWrite(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, BOOLEAN Wait, ULONG LockKey, PVOID Buffer, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoWrite, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, (PVOID)Wait, Buffer, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoMdlRead(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, ULONG LockKey, PMDL *MdlChain, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(MdlRead, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, MdlChain, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.MdlRead(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoMdlWrite(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, ULONG LockKey, PMDL *MdlChain, PIO_STATUS_BLOCK IoStatusBlock, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(PrepareMdlWrite, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, MdlChain, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.PrepareMdlWrite(FileObject, FileOffset, Length, LockKey, MdlChain, IoStatusBlock, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoMdlReadComplete(PFILE_OBJECT FileObject, PMDL MdlChain, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(MdlReadComplete, DeviceObject->DriverObject, DeviceObject, FileObject, MdlChain, NULL, NULL, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.MdlReadComplete(FileObject, MdlChain, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoMdlWriteComplete(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PMDL MdlChain, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(MdlWriteComplete, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, MdlChain, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.MdlWriteComplete(FileObject, FileOffset, MdlChain, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoReadCompressed(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, ULONG LockKey, PVOID Buffer, PMDL *MdlChain, PIO_STATUS_BLOCK IoStatusBlock, PCOMPRESSED_DATA_INFO CompressedInfo, ULONG CompressedInfoLength, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoReadCompressed, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, (PVOID)Buffer, (PVOID)CompressedInfoLength, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoReadCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatusBlock, CompressedInfo, CompressedInfoLength, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoWriteCompressed(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, ULONG Length, ULONG LockKey, PVOID Buffer, PMDL *MdlChain, PIO_STATUS_BLOCK IoStatusBlock, PCOMPRESSED_DATA_INFO CompressedInfo, ULONG CompressedInfoLength, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(FastIoWriteCompressed, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, (PVOID)Length, (PVOID)LockKey, Buffer, (PVOID)CompressedInfoLength, NULL);

		ret = driverRecord->OldFastIoDisptach.FastIoWriteCompressed(FileObject, FileOffset, Length, LockKey, Buffer, MdlChain, IoStatusBlock, CompressedInfo, CompressedInfoLength, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
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
		__debugbreak();
	}

	return ret;
}

NTSTATUS HookHandlerFastIoAcquireForModWrite(PFILE_OBJECT FileObject, PLARGE_INTEGER EndingOffset, PERESOURCE *ResourceToRelease, PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(AcquireForModWrite, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)EndingOffset->LowPart, (PVOID)EndingOffset->HighPart, NULL, NULL, NULL, NULL, NULL);

		status = driverRecord->OldFastIoDisptach.AcquireForModWrite(FileObject, EndingOffset, ResourceToRelease, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)status;
			if (NT_SUCCESS(status) && ResourceToRelease != NULL)
				request->Arg3 = *ResourceToRelease;

			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		__debugbreak();
	}

	return status;
}

NTSTATUS HookHandlerFastIoReleaseForModWrite(PFILE_OBJECT FileObject, PERESOURCE ResourceToRelease, PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(ReleaseForModWrite, DeviceObject->DriverObject, DeviceObject, FileObject, ResourceToRelease, NULL, NULL, NULL, NULL, NULL, NULL);

		status = driverRecord->OldFastIoDisptach.ReleaseForModWrite(FileObject, ResourceToRelease, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)status;
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		__debugbreak();
	}

	return status;
}

NTSTATUS HookHandlerFastIoAcquireForCcFlush(PFILE_OBJECT FileObject, PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(AcquireForCcFlush, DeviceObject->DriverObject, DeviceObject, FileObject, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		status = driverRecord->OldFastIoDisptach.AcquireForCcFlush(FileObject, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)status;
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		__debugbreak();
	}

	return status;
}

NTSTATUS HookHandlerFastIoReleaseForCcFlush(PFILE_OBJECT FileObject, PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(ReleaseForCcFlush, DeviceObject->DriverObject, DeviceObject, FileObject, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		status = driverRecord->OldFastIoDisptach.ReleaseForCcFlush(FileObject, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)status;
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		__debugbreak();
	}

	return status;
}

VOID HookHandlerFastIoAcquireFile(PFILE_OBJECT FileObject)
{
	PDEVICE_OBJECT deviceObject = NULL;
	PDRIVER_OBJECT driverObject = NULL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	if (FileObject->Vpb != NULL)
		deviceObject = FileObject->Vpb->DeviceObject;
	else deviceObject = FileObject->DeviceObject;

	driverObject = deviceObject->DriverObject;
	driverRecord = DriverHookRecordGet(driverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, deviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(AcquireFileForNtCreateSection, driverObject, deviceObject, FileObject, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		driverRecord->OldFastIoDisptach.AcquireFileForNtCreateSection(FileObject);
		if (request != NULL)
			RequestQueueInsert(&request->Header);

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", FileObject->DeviceObject->DriverObject);
		__debugbreak();
	}

	return;
}

VOID HookHandlerFastIoReleaseFile(PFILE_OBJECT FileObject)
{
	PDEVICE_OBJECT deviceObject = NULL;
	PDRIVER_OBJECT driverObject = NULL;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	
	if (FileObject->Vpb != NULL) {
		deviceObject = FileObject->Vpb->DeviceObject;
	} else deviceObject = FileObject->DeviceObject;

	driverObject = deviceObject->DriverObject;
	driverRecord = DriverHookRecordGet(driverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, deviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(ReleaseFileForNtCreateSection, driverObject, deviceObject, FileObject, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		driverRecord->OldFastIoDisptach.ReleaseFileForNtCreateSection(FileObject);
		if (request != NULL)
			RequestQueueInsert(&request->Header);

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", FileObject->DeviceObject->DriverObject);
		__debugbreak();
	}

	return;
}

BOOLEAN HookHandlerFastIoMdlReadCompleteCompressed(PFILE_OBJECT FileObject, PMDL MdlChain, PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(MdlReadCompleteCompressed, DeviceObject->DriverObject, DeviceObject, FileObject, MdlChain, NULL, NULL, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.MdlReadCompleteCompressed(FileObject, MdlChain, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		__debugbreak();
	}

	return ret;
}

BOOLEAN HookHandlerFastIoMdlWriteCompleteCompressed(PFILE_OBJECT FileObject, PLARGE_INTEGER FileOffset, PMDL MdlChain,  PDEVICE_OBJECT DeviceObject)
{
	BOOLEAN ret = FALSE;
	PREQUEST_FASTIO request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;

	driverRecord = DriverHookRecordGet(DeviceObject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, DeviceObject);
		if (_CatchRequest(driverRecord, deviceRecord))
			request = _CreateFastIoRequest(MdlWriteCompleteCompressed, DeviceObject->DriverObject, DeviceObject, FileObject, (PVOID)FileOffset->LowPart, (PVOID)FileOffset->HighPart, MdlChain, NULL, NULL, NULL, NULL);

		ret = driverRecord->OldFastIoDisptach.MdlWriteCompleteCompressed(FileObject, FileOffset, MdlChain, DeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)ret;
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DeviceObject->DriverObject);
		__debugbreak();
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
		if (driverRecord->MonitorStartIo && _CatchRequest(driverRecord, deviceRecord)) {
			request = (PREQUEST_STARTIO)HeapMemoryAllocNonPaged(sizeof(REQUEST_STARTIO));
			if (request != NULL) {
				request->Header.Type = ertStartIo;
				KeQuerySystemTime(&request->Header.Time);
				request->Header.Driver = DeviceObject->DriverObject;
				request->Header.Device = DeviceObject;
				request->IRPAddress = Irp;
				request->MajorFunction = IrpStack->MajorFunction;
				request->MinorFunction = IrpStack->MinorFunction;
				request->IrpFlags = Irp->Flags;
				request->FileObject = IrpStack->FileObject;
				request->Status = STATUS_UNSUCCESSFUL;
				request->Information = 0;
			}
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
		__debugbreak();
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

typedef struct _IRP_COMPLETION_CONTEXT {
	PVOID OriginalContext;
	PIO_COMPLETION_ROUTINE OriginalRoutine;
	ULONG OriginalControl;
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;
} IRP_COMPLETION_CONTEXT, *PIRP_COMPLETION_CONTEXT;

static NTSTATUS _HookHandlerIRPCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	NTSTATUS status = STATUS_CONTINUE_COMPLETION;
	PREQUEST_IRP_COMPLETION completionRequest = NULL;
	PIRP_COMPLETION_CONTEXT cc = (PIRP_COMPLETION_CONTEXT)Context;
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p; Context=0x%p", DeviceObject, Irp, Context);

	completionRequest = (PREQUEST_IRP_COMPLETION)HeapMemoryAllocNonPaged(sizeof(REQUEST_IRP_COMPLETION));
	if (completionRequest != NULL) {
		completionRequest->Header.Type = ertIRPCompletion;
		InitializeListHead(&completionRequest->Header.Entry);
		KeQuerySystemTime(&completionRequest->Header.Time);
		completionRequest->Header.Driver = cc->DriverObject;
		completionRequest->Header.Device = cc->DeviceObject;
		completionRequest->Header.Result = (PVOID)Irp->IoStatus.Status;
		completionRequest->IRPAddress = Irp;
		completionRequest->CompletionInformation = Irp->IoStatus.Information;
		completionRequest->CompletionStatus = Irp->IoStatus.Status;
		completionRequest->ProcessId = PsGetCurrentProcessId();
		completionRequest->ThreadId = PsGetCurrentThread();
		RequestQueueInsert(&completionRequest->Header);
	}

	if (cc->OriginalRoutine == NULL) {
		if (Irp->PendingReturned)
			IoMarkIrpPending(Irp);
	} else {
		NTSTATUS irpStatus = Irp->IoStatus.Status;

		if ((irpStatus == STATUS_CANCELLED && (cc->OriginalControl & SL_INVOKE_ON_CANCEL)) ||
			(irpStatus != STATUS_CANCELLED && !NT_SUCCESS(irpStatus) && (cc->OriginalControl & SL_INVOKE_ON_ERROR)) ||
			(NT_SUCCESS(irpStatus) && (cc->OriginalControl & SL_INVOKE_ON_SUCCESS)))
			status = cc->OriginalRoutine(DeviceObject, Irp, cc->OriginalContext);
		else if (Irp->PendingReturned)
			IoMarkIrpPending(Irp);
	}

	ExFreePool(cc);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static VOID _HookIRPCompletionRoutine(PIRP Irp, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject)
{
	PIO_STACK_LOCATION irpStack = NULL;
	PIRP_COMPLETION_CONTEXT cc = NULL;
	DEBUG_ENTER_FUNCTION("Irp=0x%p; DriverObject=0x%p; DeviceObject=0x%p", Irp, DriverObject, DeviceObject);

	cc = (PIRP_COMPLETION_CONTEXT)ExAllocatePool(NonPagedPool, sizeof(IRP_COMPLETION_CONTEXT));
	if (cc != NULL) {
		irpStack = IoGetCurrentIrpStackLocation(Irp);
		if (irpStack->CompletionRoutine != NULL) {
			cc->OriginalContext = irpStack->Context;
			cc->OriginalRoutine = irpStack->CompletionRoutine;
			cc->OriginalControl = irpStack->Control;
		} else {
			cc->OriginalContext = NULL;
			cc->OriginalRoutine = NULL;
			cc->OriginalControl = 0;
			irpStack->Control = 0;
		}

		cc->DriverObject = DriverObject;
		cc->DeviceObject = DeviceObject;
		irpStack->Control |= (SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL);
		irpStack->CompletionRoutine = _HookHandlerIRPCompletion;
		irpStack->Context = cc;
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

NTSTATUS HookHandlerIRPDisptach(PDEVICE_OBJECT Deviceobject, PIRP Irp)
{
	PREQUEST_IRP request = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", Deviceobject, Irp);
	
	driverRecord = DriverHookRecordGet(Deviceobject->DriverObject);
	if (driverRecord != NULL) {
		deviceRecord = DriverHookRecordGetDevice(driverRecord, Deviceobject);
		if (_CatchRequest(driverRecord, deviceRecord)) {
			request = (PREQUEST_IRP)HeapMemoryAllocNonPaged(sizeof(REQUEST_IRP));
			if (request != NULL) {
				request->Header.Type = ertIRP;
				KeQuerySystemTime(&request->Header.Time);
				request->Header.Driver = Deviceobject->DriverObject;
				request->Header.Device = Deviceobject;
				request->Header.Result = (PVOID)STATUS_PENDING;
				request->IRPAddress = Irp;
				request->MajorFunction = IrpStack->MajorFunction;
				request->MinorFunction = IrpStack->MinorFunction;
				request->PreviousMode = ExGetPreviousMode();
				request->RequestorMode = Irp->RequestorMode;
				request->ThreadId = PsGetCurrentThreadId();
				request->ProcessId = PsGetCurrentProcessId();
				request->Arg1 = IrpStack->Parameters.Others.Argument1;
				request->Arg2 = IrpStack->Parameters.Others.Argument2;
				request->Arg3 = IrpStack->Parameters.Others.Argument3;
				request->Arg4 = IrpStack->Parameters.Others.Argument4;
				request->IrpFlags = Irp->Flags;
				request->FileObject = IrpStack->FileObject;
			}

			if (driverRecord->MonitorIRPCompletion)
				_HookIRPCompletionRoutine(Irp, Deviceobject->DriverObject, Deviceobject);
		}
		
		status = driverRecord->OldMajorFunction[IrpStack->MajorFunction](Deviceobject, Irp);
		if (request != NULL) {
			request->Header.Result = (PVOID)status;
			RequestQueueInsert(&request->Header);
		}

		if (deviceRecord != NULL)
			DeviceHookRecordDereference(deviceRecord);

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", Deviceobject->DriverObject);
		__debugbreak();
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

NTSTATUS HookHandlerAddDeviceDispatch(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject)
{
	PREQUEST_ADDDEVICE request = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; PhysicalDeviceObject=0x%p", DriverObject, PhysicalDeviceObject);

	driverRecord = DriverHookRecordGet(DriverObject);
	if (driverRecord != NULL) {
		if (driverRecord->MonitoringEnabled && driverRecord->MonitorAddDevice) {
			request = (PREQUEST_ADDDEVICE)HeapMemoryAllocNonPaged(sizeof(REQUEST_ADDDEVICE));
			if (request != NULL) {
				KeQuerySystemTime(&request->Header.Time);
				request->Header.Type = ertAddDevice;
				request->Header.Driver = DriverObject;
				request->Header.Device = PhysicalDeviceObject;
			}
		}

		status = driverRecord->OldAddDevice(DriverObject, PhysicalDeviceObject);
		if (request != NULL) {
			request->Header.Result = (PVOID)status;
			RequestQueueInsert(&request->Header);
		}

		DriverHookRecordDereference(driverRecord);
	} else {
		DEBUG_ERROR("Hook is installed for non-hooked driver object 0x%p", DriverObject);
		__debugbreak();
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
			request = (PREQUEST_UNLOAD)HeapMemoryAllocNonPaged(sizeof(REQUEST_UNLOAD));
			if (request != NULL) {
				KeQuerySystemTime(&request->Header.Time);
				request->Header.Type = ertDriverUnload;
				request->Header.Driver = DriverObject;
				request->Header.Device = NULL;
				request->Header.Result = NULL;
			}
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
		__debugbreak();
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
