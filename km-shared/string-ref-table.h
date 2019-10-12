
#ifndef __STRING_REF_TABLE_H__
#define __STRING_REF_TABLE_H__


#include <ntifs.h>


typedef VOID(STRING_REF_REFERENCE)(_Inout_ PVOID Object);
typedef VOID(STRING_REF_DEREFERENCE)(_Inout_ PVOID Object);


typedef struct _STRING_REF_KEY {
	UNICODE_STRING String;
	ULONG Hash;
} STRING_REF_KEY, *PSTRING_REF_KEY;


typedef struct _STRING_REF_ITEM {
	struct _STRING_REF_ITEM *Next;
	STRING_REF_KEY Key;
} STRING_REF_ITEM, *PSTRING_REF_ITEM;


typedef struct _STRING_REF_TABLE {
	STRING_REF_REFERENCE *Reference;
	STRING_REF_DEREFERENCE *Dereference;
	SIZE_T BucketCount;
	SIZE_T ObjectKeyOffset;
	PSTRING_REF_ITEM *Buckets;
} STRING_REF_TABLE, *PSTRING_REF_TABLE;


NTSTATUS StringRefTableInit(_In_ SIZE_T ObjectKeyOffset, _In_ SIZE_T BucketCount, _In_opt_ STRING_REF_REFERENCE *Reference, _In_opt_ STRING_REF_DEREFERENCE *Dereference, _Out_ PSTRING_REF_TABLE Table);
VOID StringRefTableDestroy(_In_ PSTRING_REF_TABLE Table);
VOID StringRefTableClear(_In_ PSTRING_REF_TABLE Table);
PVOID StringRefTableInsert(_In_ PSTRING_REF_TABLE Table, _In_ PVOID Object);
PVOID StringRefTableDelete(_In_ PSTRING_REF_TABLE Table, _In_ PUNICODE_STRING String);
VOID StringRefTableDeleteDereference(_In_ PSTRING_REF_TABLE Table, _In_ PUNICODE_STRING String);
PVOID StringRefTableGet(_In_ PSTRING_REF_TABLE Table, _In_ PUNICODE_STRING String);



#endif
