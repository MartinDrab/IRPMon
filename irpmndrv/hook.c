
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "general-types.h"
#include "utils.h"
#include "hash_table.h"
#include "hook-handlers.h"
#include "hook.h"


#pragma warning( disable : 4152 55 )

/************************************************************************/
/*                       GLOBAL VARIABLES                               */
/************************************************************************/

static PHASH_TABLE _driverTable = NULL;
static KSPIN_LOCK _driverTableLock;

static PHASH_TABLE _driverValidationTable = NULL;
static KSPIN_LOCK _driverValidationTableLock;
static PHASH_TABLE _deviceValidationTable = NULL;
static KSPIN_LOCK _deviceValidationTableLock;
static IO_REMOVE_LOCK _rundownLock;
static BOOLEAN _shutdownInProgress = FALSE;

/************************************************************************/
/*                       FORWARD DECLARATIONS                           */
/************************************************************************/

static VOID _HookDriverObject(PDRIVER_OBJECT DriverObject, PDRIVER_HOOK_RECORD HookRecord);
static VOID _UnhookDriverObject(PDRIVER_HOOK_RECORD HookRecord);

/************************************************************************/
/*                        HELPER FUNCTIONS                              */
/************************************************************************/

static ULONG32 _HashFunction(PVOID Key)
{
	return (ULONG32)((ULONG_PTR)Key / 4);
}

static BOOLEAN _DriverCompareFunction(PHASH_ITEM HashItem, PVOID Key)
{
	PDRIVER_HOOK_RECORD r = CONTAINING_RECORD(HashItem, DRIVER_HOOK_RECORD, HashItem);

	return (Key == r->DriverObject);
}

static BOOLEAN _DeviceCompareFunction(PHASH_ITEM HashItem, PVOID Key)
{
	PDEVICE_HOOK_RECORD r = CONTAINING_RECORD(HashItem, DEVICE_HOOK_RECORD, HashItem);

	return (Key == r->DeviceObject);
}

static BOOLEAN _DriverValidationCompareFunction(PHASH_ITEM HashItem, PVOID Key)
{
	PDRIVER_HOOK_RECORD r = CONTAINING_RECORD(HashItem, DRIVER_HOOK_RECORD, ValidationHashItem);

	return (Key == r);
}

static BOOLEAN _DeviceValidationCompareFunction(PHASH_ITEM HashItem, PVOID Key)
{
	PDEVICE_HOOK_RECORD r = CONTAINING_RECORD(HashItem, DEVICE_HOOK_RECORD, ValidationHashItem);

	return (Key == r);
}

static VOID _DriverFreeFunction(PHASH_ITEM HashItem)
{
	PDRIVER_HOOK_RECORD r = CONTAINING_RECORD(HashItem, DRIVER_HOOK_RECORD, HashItem);
	DEBUG_ENTER_FUNCTION("HashItem=0x%p", HashItem);

	if (r->MonitoringEnabled) {
		_UnhookDriverObject(r);
		r->MonitoringEnabled = FALSE;
	}

	HashTableClear(r->SelectedDevices, TRUE);
	DriverHookRecordDereference(r);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static VOID _DeviceFreeFunction(PHASH_ITEM HashItem)
{
	PDEVICE_HOOK_RECORD r = CONTAINING_RECORD(HashItem, DEVICE_HOOK_RECORD, HashItem);
	DEBUG_ENTER_FUNCTION("HashItem=0x%p", HashItem);

	DeviceHookRecordDereference(r);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                            VALIDATION                                */
/************************************************************************/

static VOID _MakeDriverHookRecordValid(PDRIVER_HOOK_RECORD DriverRecord)
{
	KIRQL irql;
	DEBUG_ENTER_FUNCTION("DriverRecord=0x%p", DriverRecord);

	KeAcquireSpinLock(&_driverValidationTableLock, &irql);
	ASSERT(HashTableGet(_driverValidationTable, DriverRecord) == NULL);
	HashTableInsert(_driverValidationTable, &DriverRecord->ValidationHashItem, DriverRecord);
	KeReleaseSpinLock(&_driverValidationTableLock, irql);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static VOID _InvalidateDriverHookRecord(PDRIVER_HOOK_RECORD DriverRecord)
{
	KIRQL irql;
	DEBUG_ENTER_FUNCTION("DriverRecord=0x%p", DriverRecord);

	KeAcquireSpinLock(&_driverValidationTableLock, &irql);
	ASSERT(HashTableGet(_driverValidationTable, DriverRecord) != NULL);
	HashTableDelete(_driverValidationTable, DriverRecord);
	KeReleaseSpinLock(&_driverValidationTableLock, irql);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static VOID _MakeDeviceHookRecordValid(PDEVICE_HOOK_RECORD DeviceRecord)
{
	KIRQL irql;
	DEBUG_ENTER_FUNCTION("DeviceRecord=0x%p", DeviceRecord);

	KeAcquireSpinLock(&_deviceValidationTableLock, &irql);
	HashTableInsert(_deviceValidationTable, &DeviceRecord->ValidationHashItem, DeviceRecord);
	KeReleaseSpinLock(&_deviceValidationTableLock, irql);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static VOID _InvalidateDeviceHookRecord(PDEVICE_HOOK_RECORD DeviceRecord)
{
	KIRQL irql;
	DEBUG_ENTER_FUNCTION("DeviceRecord=0x%p", DeviceRecord);

	KeAcquireSpinLock(&_deviceValidationTableLock, &irql);
	HashTableDelete(_deviceValidationTable, DeviceRecord);
	KeReleaseSpinLock(&_deviceValidationTableLock, irql);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                       HOOKING                                        */
/************************************************************************/

static VOID _HookFastIoRoutine(PVOID *DriverRoutine, PVOID *RecordRoutine, PVOID HookRoutine)
{
	if (*DriverRoutine != NULL) 
		*RecordRoutine = InterlockedExchangePointer(DriverRoutine, HookRoutine);

	return;
}

static VOID _UnhookFastIoRoutine(PVOID *DriverRoutine, PVOID RecordRoutine)
{
	if (*DriverRoutine != NULL) 
		*DriverRoutine = RecordRoutine;

	return;
}

static VOID _HookDriverObject(PDRIVER_OBJECT DriverObject, PDRIVER_HOOK_RECORD HookRecord)
{
	ULONG i = 0;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; HookRecord=0x%p", DriverObject, HookRecord);

	HookRecord->DriverObject = DriverObject;
	if (HookRecord->MonitorIRP) {
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; ++i)
			HookRecord->OldMajorFunction[i] = (PDRIVER_DISPATCH)InterlockedExchangePointer((PVOID *)&DriverObject->MajorFunction[i], HookHandlerIRPDisptach);
	}

	HookRecord->AddDevicePresent = DriverObject->DriverExtension->AddDevice != NULL;
	if (HookRecord->AddDevicePresent) {
		if (HookRecord->MonitorAddDevice)
			HookRecord->OldAddDevice = (PDRIVER_ADD_DEVICE)InterlockedExchangePointer((PVOID *)&DriverObject->DriverExtension->AddDevice, HookHandlerAddDeviceDispatch);
	}

	HookRecord->DriverUnloadPresent = DriverObject->DriverUnload != NULL;
	if (HookRecord->DriverUnloadPresent) {
		if (HookRecord->MonitorDriverUnload)
			HookRecord->OldDriverUnload = (PDRIVER_UNLOAD)InterlockedExchangePointer((PVOID *)&DriverObject->DriverUnload, HookHandlerDriverUnloadDisptach);
	}

	HookRecord->StartIoPresent = DriverObject->DriverStartIo != NULL;
	if (HookRecord->StartIoPresent) {
		if (HookRecord->MonitorStartIo)
			HookRecord->OldStartIo = (PDRIVER_STARTIO)InterlockedExchangePointer((PVOID *)&DriverObject->DriverStartIo, HookHandlerStartIoDispatch);
	}

	HookRecord->FastIoPresent = DriverObject->FastIoDispatch != NULL;
	if (HookRecord->FastIoPresent) {
		if (HookRecord->MonitorFastIo) {
			PFAST_IO_DISPATCH fastIo = DriverObject->FastIoDispatch;
			PFAST_IO_DISPATCH hookFastIo = &HookRecord->OldFastIoDisptach;

			HookRecord->OldFastIoDisptach.SizeOfFastIoDispatch = fastIo->SizeOfFastIoDispatch;
			_HookFastIoRoutine((PVOID *)&fastIo->AcquireForCcFlush, (PVOID *)&hookFastIo->AcquireForCcFlush, HookHandlerFastIoAcquireForCcFlush);
			_HookFastIoRoutine((PVOID *)&fastIo->AcquireForModWrite, (PVOID *)&hookFastIo->AcquireForModWrite, HookHandlerFastIoAcquireForModWrite);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoCheckIfPossible, (PVOID *)&hookFastIo->FastIoCheckIfPossible, HookHandlerFastIoCheckIfPossible);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoDetachDevice, (PVOID *)&hookFastIo->FastIoDetachDevice, HookHandlerFastIoDetachDevice);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoDeviceControl, (PVOID *)&hookFastIo->FastIoDeviceControl, HookHandlerFastIoDeviceControl);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoLock, (PVOID *)&hookFastIo->FastIoLock, HookHandlerFastIoLock);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoQueryBasicInfo, (PVOID *)&hookFastIo->FastIoQueryBasicInfo, HookHandlerFastIoQueryBasicInfo);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoQueryNetworkOpenInfo, (PVOID *)&hookFastIo->FastIoQueryNetworkOpenInfo, HookHandlerFastIoQueryNetworkOpenInfo);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoQueryOpen, (PVOID *)&hookFastIo->FastIoQueryOpen, HookHandlerFastIoQueryOpenInfo);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoQueryStandardInfo, (PVOID *)&hookFastIo->FastIoQueryStandardInfo, HookHandlerFastIoQueryStandardInfo);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoRead, (PVOID *)&hookFastIo->FastIoRead, HookHandlerFastIoRead);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoReadCompressed, (PVOID *)&hookFastIo->FastIoReadCompressed, HookHandlerFastIoReadCompressed);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoUnlockAll, (PVOID *)&hookFastIo->FastIoUnlockAll, HookHandlerFastIoUnlockAll);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoUnlockAllByKey, (PVOID *)&hookFastIo->FastIoUnlockAllByKey, HookHandlerFastIoUnlockByKey);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoUnlockSingle, (PVOID *)&hookFastIo->FastIoUnlockSingle, HookHandlerFastIoUnlockSingle);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoWrite, (PVOID *)&hookFastIo->FastIoWrite, HookHandlerFastIoWrite);
			_HookFastIoRoutine((PVOID *)&fastIo->FastIoWriteCompressed, (PVOID *)&hookFastIo->FastIoWriteCompressed, HookHandlerFastIoWriteCompressed);
			_HookFastIoRoutine((PVOID *)&fastIo->MdlRead, (PVOID *)&hookFastIo->MdlRead, HookHandlerFastIoMdlRead);
			_HookFastIoRoutine((PVOID *)&fastIo->MdlReadComplete, (PVOID *)&hookFastIo->MdlReadComplete, HookHandlerFastIoMdlReadComplete);
			_HookFastIoRoutine((PVOID *)&fastIo->MdlReadCompleteCompressed, (PVOID *)&hookFastIo->MdlReadCompleteCompressed, HookHandlerFastIoMdlReadCompleteCompressed);
			_HookFastIoRoutine((PVOID *)&fastIo->MdlWriteComplete, (PVOID *)&hookFastIo->MdlWriteComplete, HookHandlerFastIoMdlWriteComplete);
			_HookFastIoRoutine((PVOID *)&fastIo->MdlWriteCompleteCompressed, (PVOID *)&hookFastIo->MdlWriteCompleteCompressed, HookHandlerFastIoMdlWriteCompleteCompressed);
			_HookFastIoRoutine((PVOID *)&fastIo->PrepareMdlWrite, (PVOID *)&hookFastIo->PrepareMdlWrite, HookHandlerFastIoMdlWrite);
			_HookFastIoRoutine((PVOID *)&fastIo->ReleaseForCcFlush, (PVOID *)&hookFastIo->ReleaseForCcFlush, HookHandlerFastIoReleaseForCcFlush);
			_HookFastIoRoutine((PVOID *)&fastIo->ReleaseForModWrite, (PVOID *)&hookFastIo->ReleaseForModWrite, HookHandlerFastIoReleaseForModWrite);
		}
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static VOID _UnhookDriverObject(PDRIVER_HOOK_RECORD HookRecord)
{
	ULONG i = 0;
	PDRIVER_OBJECT driverObject = NULL;
	DEBUG_ENTER_FUNCTION("HookRecord=0x%p", HookRecord);

	driverObject = HookRecord->DriverObject;
	if (HookRecord->MonitorIRP) {
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; ++i)
			driverObject->MajorFunction[i] = HookRecord->OldMajorFunction[i];
	}

	if (HookRecord->AddDevicePresent) {
		if (HookRecord->MonitorAddDevice)
			driverObject->DriverExtension->AddDevice = HookRecord->OldAddDevice;
	}

	if (HookRecord->DriverUnloadPresent) {
		if (HookRecord->MonitorDriverUnload)
			driverObject->DriverUnload = HookRecord->OldDriverUnload;
	}

	if (HookRecord->StartIoPresent) {
		if (HookRecord->MonitorStartIo)
			driverObject->DriverStartIo = HookRecord->OldStartIo;
	}

	if (HookRecord->FastIoPresent) {
		if (HookRecord->MonitorFastIo) {
			PFAST_IO_DISPATCH fastIo = driverObject->FastIoDispatch;
			PFAST_IO_DISPATCH hookFastIo = &HookRecord->OldFastIoDisptach;

			_UnhookFastIoRoutine((PVOID *)&fastIo->AcquireForCcFlush, hookFastIo->AcquireForCcFlush);
			_UnhookFastIoRoutine((PVOID *)&fastIo->AcquireForModWrite, hookFastIo->AcquireForModWrite);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoCheckIfPossible, hookFastIo->FastIoCheckIfPossible);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoDetachDevice, hookFastIo->FastIoDetachDevice);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoDeviceControl, hookFastIo->FastIoDeviceControl);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoLock, hookFastIo->FastIoLock);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoQueryBasicInfo, hookFastIo->FastIoQueryBasicInfo);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoQueryNetworkOpenInfo, hookFastIo->FastIoQueryNetworkOpenInfo);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoQueryOpen, hookFastIo->FastIoQueryOpen);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoQueryStandardInfo, hookFastIo->FastIoQueryStandardInfo);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoRead, hookFastIo->FastIoRead);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoReadCompressed, hookFastIo->FastIoReadCompressed);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoUnlockAll, hookFastIo->FastIoUnlockAll);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoUnlockAllByKey, hookFastIo->FastIoUnlockAllByKey);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoUnlockSingle, hookFastIo->FastIoUnlockSingle);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoWrite, hookFastIo->FastIoWrite);
			_UnhookFastIoRoutine((PVOID *)&fastIo->FastIoWriteCompressed, hookFastIo->FastIoWriteCompressed);
			_UnhookFastIoRoutine((PVOID *)&fastIo->MdlRead, hookFastIo->MdlRead);
			_UnhookFastIoRoutine((PVOID *)&fastIo->MdlReadComplete, hookFastIo->MdlReadComplete);
			_UnhookFastIoRoutine((PVOID *)&fastIo->MdlReadCompleteCompressed, hookFastIo->MdlReadCompleteCompressed);
			_UnhookFastIoRoutine((PVOID *)&fastIo->MdlWriteComplete, hookFastIo->MdlWriteComplete);
			_UnhookFastIoRoutine((PVOID *)&fastIo->MdlWriteCompleteCompressed, hookFastIo->MdlWriteCompleteCompressed);
			_UnhookFastIoRoutine((PVOID *)&fastIo->PrepareMdlWrite, hookFastIo->PrepareMdlWrite);
			_UnhookFastIoRoutine((PVOID *)&fastIo->ReleaseForCcFlush, hookFastIo->ReleaseForCcFlush);
			_UnhookFastIoRoutine((PVOID *)&fastIo->ReleaseForModWrite, hookFastIo->ReleaseForModWrite);
		}
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                      DRIVER HOOK RECORDS                             */
/************************************************************************/

static NTSTATUS _DriverHookRecordCreate(PDRIVER_OBJECT DriverObject, PDRIVER_MONITOR_SETTINGS MonitorSettings, BOOLEAN MonitoringEnabled, PDRIVER_HOOK_RECORD *Record)
{
	PDRIVER_HOOK_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; MonitorSettings=0x%p; MonitoringEnabled=%u; Record=0x%p", DriverObject, MonitorSettings, MonitoringEnabled, Record);

	tmpRecord = (PDRIVER_HOOK_RECORD)HeapMemoryAllocNonPaged(sizeof(DRIVER_HOOK_RECORD));
	if (tmpRecord != NULL) {
		status = IoAcquireRemoveLock(&_rundownLock, tmpRecord);
		if (NT_SUCCESS(status)) {
			memset(tmpRecord, 0, sizeof(DRIVER_HOOK_RECORD));
			tmpRecord->ReferenceCount = 1;
			status = _GetObjectName(DriverObject, &tmpRecord->DriverName);
			if (NT_SUCCESS(status)) {
				tmpRecord->DriverObject = DriverObject;
				tmpRecord->MonitoringEnabled = MonitoringEnabled;
				tmpRecord->MonitorNewDevices = MonitorSettings->MonitorNewDevices;
				tmpRecord->MonitorStartIo = MonitorSettings->MonitorStartIo;
				tmpRecord->MonitorAddDevice = MonitorSettings->MonitorAddDevice;
				tmpRecord->MonitorDriverUnload = MonitorSettings->MonitorUnload;
				tmpRecord->MonitorIRP = MonitorSettings->MonitorIRP;
				tmpRecord->MonitorIRPCompletion = MonitorSettings->MonitorIRPCompletion;
				tmpRecord->MonitorData = MonitorSettings->MonitorData;
				tmpRecord->MonitorFastIo = MonitorSettings->MonitorFastIo;
				memcpy(tmpRecord->IRPSettings, MonitorSettings->IRPSettings, sizeof(tmpRecord->IRPSettings));
				memcpy(tmpRecord->FastIoSettings, MonitorSettings->FastIoSettings, sizeof(tmpRecord->FastIoSettings));
				KeInitializeSpinLock(&tmpRecord->SelectedDevicesLock);
				status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _DeviceCompareFunction, _DeviceFreeFunction, &tmpRecord->SelectedDevices);
				if (NT_SUCCESS(status))
					*Record = tmpRecord;

				if (!NT_SUCCESS(status))
					HeapMemoryFree(tmpRecord->DriverName.Buffer);
			}
		
			if (!NT_SUCCESS(status))
				IoReleaseRemoveLock(&_rundownLock, tmpRecord);
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(tmpRecord);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}

static VOID _DriverHookRecordFree(PDRIVER_HOOK_RECORD Record)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

	HashTableDestroy(Record->SelectedDevices);
	HeapMemoryFree(Record->DriverName.Buffer);
	IoReleaseRemoveLock(&_rundownLock, Record);
	HeapMemoryFree(Record);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static NTSTATUS _DeviceHookRecordCreate(PDRIVER_HOOK_RECORD DriverRecord, PDEVICE_HOOK_RECORD *Record, PUCHAR IRPSettings, PUCHAR FastIoSettings, BOOLEAN MonitoringEnabled, EDeviceRecordCreateReason CreateReason, PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_HOOK_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverRecord=0x%p; Record=0x%p; IRPSettings=0x%p; FastIoSettings=0x%p; MonitoringEnabled=%u; CreateReason=%u", DriverRecord, Record, IRPSettings, FastIoSettings, MonitoringEnabled, CreateReason, DeviceObject);

	tmpRecord = (PDEVICE_HOOK_RECORD)HeapMemoryAllocNonPaged(sizeof(DEVICE_HOOK_RECORD));
	if (tmpRecord != NULL) {
		status = IoAcquireRemoveLock(&_rundownLock, tmpRecord);
		if (NT_SUCCESS(status)) {
			memset(tmpRecord, 0, sizeof(DEVICE_HOOK_RECORD));
			tmpRecord->ReferenceCount = 1;
			status = _GetObjectName(DeviceObject, &tmpRecord->DeviceName);
			if (NT_SUCCESS(status)) {
				DriverHookRecordReference(DriverRecord);
				tmpRecord->DriverRecord = DriverRecord;
				tmpRecord->DeviceObject = DeviceObject;
				tmpRecord->MonitoringEnabled = MonitoringEnabled;
				tmpRecord->CreateReason = CreateReason;
				memcpy(tmpRecord->IRPMonitorSettings, DriverRecord->IRPSettings, sizeof(tmpRecord->IRPMonitorSettings));
				memcpy(tmpRecord->FastIoMonitorSettings, DriverRecord->FastIoSettings, sizeof(tmpRecord->FastIoMonitorSettings));			if (IRPSettings != NULL)
					memcpy(&tmpRecord->IRPMonitorSettings, IRPSettings, sizeof(tmpRecord->IRPMonitorSettings));

				if (FastIoSettings != NULL)
					memcpy(&tmpRecord->FastIoMonitorSettings, FastIoSettings, sizeof(tmpRecord->FastIoMonitorSettings));

				*Record = tmpRecord;
			}
		
			if (!NT_SUCCESS(status))
				IoReleaseRemoveLock(&_rundownLock, tmpRecord);
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(tmpRecord);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}

static VOID _DeviceHookRecordFree(PDEVICE_HOOK_RECORD Record)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

	_InvalidateDeviceHookRecord(Record);
	DriverHookRecordDereference(Record->DriverRecord);
	HeapMemoryFree(Record->DeviceName.Buffer);
	IoReleaseRemoveLock(&_rundownLock, Record);
	HeapMemoryFree(Record);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static NTSTATUS _CreateRecordsForExistingDevices(PDRIVER_HOOK_RECORD DriverRecord, PDEVICE_HOOK_RECORD **Records, PULONG Count)
{
	ULONG i = 0;
	ULONG deviceCount = 0;
	PDEVICE_OBJECT *devices = NULL;
	PDEVICE_HOOK_RECORD *hookRecords = NULL;
	ULONG hookRecordCount = 0;
	PDRIVER_OBJECT driverObject = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverRecord=0x%p; Records=0x%p; Count=0x%p", DriverRecord, Records, Count);

	driverObject = DriverRecord->DriverObject;
	status = _EnumDriverDevices(driverObject, &devices, &deviceCount);
	if (NT_SUCCESS(status)) {
		hookRecordCount = deviceCount;
		hookRecords = (PDEVICE_HOOK_RECORD *)HeapMemoryAllocNonPaged(sizeof(PDEVICE_HOOK_RECORD)*hookRecordCount);
		if (hookRecords != NULL) {
			for (i = 0; i < deviceCount; ++i) {
				PDEVICE_HOOK_RECORD deviceRecord = NULL;

				status = _DeviceHookRecordCreate(DriverRecord, &deviceRecord, NULL, NULL, FALSE, edrcrDriverHooked, devices[i]);
				if (NT_SUCCESS(status))
					hookRecords[i] = deviceRecord;

				if (!NT_SUCCESS(status)) {
					ULONG j = 0;

					for (j = 0; j < i; ++j)
						DeviceHookRecordDereference(hookRecords[j]);

					break;
				}
			}

			if (NT_SUCCESS(status)) {
				*Records = hookRecords;
				*Count = hookRecordCount;
			}

			if (!NT_SUCCESS(status))
				HeapMemoryFree(hookRecords);
		} else status = STATUS_INSUFFICIENT_RESOURCES;

		_ReleaseDeviceArray(devices, deviceCount);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Records=0x%p, *Count=%u", status, *Records, *Count);
	return status;
}

static VOID _FreeDeviceHookRecordArray(PDEVICE_HOOK_RECORD *Array, ULONG Count)
{
	ULONG i = 0;
	DEBUG_ENTER_FUNCTION("Array=0x%p; Count=%u", Array, Count);

	for (i = 0; i < Count; ++i)
		DeviceHookRecordDereference(Array[i]);

	HeapMemoryFree(Array);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                      PUBLIC FUNCTIONS                                */
/************************************************************************/

VOID DriverHookRecordReference(PDRIVER_HOOK_RECORD Record)
{
	InterlockedIncrement(&Record->ReferenceCount);

	return;
}


VOID DriverHookRecordDereference(PDRIVER_HOOK_RECORD Record)
{
	if (InterlockedDecrement(&Record->ReferenceCount) == 0)
		_DriverHookRecordFree(Record);

	return;
}

VOID DeviceHookRecordReference(PDEVICE_HOOK_RECORD Record)
{
	InterlockedIncrement(&Record->ReferenceCount);

	return;
}


VOID DeviceHookRecordDereference(PDEVICE_HOOK_RECORD Record)
{
	if (InterlockedDecrement(&Record->ReferenceCount) == 0)
		_DeviceHookRecordFree(Record);

	return;
}

NTSTATUS HookDriverObject(PDRIVER_OBJECT DriverObject, PDRIVER_MONITOR_SETTINGS MonitorSettings, PDRIVER_HOOK_RECORD *DriverRecord)
{
	KIRQL irql;
	PDRIVER_HOOK_RECORD record = NULL;
	PDEVICE_HOOK_RECORD *existingDevices = NULL;
	ULONG existingDeviceCount = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; MonitorSettings=%u; DriverRecord=0x%p", DriverObject, MonitorSettings, DriverRecord);

	status = _DriverHookRecordCreate(DriverObject, MonitorSettings, FALSE, &record);
	if (NT_SUCCESS(status)) {
		status = _CreateRecordsForExistingDevices(record, &existingDevices, &existingDeviceCount);
		if (NT_SUCCESS(status)) {
			KeAcquireSpinLock(&_driverTableLock, &irql);
			if (HashTableGet(_driverTable, DriverObject) == NULL) {
				KIRQL irql2;
				ULONG i = 0;

				DriverHookRecordReference(record);
				HashTableInsert(_driverTable, &record->HashItem, DriverObject);
				KeAcquireSpinLock(&record->SelectedDevicesLock, &irql2);
				for (i = 0; i < existingDeviceCount; ++i) {
					PDEVICE_HOOK_RECORD deviceRecord = existingDevices[i];

					DeviceHookRecordReference(deviceRecord);
					HashTableInsert(record->SelectedDevices, &deviceRecord->HashItem, deviceRecord->DeviceObject);
				}

				KeReleaseSpinLock(&record->SelectedDevicesLock, irql2);
				KeReleaseSpinLock(&_driverTableLock, irql);
				_MakeDriverHookRecordValid(record);
				if (record->MonitoringEnabled)
					_HookDriverObject(DriverObject, record);

				DriverHookRecordReference(record);
				*DriverRecord = record;
			} else {
				KeReleaseSpinLock(&_driverTableLock, irql);
				status = STATUS_ALREADY_REGISTERED;
			}

			_FreeDeviceHookRecordArray(existingDevices, existingDeviceCount);
		}

		DriverHookRecordDereference(record);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *DriverRecord=0x%p", status, *DriverRecord);
	return status;
}

NTSTATUS UnhookDriverObject(PDRIVER_HOOK_RECORD DriverRecord)
{
	KIRQL irql;
	PHASH_ITEM h = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverRecord=0x%p", DriverRecord);

	KeAcquireSpinLock(&_driverTableLock, &irql);
	h = HashTableDelete(_driverTable, DriverRecord->DriverObject);
	if (h != NULL) {
		KeReleaseSpinLock(&_driverTableLock, irql);
		if (DriverRecord->MonitoringEnabled) {
			_UnhookDriverObject(DriverRecord);
			DriverRecord->MonitoringEnabled = FALSE;
		}
		
		KeAcquireSpinLock(&DriverRecord->SelectedDevicesLock, &irql);
		HashTableClear(DriverRecord->SelectedDevices, TRUE);
		KeReleaseSpinLock(&DriverRecord->SelectedDevicesLock, irql);
		_InvalidateDriverHookRecord(DriverRecord);
		DriverHookRecordDereference(DriverRecord);
		status = STATUS_SUCCESS;
	} else {
		KeReleaseSpinLock(&_driverTableLock, irql);
		status = STATUS_NOT_FOUND;
		ASSERT(FALSE);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

PDRIVER_HOOK_RECORD DriverHookRecordGet(PDRIVER_OBJECT DriverObject)
{
	KIRQL irql;
	PHASH_ITEM h = NULL;
	PDRIVER_HOOK_RECORD ret = NULL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	KeAcquireSpinLock(&_driverTableLock, &irql);
	if (!_shutdownInProgress) {
		h = HashTableGet(_driverTable, DriverObject);
		if (h != NULL) {
			ret = CONTAINING_RECORD(h, DRIVER_HOOK_RECORD, HashItem);
			DriverHookRecordReference(ret);
		}
	}

	KeReleaseSpinLock(&_driverTableLock, irql);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}

NTSTATUS DriverHookRecordSetInfo(PDRIVER_HOOK_RECORD Record, PDRIVER_MONITOR_SETTINGS DriverSettings)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%x; DriverSettings=0x%p", Record, DriverSettings);

	Record->MonitorNewDevices = DriverSettings->MonitorNewDevices;
	Record->MonitorIRPCompletion = DriverSettings->MonitorIRPCompletion;
	Record->MonitorData = DriverSettings->MonitorData;
	if (!Record->MonitoringEnabled) {
		Record->MonitorAddDevice = DriverSettings->MonitorAddDevice;
		Record->MonitorStartIo = DriverSettings->MonitorStartIo;
		Record->MonitorDriverUnload = DriverSettings->MonitorUnload;
		Record->MonitorIRP = DriverSettings->MonitorIRP;
		Record->MonitorFastIo = DriverSettings->MonitorFastIo;
		memcpy(Record->IRPSettings, DriverSettings->IRPSettings, sizeof(Record->IRPSettings));
		memcpy(Record->FastIoSettings, DriverSettings->FastIoSettings, sizeof(Record->FastIoSettings));
	}

	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

VOID DriverHookRecordGetInfo(PDRIVER_HOOK_RECORD Record, PDRIVER_MONITOR_SETTINGS DriverSettings, PBOOLEAN Enabled)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p; DriverSettings=0x%p; Enabled=0x%p", Record, DriverSettings, Enabled);

	DriverSettings->MonitorAddDevice = Record->MonitorAddDevice;
	DriverSettings->MonitorFastIo = Record->MonitorFastIo;
	DriverSettings->MonitorIRP = Record->MonitorIRP;
	DriverSettings->MonitorIRPCompletion = Record->MonitorIRPCompletion;
	DriverSettings->MonitorNewDevices = Record->MonitorNewDevices;
	DriverSettings->MonitorStartIo = Record->MonitorStartIo;
	DriverSettings->MonitorUnload = Record->MonitorDriverUnload;
	DriverSettings->MonitorData = Record->MonitorData;
	memcpy(DriverSettings->IRPSettings, Record->IRPSettings, sizeof(DriverSettings->IRPSettings));
	memcpy(DriverSettings->FastIoSettings, Record->FastIoSettings, sizeof(DriverSettings->FastIoSettings));
	*Enabled = Record->MonitoringEnabled;

	DEBUG_EXIT_FUNCTION("void, *Enabled=%u", *Enabled);
	return;
}



NTSTATUS DriverHookRecordEnable(PDRIVER_HOOK_RECORD Record, BOOLEAN Enable)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Enable=%u", Record, Enable);

	if (Enable) {
		if (!Record->MonitoringEnabled) {
			Record->MonitoringEnabled = TRUE;
			_HookDriverObject(Record->DriverObject, Record);
			status = STATUS_SUCCESS;
		} else status = STATUS_DEVICE_NOT_READY;
	} else {
		if (Record->MonitoringEnabled) {
			Record->MonitoringEnabled = FALSE;
			_UnhookDriverObject(Record);
			status = STATUS_SUCCESS;
		} else status = STATUS_DEVICE_NOT_READY;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS DriverHookRecordAddDevice(PDRIVER_HOOK_RECORD DriverRecord, PDEVICE_OBJECT DeviceObject, PUCHAR IRPSettings, PUCHAR FastIoSettings, BOOLEAN MonitoringEnabled, PDEVICE_HOOK_RECORD *DeviceRecord)
{
	KIRQL irql;
	PHASH_ITEM h = NULL;
	PDEVICE_HOOK_RECORD newDeviceRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; DeviceObject=0x%p; IRPSettings=0x%p; FastIoSettings=0x%p; MonitoringEnabled=%u; DeviceRecord=0x%p", DriverRecord, DeviceObject, IRPSettings, FastIoSettings, MonitoringEnabled, DeviceRecord);
	
	status = _DeviceHookRecordCreate(DriverRecord, &newDeviceRecord, IRPSettings, FastIoSettings, MonitoringEnabled, edrcrUserRequest, DeviceObject);
	if (NT_SUCCESS(status)) {
		KeAcquireSpinLock(&DriverRecord->SelectedDevicesLock, &irql);
		h = HashTableGet(DriverRecord->SelectedDevices, DeviceObject);
		if (h == NULL) {
			// For the hash table
			DeviceHookRecordReference(newDeviceRecord);
			HashTableInsert(DriverRecord->SelectedDevices, &newDeviceRecord->HashItem, DeviceObject);
			KeReleaseSpinLock(&DriverRecord->SelectedDevicesLock, irql);
			_MakeDeviceHookRecordValid(newDeviceRecord);
			// For the reference going out of this routine
			DeviceHookRecordReference(newDeviceRecord);
			*DeviceRecord = newDeviceRecord;
		} else {
			PDEVICE_HOOK_RECORD existingDeviceRecord = NULL;

			existingDeviceRecord = CONTAINING_RECORD(h, DEVICE_HOOK_RECORD, HashItem);
			DeviceHookRecordReference(existingDeviceRecord);
			KeReleaseSpinLock(&DriverRecord->SelectedDevicesLock, irql);
			if (existingDeviceRecord->CreateReason == edrcrDriverHooked) {
				memset(existingDeviceRecord->IRPMonitorSettings, TRUE, (IRP_MJ_MAXIMUM_FUNCTION + 1)*sizeof(UCHAR));
				if (IRPSettings != NULL)
					memcpy(existingDeviceRecord->IRPMonitorSettings, IRPSettings, (IRP_MJ_MAXIMUM_FUNCTION + 1)*sizeof(UCHAR));

				memset(existingDeviceRecord->FastIoMonitorSettings, TRUE, FastIoMax*sizeof(UCHAR));
				if (FastIoSettings != NULL)
					memcpy(existingDeviceRecord->FastIoMonitorSettings, FastIoSettings,  FastIoMax*sizeof(UCHAR));

				existingDeviceRecord->CreateReason = edrcrUserRequest;
				existingDeviceRecord->MonitoringEnabled = MonitoringEnabled;
				_MakeDeviceHookRecordValid(existingDeviceRecord);
				DeviceHookRecordReference(existingDeviceRecord);
				*DeviceRecord = existingDeviceRecord;
				status = STATUS_SUCCESS;
			} else status = STATUS_ALREADY_REGISTERED;

			DeviceHookRecordDereference(existingDeviceRecord);
		}

		DeviceHookRecordDereference(newDeviceRecord);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *DeviceRecord=0x%p", status, *DeviceRecord);
	return status;
}


NTSTATUS DriverHookRecordDeleteDevice(PDEVICE_HOOK_RECORD DeviceRecord)
{
	KIRQL irql;
	PHASH_ITEM h = NULL;
	PDRIVER_HOOK_RECORD driverRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	DEBUG_ENTER_FUNCTION("DeviceRecord=0x%p", DeviceRecord);

	driverRecord = DeviceRecord->DriverRecord;
	KeAcquireSpinLock(&driverRecord->SelectedDevicesLock, &irql);
	h = HashTableGet(driverRecord->SelectedDevices, DeviceRecord->DeviceObject);
	if (h != NULL) {
		deviceRecord = CONTAINING_RECORD(h, DEVICE_HOOK_RECORD, HashItem);
		DeviceHookRecordReference(deviceRecord);
		KeReleaseSpinLock(&driverRecord->SelectedDevicesLock, irql);
		if (deviceRecord->CreateReason == edrcrUserRequest) {
			deviceRecord->CreateReason = edrcrDriverHooked;
			deviceRecord->MonitoringEnabled = FALSE;
			_InvalidateDeviceHookRecord(deviceRecord);
			status = STATUS_SUCCESS;
		} else status = STATUS_NOT_FOUND;
		
		DeviceHookRecordDereference(deviceRecord);
	} else {
		KeReleaseSpinLock(&driverRecord->SelectedDevicesLock, irql);
		status = STATUS_NOT_FOUND;
		ASSERT(FALSE);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


PDEVICE_HOOK_RECORD DriverHookRecordGetDevice(PDRIVER_HOOK_RECORD Record, PDEVICE_OBJECT DeviceObject)
{
	KIRQL irql;
	PHASH_ITEM h = NULL;
	PDEVICE_HOOK_RECORD ret = NULL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; DeviceObject=0x%p", Record, DeviceObject);

	KeAcquireSpinLock(&Record->SelectedDevicesLock, &irql);
	h = HashTableGet(Record->SelectedDevices, DeviceObject);
	if (h != NULL) {
		ret = CONTAINING_RECORD(h, DEVICE_HOOK_RECORD, HashItem);
		DeviceHookRecordReference(ret);
	}

	KeReleaseSpinLock(&Record->SelectedDevicesLock, irql);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


NTSTATUS DeviceHookRecordSetInfo(PDEVICE_HOOK_RECORD Record, PUCHAR IRPSettings, PUCHAR FastIoSettings, BOOLEAN MonitoringEnabled)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; IRPSettings=0x%p; FastIoSettings=0x%p; MonitoringEnabled=%u", Record, IRPSettings, FastIoSettings, MonitoringEnabled);

	if (IRPSettings != NULL)
		memcpy(Record->IRPMonitorSettings, IRPSettings, sizeof(Record->IRPMonitorSettings));
	
	if (FastIoSettings != NULL)
		memcpy(Record->FastIoMonitorSettings, FastIoSettings, sizeof(Record->FastIoMonitorSettings));
	
	Record->MonitoringEnabled = MonitoringEnabled;
	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID DeviceHookRecordGetInfo(PDEVICE_HOOK_RECORD Record, PUCHAR IRPSettings, PUCHAR FastIoSettings, PBOOLEAN MonitoringEnabled)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p; IRPSettings=0x%p; FastIoSettings=0x%p; MonitoringEnabled=0x%p", Record, IRPSettings, FastIoSettings, MonitoringEnabled);

	memcpy(IRPSettings, Record->IRPMonitorSettings, sizeof(Record->IRPMonitorSettings));
	memcpy(FastIoSettings, Record->FastIoMonitorSettings, sizeof(Record->FastIoMonitorSettings));
	*MonitoringEnabled = Record->MonitoringEnabled;

	DEBUG_EXIT_FUNCTION("void, *MonitoringEnabled=%u", *MonitoringEnabled);
	return;
}


NTSTATUS HookObjectsEnumerate(PVOID Buffer, ULONG BufferLength, PULONG ReturnLength)
{
	KIRQL irql = 0;
	ULONG requiredLength = sizeof(HOOKED_OBJECTS_INFO);
	HASH_TABLE_ITERATOR itDrivers;
	PHASH_ITEM hDrivers = NULL;
	HASH_TABLE_ITERATOR itDevices;
	PHASH_ITEM hDevices = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Buffer=0x%p; BufferLength=%u; ReturnLength=0x%p", Buffer, BufferLength, ReturnLength);

	KeAcquireSpinLock(&_driverTableLock, &irql);
	if (HashTableGetFirst(_driverTable, &itDrivers)) {
		do {
			KIRQL irql2;
			PDRIVER_HOOK_RECORD driverRecord = NULL;

			hDrivers = HashTableIteratorGetData(&itDrivers);
			driverRecord = CONTAINING_RECORD(hDrivers, DRIVER_HOOK_RECORD, HashItem);
			requiredLength += sizeof(HOOKED_DRIVER_INFO) + driverRecord->DriverName.Length + sizeof(WCHAR);
			KeAcquireSpinLock(&driverRecord->SelectedDevicesLock, &irql2);
			if (HashTableGetFirst(driverRecord->SelectedDevices, &itDevices)) {
				do {
					PDEVICE_HOOK_RECORD deviceRecord = NULL;

					hDevices = HashTableIteratorGetData(&itDevices);
					deviceRecord = CONTAINING_RECORD(hDevices, DEVICE_HOOK_RECORD, HashItem);
					if (deviceRecord->CreateReason == edrcrUserRequest)
						requiredLength += sizeof(HOOKED_DEVICE_INFO) + deviceRecord->DeviceName.Length + sizeof(WCHAR);
				} while (HashTableGetNext(&itDevices));

				HashTableIteratorFinit(&itDevices);
			}

			KeReleaseSpinLock(&driverRecord->SelectedDevicesLock, irql);
		} while (HashTableGetNext(&itDrivers));

		HashTableIteratorFinit(&itDrivers);
	}

	*ReturnLength = requiredLength;
	if (requiredLength <= BufferLength) {
		PHOOKED_OBJECTS_INFO infoHeader = (PHOOKED_OBJECTS_INFO)Buffer;
		PHOOKED_DRIVER_INFO driverInfo = (PHOOKED_DRIVER_INFO)(infoHeader + 1);

		infoHeader->NumberOfHookedDevices = 0;
		infoHeader->NumberOfHookedDrivers = 0;
		status = STATUS_SUCCESS;
		if (HashTableGetFirst(_driverTable, &itDrivers)) {
			do {
				KIRQL irql2;
				PDRIVER_HOOK_RECORD driverRecord = NULL;

				hDrivers = HashTableIteratorGetData(&itDrivers);
				driverRecord = CONTAINING_RECORD(hDrivers, DRIVER_HOOK_RECORD, HashItem);
				infoHeader->NumberOfHookedDrivers++;
				driverInfo->ObjectId = driverRecord;
				driverInfo->EntrySize = sizeof(HOOKED_DRIVER_INFO) + driverRecord->DriverName.Length + sizeof(WCHAR);
				driverInfo->DriverObject = driverRecord->DriverObject;
				driverInfo->MonitoringEnabled = driverRecord->MonitoringEnabled;
				driverInfo->MonitorSettings.MonitorAddDevice = driverRecord->MonitorAddDevice;
				driverInfo->MonitorSettings.MonitorNewDevices = driverRecord->MonitorNewDevices;
				driverInfo->MonitorSettings.MonitorStartIo = driverRecord->MonitorStartIo;
				driverInfo->MonitorSettings.MonitorUnload = driverRecord->MonitorDriverUnload;
				driverInfo->MonitorSettings.MonitorIRP = driverRecord->MonitorIRP;
				driverInfo->MonitorSettings.MonitorIRPCompletion = driverRecord->MonitorIRPCompletion;
				driverInfo->MonitorSettings.MonitorFastIo = driverRecord->MonitorFastIo;
				driverInfo->MonitorSettings.MonitorData = driverRecord->MonitorData;
				memcpy(driverInfo->MonitorSettings.IRPSettings, driverRecord->IRPSettings, sizeof(driverInfo->MonitorSettings.IRPSettings));
				memcpy(driverInfo->MonitorSettings.FastIoSettings, driverRecord->FastIoSettings, sizeof(driverInfo->MonitorSettings.FastIoSettings));
				driverInfo->NumberOfHookedDevices = 0;
				driverInfo->DriverNameLen = driverRecord->DriverName.Length + sizeof(WCHAR);
				memcpy(driverInfo->DriverName, driverRecord->DriverName.Buffer, driverRecord->DriverName.Length + sizeof(WCHAR));
				KeAcquireSpinLock(&driverRecord->SelectedDevicesLock, &irql2);
				if (HashTableGetFirst(driverRecord->SelectedDevices, &itDevices)) {
					PHOOKED_DEVICE_INFO deviceInfo = (PHOOKED_DEVICE_INFO)((PUCHAR)driverInfo + driverInfo->EntrySize);

					do {
						PDEVICE_HOOK_RECORD deviceRecord = NULL;

						hDevices = HashTableIteratorGetData(&itDevices);
						deviceRecord = CONTAINING_RECORD(hDevices, DEVICE_HOOK_RECORD, HashItem);
						if (deviceRecord->CreateReason == edrcrUserRequest) {
							driverInfo->NumberOfHookedDevices++;
							infoHeader->NumberOfHookedDevices++;
							deviceInfo->EntrySize = sizeof(HOOKED_DEVICE_INFO) + deviceRecord->DeviceName.Length + sizeof(WCHAR);
							deviceInfo->ObjectId = deviceRecord;
							deviceInfo->DeviceObject = deviceRecord->DeviceObject;
							memcpy(deviceInfo->FastIoSettings, deviceRecord->FastIoMonitorSettings, sizeof(deviceInfo->FastIoSettings));
							memcpy(deviceInfo->IRPSettings, deviceRecord->IRPMonitorSettings, sizeof(deviceInfo->IRPSettings));
							deviceInfo->MonitoringEnabled = deviceRecord->MonitoringEnabled;
							deviceInfo->DeviceNameLen = deviceRecord->DeviceName.Length + sizeof(WCHAR);
							memcpy(&deviceInfo->DeviceName, deviceRecord->DeviceName.Buffer, deviceRecord->DeviceName.Length + sizeof(WCHAR));
							deviceInfo = (PHOOKED_DEVICE_INFO)((PUCHAR)deviceInfo + deviceInfo->EntrySize);
						}
					} while (HashTableGetNext(&itDevices));

					HashTableIteratorFinit(&itDevices);
					driverInfo = (PHOOKED_DRIVER_INFO)deviceInfo;
				}

				KeReleaseSpinLock(&driverRecord->SelectedDevicesLock, irql);
			} while (HashTableGetNext(&itDrivers));

			HashTableIteratorFinit(&itDrivers);
		}
	} else status = STATUS_BUFFER_TOO_SMALL;

	KeReleaseSpinLock(&_driverTableLock, irql);

	DEBUG_EXIT_FUNCTION("0x%x, *ReturnLength=%u", status, *ReturnLength);
	return status;
}


BOOLEAN DeviceHookRecordValid(PDEVICE_HOOK_RECORD DeviceRecord)
{
	KIRQL irql;
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("DeviceRecord=0x%p", DeviceRecord);

	KeAcquireSpinLock(&_deviceValidationTableLock, &irql);
	ret = HashTableGet(_deviceValidationTable, DeviceRecord) != NULL;
	if (ret)
		DeviceHookRecordReference(DeviceRecord);

	KeReleaseSpinLock(&_deviceValidationTableLock, irql);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


BOOLEAN DriverHookRecordValid(PDRIVER_HOOK_RECORD DriverRecord)
{
	KIRQL irql;
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("DriverRecord=0x%p", DriverRecord);

	KeAcquireSpinLock(&_driverValidationTableLock, &irql);
	ret = HashTableGet(_driverValidationTable, DriverRecord) != NULL;
	if (ret)
		DriverHookRecordReference(DriverRecord);

	KeReleaseSpinLock(&_driverValidationTableLock, irql);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}



/************************************************************************/
/*                       INITIALIZATION AND FINALIZATION                */
/************************************************************************/



NTSTATUS HookModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(Context);

	IoInitializeRemoveLock(&_rundownLock, (ULONG)' LRH', 0x7FFFFFFF, 0x7FFFFFFF);
	status = IoAcquireRemoveLock(&_rundownLock, DriverObject);
	if (NT_SUCCESS(status)) {
		KeInitializeSpinLock(&_driverValidationTableLock);
		status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _DriverValidationCompareFunction, NULL, &_driverValidationTable);
		if (NT_SUCCESS(status)) {
			KeInitializeSpinLock(&_deviceValidationTableLock);
			status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _DeviceValidationCompareFunction, NULL, &_deviceValidationTable);
			if (NT_SUCCESS(status)) {
				KeInitializeSpinLock(&_driverTableLock);
				status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _DriverCompareFunction, _DriverFreeFunction, &_driverTable);
				if (!NT_SUCCESS(status))
					HashTableDestroy(_deviceValidationTable);
			}

			if (!NT_SUCCESS(status))
				HashTableDestroy(_driverValidationTable);
		}

		if (!NT_SUCCESS(status))
			IoReleaseRemoveLockAndWait(&_rundownLock, DriverObject);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}



VOID HookModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	KIRQL irql;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(Context);
	
	KeAcquireSpinLock(&_driverTableLock, &irql);
	_shutdownInProgress = TRUE;
	KeReleaseSpinLock(&_driverTableLock, irql);
	HashTableDestroy(_deviceValidationTable);
	HashTableDestroy(_driverValidationTable);
	HashTableDestroy(_driverTable);
	IoReleaseRemoveLockAndWait(&_rundownLock, DriverObject);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
