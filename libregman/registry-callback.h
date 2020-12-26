
#ifndef __REGMAN_REGISTRY_CALLBACK_H__
#define __REGMAN_REGISTRY_CALLBACK_H__



#include <ntifs.h>
#include "value-record.h"
#include "key-record.h"



typedef struct _RAW_CALLBACK_RECORD {
	LIST_ENTRY Entry;
	EX_CALLBACK_FUNCTION *Callback;
	void *Context;
} RAW_CALLBACK_RECORD, *PRAW_CALLBACK_RECORD;



NTSTATUS RegCallbackKeyRegister(_In_ PUNICODE_STRING KeyName, PREGMAN_KEY_RECORD *KeyRecord);
NTSTATUS RegCallbackKeyUnregister(PREGMAN_KEY_RECORD KeyRecord);
NTSTATUS RegRawCallbackRegister(EX_CALLBACK_FUNCTION *Callback, void *Context, PHANDLE Handle);
void RegRawCallbackUnregister(HANDLE Handle);

NTSTATUS RegCallbackModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context);
VOID RegCallbackModuleFinit(_In_ PDRIVER_OBJECT DriverObject, _In_opt_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context);




#endif
