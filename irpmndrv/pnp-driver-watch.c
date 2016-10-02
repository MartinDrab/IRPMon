
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hash_table.h"
#include "string-hash-table.h"
#include "ioctls.h"
#include "kernel-shared.h"
#include "hook.h"
#include "req-queue.h"
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
static UNICODE_STRING _driverServiceName;
static volatile LONG _numberofClasses = 0;
static PDRIVER_OBJECT _driverObject = NULL;

/************************************************************************/
/*                    HELPER FUNCTIONS                                  */
/************************************************************************/


static NTSTATUS _AppendFilter(BOOLEAN Beginning, PWCHAR Filters, PSIZE_T ResultLength)
{
	SIZE_T len = 0;
	UNICODE_STRING uFilter;
	PWCHAR tmp = Filters;
	SIZE_T dataLength = sizeof(WCHAR);
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Beginning=%u; Filters=\"%S\"; ResultLength=0x%p", Beginning, Filters, ResultLength);

	status = STATUS_SUCCESS;
	while (*tmp != L'\0') {
		len = wcslen(tmp);
		dataLength += ((len + 1)*sizeof(WCHAR));
		RtlInitUnicodeString(&uFilter, tmp);
		if (RtlEqualUnicodeString(&uFilter, &_driverServiceName, TRUE)) {
			status = STATUS_OBJECT_NAME_COLLISION;
			break;
		}

		tmp += (len + 1);
	}

	if (NT_SUCCESS(status)) {
		if (Beginning) {
			memmove(Filters + _driverServiceName.Length / sizeof(WCHAR) + 1, Filters, dataLength);
			memcpy(Filters, _driverServiceName.Buffer, _driverServiceName.Length);
			Filters[_driverServiceName.Length / sizeof(WCHAR)] = L'\0';
		} else {
			memcpy(Filters + dataLength / sizeof(WCHAR) - 1, _driverServiceName.Buffer, _driverServiceName.Length);
			Filters[dataLength / sizeof(WCHAR) - 1 + _driverServiceName.Length / sizeof(WCHAR)] = L'\0';
			Filters[dataLength / sizeof(WCHAR) + _driverServiceName.Length / sizeof(WCHAR)] = L'\0';
		}

		*ResultLength = dataLength + _driverServiceName.Length + sizeof(WCHAR);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *ResultLength=%Iu", status, *ResultLength);
	return status;
}


static NTSTATUS _DeleteFilter(PWCHAR Filters, PSIZE_T ResultLength)
{
	UNICODE_STRING uFilter;
	SIZE_T len = 0;
	PWCHAR ours = NULL;
	PWCHAR tmp = Filters;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Filters=\"%S\"; ResultLength=0x%p", Filters, ResultLength);

	status = STATUS_SUCCESS;
	while (*tmp != L'\0') {
		len = wcslen(tmp);
		RtlInitUnicodeString(&uFilter, tmp);
		if (RtlEqualUnicodeString(&uFilter, &_driverServiceName, TRUE))
			ours = tmp;

		tmp += (len + 1);
	}

	if (ours != NULL) {
		*ResultLength = (tmp - Filters + 1)*sizeof(WCHAR) - _driverServiceName.Length - sizeof(WCHAR);
		memmove(ours, ours + _driverServiceName.Length / sizeof(WCHAR) + 1, sizeof(WCHAR)*(tmp - ours - _driverServiceName.Length / 2));
		ours[_driverServiceName.Length / sizeof(WCHAR) + 1] = L'\0';
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *ResultLength=%Iu", status, *ResultLength);
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
		len = wcslen(String)*sizeof(WCHAR);
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


static NTSTATUS _RegisterUnregisterFilter(BOOLEAN Beginning, BOOLEAN UpperFilter, BOOLEAN Register, PUNICODE_STRING ClassGuid)
{
	PWCHAR dataBuffer = NULL;
	SIZE_T dataLength = 0;
	ULONG returnLength = 0;
	PKEY_VALUE_PARTIAL_INFORMATION kvpi = NULL;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uValueName;
	UNICODE_STRING uKeyName;
	HANDLE classesKey = NULL;
	HANDLE classKey = NULL;
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
	DEBUG_ENTER_FUNCTION("Beginning=%u; UpperFilter=%u; Register=%u; ClassGuid=\"%wZ\"", Beginning, UpperFilter, Register, ClassGuid);

	RtlInitUnicodeString(&uKeyName, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Class");
	InitializeObjectAttributes(&oa, &uKeyName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&classesKey, KEY_READ, &oa);
	if (NT_SUCCESS(status)) {
		InitializeObjectAttributes(&oa, ClassGuid, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, classesKey, NULL);
		status = ZwOpenKey(&classKey, KEY_QUERY_VALUE | KEY_SET_VALUE, &oa);
		if (NT_SUCCESS(status)) {
			RtlInitUnicodeString(&uValueName, (UpperFilter) ? L"UpperFilters" : L"LowerFilters");
			status = ZwQueryValueKey(classKey, &uValueName, KeyValuePartialInformation, NULL, 0, &returnLength);
			if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
				dataLength = _driverServiceName.Length + 2 * sizeof(WCHAR);
				dataBuffer = (PWCHAR)HeapMemoryAllocPaged(dataLength);
				if (dataBuffer != NULL) {
					RtlSecureZeroMemory(dataBuffer, dataLength);
					status = STATUS_SUCCESS;
				} else status = STATUS_INSUFFICIENT_RESOURCES;
			} else if (status == STATUS_BUFFER_TOO_SMALL) {
				dataLength = returnLength + _driverServiceName.Length + 2*sizeof(WCHAR);
				kvpi = (PKEY_VALUE_PARTIAL_INFORMATION)HeapMemoryAllocPaged(sizeof(KEY_VALUE_PARTIAL_INFORMATION) + dataLength);
				if (kvpi != NULL) {
					dataBuffer = (PWCHAR)kvpi->Data;
					RtlSecureZeroMemory(dataBuffer, dataLength);
					status = ZwQueryValueKey(classKey, &uValueName, KeyValuePartialInformation, kvpi, returnLength, &returnLength);					
					if (!NT_SUCCESS(status))
						HeapMemoryFree(kvpi);
				} else status = STATUS_INSUFFICIENT_RESOURCES;
			}
			
			if (NT_SUCCESS(status)) {
				if (Register)
					status = _AppendFilter(Beginning, dataBuffer, &dataLength);
				else status = _DeleteFilter(dataBuffer, &dataLength);

				if (NT_SUCCESS(status)) {
					if (dataLength > 2 * sizeof(WCHAR))
						status = ZwSetValueKey(classKey, &uValueName, 0, REG_MULTI_SZ, dataBuffer, (ULONG)dataLength);
					else status = ZwDeleteValueKey(classKey, &uValueName);
				}

				if (kvpi != NULL)
					HeapMemoryFree(kvpi);
				else HeapMemoryFree(dataBuffer);
			}

			ZwClose(classKey);
		} else {
			DEBUG_PRINT_LOCATION("ZwOpenKey(Class key): 0x%x", status);
		}

		ZwClose(classesKey);
	} else {
		DEBUG_PRINT_LOCATION("ZwOpenKey(Classes key): 0x%x", status);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
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
					status = HookDriverObject(DriverObject, &nameRecord->MonitorSettings, &hookRecord);
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

	DEBUG_EXIT_FUNCTION("0x%x", status)
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


static VOID _FreeFunction(PHASH_ITEM Item)
{
	PDEVICE_CLASS_WATCH_RECORD rec = CONTAINING_RECORD(Item, DEVICE_CLASS_WATCH_RECORD, HashItem);
	DEBUG_ENTER_FUNCTION("Item=0x%p", Item);

	_RegisterUnregisterFilter((rec->Flags & CLASS_WATCH_FLAG_BEGINNING) != 0, (rec->Flags & CLASS_WATCH_FLAG_UPPERFILTER) != 0, FALSE, &rec->ClassGuidString);
	RtlFreeUnicodeString(&rec->ClassGuidString);
	HeapMemoryFree(rec);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
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
			rec->ClassGuid = *ClassGuid;
			rec->Flags = 0;
			if (UpperFilter)
				rec->Flags |= CLASS_WATCH_FLAG_UPPERFILTER;

			if (Beginning)
				rec->Flags |= CLASS_WATCH_FLAG_BEGINNING;

			RtlSecureZeroMemory(&uGuid, sizeof(uGuid));
			status = RtlStringFromGUID(ClassGuid, &uGuid);
			if (NT_SUCCESS(status)) {
				rec->ClassGuidString = uGuid;
				if (InterlockedIncrement(&_numberofClasses) == 1)
					_driverObject->DriverExtension->AddDevice = _AddDevice;

				status = _RegisterUnregisterFilter(Beginning, UpperFilter, TRUE, &uGuid);
				if (NT_SUCCESS(status))
					HashTableInsert(targetTable, &rec->HashItem, ClassGuid);

				if (!NT_SUCCESS(status)) {
					if (InterlockedDecrement(&_numberofClasses) == 0)
						_driverObject->DriverExtension->AddDevice = NULL;
				}

				if (!NT_SUCCESS(status))
					RtlFreeUnicodeString(&uGuid);
			}

			if (!NT_SUCCESS(status))
				HeapMemoryFree(rec);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else status = STATUS_ALREADY_REGISTERED;

	ExReleaseResourceLite(&_classGuidsLock);
	KeLeaveCriticalRegion();

	DEBUG_ENTER_FUNCTION("0x%x", status);
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
	h = HashTableGet(targetTable, ClassGuid);
	if (h != NULL) {
		rec = CONTAINING_RECORD(h, DEVICE_CLASS_WATCH_RECORD, HashItem);
		if (InterlockedDecrement(&_numberofClasses) == 0)
			_driverObject->DriverExtension->AddDevice = NULL;

		status = _RegisterUnregisterFilter(Beginning, UpperFilter, FALSE, &rec->ClassGuidString);
		if (NT_SUCCESS(status)) {
			HashTableDelete(targetTable, ClassGuid);
			_FreeFunction(h);
		}

		if (!NT_SUCCESS(status)) {
			if (InterlockedIncrement(&_numberofClasses) == 1)
				_driverObject->DriverExtension->AddDevice = _AddDevice;
		}
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
	requiredLength = (_lowerClassGuidTable->NumberOfItems + _upperClassGuidTable->NumberOfItems - 1)*sizeof(CLASS_WATCH_ENTRY) + sizeof(IOCTL_IRPMNDRV_CLASS_WATCH_ENUM_OUTPUT);
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

NTSTATUS PWDModuleInit(PDRIVER_OBJECT DriverObject, PVOID Context)
{
	PWCH tmp = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; Context=0x%p", DriverObject, Context);

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
					if (NT_SUCCESS(status)) {
						_driverServiceName = *(PUNICODE_STRING)Context;
						_driverServiceName.Buffer += (_driverServiceName.Length / sizeof(WCHAR));
						_driverServiceName.Length = 0;
						do {
							--_driverServiceName.Buffer;
							_driverServiceName.Length += sizeof(WCHAR);
						} while (*_driverServiceName.Buffer != L'\\');

						++_driverServiceName.Buffer;
						_driverServiceName.Length -= sizeof(WCHAR);
						tmp = (PWCH)HeapMemoryAllocPaged(_driverServiceName.Length);
						if (tmp != NULL) {
							memcpy(tmp, _driverServiceName.Buffer, _driverServiceName.Length);
							_driverServiceName.Buffer = tmp;
							_driverServiceName.MaximumLength = _driverServiceName.Length;
						} else status = STATUS_INSUFFICIENT_RESOURCES;
					}

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

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID PWDModuleFinit(PDRIVER_OBJECT DriverObject, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; Context=0x%p", DriverObject, Context);

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Context);

	HeapMemoryFree(_driverServiceName.Buffer);
	StringHashTableDestroy(_driverNameTable);
	ExDeleteResourceLite(&_driverNamesLock);
	HashTableDestroy(_upperClassGuidTable);
	HashTableDestroy(_lowerClassGuidTable);
	ExDeleteResourceLite(&_classGuidsLock);
	_driverObject = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
