
#ifndef __VRTULETREE_UTILS_H_
#define __VRTULETREE_UTILS_H_

#include <ntifs.h>
#include "kbase-exports.h"
#include "kernel-shared.h"


typedef NTSTATUS (DEVICE_CONDITION_CALLBACK)(PDEVICE_OBJECT DeviceObject, PVOID Context, PVOID ReturnBuffer, ULONG ReturnBufferLength);


KBASE_API
VOID _ReleaseDriverArray(PDRIVER_OBJECT *DriverArray, SIZE_T DriverCount);
KBASE_API
VOID _ReleaseDeviceArray(PDEVICE_OBJECT *DeviceArray, SIZE_T ArrayLength);
KBASE_API
NTSTATUS _GetObjectName(PVOID Object, PUNICODE_STRING Name);
KBASE_API
NTSTATUS _GetDriversInDirectory(PUNICODE_STRING Directory, PDRIVER_OBJECT **DriverArray, PSIZE_T DriverCount);
KBASE_API
NTSTATUS _EnumDriverDevices(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT **DeviceArray, PULONG DeviceArrayLength);
KBASE_API
NTSTATUS _GetDeviceAddress(PUNICODE_STRING DeviceName, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PDEVICE_OBJECT *Object);
KBASE_API
NTSTATUS GetDriverObjectByName(PUNICODE_STRING Name, PDRIVER_OBJECT *DriverObject);
KBASE_API
NTSTATUS VerifyDeviceByAddress(PVOID Address, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PDEVICE_OBJECT *Object);



#endif
