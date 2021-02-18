
#ifndef __PROCESS_CONTEXT_TABLE_H__
#define __PROCESS_CONTEXT_TABLE_H__


#include <ntifs.h>
#include <fltKernel.h>
#include "general-types.h"


typedef void (PS_CONTEXT_FREE_ROUTINE)(void *PsContext);

typedef struct _PROCESS_DLL_ENTRY {
	LIST_ENTRY Entry;
	void *ImageBase;
	size_t ImageSize;
	ULONG InageNameLen;
	ULONG Padding;
	size_t FrameCount;
	void *Stack[REQUEST_STACKTRACE_SIZE];
	// Image name
} PROCESS_DLL_ENTRY, *PPROCESS_DLL_ENTRY;

typedef struct _PROCESS_OBJECT_CONTEXT {
	volatile LONG ReferenceCount;
	CLONG DataSize;
	EX_PUSH_LOCK DllListLock;
	LIST_ENTRY DllListHead;
	PS_CONTEXT_FREE_ROUTINE *FreeRoutine;
	// Data
} PROCESS_OBJECT_CONTEXT, *PPROCESS_OBJECT_CONTEXT;

typedef struct _PS_TABLE_ENTRY {
	HANDLE ProcessId;
	PPROCESS_OBJECT_CONTEXT PsContext;
} PS_TABLE_ENTRY, *PPS_TABLE_ENTRY;

typedef struct _PS_CONTEXT_TABLE {
	RTL_GENERIC_TABLE Table;
	KSPIN_LOCK Lock;
	PS_CONTEXT_FREE_ROUTINE *PsCFreeRoutine;
} PS_CONTEXT_TABLE, *PPS_CONTEXT_TABLE;



#define PS_CONTEXT_TO_DATA(aPSC)	(aPSC + 1)


void PsTableInit(PPS_CONTEXT_TABLE Table, PS_CONTEXT_FREE_ROUTINE *FOCFreeRoutine);
void PsTableFinit(PPS_CONTEXT_TABLE Table);
NTSTATUS PsTableInsert(PPS_CONTEXT_TABLE Table, HANDLE ProcessId, const void *Buffer, CLONG Length);
PPROCESS_OBJECT_CONTEXT PsTableDelete(PPS_CONTEXT_TABLE Table, HANDLE ProcessId);
void PsTableDeleteNoReturn(PPS_CONTEXT_TABLE Table, HANDLE ProcessId);
PPROCESS_OBJECT_CONTEXT PsTableGet(PPS_CONTEXT_TABLE Table, HANDLE ProcessId);
NTSTATUS PsTableEnum(PPS_CONTEXT_TABLE Table, PPROCESS_OBJECT_CONTEXT **PsContexts, PULONG Count);
void PsContextDereference(PPROCESS_OBJECT_CONTEXT FoContext);



#endif
