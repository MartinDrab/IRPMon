
#include <ntifs.h>
#include "allocator.h"
#include "preprocessor.h"
#include "hash_table.h"
#include "process-db.h"



/************************************************************************/
/*                       IMPORTED FUNCTIONS                             */
/************************************************************************/

extern NTSTATUS ZwQuerySystemInformation(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
extern NTSTATUS ZwQueryInformationProcess(HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation, ULONG Length, PULONG ReturnLength);

/************************************************************************/
/*                   TYPE DEFINITIONS                                   */
/************************************************************************/

#define SystemProcessInformation 5

#define PROCESS_TERMINATE 1

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;
	LARGE_INTEGER SpareLi1;
	LARGE_INTEGER SpareLi2;
	LARGE_INTEGER SpareLi3;
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ImageName;
	LONG BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
	ULONG HandleCount;
	ULONG SessionId;
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

/************************************************************************/
/*                   GLOBAL VARIABLES                                   */
/************************************************************************/

static PHASH_TABLE _pdbTable = NULL;
static ERESOURCE _pdbLock;


/************************************************************************/
/*                  HELPER FUNCTIONS                                    */
/************************************************************************/


NTSTATUS _EnumProcesses(PSYSTEM_PROCESS_INFORMATION *Processes)
{
	ULONG retLen = 0;
	ULONG spiSize = 64;
	PSYSTEM_PROCESS_INFORMATION spi = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Processes=0x%p", Processes);

	status = STATUS_INFO_LENGTH_MISMATCH;
	do {
		spi = (PSYSTEM_PROCESS_INFORMATION)HeapMemoryAllocPaged(spiSize);
		if (spi != NULL) {
			status = ZwQuerySystemInformation(SystemProcessInformation, spi, spiSize, &retLen);
			if (status == STATUS_INFO_LENGTH_MISMATCH) {
				HeapMemoryFree(spi);
				spiSize *= 2;
			}
		}
	} while (status == STATUS_INFO_LENGTH_MISMATCH);

	if (NT_SUCCESS(status))
		*Processes = spi;

	DEBUG_EXIT_FUNCTION("0x%x", status, *Processes);
	return status;
}


static VOID _FreeProcessEnumeration(PSYSTEM_PROCESS_INFORMATION Processes)
{
	DEBUG_ENTER_FUNCTION("Processes=0x%p", Processes);

	HeapMemoryFree(Processes);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _GetProcessNameById(HANDLE ProcessId, PUNICODE_STRING Name)
{
	ULONG retLength = 0;
	ULONG pniSize = 512;
	CLIENT_ID ci;
	OBJECT_ATTRIBUTES oa;
	PUNICODE_STRING pni = NULL;
	HANDLE processHandle = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ProcessId=0x%p; Name=0x%p", ProcessId, Name);

	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	ci.UniqueProcess = ProcessId;
	ci.UniqueThread = NULL;
	status = ZwOpenProcess(&processHandle, MAXIMUM_ALLOWED, &oa, &ci);
	if (NT_SUCCESS(status)) {
		do {
			pni = (PUNICODE_STRING)HeapMemoryAllocPaged(pniSize);
			if (pni != NULL) {
				status = ZwQueryInformationProcess(processHandle, 27, pni, pniSize, &retLength);
				if (status == STATUS_INFO_LENGTH_MISMATCH) {
					HeapMemoryFree(pni);
					pniSize *= 2;
				}
			} else status = STATUS_INSUFFICIENT_RESOURCES;
		} while (status == STATUS_INFO_LENGTH_MISMATCH);

		if (NT_SUCCESS(status)) {
			*Name = *pni;
			Name->MaximumLength = Name->Length;
			Name->Buffer = (PWCH)HeapMemoryAllocPaged(Name->Length);
			if (Name->Buffer != NULL)
				memcpy(Name->Buffer, pni->Buffer, Name->Length);
			else status = STATUS_INSUFFICIENT_RESOURCES;
		
			HeapMemoryFree(pni);
		}

		ZwClose(processHandle);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Name=\"%wZ\"", status, Name);
	return status;
}


static ULONG32 _HashFunction(PVOID Key)
{
	return (ULONG32)((ULONG_PTR)Key >> 2);
}

static BOOLEAN _CompareFunction(PHASH_ITEM Item, PVOID Key)
{
	PPROCESSDB_RECORD rec = CONTAINING_RECORD(Item, PROCESSDB_RECORD, Item);

	return (rec->ProcessId == Key);
}


static VOID _FreeFunction(PHASH_ITEM Item)
{
	PPROCESSDB_RECORD rec = CONTAINING_RECORD(Item, PROCESSDB_RECORD, Item);
	DEBUG_ENTER_FUNCTION("rec=0x%p", rec);

	PDBRecordDereference(rec);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


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
		tmpRecord->ReferenceCount = 1;
		tmpRecord->Origin = pdbroProcessNotify;
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


static NTSTATUS _PDBRecordCreateByEnum(PSYSTEM_PROCESS_INFORMATION Info, PPROCESSDB_RECORD *Record)
{
	PPROCESSDB_RECORD tmpRecord = NULL;
	SIZE_T tmpRecordSize = sizeof(PROCESSDB_RECORD);
	UNICODE_STRING uImageName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Info=0x%p; Record=0x%p", Info, Record);

	RtlSecureZeroMemory(&uImageName, sizeof(uImageName));
	status = _GetProcessNameById(Info->UniqueProcessId, &uImageName);
	if (NT_SUCCESS(status)) {
		tmpRecordSize += uImageName.Length;
		tmpRecord = (PPROCESSDB_RECORD)HeapMemoryAllocPaged(tmpRecordSize);
		if (tmpRecord != NULL) {
			tmpRecord->ReferenceCount = 1;
			tmpRecord->Origin = pdbroProcessEnumeration;
			tmpRecord->ProcessId = Info->UniqueProcessId;
			tmpRecord->ParentId = Info->InheritedFromUniqueProcessId;
			tmpRecord->CreatorId = NULL;
			tmpRecord->ImageName.Length = uImageName.Length;
			tmpRecord->ImageName.MaximumLength = tmpRecord->ImageName.Length;
			tmpRecord->ImageName.Buffer = (PWCH)(tmpRecord + 1);
			memcpy(tmpRecord->ImageName.Buffer, uImageName.Buffer, tmpRecord->ImageName.Length);
			RtlSecureZeroMemory(&tmpRecord->CommandLine, sizeof(UNICODE_STRING));
			*Record = tmpRecord;
			status = STATUS_SUCCESS;
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	
		if (uImageName.Length > 0)
			HeapMemoryFree(uImageName.Buffer);
	}

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


VOID _ProcessNotifyEx(HANDLE ParentId, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	PHASH_ITEM h = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PPROCESSDB_RECORD rec = NULL;
	DEBUG_ENTER_FUNCTION("ParentId=0x%p; ProcessId=0x%p; CreateInfo=0x%p", ParentId, ProcessId, CreateInfo);

	UNREFERENCED_PARAMETER(ParentId);

	if (CreateInfo != NULL) {
		status = _PDBRecordCreateByNotify(ProcessId, CreateInfo, &rec);
		if (NT_SUCCESS(status)) {
			PDBRecordReference(rec);
			KeEnterCriticalRegion();
			ExAcquireResourceExclusiveLite(&_pdbLock, TRUE);
			HashTableInsert(_pdbTable, &rec->Item, ProcessId);
			ExReleaseResourceLite(&_pdbLock);
			KeLeaveCriticalRegion();
			PDBRecordDereference(rec);
		}

		CreateInfo->CreationStatus = status;
	} else {
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&_pdbLock, TRUE);
		h = HashTableDelete(_pdbTable, ProcessId);
		ExReleaseResourceLite(&_pdbLock);
		KeLeaveCriticalRegion();
		if (h != NULL) {
			rec = CONTAINING_RECORD(h, PROCESSDB_RECORD, Item);
			PDBRecordDereference(rec);
		}
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                 PUBLIC FUNCTIONS                                     */
/************************************************************************/


VOID PDBRecordReference(PPROCESSDB_RECORD Record)
{
	InterlockedIncrement(&Record->ReferenceCount);

	return;
}


VOID PDBRecordDereference(PPROCESSDB_RECORD Record)
{
	if (InterlockedDecrement(&Record->ReferenceCount) == 0)
		_PDBRecordFree(Record);

	return;
}


/************************************************************************/
/*                INITIALIZATION AND FINALIZATION                       */
/************************************************************************/


NTSTATUS ProcessDBModuleInit(PDRIVER_OBJECT DriverObject, PVOID Context)
{
	PPROCESSDB_RECORD rec = NULL;
	PSYSTEM_PROCESS_INFORMATION tmp = NULL;
	PSYSTEM_PROCESS_INFORMATION spi = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; Context=0x%p", DriverObject, Context);

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Context);

	status = ExInitializeResourceLite(&_pdbLock);
	if (NT_SUCCESS(status)) {
		status = HashTableCreate(httNoSynchronization, 413, _HashFunction, _CompareFunction, _FreeFunction, &_pdbTable);
		if (NT_SUCCESS(status)) {
			status = _EnumProcesses(&spi);
			if (NT_SUCCESS(status)) {
				tmp = spi;
				status = _PDBRecordCreateByEnum(tmp, &rec);
				if (NT_SUCCESS(status)) {
					PDBRecordReference(rec);
					HashTableInsert(_pdbTable, &rec->Item, rec->ProcessId);
					PDBRecordDereference(rec);
					if (tmp->NextEntryOffset > 0) {
						do {
							tmp = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)tmp + tmp->NextEntryOffset);
							status = _PDBRecordCreateByEnum(tmp, &rec);
							if (NT_SUCCESS(status)) {
								PDBRecordReference(rec);
								HashTableInsert(_pdbTable, &rec->Item, rec->ProcessId);
								PDBRecordDereference(rec);
							}
						} while (NT_SUCCESS(status) && tmp->NextEntryOffset > 0);
					}

					if (NT_SUCCESS(status)) {
						status = PsSetCreateProcessNotifyRoutineEx(_ProcessNotifyEx, FALSE);
						if (!NT_SUCCESS(status)) {

						}
					}
				}

				_FreeProcessEnumeration(spi);
			}
		
			if (!NT_SUCCESS(status))
				HashTableDestroy(_pdbTable);
		}

		if (!NT_SUCCESS(status))
			ExDeleteResourceLite(&_pdbLock);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID ProcessDBModuleFinit(PDRIVER_OBJECT DriverObject, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; Context=0x%p", DriverObject, Context);

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Context);

	PsSetCreateProcessNotifyRoutineEx(_ProcessNotifyEx, TRUE);
	HashTableDestroy(_pdbTable);
	ExDeleteResourceLite(&_pdbLock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
