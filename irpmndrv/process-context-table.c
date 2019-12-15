
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "process-context-table.h"



/************************************************************************/
/*                HELPER FUNCTIONS                                      */
/************************************************************************/


static RTL_GENERIC_COMPARE_RESULTS _PsCompareRoutine(__in struct _RTL_GENERIC_TABLE *Table, __in PVOID FirstStruct, __in PVOID SecondStruct)
{
	RTL_GENERIC_COMPARE_RESULTS ret = GenericEqual;
	const PS_TABLE_ENTRY *a = (PPS_TABLE_ENTRY)FirstStruct;
	const PS_TABLE_ENTRY *b = (PPS_TABLE_ENTRY)SecondStruct;
	DEBUG_ENTER_FUNCTION("Table=0x%p; FirstStruct=0x%p; SecondScrut=0x%p", Table, FirstStruct, SecondStruct);

	if ((uintptr_t)a->ProcessId > (uintptr_t)b->ProcessId)
		ret = GenericGreaterThan;
	else if ((uintptr_t)a->ProcessId < (uintptr_t)b->ProcessId)
		ret = GenericLessThan;

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


static PVOID _PsAllocRoutine(__in struct _RTL_GENERIC_TABLE  *Table, __in CLONG  ByteSize)
{
	PVOID ret = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; ByteSize=%u", Table, ByteSize);

	ret = HeapMemoryAllocNonPaged(ByteSize);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


static VOID _PsFreeRoutine(__in struct _RTL_GENERIC_TABLE *Table, __in PVOID  Buffer)
{
	DEBUG_ENTER_FUNCTION("Table=0x%p; Buffer=0x%p", Table, Buffer);

	HeapMemoryFree(Buffer);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                   PUBLIC FUNCTIONS                                   */
/************************************************************************/


void PsTableInit(PPS_CONTEXT_TABLE Table, PS_CONTEXT_FREE_ROUTINE *PsCFreeRoutine)
{
	DEBUG_ENTER_FUNCTION("Table=0x%p; PsCFreeRoutine=0x%p", Table, PsCFreeRoutine);

	RtlInitializeGenericTable(&Table->Table, _PsCompareRoutine, _PsAllocRoutine, _PsFreeRoutine, NULL);
	Table->PsCFreeRoutine = PsCFreeRoutine;
	KeInitializeSpinLock(&Table->Lock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


void PsTableFinit(PPS_CONTEXT_TABLE Table)
{
	PPS_TABLE_ENTRY entry = NULL;
	PPROCESS_OBJECT_CONTEXT psc = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

	while (!RtlIsGenericTableEmpty(&Table->Table)) {
		entry = RtlGetElementGenericTable(&Table->Table, 0);
		psc = entry->PsContext;
		RtlDeleteElementGenericTable(&Table->Table, entry);
		PsContextDereference(psc);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS PsTableInsert(PPS_CONTEXT_TABLE Table, HANDLE ProcessId, const void *Buffer, CLONG Length)
{
	KIRQL irql;
	BOOLEAN inserted = FALSE;
	PPROCESS_OBJECT_CONTEXT psc = NULL;
	PS_TABLE_ENTRY entry;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; ProcessId=0x%p; Buffer=0x%p; Length=%u", Table, ProcessId, Buffer, Length);

	if (ProcessId != NULL) {
		psc = HeapMemoryAllocNonPaged(sizeof(PROCESS_OBJECT_CONTEXT) + Length);
		if (psc != NULL) {
			memset(psc, 0, sizeof(PROCESS_OBJECT_CONTEXT) + Length);
			InterlockedExchange(&psc->ReferenceCount, 1);
			psc->FreeRoutine = Table->PsCFreeRoutine;
			psc->DataSize = Length;
			memcpy(psc + 1, Buffer, Length);
			entry.ProcessId = ProcessId;
			entry.PsContext = psc;
			KeAcquireSpinLock(&Table->Lock, &irql);
			RtlInsertElementGenericTable(&Table->Table, &entry, sizeof(entry), &inserted);
			status = (inserted) ? STATUS_SUCCESS : STATUS_ALREADY_REGISTERED;
			if (NT_SUCCESS(status))
				InterlockedIncrement(&psc->ReferenceCount);

			KeReleaseSpinLock(&Table->Lock, irql);
			PsContextDereference(psc);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else status = STATUS_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


PPROCESS_OBJECT_CONTEXT PsTableDelete(PPS_CONTEXT_TABLE Table, HANDLE ProcessId)
{
	KIRQL irql;
	PS_TABLE_ENTRY entry;
	PPS_TABLE_ENTRY entry2 = NULL;
	PPROCESS_OBJECT_CONTEXT ret = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; ProcessId=0x%p", Table, ProcessId);

	entry.ProcessId = ProcessId;
	KeAcquireSpinLock(&Table->Lock, &irql);
	entry2 = RtlLookupElementGenericTable(&Table->Table, &entry);
	if (entry2 != NULL) {
		ret = entry2->PsContext;
		if (!RtlDeleteElementGenericTable(&Table->Table, &entry))
			__debugbreak();
	}

	KeReleaseSpinLock(&Table->Lock, irql);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


void PsTableDeleteNoReturn(PPS_CONTEXT_TABLE Table, HANDLE ProcessId)
{
	PPROCESS_OBJECT_CONTEXT psc = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; ProcessId=0x%p", Table, ProcessId);

	psc = PsTableDelete(Table, ProcessId);
	if (psc != NULL)
		PsContextDereference(psc);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


PPROCESS_OBJECT_CONTEXT PsTableGet(PPS_CONTEXT_TABLE Table, HANDLE ProcessId)
{
	KIRQL irql;
	PS_TABLE_ENTRY entry;
	PPS_TABLE_ENTRY entry2 = NULL;
	PPROCESS_OBJECT_CONTEXT ret = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; ProcessId=0x%p", Table, ProcessId);

	entry.ProcessId = ProcessId;
	KeAcquireSpinLock(&Table->Lock, &irql);
	entry2 = RtlLookupElementGenericTable(&Table->Table, &entry);
	if (entry2 != NULL) {
		ret = entry2->PsContext;
		InterlockedIncrement(&ret->ReferenceCount);
	}

	KeReleaseSpinLock(&Table->Lock, irql);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


NTSTATUS PsTableEnum(PPS_CONTEXT_TABLE Table, PPROCESS_OBJECT_CONTEXT **PsContexts, PULONG Count)
{
	KIRQL irql;
	ULONG elemCount = 0;
	PPS_TABLE_ENTRY entry = NULL;
	PPROCESS_OBJECT_CONTEXT psc = NULL;
	PPROCESS_OBJECT_CONTEXT *tmpContexts = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; PsContexts=0x%p; Count=0x%p", Table, PsContexts, Count);

	*Count = 0;
	*PsContexts = NULL;
	status = STATUS_SUCCESS;
	KeAcquireSpinLock(&Table->Lock, &irql);
	if (!RtlIsGenericTableEmpty(&Table->Table)) {
		elemCount = RtlNumberGenericTableElements(&Table->Table);
		tmpContexts = HeapMemoryAllocNonPaged(elemCount*sizeof(PPROCESS_OBJECT_CONTEXT));
		if (tmpContexts != NULL) {
			for (ULONG i = 0; i < elemCount; ++i) {
				entry = RtlGetElementGenericTable(&Table->Table, i);
				psc = entry->PsContext;
				InterlockedIncrement(&psc->ReferenceCount);
				tmpContexts[i] = psc;
			}

			*PsContexts = tmpContexts;
			*Count = elemCount;
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	KeReleaseSpinLock(&Table->Lock, irql);

	DEBUG_EXIT_FUNCTION("0x%x, *PsContects=0x%p, *Count=%u", status, *PsContexts, *Count);
	return status;
}


void PsContextDereference(PPROCESS_OBJECT_CONTEXT PsContext)
{
	DEBUG_ENTER_FUNCTION("PsContext=0x%p", PsContext);

	if (InterlockedDecrement(&PsContext->ReferenceCount) == 0) {
		if (PsContext->FreeRoutine != NULL)
			PsContext->FreeRoutine(PS_CONTEXT_TO_DATA(PsContext));

		HeapMemoryFree(PsContext);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
