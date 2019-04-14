
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "value-record.h"



/************************************************************************/
/*                    HELPER FUNCTIONS                                  */
/************************************************************************/


static VOID _ValueRecordDestroy(_In_ PREGMAN_VALUE_RECORD Record)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

	if (Record->DataSize > 0)
		ExFreePoolWithTag(Record->Data, 0);

	ExDeleteResourceLite(&Record->CallbackLock);
	ExDeleteResourceLite(&Record->Lock);
	HeapMemoryFree(Record);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _ValuePostRecordAlloc(PREGMAN_VALUE_RECORD ValueRecord, ERegManValuePostContextType Type, PREGMAN_VALUE_POST_CONTEXT *Record)
{
	PREGMAN_VALUE_POST_CONTEXT tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ValueRecord=0x%p; Type=%u; Record=0x%p", ValueRecord, Type, Record);

	tmpRecord = HeapMemoryAllocPaged(sizeof(REGMAN_VALUE_POST_CONTEXT));
	if (tmpRecord != NULL) {
		memset(tmpRecord, 0, sizeof(REGMAN_VALUE_POST_CONTEXT));
		tmpRecord->Type = Type;
		ValueRecordReference(ValueRecord);
		tmpRecord->ValueRecord = ValueRecord;
		*Record = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status, *Record);
	return status;
}


static NTSTATUS _ValueRecordCallbackDeleteInvoke(_In_ PREGMAN_VALUE_RECORD Record, _Out_ PBOOLEAN Delete)
{
	REGMAN_DELETE_INFO info;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREGMAN_VALUE_QUERY_CALLBACK_RECORD cr = NULL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Delete=0x%p", Record, Delete);

	status = STATUS_SUCCESS;
	info.ValueRecord = Record;
	info.StopEmulation = FALSE;
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&Record->CallbackLock, TRUE);
	cr = CONTAINING_RECORD(Record->CallbackList.Flink, REGMAN_VALUE_QUERY_CALLBACK_RECORD, ListEntry);
	while (&cr->ListEntry != &Record->CallbackList) {
		status = cr->Callbacks.DeleteValue(&info, cr->Callbacks.Context);
		if (!NT_SUCCESS(status))
			break;

		cr = CONTAINING_RECORD(cr->ListEntry.Flink, REGMAN_VALUE_QUERY_CALLBACK_RECORD, ListEntry);
	}

	ExReleaseResourceLite(&Record->CallbackLock);
	KeLeaveCriticalRegion();
	if (NT_SUCCESS(status))
		*Delete = info.StopEmulation;

	DEBUG_EXIT_FUNCTION("0x%x, *Delete=%u", status, *Delete);
	return status;
}


static NTSTATUS _ValueRecordCallbackSetInvoke(_In_ PREGMAN_VALUE_RECORD Record, _Inout_ PVOID *Data, _Inout_ PULONG DataSize, _Inout_ PULONG Type)
{
	PREGMAN_VALUE_QUERY_CALLBACK_RECORD cr = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	REGMAN_QUERY_INFO info;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Data=0x%p; DataSize=0x%p; Type=0x%p", Record, Data, DataSize, Type);

	info.ValueRecord = Record;
	info.CurrentDataSize = *DataSize;
	info.CurrentType = *Type;
	info.CurrentData = ExAllocatePoolWithTag(PagedPool, info.CurrentDataSize, 0);
	if (info.CurrentData != NULL) {
		status = STATUS_SUCCESS;
		memcpy(info.CurrentData, *Data, info.CurrentDataSize);
		KeEnterCriticalRegion();
		ExAcquireResourceSharedLite(&Record->CallbackLock, TRUE);
		cr = CONTAINING_RECORD(Record->CallbackList.Flink, REGMAN_VALUE_QUERY_CALLBACK_RECORD, ListEntry);
		while (&cr->ListEntry != &Record->CallbackList) {
			status = cr->Callbacks.SetValue(&info, cr->Callbacks.Context);
			if (!NT_SUCCESS(status))
				break;

			cr = CONTAINING_RECORD(cr->ListEntry.Flink, REGMAN_VALUE_QUERY_CALLBACK_RECORD, ListEntry);
		}

		ExReleaseResourceLite(&Record->CallbackLock);
		KeLeaveCriticalRegion();
		if (NT_SUCCESS(status)) {
			if (*Data != NULL)
				ExFreePoolWithTag(*Data, 0);

			*Data = info.CurrentData;
			*DataSize = info.CurrentDataSize;
			*Type = info.CurrentType;
		}

		if (!NT_SUCCESS(status) && info.CurrentData != NULL)
			ExFreePoolWithTag(info.CurrentData, 0);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Data=0x%p, *DataSize=%u, *Type=%u", status, *Data, *DataSize, *Type);
	return status;
}


/************************************************************************/
/*                    PUBLIC FUNCTIONS                                  */
/************************************************************************/


NTSTATUS ValueRecordAlloc(_In_opt_ PUNICODE_STRING Name, _In_ ULONG Type, _In_opt_ PVOID Data, _In_ ULONG DataLength, _Out_ PREGMAN_VALUE_RECORD *Record)
{
	USHORT nameLen = 0;
	PWCH nameBuffer = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREGMAN_VALUE_RECORD tmpRecord = NULL;
	DEBUG_ENTER_FUNCTION("Name=\"%wZ\"; Type=%u; Data=0x%p; DataLength=%u; Record=0x%p", Name, Type, Data, DataLength, Record);

	if (Name != NULL) {
		nameLen = Name->Length;
		nameBuffer = Name->Buffer;
	}

	tmpRecord = HeapMemoryAllocNonPaged(sizeof(REGMAN_VALUE_RECORD) + nameLen);
	if (tmpRecord != NULL) {
		tmpRecord->ReferenceCount = 1;
		tmpRecord->KeyRecord = NULL;
		status = ExInitializeResourceLite(&tmpRecord->Lock);
		if (NT_SUCCESS(status)) {
			tmpRecord->CallbackCount = 0;
			InitializeListHead(&tmpRecord->CallbackList);
			status = ExInitializeResourceLite(&tmpRecord->CallbackLock);
			if (NT_SUCCESS(status)) {
				tmpRecord->Item.Key.String.Length = nameLen;
				tmpRecord->Item.Key.String.MaximumLength = nameLen;
				tmpRecord->Item.Key.String.Buffer = (PWCH)(tmpRecord + 1);
				memcpy(tmpRecord->Item.Key.String.Buffer, nameBuffer, tmpRecord->Item.Key.String.Length);
				tmpRecord->Type = Type;
				tmpRecord->DataSize = DataLength;
				tmpRecord->Data = NULL;
				if (tmpRecord->DataSize > 0) {
					tmpRecord->Data = ExAllocatePoolWithTag(PagedPool, tmpRecord->DataSize, 0);
					if (tmpRecord->Data != NULL) {
						__try {
							memcpy(tmpRecord->Data, Data, tmpRecord->DataSize);
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							status = GetExceptionCode();
						}

						if (!NT_SUCCESS(status))
							ExFreePoolWithTag(tmpRecord->Data, 0);
					} else status = STATUS_INSUFFICIENT_RESOURCES;
				}

				if (NT_SUCCESS(status))
					*Record = tmpRecord;

				if (!NT_SUCCESS(status))
					ExDeleteResourceLite(&tmpRecord->CallbackLock);
			}

			if (!NT_SUCCESS(status))
				ExDeleteResourceLite(&tmpRecord->Lock);
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(tmpRecord);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}


VOID ValueRecordReference(_Inout_ PREGMAN_VALUE_RECORD Record)
{
	InterlockedIncrement(&Record->ReferenceCount);

	return;
}


VOID ValueRecordDereference(_Inout_ PREGMAN_VALUE_RECORD Record)
{
	if (InterlockedDecrement(&Record->ReferenceCount) == 0)
		_ValueRecordDestroy(Record);

	return;
}


NTSTATUS ValueRecordCallbackRegister(_In_ PREGMAN_VALUE_RECORD Record, _In_ const REGMAN_CALLBACKS *Callbacks, _Out_ PHANDLE Handle)
{
	PREGMAN_VALUE_QUERY_CALLBACK_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Callbacks=0x%p; Handle=0x%p", Record, Callbacks, Handle);

	tmpRecord = HeapMemoryAllocPaged(sizeof(REGMAN_VALUE_QUERY_CALLBACK_RECORD));
	if (tmpRecord != NULL) {
		memset(tmpRecord, 0, sizeof(REGMAN_VALUE_QUERY_CALLBACK_RECORD));
		tmpRecord->Record = Record;
		tmpRecord->Callbacks = *Callbacks;
		InitializeListHead(&tmpRecord->ListEntry);
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&Record->CallbackLock, TRUE);
		InsertTailList(&Record->CallbackList, &tmpRecord->ListEntry);
		InterlockedIncrement(&Record->CallbackCount);
		ExReleaseResourceLite(&Record->CallbackLock);
		KeLeaveCriticalRegion();
		*Handle = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Handle=0x%p", status, *Handle);
	return status;
}


VOID ValueRecordCallbackUnregister(_In_ HANDLE Handle)
{
	PREGMAN_VALUE_QUERY_CALLBACK_RECORD cr = (PREGMAN_VALUE_QUERY_CALLBACK_RECORD)Handle;
	DEBUG_ENTER_FUNCTION("Handle=0x%p", Handle);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&cr->Record->CallbackLock, TRUE);
	InterlockedDecrement(&cr->Record->CallbackCount);
	RemoveEntryList(&cr->ListEntry);
	ExReleaseResourceLite(&cr->Record->CallbackLock);
	KeLeaveCriticalRegion();
	HeapMemoryFree(cr);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS ValueRecordCallbackQueryInvoke(_In_ PREGMAN_VALUE_RECORD Record, _Inout_ PVOID *Data, _Inout_ PULONG DataSize, _Inout_ PULONG Type)
{
	PREGMAN_VALUE_QUERY_CALLBACK_RECORD cr = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	REGMAN_QUERY_INFO info;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Data=0x%p; DataSize=0x%p; Type=0x%p", Record, Data, DataSize, Type);

	info.ValueRecord = Record;
	info.CurrentDataSize = Record->DataSize;
	info.CurrentType = Record->Type;
	info.CurrentData = ExAllocatePoolWithTag(PagedPool, info.CurrentDataSize, 0);
	if (info.CurrentData != NULL) {
		status = STATUS_SUCCESS;
		memcpy(info.CurrentData, Record->Data, info.CurrentDataSize);
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&Record->CallbackLock, TRUE);
		cr = CONTAINING_RECORD(Record->CallbackList.Flink, REGMAN_VALUE_QUERY_CALLBACK_RECORD, ListEntry);
		while (&cr->ListEntry != &Record->CallbackList) {
			status = cr->Callbacks.QueryValue(&info, cr->Callbacks.Context);
			if (!NT_SUCCESS(status))
				break;

			cr = CONTAINING_RECORD(cr->ListEntry.Flink, REGMAN_VALUE_QUERY_CALLBACK_RECORD, ListEntry);
		}

		ExReleaseResourceLite(&Record->CallbackLock);
		KeLeaveCriticalRegion();
		if (NT_SUCCESS(status)) {
			if (Data != NULL)
				*Data = info.CurrentData;
			else ExFreePoolWithTag(info.CurrentData, 0);

			if (DataSize != NULL)
				*DataSize = info.CurrentDataSize;
			
			if (Type != NULL)
				*Type = info.CurrentType;
		}

		if (!NT_SUCCESS(status) && info.CurrentData != NULL)
			ExFreePoolWithTag(info.CurrentData, 0);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/************************************************************************/
/*                    CM CALLBACK ROUTINES                              */
/************************************************************************/


NTSTATUS ValueRecordOnQueryValueEntry(_In_ PREGMAN_VALUE_RECORD Value, _Out_ PVOID *Data, _Out_ PULONG DataLength, _Out_ PULONG Type)
{
	ULONG valueType = REG_NONE;
	ULONG valueDataLength = 0;
	PVOID valueData = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Value=0x%p; Data=0x%p; DataLength=0x%p; Type=0x%p", Value, Data, DataLength, Type);

	status = ValueRecordCallbackQueryInvoke(Value, &valueData, &valueDataLength, &valueType);
	if (NT_SUCCESS(status)) {
		*Data = valueData;
		*DataLength = valueDataLength;
		*Type = valueType;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Data=0x%p, *DataLength=%u, *Type=%u", status, *Data, *DataLength, *Type);
	return status;
}


NTSTATUS ValueRecordOnQueryValue(_In_ PREGMAN_VALUE_RECORD Value, _Inout_ PREG_QUERY_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated)
{
	ULONG requiredLength = 0;
	ULONG valueType = REG_NONE;
	ULONG valueDataLength = 0;
	PVOID valueData = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Value=0x%p; Info=0x%p; Emulated=0x%p", Value, Info, Emulated);

	*Emulated = FALSE;
	status = STATUS_SUCCESS;
	if (Info->KeyValueInformationClass == KeyValueBasicInformation || Info->KeyValueInformationClass == KeyValuePartialInformation ||
		Info->KeyValueInformationClass == KeyValuePartialInformationAlign64 || Info->KeyValueInformationClass == KeyValueFullInformation ||
		Info->KeyValueInformationClass == KeyValueFullInformationAlign64) {
		status = ValueRecordCallbackQueryInvoke(Value, &valueData, &valueDataLength, &valueType);
		if (NT_SUCCESS(status)) {
			switch (Info->KeyValueInformationClass) {
				case KeyValueBasicInformation:
					requiredLength = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name) + Value->Item.Key.String.Length;
					break;
				case KeyValuePartialInformation:
					requiredLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + valueDataLength;
					break;
				case KeyValuePartialInformationAlign64:
					requiredLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data) + valueDataLength;
					break;
				case KeyValueFullInformation:
					requiredLength = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) + Value->Item.Key.String.Length + valueDataLength;
					break;
				case KeyValueFullInformationAlign64:
					requiredLength = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) + Value->Item.Key.String.Length + valueDataLength;
					requiredLength += (ULONG)(ALIGN_UP_BY((ULONG_PTR)Info->KeyValueInformation, 8) - (ULONG_PTR)Info->KeyValueInformation);
					break;
				default:
					ASSERT(FALSE);
					break;
			}

			DEBUG_PRINT_LOCATION("Requred length           = %u", requiredLength);
			if (Info->Length >= requiredLength) {
				switch (Info->KeyValueInformationClass) {
					case KeyValueBasicInformation: {
						PKEY_VALUE_BASIC_INFORMATION kvbi = NULL;

						kvbi = (PKEY_VALUE_BASIC_INFORMATION)Info->KeyValueInformation;
						__try {
							kvbi->TitleIndex = 0;
							kvbi->Type = valueType;
							kvbi->NameLength = Value->Item.Key.String.Length;
							memcpy(kvbi->Name, Value->Item.Key.String.Buffer, Value->Item.Key.String.Length);
							*Info->ResultLength = requiredLength;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							status = GetExceptionCode();
						}
					} break;
					case KeyValuePartialInformation: {
						PKEY_VALUE_PARTIAL_INFORMATION kvpi = NULL;

						kvpi = (PKEY_VALUE_PARTIAL_INFORMATION)Info->KeyValueInformation;
						__try {
							kvpi->TitleIndex = 0;
							kvpi->Type = valueType;
							kvpi->DataLength = valueDataLength;
							memcpy(kvpi->Data, valueData, valueDataLength);
							*Info->ResultLength = requiredLength;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							status = GetExceptionCode();
						}
					} break;
					case KeyValuePartialInformationAlign64: {
						PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64 kvpi64 = NULL;

						kvpi64 = (PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64)Info->KeyValueInformation;
						__try {
							kvpi64->Type = valueType;
							kvpi64->DataLength = valueDataLength;
							memcpy(kvpi64->Data, valueData, valueDataLength);
							*Info->ResultLength = requiredLength;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							status = GetExceptionCode();
						}
					} break;
					case KeyValueFullInformation:
					case KeyValueFullInformationAlign64: {
						PKEY_VALUE_FULL_INFORMATION kvfi = NULL;

						kvfi = (PKEY_VALUE_FULL_INFORMATION)Info->KeyValueInformation;
						if (Info->KeyValueInformationClass == KeyValueFullInformationAlign64)
							kvfi = (PKEY_VALUE_FULL_INFORMATION)ALIGN_UP_BY((ULONG_PTR)kvfi, 8);

						__try {
							ULONG dataOffset = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) + Value->Item.Key.String.Length;

							kvfi->TitleIndex = 0;
							kvfi->Type = valueType;
							kvfi->NameLength = Value->Item.Key.String.Length;
							memcpy(kvfi->Name, Value->Item.Key.String.Buffer, Value->Item.Key.String.Length);
							kvfi->DataOffset = dataOffset;
							kvfi->DataLength = valueDataLength;
							memcpy((PUCHAR)kvfi + dataOffset, valueData, valueDataLength);
							*Info->ResultLength = requiredLength;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							status = GetExceptionCode();
						}
					} break;
					default:
						ASSERT(FALSE);
						break;
				}
			} else {
				status = STATUS_BUFFER_TOO_SMALL;
				switch (Info->KeyValueInformationClass) {
					case KeyValuePartialInformation:
						if (Info->Length >= (ULONG)FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) {
							PKEY_VALUE_PARTIAL_INFORMATION kvpi = NULL;

							status = STATUS_BUFFER_OVERFLOW;
							kvpi = (PKEY_VALUE_PARTIAL_INFORMATION)Info->KeyValueInformation;
							__try {
								kvpi->TitleIndex = 0;
								kvpi->Type = valueType;
								kvpi->DataLength = valueDataLength;
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								status = GetExceptionCode();
							}
						}
						break;
					case KeyValuePartialInformationAlign64:
						if (Info->Length >= (ULONG)FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data)) {
							PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64 kvpi64 = NULL;

							status = STATUS_BUFFER_OVERFLOW;
							kvpi64 = (PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64)Info->KeyValueInformation;
							__try {
								kvpi64->Type = valueType;
								kvpi64->DataLength = valueDataLength;
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								status = GetExceptionCode();
							}
						}
						break;
					default:
						break;
				}

				__try {
					*Info->ResultLength = requiredLength;
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					status = GetExceptionCode();
				}
			}

			if (valueData != NULL)
				ExFreePool(valueData);
		}

		*Emulated = NT_SUCCESS(status);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Emulated=%u", status, *Emulated);
	return status;
}


NTSTATUS ValueRecordOnEnumValue(_In_ PREGMAN_VALUE_RECORD Value, _Inout_ PREG_ENUMERATE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated)
{
	REG_QUERY_VALUE_KEY_INFORMATION	queryInfo;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Value=0x%p; Info=0x%p; Emulated=0x%p", Value, Info, Emulated);
	
	RtlSecureZeroMemory(&queryInfo, sizeof(queryInfo));
	queryInfo.KeyValueInformation = Info->KeyValueInformation;
	queryInfo.Length = Info->Length;
	queryInfo.KeyValueInformationClass = Info->KeyValueInformationClass;
	queryInfo.ResultLength = Info->ResultLength;
	queryInfo.CallContext = Info->CallContext;
	queryInfo.Object = Info->Object;
	queryInfo.ObjectContext = Info->ObjectContext;
	queryInfo.Reserved = Info->Reserved;
	queryInfo.ValueName = &Value->Item.Key.String;
	status = ValueRecordOnQueryValue(Value, &queryInfo, Emulated);

	DEBUG_EXIT_FUNCTION("0x%x; *Emulated=%u", status, *Emulated);
	return status;
}


NTSTATUS ValueRecordOnSetValue(_In_ PREGMAN_VALUE_RECORD Value, _In_ PREG_SET_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated)
{
	ULONG valueType = REG_NONE;
	ULONG valueSize = 0;
	PVOID valueData = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Value=0x%p; Info=0x%p; Emulated=0x%p", Value, Info, Emulated);

	*Emulated = FALSE;
	status = STATUS_SUCCESS;
	valueSize = Info->DataSize;
	valueType = Info->Type;
	if (valueSize > 0) {
		valueData = ExAllocatePoolWithTag(PagedPool, valueSize, 0);
		if (valueData != NULL) {
			__try {
				memcpy(valueData, Info->Data, valueSize);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				status = GetExceptionCode();
			}
		
			if (!NT_SUCCESS(status))
				ExFreePoolWithTag(valueData, 0);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	if (NT_SUCCESS(status)) {
		status = _ValueRecordCallbackSetInvoke(Value, &valueData, &valueSize, &valueType);
		if (NT_SUCCESS(status)) {
			HANDLE keyHandle = NULL;

			status = ObOpenObjectByPointer(Info->Object, OBJ_KERNEL_HANDLE, NULL, KEY_SET_VALUE, NULL, KernelMode, &keyHandle);
			if (NT_SUCCESS(status)) {
				status = ZwSetValueKey(keyHandle, &Value->Item.Key.String, 0, valueType, valueData, valueSize);
				ZwClose(keyHandle);
				if (NT_SUCCESS(status)) {
					*Emulated = TRUE;
					KeEnterCriticalRegion();
					ExAcquireResourceExclusiveLite(&Value->Lock, TRUE);
					if (Value->Data != NULL)
						ExFreePoolWithTag(Value->Data, 0);

					Value->Data = valueData;
					Value->DataSize = valueSize;
					Value->Type = valueType;
					ExReleaseResourceLite(&Value->Lock);
					KeLeaveCriticalRegion();
				}
			}
		}

		if (!NT_SUCCESS(status) && valueData != NULL)
			ExFreePoolWithTag(valueData, 0);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Emulated=0x%p", status, *Emulated);
	return status;
}


NTSTATUS ValueRecordOnDeleteValue(_In_ PREGMAN_VALUE_RECORD Value, _In_ PREG_DELETE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated)
{
	BOOLEAN stopEmulation = TRUE;
	PREGMAN_VALUE_POST_CONTEXT postRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Value=0x%p; Info=0x%p; Emulated=0x%p", Value, Info, Emulated);

	*Emulated = FALSE;
	status = _ValueRecordCallbackDeleteInvoke(Value, &stopEmulation);
	if (NT_SUCCESS(status)) {
		status = _ValuePostRecordAlloc(Value, rmvpctDeleteValue, &postRecord);
		if (NT_SUCCESS(status)) {
			postRecord->Data.DeleteValue.StopEmulation = stopEmulation;
			Info->CallContext = postRecord;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS ValueRecordOnPostOperation(PREG_POST_OPERATION_INFORMATION Info, PBOOLEAN Emulated)
{
	PREGMAN_VALUE_POST_CONTEXT postRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Info=0x%p; Emulated=0x%p", Info, Emulated);

	*Emulated = FALSE;
	status = STATUS_SUCCESS;
	postRecord = Info->CallContext;
	if (NT_SUCCESS(Info->Status) || Info->Status == STATUS_OBJECT_NAME_NOT_FOUND) {
		PREGMAN_VALUE_RECORD valueRecord = postRecord->ValueRecord;

		switch (postRecord->Type) {
			case rmvpctDeleteValue: {
				KeEnterCriticalRegion();
				ExAcquireResourceExclusiveLite(&valueRecord->Lock, TRUE);
				if (valueRecord->Data != NULL)
					ExFreePoolWithTag(valueRecord->Data, 0);

				valueRecord->Data = NULL;
				valueRecord->DataSize = 0;
				valueRecord->Type = REG_NONE;
				ExReleaseResourceLite(&valueRecord->Lock);
				KeLeaveCriticalRegion();
				if (Info->Status == STATUS_OBJECT_NAME_NOT_FOUND) {
					Info->ReturnStatus = STATUS_SUCCESS;
					*Emulated = TRUE;
				}
			} break;
			default:
				break;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Emulated=0x%p", status, *Emulated);
	return status;
}


VOID ValuePostRecordFree(_Inout_ PREGMAN_VALUE_POST_CONTEXT Record)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

	ValueRecordDereference(Record->ValueRecord);
	HeapMemoryFree(Record);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
