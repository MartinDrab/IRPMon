
#ifndef __SHARED_REQUEST_H__
#define __SHARED_REQUEST_H__



#include "general-types.h"



size_t RequestGetSize(const REQUEST_HEADER *Header);
#ifndef _KERNEL_MODE
DWORD RequestEmulateDriverDetected(void *DriverObject, const wchar_t *DriverName, PREQUEST_DRIVER_DETECTED *Request);
DWORD RequestEmulateDeviceDetected(void *DriverObject, void *DeviceObject, const wchar_t *DeviceName, PREQUEST_DEVICE_DETECTED *Request);
#endif


#endif
