
#ifndef __REQ_QUEUE_H__
#define __REQ_QUEUE_H__

#include <ntifs.h>
#include "kernel-shared.h"



void RequestHeaderInit(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType);
void RequestHeaderInitNoId(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType);
NTSTATUS RequestXXXDetectedCreate(ERequesttype Type, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PREQUEST_HEADER *Header);
NTSTATUS RequestQueueGet(PREQUEST_HEADER *Buffer, PSIZE_T Length);
void RequestQueueInsert(PREQUEST_HEADER Header);
NTSTATUS ListDriversAndDevicesByEvents(PLIST_ENTRY ListHead);
void RequestQueueClear(void);

NTSTATUS RequestQueueConnect(void);
VOID RequestQueueDisconnect(void);

NTSTATUS RequestQueueModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
VOID RequestQueueModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif 
