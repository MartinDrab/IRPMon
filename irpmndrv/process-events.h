
#ifndef __PROCESS_EVENTS_H__
#define __PROCESS_EVENTS_H__









NTSTATUS ProcessEventsModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
VOID ProcessEventsModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
