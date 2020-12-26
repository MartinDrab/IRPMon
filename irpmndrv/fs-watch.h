
#ifndef __FS_WATCH_H__
#define __FS_WATCH_H__

#include <ntifs.h>



NTSTATUS FSWMonitor(BOOLEAN Monitor);

NTSTATUS FSWModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void FSWModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
