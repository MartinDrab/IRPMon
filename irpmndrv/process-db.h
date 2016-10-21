
#ifndef __PROCESS_DB_H__
#define __PROCESS_DB_H__

#include <ntifs.h>
#include "hash_table.h"



typedef struct _PROCESSDB_RECORD {
	LIST_ENTRY Entry;
	volatile LONG ReferenceCount;
	UNICODE_STRING ImageName;
	UNICODE_STRING CommandLine;
	HANDLE ProcessId;
	HANDLE ParentId;
	HANDLE CreatorId;
} PROCESSDB_RECORD, *PPROCESSDB_RECORD;





#endif
