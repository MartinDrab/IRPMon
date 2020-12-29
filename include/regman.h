
#ifndef __REGMAN_H__
#define __REGMAN_H__


#include <ntifs.h>


#define REGMAN_POOL_TAG			(ULONG)'nMgR'

typedef struct _REGMAN_VALUE_QUERY_INFO {
	PVOID ValueRecord;
	ULONG CurrentType;
	PVOID CurrentData;
	ULONG CurrentDataSize;
} REGMAN_QUERY_INFO, *PREGMAN_VALUE_INFO;

typedef struct _REGMAN_DELETE_INFO {
	PVOID ValueRecord;
	BOOLEAN StopEmulation;
} REGMAN_DELETE_INFO, *PREGMAN_DELETE_INFO;

typedef NTSTATUS(REGMAN_VALUE_QUERY_CALLBACK)(_In_ PREGMAN_VALUE_INFO ValueInfo, _In_opt_ PVOID Context);
typedef NTSTATUS(REGMAN_VALUE_SET_CALLBACK)(_In_ PREGMAN_VALUE_INFO ValueInfo, _In_opt_ PVOID Context);
typedef NTSTATUS(REGMAN_VALUE_DELETE_CALLBACK)(_In_ PREGMAN_DELETE_INFO Info, _In_opt_ PVOID Context);

typedef struct _REGMAN_CALLBACKS {
	REGMAN_VALUE_QUERY_CALLBACK *QueryValue;
	REGMAN_VALUE_SET_CALLBACK *SetValue;
	REGMAN_VALUE_DELETE_CALLBACK *DeleteValue;
	PVOID Context;
} REGMAN_CALLBACKS, *PREGMAN_CALLBACKS;

NTSTATUS RegManKeyRegister(_In_ PUNICODE_STRING KeyName, _Out_ PHANDLE KeyHandle);
VOID RegManKeyUnregister(_In_ HANDLE KeyHandle);
NTSTATUS RegManKeyValueAdd(_In_ HANDLE KeyHandle, _In_opt_ PUNICODE_STRING ValueName, _In_opt_ PVOID Data, _In_opt_ ULONG Length, _In_opt_ ULONG ValueType, _Out_ PHANDLE ValueHandle);
VOID RegManKeyValueDelete(_In_ HANDLE ValueHandle);
NTSTATUS RegManValueCallbacksRegister(_In_ HANDLE ValueHandle, _In_ const REGMAN_CALLBACKS *Callbacks, _Out_ PHANDLE CallbackHandle);
VOID RegManValueCallbackUnregiser(_In_ HANDLE CallbackHandle);
NTSTATUS RegManRawCallbackRegister(EX_CALLBACK_FUNCTION *Callback, void *Context, PHANDLE Handle);
void RegManRawCallbackUnregister(HANDLE Handle);

NTSTATUS RegManModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void RegManModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
