
#include <ntifs.h>
#include "allocator.h"
#include "preprocessor.h"
#include "hash_table.h"
#include "process-db.h"
#include "string-hash-table.h"


/************************************************************************/
/*                   GLOBAL VARIABLES                                   */
/************************************************************************/

static LIST_ENTRY _pdbList;
static PHASH_TABLE _pdbTable = NULL;
static ERESOURCE _pdbLock;


/************************************************************************/
/*                  HELPER FUNCTIONS                                    */
/************************************************************************/

static NTSTATUS _PDBRecordCreateByNotify(HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO NotifyInfo, PPROCESSDB_RECORD *Record)
{
	SIZE_T tmpRecordSize = sizeof(PROCESSDB_RECORD);
	PPROCESSDB_RECORD tmpRecord = NULL;
	UNICODE_STRING uImageName;
	UNICODE_STRING uCommandLine;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ProcessId=0x%p; NotifyInfo=0x%p; Record=0x%p", ProcessId, NotifyInfo, Record);

	RtlSecureZeroMemory(&uImageName, sizeof(uImageName));
	if (NotifyInfo->ImageFileName != NULL) {
		uImageName = *NotifyInfo->ImageFileName;
		tmpRecordSize += uImageName.Length;
	}

	RtlSecureZeroMemory(&uCommandLine, sizeof(uCommandLine));
	if (NotifyInfo->CommandLine) {
		uCommandLine = *NotifyInfo->CommandLine;
		tmpRecordSize += uCommandLine.Length;
	}

	tmpRecord = (PPROCESSDB_RECORD)HeapMemoryAllocPaged(tmpRecordSize);
	if (tmpRecord != NULL) {
		InitializeListHead(&tmpRecord->Entry);
		tmpRecord->ReferenceCount = 1;
		tmpRecord->CreatorId = PsGetCurrentProcessId();
		tmpRecord->ParentId = NotifyInfo->ParentProcessId;
		tmpRecord->ProcessId = ProcessId;
		tmpRecord->ImageName.Length = uImageName.Length;
		tmpRecord->ImageName.MaximumLength = tmpRecord->ImageName.Length;
		tmpRecord->ImageName.Buffer = (PWCH)(tmpRecord + 1);
		memcpy(tmpRecord->ImageName.Buffer, uImageName.Buffer, tmpRecord->ImageName.Length);
		tmpRecord->CommandLine.Length = uCommandLine.Length;
		tmpRecord->CommandLine.MaximumLength = tmpRecord->CommandLine.Length;
		tmpRecord->CommandLine.Buffer = tmpRecord->ImageName.Buffer + tmpRecord->ImageName.Length / sizeof(WCHAR);
		memcpy(tmpRecord->CommandLine.Buffer, uCommandLine.Buffer, tmpRecord->CommandLine.Length);
		*Record = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}


NTSTATUS _PDBRecordCreateById(HANDLE ProcessId, PPROCESSDB_RECORD *Record)
{
	PPROCESSDB_RECORD tmpRecord = NULL;
	SIZE_T tmpRecordSize = sizeof(PROCESSDB_RECORD);
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ProcessId=0x%p; Record=0x%p", ProcessId, Record);

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}



static VOID _PDBRecordFree(PPROCESSDB_RECORD Record)
{
	DEBUG_EXIT_FUNCTION("Record=0x%p", Record);

	HeapMemoryFree(Record);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}




/************************************************************************/
/*                 PUBLIC FUNCTIONS                                     */
/************************************************************************/