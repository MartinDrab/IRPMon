
#ifndef __STRING_HASH_TABLE_H__
#define __STRING_HASH_TABLE_H__

#include <ntifs.h>
#include "kbase-exports.h"
#include "hash_table.h"


typedef VOID (STRING_HASH_TABLE_ACTION_ROUTINE)(PWCHAR String, PVOID Data, PVOID Context);
typedef BOOLEAN (STRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE)(PWCHAR String, PVOID Data, PVOID Context);


typedef struct {
   STRING_HASH_TABLE_ACTION_ROUTINE *Routine;
   PVOID Context;
} STRING_HASH_TABLE_ACTION_ROUTINE_CONTEXT, *PSTRING_HASH_TABLE_ACTION_ROUTINE_CONTEXT;

typedef struct {
   STRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE *Routine;
   PVOID Context;
} STRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE_CONTEXT, *PSTRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE_CONTEXT;

typedef struct {
   HASH_ITEM Item;
   PVOID Data;
   ULONG StringLength;
   WCHAR String[1];
} STRING_HASH_ITEM, *PSTRING_HASH_ITEM;



KBASE_API
NTSTATUS StringHashTableCreate(EHashTableType Type, ULONG Size, PHASH_TABLE *Table);
KBASE_API
VOID StringHashTableDestroy(PHASH_TABLE Table);
KBASE_API
NTSTATUS StringHashTableInsert(PHASH_TABLE Table, PWCHAR String, PVOID Data);
KBASE_API
NTSTATUS StringHashTableInsertUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String, PVOID Data);
KBASE_API
PVOID StringHashTableGet(PHASH_TABLE Table, PWCHAR String);
KBASE_API
PVOID StringHashTableGetUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String);
KBASE_API
PVOID StringHashTableDelete(PHASH_TABLE Table, PWCHAR String);
KBASE_API
PVOID StringHashTableDeleteUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String);
KBASE_API
VOID StringHashTablePerform(PHASH_TABLE Table, STRING_HASH_TABLE_ACTION_ROUTINE *Routine, PVOID Context);
KBASE_API
VOID StringHashTablePerformWithFeedback(PHASH_TABLE Table, STRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE *Routine, PVOID Context);
KBASE_API
ULONG StringHashTableGetItemCount(PHASH_TABLE Table);

#define StringHashTableClear(aHashTable) \
	HashTableClear(aHashTable, TRUE)



#endif
