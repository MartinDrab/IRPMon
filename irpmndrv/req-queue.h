
#ifndef __REQ_QUEUE_H__
#define __REQ_QUEUE_H__

#include <ntifs.h>
#include "kernel-shared.h"


typedef void (REQUEST_QUEUE_CALLBACK)(PREQUEST_HEADER Header, void *Context);

typedef struct _REQUEST_QUEUE_CALLBACK_RECORD {
	LIST_ENTRY Entry;
	volatile LONG ReferenceCount;
	REQUEST_QUEUE_CALLBACK *Callback;
	void *Context;
	KEVENT Event;
} REQUEST_QUEUE_CALLBACK_RECORD, *PREQUEST_QUEUE_CALLBACK_RECORD;


void RequestHeaderInit(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType);
void RequestHeaderInitNoId(PREQUEST_HEADER Header, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, ERequesttype RequestType);
NTSTATUS RequestXXXDetectedCreate(ERequesttype Type, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, PREQUEST_HEADER *Header);
NTSTATUS RequestQueueGet(PREQUEST_HEADER *Buffer, PSIZE_T Length);
void RequestQueueInsert(PREQUEST_HEADER Header);
NTSTATUS ListDriversAndDevicesByEvents(PLIST_ENTRY ListHead);
void RequestQueueClear(void);

NTSTATUS RequestQueueConnect(void);
void RequestQueueDisconnect(void);

NTSTATUS RequestQueueCallbackRegister(REQUEST_QUEUE_CALLBACK *Callback, void *Context, PHANDLE Handle);
void RequestQueueCallbackUnregister(HANDLE Handle);

NTSTATUS RequestQueueModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
void RequestQueueModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);



#endif 
