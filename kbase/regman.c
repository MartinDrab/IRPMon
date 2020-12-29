
#include <ntifs.h>
#include "allocator.h"
#include "preprocessor.h"
#include "value-record.h"
#include "key-record.h"
#include "registry-callback.h"
#include "regman.h"


/************************************************************************/
/*                  GLOBAL VARIABLES                                   */
/************************************************************************/

static BOOLEAN _emulationSupported = FALSE;

/************************************************************************/
/*                PUBLIC FUNCTIONS                                      */
/************************************************************************/


NTSTATUS RegManKeyRegister(_In_ PUNICODE_STRING KeyName, _Out_ PHANDLE KeyHandle)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyName=\"%wZ\"; KeyHandle=0x%p", KeyName, KeyHandle);

	status = RegCallbackKeyRegister(KeyName, (PREGMAN_KEY_RECORD *)KeyHandle);

	DEBUG_EXIT_FUNCTION("0x%x, *KeyHandle=0x%p", status, *KeyHandle);
	return status;
}


VOID RegManKeyUnregister(_In_ HANDLE KeyHandle)
{
	PREGMAN_KEY_RECORD keyRecord = (PREGMAN_KEY_RECORD)KeyHandle;
	DEBUG_ENTER_FUNCTION("KeyHandle=0x%p", KeyHandle);

	RegCallbackKeyUnregister(keyRecord);
	KeyRecordDereference(keyRecord);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS RegManKeyValueAdd(_In_ HANDLE KeyHandle, _In_opt_ PUNICODE_STRING ValueName, _In_opt_ PVOID Data, _In_opt_ ULONG Length, _In_opt_ ULONG ValueType, _Out_ PHANDLE ValueHandle)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyHandle=0x%p; ValueName=\"%wZ\"; Data=0x%p; Length=%u; ValueType=%u; ValueHandle=0x%p", KeyHandle, ValueName, Data, Length, ValueType, ValueHandle);

	status = KeyRecordValueAdd((PREGMAN_KEY_RECORD)KeyHandle, ValueName, Data, Length, ValueType, (PREGMAN_VALUE_RECORD *)ValueHandle);

	DEBUG_EXIT_FUNCTION("0x%x, *ValueHandle=0x%p", status, *ValueHandle);
	return status;
}


VOID RegManKeyValueDelete(_In_ HANDLE ValueHandle)
{
	PREGMAN_VALUE_RECORD valueRecord = (PREGMAN_VALUE_RECORD)ValueHandle;
	DEBUG_ENTER_FUNCTION("ValueHandle=0x%p", ValueHandle);

	KeyRecordValueDelete(valueRecord->KeyRecord, &valueRecord->Item.Key.String);
	ValueRecordDereference(valueRecord);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS RegManValueCallbacksRegister(_In_ HANDLE ValueHandle, _In_ const REGMAN_CALLBACKS *Callbacks, _Out_ PHANDLE CallbackHandle)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ValueHandle=0x%p; Callbacks=0x%p; CallbackHandle=0x%p", ValueHandle, Callbacks, CallbackHandle);

	status = ValueRecordCallbackRegister((PREGMAN_VALUE_RECORD)ValueHandle, Callbacks, CallbackHandle);

	DEBUG_EXIT_FUNCTION("0x%x, *CallbackHandle=0x%p", status, *CallbackHandle);
	return status;
}



VOID RegManValueCallbackUnregiser(_In_ HANDLE CallbackHandle)
{
	DEBUG_ENTER_FUNCTION("CallbackHandle=0x%p", CallbackHandle);

	ValueRecordCallbackUnregister(CallbackHandle);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS RegManRawCallbackRegister(EX_CALLBACK_FUNCTION *Callback, void* Context, PHANDLE Handle)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Callback=0x%p; Context=0x%p; Handle=0xp", Callback, Context, Handle);

	status = RegRawCallbackRegister(Callback, Context, Handle);

	DEBUG_EXIT_FUNCTION("0x%x, *Handle=0x%p", status, *Handle);
	return status;
}


void RegManRawCallbackUnregister(HANDLE Handle)
{
	DEBUG_ENTER_FUNCTION("Handle=0x%p", Handle);

	RegRawCallbackUnregister(Handle);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS RegManModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	RTL_OSVERSIONINFOW versionInfo;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);

	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	status = RtlGetVersion(&versionInfo);
	if (NT_SUCCESS(status)) {
		_emulationSupported = (versionInfo.dwMajorVersion >= 6);
		if (_emulationSupported)
			status = RegCallbackModuleInit(NULL, RegistryPath, NULL);		
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void RegManModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	if (_emulationSupported)
		RegCallbackModuleFinit(NULL, NULL, NULL);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
