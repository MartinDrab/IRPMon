
#ifndef __PNP_CLASS_WATCH_H__
#define __PNP_CLASS_WATCH_H__


#include <ntifs.h>
#include "kernel-shared.h"
#include "hash_table.h"


typedef struct _DEVICE_CLASS_WATCH_RECORD {
	HASH_ITEM HashItem;
	GUID ClassGuid;
	UNICODE_STRING ClassGuidString;
	ULONG Flags;
	HANDLE KeyRecord;
	HANDLE ValueRecord;
	HANDLE CallbackHandle;
} DEVICE_CLASS_WATCH_RECORD, * PDEVICE_CLASS_WATCH_RECORD;


NTSTATUS CWRegister(PGUID ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning);
NTSTATUS CWUnregister(PGUID ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning);
NTSTATUS CWEnumerate(PIOCTL_IRPMNDRV_CLASS_WATCH_OUTPUT Buffer, SIZE_T Length, PSIZE_T ReturnLength, KPROCESSOR_MODE AccessMode);
void CWClear(void);

NTSTATUS CWModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void CWModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
