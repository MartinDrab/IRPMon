
#ifndef __PROCESS_EVENTS_H__
#define __PROCESS_EVENTS_H__

#include <ntifs.h>



NTSTATUS ListProcessesByEvents(PLIST_ENTRY EventListHead);
NTSTATUS RecordImageLoad(const REQUEST_IMAGE_LOAD *Request);

NTSTATUS ProcessEventsModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
VOID ProcessEventsModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
