
#ifndef __REGMAN_VALUE_RECORD_H__
#define __REGMAN_VALUE_RECORD_H__

#include <ntifs.h>
#include "string-ref-table.h"
#include "regman.h"



typedef struct _REGMAN_VALUE_QUERY_CALLBACK_RECORD {
	LIST_ENTRY ListEntry;
	struct _REGMAN_VALUE_RECORD *Record;
	REGMAN_CALLBACKS Callbacks;
} REGMAN_VALUE_QUERY_CALLBACK_RECORD, *PREGMAN_VALUE_QUERY_CALLBACK_RECORD;


typedef struct _REGMAN_VALUE_RECORD {
	STRING_REF_ITEM Item;
	volatile LONG ReferenceCount;
	ERESOURCE Lock;
	struct _REGMAN_KEY_RECORD *KeyRecord;
	ULONG Type;
	PVOID Data;
	ULONG DataSize;
	ERESOURCE CallbackLock;
	volatile LONG CallbackCount;
	LIST_ENTRY CallbackList;
} REGMAN_VALUE_RECORD, *PREGMAN_VALUE_RECORD;


typedef enum _ERegManValuePostContextType {
	rmvpctUnknown,
	rmvpctSetValue,
	rmvpctDeleteValue,
	rmvpctMax,
} ERegManValuePostContextType, *PERegManValuePostContextType;

typedef struct _REGMAN_VALUE_POST_CONTEXT {
	PREGMAN_VALUE_RECORD ValueRecord;
	ERegManValuePostContextType Type;
	union {
		struct {
			BOOLEAN StopEmulation;
		} DeleteValue;
	} Data;
} REGMAN_VALUE_POST_CONTEXT, *PREGMAN_VALUE_POST_CONTEXT;


NTSTATUS ValueRecordAlloc(_In_opt_ PUNICODE_STRING Name, _In_ ULONG Type, _In_opt_ PVOID Data, _In_ ULONG DataLength, _Out_ PREGMAN_VALUE_RECORD *Record);
VOID ValueRecordReference(_Inout_ PREGMAN_VALUE_RECORD Record);
VOID ValueRecordDereference(_Inout_ PREGMAN_VALUE_RECORD Record);
NTSTATUS ValueRecordCallbackRegister(_In_ PREGMAN_VALUE_RECORD Record, _In_ const REGMAN_CALLBACKS *Callbacks, _Out_ PHANDLE Handle);
VOID ValueRecordCallbackUnregister(_In_ HANDLE Handle);

NTSTATUS ValueRecordCallbackQueryInvoke(_In_ PREGMAN_VALUE_RECORD Record, _Inout_ PVOID *Data, _Inout_ PULONG DataSize, _Inout_ PULONG Type);

NTSTATUS ValueRecordOnQueryValueEntry(_In_ PREGMAN_VALUE_RECORD Value, _Out_ PVOID *Data, _Out_ PULONG DataLength, _Out_ PULONG Type);
NTSTATUS ValueRecordOnQueryValue(_In_ PREGMAN_VALUE_RECORD Value, _Inout_ PREG_QUERY_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated);
NTSTATUS ValueRecordOnEnumValue(_In_ PREGMAN_VALUE_RECORD Value, _Inout_ PREG_ENUMERATE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated);
NTSTATUS ValueRecordOnSetValue(_In_ PREGMAN_VALUE_RECORD Value, _In_ PREG_SET_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated);
NTSTATUS ValueRecordOnDeleteValue(_In_ PREGMAN_VALUE_RECORD Value, _In_ PREG_DELETE_VALUE_KEY_INFORMATION Info, PBOOLEAN Emulated);
NTSTATUS ValueRecordOnPostOperation(PREG_POST_OPERATION_INFORMATION Info, PBOOLEAN Emulated);
VOID ValuePostRecordFree(_Inout_ PREGMAN_VALUE_POST_CONTEXT Record);



#endif
