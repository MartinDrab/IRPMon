
#ifndef __KBASE_H__
#define __KBASE_H__


#include <ntifs.h>


NTSTATUS KBaseInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void KBaseFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
