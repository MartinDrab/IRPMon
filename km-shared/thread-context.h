
#ifndef __THREAD_CONTEXT_H__
#define __THREAD_CONTEXT_H__


#include <ntifs.h>
#include "hash_table.h"



typedef struct _THREAD_CONTEXT_RECORD {
	HASH_ITEM Item;
	volatile LONG ReferenceCount;
	HANDLE ThreadId;
	volatile LONG StackTraceInProgress;
} THREAD_CONTEXT_RECORD, *PTHREAD_CONTEXT_RECORD;




void ThreadContextReference(PTHREAD_CONTEXT_RECORD Record);
void ThreadContextDereference(PTHREAD_CONTEXT_RECORD Record);
PTHREAD_CONTEXT_RECORD ThreadContextGet(void);
void ThreadContextDelete(void);

NTSTATUS ThreadContextModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void ThreadContextModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif
