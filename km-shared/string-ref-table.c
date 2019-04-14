
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "string-ref-table.h"



/************************************************************************/
/*                 HELPER FUNCTIONS                                     */
/************************************************************************/

static BOOLEAN _KeysEqual(PSTRING_REF_KEY K1, PSTRING_REF_KEY K2)
{
	return (K1->Hash == K2->Hash || RtlEqualUnicodeString(&K1->String, &K2->String, TRUE));
}


static PSTRING_REF_ITEM _ChainWalk(_In_ PSTRING_REF_ITEM *ChainStart, _In_ PSTRING_REF_KEY Key, _In_ BOOLEAN Delete)
{
	PSTRING_REF_ITEM prev = NULL;
	PSTRING_REF_ITEM current = NULL;
	PSTRING_REF_ITEM ret = NULL;
//	DEBUG_ENTER_FUNCTION("ChainStart=0x%p; Delete=%u", ChainStart, Delete);

	current = *ChainStart;
	while (current != NULL) {
		if (_KeysEqual(&current->Key, Key)) {
			ret = current;
			if (Delete) {
				if (prev != NULL)
					prev->Next = current->Next;
				else *ChainStart = current->Next;
			}
		}

		prev = current;
		current = current->Next;
	}

//	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


static VOID _NOPRoutine(_Inout_ PVOID Object)
{
	UNREFERENCED_PARAMETER(Object);

	return;
}


/************************************************************************/
/*                   PUBLIC FUNCTIONS                                   */
/************************************************************************/

NTSTATUS StringRefTableInit(_In_ SIZE_T ObjectKeyOffset, _In_ SIZE_T BucketCount, _In_opt_ STRING_REF_REFERENCE *Reference, _In_opt_ STRING_REF_DEREFERENCE *Dereference, _Out_ PSTRING_REF_TABLE Table)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ObjectKeyOffset=%Iu; BucketCount=%Iu; Reference=0x%p; Dereference=0x%p; Table=0x%p", ObjectKeyOffset, BucketCount, Reference, Dereference, Table);

	Table->ObjectKeyOffset = ObjectKeyOffset;
	Table->BucketCount = BucketCount;
	Table->Reference = (Reference != NULL) ? Reference : _NOPRoutine;
	Table->Dereference = (Dereference != NULL) ? Dereference : _NOPRoutine;
	Table->Buckets = (PSTRING_REF_ITEM *)HeapMemoryAllocPaged(sizeof(PSTRING_REF_ITEM)*Table->BucketCount);
	if (Table->Buckets != NULL) {
		RtlSecureZeroMemory(Table->Buckets, sizeof(PSTRING_REF_ITEM)*Table->BucketCount);
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%p", status);
	return status;
}


VOID StringRefTableDestroy(_In_ PSTRING_REF_TABLE Table)
{
	DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

	StringRefTableClear(Table);
	HeapMemoryFree(Table->Buckets);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


VOID StringRefTableClear(_In_ PSTRING_REF_TABLE Table)
{
	PSTRING_REF_ITEM old = NULL;
	PSTRING_REF_ITEM item = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

	for (SIZE_T i = 0; i < Table->BucketCount; ++i) {
		item = Table->Buckets[i];
		while (item != NULL) {
			old = item;
			item = item->Next;
			Table->Dereference((PUCHAR)old - Table->ObjectKeyOffset);
		}
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


PVOID StringRefTableInsert(_In_ PSTRING_REF_TABLE Table, _In_ PVOID Object)
{
	PSTRING_REF_ITEM item = NULL;
	PSTRING_REF_ITEM *pBucket = NULL;
	PVOID ret = Object;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
//	DEBUG_ENTER_FUNCTION("Table=0x%p; Object=0x%p", Table, Object);

	item = (PSTRING_REF_ITEM)((PUCHAR)Object + Table->ObjectKeyOffset);
	status = RtlHashUnicodeString(&item->Key.String, TRUE, HASH_STRING_ALGORITHM_DEFAULT, &item->Key.Hash);
	if (NT_SUCCESS(status)) {
		pBucket = Table->Buckets + item->Key.Hash % Table->BucketCount;
		item = _ChainWalk(pBucket, &item->Key, FALSE);
		if (item == NULL) {
			item = (PSTRING_REF_ITEM)((PUCHAR)Object + Table->ObjectKeyOffset);
			item->Next = InterlockedExchangePointer(pBucket, item);
		} else ret = (PUCHAR)item - Table->ObjectKeyOffset;

		Table->Reference(ret);
	}

//	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


PVOID StringRefTableDelete(_In_ PSTRING_REF_TABLE Table, _In_ PUNICODE_STRING String)
{
	PSTRING_REF_ITEM *pBucket = NULL;
	PSTRING_REF_ITEM item = NULL;
	STRING_REF_KEY key;
	PVOID ret = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%wZ\"", Table, String);

	key.String = *String;
	status = RtlHashUnicodeString(String, TRUE, HASH_STRING_ALGORITHM_DEFAULT, &key.Hash);
	if (NT_SUCCESS(status)) {
		pBucket = Table->Buckets + key.Hash % Table->BucketCount;
		item = _ChainWalk(pBucket, &key, TRUE);
		if (item != NULL)
			ret = (PUCHAR)item - Table->ObjectKeyOffset;
	}

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


VOID StringRefTableDeleteDereference(_In_ PSTRING_REF_TABLE Table, _In_ PUNICODE_STRING String)
{
	PVOID obj = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%wZ\"", Table, String);

	obj = StringRefTableDelete(Table, String);
	if (obj != NULL)
		Table->Dereference(obj);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}




PVOID StringRefTableGet(_In_ PSTRING_REF_TABLE Table, _In_ PUNICODE_STRING String)
{
	PSTRING_REF_ITEM *pBucket = NULL;
	PVOID item = NULL;
	STRING_REF_KEY key;
	PVOID ret = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
//	DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%wZ\"", Table, String);

	key.String = *String;
	status = RtlHashUnicodeString(String, TRUE, HASH_STRING_ALGORITHM_DEFAULT, &key.Hash);
	if (NT_SUCCESS(status)) {
		pBucket = Table->Buckets + key.Hash % Table->BucketCount;
		item = _ChainWalk(pBucket, &key, FALSE);
		if (item != NULL) {
			ret = (PUCHAR)item - Table->ObjectKeyOffset;
			Table->Reference(ret);
		}
	}

//	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}
