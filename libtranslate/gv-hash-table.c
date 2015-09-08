        
/**
 * @file
 *
 * Implements a hash table mapping of system integer constants to their string
 * representation adn description. Generally speaking, the table maps 32-bit integers
 * to @link(GENERAL_VALUE) structures. Such type of hash table is called General Value Table.
 *
 * The purpose of the table implementation is to wrap general hash table implementation
 * and hide typecast pointer arithmatcy required to implement the mapping through the
 * general hash table directly. 
 */

#include <windows.h>
#include "debug.h"
#include "allocator.h"
#include "libtranslate-hash-table.h"
#include "gv-hash-table.h"


/************************************************************************/
/*                         HELPER ROUTINES                              */
/************************************************************************/

/** Hash Function for General Value Table.
 *
 *  @param Key Key value the hash of which is used as an index to the table.
 *
 *  @return
 *  Returns a hashed value for the given key.
 */
static ULONG32 _GVHashFunction(PVOID Key)
{
   return (ULONG32)Key;
}

/** Free function for General Value Table.
 *
 *  @param Item Address of table item to free.
 */
static VOID _GVFreeFunction(PHASH_ITEM Item)
{
   DEBUG_ENTER_FUNCTION("Item=0x%p", Item);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Compare function for General Value Table.
 *
 *  @param ObjectInTable The object to be compared with the key.
 *  @param Key The key.
 *
 *  @return
 *  Returns TRUE if the given object matches the given key and FALSE otherwise.
 */
static BOOLEAN _GVCompareFunction(PHASH_ITEM ObjectInTable, PVOID Key)
{
   ULONG32 ul = (ULONG32)Key;
   PGENERAL_VALUE pt = NULL;

   pt = CONTAINING_RECORD(ObjectInTable, GENERAL_VALUE, HashItem);
   return (ul == pt->Value);
}

/************************************************************************/
/*                        PUBLIC ROUTINES                               */
/************************************************************************/

/** Retrieves the @link(GENERAl_VALUE) structure corresponding to given integer
 *  value.
 *
 *  @param Table The table in question.
 *  @param Value The integer value for presence of which the table is queried.
 *
 *  @return
 *  Returns @link(GENERAL_VALUE) structure corresponding to the given integer value.
 *  If the table contains no such structure for the given value, NULL is returned.
 */
PGENERAL_VALUE GVHashTableGet(PHASH_TABLE Table, ULONG Value)
{
   PHASH_ITEM item = NULL;
   PGENERAL_VALUE ret = NULL;

   item = HashTableGet(Table, (PVOID)Value);
   if (item != NULL) {
      ret = CONTAINING_RECORD(item, GENERAL_VALUE, HashItem);
   }

   return ret;
}

/** Creates a new General Value Table.
 *
 *  @param Size Number of buckets in the new table.
 *  @param Table Address of variable that receives addres of the newly created
 *  General Value Table. 
 *
 *  @return
 *  Returns ELibrary error value indicating success or failure of the operation.
 */
DWORD GVHashTableCreate(ULONG32 Size, PHASH_TABLE *Table)
{
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION("Size=%u; Table=0x%p", Size, Table);

   ret = HashTableCreate(Size, _GVHashFunction, _GVCompareFunction, _GVFreeFunction, Table);

   DEBUG_EXIT_FUNCTION("%u, *Table=0x%p", ret, *Table);
   return ret;
}

/** Inserts a new integer-string mapping to a given General Value Table.
 *
 *  @param Table The table to be changed.
 *  @param Mapping The mapping to be inserted. Value member of the @link(GENERAL_VALUE)
 *  structure is used as a key for the table.
 */
VOID GVHashTableInsert(PHASH_TABLE Table, PGENERAL_VALUE Mapping)
{
   DEBUG_ENTER_FUNCTION("Table=0x%p; Textitem=0x%p", Table, Mapping);

   HashTableInsert(Table, &Mapping->HashItem, (PVOID)Mapping->Value);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Destroys a given General Value Table.
 *
 *  @param Table The table to destroy.
 */
VOID GVHashTableDestroy(PHASH_TABLE Table)
{
   DEBUG_ENTER_FUNCTION("Table=0x%p", Table);

   HashTableDestroy(Table);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}
