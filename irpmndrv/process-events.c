
#include <ntifs.h>
#include "general-types.h"
#include "preprocessor.h"
#include "allocator.h"
#include "request.h"
#include "req-queue.h"
#include "process-context-table.h"
#include "process-events.h"



/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/


typedef struct _PROCESS_RECORD {
	HANDLE ProcessId;
	HANDLE ParentId;
	LARGE_INTEGER CreateTime;
	UNICODE_STRING ImageName;
	UNICODE_STRING CommandLine;
} PROCESS_RECORD, *PPROCESS_RECORD;


typedef NTSTATUS (PSSETCREATEPROCESSNOTIFYROUTINEEX)(PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine, BOOLEAN Remove);

static PSSETCREATEPROCESSNOTIFYROUTINEEX *_PsSetCreateProcessNotifyROutineEx = NULL;
static PS_CONTEXT_TABLE _processTable;


/************************************************************************/
/*              HELPER FUNCTIONS                                        */
/************************************************************************/


static void _PsRecordFree(void *PsContext)
{
	PPROCESS_RECORD pr = (PPROCESS_RECORD)PsContext;
	DEBUG_ENTER_FUNCTION("PsContext=0x%p", PsContext);

	if (pr->ImageName.Buffer != NULL)
		HeapMemoryFree(pr->ImageName.Buffer);

	if (pr->CommandLine.Buffer != NULL)
		HeapMemoryFree(pr->CommandLine.Buffer);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _ProcessCreateEventAlloc(HANDLE ProcessId, const PS_CREATE_NOTIFY_INFO *NotifyInfo, PREQUEST_PROCESS_CREATED *Record)
{
	size_t cmdLineOffset = 0;
	size_t imageNameOffset = 0;
	size_t cmdLineSize = 0;
	size_t imageNameSize = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_PROCESS_CREATED tmpRecord = NULL;
	DEBUG_ENTER_FUNCTION("ProcessId=0x%p; NotifyInfo=0x%p; Record=0x%p", ProcessId, NotifyInfo, Record);

	if (NotifyInfo->ImageFileName != NULL)
		imageNameSize = NotifyInfo->ImageFileName->Length;

	if (NotifyInfo->CommandLine != NULL)
		cmdLineSize = NotifyInfo->CommandLine->Length;

	tmpRecord = HeapMemoryAllocNonPaged(sizeof(REQUEST_PROCESS_CREATED) + imageNameSize + cmdLineSize);
	if (tmpRecord != NULL) {
		RequestHeaderInit(&tmpRecord->Header, NULL, NULL, ertProcessCreated);
		tmpRecord->ProcessId = ProcessId;
		tmpRecord->ParentId = NotifyInfo->ParentProcessId;
		tmpRecord->CreatorId = PsGetCurrentProcessId();
		tmpRecord->ImageNameLength = (ULONG)imageNameSize;
		imageNameOffset = sizeof(REQUEST_PROCESS_CREATED);
		tmpRecord->CommandLineLength = (ULONG)cmdLineSize;
		cmdLineOffset = imageNameOffset + tmpRecord->ImageNameLength;
		if (imageNameSize > 0)
			memcpy((unsigned char *)tmpRecord + imageNameOffset, NotifyInfo->ImageFileName->Buffer, tmpRecord->ImageNameLength);
		
		if (cmdLineSize > 0)
			memcpy((unsigned char *)tmpRecord + cmdLineOffset, NotifyInfo->CommandLine->Buffer, tmpRecord->CommandLineLength);
		
		*Record = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}


static NTSTATUS _ProcessExittedEventAlloc(HANDLE ProcessId, PREQUEST_PROCESS_EXITTED *Record)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_PROCESS_EXITTED tmpRecord = NULL;
	DEBUG_ENTER_FUNCTION("ProcessId=0x%p; Record=0x%p", ProcessId, Record);

	tmpRecord = HeapMemoryAllocNonPaged(sizeof(REQUEST_PROCESS_EXITTED));
	if (tmpRecord != NULL) {
		RequestHeaderInit(&tmpRecord->Header, NULL, NULL, ertProcessExitted);
		tmpRecord->ProcessId = ProcessId;
		*Record = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}


static void _ProcessNotifyEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	PPROCESS_RECORD pr = NULL;
	BASIC_CLIENT_INFO clientInfo;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_PROCESS_CREATED createRecord = NULL;
	PREQUEST_PROCESS_EXITTED exitRecord = NULL;
	DEBUG_ENTER_FUNCTION("Process=0x%p; ProcessId=0x%p; CreateInfo=0x%p", Process, ProcessId, CreateInfo);

	status = STATUS_SUCCESS;
	QueryClientBasicInformation(&clientInfo);
	if (CreateInfo != NULL) {
		status = _ProcessCreateEventAlloc(ProcessId, CreateInfo, &createRecord);
		if (NT_SUCCESS(status)) {
			_SetRequestFlags(&createRecord->Header, &clientInfo);
			RequestQueueInsert(&createRecord->Header);
			pr = HeapMemoryAllocNonPaged(sizeof(PROCESS_RECORD));
			if (pr != NULL) {
				memset(pr, 0, sizeof(PROCESS_RECORD));
				pr->ParentId = CreateInfo->ParentProcessId;
				pr->ProcessId = ProcessId;
				pr->CreateTime.QuadPart = PsGetProcessCreateTimeQuadPart(Process);
				if (CreateInfo->ImageFileName != NULL)
					status = UtilsCopyUnicodeString(NonPagedPool, &pr->ImageName, CreateInfo->ImageFileName);

				if (NT_SUCCESS(status) && CreateInfo->CommandLine != NULL)
					status = UtilsCopyUnicodeString(NonPagedPool, &pr->CommandLine, CreateInfo->CommandLine);

				if (!NT_SUCCESS(status) && CreateInfo->ImageFileName != NULL)
					HeapMemoryFree(pr->ImageName.Buffer);

				if (NT_SUCCESS(status))
					status = PsTableInsert(&_processTable, pr->ProcessId, pr, sizeof(PROCESS_RECORD));
								
				HeapMemoryFree(pr);
			} else status = STATUS_INSUFFICIENT_RESOURCES;
		}
	} else {
		status = _ProcessExittedEventAlloc(ProcessId, &exitRecord);
		if (NT_SUCCESS(status)) {
			_SetRequestFlags(&exitRecord->Header, &clientInfo);
			RequestQueueInsert(&exitRecord->Header);
		}

		PsTableDeleteNoReturn(&_processTable, ProcessId);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static NTSTATUS _FillProcessTable(void)
{
	PROCESS_RECORD pr;
	CLIENT_ID clientId;
	OBJECT_ATTRIBUTES oa;
	HANDLE hProcess = NULL;
	PSYSTEM_PROCESS_INFORMATION_REAL tmp = NULL;
	PSYSTEM_PROCESS_INFORMATION_REAL spir = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	status = ProcessEnumerate(&spir);
	if (NT_SUCCESS(status)) {
		tmp = spir;
		do {
			if (tmp->UniqueProcessId != NULL) {
				memset(&pr, 0, sizeof(pr));
				pr.CreateTime = tmp->CreateTime;
				pr.ProcessId = tmp->UniqueProcessId;
				pr.ParentId = tmp->InheritedFromUniqueProcessId;
				memset(&clientId, 0, sizeof(clientId));
				clientId.UniqueProcess = pr.ProcessId;
				InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
				status = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &oa, &clientId);
				if (NT_SUCCESS(status)) {
					status = ProcessQueryFullImageName(hProcess, NonPagedPool, &pr.ImageName);
					if (NT_SUCCESS(status)) {
						if (_PsSetCreateProcessNotifyROutineEx != NULL)
							status = ProcessQueryCommandLine(hProcess, NonPagedPool, &pr.CommandLine);

						if (NT_SUCCESS(status)) {
							status = PsTableInsert(&_processTable, pr.ProcessId, &pr, sizeof(pr));
							memset(&pr, 0, sizeof(pr));
							if (!NT_SUCCESS(status) && pr.CommandLine.Buffer != NULL)
								HeapMemoryFree(pr.CommandLine.Buffer);
						}

						if (!NT_SUCCESS(status) && pr.ImageName.Buffer != NULL)
							HeapMemoryFree(pr.ImageName.Buffer);
					}

					ZwClose(hProcess);
				} else status = STATUS_SUCCESS;
			}

			if (tmp->NextEntryOffset == 0)
				break;

			tmp = (PSYSTEM_PROCESS_INFORMATION_REAL)((unsigned char *)tmp + tmp->NextEntryOffset);
		} while (TRUE);

		ProcessEnumerationFree(spir);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/************************************************************************/
/*                     PUBLIC FUNCTIONS                                 */
/************************************************************************/


NTSTATUS ListProcessesByEvents(PLIST_ENTRY EventListHead)
{
	ULONG count = 0;
	PS_CREATE_NOTIFY_INFO info;
	PPROCESS_RECORD record = NULL;
	PPROCESS_OBJECT_CONTEXT psc = NULL;
	PREQUEST_PROCESS_CREATED request = NULL;
	PPROCESS_OBJECT_CONTEXT *contexts = NULL;
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
	DEBUG_ENTER_FUNCTION("EventListHead=0x%p", EventListHead);

	status = PsTableEnum(&_processTable, &contexts, &count);
	if (NT_SUCCESS(status)) {
		if (count > 0) {
			for (ULONG i = 0; i < count; ++i) {
				psc = contexts[i];
				if (NT_SUCCESS(status)) {
					record = (PPROCESS_RECORD)PS_CONTEXT_TO_DATA(psc);
					memset(&info, 0, sizeof(info));
					info.Size = sizeof(info);
					info.CommandLine = &record->CommandLine;
					info.ImageFileName = &record->ImageName;
					info.ParentProcessId = record->ParentId;
					status = _ProcessCreateEventAlloc(record->ProcessId, &info, &request);
					if (NT_SUCCESS(status)) {
						request->Header.Flags |= REQUEST_FLAG_EMULATED;
						InsertTailList(EventListHead, &request->Header.Entry);
					}
				}

				PsContextDereference(psc);
			}

			HeapMemoryFree(contexts);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


/************************************************************************/
/*                   INITIALIZATION AND FINALIZATION                    */
/************************************************************************/


NTSTATUS ProcessEventsModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
//	PKLDR_DATA_TABLE_ENTRY loaderEntry = NULL;
	UNICODE_STRING uRoutineName;
	OSVERSIONINFOW vi;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	PsTableInit(&_processTable, _PsRecordFree);
	vi.dwOSVersionInfoSize = sizeof(vi);
	status = RtlGetVersion(&vi);
	if (NT_SUCCESS(status)) {
		status = _FillProcessTable();
		if (NT_SUCCESS(status)) {
			if (vi.dwBuildNumber >= 6001) {
				RtlInitUnicodeString(&uRoutineName, L"PsSetCreateProcessNotifyRoutineEx");
				_PsSetCreateProcessNotifyROutineEx = (PSSETCREATEPROCESSNOTIFYROUTINEEX *)MmGetSystemRoutineAddress(&uRoutineName);
				if (_PsSetCreateProcessNotifyROutineEx != NULL)
					status = _PsSetCreateProcessNotifyROutineEx(_ProcessNotifyEx, FALSE);
				else status = STATUS_NOT_FOUND;
			}
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID ProcessEventsModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"; Context=0x%p", DriverObject, RegistryPath, Context);

	if (_PsSetCreateProcessNotifyROutineEx != NULL)
		_PsSetCreateProcessNotifyROutineEx(_ProcessNotifyEx, TRUE);

	PsTableFinit(&_processTable);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
