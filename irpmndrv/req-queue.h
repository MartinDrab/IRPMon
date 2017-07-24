
#ifndef __REQ_QUEUE_H__
#define __REQ_QUEUE_H__

#include <ntifs.h>
#include "kernel-shared.h"


VOID RequestHeaderInit(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType);
NTSTATUS RequestXXXDetectedCreate(ERequesttype Type, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PREQUEST_HEADER *Header);
NTSTATUS RequestProcessCreatedCreated(HANDLE ProcessId, HANDLE ParentId, HANDLE CreatorId, PCUNICODE_STRING ImageName, PCUNICODE_STRING CommandLine, PREQUEST_PROCESS_CREATED *Request);
NTSTATUS RequestProcessExittedCreate(HANDLE ProcessId, PREQUEST_PROCESS_EXITTED *Request);
NTSTATUS RequestQueueGet(PREQUEST_HEADER Buffer, PULONG Length);
VOID RequestQueueInsert(PREQUEST_HEADER Header);

NTSTATUS RequestQueueConnect(HANDLE hSemaphore);
VOID RequestQueueDisconnect(VOID);

NTSTATUS RequestQueueModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
VOID RequestQueueModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif 
