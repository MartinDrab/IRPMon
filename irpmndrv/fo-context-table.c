
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "fo-context-table.h"



/************************************************************************/
/*                HELPER FUNCTIONS                                      */
/************************************************************************/


static RTL_GENERIC_COMPARE_RESULTS _FoCompareRoutine(__in struct _RTL_GENERIC_TABLE *Table, __in PVOID FirstStruct, __in PVOID SecondStruct)
{
	RTL_GENERIC_COMPARE_RESULTS ret = GenericEqual;
	const FO_TABLE_ENTRY *a = (PFO_TABLE_ENTRY)FirstStruct;
	const FO_TABLE_ENTRY *b = (PFO_TABLE_ENTRY)SecondStruct;
	DEBUG_ENTER_FUNCTION("Table=0x%p; FirstStruct=0x%p; SecondScrut=0x%p", Table,FirstStruct, SecondStruct);

	if ((uintptr_t)a->FileObject > (uintptr_t)b->FileObject)
		ret = GenericGreaterThan;
	else if ((uintptr_t)a->FileObject > (uintptr_t)b->FileObject)
		ret = GenericLessThan;

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


static PVOID _FoAllocRoutine(__in struct _RTL_GENERIC_TABLE  *Table, __in CLONG  ByteSize)
{
	PVOID ret = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; ByteSize=%u", Table,ByteSize);

	ret = HeapMemoryAllocNonPaged(ByteSize);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


static VOID _FoFreeRoutine(__in struct _RTL_GENERIC_TABLE *Table, __in PVOID  Buffer)
{
	DEBUG_ENTER_FUNCTION("Table=0x%p; Buffer=0x%p", Table, Buffer);

	HeapMemoryFree(Buffer);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                   PUBLIC FUNCTIONS                                   */
/************************************************************************/


void FoTableInit(PFO_CONTEXT_TABLE Table, FO_CONTEXT_FREE_ROUTINE *FOCFreeRoutine)
{
	DEBUG_ENTER_FUNCTION("Table=0x%p; FOCFreeRoutine=0x%p", Table, FOCFreeRoutine);

	RtlInitializeGenericTable(&Table->Table, _FoCompareRoutine, _FoAllocRoutine, _FoFreeRoutine, NULL);
	Table->FOCFreeRoutine = FOCFreeRoutine;
	KeInitializeSpinLock(&Table->Lock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


void FoTableFinit(PFO_CONTEXT_TABLE Table)
{
	PFILE_OBJECT_CONTEXT foc = NULL;
	PFO_TABLE_ENTRY entry = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

	while (!RtlIsGenericTableEmpty(&Table->Table)) {
		entry = RtlGetElementGenericTable(&Table->Table, 0);
		foc = entry->FoContext;
		RtlDeleteElementGenericTable(&Table->Table, entry);
		FoContextDereference(foc);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS FoTableInsert(PFO_CONTEXT_TABLE Table, PFILE_OBJECT FileObject, const void *Buffer, CLONG Length)
{
	KIRQL irql;
	BOOLEAN inserted = FALSE;
	PFILE_OBJECT_CONTEXT foc = NULL;
	FO_TABLE_ENTRY entry;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; FileObject=0x%p; Buffer=0x%p; Length=%u", Table, FileObject, Buffer, Length);

	if (FileObject != NULL) {
		foc = HeapMemoryAllocNonPaged(sizeof(FILE_OBJECT_CONTEXT) + Length);
		if (foc != NULL) {
			InterlockedExchange(&foc->ReferenceCount, 1);
			foc->FreeRoutine = Table->FOCFreeRoutine;
			foc->DataSize = Length;
			memcpy(foc + 1, Buffer, Length);
			entry.FileObject = FileObject;
			entry.FoContext = foc;
			KeAcquireSpinLock(&Table->Lock, &irql);
			RtlInsertElementGenericTable(&Table->Table, &entry, sizeof(entry), &inserted);
			KeReleaseSpinLock(&Table->Lock, irql);
			status = (inserted) ? STATUS_SUCCESS : STATUS_ALREADY_REGISTERED;
			if (NT_SUCCESS(status))
				InterlockedIncrement(&foc->ReferenceCount);

			FoContextDereference(foc);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else status = STATUS_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


PFILE_OBJECT_CONTEXT FoTableDelete(PFO_CONTEXT_TABLE Table, PFILE_OBJECT FileObject)
{
	KIRQL irql;
	FO_TABLE_ENTRY entry;
	PFO_TABLE_ENTRY entry2 = NULL;
	PFILE_OBJECT_CONTEXT ret = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; FileObject=0x%p", Table, FileObject);

	entry.FileObject = FileObject;
	KeAcquireSpinLock(&Table->Lock, &irql);
	entry2 = RtlLookupElementGenericTable(&Table->Table, &entry);
	if (entry2 != NULL) {
		ret = entry2->FoContext;
		RtlDeleteElementGenericTable(&Table->Table, &entry);
	}

	KeReleaseSpinLock(&Table->Lock, irql);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


void FoTableDeleteNoReturn(PFO_CONTEXT_TABLE Table, PFILE_OBJECT FileObject)
{
	PFILE_OBJECT_CONTEXT foc = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; FileObject=0x%p", Table, FileObject);

	foc = FoTableDelete(Table, FileObject);
	if (foc != NULL)
		FoContextDereference(foc);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


PFILE_OBJECT_CONTEXT FoTableGet(PFO_CONTEXT_TABLE Table, PFILE_OBJECT FileObject)
{
	KIRQL irql;
	FO_TABLE_ENTRY entry;
	PFO_TABLE_ENTRY entry2 = NULL;
	PFILE_OBJECT_CONTEXT ret = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; FileObject=0x%p", Table, FileObject);

	entry.FileObject = FileObject;
	KeAcquireSpinLock(&Table->Lock, &irql);
	entry2 = RtlLookupElementGenericTable(&Table->Table, &entry);
	if (entry2 != NULL) {
		ret = entry2->FoContext;
		InterlockedIncrement(&ret->ReferenceCount);
	}

	KeReleaseSpinLock(&Table->Lock, irql);

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


void FoContextDereference(PFILE_OBJECT_CONTEXT FoContext)
{
	DEBUG_ENTER_FUNCTION("", FoContext);

	if (InterlockedDecrement(&FoContext->ReferenceCount) == 0) {
		if (FoContext->FreeRoutine != NULL)
			FoContext->FreeRoutine(FO_CONTEXT_TO_DATA(FoContext));

		HeapMemoryFree(FoContext);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
