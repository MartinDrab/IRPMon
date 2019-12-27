
#ifndef __SHARED_REQUEST_H__
#define __SHARED_REQUEST_H__



#include "general-types.h"
#ifdef _KERNEL_MODE
#include "utils.h"
#endif



#ifdef _KERNEL_MODE

#define ERROR_TYPE				NTSTATUS
#define ERROR_VALUE_SUCCESS		STATUS_SUCCESS
#define ERROR_VALUE_NOMEM		STATUS_INSUFFICIENT_RESOURCES
#define ERROR_VALUE_INVAL		STATUS_UNSUCCESSFUL

#else

#define ERROR_TYPE				DWORD
#define ERROR_VALUE_SUCCESS		ERROR_SUCCESS
#define ERROR_VALUE_NOMEM		ERROR_NOT_ENOUGH_MEMORY
#define ERROR_VALUE_INVAL		ERROR_GEN_FAILURE

#endif



size_t RequestGetSize(const REQUEST_HEADER *Header);

BOOLEAN RequestCompress(PREQUEST_HEADER Header);
PREQUEST_HEADER RequestDecompress(const REQUEST_HEADER *Header);
PREQUEST_HEADER RequestCopy(const REQUEST_HEADER *Header);

ERROR_TYPE RequestEmulateDriverDetected(void *DriverObject, const wchar_t *DriverName, PREQUEST_DRIVER_DETECTED *Request);
ERROR_TYPE RequestEmulateDeviceDetected(void *DriverObject, void *DeviceObject, const wchar_t *DeviceName, PREQUEST_DEVICE_DETECTED *Request);
ERROR_TYPE RequestEmulateFileNameAssigned(void *FileObject, const wchar_t *FileName, PREQUEST_FILE_OBJECT_NAME_ASSIGNED *Request);
ERROR_TYPE RequestEmulateFileNameDeleted(void *FileObject, PREQUEST_FILE_OBJECT_NAME_DELETED *Request);
ERROR_TYPE RequestEmulateProcessCreated(HANDLE ProcessId, HANDLE ParentId, const wchar_t *ImageName, const wchar_t *CommandLine, PREQUEST_PROCESS_CREATED *Request);
ERROR_TYPE RequestEmulateProcessExitted(HANDLE ProcessId, PREQUEST_PROCESS_EXITTED *Request);

#ifdef _KERNEL_MODE
void _SetRequestFlags(PREQUEST_HEADER Request, const BASIC_CLIENT_INFO *Info);
#endif

PREQUEST_HEADER RequestMemoryAlloc(size_t Size);
void RequestMemoryFree(PREQUEST_HEADER Request);



#endif
