
#ifdef _KERNEL_MODE
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#else
#include <windows.h>
#include <strsafe.h>
#endif
#include "general-types.h"
#include "request.h"



/************************************************************************/
/*              HELPER FUNCTIONS                                        */
/************************************************************************/

static void _RequestHeaderInit(PREQUEST_HEADER Header, void *DriverObject, void *DeviceObject, ERequesttype RequestType)
{
	RtlSecureZeroMemory(Header, sizeof(REQUEST_HEADER));
	Header->Flags |= REQUEST_FLAG_EMULATED;
	Header->Device = DeviceObject;
	Header->Driver = DriverObject;
	Header->Type = RequestType;
	Header->ResultType = rrtUndefined;
	Header->Result.Other = NULL;
#ifdef _KERNEL_MODE
	Header->ProcessId = PsGetCurrentProcessId();
	Header->ThreadId = PsGetCurrentThreadId();
	Header->Irql = KeGetCurrentIrql();
#else
	Header->ProcessId = (HANDLE)(ULONG_PTR)GetCurrentProcessId();
	Header->ThreadId = (HANDLE)(ULONG_PTR)GetCurrentThreadId();
	Header->Irql = 0;
#endif

	return;
}


/************************************************************************/
/*                  PUBLIC FUNCTIONS                                    */
/************************************************************************/


#ifdef _KERNEL_MODE

void _SetRequestFlags(PREQUEST_HEADER Request, const BASIC_CLIENT_INFO *Info)
{
	DEBUG_ENTER_FUNCTION("Request=0x%p; Info=0x%p", Request, Info);

	if (Info->Admin)
		Request->Flags |= REQUEST_FLAG_ADMIN;

	if (Info->Impersonated)
		Request->Flags |= REQUEST_FLAG_IMPERSONATED;

	if (Info->ImpersonatedAdmin)
		Request->Flags |= REQUEST_FLAG_IMPERSONATED_ADMIN;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


#endif


/// <summary>Gets size of a request, in bytes.
/// </summary>
/// <param name="Request">
/// Pointer to the request retrieved via <see cref="IRPMonDllGetRequest"/>.
/// </param>
/// <returns>
/// If successful, returns the request size, in bytes.
/// On error, zero is returned.
/// </returns>
size_t RequestGetSize(const REQUEST_HEADER *Header)
{
	size_t ret = 0;
	const REQUEST_IRP *irp = NULL;
	const REQUEST_IRP_COMPLETION *irpComp = NULL;
	const REQUEST_STARTIO *startIo = NULL;
	const REQUEST_DRIVER_DETECTED *drr = CONTAINING_RECORD(Header, REQUEST_DRIVER_DETECTED, Header);
	const REQUEST_DEVICE_DETECTED *der = CONTAINING_RECORD(Header, REQUEST_DEVICE_DETECTED, Header);
	const REQUEST_FILE_OBJECT_NAME_ASSIGNED *ar = NULL;
	const REQUEST_FILE_OBJECT_NAME_DELETED *dr = NULL;
	const REQUEST_PROCESS_CREATED *pcr = CONTAINING_RECORD(Header, REQUEST_PROCESS_CREATED, Header);
	const REQUEST_IMAGE_LOAD *ilr = NULL;

	switch (Header->Type) {
		case ertIRP:
			ret = sizeof(REQUEST_IRP);
			irp = CONTAINING_RECORD(Header, REQUEST_IRP, Header);
			ret += irp->DataSize;
			break;
		case ertIRPCompletion:
			ret = sizeof(REQUEST_IRP_COMPLETION);
			irpComp = CONTAINING_RECORD(Header, REQUEST_IRP_COMPLETION, Header);
			ret += irpComp->DataSize;
			break;
		case ertFastIo:
			ret = sizeof(REQUEST_FASTIO);
			break;
		case ertAddDevice:
			ret = sizeof(REQUEST_ADDDEVICE);
			break;
		case ertDriverUnload:
			ret = sizeof(REQUEST_UNLOAD);
			break;
		case ertStartIo:
			ret = sizeof(REQUEST_STARTIO);
			startIo = CONTAINING_RECORD(Header, REQUEST_STARTIO, Header);
			ret += startIo->DataSize;
			break;
		case ertDriverDetected:
			ret = sizeof(REQUEST_DRIVER_DETECTED) + drr->DriverNameLength;
			break;
		case ertDeviceDetected:
			ret = sizeof(REQUEST_DEVICE_DETECTED) + der->DeviceNameLength;
			break;
		case ertFileObjectNameAssigned:
			ar = CONTAINING_RECORD(Header, REQUEST_FILE_OBJECT_NAME_ASSIGNED, Header);
			ret = sizeof(REQUEST_FILE_OBJECT_NAME_ASSIGNED) + ar->NameLength;
			break;
		case ertFileObjectNameDeleted:
			dr = CONTAINING_RECORD(Header, REQUEST_FILE_OBJECT_NAME_DELETED, Header);
			ret = sizeof(REQUEST_FILE_OBJECT_NAME_DELETED);
			break;
		case ertProcessCreated:
			ret = sizeof(REQUEST_PROCESS_CREATED) + pcr->ImageNameLength + pcr->CommandLineLength;
			break;
		case ertProcessExitted:
			ret = sizeof(REQUEST_PROCESS_EXITTED);
			break;
		case ertImageLoad:
			ilr = CONTAINING_RECORD(Header, REQUEST_IMAGE_LOAD, Header);
			ret = sizeof(REQUEST_IMAGE_LOAD) + ilr->DataSize;
			break;
	}

	return ret;
}


PREQUEST_HEADER RequestCopy(const REQUEST_HEADER *Header)
{
	size_t reqSize = 0;
	PREQUEST_HEADER ret = NULL;

	reqSize = RequestGetSize(Header);
	ret = RequestMemoryAlloc(reqSize);
	if (ret != NULL)
		memcpy(ret, Header, reqSize);

	return ret;
}


ERROR_TYPE RequestEmulateDriverDetected(void *DriverObject, const wchar_t *DriverName, PREQUEST_DRIVER_DETECTED *Request)
{
	ERROR_TYPE ret = ERROR_VALUE_INVAL;
	PREQUEST_DRIVER_DETECTED tmpRequest = NULL;
	size_t driverNameLen = 0;

	if (DriverName != NULL)
		driverNameLen = wcslen(DriverName) * sizeof(wchar_t);

	tmpRequest = (PREQUEST_DRIVER_DETECTED)RequestMemoryAlloc(sizeof(REQUEST_DRIVER_DETECTED) + driverNameLen);
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, DriverObject, NULL, ertDriverDetected);
		tmpRequest->DriverNameLength = (ULONG)driverNameLen;
		memcpy(tmpRequest + 1, DriverName, driverNameLen);;
		*Request = tmpRequest;
		ret = ERROR_VALUE_SUCCESS;
	} else ret = ERROR_VALUE_NOMEM;

	return ret;
}


ERROR_TYPE RequestEmulateDeviceDetected(void *DriverObject, void *DeviceObject, const wchar_t *DeviceName, PREQUEST_DEVICE_DETECTED *Request)
{
	ERROR_TYPE ret = ERROR_VALUE_INVAL;
	PREQUEST_DEVICE_DETECTED tmpRequest = NULL;
	size_t deviceNameLen = 0;

	if (DeviceName != NULL)
		deviceNameLen = wcslen(DeviceName)*sizeof(wchar_t);

	tmpRequest = (PREQUEST_DEVICE_DETECTED)RequestMemoryAlloc(sizeof(REQUEST_DEVICE_DETECTED) + deviceNameLen);
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, DriverObject, DeviceObject, ertDeviceDetected);
		tmpRequest->DeviceNameLength = (ULONG)deviceNameLen;
		memcpy(tmpRequest + 1, DeviceName, deviceNameLen);;
		*Request = tmpRequest;
		ret = ERROR_VALUE_SUCCESS;
	} else ret = ERROR_VALUE_NOMEM;

	return ret;
}


ERROR_TYPE RequestEmulateFileNameAssigned(void *FileObject, const wchar_t *FileName, PREQUEST_FILE_OBJECT_NAME_ASSIGNED *Request)
{
	ERROR_TYPE ret = ERROR_VALUE_INVAL;
	PREQUEST_FILE_OBJECT_NAME_ASSIGNED tmpRequest = NULL;
	size_t fileNameLen = 0;

	if (FileName != NULL)
		fileNameLen = wcslen(FileName) * sizeof(wchar_t);

	tmpRequest = (PREQUEST_FILE_OBJECT_NAME_ASSIGNED)RequestMemoryAlloc(sizeof(REQUEST_FILE_OBJECT_NAME_ASSIGNED) + fileNameLen);
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertFileObjectNameAssigned);
		tmpRequest->FileObject = FileObject;
		tmpRequest->NameLength = (ULONG)fileNameLen;
		memcpy(tmpRequest + 1, FileName, fileNameLen);;
		*Request = tmpRequest;
		ret = ERROR_VALUE_SUCCESS;
	} else ret = ERROR_VALUE_NOMEM;

	return ret;
}


ERROR_TYPE RequestEmulateFileNameDeleted(void *FileObject, PREQUEST_FILE_OBJECT_NAME_DELETED *Request)
{
	ERROR_TYPE ret = ERROR_VALUE_INVAL;
	PREQUEST_FILE_OBJECT_NAME_DELETED tmpRequest = NULL;

	tmpRequest = (PREQUEST_FILE_OBJECT_NAME_DELETED)RequestMemoryAlloc(sizeof(REQUEST_FILE_OBJECT_NAME_DELETED));
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertFileObjectNameDeleted);
		tmpRequest->FileObject = FileObject;
		*Request = tmpRequest;
		ret = ERROR_VALUE_SUCCESS;
	} else ret = ERROR_VALUE_NOMEM;

	return ret;
}


ERROR_TYPE RequestEmulateProcessCreated(HANDLE ProcessId, HANDLE ParentId, const wchar_t *ImageName, const wchar_t *CommandLine, PREQUEST_PROCESS_CREATED *Request)
{
	ERROR_TYPE ret = ERROR_VALUE_INVAL;
	PREQUEST_PROCESS_CREATED tmpRequest = NULL;
	size_t imageNameLen = 0;
	size_t commandLineLen = 0;

	if (ImageName != NULL)
		imageNameLen = wcslen(ImageName) * sizeof(wchar_t);

	if (CommandLine != NULL)
		commandLineLen = wcslen(CommandLine) * sizeof(wchar_t);

	tmpRequest = (PREQUEST_PROCESS_CREATED)RequestMemoryAlloc(sizeof(REQUEST_PROCESS_CREATED) + imageNameLen + commandLineLen);
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertProcessCreated);
		tmpRequest->ProcessId = ProcessId;
		tmpRequest->ParentId = ParentId;
		tmpRequest->ImageNameLength = (ULONG)imageNameLen;
		memcpy(tmpRequest + 1, ImageName, tmpRequest->ImageNameLength);
		tmpRequest->CommandLineLength = (ULONG)commandLineLen;
		memcpy((unsigned char *)(tmpRequest + 1) + tmpRequest->ImageNameLength, CommandLine, tmpRequest->CommandLineLength);
		*Request = tmpRequest;
		ret = ERROR_VALUE_SUCCESS;
	} else ret = ERROR_VALUE_NOMEM;

	return ret;
}


ERROR_TYPE RequestEmulateProcessExitted(HANDLE ProcessId, PREQUEST_PROCESS_EXITTED *Request)
{
	ERROR_TYPE ret = ERROR_VALUE_INVAL;
	PREQUEST_PROCESS_EXITTED tmpRequest = NULL;

	tmpRequest = (PREQUEST_PROCESS_EXITTED)RequestMemoryAlloc(sizeof(REQUEST_PROCESS_EXITTED));
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertProcessExitted);
		tmpRequest->Header.ProcessId = ProcessId;
		tmpRequest->ProcessId = ProcessId;
		*Request = tmpRequest;
		ret = ERROR_VALUE_SUCCESS;
	} else ret = ERROR_VALUE_NOMEM;

	return ret;
}


PREQUEST_HEADER RequestMemoryAlloc(size_t Size)
{
	PREQUEST_HEADER ret = NULL;
#ifdef _KERNEL_MODE
	POOL_TYPE pt;

	pt = (KeGetCurrentIrql() < DISPATCH_LEVEL) ? PagedPool : NonPagedPool;
	ret = (PREQUEST_HEADER)HeapMemoryAlloc(pt, Size);
	if (ret != NULL) {
		memset(ret, 0, Size);
		switch (pt) {
			case NonPagedPool:
				ret->Flags |= REQUEST_FLAG_NONPAGED;
				break;
			case PagedPool:
				ret->Flags |= REQUEST_FLAG_PAGED;
				break;
		}
	}

#else
	ret = (PREQUEST_HEADER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
#endif

	return ret;
}


void RequestMemoryFree(PREQUEST_HEADER Request)
{
#ifdef _KERNEL_MODE
	HeapMemoryFree(Request);
#else
	HeapFree(GetProcessHeap(), 0, Request);
#endif

	return;
}
