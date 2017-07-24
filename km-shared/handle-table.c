
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hash_table.h"
#include "handle-table.h"


/************************************************************************/
/*                       HELPER FUNCTIONS                               */
/************************************************************************/

static ULONG32 _HashFunction(PVOID Key)
{
	return PtrToUlong(Key);
}

static BOOLEAN _CompareFunction(PHASH_ITEM ObjectInTable, PVOID Key)
{
	PCHANDLE_TABLE_MAPPING mapping = CONTAINING_RECORD(ObjectInTable, CHANDLE_TABLE_MAPPING, HashItem);

	return (mapping->HandleValue == (HANDLE)Key);
}

static VOID _FreeFunction(PHASH_ITEM ObjectInTable)
{
	PVOID object = NULL;
	HANDLE handleValue = NULL;
	PCHANDLE_TABLE handleTable = NULL;

	PCHANDLE_TABLE_MAPPING mapping = CONTAINING_RECORD(ObjectInTable, CHANDLE_TABLE_MAPPING, HashItem);
	DEBUG_ENTER_FUNCTION("ObjectInTable=0x%p", ObjectInTable);

	handleTable = mapping->HandleTable;
	object = mapping->Object;
	handleValue = mapping->HandleValue;
	HeapMemoryFree(mapping);
	handleTable->HandleDeleteProcedure(handleTable, object, handleValue);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static VOID _HandleTableLockShared(PCHANDLE_TABLE HandleTable, PKIRQL Irql)
{
	switch (HandleTable->HandleTableType) {
		case httPassiveLevel:
			KeEnterCriticalRegion();
			ExAcquireResourceSharedLite(&HandleTable->LockP, TRUE);
			break;
		case httDispatchLevel:
			KeAcquireSpinLock(&HandleTable->LockD, Irql);
			break;
		case httNoSynchronization:
			break;
		default:
			ASSERT(FALSE);
			break;
	}

	return;
}

static VOID _HandleTableLockExclusive(PCHANDLE_TABLE HandleTable, PKIRQL Irql)
{
	switch (HandleTable->HandleTableType) {
		case httPassiveLevel:
			KeEnterCriticalRegion();
			ExAcquireResourceExclusiveLite(&HandleTable->LockP, TRUE);
			break;
		case httDispatchLevel:
			KeAcquireSpinLock(&HandleTable->LockD, Irql);
			ASSERT(HandleTable->ExclusiveLocker == NULL);
			HandleTable->ExclusiveLocker = KeGetCurrentThread();
			break;
		case httNoSynchronization:
			break;
		default:
			ASSERT(FALSE);
			break;
	}

	return;
}

static VOID _HandleTableUnlock(PCHANDLE_TABLE HandleTable, KIRQL Irql)
{
	switch (HandleTable->HandleTableType) {
		case httPassiveLevel:
			ExReleaseResourceLite(&HandleTable->LockP);
			KeLeaveCriticalRegion();
			break;
	case httDispatchLevel:
		ASSERT(HandleTable->ExclusiveLocker == KeGetCurrentThread());
		HandleTable->ExclusiveLocker = NULL;
		KeReleaseSpinLock(&HandleTable->LockD, Irql);
		break;
	case httNoSynchronization:
		break;
	default:
		ASSERT(FALSE);
		break;
	}

	return;
}

/************************************************************************/
/*                         PUBLIC FUNCTIONS                             */
/************************************************************************/

NTSTATUS HandleTableCreate(EHashTableType HandleTableType, CHANDLE_TABLE_HANDLE_CREATED *HandleCreateProcedure, CHANDLE_TABLE_HANDLE_DELETED *HandleDeleteProcedure, CHANDLE_TABLE_HANDLE_TRANSLATED *HandleTranslateProcedure, PCHANDLE_TABLE *HandleTable)
{
	POOL_TYPE poolType = (HandleTableType == httPassiveLevel) ? PagedPool : NonPagedPool;
	PCHANDLE_TABLE tmpHandleTable = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("HandleTableType=%u; HandleCreateProcedure=0x%p; HandleDeleteProcedure=0x%p; HandleTranslateProcedure=0x%p; HandleTable=0x%p", HandleTableType, HandleCreateProcedure, HandleDeleteProcedure, HandleTranslateProcedure, HandleTable);

	tmpHandleTable = (PCHANDLE_TABLE)HeapMemoryAllocNonPaged(sizeof(CHANDLE_TABLE));
	if (tmpHandleTable != NULL) {
		tmpHandleTable->NextFreeHandle = 1;
		tmpHandleTable->HandleTableType = HandleTableType;
		tmpHandleTable->PoolType = poolType;
		tmpHandleTable->HandleCreateProcedure = HandleCreateProcedure;
		tmpHandleTable->HandleDeleteProcedure = HandleDeleteProcedure;
		tmpHandleTable->HandleTranslateProcedure = HandleTranslateProcedure;
		switch (tmpHandleTable->HandleTableType) {
			case httPassiveLevel:
				status = ExInitializeResourceLite(&tmpHandleTable->LockP);
				break;
			case httDispatchLevel:
				KeInitializeSpinLock(&tmpHandleTable->LockD);
				tmpHandleTable->ExclusiveLocker = NULL;
				status = STATUS_SUCCESS;
				break;
			case httNoSynchronization:
				status = STATUS_SUCCESS;
				break;
			default:
				status = STATUS_NOT_SUPPORTED;
				break;
		}

		if (NT_SUCCESS(status)) {
			status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _CompareFunction, _FreeFunction, &tmpHandleTable->HashTable);
			if (NT_SUCCESS(status))
				*HandleTable = tmpHandleTable;

			if (!NT_SUCCESS(status)) {
				switch (tmpHandleTable->HandleTableType) {
					case httPassiveLevel:
						ExDeleteResourceLite(&tmpHandleTable->LockP);
						break;
					case httDispatchLevel:
						break;
					case httNoSynchronization:
						break;
					default:
						break;
				}
			}
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(tmpHandleTable);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *HandleTable=0x%p", status, *HandleTable);
	return status;
}

VOID HandleTableDestroy(PCHANDLE_TABLE HandleTable)
{
	DEBUG_ENTER_FUNCTION("HandleTable=0x%p", HandleTable);

	HashTableDestroy(HandleTable->HashTable);
	switch (HandleTable->HandleTableType) {
		case httPassiveLevel:
			ExDeleteResourceLite(&HandleTable->LockP);
			break;
		case httDispatchLevel:
			break;
		case httNoSynchronization:
			break;
		default:
			break;
	}

	HeapMemoryFree(HandleTable);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

VOID HandleTableClear(PCHANDLE_TABLE HandleTable)
{
	KIRQL irql;
	DEBUG_ENTER_FUNCTION("Handletable=0x%p", HandleTable);

	_HandleTableLockExclusive(HandleTable, &irql);
	HashTableClear(HandleTable->HashTable, TRUE);
	_HandleTableUnlock(HandleTable, irql);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

NTSTATUS HandleTableHandleCreate(PCHANDLE_TABLE HandleTable, PVOID Object, PHANDLE NewHandle)
{
	KIRQL irql;
	PCHANDLE_TABLE_MAPPING mapping = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("HandleTable=0x%p; Object=0x%p; NewHandle=0x%p", HandleTable, Object, NewHandle);

	mapping = (PCHANDLE_TABLE_MAPPING)HeapMemoryAlloc(HandleTable->PoolType, sizeof(CHANDLE_TABLE_MAPPING));
	if (mapping != NULL) {
		mapping->HandleTable = HandleTable;
		mapping->HandleValue = (HANDLE)InterlockedIncrement(&HandleTable->NextFreeHandle);
		mapping->Object = Object;
		HandleTable->HandleCreateProcedure(mapping->HandleTable, mapping->Object, mapping->HandleValue);
		_HandleTableLockExclusive(HandleTable, &irql);
		HashTableInsert(HandleTable->HashTable, &mapping->HashItem, mapping->HandleValue);
		_HandleTableUnlock(HandleTable, irql);

		*NewHandle = mapping->HandleValue;
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *NewHandle=0x%p", status, *NewHandle);
	return status;
}

NTSTATUS HandleTablehandleTranslate(PCHANDLE_TABLE HandleTable, HANDLE Handle, PVOID *Object)
{
	KIRQL irql;
	PCHANDLE_TABLE_MAPPING mapping = NULL;
	PHASH_ITEM hashItem = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Handletable=0x%p; Handle=0x%p; Object=0x%p", HandleTable, Handle, Object);

	_HandleTableLockShared(HandleTable, &irql);
	hashItem = HashTableGet(HandleTable->HashTable, Handle);
	if (hashItem != NULL) {
		PVOID object = NULL;
		HANDLE handleValue = NULL;
		PCHANDLE_TABLE handleTable = NULL;

		mapping = CONTAINING_RECORD(hashItem, CHANDLE_TABLE_MAPPING, HashItem);
		object = mapping->Object;
		handleValue = mapping->HandleValue;
		handleTable = mapping->HandleTable;
		_HandleTableUnlock(HandleTable, irql);
		HandleTable->HandleTranslateProcedure(handleTable, object, handleValue);
		*Object = object;
		status = STATUS_SUCCESS;
	} else {
		_HandleTableUnlock(HandleTable, irql);
		status = STATUS_INVALID_HANDLE;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Object=0x%p", status, *Object);
	return status;
}

NTSTATUS HandleTableHandleDuplicate(PCHANDLE_TABLE HandleTable, HANDLE Handle, PHANDLE NewHandle)
{
	KIRQL irql;
	PCHANDLE_TABLE_MAPPING newMapping = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("HandleTable=0x%p; Handle=0x%p; NewHandle=0x%p", HandleTable, Handle, NewHandle);

	newMapping = (PCHANDLE_TABLE_MAPPING)HeapMemoryAlloc(HandleTable->PoolType, sizeof(CHANDLE_TABLE_MAPPING));
	if (newMapping != NULL) {
		PVOID object = NULL;

		status = HandleTablehandleTranslate(HandleTable, Handle, &object);
		if (NT_SUCCESS(status)) {
			newMapping->HandleTable = HandleTable;
			newMapping->Object = object;
			_HandleTableLockExclusive(HandleTable, &irql);
			newMapping->HandleValue = (HANDLE)InterlockedIncrement(&HandleTable->NextFreeHandle);
			HashTableInsert(HandleTable->HashTable, &newMapping->HashItem, newMapping->HandleValue);
			_HandleTableUnlock(HandleTable, irql);
			*NewHandle = newMapping->HandleValue;
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(newMapping);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *NewHandle", status, *NewHandle);
	return status;
}


NTSTATUS HandleTableHandleClose(PCHANDLE_TABLE HandleTable, HANDLE Handle)
{
	KIRQL irql;
	PHASH_ITEM hashItem = NULL;
	PCHANDLE_TABLE_MAPPING mapping = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("HandleTable=0x%p; Handle=0x%p", HandleTable, Handle);

	_HandleTableLockExclusive(HandleTable, &irql);
	hashItem = HashTableDelete(HandleTable->HashTable, Handle);
	if (hashItem != NULL) {
		PVOID object = NULL;
		HANDLE handleValue = NULL;
		PCHANDLE_TABLE handleTable = NULL;
		
		mapping = CONTAINING_RECORD(hashItem, CHANDLE_TABLE_MAPPING, HashItem);
		object = mapping->Object;
		handleValue = mapping->HandleValue;
		HandleTable = mapping->HandleTable;
		_HandleTableUnlock(HandleTable, irql);
		HeapMemoryFree(mapping);
		HandleTable->HandleDeleteProcedure(handleTable, object, handleValue);
	} else {
		_HandleTableUnlock(HandleTable, irql);
		status = STATUS_INVALID_HANDLE;
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}
