
#ifndef __SHARED_REQUEST_H__
#define __SHARED_REQUEST_H__



#include "general-types.h"
#ifdef _KERNEL_MODE
#include "utils.h"
#endif


size_t RequestGetSize(const REQUEST_HEADER *Header);
BOOLEAN RequestCompress(PREQUEST_HEADER Header);
PREQUEST_HEADER RequestDecompress(const REQUEST_HEADER *Header);
DWORD RequestEmulateDriverDetected(void *DriverObject, const wchar_t *DriverName, PREQUEST_DRIVER_DETECTED *Request);
DWORD RequestEmulateDeviceDetected(void *DriverObject, void *DeviceObject, const wchar_t *DeviceName, PREQUEST_DEVICE_DETECTED *Request);
DWORD RequestEmulateFileNameAssigned(void *FileObject, const wchar_t *FileName, PREQUEST_FILE_OBJECT_NAME_ASSIGNED *Request);
DWORD RequestEmulateFileNameDeleted(void *FileObject, PREQUEST_FILE_OBJECT_NAME_DELETED *Request);
DWORD RequestEmulateProcessCreated(HANDLE ProcessId, HANDLE ParentId, const wchar_t *ImageName, const wchar_t *CommandLine, PREQUEST_PROCESS_CREATED *Request);
DWORD RequestEmulateProcessExitted(HANDLE ProcessId, PREQUEST_PROCESS_EXITTED *Request);
void RequestEmulatedFree(PREQUEST_HEADER Header);
#ifdef _KERNEL_MODE
void _SetRequestFlags(PREQUEST_HEADER Request, const BASIC_CLIENT_INFO *Info);
#endif


#endif
