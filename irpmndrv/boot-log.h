
#ifndef __BOOT_LOG_H__
#define __BOOT_LOG_H__


#include <ntifs.h>
#include "general-types.h"



void BLLogRequest(PREQUEST_HEADER Request);
BOOLEAN BLEnabled(void);
void BLDisable(void);

NTSTATUS BLModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void BLModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
