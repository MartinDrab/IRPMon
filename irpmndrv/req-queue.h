
#ifndef __REQ_QUEUE_H__
#define __REQ_QUEUE_H__

#include <ntifs.h>
#include "kernel-shared.h"



NTSTATUS RequestQueueGet(PREQUEST_HEADER Buffer, PULONG Length);
VOID RequestQueueInsert(PREQUEST_HEADER Header);

NTSTATUS RequestQueueConnect(HANDLE hSemaphore);
VOID RequestQueueDisconnect(VOID);

NTSTATUS RequestQueueModuleInit(PDRIVER_OBJECT DriverObject, PVOID Context);
VOID RequestQueueModuleFinit(PDRIVER_OBJECT DriverObject, PVOID Context);



#endif 
