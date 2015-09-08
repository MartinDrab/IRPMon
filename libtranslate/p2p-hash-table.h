
#ifndef __P2P_HASH_TABLE_H__
#define __P2P_HASH_TABLE_H__

#include <windows.h>
#include "libtranslate-hash-table.h"


typedef struct {
   HASH_ITEM HashItem;
   ULONG_PTR Key;
   ULONG_PTR Value;
} P2P_ITEM, *PP2P_ITEM;


DWORD P2PHashTableGet(PHASH_TABLE Table, ULONG_PTR Key, PULONG_PTR Value);
DWORD P2PHashTableCreate(ULONG32 Size, PHASH_TABLE *Table);
DWORD P2PHashTableInsert(PHASH_TABLE Table, ULONG_PTR Key, ULONG_PTR Value);
VOID P2PHashTableDestroy(PHASH_TABLE Table);
VOID P2PHashTableDelete(PHASH_TABLE Table, ULONG_PTR Key);


#endif 
