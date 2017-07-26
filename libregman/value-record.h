
#ifndef __REGMAN_VALUE_RECORD_H__
#define __REGMAN_VALUE_RECORD_H__

#include <ntifs.h>
#include "string-ref-table.h"
#include "regman.h"



typedef struct _REGMAN_VALUE_QUERY_CALLBACK_RECORD {
	LIST_ENTRY ListEntry;
	struct _REGMAN_VALUE_RECORD *Record;
	REGMAN_VALUE_QUERY_CALLBACK *QueryRoutine;
	REGMAN_VALUE_SET_CALLBACK *SetRoutine;
	PVOID Context;
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



NTSTATUS ValueRecordAlloc(_In_opt_ PUNICODE_STRING Name, _In_ ULONG Type, _In_opt_ PVOID Data, _In_ ULONG DataLength, _Out_ PREGMAN_VALUE_RECORD *Record);
VOID ValueRecordReference(_Inout_ PREGMAN_VALUE_RECORD Record);
VOID ValueRecordDereference(_Inout_ PREGMAN_VALUE_RECORD Record);
NTSTATUS ValueRecordCallbackRegister(_In_ PREGMAN_VALUE_RECORD Record, _In_ _In_opt_ REGMAN_VALUE_QUERY_CALLBACK *QueryRoutine, _In_opt_ REGMAN_VALUE_SET_CALLBACK *SetRoutine, _In_opt_ PVOID Context, _Out_ PHANDLE Handle);
VOID ValueRecordCallbackUnregister(_In_ HANDLE Handle);

NTSTATUS ValueRecordCallbackQueryInvoke(_In_ PREGMAN_VALUE_RECORD Record, _Inout_ PVOID *Data, _Inout_ PULONG DataSize, _Inout_ PULONG Type);
NTSTATUS ValueRecordCallbackSetInvoke(_In_ PREGMAN_VALUE_RECORD Record, _Inout_ PVOID *Data, _Inout_ PULONG DataSize, _Inout_ PULONG Type);

NTSTATUS ValueRecordOnQueryValueEntry(_In_ PREGMAN_VALUE_RECORD Value, _Out_ PVOID *Data, _Out_ PULONG DataLength, _Out_ PULONG Type);
NTSTATUS ValueRecordOnQueryValue(_In_ PREGMAN_VALUE_RECORD Value, _Inout_ PREG_QUERY_VALUE_KEY_INFORMATION Info);
NTSTATUS ValueRecordOnEnumValue(_In_ PREGMAN_VALUE_RECORD Value, _Inout_ PREG_ENUMERATE_VALUE_KEY_INFORMATION Info);
NTSTATUS ValueRecordOnSetValue(_In_ PREGMAN_VALUE_RECORD Value, _In_ PREG_SET_VALUE_KEY_INFORMATION Info);
NTSTATUS ValueRecordOnDeleteValue(_In_ PREGMAN_VALUE_RECORD Value, _In_ PREG_DELETE_VALUE_KEY_INFORMATION Info);

NTSTATUS ValueRecordModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void ValueRecordModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
