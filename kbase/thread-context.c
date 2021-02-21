
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include <fltkernel.h>
#include "hash_table.h"
#include "thread-context.h"




static PHASH_TABLE _contextTable = NULL;
static EX_PUSH_LOCK _contextLock;



static ULONG32 _HashFunction(PVOID Key)
{
	return (ULONG32)(ULONG_PTR)Key >> 2;
}


static BOOLEAN _CompareFunction(PHASH_ITEM Item, PVOID Key)
{
	PTHREAD_CONTEXT_RECORD r = CONTAINING_RECORD(Item, THREAD_CONTEXT_RECORD, Item);

	return (r->ThreadId == (HANDLE)Key);
}


static void _FreeFunction(PHASH_ITEM Item)
{
	PTHREAD_CONTEXT_RECORD r = CONTAINING_RECORD(Item, THREAD_CONTEXT_RECORD, Item);
	DEBUG_ENTER_FUNCTION("Item=0x%p", Item);

	ThreadContextDereference(r);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static void _ThreadNotify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create)
{
	if (!Create)
		ThreadContextDelete();


	return;
}


void ThreadContextReference(PTHREAD_CONTEXT_RECORD Record)
{
	InterlockedIncrement(&Record->ReferenceCount);

	return;
}


void ThreadContextDereference(PTHREAD_CONTEXT_RECORD Record)
{
	if (InterlockedDecrement(&Record->ReferenceCount) == 0)
		HeapMemoryFree(Record);


	return;
}


PTHREAD_CONTEXT_RECORD ThreadContextGet(void)
{
	PHASH_ITEM h = NULL;
	PTHREAD_CONTEXT_RECORD ret = NULL;

	FltAcquirePushLockShared(&_contextLock);
	h = HashTableGet(_contextTable, PsGetCurrentThreadId());
	if (h != NULL) {
		ret = CONTAINING_RECORD(h, THREAD_CONTEXT_RECORD, Item);
		ThreadContextReference(ret);
	}

	FltReleasePushLock(&_contextLock);
	if (ret == NULL) {
		ret = HeapMemoryAllocPaged(sizeof(THREAD_CONTEXT_RECORD));
		if (ret != NULL) {
			RtlSecureZeroMemory(ret, sizeof(THREAD_CONTEXT_RECORD));
			InterlockedExchange(&ret->ReferenceCount, 1);
			ret->ThreadId = PsGetCurrentThreadId();
			FltAcquirePushLockExclusive(&_contextLock);
			HashTableInsert(_contextTable, &ret->Item, ret->ThreadId);
			FltReleasePushLock(&_contextLock);
			ThreadContextReference(ret);
		}
	}


	return ret;
}


void ThreadContextDelete(void)
{

	PHASH_ITEM h = NULL;
	PTHREAD_CONTEXT_RECORD r = NULL;

	FltAcquirePushLockExclusive(&_contextLock);
	h = HashTableDelete(_contextTable, PsGetCurrentThreadId());
	if (h != NULL) {
		r = CONTAINING_RECORD(h, THREAD_CONTEXT_RECORD, Item);
		ThreadContextDereference(r);
	}

	FltReleasePushLock(&_contextLock);

	return;
}


NTSTATUS ThreadContextModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	FltInitializePushLock(&_contextLock);
	status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _CompareFunction, _FreeFunction, &_contextTable);
	if (NT_SUCCESS(status)) {
		status = PsSetCreateThreadNotifyRoutine(_ThreadNotify);
		if (!NT_SUCCESS(status))
			HashTableDestroy(_contextTable);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


void ThreadContextModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	PsRemoveCreateThreadNotifyRoutine(_ThreadNotify);
	HashTableDestroy(_contextTable);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
