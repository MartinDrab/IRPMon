
#ifndef __PNP_DRIVER_WATCH_H__
#define __PNP_DRIVER_WATCH_H__

#include <ntifs.h>
#include "ioctls.h"
#include "kernel-shared.h"
#include "hash_table.h"


typedef struct _DRIVER_NAME_WATCH_RECORD {
	DRIVER_MONITOR_SETTINGS MonitorSettings;
} DRIVER_NAME_WATCH_RECORD, *PDRIVER_NAME_WATCH_RECORD;


NTSTATUS PDWCheckDriver(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject);
void PDWCheckDrivers(void);

NTSTATUS PWDDriverNameRegister(PUNICODE_STRING Name, const DRIVER_MONITOR_SETTINGS *Settings);
NTSTATUS PWDDriverNameUnregister(PUNICODE_STRING Name);
NTSTATUS PWDDriverNameEnumerate(PIOCTL_IRPMNDRV_DRIVER_WATCH_ENUM_OUTPUT Buffer, SIZE_T Length, PSIZE_T ReturnLength, KPROCESSOR_MODE AccessMode);

NTSTATUS PWDModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void PWDModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
