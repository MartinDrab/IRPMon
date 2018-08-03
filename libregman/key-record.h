
#ifndef __REGMAN_KEY_RECORD_H__
#define __REGMAN_KEY_RECORD_H__


#include <ntifs.h>

struct _REGMAN_KEY_RECORD;

#include "value-record.h"
#include "string-ref-table.h"



typedef struct _REGMAN_KEY_RECORD {
	STRING_REF_ITEM Item;
	volatile LONG ReferenceCount;
	STRING_REF_TABLE ValueTable;
	PREGMAN_VALUE_RECORD *ValueRecords;
	volatile LONG ValueCount;
	volatile LONG AllocValueCount;
	ERESOURCE OperationLock;
} REGMAN_KEY_RECORD, *PREGMAN_KEY_RECORD;



NTSTATUS KeyRecordAlloc(_In_ PUNICODE_STRING Name, PREGMAN_KEY_RECORD *Record);
VOID KeyRecordReference(_Inout_ PREGMAN_KEY_RECORD Record);
VOID KeyRecordDereference(_Inout_ PREGMAN_KEY_RECORD Record);

NTSTATUS KeyRecordValueAdd(_In_ PREGMAN_KEY_RECORD Record, _In_opt_ PUNICODE_STRING ValueName, _In_opt_ PVOID Data, _In_opt_ ULONG Size, _In_ ULONG Type, _Out_opt_ PREGMAN_VALUE_RECORD *Value);
PREGMAN_VALUE_RECORD KeyRecordValueGet(_In_ PREGMAN_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName);
VOID KeyRecordValueDelete(_In_ PREGMAN_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName);
NTSTATUS KeyRecordFromKey(_In_ PUNICODE_STRING KeyName, PREGMAN_KEY_RECORD *Record);

NTSTATUS KeyRecordOnQuery(_In_ PREGMAN_KEY_RECORD Record, PREG_QUERY_KEY_INFORMATION Info, _Out_opt_ PBOOLEAN Bypass);
NTSTATUS KeyRecordQueryMultipleValues(_In_ PREGMAN_KEY_RECORD Record, _Inout_ PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated);
NTSTATUS KeyRecordOnQueryValue(_In_ PREGMAN_KEY_RECORD Record, _Inout_ PREG_QUERY_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulate);
NTSTATUS KeyRecordOnEnumValue(_In_ PREGMAN_KEY_RECORD Record, _Inout_ PREG_ENUMERATE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated);
NTSTATUS KeyRecordOnSetValue(_In_ PREGMAN_KEY_RECORD Record, _In_ PREG_SET_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated);
NTSTATUS KeyRecordOnDeleteValue(_In_ PREGMAN_KEY_RECORD Record, _In_ PREG_DELETE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated);
NTSTATUS KeyRecordOnPostOperation(PREGMAN_KEY_RECORD Record, PREG_POST_OPERATION_INFORMATION Info, PBOOLEAN Emulated);



#endif
