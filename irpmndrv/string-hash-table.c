
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hash_table.h"
#include "string-hash-table.h"


/************************************************************************/
/*                          HELPER ROUTINES                             */
/************************************************************************/

static ULONG32 _HashFunction(PVOID Key)
{
   ULONG i = 0;
   ULONG32 ret = 0;
   PUNICODE_STRING str = (PUNICODE_STRING)Key;

   for (i = 0; i < str->Length / sizeof(WCHAR); ++i)
      ret += i * towupper(str->Buffer[i]);

   return ret;
}


static BOOLEAN _CompareFunction(PHASH_ITEM ObjectInTable, PVOID Key)
{
   BOOLEAN ret = FALSE;
   PUNICODE_STRING str = (PUNICODE_STRING)Key;
   PSTRING_HASH_ITEM HashItem = CONTAINING_RECORD(ObjectInTable, STRING_HASH_ITEM, Item);
   UNICODE_STRING uItemStr;

   uItemStr.Length = (USHORT)HashItem->StringLength;
   uItemStr.MaximumLength = uItemStr.Length;
   uItemStr.Buffer = &HashItem->String[0];
   ret = RtlEqualUnicodeString(&uItemStr, str, TRUE);

   return ret;
}


static VOID _FreeFunction(PHASH_ITEM Object)
{
   PSTRING_HASH_ITEM HashItem = CONTAINING_RECORD(Object, STRING_HASH_ITEM, Item);
   DEBUG_ENTER_FUNCTION("Object=0x%p", Object);

   HeapMemoryFree(HashItem);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


static PSTRING_HASH_ITEM _StringToHashItem(PHASH_TABLE Table, PUNICODE_STRING String, PVOID Data)
{
   SIZE_T itemLen = 0;
   SIZE_T strLen = 0;
   PSTRING_HASH_ITEM ret= NULL;
   POOL_TYPE itemPoolType = NonPagedPool;
   DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%wZ\"; Data=0x%p", Table, String, Data);

   UNREFERENCED_PARAMETER(Table);

   itemPoolType = NonPagedPool;
   strLen = String->Length;
   itemLen = sizeof(STRING_HASH_ITEM) + strLen + sizeof(WCHAR);
   ret = HeapMemoryAlloc(itemPoolType, itemLen);
   if (ret != NULL) {
      RtlZeroMemory(ret, itemLen);
      ret->Data = Data;
	  ret->StringLength = (ULONG)strLen;
      RtlCopyMemory(&ret->String, String->Buffer, strLen);
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}


static VOID _ActionRoutine(PHASH_ITEM HashItem, PVOID Context)
{
   PSTRING_HASH_ITEM stringItem = NULL;
   PSTRING_HASH_TABLE_ACTION_ROUTINE_CONTEXT actionContext = (PSTRING_HASH_TABLE_ACTION_ROUTINE_CONTEXT)Context;
   DEBUG_ENTER_FUNCTION("HashItem=0x%p; Context=0x%p", HashItem, Context);

   stringItem = CONTAINING_RECORD(HashItem, STRING_HASH_ITEM, Item);
   actionContext->Routine((PWCHAR)&stringItem->String, stringItem->Data, actionContext->Context);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

static BOOLEAN _ActionFeedbackRoutine(PHASH_ITEM HashItem, PVOID Context)
{
   BOOLEAN ret = FALSE;
   PSTRING_HASH_ITEM stringItem = NULL;
   PSTRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE_CONTEXT actionContext = (PSTRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE_CONTEXT)Context;
   DEBUG_ENTER_FUNCTION("HashItem=0x%p; Context=0x%p", HashItem, Context);

   stringItem = CONTAINING_RECORD(HashItem, STRING_HASH_ITEM, Item);
   ret = actionContext->Routine((PWCHAR)&stringItem->String, stringItem->Data, actionContext->Context);

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

/************************************************************************/
/*                                 PUBLIC ROUTINES                      */
/************************************************************************/

NTSTATUS StringHashTableCreate(EHashTableType Type, ULONG Size, PHASH_TABLE *Table)
{
   PHASH_TABLE tmpTable = NULL;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Type=%u; Size=%u; Table=0x%p", Type, Size, Table);

   status = HashTableCreate(Type, Size, _HashFunction, _CompareFunction, _FreeFunction, &tmpTable);
   if (NT_SUCCESS(status))
      *Table = tmpTable;

   DEBUG_EXIT_FUNCTION("0x%x, *Table=0x%p", status, *Table);
   return status;
}


VOID StringHashTableDestroy(PHASH_TABLE Table)
{
   DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

   HashTableDestroy(Table);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


NTSTATUS StringHashTableInsert(PHASH_TABLE Table, PWCHAR String, PVOID Data)
{
   UNICODE_STRING uStr;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%S\"; Data=0x%p", Table, String, Data);

   RtlInitUnicodeString(&uStr, String);
   status = StringHashTableInsertUnicodeString(Table, &uStr, Data);

   DEBUG_EXIT_FUNCTION("0x%x", status);
   return status;
}

NTSTATUS StringHashTableInsertUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String, PVOID Data)
{
	PSTRING_HASH_ITEM HashItem = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%wZ\"; Data=0x%p", Table, String, Data);

	HashItem = _StringToHashItem(Table, String, Data);
	if (HashItem != NULL) {
		HashTableInsert(Table, &HashItem->Item, String);
		status = STATUS_SUCCESS;
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;

}

PVOID StringHashTableGetUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String)
{
	PVOID ret = NULL;
	PHASH_ITEM item = NULL;
	PSTRING_HASH_ITEM stringItem = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%wZ\"", Table, String);

	item = HashTableGet(Table, String);
	if (item != NULL) {
		stringItem = CONTAINING_RECORD(item, STRING_HASH_ITEM, Item);
		ret = stringItem->Data;
	}

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;

}

PVOID StringHashTableGet(PHASH_TABLE Table, PWCHAR String)
{
   PVOID ret = NULL;
   UNICODE_STRING uStr;
   DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%S\"", Table, String);

   RtlInitUnicodeString(&uStr, String);
   ret = StringHashTableGetUnicodeString(Table, &uStr);

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}

PVOID StringHashTableDeleteUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String)
{
	PVOID ret = NULL;
	PHASH_ITEM item = NULL;
	PSTRING_HASH_ITEM stringItem = NULL;
	DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%wZ\"", Table, String);

	item = HashTableDelete(Table, String);
	if (item != NULL) {
		stringItem = CONTAINING_RECORD(item, STRING_HASH_ITEM, Item);
		ret = stringItem->Data;
		HeapMemoryFree(stringItem);
	}

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}

PVOID StringHashTableDelete(PHASH_TABLE Table, PWCHAR String)
{
   PVOID ret = NULL;
   UNICODE_STRING uStr;
   DEBUG_ENTER_FUNCTION("Table=0x%p; String=\"%S\"", Table, String);

   RtlInitUnicodeString(&uStr, String);
   ret = StringHashTableDeleteUnicodeString(Table, &uStr);

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}


VOID StringHashTablePerform(PHASH_TABLE Table, STRING_HASH_TABLE_ACTION_ROUTINE *Routine, PVOID Context)
{
   STRING_HASH_TABLE_ACTION_ROUTINE_CONTEXT actionContext;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Routine=0x%p; Context=0x%p", Table, Routine, Context);

   actionContext.Routine = Routine;
   actionContext.Context = Context;
   HashTablePerform(Table, _ActionRoutine, &actionContext);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

VOID StringHashTablePerformWithFeedback(PHASH_TABLE Table, STRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE *Routine, PVOID Context)
{
   STRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE_CONTEXT actionContext;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Routine=0x%p; Context=0x%p", Table, Routine, Context);

   actionContext.Routine = Routine;
   actionContext.Context = Context;
   HashTablePerformWithFeedback(Table, _ActionFeedbackRoutine, &actionContext);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


ULONG StringHashTableGetItemCount(PHASH_TABLE Table)
{
   ULONG ret = 0;
   DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

   ret = HashTableGetItemCount(Table);

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}