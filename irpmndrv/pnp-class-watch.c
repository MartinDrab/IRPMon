
#include <ntifs.h>
#include <ntstrsafe.h>
#include <fltkernel.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hash_table.h"
#include "utils.h"
#include "multistring.h"
#include "pnp-driver-watch.h"
#include "regman.h"
#include "pnp-class-watch.h"


/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

static PHASH_TABLE _lowerClassGuidTable = NULL;
static PHASH_TABLE _upperClassGuidTable = NULL;
static EX_PUSH_LOCK _classGuidsLock;
static volatile LONG _numberofClasses = 0;
static RTL_OSVERSIONINFOW _versionInfo;
static PDRIVER_OBJECT _driverObject = NULL;
static UNICODE_STRING _driverServiceName;
static ULONG _currentControlSet = 1;


/************************************************************************/
/*                    HELPER FUNCTIONS                                  */
/************************************************************************/


static NTSTATUS _CaptureServiceName(PUNICODE_STRING RegistryPath)
{
	const wchar_t* wServiceName = NULL;
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
	}
	else status = STATUS_INSUFFICIENT_RESOURCES;

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


static NTSTATUS _AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject)
{
	PDEVICE_OBJECT deviceObject = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; PhysicalDeviceObject=0x%p", DriverObject, PhysicalDeviceObject);

	UNREFERENCED_PARAMETER(DriverObject);

	deviceObject = PhysicalDeviceObject;
	do {
		status = PDWCheckDriver(deviceObject->DriverObject, deviceObject);
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


static BOOLEAN _CompareFunction(PHASH_ITEM Item, PVOID Key)
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
				kvfi = HeapMemoryAllocPaged(kvfiLen);
				if (kvfi != NULL) {
					RtlSecureZeroMemory(kvfi, kvfiLen);
					if (status == STATUS_OBJECT_NAME_NOT_FOUND)
						status = STATUS_SUCCESS;

					if (status == STATUS_BUFFER_TOO_SMALL)
						status = ZwQueryValueKey(keyHandle, &uValueName, KeyValueFullInformation, kvfi, kvfiLen, &kvfiLen);

					if (NT_SUCCESS(status)) {
						size_t newSize = 0;
						wchar_t* data = (wchar_t*)((PUCHAR)kvfi + kvfi->DataOffset);

						if (Install) {
							if (_MultiStringExists(data, &_driverServiceName, NULL))
								status = STATUS_OBJECT_NAME_COLLISION;

							if (NT_SUCCESS(status))
								_MultiStringInsert(data, &_driverServiceName, Beginning, &newSize);
						}
						else status = _MultiStringRemove(data, &_driverServiceName, &newSize) ? STATUS_SUCCESS : STATUS_OBJECT_NAME_NOT_FOUND;


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
	wchar_t* tmpData = NULL;
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
	wchar_t* tmpData = NULL;
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


NTSTATUS CWRegister(PGUID ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	PHASH_ITEM h = NULL;
	UNICODE_STRING uGuid;
	PHASH_TABLE targetTable = NULL;
	PDEVICE_CLASS_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ClassGuid=0x%p; UpperFilter=%u; Beginning=%u", ClassGuid, UpperFilter, Beginning);

	targetTable = (UpperFilter) ? _upperClassGuidTable : _lowerClassGuidTable;
	FltAcquirePushLockExclusive(&_classGuidsLock);
	h = HashTableGet(targetTable, ClassGuid);
	if (h == NULL) {
		rec = HeapMemoryAllocPaged(sizeof(DEVICE_CLASS_WATCH_RECORD));
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

	FltReleasePushLock(&_classGuidsLock);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS CWUnregister(PGUID ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	PHASH_ITEM h = NULL;
	PHASH_TABLE targetTable = NULL;
	PDEVICE_CLASS_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ClassGuid=0x%p; UpperFilter=%u; Beginning=%u", ClassGuid, UpperFilter, Beginning);

	targetTable = (UpperFilter) ? _upperClassGuidTable : _lowerClassGuidTable;
	FltAcquirePushLockExclusive(&_classGuidsLock);
	h = HashTableDelete(targetTable, ClassGuid);
	if (h != NULL) {
		rec = CONTAINING_RECORD(h, DEVICE_CLASS_WATCH_RECORD, HashItem);
		if (InterlockedDecrement(&_numberofClasses) == 0)
			_driverObject->DriverExtension->AddDevice = NULL;

		_FreeFunction(h);
		status = STATUS_SUCCESS;
	} else status = STATUS_NOT_FOUND;

	FltReleasePushLock(&_classGuidsLock);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS CWEnumerate(PIOCTL_IRPMNDRV_CLASS_WATCH_OUTPUT Buffer, SIZE_T Length, PSIZE_T ReturnLength, KPROCESSOR_MODE AccessMode)
{
	ULONG index = 0;
	SIZE_T requiredLength = 0;
	HASH_TABLE_ITERATOR it;
	PHASH_ITEM h = NULL;
	PDEVICE_CLASS_WATCH_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Buffer=0x%p; Length=%Iu; ReturnLength=0x%p; AccessMode=%u", Buffer, Length, ReturnLength, AccessMode);

	status = STATUS_SUCCESS;
	FltAcquirePushLockShared(&_classGuidsLock);
	requiredLength = (_lowerClassGuidTable->NumberOfItems + _upperClassGuidTable->NumberOfItems) * sizeof(CLASS_WATCH_ENTRY) + FIELD_OFFSET(IOCTL_IRPMNDRV_CLASS_WATCH_ENUM_OUTPUT, Entries);
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

	FltReleasePushLock(&_classGuidsLock);

	DEBUG_EXIT_FUNCTION("0x%x, *ReturnLength=%Iu", status, *ReturnLength);
	return status;
}


void CWClear(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	FltAcquirePushLockExclusive(&_classGuidsLock);
	HashTableClear(_upperClassGuidTable, TRUE);
	HashTableClear(_lowerClassGuidTable, TRUE);
	FltReleasePushLock(&_classGuidsLock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                INITIALIZATION AND FINALIZATION                       */
/************************************************************************/


NTSTATUS CWModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	status = _CaptureServiceName(RegistryPath);
	if (NT_SUCCESS(status)) {
		_versionInfo.dwOSVersionInfoSize = sizeof(_versionInfo);
		status = RtlGetVersion(&_versionInfo);
		if (NT_SUCCESS(status)) {
			status = UtilsGetCurrentControlSetNumber(&_currentControlSet);
			if (NT_SUCCESS(status)) {
				_driverObject = DriverObject;
				FltInitializePushLock(&_classGuidsLock);
				status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _CompareFunction, _FreeFunction, &_lowerClassGuidTable);
				if (NT_SUCCESS(status)) {
					status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _CompareFunction, _FreeFunction, &_upperClassGuidTable);
					if (!NT_SUCCESS(status))
						HashTableDestroy(_lowerClassGuidTable);
				}
			}
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void CWModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	HashTableDestroy(_upperClassGuidTable);
	HashTableDestroy(_lowerClassGuidTable);
	_driverObject = NULL;
	_FreeServiceName();

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
