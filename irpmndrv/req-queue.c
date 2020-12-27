
#include <ntifs.h>
#include <fltKernel.h>
#include "preprocessor.h"
#include "allocator.h"
#include "utils.h"
#include "request.h"
#include "process-events.h"
#include "driver-settings.h"
#include "boot-log.h"
#include "req-queue.h"

#undef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE_ENABLED 0


/************************************************************************/
/*                            GLOBAL VARIABLES                          */
/************************************************************************/

static KSPIN_LOCK _npagedRequestListLock;
static LIST_ENTRY _npagedRequestListHead;
static EX_PUSH_LOCK _pagedRequestListLock;
static LIST_ENTRY _pagedRequestListHead;
static IO_REMOVE_LOCK _removeLock;
static EX_PUSH_LOCK _connectLock;
static PIRPMNDRV_SETTINGS _driverSettings = NULL;

/************************************************************************/
/*                             HELPER FUNCTIONS                         */
/************************************************************************/


static void _RequestInsert(PREQUEST_HEADER Header, BOOLEAN Head)
{
	KIRQL irql = HIGH_LEVEL;
	PLIST_ENTRY listHead = NULL;
	DEBUG_ENTER_FUNCTION("Header=0x%p; Head=%u", Header, Head);

	if (Header->Flags & REQUEST_FLAG_PAGED) {
		ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
		listHead = &_pagedRequestListHead;
		FltAcquirePushLockExclusive(&_pagedRequestListLock);
	} else if (Header->Flags & REQUEST_FLAG_NONPAGED) {
		listHead = &_npagedRequestListHead;
		KeAcquireSpinLock(&_npagedRequestListLock, &irql);
	} else __debugbreak();

	if (Head)
		InsertHeadList(listHead, &Header->Entry);
	else InsertTailList(listHead, &Header->Entry);

	if (Header->Flags & REQUEST_FLAG_PAGED) {
		FltReleasePushLock(&_pagedRequestListLock);
		InterlockedIncrement(&_driverSettings->ReqQueuePagedLength);
	} else if (Header->Flags & REQUEST_FLAG_NONPAGED) {
		KeReleaseSpinLock(&_npagedRequestListLock, irql);
		InterlockedIncrement(&_driverSettings->ReqQueueNonPagedLength);
	} else __debugbreak();

	InterlockedIncrement(&_driverSettings->ReqQueueLength);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static PREQUEST_HEADER _RequestRemove(PBOOLEAN NextAvailable)
{
	KIRQL irql;
	ULONG pagedId = 0;
	ULONG nonpagedId = 0;
	PREQUEST_HEADER pagedRequest = NULL;
	PREQUEST_HEADER npagedRequest = NULL;
	PREQUEST_HEADER ret = NULL;
	DEBUG_ENTER_FUNCTION("NextAvailable=0x%p", NextAvailable);

	*NextAvailable = FALSE;
	FltAcquirePushLockExclusive(&_pagedRequestListLock);
	if (!IsListEmpty(&_pagedRequestListHead)) {
		pagedRequest = CONTAINING_RECORD(_pagedRequestListHead.Flink, REQUEST_HEADER, Entry);
		pagedId = pagedRequest->Id;
	}

	KeAcquireSpinLock(&_npagedRequestListLock, &irql);
	if (!IsListEmpty(&_npagedRequestListHead)) {
		npagedRequest = CONTAINING_RECORD(_npagedRequestListHead.Flink, REQUEST_HEADER, Entry);
		nonpagedId = npagedRequest->Id;
		if (pagedRequest == NULL || nonpagedId < pagedId) {
			ret = npagedRequest;
			RemoveEntryList(&ret->Entry);
			InterlockedDecrement(&_driverSettings->ReqQueueNonPagedLength);
		}
	}

	KeReleaseSpinLock(&_npagedRequestListLock, irql);
	if (ret == NULL && pagedRequest != NULL) {
		ret = pagedRequest;
		RemoveEntryList(&ret->Entry);
		InterlockedDecrement(&_driverSettings->ReqQueuePagedLength);
	}

	FltReleasePushLock(&_pagedRequestListLock);
	if (ret != NULL) {
		InterlockedDecrement(&_driverSettings->ReqQueueLength);
		*NextAvailable = (!IsListEmpty(&_pagedRequestListHead) || !IsListEmpty(&_npagedRequestListHead));
	}

	DEBUG_EXIT_FUNCTION("0x%p, *NextAvailable=%u", ret, *NextAvailable);
	return ret;
}


/************************************************************************/
/*                            PUBLIC ROUTINES                           */
/************************************************************************/


NTSTATUS RequestQueueConnect()
{
	PREQUEST_HEADER old = NULL;
	PREQUEST_HEADER psRequest = NULL;
	LIST_ENTRY psRequests;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();
	DEBUG_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

	FltAcquirePushLockExclusive(&_connectLock);
	if (!_driverSettings->ReqQueueConnected) {
		IoInitializeRemoveLock(&_removeLock, 0, 0, 0x7fffffff);
		status = IoAcquireRemoveLock(&_removeLock, NULL);
		if (NT_SUCCESS(status)) {
			InitializeListHead(&psRequests);
			if (_driverSettings->ProcessEmulateOnConnect)
				status = ListProcessesByEvents(&psRequests);
			
			if (NT_SUCCESS(status) && _driverSettings->DriverSnapshotOnConnect)
				status = ListDriversAndDevicesByEvents(&psRequests);

			if (NT_SUCCESS(status)) {
				psRequest = CONTAINING_RECORD(psRequests.Flink, REQUEST_HEADER, Entry);
				while (&psRequest->Entry != &psRequests) {
					old = psRequest;
					psRequest = CONTAINING_RECORD(psRequest->Entry.Flink, REQUEST_HEADER, Entry);
					RemoveEntryList(&old->Entry);
					old->Id = InterlockedIncrement(&_driverSettings->ReqQueueLastRequestId);
					_RequestInsert(old, FALSE);
				}

				_driverSettings->ReqQueueConnected = TRUE;
				BLDisable();
			}

			if (!NT_SUCCESS(status)) {
				psRequest = CONTAINING_RECORD(psRequests.Flink, REQUEST_HEADER, Entry);
				while (&psRequest->Entry != &psRequests) {
					old = psRequest;
					psRequest = CONTAINING_RECORD(psRequest->Entry.Flink, REQUEST_HEADER, Entry);
					RemoveEntryList(&old->Entry);
					RequestMemoryFree(old);
				}

				IoReleaseRemoveLock(&_removeLock, NULL);
			}
		}
	} else status = STATUS_ALREADY_REGISTERED;

	FltReleasePushLock(&_connectLock);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void RequestQueueDisconnect(VOID)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	FltAcquirePushLockExclusive(&_connectLock);
	if (_driverSettings->ReqQueueConnected) {
		IoReleaseRemoveLockAndWait(&_removeLock, NULL);
		_driverSettings->ReqQueueConnected = FALSE;
		if (_driverSettings->ReqQueueClearOnDisconnect)
			RequestQueueClear();
	}

	FltReleasePushLock(&_connectLock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


void RequestQueueInsert(PREQUEST_HEADER Header)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Header=0x%p", Header);
	DEBUG_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	if (_driverSettings->ReqQueueConnected ||
		(_driverSettings->ReqQueueCollectWhenDisconnected && !BLEnabled())) {
		status = IoAcquireRemoveLock(&_removeLock, NULL);
		if (NT_SUCCESS(status)) {
			Header->Id = InterlockedIncrement(&_driverSettings->ReqQueueLastRequestId);
			_RequestInsert(Header, FALSE);
			IoReleaseRemoveLock(&_removeLock, NULL);
		}
	} else status = STATUS_CONNECTION_DISCONNECTED;
	
	if (status == STATUS_CONNECTION_DISCONNECTED && BLEnabled()) {
		Header->Id = InterlockedIncrement(&_driverSettings->ReqQueueLastRequestId);
		BLLogRequest(Header);
		status = STATUS_SUCCESS;
	}

	if (!NT_SUCCESS(status))
		RequestMemoryFree(Header);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


void RequestHeaderInit(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType)
{
	RequestHeaderInitNoId(Header, DriverObject, DeviceObject, RequestType);

	return;
}


void RequestHeaderInitNoId(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType)
{
	InitializeListHead(&Header->Entry);
	KeQuerySystemTime(&Header->Time);
	Header->Device = DeviceObject;
	Header->Driver = DriverObject;
	Header->Type = RequestType;
	Header->ResultType = rrtUndefined;
	Header->Result.Other = NULL;
	Header->ProcessId = PsGetCurrentProcessId();
	Header->ThreadId = PsGetCurrentThreadId();
	Header->Irql = KeGetCurrentIrql();
	ASSERT(Header->Flags == REQUEST_FLAG_PAGED || Header->Flags == REQUEST_FLAG_NONPAGED);

	return;
}


NTSTATUS RequestXXXDetectedCreate(ERequesttype Type, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PREQUEST_HEADER *Header)
{
	SIZE_T requestSize = 0;
	PREQUEST_HEADER tmpHeader = NULL;
	PREQUEST_DRIVER_DETECTED drr = NULL;
	PREQUEST_DEVICE_DETECTED der = NULL;
	UNICODE_STRING uObjectName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Type=%u; DriverObject=0x%p; DeviceObject=0x%p; Header=0x%p", DriverObject, DeviceObject, Type, Header);

	status = (Type == ertDeviceDetected) ?
		_GetObjectName(DeviceObject, &uObjectName) :
		_GetObjectName(DriverObject, &uObjectName);

	if (NT_SUCCESS(status)) {
		switch (Type) {
			case ertDriverDetected:
				requestSize = sizeof(REQUEST_DRIVER_DETECTED) + uObjectName.Length;
				drr = (PREQUEST_DRIVER_DETECTED)RequestMemoryAlloc(requestSize);
				if (drr != NULL) {
					drr->DriverNameLength = uObjectName.Length;
					memcpy(drr + 1, uObjectName.Buffer, drr->DriverNameLength);
					tmpHeader = &drr->Header;
				} else status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			case ertDeviceDetected:
				requestSize = sizeof(REQUEST_DEVICE_DETECTED) + uObjectName.Length;
				der = (PREQUEST_DEVICE_DETECTED)RequestMemoryAlloc(requestSize);
				if (der != NULL) {
					der->DeviceNameLength = uObjectName.Length;
					memcpy(der + 1, uObjectName.Buffer, der->DeviceNameLength);
					tmpHeader = &der->Header;
				} else status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			default:
				status = STATUS_INVALID_PARAMETER_1;
				break;
		}

		if (NT_SUCCESS(status)) {
			RequestHeaderInit(tmpHeader, DriverObject, DeviceObject, Type);
			*Header = tmpHeader;
		}

		HeapMemoryFree(uObjectName.Buffer);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Header=0x%p", status, *Header);
	return status;
}


NTSTATUS RequestQueueGet(PREQUEST_HEADER *Buffer, PSIZE_T Length)
{
	SIZE_T reqSize = 0;
	BOOLEAN nextAvailable = FALSE;
	PREQUEST_HEADER h = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Buffer=0x%p; Length=0x%p", Buffer, Length);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	if (_driverSettings->ReqQueueConnected) {
		status = IoAcquireRemoveLock(&_removeLock, NULL);
		if (NT_SUCCESS(status)) {
			h = _RequestRemove(&nextAvailable);
			if (h != NULL) {
				reqSize = RequestGetSize(h);
				if (reqSize <= *Length) {
					if (nextAvailable)
						h->Flags |= REQUEST_FLAG_NEXT_AVAILABLE;

					h->Entry.Flink = NULL;
					h->Entry.Blink = NULL;
					*Buffer = h;
					status = STATUS_SUCCESS;
				} else {
					_RequestInsert(h, TRUE);
					status = STATUS_BUFFER_TOO_SMALL;
				}
			} else status = STATUS_NO_MORE_ENTRIES;

			*Length = reqSize;
			IoReleaseRemoveLock(&_removeLock, NULL);
		}
	} else status = STATUS_CONNECTION_DISCONNECTED;

	DEBUG_EXIT_FUNCTION("0x%x, *Length=%u", status, *Length);
	return status;
}


NTSTATUS ListDriversAndDevicesByEvents(PLIST_ENTRY ListHead)
{
	const wchar_t *dirNames[2] = {
		L"\\Driver",
		L"\\FileSystem"
	};
	PDRIVER_OBJECT *driverArray = NULL;
	SIZE_T driverArrayLength;
	PDEVICE_OBJECT *deviceArray = NULL;
	ULONG deviceArrayLength = 0;
	UNICODE_STRING tmpName;
	PREQUEST_HEADER tmpRequest = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ListHead=0x%p", ListHead);

	for (size_t i = 0; i < sizeof(dirNames) / sizeof(dirNames[0]); ++i) {
		RtlInitUnicodeString(&tmpName, dirNames[i]);
		status = _GetDriversInDirectory(&tmpName, &driverArray, &driverArrayLength);
		if (NT_SUCCESS(status)) {
			for (size_t j = 0; j < driverArrayLength; ++j) {
				status = RequestXXXDetectedCreate(ertDriverDetected, driverArray[j], NULL, &tmpRequest);
				if (NT_SUCCESS(status)) {
					tmpRequest->Flags |= REQUEST_FLAG_EMULATED;
					InsertTailList(ListHead, &tmpRequest->Entry);
					status = UtilsEnumDriverDevices(driverArray[j], &deviceArray, &deviceArrayLength);
					if (NT_SUCCESS(status)) {
						for (size_t k = 0; k < deviceArrayLength; ++k) {
							status = RequestXXXDetectedCreate(ertDeviceDetected, driverArray[j], deviceArray[k], &tmpRequest);
							if (NT_SUCCESS(status)) {
								tmpRequest->Flags |= REQUEST_FLAG_EMULATED;
								InsertTailList(ListHead, &tmpRequest->Entry);
							}
						}

						_ReleaseDeviceArray(deviceArray, deviceArrayLength);
					}
				}
			}

			_ReleaseDriverArray(driverArray, driverArrayLength);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void RequestQueueClear(void)
{
	KIRQL irql;
	LIST_ENTRY tmpList;
	PREQUEST_HEADER req = NULL;
	PREQUEST_HEADER old = NULL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	InitializeListHead(&tmpList);
	KeAcquireSpinLock(&_npagedRequestListLock, &irql);
	if (!IsListEmpty(&_npagedRequestListHead)) {
		tmpList = _npagedRequestListHead;
		InitializeListHead(&_npagedRequestListHead);
	}

	KeReleaseSpinLock(&_npagedRequestListLock, irql);
	tmpList.Flink->Blink = &tmpList;
	tmpList.Blink->Flink = &tmpList;
	req = CONTAINING_RECORD(tmpList.Flink, REQUEST_HEADER, Entry);
	while (&req->Entry != &tmpList) {
		old = req;
		req = CONTAINING_RECORD(req->Entry.Flink, REQUEST_HEADER, Entry);
		RequestMemoryFree(old);
		InterlockedDecrement(&_driverSettings->ReqQueueNonPagedLength);
		InterlockedDecrement(&_driverSettings->ReqQueueLength);
	}

	InitializeListHead(&tmpList);
	FltAcquirePushLockExclusive(&_pagedRequestListLock);
	if (!IsListEmpty(&_pagedRequestListHead)) {
		tmpList = _pagedRequestListHead;
		InitializeListHead(&_pagedRequestListHead);
	}

	FltReleasePushLock(&_pagedRequestListLock);
	tmpList.Flink->Blink = &tmpList;
	tmpList.Blink->Flink = &tmpList;
	req = CONTAINING_RECORD(tmpList.Flink, REQUEST_HEADER, Entry);
	while (&req->Entry != &tmpList) {
		old = req;
		req = CONTAINING_RECORD(req->Entry.Flink, REQUEST_HEADER, Entry);
		RequestMemoryFree(old);
		InterlockedDecrement(&_driverSettings->ReqQueuePagedLength);
		InterlockedDecrement(&_driverSettings->ReqQueueLength);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                     INITIALIZATION AND FINALIZATION                  */
/************************************************************************/

NTSTATUS RequestQueueModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);
	
	_driverSettings = DriverSettingsGet();
	FltInitializePushLock(&_pagedRequestListLock);
	InitializeListHead(&_npagedRequestListHead);
	InitializeListHead(&_pagedRequestListHead);
	KeInitializeSpinLock(&_npagedRequestListLock);
	IoInitializeRemoveLock(&_removeLock, 0, 0, 0x7fffffff);
	FltInitializePushLock(&_connectLock);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

VOID RequestQueueModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	RequestQueueClear();
	_driverSettings = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
