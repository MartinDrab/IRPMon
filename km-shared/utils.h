
#ifndef __VRTULETREE_UTILS_H_
#define __VRTULETREE_UTILS_H_

#include <ntifs.h>
#include "kernel-shared.h"


typedef NTSTATUS (DEVICE_CONDITION_CALLBACK)(PDEVICE_OBJECT DeviceObject, PVOID Context, PVOID ReturnBuffer, ULONG ReturnBufferLength);


VOID _ReleaseDriverArray(PDRIVER_OBJECT *DriverArray, SIZE_T DriverCount);
VOID _ReleaseDeviceArray(PDEVICE_OBJECT *DeviceArray, SIZE_T ArrayLength);
NTSTATUS _GetObjectName(PVOID Object, PUNICODE_STRING Name);
NTSTATUS _GetDriversInDirectory(PUNICODE_STRING Directory, PDRIVER_OBJECT **DriverArray, PSIZE_T DriverCount);
NTSTATUS _EnumDriverDevices(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT **DeviceArray, PULONG DeviceArrayLength);
NTSTATUS _GetDeviceAddress(PUNICODE_STRING DeviceName, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PDEVICE_OBJECT *Object);
NTSTATUS GetDriverObjectByName(PUNICODE_STRING Name, PDRIVER_OBJECT *DriverObject);
NTSTATUS VerifyDeviceByAddress(PVOID Address, BOOLEAN SearchDrivers, BOOLEAN SearchFileSystems, PDEVICE_OBJECT *Object);



#endif
