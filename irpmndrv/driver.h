
#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <ntifs.h>


#define IRPMNDRV_DEVICE_NAME           L"\\Device\\IRPMnDrv"
#define IRPMNDRV_SYMBOLIC_LINK         L"\\DosDevices\\IRPMnDrv"


NTSTATUS DriverInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
VOID DriverFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);


#endif 
