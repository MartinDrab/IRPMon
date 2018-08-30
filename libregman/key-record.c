
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "string-ref-table.h"
#include "value-record.h"
#include "key-record.h"


/************************************************************************/
/*                   HELPER FUNCTIONS                                   */
/************************************************************************/

static VOID _KeyRecordDestroy(_In_ PREGMAN_KEY_RECORD Record)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

	if (Record->AllocValueCount > 0)
		HeapMemoryFree(Record->ValueRecords);

	StringRefTableDestroy(&Record->ValueTable);
	ExDeleteResourceLite(&Record->OperationLock);
	HeapMemoryFree(Record);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _CaptureUnicodeString(_In_opt_ PUNICODE_STRING String, _Out_ PUNICODE_STRING *Result)
{
	USHORT length = 0;
	PWCH buffer = NULL;
	PUNICODE_STRING tmp = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("String=\"%wZ\"; Result=0x%p", String, Result);

	status = STATUS_SUCCESS;
	if (String != NULL) {
		__try {
			length = String->Length;
			buffer = String->Buffer;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
		}
	}

	if (NT_SUCCESS(status)) {
		tmp = HeapMemoryAllocPaged(sizeof(UNICODE_STRING) + length);
		if (tmp != NULL) {
			tmp->Length = length;
			tmp->MaximumLength = tmp->Length;
			tmp->Buffer = (PWCH)(tmp + 1);
			__try {
				if (ExGetPreviousMode() == UserMode && buffer != NULL && (ULONG_PTR)buffer < (ULONG_PTR)MmHighestUserAddress)
					ProbeForRead(buffer, length, 1);

				memcpy(tmp->Buffer, buffer, tmp->Length);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				status = GetExceptionCode();
			}

			if (NT_SUCCESS(status))
				*Result = tmp;

			if (!NT_SUCCESS(status))
				HeapMemoryFree(tmp);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Result=\"%wZ\"", status, *Result);
	return status;
}


static NTSTATUS _KeyRecordGatherStatistics(PREGMAN_KEY_RECORD Record, PULONG MaxValueNameLen, PULONG MaxValueDataLen, PULONG ValueCount)
{
	ULONG dataLen = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; MaxValueNameLen=0x%p; MaxValueDataLen=0x%p; ValueCount=0x%p", Record, MaxValueNameLen, MaxValueDataLen, ValueCount);

	*ValueCount = (ULONG)Record->ValueCount;
	for (LONG i = 0; i < Record->ValueCount; ++i) {
		status = ValueRecordCallbackQueryInvoke(Record->ValueRecords[i], NULL, &dataLen, NULL);
		if (NT_SUCCESS(status)) {
			if (*MaxValueNameLen < Record->ValueRecords[i]->Item.Key.String.Length)
				*MaxValueNameLen = Record->ValueRecords[i]->Item.Key.String.Length;

			if (*MaxValueDataLen < dataLen)
				*MaxValueDataLen = dataLen;
		}

		if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
			*ValueCount--;
			status = STATUS_SUCCESS;
		}

		if (!NT_SUCCESS(status))
			break;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *MaxValueNameLen=%u, *MaxValueDataLen=%u, *ValueCount=%u", status, *MaxValueNameLen, *MaxValueDataLen, *ValueCount);
	return status;
}


/************************************************************************/
/*                    PUBLIC FUNCTIONS                                  */
/************************************************************************/


NTSTATUS KeyRecordAlloc(_In_ PUNICODE_STRING Name, PREGMAN_KEY_RECORD *Record)
{
	PREGMAN_KEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Name=\"%wZ\"; Record=0x%p", Name, Record);

	tmpRecord = (PREGMAN_KEY_RECORD)HeapMemoryAllocNonPaged(sizeof(REGMAN_KEY_RECORD) + Name->Length);
	if (tmpRecord != NULL) {
		tmpRecord->ReferenceCount = 1;
		tmpRecord->Item.Key.String.Length = Name->Length;
		tmpRecord->Item.Key.String.MaximumLength = Name->Length;
		tmpRecord->Item.Key.String.Buffer =  (PWCH)(tmpRecord + 1);
		memcpy(tmpRecord->Item.Key.String.Buffer, Name->Buffer, tmpRecord->Item.Key.String.Length);
		InterlockedExchange(&tmpRecord->ValueCount, 0);
		InterlockedExchange(&tmpRecord->AllocValueCount, 0);
		tmpRecord->ValueRecords = NULL;
		status = ExInitializeResourceLite(&tmpRecord->OperationLock);
		if (NT_SUCCESS(status)) {
			status = StringRefTableInit(FIELD_OFFSET(REGMAN_VALUE_RECORD, Item), 37, ValueRecordReference, ValueRecordDereference, &tmpRecord->ValueTable);
			if (NT_SUCCESS(status))
				*Record = tmpRecord;

			if (!NT_SUCCESS(status))
				ExDeleteResourceLite(&tmpRecord->OperationLock);
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(tmpRecord);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}


VOID KeyRecordReference(_Inout_ PREGMAN_KEY_RECORD Record)
{
	InterlockedIncrement(&Record->ReferenceCount);
}

VOID KeyRecordDereference(_Inout_ PREGMAN_KEY_RECORD Record)
{
	if (InterlockedDecrement(&Record->ReferenceCount) == 0)
		_KeyRecordDestroy(Record);

	return;
}


NTSTATUS KeyRecordValueAdd(_In_ PREGMAN_KEY_RECORD Record, _In_opt_ PUNICODE_STRING ValueName, _In_opt_ PVOID Data, _In_opt_ ULONG Size, _In_ ULONG Type, _Out_opt_ PREGMAN_VALUE_RECORD *Value)
{
	ULONG newCount = 0;
	PREGMAN_VALUE_RECORD tmpRecord = NULL;
	PREGMAN_VALUE_RECORD insertedRecord = NULL;
	PREGMAN_VALUE_RECORD *tmp = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; ValueName=\"%wZ\"; Data=0x%p; Size=%u; Type=%u; Value=0x%p", Record, ValueName, Data, Size, Type, Value);

	status = ValueRecordAlloc(ValueName, Type, Data, Size, &tmpRecord);
	if (NT_SUCCESS(status)) {
		insertedRecord = StringRefTableInsert(&Record->ValueTable, &tmpRecord->Item);
		if (insertedRecord == tmpRecord) {
			KeEnterCriticalRegion();
			ExAcquireResourceExclusiveLite(&Record->OperationLock, TRUE);
			if (Record->AllocValueCount == Record->ValueCount) {
				newCount = Record->ValueCount * 2 + 1;
				tmp = (PREGMAN_VALUE_RECORD *)HeapMemoryAllocPaged(sizeof(PREGMAN_VALUE_RECORD)*newCount);
				if (tmp != NULL) {
					memcpy(tmp, Record->ValueRecords, Record->ValueCount * sizeof(PREGMAN_VALUE_RECORD));
					tmp[Record->ValueCount] = tmpRecord;
					tmp = (PREGMAN_VALUE_RECORD *)InterlockedExchangePointer((PVOID *)&Record->ValueRecords, tmp);
					if (tmp != NULL)
						HeapMemoryFree(tmp);

					InterlockedExchange(&Record->AllocValueCount, newCount);
				} else status = STATUS_INSUFFICIENT_RESOURCES;
			} else Record->ValueRecords[Record->ValueCount] = tmpRecord;

			if (NT_SUCCESS(status)) {
				tmpRecord->KeyRecord = Record;
				InterlockedIncrement(&Record->ValueCount);
				if (Value != NULL) {
					ValueRecordReference(tmpRecord);
					*Value = tmpRecord;
				}
			}

			ExReleaseResourceLite(&Record->OperationLock);
			KeLeaveCriticalRegion();
			if (!NT_SUCCESS(status)) {
				StringRefTableDelete(&Record->ValueTable, &tmpRecord->Item.Key.String);
				ValueRecordDereference(tmpRecord);
			}
		} else {
			if (Value != NULL)
				*Value = insertedRecord;
			else ValueRecordDereference(insertedRecord);
			
			status = STATUS_OBJECT_NAME_EXISTS;
		}

		ValueRecordDereference(tmpRecord);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Value=0x%p", status, ((Value != NULL) ? *Value : NULL));
	return status;
}


PREGMAN_VALUE_RECORD KeyRecordValueGet(_In_ PREGMAN_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName)
{
	PREGMAN_VALUE_RECORD ret = NULL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; ValueName=\"%wZ\"", Record, ValueName);

	ret = (PREGMAN_VALUE_RECORD)StringRefTableGet(&Record->ValueTable, ValueName);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


VOID KeyRecordValueDelete(_In_ PREGMAN_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName)
{
	PREGMAN_VALUE_RECORD valueRecord = NULL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; ValueName=\"%wZ\"", Record, ValueName);

	valueRecord = (PREGMAN_VALUE_RECORD)StringRefTableDelete(&Record->ValueTable, ValueName);
	if (valueRecord != NULL) {
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&Record->OperationLock, TRUE);
		for (LONG i = 0; i < Record->ValueCount; ++i) {
			if (valueRecord == Record->ValueRecords[i]) {
				memmove(Record->ValueRecords + i, Record->ValueRecords + i + 1, (Record->ValueCount - i - 1)*sizeof(PREGMAN_VALUE_RECORD));
				InterlockedDecrement(&Record->ValueCount);
				if (Record->ValueCount == 0) {
					HeapMemoryFree(Record->ValueRecords);
					Record->ValueRecords = NULL;
					InterlockedExchange(&Record->AllocValueCount, 0);
				}

				break;
			}
		}

		ExReleaseResourceLite(&Record->OperationLock);
		KeLeaveCriticalRegion();
		valueRecord->KeyRecord = NULL;
		ValueRecordDereference(valueRecord);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS KeyRecordFromKey(_In_ PUNICODE_STRING KeyName, PREGMAN_KEY_RECORD *Record)
{
	ULONG index = 0;
	HANDLE keyHandle = NULL;
	OBJECT_ATTRIBUTES oa;
	ULONG retLen = 0;
	PKEY_VALUE_FULL_INFORMATION kvfi = NULL;
	PREGMAN_KEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyName=\"%wZ\"; Record=0x%p", KeyName, Record);

	status = KeyRecordAlloc(KeyName, &tmpRecord);
	if (NT_SUCCESS(status)) {
		InitializeObjectAttributes(&oa, KeyName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
		status = ZwOpenKey(&keyHandle, KEY_READ, &oa);
		if (NT_SUCCESS(status)) {
			do {
				status = ZwEnumerateValueKey(keyHandle, index, KeyValueFullInformation, NULL, 0, &retLen);
				if (status == STATUS_BUFFER_TOO_SMALL) {
					kvfi = (PKEY_VALUE_FULL_INFORMATION)HeapMemoryAllocPaged(retLen);
					if (kvfi != NULL) {
						status = ZwEnumerateValueKey(keyHandle, index, KeyValueFullInformation, kvfi, retLen, &retLen);
						if (NT_SUCCESS(status)) {
							UNICODE_STRING vn;

							vn.Length = (USHORT)kvfi->NameLength;
							vn.MaximumLength = vn.Length;
							vn.Buffer = kvfi->Name;
							status = KeyRecordValueAdd(tmpRecord, &vn, (PUCHAR)kvfi + kvfi->DataOffset, kvfi->DataLength, kvfi->Type, NULL);
						}

						HeapMemoryFree(kvfi);
					} else status = STATUS_INSUFFICIENT_RESOURCES;
				}

				++index;
			} while (NT_SUCCESS(status));

			if (status == STATUS_NO_MORE_ENTRIES)
				status = STATUS_SUCCESS;

			ZwClose(keyHandle);
		} else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
			status = STATUS_SUCCESS;

		if (NT_SUCCESS(status))
			*Record = tmpRecord;

		if (!NT_SUCCESS(status))
			KeyRecordDereference(tmpRecord);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}


NTSTATUS KeyRecordQueryMultipleValues(_In_ PREGMAN_KEY_RECORD Record, _Inout_ PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated)
{
	ULONG requiredLength = 0;
	ULONG bufferLength = 0;
	KEY_VALUE_ENTRY oneEntry;
	PVOID *buffers = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p; Emulated=0x%p", Record, Info, Emulated);

	buffers = HeapMemoryAllocPaged(Info->EntryCount * sizeof(PVOID));
	if (buffers != NULL) {
		status = STATUS_SUCCESS;
		RtlSecureZeroMemory(buffers, sizeof(PVOID)*Info->EntryCount);
		__try {
			bufferLength = *Info->BufferLength;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
		}

		if (NT_SUCCESS(status)) {
			for (SIZE_T i = 0; i < Info->EntryCount; ++i) {
				PUNICODE_STRING uValueName = NULL;
				
				__try {
					oneEntry = Info->ValueEntries[i];
					if (oneEntry.ValueName != NULL)
						status = _CaptureUnicodeString(oneEntry.ValueName, &uValueName);
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					status = GetExceptionCode();
				}

				if (NT_SUCCESS(status)) {
					PREGMAN_VALUE_RECORD valueRecord = NULL;

					oneEntry.DataLength = 0;
					oneEntry.DataOffset = 0;
					oneEntry.Type = REG_NONE;
					valueRecord = (PREGMAN_VALUE_RECORD)StringRefTableGet(&Record->ValueTable, uValueName);
					if (valueRecord != NULL) {
						status = ValueRecordOnQueryValueEntry(valueRecord, buffers + i, &oneEntry.DataLength, &oneEntry.Type);
						if (NT_SUCCESS(status)) {
							requiredLength += oneEntry.DataLength;
							__try {
								Info->ValueEntries[i] = oneEntry;
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								status = GetExceptionCode();
							}
						}

						ValueRecordDereference(valueRecord);
					} else status = STATUS_OBJECT_NAME_NOT_FOUND;
				
					if (uValueName != NULL)
						HeapMemoryFree(uValueName);
				}

				if (!NT_SUCCESS(status))
					break;
			}

			if (NT_SUCCESS(status)) {
				__try {
					if (Info->RequiredBufferLength != NULL)
						*Info->RequiredBufferLength = requiredLength;
					
					if (Info->ValueBuffer != NULL) {
						if (requiredLength <= bufferLength) {
							ULONG offset = 0;
							ULONG length = 0;

							for (SIZE_T i = 0; i < Info->EntryCount; ++i) {
								length = Info->ValueEntries[i].DataLength;
								Info->ValueEntries[i].DataOffset = offset;
								memcpy((PUCHAR)Info->ValueBuffer + offset, buffers[i], length);
								offset += Info->ValueEntries[i].DataLength;
							}
						} else status = STATUS_BUFFER_OVERFLOW;
					} else status = STATUS_BUFFER_TOO_SMALL;
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					status = GetExceptionCode();
				}
			}
		}

		for (SIZE_T i = 0; i < Info->EntryCount; ++i) {
			if (buffers[i] != NULL)
				HeapMemoryFree(buffers[i]);
		}

		HeapMemoryFree(buffers);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	*Emulated = NT_SUCCESS(status);

	if (!NT_SUCCESS(status)) {
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "QUERYMULTIPLEE: 0x%x\n", status);
		DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "  Key = %wZ\n", &Record->Item.Key.String);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Emulated=%u", status, *Emulated);
	return status;
}


NTSTATUS KeyRecordOnQueryValue(_In_ PREGMAN_KEY_RECORD Record, _Inout_ PREG_QUERY_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulate)
{
	PUNICODE_STRING valueName = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p; Emulate=0x%p", Record, Info, Emulate);

	*Emulate = FALSE;
	status = _CaptureUnicodeString(Info->ValueName, &valueName);
	if (NT_SUCCESS(status)) {
		PREGMAN_VALUE_RECORD valueRecord = NULL;

		KeEnterCriticalRegion();
		ExAcquireResourceSharedLite(&Record->OperationLock, TRUE);
		valueRecord = (PREGMAN_VALUE_RECORD)StringRefTableGet(&Record->ValueTable, valueName);
		if (valueRecord != NULL) {
			status = ValueRecordOnQueryValue(valueRecord, Info, Emulate);
			ValueRecordDereference(valueRecord);
		}

		ExReleaseResourceLite(&Record->OperationLock);
		KeLeaveCriticalRegion();
		HeapMemoryFree(valueName);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Emulate=%u", status, *Emulate);
	return status;
}


NTSTATUS KeyRecordOnEnumValue(_In_ PREGMAN_KEY_RECORD Record, _Inout_ PREG_ENUMERATE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated)
{
	PREGMAN_VALUE_RECORD valueRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p; Emulated=0x%p", Record, Info, Emulated);

	*Emulated = FALSE;
	if (Info->Index < (ULONG)Record->ValueCount) {
		KeEnterCriticalRegion();
		ExAcquireResourceSharedLite(&Record->OperationLock, TRUE);
		valueRecord = Record->ValueRecords[Info->Index];
		ValueRecordReference(valueRecord);
		status = ValueRecordOnEnumValue(valueRecord, Info, Emulated);
		ValueRecordDereference(valueRecord);
		ExReleaseResourceLite(&Record->OperationLock);
		KeLeaveCriticalRegion();
	} else status = STATUS_NO_MORE_ENTRIES;

	DEBUG_EXIT_FUNCTION("0x%x, *Emulated=%u", status, Emulated);
	return status;
}


NTSTATUS KeyRecordOnSetValue(_In_ PREGMAN_KEY_RECORD Record, _In_ PREG_SET_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated)
{
	PUNICODE_STRING valueName = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p; Emulated=0x%p", Record, Info, Emulated);

	*Emulated = FALSE;
	status = _CaptureUnicodeString(Info->ValueName, &valueName);
	if (NT_SUCCESS(status)) {
		PREGMAN_VALUE_RECORD valueRecord = NULL;

		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&Record->OperationLock, TRUE);
		valueRecord = (PREGMAN_VALUE_RECORD)StringRefTableGet(&Record->ValueTable, valueName);
		if (valueRecord != NULL) {
			status = ValueRecordOnSetValue(valueRecord, Info, Emulated);
			ValueRecordDereference(valueRecord);
		} else {
			status = KeyRecordValueAdd(Record, valueName, NULL, 0, REG_NONE, &valueRecord);
			if (NT_SUCCESS(status)) {
				status = ValueRecordOnSetValue(valueRecord, Info, Emulated);
				if (!NT_SUCCESS(status))
					KeyRecordValueDelete(Record, &valueRecord->Item.Key.String);

				ValueRecordDereference(valueRecord);
			}
		}

		ExReleaseResourceLite(&Record->OperationLock);
		KeLeaveCriticalRegion();
		HeapMemoryFree(valueName);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Emulated=%u", status, *Emulated);
	return status;
}


NTSTATUS KeyRecordOnDeleteValue(_In_ PREGMAN_KEY_RECORD Record, _In_ PREG_DELETE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated)
{
	PUNICODE_STRING valueName = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p; Emulated=0x%p", Record, Info, Emulated);

	*Emulated = FALSE;
	status = _CaptureUnicodeString(Info->ValueName, &valueName);
	if (NT_SUCCESS(status)) {
		PREGMAN_VALUE_RECORD valueRecord = NULL;

		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&Record->OperationLock, TRUE);
		valueRecord = (PREGMAN_VALUE_RECORD)StringRefTableGet(&Record->ValueTable, valueName);
		if (valueRecord != NULL) {
			status = ValueRecordOnDeleteValue(valueRecord, Info, Emulated);
			ValueRecordDereference(valueRecord);
		}

		ExReleaseResourceLite(&Record->OperationLock);
		KeLeaveCriticalRegion();
		HeapMemoryFree(valueName);
	}

	DEBUG_EXIT_FUNCTION("0x%x; *Emulated=%u", status, *Emulated);
	return status;
}


NTSTATUS KeyRecordOnQuery(_In_ PREGMAN_KEY_RECORD Record, PREG_QUERY_KEY_INFORMATION Info, _Out_opt_ PBOOLEAN Emulated)
{
	ULONG retLength = 0;
	PVOID keyInfo = NULL;
	HANDLE keyHandle = NULL;
	BOOLEAN emulated = FALSE;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p; Bypass=0x%p", Record, Info, Emulated);

	emulated = (
			Info->KeyInformationClass == KeyFullInformation ||
			Info->KeyInformationClass == KeyCachedInformation);
	if (emulated) {
		status = ObOpenObjectByPointer(Info->Object, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, KEY_READ, NULL, KernelMode, &keyHandle);
		if (NT_SUCCESS(status)) {
			if (Info->Length > 0) {
				keyInfo = HeapMemoryAllocPaged(Info->Length);
				if (keyInfo == NULL)
					status = STATUS_INSUFFICIENT_RESOURCES;
			}

			if (NT_SUCCESS(status)) {
				status = ZwQueryKey(keyHandle, Info->KeyInformationClass, keyInfo, Info->Length, &retLength);
				if (NT_SUCCESS(status)) {
					ULONG maxValueNameLen = 0;
					ULONG maxValueDataLen = 0;
					ULONG valueCount = 0;

					KeEnterCriticalRegion();
					ExAcquireResourceSharedLite(&Record->OperationLock, TRUE);
					status = _KeyRecordGatherStatistics(Record, &maxValueNameLen, &maxValueDataLen, &valueCount);
					ExReleaseResourceLite(&Record->OperationLock);
					KeLeaveCriticalRegion();
					if (NT_SUCCESS(status)) {
						switch (Info->KeyInformationClass) {
							case KeyFullInformation: {
								PKEY_FULL_INFORMATION kfi = (PKEY_FULL_INFORMATION)keyInfo;

								kfi->Values = valueCount;
								kfi->MaxValueNameLen = maxValueNameLen;
								kfi->MaxValueDataLen = maxValueDataLen;
							} break;
							case KeyCachedInformation: {
								PKEY_CACHED_INFORMATION kci = (PKEY_CACHED_INFORMATION)keyInfo;
							
								kci->Values = valueCount;
								kci->MaxValueNameLen = maxValueNameLen;
								kci->MaxValueDataLen = maxValueDataLen;
							} break;
							default:
								break;
						}

						__try {
							memcpy(Info->KeyInformation, keyInfo, retLength);
							*Info->ResultLength = retLength;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							status = GetExceptionCode();
						}
					}
				} else if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_BUFFER_OVERFLOW) {
					__try {
						memcpy(Info->KeyInformation, keyInfo, Info->Length);
						*Info->ResultLength = retLength;
					}
					__except (EXCEPTION_EXECUTE_HANDLER) {
						status = GetExceptionCode();
					}
				}

				if (keyInfo != NULL)
					HeapMemoryFree(keyInfo);
			}

			ZwClose(keyHandle);
		}
	}

	if (NT_SUCCESS(status) && Emulated != NULL)
		*Emulated = emulated;

	DEBUG_EXIT_FUNCTION("0x%x, *Bypass=%u", status, emulated);
	return status;
}


NTSTATUS KeyRecordOnPostOperation(_Inout_ PREGMAN_KEY_RECORD Record, _Inout_ PREG_POST_OPERATION_INFORMATION Info, _Out_ PBOOLEAN Emulated)
{
	PREGMAN_VALUE_POST_CONTEXT postRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	*Emulated = FALSE;
	status = STATUS_SUCCESS;
	postRecord = Info->CallContext;
	if (postRecord != NULL) {
		PREGMAN_VALUE_RECORD valueRecord = postRecord->ValueRecord;
		DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p; Emulated=0x%p", Record, Info, Emulated);

		switch (postRecord->Type) {
			case rmvpctDeleteValue: {
				ValueRecordReference(valueRecord);
				status = ValueRecordOnPostOperation(Info, Emulated);			
				if (NT_SUCCESS(status) && postRecord->Data.DeleteValue.StopEmulation)
					KeyRecordValueDelete(Record, &valueRecord->Item.Key.String);
				
				ValueRecordDereference(valueRecord);
			} break;
			default:
				break;
		}

		ValuePostRecordFree(postRecord);
		Info->CallContext = NULL;
		DEBUG_EXIT_FUNCTION("0x%x, *Emulated=0x%p", status, *Emulated);
	}

	return status;
}
