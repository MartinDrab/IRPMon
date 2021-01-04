
#ifndef __FO_CONTEXT_TABLE_H__
#define __FO_CONTEXT_TABLE_H__


#include <ntifs.h>


typedef void (FO_CONTEXT_FREE_ROUTINE)(void *FoContext);

typedef struct _FILE_OBJECT_CONTEXT {
	volatile LONG ReferenceCount;
	CLONG DataSize;
	FO_CONTEXT_FREE_ROUTINE *FreeRoutine;
	UNICODE_STRING FileName;
	// Data
} FILE_OBJECT_CONTEXT, *PFILE_OBJECT_CONTEXT;

typedef struct _FO_TABLE_ENTRY {
	PFILE_OBJECT FileObject;
	PFILE_OBJECT_CONTEXT FoContext;
} FO_TABLE_ENTRY, *PFO_TABLE_ENTRY;

typedef struct _FO_CONTEXT_TABLE  {
	RTL_GENERIC_TABLE Table;
	KSPIN_LOCK Lock;
	FO_CONTEXT_FREE_ROUTINE *FOCFreeRoutine;
} FO_CONTEXT_TABLE, *PFO_CONTEXT_TABLE;



#define FO_CONTEXT_TO_DATA(aFOC)	(aFOC + 1)


void FoTableInit(PFO_CONTEXT_TABLE Table, FO_CONTEXT_FREE_ROUTINE *FOCFreeRoutine);
void FoTableFinit(PFO_CONTEXT_TABLE Table);
NTSTATUS FoTableInsert(PFO_CONTEXT_TABLE Table, PFILE_OBJECT FileObject, const void *Buffer, CLONG Length);
PFILE_OBJECT_CONTEXT FoTableDelete(PFO_CONTEXT_TABLE Table, PFILE_OBJECT FileObject);
void FoTableDeleteNoReturn(PFO_CONTEXT_TABLE Table, PFILE_OBJECT FileObject);
PFILE_OBJECT_CONTEXT FoTableGet(PFO_CONTEXT_TABLE Table, PFILE_OBJECT FileObject);
void FoContextDereference(PFILE_OBJECT_CONTEXT FoContext);
NTSTATUS FoContextSetFileName(PFILE_OBJECT_CONTEXT FsContext, const UNICODE_STRING *FileName);


#endif
