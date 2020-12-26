
#include <ntifs.h>
#include <fltKernel.h>
#include "preprocessor.h"
#include "allocator.h"
#include "string-ref-table.h"
#include "key-record.h"
#include "value-record.h"
#include "registry-callback.h"


/************************************************************************/
/*                 GLOBAL VARIABLE                                      */
/************************************************************************/

static LARGE_INTEGER _registryCookie;
static STRING_REF_TABLE _keyTable;
static LIST_ENTRY _rawCallbackList;
static EX_PUSH_LOCK _rawCallbackLock;
static RTL_OSVERSIONINFOW _versionInfo;

/************************************************************************/
/*                        HELPER FUNCTIONS                              */
/************************************************************************/

static NTSTATUS _GetObjectName(_In_ PVOID Object, _Out_ POBJECT_NAME_INFORMATION *NameInfo)
{
	ULONG oniLen = 0;
	POBJECT_NAME_INFORMATION oni = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	status = ObQueryNameString(Object, NULL, 0, &oniLen);
	if (status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL) {
		oniLen += sizeof(OBJECT_NAME_INFORMATION);
		oni = (POBJECT_NAME_INFORMATION)HeapMemoryAllocPaged(oniLen);
		if (oni != NULL) {
			status = ObQueryNameString(Object, oni, oniLen, &oniLen);
			if (NT_SUCCESS(status))
				*NameInfo = oni;

			if (!NT_SUCCESS(status))
				HeapMemoryFree(oni);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	if (!NT_SUCCESS(status))
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "Failed to retrieve name for 0x%p: 0x%x\n", Object, status);

	return status;
}


static NTSTATUS _GetKeyRecord(_In_ REG_NOTIFY_CLASS OpType, _In_opt_ PVOID Data, _Out_ PREGMAN_KEY_RECORD *Record)
{
	PVOID keyObject = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	POBJECT_NAME_INFORMATION nameInfo = NULL;

	status = STATUS_SUCCESS;
	if (Data != NULL) {
		switch (OpType) {
			case RegNtPreDeleteValueKey:
				keyObject = ((PREG_DELETE_VALUE_KEY_INFORMATION)Data)->Object;
				break;
			case RegNtPreSetValueKey:
				keyObject = ((PREG_SET_VALUE_KEY_INFORMATION)Data)->Object;
				break;
			case RegNtPreQueryValueKey:
				keyObject = ((PREG_QUERY_VALUE_KEY_INFORMATION)Data)->Object;
				break;
			case RegNtPreEnumerateValueKey:
				keyObject = ((PREG_ENUMERATE_VALUE_KEY_INFORMATION)Data)->Object;
				break;
			case RegNtPreQueryMultipleValueKey:
				keyObject = ((PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION)Data)->Object;
				break;
			case RegNtPreQueryKey:
				keyObject = ((PREG_QUERY_KEY_INFORMATION)Data)->Object;
				break;
			case RegNtPostDeleteValueKey:
				keyObject = ((PREG_POST_OPERATION_INFORMATION)Data)->Object;
				break;
			default:
				break;
		}

		if (keyObject != NULL) {
			status = _GetObjectName(keyObject, &nameInfo);
			if (NT_SUCCESS(status)) {
				PSTRING_REF_ITEM sri = NULL;

				sri = StringRefTableGet(&_keyTable, &nameInfo->Name);
				if (sri != NULL)
					*Record = CONTAINING_RECORD(sri, REGMAN_KEY_RECORD, Item);

				HeapMemoryFree(nameInfo);
			}

			status = STATUS_SUCCESS;
		}
	}

	return status;
}


static NTSTATUS _RegistryCallback(_In_ PVOID Context, _In_opt_ PVOID Argument1, _In_opt_ PVOID Argument2)
{
	BOOLEAN emulated = FALSE;
	PRAW_CALLBACK_RECORD rec = NULL;
	PREGMAN_KEY_RECORD keyRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	REG_NOTIFY_CLASS opType = (REG_NOTIFY_CLASS)PtrToUlong(Argument1);

	UNREFERENCED_PARAMETER(Context);
	status = STATUS_SUCCESS;
	FltAcquirePushLockShared(&_rawCallbackLock);
	rec = CONTAINING_RECORD(_rawCallbackList.Flink, RAW_CALLBACK_RECORD, Entry);
	while (&rec->Entry != &_rawCallbackList) {
		status = rec->Callback(rec->Context, Argument1, Argument2);
		if (!NT_SUCCESS(status))
			break;

		rec = CONTAINING_RECORD(rec->Entry.Flink, RAW_CALLBACK_RECORD, Entry);
	}
	
	FltReleasePushLock(&_rawCallbackLock);
	if (NT_SUCCESS(status)) {
		status = _GetKeyRecord(opType, Argument2, &keyRecord);
		if (NT_SUCCESS(status) && keyRecord != NULL) {
			switch (opType) {
				case RegNtPreDeleteValueKey:
					status = KeyRecordOnDeleteValue(keyRecord, Argument2, &emulated);
					break;
				case RegNtPreSetValueKey:
					status = KeyRecordOnSetValue(keyRecord, Argument2, &emulated);
					break;
				case RegNtPreQueryValueKey:
					status = KeyRecordOnQueryValue(keyRecord, Argument2, &emulated);
					break;
				case RegNtPreQueryMultipleValueKey:
					status = KeyRecordQueryMultipleValues(keyRecord, Argument2, &emulated);
					break;
				case RegNtPreEnumerateValueKey:
					status = KeyRecordOnEnumValue(keyRecord, Argument2, &emulated);
					break;
				case RegNtPreQueryKey:
					status = KeyRecordOnQuery(keyRecord, Argument2, &emulated);
					break;
				case RegNtPostDeleteValueKey:
					status = KeyRecordOnPostOperation(keyRecord, (PREG_POST_OPERATION_INFORMATION)Argument2, &emulated);
					break;
				default:
					break;
			}
	
			if (NT_SUCCESS(status) && emulated)
				status = STATUS_CALLBACK_BYPASS;

			KeyRecordDereference(keyRecord);
		}
	}

	return status;
}


/************************************************************************/
/*                         PUBLIC FUNCTION                              */
/************************************************************************/


NTSTATUS RegCallbackKeyRegister(_In_ PUNICODE_STRING KeyName, PREGMAN_KEY_RECORD *KeyRecord)
{
	PREGMAN_KEY_RECORD insertedRecord = NULL;
	PREGMAN_KEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyName=\"%wZ\"; KeyRecord=0x%p", KeyName, KeyRecord);

	status = KeyRecordFromKey(KeyName, &tmpRecord);
	if (NT_SUCCESS(status)) {
		insertedRecord = (PREGMAN_KEY_RECORD)StringRefTableInsert(&_keyTable, &tmpRecord->Item);
		if (insertedRecord != tmpRecord) {
			KeyRecordDereference(tmpRecord);
			tmpRecord = insertedRecord;
		}
		
		*KeyRecord = tmpRecord;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *KeyRecord=0x%p", status, *KeyRecord);
	return status;
}


NTSTATUS RegCallbackKeyUnregister(PREGMAN_KEY_RECORD KeyRecord)
{
	PREGMAN_KEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyRecord=0x%p", KeyRecord);

	tmpRecord = (PREGMAN_KEY_RECORD)StringRefTableDelete(&_keyTable, &KeyRecord->Item.Key.String);
	status = (tmpRecord != NULL) ? STATUS_SUCCESS : STATUS_NOT_FOUND;
	if (NT_SUCCESS(status)) {
		ASSERT(KeyRecord == tmpRecord);
		KeyRecordDereference(tmpRecord);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS RegRawCallbackRegister(EX_CALLBACK_FUNCTION *Callback, void* Context, PHANDLE Handle)
{
	PRAW_CALLBACK_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Callback=0x%p; Context=0x%p; Handle=0x%p", Callback, Context, Handle);

	rec = HeapMemoryAllocPaged(sizeof(RAW_CALLBACK_RECORD));
	if (rec != NULL) {
		InitializeListHead(&rec->Entry);
		rec->Callback = Callback;
		rec->Context = Context;
		FltAcquirePushLockExclusive(&_rawCallbackLock);
		InsertTailList(&_rawCallbackList, &rec->Entry);
		FltReleasePushLock(&_rawCallbackLock);
		*Handle = rec;
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void RegRawCallbackUnregister(HANDLE Handle)
{
	PRAW_CALLBACK_RECORD rec = NULL;
	DEBUG_ENTER_FUNCTION("Handle=0x%p", Handle);

	rec = (PRAW_CALLBACK_RECORD)Handle;
	FltAcquirePushLockExclusive(&_rawCallbackLock);
	RemoveEntryList(&rec->Entry);
	FltReleasePushLock(&_rawCallbackLock);
	HeapMemoryFree(rec);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                     INITIALIZATION AND FINALIZATION                  */
/************************************************************************/


NTSTATUS RegCallbackModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	InitializeListHead(&_rawCallbackList);
	FltInitializePushLock(&_rawCallbackLock);
	_versionInfo.dwOSVersionInfoSize = sizeof(_versionInfo);
	status = RtlGetVersion(&_versionInfo);
	if (NT_SUCCESS(status)) {
		status = StringRefTableInit(FIELD_OFFSET(REGMAN_KEY_RECORD, Item), 37, KeyRecordReference, KeyRecordDereference, &_keyTable);
		if (NT_SUCCESS(status)) {
			if (_versionInfo.dwMajorVersion >= 6)
				status = CmRegisterCallback(_RegistryCallback, &_keyTable, &_registryCookie);
			
			if (!NT_SUCCESS(status))
				StringRefTableDestroy(&_keyTable);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID RegCallbackModuleFinit(_In_ PDRIVER_OBJECT DriverObject, _In_opt_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context)
{
	PRAW_CALLBACK_RECORD rec = NULL;
	PRAW_CALLBACK_RECORD old = NULL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	if (_versionInfo.dwMajorVersion >= 6)
		CmUnRegisterCallback(_registryCookie);
	
	StringRefTableDestroy(&_keyTable);
	rec = CONTAINING_RECORD(_rawCallbackList.Flink, RAW_CALLBACK_RECORD, Entry);
	while (&rec->Entry != &_rawCallbackList) {
		old = rec;
		rec = CONTAINING_RECORD(rec->Entry.Flink, RAW_CALLBACK_RECORD, Entry);
		HeapMemoryFree(old);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
