
#include <ntifs.h>
#include <ntstrsafe.h>
#include <fltKernel.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hash_table.h"
#include "string-hash-table.h"
#include "ioctls.h"
#include "kernel-shared.h"
#include "utils.h"
#include "hook.h"
#include "req-queue.h"
#include "multistring.h"
#include "regman.h"
#include "boot-log.h"
#include "pnp-driver-watch.h"


/************************************************************************/
/*                     TYPE DEFINITIONS                                 */
/************************************************************************/

typedef struct _DRIVER_NAME_WATCH_ENUM_CONTEXT {
	NTSTATUS Status;
	PDRIVER_NAME_WATCH_ENTRY CurrentEntry;
	SIZE_T RemainingLength;
	SIZE_T BytesWritten;
	KPROCESSOR_MODE AccessMode;
} DRIVER_NAME_WATCH_ENUM_CONTEXT, *PDRIVER_NAME_WATCH_ENUM_CONTEXT;


/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

static PHASH_TABLE _driverNameTable = NULL;
static EX_PUSH_LOCK _driverNamesLock;

/************************************************************************/
/*                    HELPER FUNCTIONS                                  */
/************************************************************************/


static BOOLEAN _OnDriverNameWatchEnum(PWCHAR String, PVOID Data, PVOID Context)
{
	SIZE_T len = 0;
	SIZE_T requiredLength = 0;
	PDRIVER_NAME_WATCH_RECORD rec = (PDRIVER_NAME_WATCH_RECORD)Data;
	PDRIVER_NAME_WATCH_ENUM_CONTEXT ctx = (PDRIVER_NAME_WATCH_ENUM_CONTEXT)Context;
	DEBUG_ENTER_FUNCTION("String=\"%S\"; Data=0x%p; Context=0x%p", String, Data, Context);

	if (NT_SUCCESS(ctx->Status)) {
		len = wcslen(String) * sizeof(WCHAR);
		requiredLength = sizeof(DRIVER_NAME_WATCH_ENTRY) + len;
		if (ctx->RemainingLength >= requiredLength) {
			if (ctx->AccessMode == UserMode) {
				__try {
					ProbeForWrite(ctx->CurrentEntry, requiredLength, 1);
					ctx->CurrentEntry->MonitorSettings = rec->MonitorSettings;
					ctx->CurrentEntry->NameLength = (ULONG)len;
					memcpy(ctx->CurrentEntry + 1, String, len);
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					ctx->Status = GetExceptionCode();
				}
			} else {
				ctx->CurrentEntry->MonitorSettings = rec->MonitorSettings;
				ctx->CurrentEntry->NameLength = (ULONG)len;
				memcpy(ctx->CurrentEntry + 1, String, len);
			}

			if (NT_SUCCESS(ctx->Status)) {
				ctx->CurrentEntry = (PDRIVER_NAME_WATCH_ENTRY)((PUCHAR)ctx->CurrentEntry + requiredLength);
				ctx->BytesWritten += requiredLength;
				ctx->RemainingLength -= requiredLength;
			}
		} else ctx->Status = STATUS_BUFFER_TOO_SMALL;
	}

	DEBUG_EXIT_FUNCTION("0x%x", ctx->Status);
	return NT_SUCCESS(ctx->Status);
}


static NTSTATUS _HookDriverAndDevices(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, const DRIVER_MONITOR_SETTINGS* Settings)
{
	PDRIVER_HOOK_RECORD hookRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; DeviceObject=0x%p; Settings=0x%p", DriverObject, DeviceObject, Settings);

	if ((DriverObject->Flags & DRVO_UNLOAD_INVOKED) == 0 &&
		(DriverObject->Flags & DRVO_INITIALIZED) != 0) {
		status = HookDriverObject(DriverObject, Settings, FALSE, &hookRecord);
		if (NT_SUCCESS(status)) {
			status = DriverHookRecordEnable(hookRecord, TRUE);
			if (NT_SUCCESS(status)) {
				if (DeviceObject != NULL) {
					status = DriverHookRecordAddDevice(hookRecord, DeviceObject, NULL, NULL, TRUE, &deviceRecord);
					if (NT_SUCCESS(status)) {

						DeviceHookRecordDereference(deviceRecord);
					}
				} else if (Settings->MonitorNewDevices) {
					ULONG deviceCount = 0;
					PDEVICE_OBJECT *deviceArray = NULL;

					status = UtilsEnumDriverDevices(DriverObject, &deviceArray, &deviceCount);
					if (NT_SUCCESS(status)) {
						for (size_t i = 0; i < deviceCount; ++i) {
							status = DriverHookRecordAddDevice(hookRecord, deviceArray[i], NULL, NULL, TRUE, &deviceRecord);
							if (NT_SUCCESS(status)) {

								DeviceHookRecordDereference(deviceRecord);
							}
						}

						_ReleaseDeviceArray(deviceArray, deviceCount);
					}
				}
			}

			DriverHookRecordDereference(hookRecord);
		}
	} else status = STATUS_DEVICE_NOT_READY;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static void _OnDriverNameCheck(PWCHAR String, PVOID Data, PVOID Context)
{
	PDRIVER_OBJECT driverObject = NULL;
	UNICODE_STRING uDriverName;
	PDRIVER_NAME_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("String=\"%ls\"; Data=0x%p; Context=0x%p", String, Data, Context);

	RtlInitUnicodeString(&uDriverName, String);
	status = GetDriverObjectByName(&uDriverName, &driverObject);
	if (NT_SUCCESS(status)) {
		rec = (PDRIVER_NAME_WATCH_RECORD)Data;
		status = _HookDriverAndDevices(driverObject, NULL, &rec->MonitorSettings);
		if (NT_SUCCESS(status)) {
			PREQUEST_HEADER rq = NULL;

			status = RequestXXXDetectedCreate(ertDriverDetected, driverObject, NULL, &rq);
			if (NT_SUCCESS(status))
				RequestQueueInsert(rq);
		}

		ObDereferenceObject(driverObject);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                     PUBLIC FUNCTIONS                                 */
/************************************************************************/


NTSTATUS PDWCheckDriver(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject)
{
	UNICODE_STRING uName;
	PDRIVER_NAME_WATCH_RECORD nameRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; DeviceObject=0x%p", DriverObject, DeviceObject);

	status = _GetObjectName(DriverObject, &uName);
	if (NT_SUCCESS(status)) {
		FltAcquirePushLockShared(&_driverNamesLock);
		nameRecord = (PDRIVER_NAME_WATCH_RECORD)StringHashTableGetUnicodeString(_driverNameTable, &uName);
		if (nameRecord != NULL)
			status = _HookDriverAndDevices(DriverObject, DeviceObject, &nameRecord->MonitorSettings);

		FltReleasePushLock(&_driverNamesLock);
		HeapMemoryFree(uName.Buffer);
	}

	if (NT_SUCCESS(status)) {
		PREQUEST_HEADER rq = NULL;

		status = RequestXXXDetectedCreate(ertDriverDetected, DriverObject, NULL, &rq);
		if (NT_SUCCESS(status))
			RequestQueueInsert(rq);

		if (DeviceObject != NULL) {
			status = RequestXXXDetectedCreate(ertDeviceDetected, DriverObject, DeviceObject, &rq);
			if (NT_SUCCESS(status))
				RequestQueueInsert(rq);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS PWDDriverNameRegister(PUNICODE_STRING Name, const DRIVER_MONITOR_SETTINGS *Settings)
{
	PDRIVER_NAME_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Name=\"%wZ\"; Settings=0x%p", Name, Settings);

	FltAcquirePushLockExclusive(&_driverNamesLock);
	if (StringHashTableGetUnicodeString(_driverNameTable, Name) == NULL) {
		rec = HeapMemoryAllocNonPaged(sizeof(DRIVER_NAME_WATCH_RECORD));
		if (rec != NULL) {
			rec->MonitorSettings = *Settings;
			status = StringHashTableInsertUnicodeString(_driverNameTable, Name, rec);
			if (NT_SUCCESS(status)) {
				status = BLDriverNameSave(Name, Settings);
				if (!NT_SUCCESS(status))
					StringHashTableDeleteUnicodeString(_driverNameTable, Name);
			}

			if (!NT_SUCCESS(status))
				HeapMemoryFree(rec);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else status = STATUS_OBJECT_NAME_COLLISION;

	FltReleasePushLock(&_driverNamesLock);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS PWDDriverNameUnregister(PUNICODE_STRING Name)
{
	PDRIVER_NAME_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Name=\"%wZ\"", Name);

	FltAcquirePushLockExclusive(&_driverNamesLock);
	rec = (PDRIVER_NAME_WATCH_RECORD)StringHashTableDeleteUnicodeString(_driverNameTable, Name);
	if (rec != NULL) {
		BLDriverNameDelete(Name);
		HeapMemoryFree(rec);
		status = STATUS_SUCCESS;
	} else status = STATUS_NOT_FOUND;

	FltReleasePushLock(&_driverNamesLock);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS PWDDriverNameEnumerate(PIOCTL_IRPMNDRV_DRIVER_WATCH_ENUM_OUTPUT Buffer, SIZE_T Length, PSIZE_T ReturnLength, KPROCESSOR_MODE AccessMode)
{
	DRIVER_NAME_WATCH_ENUM_CONTEXT ctx;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Buffer=0x%p; Length=%Iu; Returnlength=0x%p; AccessMode=%u", Buffer, Length, ReturnLength, AccessMode);

	status = STATUS_SUCCESS;
	FltAcquirePushLockShared(&_driverNamesLock);
	if (AccessMode == UserMode) {
		__try {
			ProbeForWrite(Buffer, sizeof(Buffer->Count), 1);
			Buffer->Count = _driverNameTable->NumberOfItems;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
		}
	} else Buffer->Count = _driverNameTable->NumberOfItems;
	
	if (NT_SUCCESS(status)) {
		ctx.AccessMode = AccessMode;
		ctx.BytesWritten = FIELD_OFFSET(IOCTL_IRPMNDRV_DRIVER_WATCH_ENUM_OUTPUT, Entry);
		ctx.RemainingLength = Length - ctx.BytesWritten;
		ctx.CurrentEntry = &Buffer->Entry;
		ctx.Status = status;
		StringHashTablePerformWithFeedback(_driverNameTable, _OnDriverNameWatchEnum, &ctx);
		status = ctx.Status;
		if (NT_SUCCESS(status))
			*ReturnLength = ctx.BytesWritten;
	}

	FltReleasePushLock(&_driverNamesLock);

	DEBUG_EXIT_FUNCTION("0x%x, *ReturnLength=%Iu", status, *ReturnLength);
	return status;
}


void PDWCheckDrivers(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	FltAcquirePushLockShared(&_driverNamesLock);
	StringHashTablePerform(_driverNameTable, _OnDriverNameCheck, NULL);
	FltReleasePushLock(&_driverNamesLock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                INITIALIZATION AND FINALIZATION                       */
/************************************************************************/

NTSTATUS PWDModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	FltInitializePushLock(&_driverNamesLock);
	status = StringHashTableCreate(httNoSynchronization, 37, &_driverNameTable);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void PWDModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);
	
	StringHashTableDestroy(_driverNameTable);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
