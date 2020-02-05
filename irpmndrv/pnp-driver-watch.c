
#include <ntifs.h>
#include <ntstrsafe.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hash_table.h"
#include "string-hash-table.h"
#include "ioctls.h"
#include "kernel-shared.h"
#include "hook.h"
#include "req-queue.h"
#include "multistring.h"
#include "regman.h"
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
static PHASH_TABLE _lowerClassGuidTable = NULL;
static PHASH_TABLE _upperClassGuidTable = NULL;
static ERESOURCE _driverNamesLock;
static ERESOURCE _classGuidsLock;
static volatile LONG _numberofClasses = 0;
static PDRIVER_OBJECT _driverObject = NULL;

static UNICODE_STRING _driverServiceName;
static ULONG _currentControlSet = 1;
static RTL_OSVERSIONINFOW _versionInfo;

/************************************************************************/
/*                    HELPER FUNCTIONS                                  */
/************************************************************************/


static NTSTATUS _GetCurrentControlSetNumber(void)
{
	HANDLE hSelectKey = NULL;
	UNICODE_STRING uSelectKey;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	RtlInitUnicodeString(&uSelectKey, L"\\Registry\\Machine\\SYSTEM\\Select");
	InitializeObjectAttributes(&oa, &uSelectKey, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenKey(&hSelectKey, KEY_QUERY_VALUE, &oa);
	if (NT_SUCCESS(status)) {
		ULONG retLength = 0;
		UNICODE_STRING uValueName;
		UCHAR kvpiStorage[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
		PKEY_VALUE_PARTIAL_INFORMATION kvpi = (PKEY_VALUE_PARTIAL_INFORMATION)kvpiStorage;

		RtlInitUnicodeString(&uValueName, L"Current");
		status = ZwQueryValueKey(hSelectKey, &uValueName, KeyValuePartialInformation, kvpi, sizeof(kvpiStorage), &retLength);
		if (NT_SUCCESS(status)) {
			if (kvpi->DataLength == sizeof(ULONG))
				_currentControlSet = *(PULONG)kvpi->Data;
			else status = STATUS_INVALID_PARAMETER;
		}

		ZwClose(hSelectKey);
	}

	DEBUG_EXIT_FUNCTION("0x%x, _currentControlSet=%u", status, _currentControlSet);
	return status;
}


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
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
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
		}
		else ctx->Status = STATUS_BUFFER_TOO_SMALL;
	}

	DEBUG_EXIT_FUNCTION("0x%x", ctx->Status);
	return NT_SUCCESS(ctx->Status);
}


static NTSTATUS _CaptureServiceName(PUNICODE_STRING RegistryPath)
{
	const wchar_t *wServiceName = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);

	status = STATUS_SUCCESS;
	memset(&_driverServiceName, 0, sizeof(_driverServiceName));
	wServiceName = RegistryPath->Buffer + RegistryPath->Length / sizeof(wchar_t) - 1;
	while (*wServiceName != L'\\') {
		--wServiceName;
		_driverServiceName.Length += sizeof(wchar_t);
	}

	_driverServiceName.MaximumLength = _driverServiceName.Length + sizeof(wchar_t);
	_driverServiceName.Buffer = HeapMemoryAllocPaged(_driverServiceName.MaximumLength);
	if (_driverServiceName.Buffer != NULL) {
		memcpy(_driverServiceName.Buffer, wServiceName + 1, _driverServiceName.Length);
		_driverServiceName.Buffer[_driverServiceName.Length / sizeof(wchar_t)] = L'\0';
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, _driverServiceName=\"%wZ\"", status, &_driverServiceName);
	return status;
}


static void _FreeServiceName(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	HeapMemoryFree(_driverServiceName.Buffer);
	memset(&_driverServiceName, 0, sizeof(_driverServiceName));

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _CheckDriver(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject)
{
	ULONG returnLength = 0;
	PDRIVER_NAME_WATCH_RECORD nameRecord = NULL;
	PDRIVER_HOOK_RECORD hookRecord = NULL;
	PDEVICE_HOOK_RECORD deviceRecord = NULL;
	POBJECT_NAME_INFORMATION oni = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; DeviceObject=0x%p", DriverObject, DeviceObject);

	status = ObQueryNameString(DriverObject, NULL, 0, &returnLength);
	if (status == STATUS_INFO_LENGTH_MISMATCH) {
		returnLength += sizeof(UNICODE_STRING) + sizeof(WCHAR);
		oni = (POBJECT_NAME_INFORMATION)HeapMemoryAllocPaged(returnLength);
		if (oni != NULL) {
			RtlSecureZeroMemory(oni, returnLength);
			status = ObQueryNameString(DriverObject, oni, returnLength, &returnLength);
			if (NT_SUCCESS(status)) {
				KeEnterCriticalRegion();
				ExAcquireResourceSharedLite(&_driverNamesLock, TRUE);
				nameRecord = (PDRIVER_NAME_WATCH_RECORD)StringHashTableGetUnicodeString(_driverNameTable, &oni->Name);
				if (nameRecord != NULL) {
					status = HookDriverObject(DriverObject, &nameRecord->MonitorSettings, FALSE, &hookRecord);
					if (NT_SUCCESS(status)) {
						status = DriverHookRecordEnable(hookRecord, TRUE);
						if (NT_SUCCESS(status)) {
						
							status = DriverHookRecordAddDevice(hookRecord, DeviceObject, NULL, NULL, TRUE, &deviceRecord);
							if (NT_SUCCESS(status)) {

								DeviceHookRecordDereference(deviceRecord);
							}
						}

						DriverHookRecordDereference(hookRecord);
					}
				}

				ExReleaseResourceLite(&_driverNamesLock);
				KeLeaveCriticalRegion();
			}

			HeapMemoryFree(oni);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	if (NT_SUCCESS(status)) {
		PREQUEST_HEADER rq = NULL;

		status = RequestXXXDetectedCreate(ertDriverDetected, DriverObject, NULL, &rq);
		if (NT_SUCCESS(status))
			RequestQueueInsert(rq);

		status = RequestXXXDetectedCreate(ertDeviceDetected, DriverObject, DeviceObject, &rq);
		if (NT_SUCCESS(status))
			RequestQueueInsert(rq);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject)
{
	PDEVICE_OBJECT deviceObject = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; PhysicalDeviceObject=0x%p", DriverObject, PhysicalDeviceObject);

	UNREFERENCED_PARAMETER(DriverObject);

	deviceObject = PhysicalDeviceObject;
	do {
		status = _CheckDriver(deviceObject->DriverObject, deviceObject);
		deviceObject = deviceObject->AttachedDevice;
	} while (deviceObject != NULL);

	status = STATUS_SUCCESS;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static ULONG32 _HashFunction(PVOID Key)
{
	ULONG32 h = 0;
	PUCHAR tmp = (PUCHAR)Key;

	for (SIZE_T i = 0; i < sizeof(GUID); ++i) {
		h += *tmp;
		++tmp;
	}

	return h;
}


BOOLEAN _CompareFunction(PHASH_ITEM Item, PVOID Key)
{
	PDEVICE_CLASS_WATCH_RECORD rec = CONTAINING_RECORD(Item, DEVICE_CLASS_WATCH_RECORD, HashItem);

	return RtlEqualMemory(&rec->ClassGuid, Key, sizeof(GUID));
}


static NTSTATUS _InstallUninstallFilterXP(_In_ PUNICODE_STRING ClassGuid, _In_ BOOLEAN UpperFilter, _In_ BOOLEAN Beginning, _In_ BOOLEAN Install);

static VOID _FreeFunction(PHASH_ITEM Item)
{
	PDEVICE_CLASS_WATCH_RECORD rec = CONTAINING_RECORD(Item, DEVICE_CLASS_WATCH_RECORD, HashItem);
	DEBUG_ENTER_FUNCTION("Item=0x%p", Item);

	if (_versionInfo.dwMajorVersion >= 6) {
		RegManValueCallbackUnregiser(rec->CallbackHandle);
		RegManKeyValueDelete(rec->ValueRecord);
		RegManKeyUnregister(rec->KeyRecord);
	} else _InstallUninstallFilterXP(&rec->ClassGuidString, (rec->Flags & CLASS_WATCH_FLAG_UPPERFILTER) != 0, (rec->Flags & CLASS_WATCH_FLAG_BEGINNING) != 0, FALSE);

	RtlFreeUnicodeString(&rec->ClassGuidString);
	HeapMemoryFree(rec);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _InstallUninstallFilterXP(_In_ PUNICODE_STRING ClassGuid, _In_ BOOLEAN UpperFilter, _In_ BOOLEAN Beginning, _In_ BOOLEAN Install)
{
	HANDLE keyHandle = NULL;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uValueName;
	ULONG kvfiLen = 0;
	PKEY_VALUE_FULL_INFORMATION kvfi = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DECLARE_UNICODE_STRING_SIZE(uClassKey, 256);
	DEBUG_ENTER_FUNCTION("ClassGuid=\"%wZ\"; UpperFilter=%u; Beginning=%u; Install=%u", ClassGuid, UpperFilter, Beginning, Install);

	status = RtlUnicodeStringPrintf(&uClassKey, L"\\Registry\\Machine\\SYSTEM\\ControlSet%.3u\\Control\\Class\\%wZ", _currentControlSet, ClassGuid);
	if (NT_SUCCESS(status)) {
		if (UpperFilter)
			RtlInitUnicodeString(&uValueName, L"UpperFilters");
		else RtlInitUnicodeString(&uValueName, L"LowerFilters");

		InitializeObjectAttributes(&oa, &uClassKey, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&keyHandle, KEY_QUERY_VALUE | KEY_SET_VALUE, &oa);
		if (NT_SUCCESS(status)) {
			status = ZwQueryValueKey(keyHandle, &uValueName, KeyValueFullInformation, NULL, 0, &kvfiLen);
			if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_OBJECT_NAME_NOT_FOUND) {
				kvfiLen += sizeof(KEY_VALUE_FULL_INFORMATION) + _driverServiceName.Length + 2 * sizeof(wchar_t);
				kvfi = (PKEY_VALUE_FULL_INFORMATION)HeapMemoryAllocPaged(kvfiLen);
				if (kvfi != NULL) {
					RtlSecureZeroMemory(kvfi, kvfiLen);
					if (status == STATUS_OBJECT_NAME_NOT_FOUND)
						status = STATUS_SUCCESS;
					
					if (status == STATUS_BUFFER_TOO_SMALL)
						status = ZwQueryValueKey(keyHandle, &uValueName, KeyValueFullInformation, kvfi, kvfiLen, &kvfiLen);
					
					if (NT_SUCCESS(status)) {
						size_t newSize = 0;
						wchar_t *data = (wchar_t *)((PUCHAR)kvfi + kvfi->DataOffset);

						if (Install) {
							if (_MultiStringExists(data, &_driverServiceName, NULL))
								status = STATUS_OBJECT_NAME_COLLISION;

							if (NT_SUCCESS(status))
								_MultiStringInsert(data, &_driverServiceName, Beginning, &newSize);
						} else status = _MultiStringRemove(data, &_driverServiceName, &newSize) ? STATUS_SUCCESS : STATUS_OBJECT_NAME_NOT_FOUND;
						

						if (NT_SUCCESS(status))
							status = ZwSetValueKey(keyHandle, &uValueName, 0, REG_MULTI_SZ, data, (ULONG)newSize);

					}

					HeapMemoryFree(kvfi);
				} else status = STATUS_INSUFFICIENT_RESOURCES;
			}

			ZwClose(keyHandle);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _QueryCallback(_Inout_ PREGMAN_VALUE_INFO ValueInfo, _In_opt_ PVOID Context)
{
	size_t newSize = 0;
	BOOLEAN inserted = FALSE;
	ULONG tmpDataSize = 0;
	wchar_t *tmpData = NULL;
	PDEVICE_CLASS_WATCH_RECORD fdr = (PDEVICE_CLASS_WATCH_RECORD)Context;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ValueInfo=0x%p; Context=0x%p", ValueInfo, Context);

	tmpDataSize = ValueInfo->CurrentDataSize + 3 * sizeof(wchar_t) + _driverServiceName.Length;
	tmpData = ExAllocatePoolWithTag(PagedPool, tmpDataSize, REGMAN_POOL_TAG);
	if (tmpData != NULL) {
		RtlSecureZeroMemory(tmpData, tmpDataSize);
		memcpy(tmpData, ValueInfo->CurrentData, ValueInfo->CurrentDataSize);
		inserted = _MultiStringInsert(tmpData, &_driverServiceName, (fdr->Flags & CLASS_WATCH_FLAG_BEGINNING) != 0, &newSize);
		if (inserted) {
			if (ValueInfo->CurrentData != NULL)
				ExFreePoolWithTag(ValueInfo->CurrentData, REGMAN_POOL_TAG);

			ValueInfo->CurrentData = tmpData;
			ValueInfo->CurrentDataSize = (ULONG)newSize;
			ValueInfo->CurrentType = REG_MULTI_SZ;
		}

		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _SetCallback(_Inout_ PREGMAN_VALUE_INFO ValueInfo, _In_opt_ PVOID Context)
{
	size_t newSize = 0;
	BOOLEAN removed = FALSE;
	ULONG tmpDataSize = 0;
	wchar_t *tmpData = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ValueInfo=0x%p; Context=0x%p", ValueInfo, Context);

	tmpDataSize = ValueInfo->CurrentDataSize + 2 * sizeof(wchar_t);
	tmpData = ExAllocatePoolWithTag(PagedPool, tmpDataSize, REGMAN_POOL_TAG);
	if (tmpData != NULL) {
		RtlSecureZeroMemory(tmpData, tmpDataSize);
		memcpy(tmpData, ValueInfo->CurrentData, ValueInfo->CurrentDataSize);
		removed = _MultiStringRemove(tmpData, &_driverServiceName, &newSize);
		if (removed) {
			if (ValueInfo->CurrentData != NULL)
				ExFreePoolWithTag(ValueInfo->CurrentData, REGMAN_POOL_TAG);

			ValueInfo->CurrentData = tmpData;
			ValueInfo->CurrentDataSize = (ULONG)newSize;
			ValueInfo->CurrentType = REG_MULTI_SZ;
		}

		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/************************************************************************/
/*                     PUBLIC FUNCTIONS                                 */
/************************************************************************/

NTSTATUS PDWClassRegister(PGUID ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	PHASH_ITEM h = NULL;
	UNICODE_STRING uGuid;
	PHASH_TABLE targetTable = NULL;
	PDEVICE_CLASS_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ClassGuid=0x%p; UpperFilter=%u; Beginning=%u", ClassGuid, UpperFilter, Beginning);

	targetTable = (UpperFilter) ? _upperClassGuidTable : _lowerClassGuidTable;
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&_classGuidsLock, TRUE);
	h = HashTableGet(targetTable, ClassGuid);
	if (h == NULL) {
		rec = (PDEVICE_CLASS_WATCH_RECORD)HeapMemoryAllocPaged(sizeof(DEVICE_CLASS_WATCH_RECORD));
		if (rec != NULL) {
			memset(rec, 0, sizeof(DEVICE_CLASS_WATCH_RECORD));
			rec->ClassGuid = *ClassGuid;
			rec->Flags = 0;
			if (UpperFilter)
				rec->Flags |= CLASS_WATCH_FLAG_UPPERFILTER;

			if (Beginning)
				rec->Flags |= CLASS_WATCH_FLAG_BEGINNING;

			RtlSecureZeroMemory(&uGuid, sizeof(uGuid));
			status = RtlStringFromGUID(ClassGuid, &uGuid);
			if (NT_SUCCESS(status)) {
				DECLARE_UNICODE_STRING_SIZE(uClassKey, 256);

				rec->ClassGuidString = uGuid;
				if (InterlockedIncrement(&_numberofClasses) == 1)
					_driverObject->DriverExtension->AddDevice = _AddDevice;

				status = RtlUnicodeStringPrintf(&uClassKey, L"\\Registry\\Machine\\SYSTEM\\ControlSet%.3u\\Control\\Class\\%wZ", _currentControlSet, &uGuid);
				if (NT_SUCCESS(status)) {
					if (_versionInfo.dwMajorVersion >= 6) {
						status = RegManKeyRegister(&uClassKey, &rec->KeyRecord);
						if (NT_SUCCESS(status)) {
							UNICODE_STRING uValueName;

							RtlInitUnicodeString(&uValueName, (!UpperFilter) ? L"LowerFilters" : L"UpperFilters");
							status = RegManKeyValueAdd(rec->KeyRecord, &uValueName, NULL, 0, REG_NONE, &rec->ValueRecord);
							if (NT_SUCCESS(status)) {
								REGMAN_CALLBACKS callbacks;

								memset(&callbacks, 0, sizeof(callbacks));
								callbacks.QueryValue = _QueryCallback;
								callbacks.SetValue = _SetCallback;
								callbacks.Context = rec;
								status = RegManValueCallbacksRegister(rec->ValueRecord, &callbacks, &rec->CallbackHandle);
								if (!NT_SUCCESS(status))
									RegManKeyValueDelete(rec->ValueRecord);
							}

							if (!NT_SUCCESS(status))
								RegManKeyUnregister(rec->KeyRecord);
						}
					} else status = _InstallUninstallFilterXP(&rec->ClassGuidString, UpperFilter, Beginning, TRUE);
				
					if (NT_SUCCESS(status))
						HashTableInsert(targetTable, &rec->HashItem, ClassGuid);
				}

				if (!NT_SUCCESS(status)) {
					if (InterlockedDecrement(&_numberofClasses) == 0)
						_driverObject->DriverExtension->AddDevice = NULL;

					RtlFreeUnicodeString(&uGuid);
				}
			}

			if (!NT_SUCCESS(status))
				HeapMemoryFree(rec);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else status = STATUS_ALREADY_REGISTERED;

	ExReleaseResourceLite(&_classGuidsLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS PDWClassUnregister(PGUID ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	PHASH_ITEM h = NULL;
	PHASH_TABLE targetTable = NULL;
	PDEVICE_CLASS_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ClassGuid=0x%p; UpperFilter=%u; Beginning=%u", ClassGuid, UpperFilter, Beginning);

	targetTable = (UpperFilter) ? _upperClassGuidTable : _lowerClassGuidTable;
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&_classGuidsLock, TRUE);
	h = HashTableDelete(targetTable, ClassGuid);
	if (h != NULL) {
		rec = CONTAINING_RECORD(h, DEVICE_CLASS_WATCH_RECORD, HashItem);
		if (InterlockedDecrement(&_numberofClasses) == 0)
			_driverObject->DriverExtension->AddDevice = NULL;

		_FreeFunction(h);
		status = STATUS_SUCCESS;
	} else status = STATUS_NOT_FOUND;

	ExReleaseResourceLite(&_classGuidsLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS PDWClassEnumerate(PIOCTL_IRPMNDRV_CLASS_WATCH_OUTPUT Buffer, SIZE_T Length, PSIZE_T ReturnLength, KPROCESSOR_MODE AccessMode)
{
	ULONG index = 0;
	SIZE_T requiredLength = 0;
	HASH_TABLE_ITERATOR it;
	PHASH_ITEM h = NULL;
	PDEVICE_CLASS_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Buffer=0x%p; Length=%Iu; ReturnLength=0x%p; AccessMode=%u", Buffer, Length, ReturnLength, AccessMode);

	status = STATUS_SUCCESS;
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&_classGuidsLock, TRUE);
	requiredLength = (_lowerClassGuidTable->NumberOfItems + _upperClassGuidTable->NumberOfItems)*sizeof(CLASS_WATCH_ENTRY) + FIELD_OFFSET(IOCTL_IRPMNDRV_CLASS_WATCH_ENUM_OUTPUT, Entries);
	if (requiredLength <= Length) {
		if (AccessMode == UserMode) {
			__try {
				ProbeForWrite(Buffer, requiredLength, 1);
				Buffer->Count = _lowerClassGuidTable->NumberOfItems + _upperClassGuidTable->NumberOfItems;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				status = GetExceptionCode();
			}
		} else Buffer->Count = _lowerClassGuidTable->NumberOfItems + _upperClassGuidTable->NumberOfItems;
		
		if (NT_SUCCESS(status)) {
			if (HashTableGetFirst(_lowerClassGuidTable, &it)) {
				do {
					h = HashTableIteratorGetData(&it);
					rec = CONTAINING_RECORD(h, DEVICE_CLASS_WATCH_RECORD, HashItem);
					if (AccessMode == UserMode) {
						__try {
							Buffer->Entries[index].ClassGuid = rec->ClassGuid;
							Buffer->Entries[index].Flags = rec->Flags;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							status = GetExceptionCode();
						}
					} else {
						Buffer->Entries[index].ClassGuid = rec->ClassGuid;
						Buffer->Entries[index].Flags = rec->Flags;
					}

					++index;
				} while (NT_SUCCESS(status) && HashTableGetNext(&it));

				HashTableIteratorFinit(&it);
			}

			if (NT_SUCCESS(status)) {
				if (HashTableGetFirst(_upperClassGuidTable, &it)) {
					do {
						h = HashTableIteratorGetData(&it);
						rec = CONTAINING_RECORD(h, DEVICE_CLASS_WATCH_RECORD, HashItem);
						if (AccessMode == UserMode) {
							__try {
								Buffer->Entries[index].ClassGuid = rec->ClassGuid;
								Buffer->Entries[index].Flags = rec->Flags;
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								status = GetExceptionCode();
							}
						} else {
							Buffer->Entries[index].ClassGuid = rec->ClassGuid;
							Buffer->Entries[index].Flags = rec->Flags;
						}

						++index;
					} while (NT_SUCCESS(status) && HashTableGetNext(&it));

					HashTableIteratorFinit(&it);
				}

				if (NT_SUCCESS(status))
					*ReturnLength = requiredLength;
			}
		}
	} else status = STATUS_BUFFER_TOO_SMALL;

	ExReleaseResourceLite(&_classGuidsLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x, *ReturnLength=%Iu", status, *ReturnLength);
	return status;
}


VOID PDWClassWatchesUnregister(VOID)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&_classGuidsLock, TRUE);
	HashTableClear(_upperClassGuidTable, TRUE);
	HashTableClear(_lowerClassGuidTable, TRUE);
	ExReleaseResourceLite(&_classGuidsLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

NTSTATUS PWDDriverNameRegister(PUNICODE_STRING Name, PDRIVER_MONITOR_SETTINGS Settings)
{
	PDRIVER_NAME_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Name=\"%wZ\"; Settings=0x%p", Name, Settings);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&_driverNamesLock, TRUE);
	if (StringHashTableGetUnicodeString(_driverNameTable, Name) == NULL) {
		rec = (PDRIVER_NAME_WATCH_RECORD)HeapMemoryAllocNonPaged(sizeof(DRIVER_NAME_WATCH_RECORD));
		if (rec != NULL) {
			rec->MonitorSettings = *Settings;
			status = StringHashTableInsertUnicodeString(_driverNameTable, Name, rec);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else status = STATUS_ALREADY_REGISTERED;

	ExReleaseResourceLite(&_driverNamesLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS PWDDriverNameUnregister(PUNICODE_STRING Name)
{
	PDRIVER_NAME_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Name=\"%wZ\"", Name);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&_driverNamesLock, TRUE);
	rec = (PDRIVER_NAME_WATCH_RECORD)StringHashTableDeleteUnicodeString(_driverNameTable, Name);
	if (rec != NULL) {
		HeapMemoryFree(rec);
		status = STATUS_SUCCESS;
	} else status = STATUS_NOT_FOUND;

	ExReleaseResourceLite(&_driverNamesLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS PWDDriverNameEnumerate(PIOCTL_IRPMNDRV_DRIVER_WATCH_ENUM_OUTPUT Buffer, SIZE_T Length, PSIZE_T ReturnLength, KPROCESSOR_MODE AccessMode)
{
	DRIVER_NAME_WATCH_ENUM_CONTEXT ctx;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Buffer=0x%p; Length=%Iu; Returnlength=0x%p; AccessMode=%u", Buffer, Length, ReturnLength, AccessMode);

	status = STATUS_SUCCESS;
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&_driverNamesLock, TRUE);
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

	ExReleaseResourceLite(&_driverNamesLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x, *ReturnLength=%Iu", status, *ReturnLength);
	return status;
}


/************************************************************************/
/*                INITIALIZATION AND FINALIZATION                       */
/************************************************************************/

NTSTATUS PWDModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	_versionInfo.dwOSVersionInfoSize = sizeof(_versionInfo);
	status = RtlGetVersion(&_versionInfo);
	if (NT_SUCCESS(status)) {
		status = _GetCurrentControlSetNumber();
		if (NT_SUCCESS(status)) {
			status = _CaptureServiceName(RegistryPath);
			if (NT_SUCCESS(status)) {
				_driverObject = DriverObject;
				status = ExInitializeResourceLite(&_classGuidsLock);
				if (NT_SUCCESS(status)) {
					status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _CompareFunction, _FreeFunction, &_lowerClassGuidTable);
					if (NT_SUCCESS(status)) {
						status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _CompareFunction, _FreeFunction, &_upperClassGuidTable);
						if (NT_SUCCESS(status)) {
							status = ExInitializeResourceLite(&_driverNamesLock);
							if (NT_SUCCESS(status)) {
								status = StringHashTableCreate(httNoSynchronization, 37, &_driverNameTable);
								if (!NT_SUCCESS(status))
									ExDeleteResourceLite(&_driverNamesLock);
							}

							if (!NT_SUCCESS(status))
								HashTableDestroy(_upperClassGuidTable);
						}

						if (!NT_SUCCESS(status))
							HashTableDestroy(_lowerClassGuidTable);
					}

					if (!NT_SUCCESS(status))
						ExDeleteResourceLite(&_classGuidsLock);
				}

				if (!NT_SUCCESS(status))
					_FreeServiceName();
			}
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID PWDModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(Context);

	StringHashTableDestroy(_driverNameTable);
	ExDeleteResourceLite(&_driverNamesLock);
	HashTableDestroy(_upperClassGuidTable);
	HashTableDestroy(_lowerClassGuidTable);
	ExDeleteResourceLite(&_classGuidsLock);
	_driverObject = NULL;
	_FreeServiceName();

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
