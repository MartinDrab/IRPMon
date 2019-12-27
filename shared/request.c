
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
	Header->ProcessId = (HANDLE)(ULONG_PTR)GetCurrentProcessId();
	Header->ThreadId = (HANDLE)(ULONG_PTR)GetCurrentThreadId();
	Header->Irql = 0;

	return;
}

#endif


static PREQUEST_HEADER _RequestMemoryAlloc(size_t Size)
{
#ifdef _KERNEL_MODE
	return HeapMemoryAllocNonPaged(Size);
#else
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
#endif
}


static void _RequestMemoryFree(PREQUEST_HEADER Request)
{
#ifdef _KERNEL_MODE
	HeapMemoryFree(Request);
#else
	HeapFree(GetProcessHeap(), 0, Request);
#endif

	return;
}


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


BOOLEAN RequestCompress(PREQUEST_HEADER Header)
{
	BOOLEAN ret = FALSE;
	wchar_t *source = NULL;
	char *target = NULL;
	size_t charCount = 0;
	PREQUEST_DRIVER_DETECTED drr = CONTAINING_RECORD(Header, REQUEST_DRIVER_DETECTED, Header);
	PREQUEST_DEVICE_DETECTED der = CONTAINING_RECORD(Header, REQUEST_DEVICE_DETECTED, Header);
	PREQUEST_FILE_OBJECT_NAME_ASSIGNED ar = NULL;
	PREQUEST_PROCESS_CREATED pcr = CONTAINING_RECORD(Header, REQUEST_PROCESS_CREATED, Header);

	if ((Header->Flags & REQUEST_FLAG_COMPRESSED) == 0) {
		switch (Header->Type) {
			case ertDriverDetected:
				source = (wchar_t *)(drr + 1);
				charCount = drr->DriverNameLength / sizeof(wchar_t);
				ret = TRUE;
				break;
			case ertDeviceDetected:
				source = (wchar_t *)(der + 1);
				charCount = der->DeviceNameLength / sizeof(wchar_t);
				ret = TRUE;
				break;
			case ertFileObjectNameAssigned:
				ar = CONTAINING_RECORD(Header, REQUEST_FILE_OBJECT_NAME_ASSIGNED, Header);
				source = (wchar_t *)(ar + 1);
				charCount = ar->NameLength / sizeof(wchar_t);
				ret = TRUE;
				break;
			case ertProcessCreated:
				source = (wchar_t *)(pcr + 1);
				charCount = (pcr->ImageNameLength + pcr->CommandLineLength) / sizeof(wchar_t);
				ret = TRUE;
				break;
		}

		if (ret) {
			for (size_t i = 0; i < charCount; ++i) {
				if (source[i] >= 0x100) {
					ret = FALSE;
					break;
				}
			}

			if (ret) {
				switch (Header->Type) {
					case ertDriverDetected:
						drr->DriverNameLength /= sizeof(wchar_t);
						break;
					case ertDeviceDetected:
						der->DeviceNameLength /= sizeof(wchar_t);
						break;
					case ertFileObjectNameAssigned:
						ar->NameLength /= sizeof(wchar_t);
						break;
					case ertProcessCreated:
						pcr->ImageNameLength /= sizeof(wchar_t);
						pcr->CommandLineLength /= sizeof(wchar_t);
						break;
				}

				target = (char *)source;
				for (size_t i = 0; i < charCount; ++i)
					target[i] = (char)source[i];

				Header->Flags |= REQUEST_FLAG_COMPRESSED;
			}
		}
	}

	return ret;
}


PREQUEST_HEADER RequestDecompress(const REQUEST_HEADER *Header)
{
	const char *source = NULL;
	wchar_t *target = NULL;
	size_t charCount = 0;
	size_t newSize = 0;
	size_t oldSize = 0;
	PREQUEST_HEADER newRequest = NULL;
	PREQUEST_DRIVER_DETECTED drr = CONTAINING_RECORD(Header, REQUEST_DRIVER_DETECTED, Header);
	PREQUEST_DEVICE_DETECTED der = CONTAINING_RECORD(Header, REQUEST_DEVICE_DETECTED, Header);
	PREQUEST_FILE_OBJECT_NAME_ASSIGNED ar = NULL;
	PREQUEST_PROCESS_CREATED pcr = CONTAINING_RECORD(Header, REQUEST_PROCESS_CREATED, Header);

	if (Header->Flags & REQUEST_FLAG_COMPRESSED) {
		switch (Header->Type) {
			case ertDriverDetected:
				source = (char *)(drr + 1);
				charCount = drr->DriverNameLength;
				break;
			case ertDeviceDetected:
				source = (char *)(der + 1);
				charCount = der->DeviceNameLength;
				break;
			case ertFileObjectNameAssigned:
				ar = CONTAINING_RECORD(Header, REQUEST_FILE_OBJECT_NAME_ASSIGNED, Header);
				source = (char *)(ar + 1);
				charCount = ar->NameLength;
				break;
			case ertProcessCreated:
				source = (char *)(pcr + 1);
				charCount = (pcr->ImageNameLength + pcr->CommandLineLength);
				break;
		}
	}

	oldSize = RequestGetSize(Header);
	newSize = oldSize + charCount;
	newRequest = _RequestMemoryAlloc(newSize);
	if (newRequest != NULL) {
		memcpy(newRequest, Header, oldSize);
		target = (wchar_t *)((unsigned char *)newRequest + newSize - charCount*sizeof(wchar_t));
		switch (Header->Type) {
			case ertDriverDetected:
				drr = CONTAINING_RECORD(newRequest, REQUEST_DRIVER_DETECTED, Header);
				drr->DriverNameLength *= sizeof(wchar_t);
				break;
			case ertDeviceDetected:
				der = CONTAINING_RECORD(newRequest, REQUEST_DEVICE_DETECTED, Header);
				der->DeviceNameLength *= sizeof(wchar_t);
				break;
			case ertFileObjectNameAssigned:
				ar = CONTAINING_RECORD(newRequest, REQUEST_FILE_OBJECT_NAME_ASSIGNED, Header);;
				ar->NameLength *= sizeof(wchar_t);
				break;
			case ertProcessCreated:
				pcr = CONTAINING_RECORD(newRequest, REQUEST_PROCESS_CREATED, Header);
				pcr->ImageNameLength *= sizeof(wchar_t);
				pcr->CommandLineLength *= sizeof(wchar_t);
				break;
		}

		for (size_t i = 0; i < charCount; ++i)
			target[i] = source[i];

		newRequest->Flags &= (~REQUEST_FLAG_COMPRESSED);
	}

	return newRequest;
}


DWORD RequestEmulateDriverDetected(void *DriverObject, const wchar_t *DriverName, PREQUEST_DRIVER_DETECTED *Request)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PREQUEST_DRIVER_DETECTED tmpRequest = NULL;
	size_t driverNameLen = 0;

	ret = S_OK;
	if (DriverName != NULL)
		ret = StringCbLengthW(DriverName, 65536, &driverNameLen);

	if (ret == S_OK) {
		tmpRequest = _RequestMemoryAlloc(sizeof(REQUEST_DRIVER_DETECTED) + driverNameLen);
		if (tmpRequest != NULL) {
			_RequestHeaderInit(&tmpRequest->Header, DriverObject, NULL, ertDriverDetected);
			tmpRequest->DriverNameLength = (ULONG)driverNameLen;
			memcpy(tmpRequest + 1, DriverName, driverNameLen);;
			*Request = tmpRequest;
			ret = ERROR_SUCCESS;
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
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
		tmpRequest = _RequestMemoryAlloc(sizeof(REQUEST_DEVICE_DETECTED) + deviceNameLen);
		if (tmpRequest != NULL) {
			_RequestHeaderInit(&tmpRequest->Header, DriverObject, DeviceObject, ertDeviceDetected);
			tmpRequest->DeviceNameLength = (ULONG)deviceNameLen;
			memcpy(tmpRequest + 1, DeviceName, deviceNameLen);;
			*Request = tmpRequest;
			ret = ERROR_SUCCESS;
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
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
		tmpRequest = _RequestMemoryAlloc(sizeof(REQUEST_FILE_OBJECT_NAME_ASSIGNED) + fileNameLen);
		if (tmpRequest != NULL) {
			_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertFileObjectNameAssigned);
			tmpRequest->FileObject = FileObject;
			tmpRequest->NameLength = (ULONG)fileNameLen;
			memcpy(tmpRequest + 1, FileName, fileNameLen);;
			*Request = tmpRequest;
			ret = ERROR_SUCCESS;
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
	}

	return ret;
}


DWORD RequestEmulateFileNameDeleted(void *FileObject, PREQUEST_FILE_OBJECT_NAME_DELETED *Request)
{
	DWORD ret = ERROR_GEN_FAILURE;
	PREQUEST_FILE_OBJECT_NAME_DELETED tmpRequest = NULL;

	tmpRequest = _RequestMemoryAlloc(sizeof(REQUEST_FILE_OBJECT_NAME_DELETED));
	if (tmpRequest != NULL) {
		_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertFileObjectNameDeleted);
		tmpRequest->FileObject = FileObject;
		*Request = tmpRequest;
		ret = ERROR_SUCCESS;
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

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
		tmpRequest = _RequestMemoryAlloc(sizeof(REQUEST_PROCESS_CREATED) + imageNameLen + commandLineLen);
		if (tmpRequest != NULL) {
			_RequestHeaderInit(&tmpRequest->Header, NULL, NULL, ertProcessCreated);
			tmpRequest->ProcessId = ProcessId;
			tmpRequest->ParentId = ParentId;
			tmpRequest->ImageNameLength = (ULONG)imageNameLen;
			memcpy(tmpRequest + 1, ImageName, tmpRequest->ImageNameLength);
			tmpRequest->CommandLineLength = (ULONG)commandLineLen;
			memcpy((unsigned char *)(tmpRequest + 1) + tmpRequest->ImageNameLength, CommandLine, tmpRequest->CommandLineLength);
			*Request = tmpRequest;
			ret = ERROR_SUCCESS;
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
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
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}


void RequestEmulatedFree(PREQUEST_HEADER Header)
{
	_RequestMemoryFree(Header);

	return;
}

