
#ifdef _KERNEL_MODE
#include <ntifs.h>
#else
#include <windows.h>
#include <strsafe.h>
#endif
#include "general-types.h"
#include "request.h"


#ifndef _KERNEL_MODE

static void _RequestHeaderInit(PREQUEST_HEADER Header, void *DriverObject, void *DeviceObject, ERequesttype RequestType)
{
	RtlSecureZeroMemory(Header, sizeof(REQUEST_HEADER));
	Header->Flags |= REQUEST_FLAG_EMULATED;
	Header->Device = DeviceObject;
	Header->Driver = DriverObject;
	Header->Type = RequestType;
	Header->ResultType = rrtUndefined;
	Header->Result.Other = NULL;
	Header->ProcessId = (HANDLE)GetCurrentProcessId();
	Header->ThreadId = (HANDLE)GetCurrentThreadId();
	Header->Irql = 0;

	return;
}

#endif


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
	}

	return ret;
}

#ifndef _KERNEL_MODE

DWORD RequestEmulateDriverDetected(void *DriverObject, const wchar_t *DriverName, PREQUEST_DRIVER_DETECTED *Request)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PREQUEST_DRIVER_DETECTED tmpRequest = NULL;
	size_t driverNameLen = 0;

	ret = S_OK;
	if (DriverName != NULL)
		ret = StringCbLengthW(DriverName, 65536, &driverNameLen);

	if (ret == S_OK) {
		tmpRequest = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(REQUEST_DRIVER_DETECTED) + driverNameLen);
		if (tmpRequest != NULL) {
			_RequestHeaderInit(&tmpRequest->Header, DriverObject, NULL, ertDriverDetected);
			tmpRequest->DriverNameLength = driverNameLen;
			memcpy(tmpRequest + 1, DriverName, driverNameLen);;
			*Request = tmpRequest;
			ret = ERROR_SUCCESS;
		} else ret = GetLastError();
	}

	return ret;
}


DWORD RequestEmulateDeviceDetected(void *DriverObject, void *DeviceObject, const wchar_t *DeviceName, PREQUEST_DEVICE_DETECTED *Request)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PREQUEST_DEVICE_DETECTED tmpRequest = NULL;
	size_t deviceNameLen = 0;

	ret = S_OK;
	if (DeviceName != NULL)
		ret = StringCbLengthW(DeviceName, 65536, &deviceNameLen);

	if (ret == S_OK) {
		tmpRequest = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(REQUEST_DEVICE_DETECTED) + deviceNameLen);
		if (tmpRequest != NULL) {
			_RequestHeaderInit(&tmpRequest->Header, DriverObject, DeviceObject, ertDeviceDetected);
			tmpRequest->DeviceNameLength = deviceNameLen;
			memcpy(tmpRequest + 1, DeviceName, deviceNameLen);;
			*Request = tmpRequest;
			ret = ERROR_SUCCESS;
		} else ret = GetLastError();
	}

	return ret;
}


DWORD RequestEmulateFileNameAssigned(void *FileObject, const wchar_t *FileName, PREQUEST_FILE_OBJECT_NAME_ASSIGNED *Request)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PREQUEST_FILE_OBJECT_NAME_ASSIGNED tmpRequest = NULL;
	size_t fileNameLen = 0;

	ret = S_OK;
	if (FileName != NULL)
		ret = StringCbLengthW(FileName, 65536, &fileNameLen);

	if (ret == S_OK) {
		tmpRequest = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(REQUEST_FILE_OBJECT_NAME_ASSIGNED) + fileNameLen);
		if (tmpRequest != NULL) {
			_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertFileObjectNameAssigned);
			tmpRequest->FileObject = FileObject;
			tmpRequest->NameLength = fileNameLen;
			memcpy(tmpRequest + 1, FileName, fileNameLen);;
			*Request = tmpRequest;
			ret = ERROR_SUCCESS;
		}
		else ret = GetLastError();
	}

	return ret;
}


DWORD RequestEmulateFileNameDeleted(void *FileObject, PREQUEST_FILE_OBJECT_NAME_DELETED *Request)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PREQUEST_FILE_OBJECT_NAME_DELETED tmpRequest = NULL;

	tmpRequest = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(REQUEST_FILE_OBJECT_NAME_DELETED));
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertFileObjectNameDeleted);
		tmpRequest->FileObject = FileObject;
		*Request = tmpRequest;
		ret = ERROR_SUCCESS;
	} else ret = GetLastError();

	return ret;
}


DWORD RequestEmulateProcessCreated(HANDLE ProcessId, HANDLE ParentId, const wchar_t *ImageName, const wchar_t *CommandLine, PREQUEST_PROCESS_CREATED *Request)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PREQUEST_PROCESS_CREATED tmpRequest = NULL;
	size_t imageNameLen = 0;
	size_t commandLineLen = 0;

	ret = S_OK;
	if (ImageName != NULL)
		ret = StringCbLengthW(ImageName, 65536, &imageNameLen);

	if (ret == S_OK && CommandLine != NULL)
		ret = StringCbLengthW(CommandLine, 65536, &commandLineLen);

	if (ret == S_OK) {
		tmpRequest = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(REQUEST_PROCESS_CREATED) + imageNameLen + commandLineLen);
		if (tmpRequest != NULL) {
			_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertProcessCreated);
			tmpRequest->ProcessId = ProcessId;
			tmpRequest->ParentId = ParentId;
			tmpRequest->ImageNameLength = imageNameLen;
			memcpy(tmpRequest + 1, ImageName, tmpRequest->ImageNameLength);
			tmpRequest->CommandLineLength = commandLineLen;
			memcpy((unsigned char *)(tmpRequest + 1) + tmpRequest->ImageNameLength, CommandLine, tmpRequest->CommandLineLength);
			*Request = tmpRequest;
			ret = ERROR_SUCCESS;
		} else ret = GetLastError();
	}

	return ret;
}


DWORD RequestEmulateProcessExitted(HANDLE ProcessId, PREQUEST_PROCESS_EXITTED *Request)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PREQUEST_PROCESS_EXITTED tmpRequest = NULL;

	tmpRequest = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(REQUEST_PROCESS_EXITTED));
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertProcessExitted);
		tmpRequest->Header.ProcessId = ProcessId;
		tmpRequest->ProcessId = ProcessId;
		*Request = tmpRequest;
		ret = ERROR_SUCCESS;
	}
	else ret = GetLastError();

	return ret;
}


void RequestEmulatedFree(PREQUEST_HEADER Header)
{
	HeapFree(GetProcessHeap(), 0, Header);

	return;
}


#endif
