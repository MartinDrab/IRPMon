
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "utils.h"
#include "request.h"
#include "req-queue.h"


/************************************************************************/
/*                            GLOBAL VARIABLES                          */
/************************************************************************/

static PKSEMAPHORE _requestListSemaphore = NULL;
static KSPIN_LOCK _requestListLock;
static volatile LONG _requestCount = 0;
static LIST_ENTRY _requestListHead;
static volatile LONG _connected = FALSE;
static IO_REMOVE_LOCK _removeLock;
static ERESOURCE _connectLock;
static volatile LONG _lastRequestId = 0;

/************************************************************************/
/*                             HELPER FUNCTIONS                         */
/************************************************************************/


static VOID _RequestQueueClear(VOID)
{
	PREQUEST_HEADER req = NULL;
	PREQUEST_HEADER old = NULL;

	req = CONTAINING_RECORD(_requestListHead.Flink, REQUEST_HEADER, Entry);
	while (&req->Entry != &_requestListHead) {
		old = req;
		req = CONTAINING_RECORD(req->Entry.Flink, REQUEST_HEADER, Entry);
		HeapMemoryFree(old);
	}

	InterlockedExchange(&_requestCount, 0);

	return;
}


/************************************************************************/
/*                            PUBLIC ROUTINES                           */
/************************************************************************/


NTSTATUS RequestQueueConnect(HANDLE hSemaphore)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("hSemaphore=0x%p", hSemaphore);
	DEBUG_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&_connectLock, TRUE);
	if (!_connected) {
		IoInitializeRemoveLock(&_removeLock, 0, 0, 0x7fffffff);
		status = IoAcquireRemoveLock(&_removeLock, NULL);
		if (NT_SUCCESS(status)) {
			if (hSemaphore != NULL)
				status = ObReferenceObjectByHandle(hSemaphore, SEMAPHORE_ALL_ACCESS, *ExSemaphoreObjectType, ExGetPreviousMode(), &_requestListSemaphore, NULL);
			
			if (NT_SUCCESS(status)) {
				_connected = TRUE;
				if (_requestListSemaphore != NULL)
					KeReleaseSemaphore(_requestListSemaphore, IO_NO_INCREMENT, _requestCount, FALSE);
			}

			if (!NT_SUCCESS(status))
				_requestListSemaphore = NULL;
		}
	} else status = STATUS_ALREADY_REGISTERED;

	ExReleaseResourceLite(&_connectLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x", status)
	return status;
}


VOID RequestQueueDisconnect(VOID)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&_connectLock, TRUE);
	if (_connected) {		
		IoReleaseRemoveLockAndWait(&_removeLock, NULL);
		if (_requestListSemaphore != NULL) {
			ObDereferenceObject(_requestListSemaphore);
			_requestListSemaphore = NULL;
		}

		_connected = FALSE;
	}

	ExReleaseResourceLite(&_connectLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID RequestQueueInsert(PREQUEST_HEADER Header)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Header=0x%p", Header);
	DEBUG_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

	if (_connected) {
		status = IoAcquireRemoveLock(&_removeLock, NULL);
		if (NT_SUCCESS(status)) {
			ExInterlockedInsertTailList(&_requestListHead, &Header->Entry, &_requestListLock);
			InterlockedIncrement(&_requestCount);
			if (_requestListSemaphore != NULL)
				KeReleaseSemaphore(_requestListSemaphore, IO_NO_INCREMENT, 1, FALSE);
			
			IoReleaseRemoveLock(&_removeLock, NULL);
		}
	} else status = STATUS_CONNECTION_DISCONNECTED;
	
	if (!NT_SUCCESS(status))
		HeapMemoryFree(Header);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


ULONG RequestIdReserve(void)
{
	return InterlockedIncrement(&_lastRequestId);
}


VOID RequestHeaderInit(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType)
{
	RequestHeaderInitNoId(Header, DriverObject, DeviceObject, RequestType);
	Header->Id = RequestIdReserve();

	return;
}


VOID RequestHeaderInitNoId(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType)
{
	RtlSecureZeroMemory(Header, sizeof(REQUEST_HEADER));
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
				drr = (PREQUEST_DRIVER_DETECTED)HeapMemoryAllocNonPaged(requestSize);
				if (drr != NULL) {
					drr->DriverNameLength = uObjectName.Length;
					memcpy(drr + 1, uObjectName.Buffer, drr->DriverNameLength);
					tmpHeader = &drr->Header;
				} else status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			case ertDeviceDetected:
				requestSize = sizeof(REQUEST_DEVICE_DETECTED) + uObjectName.Length;
				der = (PREQUEST_DEVICE_DETECTED)HeapMemoryAllocNonPaged(requestSize);
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
	PREQUEST_HEADER h = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Buffer=0x%p; Length=0x%p", Buffer, Length);
	DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);

	if (_connected) {
		status = IoAcquireRemoveLock(&_removeLock, NULL);
		if (NT_SUCCESS(status)) {
			PLIST_ENTRY l = NULL;
			
			l = ExInterlockedRemoveHeadList(&_requestListHead, &_requestListLock);
			if (l != NULL) {
				h = CONTAINING_RECORD(l, REQUEST_HEADER, Entry);
				reqSize = RequestGetSize(h);
				if (reqSize <= *Length) {
					InterlockedDecrement(&_requestCount);
					*Buffer = h;
					status = STATUS_SUCCESS;
				} else {
					ExInterlockedInsertHeadList(&_requestListHead, l, &_requestListLock);
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

/************************************************************************/
/*                     INITIALIZATION AND FINALIZATION                  */
/************************************************************************/

NTSTATUS RequestQueueModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(Context);
	
	InitializeListHead(&_requestListHead);
	KeInitializeSpinLock(&_requestListLock);
	IoInitializeRemoveLock(&_removeLock, 0, 0, 0x7fffffff);
	status = ExInitializeResourceLite(&_connectLock);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

VOID RequestQueueModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(Context);

	_RequestQueueClear();
	ExDeleteResourceLite(&_connectLock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
