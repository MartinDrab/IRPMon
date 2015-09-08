
#include <windows.h>
#include "debug.h"
#include "allocator.h"
#include "libtranslate-hash-table.h"
#include "p2p-hash-table.h"


/************************************************************************/
/*                     HELPER FUNCTIONS                                 */
/************************************************************************/

static ULONG32 _HashFunction(PVOID Key)
{
   return (ULONG32)Key;
}

static BOOLEAN _CompareFunction(PHASH_ITEM Item, PVOID Key)
{
   PP2P_ITEM item = CONTAINING_RECORD(Item, P2P_ITEM, HashItem);

   return (item->Key == (ULONG_PTR)Key);
}

static VOID _FreeFunction(PHASH_ITEM Item)
{
   PP2P_ITEM item = CONTAINING_RECORD(Item, P2P_ITEM, HashItem);
   DEBUG_ENTER_FUNCTION("Item=0x%p", Item);

   HeapMemoryFree(item);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/************************************************************************/
/*                     PUBLIC FUNCTIONS                                 */
/************************************************************************/

DWORD P2PHashTableGet(PHASH_TABLE Table, ULONG_PTR Key, PULONG_PTR Value)
{
   PHASH_ITEM h = NULL;
   PP2P_ITEM item = NULL;
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Key=0x%p; Value=0x%p", Table, Key, Value);

   h = HashTableGet(Table, (PVOID)Key);
   if (h != NULL) {
      item = CONTAINING_RECORD(h, P2P_ITEM, HashItem);
      *Value = item->Value;
      ret = ERROR_SUCCESS;
   } else ret = ERROR_NOT_FOUND;

   DEBUG_EXIT_FUNCTION("%u, *Value=0x%p", ret, *Value);
   return ret;
}

DWORD P2PHashTableCreate(ULONG32 Size, PHASH_TABLE *Table)
{
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

   ret = HashTableCreate(Size, _HashFunction, _CompareFunction, _FreeFunction, Table);

   DEBUG_EXIT_FUNCTION("%u, *Table=0x%p", ret, *Table);
   return ret;
}

DWORD P2PHashTableInsert(PHASH_TABLE Table, ULONG_PTR Key, ULONG_PTR Value)
{
   PP2P_ITEM item = NULL;
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Key=0x%p; Value=0x%p", Table, Key, Value);

   item = (PP2P_ITEM)HeapMemoryAlloc(sizeof(P2P_ITEM));
   if (item != NULL) {
      item->Key = Key;
      item->Value = Value;
      HashTableInsert(Table, &item->HashItem, (PVOID)Key);
      ret = ERROR_SUCCESS;
   } else ret = ERROR_NOT_ENOUGH_MEMORY;

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

VOID P2PHashTableDestroy(PHASH_TABLE Table)
{
   DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

   HashTableDestroy(Table);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

VOID P2PHashTableDelete(PHASH_TABLE Table, ULONG_PTR Key)
{
   PHASH_ITEM h = NULL;
   PP2P_ITEM item = NULL;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Key=0x%p", Table, Key);

   h = HashTableDelete(Table, (PVOID)Key);
   if (h != NULL) {
      item = CONTAINING_RECORD(h, P2P_ITEM, HashItem);
      HeapMemoryFree(item);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}