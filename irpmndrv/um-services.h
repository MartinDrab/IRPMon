
#ifndef __UM_SERVICES_H__
#define __UM_SERVICES_H__

#include <ntifs.h>
#include "kernel-shared.h"

typedef struct _UM_DEVICE_INFO {
	PDEVICE_OBJECT DeviceObject;
	PDEVICE_OBJECT AttachedDevice;
	UNICODE_STRING DeviceName;
} UM_DEVICE_INFO, *PUM_DEVICE_INFO;

typedef struct _UM_DRIVER_INFO {
	PDRIVER_OBJECT DriverObject;
	UNICODE_STRING DriverName;
	ULONG DeviceCount;
	PDEVICE_OBJECT *Devices;
	PUM_DEVICE_INFO DeviceInfo;
} UM_DRIVER_INFO, *PUM_DRIVER_INFO;


NTSTATUS UMHookDriver(PIOCTL_IRPMNDRV_HOOK_DRIVER_INPUT InputBuffer, ULONG InputBufferLength, PIOCTL_IRPMNDRV_HOOK_DRIVER_OUTPUT OutputBuffer, ULONG OutputBufferLength);
NTSTATUS UMUnhookDriver(PIOCTL_IRPMNDRV_UNHOOK_DRIVER_INPUT InputBuffer, ULONG InputBufferLength);
NTSTATUS UMHookAddDevice(PIOCTL_IRPMNDRV_HOOK_ADD_DEVICE_INPUT InputBUffer, ULONG InputBufferLength, PIOCTL_IRPMNDRV_HOOK_ADD_DEVICE_OUTPUT OutputBuffer, ULONG OutputBufferLength);
NTSTATUS UMHookDeleteDevice(PIOCTL_IRPMNDRV_HOOK_REMOVE_DEVICE_INPUT InputBuffer, ULONG InputBufferLength);
NTSTATUS UMGetRequestRecord(PVOID Buffer, ULONG BufferLength, PSIZE_T ReturnLength);
NTSTATUS UMEnumDriversDevices(PVOID OutputBuffer, ULONG OutputBufferLength, PULONG ReturnLength);
NTSTATUS UMRequestQueueConnect(void);
VOID UMRequestQueueDisconnect(VOID);
void UMRequestQueueClear(void);

NTSTATUS UMHookedDriverSetInfo(PIOCTL_IRPMNDRV_HOOK_DRIVER_SET_INFO_INPUT InputBuffer, ULONG InputBufferLength);
NTSTATUS UMHookedDriverGetInfo(PIOCTL_IRPMNDRV_HOOK_DRIVER_GET_INFO_INPUT InputBuffer, ULONG InputBufferLength, PIOCTL_IRPMNDRV_HOOK_DRIVER_GET_INFO_OUTPUT OutputBuffer, ULONG OutputBufferLength);
NTSTATUS UMHookedDeviceSetInfo(PIOCTL_IRPMNDRV_HOOK_DEVICE_SET_INFO_INPUT InputBuffer, ULONG InputBufferLength);
NTSTATUS UMHookedDeviceGetInfo(PIOCTL_IRPMNDRV_HOOK_DEVICE_GET_INFO_INPUT InputBuffer, ULONG InputBufferLength, PIOCTL_IRPMNDRV_HOOK_DEVICE_GET_INFO_OUTPUT OutputBuffer, ULONG OutputBufferLength);

NTSTATUS UMHookedDriverMonitoringEnable(PIOCTL_IRPMNDRV_HOOK_DRIVER_MONITORING_CHANGE_INPUT InputBuffer, ULONG InputBufferLength);
NTSTATUS UMHookedObjectsEnumerate(PIOCTL_IRPMONDRV_HOOK_GET_INFO_OUTPUT OutputBuffer, ULONG OutputBufferLength);
VOID UMDeleteHandlesForProcess(PEPROCESS Process);

NTSTATUS UMOpenObjectById(PIOCTL_IRPMONDRV_HOOK_OPEN_INPUT InputBuffer, ULONG InputBufferLength, PIOCTL_IRPMONDRV_HOOK_OPEN_OUTPUT OutputBuffer, ULONG OutputBufferLength);
NTSTATUS UMCloseHandle(PIOCTL_IRPMONDRV_HOOK_CLOSE_INPUT InputBuffer, ULONG InputBufferLength);

NTSTATUS UMClassWatchRegister(PIOCTL_IRPMNDRV_CLASS_WATCH_REGISTER_INPUT InputBuffer, ULONG InputBufferLength);
NTSTATUS UMClassWatchUnregister(PIOCTL_IRPMNDRV_CLASS_WATCH_UNREGISTER_INPUT InputBuffer, ULONG InputBUfferLength);
NTSTATUS UMDriverNameWatchRegister(PIOCTL_IRPMNDRV_DRIVER_WATCH_REGISTER_INPUT InputBuffer, ULONG InputBufferLength);
NTSTATUS UMDriverNamehUnregister(PIOCTL_IRPMNDRV_DRIVER_WATCH_UNREGISTER_INPUT InputBuffer, ULONG InputBUfferLength);

NTSTATUS UMListDriversDevicesByEvents(void);
NTSTATUS UMListProcessesByEvents(void);

NTSTATUS UMDriverSettingsQuery(PIOCTL_IRPMNDRV_SETTINGS_QUERY_OUTPUT OutputBuffer, ULONG OutputBufferLength, PULONG_PTR ReturnLength);
NTSTATUS UMDriverSettingsSet(PIOCTL_IRPMNDRV_SETTINGS_SET_INPUT InputBuffer, ULONG InputBufferLength);

NTSTATUS UMServicesModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
VOID UMServicesModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif 
