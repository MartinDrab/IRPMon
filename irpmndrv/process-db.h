
#ifndef __PROCESS_DB_H__
#define __PROCESS_DB_H__

#include <ntifs.h>
#include "hash_table.h"
#include "req-queue.h"



typedef enum _EProcessDBRecordOrigin {
	pdbroProcessNotify,
	pdbroProcessEnumeration,
} EProcessDBRecordOrigin, *PEProcessDBRecordOrigin;

typedef struct _PROCESSDB_RECORD {
	HASH_ITEM Item;
	volatile LONG ReferenceCount;
	EProcessDBRecordOrigin Origin;
	UNICODE_STRING ImageName;
	UNICODE_STRING CommandLine;
	HANDLE ProcessId;
	HANDLE ParentId;
	HANDLE CreatorId;
	PREQUEST_PROCESS_EXITTED ExitRequest;
} PROCESSDB_RECORD, *PPROCESSDB_RECORD;



VOID PDBRecordReference(PPROCESSDB_RECORD Record);
VOID PDBRecordDereference(PPROCESSDB_RECORD Record);
NTSTATUS PDBEnumerateToQueue(VOID);

NTSTATUS ProcessDBModuleInit(PDRIVER_OBJECT DriverObject, PVOID Context);
VOID ProcessDBModuleFinit(PDRIVER_OBJECT DriverObject, PVOID Context);



#endif
