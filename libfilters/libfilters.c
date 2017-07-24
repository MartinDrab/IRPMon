
#include <ntifs.h>
#include <ntstrsafe.h>
#include "preprocessor.h"
#include "allocator.h"
#include "string-ref-table.h"
#include "multistring.h"
#include "regman.h"
#include "libfilters-internal.h"
#include "libfilters.h"



/************************************************************************/
/*                   MACRO DEFINITIONS                                  */
/************************************************************************/

#define LOWER_FILTERS_VALUE			L"LowerFilters"
#define UPPER_FILTERS_VALUE			L"UpperFilters"
#define CONTROL_SET_SELECT_KEY		L"\\Registry\\Machine\\SYSTEM\\Select"
#define CONTROL_SET_CURRENT_VALUE	L"Current"
#define IMAGE_PATH_VALUE			L"ImagePath"

/************************************************************************/
/*                  GLOBAL VARIABLES                                    */
/************************************************************************/


static STRING_REF_TABLE _lowerFilterTable;
static STRING_REF_TABLE _upperFilterTable;
static ULONG _currentControlSet = 1;


/************************************************************************/
/*                  HEPER FUNCTIONS                                     */
/************************************************************************/

static NTSTATUS _GetCurrentControlSetNumber(void)
{
	HANDLE hSelectKey = NULL;
	UNICODE_STRING uSelectKey;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	RtlInitUnicodeString(&uSelectKey, CONTROL_SET_SELECT_KEY);
	InitializeObjectAttributes(&oa, &uSelectKey, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenKey(&hSelectKey, KEY_QUERY_VALUE, &oa);
	if (NT_SUCCESS(status)) {
		ULONG retLength = 0;
		UNICODE_STRING uValueName;
		UCHAR kvpiStorage[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
		PKEY_VALUE_PARTIAL_INFORMATION kvpi = (PKEY_VALUE_PARTIAL_INFORMATION)kvpiStorage;

		RtlInitUnicodeString(&uValueName, CONTROL_SET_CURRENT_VALUE);
		status = ZwQueryValueKey(hSelectKey, &uValueName, KeyValuePartialInformation, kvpi, sizeof(kvpiStorage), &retLength);
		if (NT_SUCCESS(status)) {
			if (kvpi->DataLength == sizeof(ULONG))
				_currentControlSet = *(PULONG)kvpi->Data;
			else status = STATUS_INVALID_PARAMETER;
		}

		ZwClose(hSelectKey);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static VOID _FilterDriverReference(PFILTER_DRIVER_RECORD Record)
{
	InterlockedIncrement(&Record->ReferenceCount);

	return;
}


static VOID _FilterDriverDereference(PFILTER_DRIVER_RECORD Record)
{
	if (InterlockedDecrement(&Record->ReferenceCount) == 0) {
		if (Record->ValueRecord != NULL)
			RegManKeyValueDelete(Record->ValueRecord);

		if (Record->KeyRecord != NULL)
			RegManKeyUnregister(Record->KeyRecord);

		HeapMemoryFree(Record);
	}

	return;
}


static NTSTATUS _QueryCallback(_In_ PREGMAN_VALUE_INFO ValueInfo, _In_opt_ PVOID Context)
{
	size_t newSize = 0;
	BOOLEAN inserted = FALSE;
	ULONG tmpDataSize = 0;
	wchar_t *tmpData = NULL;
	PFILTER_DRIVER_RECORD fdr = (PFILTER_DRIVER_RECORD)Context;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ValueInfo=0x%p; Context=0x%p", ValueInfo, Context);

	tmpDataSize = ValueInfo->CurrentDataSize + 3 * sizeof(wchar_t) + fdr->ServiceName.Length;
	tmpData = (wchar_t *)ExAllocatePoolWithTag(PagedPool, tmpDataSize, 0);
	if (tmpData != NULL) {
		RtlSecureZeroMemory(tmpData, tmpDataSize);
		memcpy(tmpData, ValueInfo->CurrentData, ValueInfo->CurrentDataSize);
		inserted = _MultiStringInsert(tmpData, &fdr->ServiceName, fdr->First, &newSize);
		if (inserted) {
			if (ValueInfo->CurrentData != NULL)
				ExFreePoolWithTag(ValueInfo->CurrentData, 0);

			ValueInfo->CurrentData = tmpData;
			ValueInfo->CurrentDataSize = (ULONG)newSize;
			ValueInfo->CurrentType = REG_MULTI_SZ;
		}

		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _SetCallback(_In_ PREGMAN_VALUE_INFO ValueInfo, _In_opt_ PVOID Context)
{
	size_t newSize = 0;
	BOOLEAN removed = FALSE;
	ULONG tmpDataSize = 0;
	wchar_t *tmpData = NULL;
	PFILTER_DRIVER_RECORD fdr = (PFILTER_DRIVER_RECORD)Context;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ValueInfo=0x%p; Context=0x%p", ValueInfo, Context);

	tmpDataSize = ValueInfo->CurrentDataSize + 2 * sizeof(wchar_t);
	tmpData = (wchar_t *)ExAllocatePoolWithTag(PagedPool, tmpDataSize, 0);
	if (tmpData != NULL) {
		RtlSecureZeroMemory(tmpData, tmpDataSize);
		memcpy(tmpData, ValueInfo->CurrentData, ValueInfo->CurrentDataSize);
		removed = _MultiStringRemove(tmpData, &fdr->ServiceName, &newSize);
		if (removed) {
			if (ValueInfo->CurrentData != NULL)
				ExFreePoolWithTag(ValueInfo->CurrentData, 0);

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
/*                 PUBLIC FUNCTIONS                                     */
/************************************************************************/


NTSTATUS LibFiltersFilterInstall(_In_ PUNICODE_STRING ClassGuid, _In_ PUNICODE_STRING ServiceName, _In_ EFilterDriverType Type, _In_ BOOLEAN First)
{
	PSTRING_REF_TABLE targetTable = NULL;
	PFILTER_DRIVER_RECORD fdr = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ClassGuid=\"%wZ\"; ServiceName=\"%wZ\"; Type=%u; First=%u", ClassGuid, ServiceName, Type, First);

	fdr = (PFILTER_DRIVER_RECORD)HeapMemoryAllocNonPaged(sizeof(FILTER_DRIVER_RECORD) + ClassGuid->Length + ServiceName->Length);
	if (fdr != NULL) {
		DECLARE_UNICODE_STRING_SIZE(uClassKey, 255);

		fdr->ReferenceCount = 1;
		fdr->First = First;
		fdr->Type = Type;
		fdr->Item.Key.String.Length = ClassGuid->Length;
		fdr->Item.Key.String.MaximumLength = ClassGuid->Length;
		fdr->Item.Key.String.Buffer = (PWCH)(fdr + 1);
		memcpy(fdr->Item.Key.String.Buffer, ClassGuid->Buffer, fdr->Item.Key.String.Length);
		fdr->ServiceName.Length = ServiceName->Length;
		fdr->ServiceName.MaximumLength = fdr->ServiceName.Length;
		fdr->ServiceName.Buffer = fdr->Item.Key.String.Buffer + fdr->Item.Key.String.Length / sizeof(wchar_t);
		memcpy(fdr->ServiceName.Buffer, ServiceName->Buffer, fdr->ServiceName.Length);
		targetTable = (Type == fdtLower) ? &_lowerFilterTable : &_upperFilterTable;
		status = (StringRefTableInsert(targetTable, &fdr->Item) == fdr) ? STATUS_SUCCESS : STATUS_OBJECT_NAME_COLLISION;
		if (NT_SUCCESS(status)) {
			status = RtlUnicodeStringPrintf(&uClassKey, L"\\Registry\\Machine\\SYSTEM\\ControlSet%.3u\\Control\\Class\\%wZ", _currentControlSet, ClassGuid);
			if (NT_SUCCESS(status))
				status = RegManKeyRegister(&uClassKey, &fdr->KeyRecord);

			if (NT_SUCCESS(status)) {
				UNICODE_STRING uValueName;

				RtlInitUnicodeString(&uValueName, (Type == fdtLower) ? LOWER_FILTERS_VALUE : UPPER_FILTERS_VALUE);
				status = RegManKeyValueAdd(fdr->KeyRecord, &uValueName, NULL, 0, REG_NONE, &fdr->ValueRecord);
				if (NT_SUCCESS(status))
					status = RegManValueCallbacksRegister(fdr->ValueRecord, _QueryCallback, _SetCallback, fdr, &fdr->CallbackHandle);					
			}
		
			if (!NT_SUCCESS(status)) {
				StringRefTableDelete(targetTable, &fdr->Item.Key.String);
				_FilterDriverDereference(fdr);
			}
		}

		_FilterDriverDereference(fdr);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID LibFiltersFilterUninstall(_In_ PUNICODE_STRING ClassGuid, EFilterDriverType Type)
{
	PSTRING_REF_TABLE targetTable = NULL;
	DEBUG_ENTER_FUNCTION("ClassGuid=\"%wZ\"; Type=%u", ClassGuid, Type);

	targetTable = (Type == fdtLower) ? &_lowerFilterTable : &_upperFilterTable;
	StringRefTableDeleteDereference(targetTable, ClassGuid);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                 INITIALIZATION AND FINALIZATION                      */
/************************************************************************/


NTSTATUS LibFiltersModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_opt_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	status = _GetCurrentControlSetNumber();
	if (NT_SUCCESS(status)) {
		status = StringRefTableInit(FIELD_OFFSET(FILTER_DRIVER_RECORD, Item), 37, _FilterDriverReference, _FilterDriverDereference, &_lowerFilterTable);
		if (NT_SUCCESS(status)) {
			status = StringRefTableInit(FIELD_OFFSET(FILTER_DRIVER_RECORD, Item), 37, _FilterDriverReference, _FilterDriverDereference, &_upperFilterTable);
			if (!NT_SUCCESS(status))
				StringRefTableDestroy(&_lowerFilterTable);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID LibFiltersModuleFinit(_In_ PDRIVER_OBJECT DriverObject, _In_opt_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	StringRefTableDestroy(&_upperFilterTable);
	StringRefTableDestroy(&_lowerFilterTable);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
