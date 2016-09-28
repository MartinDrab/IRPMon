
#ifndef __PNP_DRIVER_WATCH_H__
#define __PNP_DRIVER_WATCH_H__

#include <ntifs.h>
#include "kernel-shared.h"
#include "hash_table.h"


typedef struct _DEVICE_CLASS_WATCH_RECORD {
	HASH_ITEM HashItem;
	GUID ClassGuid;
} DEVICE_CLASS_WATCH_RECORD, *PDEVICE_CLASS_WATCH_RECORD;

typedef struct _DRIVER_NAME_WATCH_RECORD {
	DRIVER_MONITOR_SETTINGS MonitorSettings;
} DRIVER_NAME_WATCH_RECORD, *PDRIVER_NAME_WATCH_RECORD;


NTSTATUS PDWClassRegister(PGUID ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning);
NTSTATUS PDWClassUnregister(PGUID ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning);
NTSTATUS PDWClassEnumerate(PIOCTL_IRPMNDRV_CLASS_WATCH_OUTPUT Buffer, SIZE_T Length, PSIZE_T ReturnLength, KPROCESSOR_MODE AccessMode);

NTSTATUS PWDDriverNameRegister(PUNICODE_STRING Name, PDRIVER_MONITOR_SETTINGS Settings);
NTSTATUS PWDDriverNameUnregister(PUNICODE_STRING Name);
NTSTATUS PWDDriverNameEnumerate(PIOCTL_IRPMNDRV_DRIVER_WATCH_ENUM_OUTPUT Buffer, SIZE_T Length, PSIZE_T ReturnLength, KPROCESSOR_MODE AccessMode);

NTSTATUS PWDModuleInit(PDRIVER_OBJECT DriverObject, PVOID Context);
VOID PWDModuleFinit(PDRIVER_OBJECT DriverObject, PVOID Context);



#endif
