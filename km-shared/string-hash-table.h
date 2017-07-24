
#ifndef __STRING_HASH_TABLE_H__
#define __STRING_HASH_TABLE_H__

#include <ntifs.h>
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



NTSTATUS StringHashTableCreate(EHashTableType Type, ULONG Size, PHASH_TABLE *Table);
VOID StringHashTableDestroy(PHASH_TABLE Table);
NTSTATUS StringHashTableInsert(PHASH_TABLE Table, PWCHAR String, PVOID Data);
NTSTATUS StringHashTableInsertUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String, PVOID Data);
PVOID StringHashTableGet(PHASH_TABLE Table, PWCHAR String);
PVOID StringHashTableGetUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String);
PVOID StringHashTableDelete(PHASH_TABLE Table, PWCHAR String);
PVOID StringHashTableDeleteUnicodeString(PHASH_TABLE Table, PUNICODE_STRING String);
VOID StringHashTablePerform(PHASH_TABLE Table, STRING_HASH_TABLE_ACTION_ROUTINE *Routine, PVOID Context);
VOID StringHashTablePerformWithFeedback(PHASH_TABLE Table, STRING_HASH_TABLE_ACTION_WITH_FEEDBACK_ROUTINE *Routine, PVOID Context);
ULONG StringHashTableGetItemCount(PHASH_TABLE Table);

#define StringHashTableClear(aHashTable) \
	HashTableClear(aHashTable, TRUE)



#endif
