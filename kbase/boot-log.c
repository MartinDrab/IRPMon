
#include <ntifs.h>
#include <fltKernel.h>
#include <ntstrsafe.h>
#include "allocator.h"
#include "utils.h"
#include "preprocessor.h"
#include "general-types.h"
#include "request.h"
#include "req-queue.h"
#include "driver-settings.h"
#include "pnp-driver-watch.h"
#include "regman.h"
#include "boot-log.h"



static LIST_ENTRY _blRequestListHead;
static EX_PUSH_LOCK _blRequestListLock;
static LIST_ENTRY _blNPCache;
static KSPIN_LOCK _blNPLock;
static volatile BOOLEAN _blEnabled = FALSE;
static PETHREAD _blSavingThread = NULL;
static UNICODE_STRING _registryPath;
static HANDLE _blRegCallbackHandle = NULL;
static ULONG _currentControlSetNumber = 0;
static HANDLE _rqCallbackHandle = NULL;


static void _ListHeadMove(PLIST_ENTRY Original, PLIST_ENTRY New)
{
	InitializeListHead(New);
	if (!IsListEmpty(Original)) {
		*New = *Original;
		New->Flink->Blink = New;
		New->Blink->Flink = New;
		InitializeListHead(Original);
	}

	return;
}


static void _AddTailList(PLIST_ENTRY Head, PLIST_ENTRY List)
{
	if (!IsListEmpty(List)) {
		List->Flink->Blink = Head->Blink;
		List->Blink->Flink = Head;
		Head->Blink->Flink = List->Flink;
		Head->Blink = List->Blink;
		InitializeListHead(List);
	}

	return;
}

static void _NPCacheInsert(PREQUEST_HEADER Request)
{
	KIRQL irql;
	DEBUG_ENTER_FUNCTION("Request=0x%p", Request);

	KeAcquireSpinLock(&_blNPLock, &irql);
	InsertTailList(&_blNPCache, &Request->Entry);
	KeReleaseSpinLock(&_blNPLock, irql);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static void _NPCacheFlush(PLIST_ENTRY Head)
{
	KIRQL irql;
	DEBUG_ENTER_FUNCTION("Head=0x%p", Head);

	KeAcquireSpinLock(&_blNPLock, &irql);
	_ListHeadMove(&_blNPCache, Head);
	KeReleaseSpinLock(&_blNPLock, irql);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _FlushBootRequests(HANDLE FileHandle)
{
	ULONG requestSize = 0;
	IO_STATUS_BLOCK iosb;
	LIST_ENTRY reqsToSave;
	PREQUEST_HEADER tmp = NULL;
	PREQUEST_HEADER old = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("FileHandle=0x%p", FileHandle);

	status = STATUS_SUCCESS;
	InitializeListHead(&reqsToSave);
	_NPCacheFlush(&reqsToSave);
	FltAcquirePushLockExclusive(&_blRequestListLock);
	_AddTailList(&_blRequestListHead, &reqsToSave);
	_ListHeadMove(&_blRequestListHead, &reqsToSave);
	FltReleasePushLock(&_blRequestListLock);
	tmp = CONTAINING_RECORD(reqsToSave.Flink, REQUEST_HEADER, Entry);
	while (&tmp->Entry != &reqsToSave) {
		old = tmp;
		tmp = CONTAINING_RECORD(tmp->Entry.Flink, REQUEST_HEADER, Entry);
		requestSize = (ULONG)RequestGetSize(old);
		RtlSecureZeroMemory(&old->Entry, sizeof(old->Entry));
		status = ZwWriteFile(FileHandle, NULL, NULL, NULL, &iosb, &requestSize, sizeof(requestSize), NULL, NULL);
		if (NT_SUCCESS(status))
			status = ZwWriteFile(FileHandle, NULL, NULL, NULL, &iosb, old, requestSize, NULL, NULL);
		
		if (!NT_SUCCESS(status)) {
			DEBUG_ERROR("Unable to save request 0x%p to boot log: 0x%p", old, status);
		}

		RequestMemoryFree(old);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static void _BLSaveThread(PVOID Context)
{
	IO_STATUS_BLOCK iosb;
	OBJECT_ATTRIBUTES oa;
	HANDLE hFile = NULL;
	LARGE_INTEGER timeout;
	wchar_t fileNameBuffer[260];
	UNICODE_STRING uFileName;
	BINARY_LOG_HEADER hdr;
	LARGE_INTEGER time;
	TIME_FIELDS timeFields;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Context=0x%p", Context);

	KeQuerySystemTime(&time);
	RtlTimeToTimeFields(&time, &timeFields);
	RtlSecureZeroMemory(&hdr, sizeof(hdr));
	hdr.Signature = LOGHEADER_SIGNATURE;
	hdr.Version = LOGHEADER_VERSION;
#ifdef _X86_
	hdr.Architecture = LOGHEADER_ARCHITECTURE_X86;
#elif defined(_AMD64_)
	hdr.Architecture = LOGHEADER_ARCHITECTURE_X64;
#else
#error Unsupported architecture
#endif
	timeout.QuadPart = -10000000;
	uFileName.Length = 0;
	uFileName.MaximumLength = sizeof(fileNameBuffer);
	uFileName.Buffer = fileNameBuffer;
	status = RtlUnicodeStringPrintf(&uFileName, L"\\SystemRoot\\IRPMon-%.4u-%.2u-%.2u %.2u-%.2u-%.2u.bin", timeFields.Year, timeFields.Month, timeFields.Day, timeFields.Hour, timeFields.Minute, timeFields.Second);
	if (NT_SUCCESS(status)) {
		InitializeObjectAttributes(&oa, &uFileName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
		while (BLEnabled()) {
			status = ZwCreateFile(&hFile, GENERIC_WRITE | SYNCHRONIZE, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_SUPERSEDE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
			if (NT_SUCCESS(status)) {
				status = ZwWriteFile(hFile, NULL, NULL, NULL, &iosb, &hdr, sizeof(hdr), NULL, NULL);
				if (!NT_SUCCESS(status)) {
					DEBUG_ERROR("nable to write boot log file header: 0x%x", status);
				}
			
				if (NT_SUCCESS(status)) {
					while (BLEnabled()) {
						status = _FlushBootRequests(hFile);
						KeDelayExecutionThread(KernelMode, FALSE, &timeout);
					}
				}

				status = _FlushBootRequests(hFile);
				ZwClose(hFile);
			} else {
				DEBUG_ERROR("ZwCreateFile(\"%wZ\"): 0x%x", &uFileName, status);
			}

			KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}
	} else {
		DEBUG_ERROR("RtlUnicodeStringPrintf: 0x%x", status);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _RegReadUInt32(HANDLE KeyHandle, PUNICODE_STRING ValueName, PULONG Value)
{
	ULONG retLength = 0;
	PKEY_VALUE_PARTIAL_INFORMATION kvpi = NULL;
	unsigned char buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyHandle=0x%p; ValueName=\"%wZ\"; Value=0x%p", KeyHandle, ValueName, Value);

	kvpi = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
	status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, kvpi, sizeof(buffer), &retLength);
	if (NT_SUCCESS(status)) {
		if (kvpi->Type == REG_DWORD && kvpi->DataLength == sizeof(ULONG))
			*Value = *(PULONG)kvpi->Data;
		else status = STATUS_OBJECT_TYPE_MISMATCH;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Value=%u", status, *Value);
	return status;
}


static NTSTATUS _RegReadString(HANDLE KeyHandle, PUNICODE_STRING ValueName, PUNICODE_STRING US)
{
	ULONG retLength = 0;
	wchar_t *data = NULL;
	PKEY_VALUE_PARTIAL_INFORMATION kvpi = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyHandle=0x%p; ValueName=\"%wZ\"; US=0x%p", KeyHandle, ValueName, US);

	RtlSecureZeroMemory(US, sizeof(UNICODE_STRING));
	status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, NULL, 0, &retLength);
	if (status == STATUS_BUFFER_TOO_SMALL) {
		kvpi = HeapMemoryAllocPaged(retLength);
		if (kvpi != NULL) {
			status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, kvpi, retLength, &retLength);
			if (NT_SUCCESS(status)) {
				if (kvpi->Type != REG_SZ && kvpi->Type != REG_EXPAND_SZ)
					status = STATUS_OBJECT_TYPE_MISMATCH;

				if (NT_SUCCESS(status) && kvpi->DataLength % 2 != 0)
					status = STATUS_INVALID_PARAMETER;

				if (NT_SUCCESS(status) && kvpi->DataLength > 0) {
					data = (wchar_t *)kvpi->Data;
					data += (kvpi->DataLength / sizeof(wchar_t)) - 1;
					if (*data == L'\0')
						kvpi->DataLength -= sizeof(wchar_t);

					US->Length = (USHORT)kvpi->DataLength;
					US->MaximumLength = US->Length;
					US->Buffer = HeapMemoryAllocPaged(US->Length);
					if (US->Buffer != NULL)
						memcpy(US->Buffer, kvpi->Data, US->Length);
					else status = STATUS_INSUFFICIENT_RESOURCES;
				}
			}

			HeapMemoryFree(kvpi);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	DEBUG_EXIT_FUNCTION("0x%x, US=\"%wZ\"", status, US);
	return status;
}

static NTSTATUS _LoadDriverMonitoringSettings(HANDLE KeyHandle, PDRIVER_MONITOR_SETTINGS Settings)
{
	const wchar_t *valueNames[] = {
		L"NewDevices",
		L"IRP",
		L"FastIo",
		L"IRPCompletion",
		L"StartIo",
		L"AddDevice",
		L"Unload",
		L"Data",
		L"StackTrace",
	};
	PBOOLEAN values[] = {
		&Settings->MonitorNewDevices,
		&Settings->MonitorIRP,
		&Settings->MonitorFastIo,
		&Settings->MonitorIRPCompletion,
		&Settings->MonitorStartIo,
		&Settings->MonitorAddDevice,
		&Settings->MonitorUnload,
		&Settings->MonitorData,
		&Settings->MonitorStackTrace,
	};
	ULONG value = 0;
	UNICODE_STRING uValueName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyHandle=0x%p; Settings=0x%p", KeyHandle, Settings);

	status = STATUS_SUCCESS;
	RtlSecureZeroMemory(Settings, sizeof(DRIVER_MONITOR_SETTINGS));
	for (size_t i = 0; i < sizeof(Settings->IRPSettings) / sizeof(Settings->IRPSettings[0]); ++i)
		Settings->IRPSettings[i] = TRUE;

	for (size_t i = 0; i < sizeof(Settings->FastIoSettings) / sizeof(Settings->FastIoSettings[0]); ++i)
		Settings->FastIoSettings[i] = TRUE;

	for (size_t i = 0; i < sizeof(valueNames) / sizeof(valueNames[0]); ++i) {
		RtlInitUnicodeString(&uValueName, valueNames[i]);
		status = _RegReadUInt32(KeyHandle, &uValueName, &value);
		if (NT_SUCCESS(status))
			*(values[i]) = (value != 0);
	
		if (status == STATUS_OBJECT_NAME_NOT_FOUND)
			status = STATUS_SUCCESS;

		if (!NT_SUCCESS(status))
			break;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _SaveDriverMonitoringSettings(HANDLE KeyHandle, const DRIVER_MONITOR_SETTINGS *Settings)
{
	const wchar_t* valueNames[] = {
		L"NewDevices",
		L"IRP",
		L"FastIo",
		L"IRPCompletion",
		L"StartIo",
		L"AddDevice",
		L"Unload",
		L"Data",
		L"StackTrace",
	};
	const BOOLEAN *values[] = {
		&Settings->MonitorNewDevices,
		&Settings->MonitorIRP,
		&Settings->MonitorFastIo,
		&Settings->MonitorIRPCompletion,
		&Settings->MonitorStartIo,
		&Settings->MonitorAddDevice,
		&Settings->MonitorUnload,
		&Settings->MonitorData,
		&Settings->MonitorStackTrace,
	};
	ULONG value = 0;
	UNICODE_STRING uValueName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyHandle=0x%p; Settings=0x%p", KeyHandle, Settings);

	status = STATUS_SUCCESS;
	for (size_t i = 0; i < sizeof(valueNames) / sizeof(valueNames[0]); ++i) {
		value = *(values[i]);
		RtlInitUnicodeString(&uValueName, valueNames[i]);
		status = ZwSetValueKey(KeyHandle, &uValueName, 0, REG_DWORD, &value, sizeof(value));
		if (!NT_SUCCESS(status))
			break;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _LoadSettings(PUNICODE_STRING RegistryPath)
{
	ULONG index = 0;
	ULONG retLength = 0;
	PKEY_BASIC_INFORMATION kbi = NULL;
	HANDLE driverKey = NULL;
	HANDLE blKey = NULL;
	HANDLE servicesKey = NULL;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uName;
	UNICODE_STRING uValueName;
	DRIVER_MONITOR_SETTINGS monitorSettings;
	PIRPMNDRV_SETTINGS globalSettings = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("RegistryPath=\"%wZ\"", RegistryPath);

	status = STATUS_SUCCESS;
	globalSettings = DriverSettingsGet();
	_blEnabled = globalSettings->LogBoot;
	if (_blEnabled) {
		InitializeObjectAttributes(&oa, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
		status = ZwOpenKey(&servicesKey, KEY_READ, &oa);
		if (NT_SUCCESS(status)) {
			RtlInitUnicodeString(&uName, L"BootLogging");
			InitializeObjectAttributes(&oa, &uName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, servicesKey, NULL);
			status = ZwOpenKey(&blKey, KEY_READ, &oa);
			if (NT_SUCCESS(status)) {
				do {
					status = ZwEnumerateKey(blKey, index, KeyBasicInformation, NULL, 0, &retLength);
					if (status == STATUS_BUFFER_TOO_SMALL) {
						kbi = HeapMemoryAllocPaged(retLength);
						if (kbi != NULL) {
							status = ZwEnumerateKey(blKey, index, KeyBasicInformation, kbi, retLength, &retLength);
							if (NT_SUCCESS(status)) {
								uName.Length = (USHORT)kbi->NameLength;
								uName.MaximumLength = uName.Length;
								uName.Buffer = kbi->Name;
								InitializeObjectAttributes(&oa, &uName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, blKey, NULL);
								status = ZwOpenKey(&driverKey, KEY_READ, &oa);
							}
						
							HeapMemoryFree(kbi);
						} else status = STATUS_INSUFFICIENT_RESOURCES;
					}

					if (NT_SUCCESS(status)) {
						RtlInitUnicodeString(&uValueName, L"DriverObjectName");
						status = _RegReadString(driverKey, &uValueName, &uName);
						if (NT_SUCCESS(status)) {
							status = _LoadDriverMonitoringSettings(driverKey, &monitorSettings);
							if (NT_SUCCESS(status))
								status = PWDDriverNameRegister(&uName, &monitorSettings);
				
							HeapMemoryFree(uName.Buffer);
						}

						ZwClose(driverKey);
					}

					++index;
				} while (NT_SUCCESS(status));

				if (status == STATUS_NO_MORE_ENTRIES)
					status = STATUS_SUCCESS;

				ZwClose(blKey);
			}

			ZwClose(servicesKey);
		}

		if (status == STATUS_OBJECT_NAME_NOT_FOUND)
			status = STATUS_SUCCESS;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


static NTSTATUS _BLRegistryCallback(PVOID Context, PVOID Argument1, PVOID Argument2)
{
	UNICODE_STRING uKeyName;
	BOOLEAN serviceKey = FALSE;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREG_KEY_HANDLE_CLOSE_INFORMATION closeInfo = NULL;
	REG_NOTIFY_CLASS regOpClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;
	wchar_t prefixBuffer[260];
	UNICODE_STRING uPrefix;
	UNICODE_STRING tmp;

	if (_blEnabled && PsGetCurrentProcess() == PsInitialSystemProcess && regOpClass == RegNtPreKeyHandleClose) {
		closeInfo = (PREG_KEY_HANDLE_CLOSE_INFORMATION)Argument2;
		status = _GetObjectName(closeInfo->Object, &uKeyName);
		if (NT_SUCCESS(status)) {
			uPrefix.Length = 0;
			uPrefix.MaximumLength = sizeof(prefixBuffer);
			uPrefix.Buffer = prefixBuffer;
			status = RtlUnicodeStringPrintf(&uPrefix, L"\\Registry\\Machine\\SYSTEM\\ControlSet%.3u\\services\\", _currentControlSetNumber);
			if (NT_SUCCESS(status)) {
				tmp.Buffer = uKeyName.Buffer;
				tmp.Length = uPrefix.Length;
				tmp.MaximumLength = tmp.Length;
				if (RtlEqualUnicodeString(&uPrefix, &tmp, TRUE)) {
					serviceKey = TRUE;
					tmp.Length = uKeyName.Length - uPrefix.Length;
					tmp.MaximumLength = tmp.Length;
					tmp.Buffer = uKeyName.Buffer + (uKeyName.Length - tmp.Length) / sizeof(wchar_t);
					for (size_t i = 0; i < tmp.Length / sizeof(wchar_t); ++i) {
						if (tmp.Buffer[i] == L'\\') {
							serviceKey = FALSE;
							break;
						}
					}

					if (serviceKey)
						PDWCheckDrivers();
				}
			} else {
				DEBUG_ERROR("RtlUnicodeStringPrintf(%u): 0x%x", _currentControlSetNumber, status);
			}

			HeapMemoryFree(uKeyName.Buffer);
		} else {
			DEBUG_ERROR("_GetObjectName: 0x%p", status);
		}
	}

	status = STATUS_SUCCESS;

	return status;
}


static void _BLOnRequest(PREQUEST_HEADER Request, void* Context)
{
	LIST_ENTRY cachedHead;
	DEBUG_ENTER_FUNCTION("Request=0x%p; Context=0x%p", Request, Context);

	if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
		InitializeListHead(&cachedHead);
		_NPCacheFlush(&cachedHead);
		FltAcquirePushLockExclusive(&_blRequestListLock);
		_AddTailList(&_blRequestListHead, &cachedHead);
		InsertTailList(&_blRequestListHead, &Request->Entry);
		FltReleasePushLock(&_blRequestListLock);
	} else _NPCacheInsert(Request);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS BLDriverNameSave(PUNICODE_STRING DriverName, const DRIVER_MONITOR_SETTINGS* Settings)
{
	HANDLE driverKey = NULL;
	HANDLE serviceKey = NULL;
	HANDLE blKey = NULL;
	UNICODE_STRING uName;
	UNICODE_STRING uValueName;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverName=\"%wZ\"; Settings=0x%p", DriverName, Settings);

	InitializeObjectAttributes(&oa, &_registryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenKey(&serviceKey, KEY_WRITE, &oa);
	if (NT_SUCCESS(status)) {
		RtlInitUnicodeString(&uName, L"BootLogging");
		InitializeObjectAttributes(&oa, &uName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, serviceKey, NULL);
		status = ZwCreateKey(&blKey, KEY_WRITE, &oa, 0, NULL, 0, NULL);
		if (NT_SUCCESS(status)) {
			uName = *DriverName;
			uName.MaximumLength = uName.Length;
			uName.Buffer = HeapMemoryAllocPaged(uName.Length);
			if (uName.Buffer != NULL) {
				memcpy(uName.Buffer, DriverName->Buffer, uName.Length);
				for (size_t i = 0; i < uName.Length / sizeof(wchar_t); ++i) {
					if (uName.Buffer[i] == L'\\')
						uName.Buffer[i] = L'_';
				}
			} else status = STATUS_INSUFFICIENT_RESOURCES;

			if (NT_SUCCESS(status)) {
				InitializeObjectAttributes(&oa, &uName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, blKey, NULL);
				status = ZwCreateKey(&driverKey, KEY_WRITE, &oa, 0, NULL, 0, NULL);
				if (NT_SUCCESS(status)) {
					RtlInitUnicodeString(&uValueName, L"DriverObjectName");
					status = ZwSetValueKey(driverKey, &uValueName, 0, REG_SZ, DriverName->Buffer, DriverName->Length);
					if (NT_SUCCESS(status))
						status = _SaveDriverMonitoringSettings(driverKey, Settings);
					
					ZwClose(driverKey);
				}

				HeapMemoryFree(uName.Buffer);
			}

			ZwClose(blKey);
		}

		ZwClose(serviceKey);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS BLDriverNameDelete(PUNICODE_STRING DriverName)
{
	HANDLE driverKey = NULL;
	HANDLE serviceKey = NULL;
	HANDLE blKey = NULL;
	UNICODE_STRING uName;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverName=\"%wZ\"", DriverName);

	InitializeObjectAttributes(&oa, &_registryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenKey(&serviceKey, KEY_WRITE, &oa);
	if (NT_SUCCESS(status)) {
		RtlInitUnicodeString(&uName, L"BootLogging");
		InitializeObjectAttributes(&oa, &uName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, serviceKey, NULL);
		status = ZwCreateKey(&blKey, KEY_WRITE, &oa, 0, NULL, 0, NULL);
		if (NT_SUCCESS(status)) {
			uName = *DriverName;
			uName.MaximumLength = uName.Length;
			uName.Buffer = HeapMemoryAllocPaged(uName.Length);
			if (uName.Buffer != NULL) {
				memcpy(uName.Buffer, DriverName->Buffer, uName.Length);
				for (size_t i = 0; i < uName.Length / sizeof(wchar_t); ++i) {
					if (uName.Buffer[i] == L'\\')
						uName.Buffer[i] = L'_';
				}
			} else status = STATUS_INSUFFICIENT_RESOURCES;

			if (NT_SUCCESS(status)) {
				InitializeObjectAttributes(&oa, &uName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, blKey, NULL);
				status = ZwCreateKey(&driverKey, DELETE, &oa, 0, NULL, 0, NULL);
				if (NT_SUCCESS(status)) {
					status = ZwDeleteKey(driverKey);
					ZwClose(driverKey);
				}

				HeapMemoryFree(uName.Buffer);
			}

			ZwClose(blKey);
		}

		ZwClose(serviceKey);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


BOOLEAN BLEnabled(void)
{
	return _blEnabled;
}


void BLDisable(void)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	if (_blEnabled) {
		RequestQueueCallbackUnregister(_rqCallbackHandle);
		_blEnabled = FALSE;
	}

	KeWaitForSingleObject(_blSavingThread, Executive, KernelMode, FALSE, NULL);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS BLModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	HANDLE hThread = NULL;
	CLIENT_ID clientId;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("", DriverObject, RegistryPath, Context);

	InitializeListHead(&_blRequestListHead);
	FltInitializePushLock(&_blRequestListLock);
	InitializeListHead(&_blNPCache);
	KeInitializeSpinLock(&_blNPLock);
	_registryPath = *RegistryPath;
	_registryPath.MaximumLength = _registryPath.Length;
	_registryPath.Buffer = HeapMemoryAllocPaged(_registryPath.Length);
	if (_registryPath.Buffer != NULL) {
		memcpy(_registryPath.Buffer, RegistryPath->Buffer, _registryPath.Length);
		status = _LoadSettings(RegistryPath);
		if (NT_SUCCESS(status)) {
			status = UtilsGetCurrentControlSetNumber(&_currentControlSetNumber);
			if (NT_SUCCESS(status)) {
				status = RequestQueueCallbackRegister(_BLOnRequest, NULL, &_rqCallbackHandle);
				if (NT_SUCCESS(status)) {
					status = RegManRawCallbackRegister(_BLRegistryCallback, NULL, &_blRegCallbackHandle);
					if (NT_SUCCESS(status)) {
						InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
						status = PsCreateSystemThread(&hThread, SYNCHRONIZE, &oa, NULL, &clientId, _BLSaveThread, NULL);
						if (NT_SUCCESS(status)) {
							status = ObReferenceObjectByHandle(hThread, SYNCHRONIZE, *PsThreadType, KernelMode, &_blSavingThread, NULL);
							ZwClose(hThread);
						}

						if (!NT_SUCCESS(status))
							RegManRawCallbackUnregister(_blRegCallbackHandle);
					}

					if (!NT_SUCCESS(status))
						RequestQueueCallbackUnregister(_rqCallbackHandle);
				}
			}
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(_registryPath.Buffer);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void BLModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	PREQUEST_HEADER tmp = NULL;
	PREQUEST_HEADER old = NULL;
	DEBUG_ENTER_FUNCTION("", DriverObject, RegistryPath, Context);

	BLDisable();
	RegManRawCallbackUnregister(_blRegCallbackHandle);
	ObDereferenceObject(_blSavingThread);
	HeapMemoryFree(_registryPath.Buffer);
	tmp = CONTAINING_RECORD(_blNPCache.Flink, REQUEST_HEADER, Entry);
	while (&tmp->Entry != &_blNPCache) {
		old = tmp;
		tmp = CONTAINING_RECORD(tmp->Entry.Flink, REQUEST_HEADER, Entry);
		RequestMemoryFree(old);
	}

	tmp = CONTAINING_RECORD(_blRequestListHead.Flink, REQUEST_HEADER, Entry);
	while (&tmp->Entry != &_blRequestListHead) {
		old = tmp;
		tmp = CONTAINING_RECORD(tmp->Entry.Flink, REQUEST_HEADER, Entry);
		RequestMemoryFree(old);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
