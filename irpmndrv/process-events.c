
#include <ntifs.h>
#include "general-types.h"
#include "preprocessor.h"
#include "allocator.h"
#include "request.h"
#include "req-queue.h"
#include "process-events.h"



/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

typedef NTSTATUS (PSSETCREATEPROCESSNOTIFYROUTINEEX)(PCREATE_PROCESS_NOTIFY_ROUTINE_EX NotifyRoutine, BOOLEAN Remove);

static PSSETCREATEPROCESSNOTIFYROUTINEEX *_PsSetCreateProcessNotifyROutineEx = NULL;


/************************************************************************/
/*              HELPER FUNCTIONS                                        */
/************************************************************************/


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
	BASIC_CLIENT_INFO clientInfo;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PREQUEST_PROCESS_CREATED createRecord = NULL;
	PREQUEST_PROCESS_EXITTED exitRecord = NULL;
	DEBUG_ENTER_FUNCTION("Process=0x%p; ProcessId=0x%p; CreateInfo=0x%p", Process, ProcessId, CreateInfo);

	QueryClientBasicInformation(&clientInfo);
	if (CreateInfo != NULL) {
		status = _ProcessCreateEventAlloc(ProcessId, CreateInfo, &createRecord);
		if (NT_SUCCESS(status)) {
			_SetRequestFlags(&createRecord->Header, &clientInfo);
			RequestQueueInsert(&createRecord->Header);
			CreateInfo->CreationStatus = STATUS_SUCCESS;
		}
	} else {
		status = _ProcessExittedEventAlloc(ProcessId, &exitRecord);
		if (NT_SUCCESS(status)) {
			_SetRequestFlags(&exitRecord->Header, &clientInfo);
			RequestQueueInsert(&exitRecord->Header);
		}
	}


	DEBUG_EXIT_FUNCTION_VOID();
	return;
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

	vi.dwOSVersionInfoSize = sizeof(vi);
	status = RtlGetVersion(&vi);
	if (NT_SUCCESS(status)) {
		if (vi.dwBuildNumber >= 6001) {
			RtlInitUnicodeString(&uRoutineName, L"PsSetCreateProcessNotifyRoutineEx");
			_PsSetCreateProcessNotifyROutineEx = (PSSETCREATEPROCESSNOTIFYROUTINEEX *)MmGetSystemRoutineAddress(&uRoutineName);
			if (_PsSetCreateProcessNotifyROutineEx != NULL) {
//				loaderEntry = (PKLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
//				loaderEntry->Flags |= 0x20;
				status = _PsSetCreateProcessNotifyROutineEx(_ProcessNotifyEx, FALSE);
//				if (status == STATUS_ACCESS_DENIED) {
//					status = STATUS_SUCCESS;
//					_PsSetCreateProcessNotifyROutineEx = NULL;
//				}
			} else status = STATUS_NOT_FOUND;
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

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
