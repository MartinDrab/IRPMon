
#include <ntifs.h>
#include "preprocessor.h"
#include "general-types.h"
#include "driver-settings.h"



/************************************************************************/
/*                    GLOBAL VARIABLES                                  */
/************************************************************************/

static IRPMNDRV_SETTINGS _globalSettings;
static UNICODE_STRING _uServiceKey;


/************************************************************************/
/*               HELPER FUNCTIONS                                       */
/************************************************************************/


static NTSTATUS _LoadFromRegistry(PUNICODE_STRING ServiceKey)
{
	OBJECT_ATTRIBUTES oa;
	HANDLE hParametersKey = NULL;
	HANDLE hServiceKey = NULL;
	UNICODE_STRING uName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ServiceKey=\"%wZ\"", ServiceKey);

	InitializeObjectAttributes(&oa, ServiceKey, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&hServiceKey, KEY_READ, &oa);
	if (NT_SUCCESS(status)) {
		RtlInitUnicodeString(&uName, L"Parameters");
		InitializeObjectAttributes(&oa, &uName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, hServiceKey, NULL);
		status = ZwOpenKey(&hParametersKey, KEY_QUERY_VALUE, &oa);
		if (NT_SUCCESS(status)) {
			const wchar_t *valueNames[] = {
				L"ReqQueueClearOnDisconnect",
				L"ReqQueueCollectWhenDisconnected",
				L"ProcessEventsCollect",
				L"FileObjectEventsCollect",
				L"DriverSnapshotEventsCollect",
				L"ProcessEmulateOnConnect",
				L"DriverSnapshotOnConnect",
			};
			BOOLEAN *settingsValues[] = {
				&_globalSettings.ReqQueueClearOnDisconnect,
				&_globalSettings.ReqQueueCollectWhenDisconnected,
				&_globalSettings.ProcessEventsCollect,
				&_globalSettings.FileObjectEventsCollect,
				&_globalSettings.DriverSnapshotEventsCollect,
				&_globalSettings.ProcessEmulateOnConnect,
				&_globalSettings.DriverSnapshotOnConnect,
			};
			ULONG retLength = 0;
			PKEY_VALUE_FULL_INFORMATION kvfi = NULL;

			for (size_t i = 0; i < sizeof(valueNames) / sizeof(valueNames[0]); ++i) {
				RtlInitUnicodeString(&uName, valueNames[i]);
				status = ZwQueryValueKey(hParametersKey, &uName, KeyValueFullInformation, NULL, 0, &retLength);
				if (status == STATUS_BUFFER_TOO_SMALL) {
					kvfi = ExAllocatePoolWithTag(PagedPool, retLength, 'MPRI');
					if (kvfi != NULL) {
						memset(kvfi, 0, retLength);
						status = ZwQueryValueKey(hParametersKey, &uName, KeyValueFullInformation, kvfi, retLength, &retLength);
						if (NT_SUCCESS(status)) {
							if (kvfi->Type == REG_DWORD && kvfi->DataLength == sizeof(ULONG))
								*(settingsValues[i]) = (*(PULONG)((unsigned char *)kvfi + kvfi->DataOffset) != 0);
						}
						
						ExFreePoolWithTag(kvfi, 'MPRI');
					} else status = STATUS_INSUFFICIENT_RESOURCES;
				} else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
					status = STATUS_SUCCESS;
			
				if (!NT_SUCCESS(status))
					break;
			}

			ZwClose(hParametersKey);
		} else status = STATUS_SUCCESS;

		ZwClose(hServiceKey);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}



static NTSTATUS _SaveToRegistry(PUNICODE_STRING ServiceKey)
{
	ULONG keyDisposition = 0;
	OBJECT_ATTRIBUTES oa;
	HANDLE hParametersKey = NULL;
	HANDLE hServiceKey = NULL;
	UNICODE_STRING uName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ServiceKey=\"%wZ\"", ServiceKey);

	InitializeObjectAttributes(&oa, ServiceKey, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	status = ZwOpenKey(&hServiceKey, KEY_WRITE, &oa);
	if (NT_SUCCESS(status)) {
		RtlInitUnicodeString(&uName, L"Parameters");
		InitializeObjectAttributes(&oa, &uName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, hServiceKey, NULL);
		status = ZwCreateKey(&hParametersKey, KEY_SET_VALUE, &oa, 0, NULL, 0, &keyDisposition);
		if (NT_SUCCESS(status)) {
			const wchar_t *valueNames[] = {
				L"ReqQueueClearOnDisconnect",
				L"ReqQueueCollectWhenDisconnected",
				L"ProcessEventsCollect",
				L"FileObjectEventsCollect",
				L"DriverSnapshotEventsCollect",
				L"ProcessEmulateOnConnect",
				L"DriverSnapshotOnConnect",
			};
			BOOLEAN *settingsValues[] = {
				&_globalSettings.ReqQueueClearOnDisconnect,
				&_globalSettings.ReqQueueCollectWhenDisconnected,
				&_globalSettings.ProcessEventsCollect,
				&_globalSettings.FileObjectEventsCollect,
				&_globalSettings.DriverSnapshotEventsCollect,
				&_globalSettings.ProcessEmulateOnConnect,
				&_globalSettings.DriverSnapshotOnConnect,
			};
			ULONG regValue = 0;

			for (size_t i = 0; i < sizeof(valueNames) / sizeof(valueNames[0]); ++i) {
				RtlInitUnicodeString(&uName, valueNames[i]);
				regValue = *(settingsValues[i]);
				status = ZwSetValueKey(hParametersKey, &uName, 0, REG_DWORD, &regValue, sizeof(regValue));
				if (!NT_SUCCESS(status))
					break;
			}

			ZwClose(hParametersKey);
		}

		ZwClose(hServiceKey);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/************************************************************************/
/*                    PUBLIC FUNCTIONS                                  */
/************************************************************************/


PIRPMNDRV_SETTINGS DriverSettingsGet(void)
{
	PIRPMNDRV_SETTINGS ret = NULL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = &_globalSettings;

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


NTSTATUS DriverSettingsSet(const IRPMNDRV_SETTINGS *Settings, BOOLEAN Save)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Settings=0x%p; Save=%u", Settings, Save);

	status = STATUS_SUCCESS;
	_globalSettings.DriverSnapshotEventsCollect = Settings->DriverSnapshotEventsCollect;
	_globalSettings.DriverSnapshotOnConnect = Settings->DriverSnapshotOnConnect;
	_globalSettings.FileObjectEventsCollect = Settings->FileObjectEventsCollect;
	_globalSettings.ProcessEmulateOnConnect = Settings->ProcessEmulateOnConnect;
	_globalSettings.ProcessEventsCollect = Settings->ProcessEventsCollect;
	_globalSettings.ReqQueueClearOnDisconnect = Settings->ReqQueueClearOnDisconnect;
	_globalSettings.ReqQueueCollectWhenDisconnected = Settings->ReqQueueCollectWhenDisconnected;
	if (Save)
		status = _SaveToRegistry(&_uServiceKey);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/************************************************************************/
/*                  INITIALIZATION AND FINALIZATION                     */
/************************************************************************/

NTSTATUS DriverSettingsInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("", DriverObject, RegistryPath, Context);

	_uServiceKey = *RegistryPath;
	_uServiceKey.MaximumLength = _uServiceKey.Length;
	_uServiceKey.Buffer = ExAllocatePoolWithTag(PagedPool, _uServiceKey.Length, 'MPRI');
	if (_uServiceKey.Buffer != NULL) {
		memcpy(_uServiceKey.Buffer, RegistryPath->Buffer, _uServiceKey.Length);
		_globalSettings.DriverSnapshotEventsCollect = TRUE;
		_globalSettings.DriverSnapshotOnConnect = FALSE;
		_globalSettings.FileObjectEventsCollect = TRUE;
		_globalSettings.ProcessEmulateOnConnect = TRUE;
		_globalSettings.ProcessEventsCollect = TRUE;
		_globalSettings.ReqQueueClearOnDisconnect = TRUE;
		_globalSettings.ReqQueueCollectWhenDisconnected = FALSE;
		status = _LoadFromRegistry(RegistryPath);
		if (NT_SUCCESS(status))
			status = _SaveToRegistry(RegistryPath);
	
		if (!NT_SUCCESS(status))
			ExFreePoolWithTag(_uServiceKey.Buffer, 'MPRI');
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID DriverSettingsFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("", DriverObject, RegistryPath, Context);

	ExFreePoolWithTag(_uServiceKey.Buffer, 'MPRI');

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
