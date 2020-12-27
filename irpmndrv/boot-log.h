
#ifndef __BOOT_LOG_H__
#define __BOOT_LOG_H__


#include <ntifs.h>
#include "general-types.h"



BOOLEAN BLEnabled(void);
void BLDisable(void);
NTSTATUS BLDriverNameSave(PUNICODE_STRING DriverName, const DRIVER_MONITOR_SETTINGS* Settings);
NTSTATUS BLDriverNameDelete(PUNICODE_STRING DriverName);

NTSTATUS BLModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void BLModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
