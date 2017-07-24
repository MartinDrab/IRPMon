
#ifndef __REGMAN_REGISTRY_CALLBACK_H__
#define __REGMAN_REGISTRY_CALLBACK_H__



#include <ntifs.h>
#include "value-record.h"
#include "key-record.h"



NTSTATUS RegCallbackKeyRegister(_In_ PUNICODE_STRING KeyName, PREGMAN_KEY_RECORD *KeyRecord);
NTSTATUS RegCallbackKeyUnregister(PREGMAN_KEY_RECORD KeyRecord);


NTSTATUS RegCallbackModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context);
VOID RegCallbackModuleFinit(_In_ PDRIVER_OBJECT DriverObject, _In_opt_ PUNICODE_STRING RegistryPath, _In_opt_ PVOID Context);




#endif
