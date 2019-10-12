
#ifndef __CUSTOM_HANDLE_TABLE_H__
#define __CUSTOM_HANDLE_TABLE_H__

#include <ntifs.h>
#include "hash_table.h"


struct _CHANDLE_TABLE;


typedef VOID (NTAPI CHANDLE_TABLE_HANDLE_CREATED)(struct _CHANDLE_TABLE *HandleTable, PVOID Object, HANDLE Handle);
typedef VOID (NTAPI CHANDLE_TABLE_HANDLE_DELETED)(struct _CHANDLE_TABLE *HandleTable, PVOID Object, HANDLE Handle);
typedef VOID (NTAPI CHANDLE_TABLE_HANDLE_TRANSLATED)(struct _CHANDLE_TABLE *HandleTable, PVOID Object, HANDLE Handle);

typedef struct _CHANDLE_TABLE {
	PHASH_TABLE HashTable;
	KSPIN_LOCK LockD;
	PKTHREAD ExclusiveLocker;
	ERESOURCE LockP;
	volatile LONG NextFreeHandle;
	EHashTableType HandleTableType;
	POOL_TYPE PoolType;
	CHANDLE_TABLE_HANDLE_CREATED *HandleCreateProcedure;
	CHANDLE_TABLE_HANDLE_TRANSLATED *HandleTranslateProcedure;
	CHANDLE_TABLE_HANDLE_DELETED *HandleDeleteProcedure;
} CHANDLE_TABLE, *PCHANDLE_TABLE;

typedef struct _CHANDLE_TABLE_MAPPING {
	HASH_ITEM HashItem;
	HANDLE HandleValue;
	PVOID Object;
	PCHANDLE_TABLE HandleTable;
} CHANDLE_TABLE_MAPPING, *PCHANDLE_TABLE_MAPPING;


NTSTATUS HandleTableCreate(EHashTableType HandleTableType, CHANDLE_TABLE_HANDLE_CREATED *HandleCreateProcedure, CHANDLE_TABLE_HANDLE_DELETED *HandleDeleteProcedure, CHANDLE_TABLE_HANDLE_TRANSLATED *HandleTranslateProcedure, PCHANDLE_TABLE *HandleTable);
VOID HandleTableDestroy(PCHANDLE_TABLE HandleTable);
VOID HandleTableClear(PCHANDLE_TABLE HandleTable);

NTSTATUS HandleTableHandleCreate(PCHANDLE_TABLE HandleTable, PVOID Object, PHANDLE NewHandle);
NTSTATUS HandleTableHandleClose(PCHANDLE_TABLE HandleTable, HANDLE Handle);
NTSTATUS HandleTablehandleTranslate(PCHANDLE_TABLE HandleTable, HANDLE Handle, PVOID *Object);
NTSTATUS HandleTableHandleDuplicate(PCHANDLE_TABLE HandleTable, HANDLE Handle, PHANDLE NewHandle);



#endif 
