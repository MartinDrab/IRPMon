
#ifndef __DRIVER_SETTINGS_H__
#define __DRIVER_SETTINGS_H__



#include <ntifs.h>
#include "general-types.h"



PIRPMNDRV_SETTINGS DriverSettingsGet(void);
NTSTATUS DriverSettingsSet(const IRPMNDRV_SETTINGS *Settings, BOOLEAN Save);

NTSTATUS DriverSettingsInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
VOID DriverSettingsFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);


#endif
