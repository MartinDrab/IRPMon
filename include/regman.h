
#ifndef __REGMAN_H__
#define __REGMAN_H__


#include <ntifs.h>



typedef struct _REGMAN_VALUE_INFO {
	PVOID ValueRecord;
	ULONG CurrentType;
	PVOID CurrentData;
	ULONG CurrentDataSize;
} REGMAN_QUERY_INFO, *PREGMAN_VALUE_INFO;

typedef NTSTATUS(REGMAN_VALUE_QUERY_CALLBACK)(_In_ PREGMAN_VALUE_INFO ValueInfo, _In_opt_ PVOID Context);
typedef NTSTATUS(REGMAN_VALUE_SET_CALLBACK)(_In_ PREGMAN_VALUE_INFO ValueInfo, _In_opt_ PVOID Context);



NTSTATUS RegManKeyRegister(_In_ PUNICODE_STRING KeyName, _Out_ PHANDLE KeyHandle);
VOID RegManKeyUnregister(_In_ HANDLE KeyHandle);
NTSTATUS RegManKeyValueAdd(_In_ HANDLE KeyHandle, _In_opt_ PUNICODE_STRING ValueName, _In_opt_ PVOID Data, _In_opt_ ULONG Length, _In_opt_ ULONG ValueType, _Out_ PHANDLE ValueHandle);
VOID RegManKeyValueDelete(_In_ HANDLE ValueHandle);
NTSTATUS RegManValueCallbacksRegister(_In_ HANDLE ValueHandle, _In_ REGMAN_VALUE_QUERY_CALLBACK *QueryCallback, _In_ REGMAN_VALUE_SET_CALLBACK *SetCallback, _In_opt_ PVOID Context, _Out_ PHANDLE CallbackHandle);
VOID RegManValueCallbackUnregiser(_In_ HANDLE CallbackHandle);

NTSTATUS RegManInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath, PVOID Context);
VOID RegManFinit(_In_ PDRIVER_OBJECT DriverObject, _In_opt_ PUNICODE_STRING RegistryPath, PVOID Context);



#endif
