
/**
 * @file
 *
 * GENERAL HASH TABLE
 *
 * General hash table (or hash table, in short) is a hash table that
 * can be used to store any kind of data and use any data type as a key.
 *
 * When creating a general hash table, the user specifies four parameters:
 * 1) number of buckets of the table,
 * 2) a hash function used to convert values of the key to hashes,
 * 3) a compare function that is used to determine whether a table item
 *    corresponds to a given key,
 * 4) a free function that is used to delete all items present in the table
 *    during its destruction. Specifying this function is optional.
 *
 * General hash tables solves collisions by chaining.
 *
 * Probably the most interesting feature is that no memory allocations
 * are needed when inserting items to the table because table contents
 * is linked together using spare fields in the data (the data records
 * stored in the table must have such field of @link(HASH_ITEM) type).
 *
 * General hash tables may be protected by a reader-writer lock to
 * ensure thread-safety. Two types of the locks can be used:
 * * Executive resources (passive IRQL table). Access to the table is
 *   synchronized via an executive resource. Table items may be stored
 *   either in paged or in nonpaged memory. The table must be accessed
 *   at IRQL below DISPATCH_LEVEL.
 * * Reader-writer spin locks (dispatch IRQL table). Access to the table
 *   is synchronized via a reader-writer spin lock. Table items must be
 *   stored in nonpaged memory. The table can be accessed at IRQL <= DISPATCH_LEVEL,
 *   however, its contents is always accessed at DISPATCH_LEVEL, including table
 *   traversal function calls and usage of iterators.
 *
 * It is also possible to create a general hash table with no synchronization
 * at all. Such table is called a no-synchronization table can be accessed 
 * at any IRQL.
 *
 * There are two ways how to go through all contents of a general hash table. 
 * The first way is to specify a callback routine that is invoked for every table
 * item at the following IRQLs:
 * * dispatch IRQL for dispatch IRQL tables,
 * * IRQL of the entity that specified the callback for tables access to whom
 *   is not synchronized, or when the table is a passive IRQL table.
 *
 * General hash tables do not handle duplicities in any way. They do not
 * test for them when inserting. Hence it is possible to store duplicate items
 * in them, however, the user must take extreme caution. Duplicate items are
 * guaranteed to be stored in the same bucket, no other guarantees are given.
 *
 * When iterating through a general hash table, the order of retrieved items
 * does not respect the order of their insertion or any other meaningful one.
 * The items are retrieved in the order they are physically stored in the table.
 */

#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hash_table.h"


/************************************************************************/
/*                     HELPER MACROS                                    */
/************************************************************************/

#undef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE_ENABLED 0

#ifdef _DEBUG

/** Performs IRQL validation with respect to type of the table.
 *
 *  @param aTable The table in question.
 *
 *  @remark
 *  The macro triggers a bug check (or a breakpoint) when one of the
 *  following conditions is true:
 *  * the table is a passive IRQL table and the current IRQL >= DISPATCH_LEVEL,
 *  * the table is a dispatch IRQL table and the current IRQL > DISPATCH_LEVEL.
 *
 *  The macro is defined only for debug builds.
 */
#define HASH_TABLE_IRQL_VALIDATE(aTable)                           \
   switch (aTable->Type) {                                         \
      case httPassiveLevel:                                        \
         DEBUG_IRQL_LESS_OR_EQUAL(APC_LEVEL);                      \
         break;                                                    \
      case httDispatchLevel:                                       \
         DEBUG_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);                 \
         break;                                                    \
      case httNoSynchronization:                                   \
         DEBUG_IRQL_LESS_OR_EQUAL(HIGH_LEVEL);                     \
         break;                                                    \
      default:                                                     \
         DEBUG_ERROR("Invalid hash table type: %u", aTable->Type); \
         break;                                                    \
   }                                                               \

#else

#define HASH_TABLE_IRQL_VALIDATE(aTable) 

#endif

/************************************************************************/
/*                           HELPER FUNCTIONS                           */
/************************************************************************/

/** Locks a given table bucket for shared access.
 *
 *  @param Table Table which bucket is about to be locked.
 *  @param Index A zero-based index of the bucket to lock.
 *  @param Irql Address of variable. If the table is not a dispatch IRQL
 *  one, this parameter is ignored. Otherwise, the variable is filled with
 *  the current IRQL value just before the locking in shared mode is performed.
 *
 *  @remark
 *  If access to the table is not synchronized, the routine performs nothing.
 *
 *  The @link(HASH_TABLE_IRQL_VALIDATE) is used to check whether the caller
 *  runs at a valid IRQL.
 */
static VOID HashTableLockShared(PHASH_TABLE Table, ULONG32 Index, PKIRQL Irql)
{
   HASH_TABLE_IRQL_VALIDATE(Table);
   ASSERT(Index < Table->Size);

   switch (Table->Type) {
      case httPassiveLevel:
         KeEnterCriticalRegion();
         ExAcquireResourceSharedLite(&Table->Locks[Index], TRUE);
         break;
      case httDispatchLevel:
         KeAcquireSpinLock(&Table->DispatchLocks[Index], Irql);
         break;
      case httNoSynchronization:
         break;
      default:
         DEBUG_ERROR("Invalid hash table type: %u", Table->Type);
         break;
   }

   return;
}

/** Locks a given table bucket for exclusive access.
 *
 *  @param Table Table which bucket is about to be locked.
 *  @param Index A zero-based index of the bucket to lock.
 *  @param Irql Address of variable. If the table is not a dispatch IRQL
 *  one, this parameter is ignored. Otherwise, the variable is filled with
 *  the current IRQL value just before the locking in exclusive mode is performed.
 *
 *  @remark
 *  If access to the table is not synchronized, the routine performs nothing.
 *
 *  The @link(HASH_TABLE_IRQL_VALIDATE) is used to check whether the caller
 *  runs at a valid IRQL.
 */
static VOID HashTableLockExclusive(PHASH_TABLE Table, ULONG32 Index, PKIRQL Irql)
{
   HASH_TABLE_IRQL_VALIDATE(Table);
   ASSERT(Index < Table->Size);

   switch (Table->Type) {
      case httPassiveLevel:
         KeEnterCriticalRegion();
         ExAcquireResourceExclusiveLite(&(Table->Locks[Index]), TRUE);
         break;
      case httDispatchLevel:
         KeAcquireSpinLock(&Table->DispatchLocks[Index], Irql);
         Table->DispatchLockExclusive[Index] = TRUE;
         break;
      case httNoSynchronization:
         break;
      default:
         DEBUG_ERROR("Invalid hash table type: %u", Table->Type);
         break;
   }

   return;
}

/** Unlocks a given bucket of a general hash table.
 *
 *  @param Table A hash table the bucket of which is to be unlocked.
 *  @param Index A zero-based index of the bucked to unlock.
 *  @param Irql A value of IRQL the caller had been running before the table
 *  was locked. The parameter is ignored for passive IRQL tables and tables access
 *  to whom is not synchronized.
 *
 *  @remark
 *  If access to the table is not synchronized, the routine performs nothing.
 *
 *  The @link(HASH_TABLE_IRQL_VALIDATE) is used to check whether the caller
 *  runs at a valid IRQL.
 */
static VOID HashTableUnlock(PHASH_TABLE Table, ULONG32 Index, KIRQL Irql)
{
   HASH_TABLE_IRQL_VALIDATE(Table);
   ASSERT(Index < Table->Size);

   switch (Table->Type) {
      case httPassiveLevel:
         ExReleaseResourceLite(&Table->Locks[Index]);
         KeLeaveCriticalRegion();
         break;
      case httDispatchLevel:
         if (Table->DispatchLockExclusive[Index]) {
            Table->DispatchLockExclusive[Index] = FALSE;
            KeReleaseSpinLock(&Table->DispatchLocks[Index], Irql);
         } else {
            KeReleaseSpinLock(&Table->DispatchLocks[Index], Irql);
         }
         break;
      case httNoSynchronization:
         break;
      default:
         DEBUG_ERROR("Invalid hash table type: %u", Table->Type);
         KeBugCheck(0);
         break;
   }

   return;
}

/** Prepares a synchronization primitive that will be used to synchronize
 *  access to a given hash table. Passive IRQL tables use executive resources,
 *  dispatch IRQL tables use reader-writer spin locks.
 * 
 *  @param Table The table in question.
 *
 *  @return
 *  Returns a NTSTATUS value indicating success or failure of the operation.
 */
static NTSTATUS _HashTableSynchronizationAlloc(PHASH_TABLE Table)
{
   LONG i = 0;
   LONG j = 0;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Table=0x%p", Table);
   HASH_TABLE_IRQL_VALIDATE(Table);

   switch (Table->Type) {
      case httPassiveLevel:
         Table->Locks = (PERESOURCE)HeapMemoryAlloc(NonPagedPool, Table->Size * sizeof(ERESOURCE));
         if (Table->Locks != NULL) {
            for (i = 0; i < (LONG)Table->Size; ++i) {
               status = ExInitializeResourceLite(&Table->Locks[i]);
               if (!NT_SUCCESS(status)) {
                  for (j = i - 1; j >= 0; --j) {
                     ExDeleteResourceLite(&Table->Locks[j]);
                  }

                  break;
               }

               if (!NT_SUCCESS(status)) {
                  HeapMemoryFree(Table->Locks);
                  Table->Locks = NULL;
               }
            }
         } else status = STATUS_INSUFFICIENT_RESOURCES;
         break;
      case httDispatchLevel:
         Table->DispatchLockExclusive = (PBOOLEAN)HeapMemoryAlloc(NonPagedPool, Table->Size * sizeof(BOOLEAN));
         if (Table->DispatchLockExclusive != NULL) {
            Table->DispatchLocks = (PKSPIN_LOCK)HeapMemoryAlloc(NonPagedPool, Table->Size * sizeof(KSPIN_LOCK));
            if (Table->DispatchLocks != NULL) {
               for (i = 0; i < (LONG)Table->Size; ++i) {
                  Table->DispatchLockExclusive[i] = FALSE;
                  KeInitializeSpinLock(&Table->DispatchLocks[i]);
               }

               status = STATUS_SUCCESS;
            } else {
               status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(status)) {
               HeapMemoryFree(Table->DispatchLockExclusive);
               Table->DispatchLockExclusive = NULL;
            }
         } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
         }
         break;
      case httNoSynchronization:
         status = STATUS_SUCCESS;
         break;
      default:
         DEBUG_ERROR("Invalid table type %u", Table->Type);
         break;
   }

   DEBUG_EXIT_FUNCTION("0x%x", status);
   return status;
}

/** Deletes a synchronization primitive used to synchronize access to
 *  a given hash table.
 *
 *  @param Table The hash table in question.
 */
static VOID _HashTableSynchronizationFree(PHASH_TABLE Table)
{
   LONG i = 0;
   DEBUG_ENTER_FUNCTION("Table=0x%p", Table);
   HASH_TABLE_IRQL_VALIDATE(Table);

   switch (Table->Type) {
      case httPassiveLevel:
         for (i = (LONG)Table->Size - 1; i >= 0; --i) {
            ExDeleteResourceLite(&Table->Locks[i]);
         }

         HeapMemoryFree(Table->Locks);
         Table->Locks = NULL;
         break;
      case httDispatchLevel:
         HeapMemoryFree(Table->DispatchLocks);
         Table->DispatchLocks = NULL;
         HeapMemoryFree(Table->DispatchLockExclusive);
         Table->DispatchLockExclusive = NULL;
         break;
      case httNoSynchronization:
         break;
      default:
         DEBUG_ERROR("Invalid table type %u", Table->Type);
         break;
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/************************************************************************/
/*                   PUBLIC FUNCTIONS                                   */
/************************************************************************/

/** Creates a new general hash table.
 *
 *  @param Type Type of the table. This argument can have the following values.
 *    @value httPassiveLevel A passive IRQL table, accessible at IRQL below DISPATCH_LEVEL
 *    and synchronized via an executive resource.
 *    @value httDispatchLevel A dispatch IRQL table, accessible at IRQL <= DISPATCH_LEVEL
 *    and synchronized via a reader-writer spin lock.
 *    @value httNoSynchronization A table accessible at any IRQL and with no synchronization
 *    employed.
 *  @param Size Number of buckets of the new table.
 *  @param HashFunction Address of a hash function for the new table.
 *  @param CompareFunction Address of a compare function for the new table.
 *  @param FreeFunction Address of a free function for the new table. If this
 *  parameter is NULL, the new table will have no free function.
 *  @param Table Address of variable that receives the newly created
 *  general hash table.
 *
 *  @return
 *  The routine can return the following NTSTATUS values:
 *   @value STATUS_SUCCESS The table was successfully created.
 *   @value STATUS_INVALID_PARAMETER_X No hash function or compare function
 *   specified, or number of bucket is zero, or the type of the table is
 *   unknown.
 *   @value STATUS_INSUFFICIENT_RESOURCES There is not enough free memory
 *   to create the table.
 *   @value Other Some other error occurred.
 *
 *  @remark
 *  The routine can be called at IRQL <= DISPATCH_LEVEL.
 */
NTSTATUS HashTableCreate(EHashTableType Type, ULONG32 Size, IN HASH_FUNCTION HashFunction, COMPARE_FUNCTION CompareFunction, FREE_ITEM_FUNCTION FreeFunction, PHASH_TABLE *Table)
{
   PHASH_TABLE tmpTable = NULL;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   SIZE_T tableLength = sizeof(HASH_TABLE) + Size * sizeof(PHASH_ITEM);
   DEBUG_ENTER_FUNCTION("Type=%u; Size=%u; HashFunction=0x%p; CompareFunction=0x%p; FreeFunction=0x%p; Table=0x%p", Type, Size, HashFunction, CompareFunction, FreeFunction, Table);
   DEBUG_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

   if ((Type == httPassiveLevel || Type == httDispatchLevel || Type == httNoSynchronization) && 
       Size > 0 && HashFunction != NULL && CompareFunction != NULL) {
      tmpTable = (PHASH_TABLE)HeapMemoryAlloc(NonPagedPool, tableLength);
      if (tmpTable != NULL) {
         RtlZeroMemory(tmpTable, tableLength);
         tmpTable->Type = Type;
         tmpTable->Size = Size;
         tmpTable->HashFunction = HashFunction;
         tmpTable->CompareFunction = CompareFunction;
         tmpTable->FreeFunction = FreeFunction;
         tmpTable->NumberOfItems = 0;
         status = _HashTableSynchronizationAlloc(tmpTable);
         if (NT_SUCCESS(status)) {
            *Table = tmpTable;
         }

         if (!NT_SUCCESS(status)) {
            HeapMemoryFree(tmpTable);
         }
      } else {
         status = STATUS_INSUFFICIENT_RESOURCES;
      }
   } else {
      // Set the right error status according to what caused it.
      status = STATUS_INVALID_PARAMETER;
      if (Type != httPassiveLevel && Type != httDispatchLevel &&
          Type != httNoSynchronization) {
         status = STATUS_INVALID_PARAMETER_1;
      } else if (Size == 0) {
         status = STATUS_INVALID_PARAMETER_2;
      } else if (HashFunction == NULL) {
         status = STATUS_INVALID_PARAMETER_3;
      } else if (CompareFunction == NULL) {
         status = STATUS_INVALID_PARAMETER_4;
      }
   }

   DEBUG_EXIT_FUNCTION("0x%x, *Table=0x%p", status, *Table);
   return status;
}

/** Destroys a given general hash table.
 *
 *  @param Table A hash table to destroy.
 *
 *  @remark
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
VOID HashTableDestroy(PHASH_TABLE Table)
{
   LONG i = 0;
   PHASH_ITEM Tmp = NULL;
   PHASH_ITEM Bucket = NULL;
   DEBUG_ENTER_FUNCTION("Table=%p", Table);
   HASH_TABLE_IRQL_VALIDATE(Table);

   _HashTableSynchronizationFree(Table);
   for (i = 0; i < (LONG)Table->Size; i++) {
      Bucket = Table->Buckets[i];
      if (Bucket != NULL) {
         do {
            Tmp = Bucket;
            Bucket = Bucket->Next;
            if (Table->FreeFunction != NULL) {
               Table->FreeFunction(Tmp);
            }
         } while (Bucket != NULL);
      }
   }

   HeapMemoryFree(Table);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Inserts a new item into a given general hash table.
 *
 *  @param The table to which the new item should be inserted.
 *  @param Object The item to insert. The parameter is expected to be
 *  a pointer to a spare @link(HASH_ITEM) field in a data structure 
 *  that the caller wants to store in the table.
 *  @param Key Key value under which the item should be stored.
 *  
 *  @remark
 *  The routine uses the given HASH_ITEM structure to link the data into itself.
 *
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
VOID HashTableInsert(PHASH_TABLE Table, PHASH_ITEM Object, PVOID Key)
{
   KIRQL Irql;
   ULONG32 Index = 0;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Object=0x%p; Key=0x%p", Table, Object, Key);
   HASH_TABLE_IRQL_VALIDATE(Table);

   Index = Table->HashFunction(Key) % Table->Size;
   HashTableLockExclusive(Table, Index, &Irql);
   Object->Next = InterlockedExchangePointer(&Table->Buckets[Index], Object);
   HashTableUnlock(Table, Index, Irql);
   Table->NumberOfItems++;

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Deletes an item from a general hash table according to a given key.
 *
 *  @param Table A hash table on which the deletion operation should be
 *  performed.
 *  @param Key A key value used to search for the item to delete.
 *
 *  @return
 *  If the routine successfully finds and deletes the item identified
 *  by the given key, it returns its address. Otherwise, NULL is returned.
 *
 *  @remark
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
PHASH_ITEM HashTableDelete(PHASH_TABLE Table, PVOID Key)
{
   KIRQL Irql;
   ULONG32 Index = 0;
   PHASH_ITEM Akt = NULL;
   PHASH_ITEM Prev = NULL;
   BOOLEAN Ret = FALSE;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Key=0x%p", Table, Key);
   HASH_TABLE_IRQL_VALIDATE(Table);

   Index = Table->HashFunction(Key) % Table->Size;
   HashTableLockExclusive(Table, Index, &Irql);
   Akt = Table->Buckets[Index];
   Prev = NULL;
   while (Akt != NULL) {
      Ret = Table->CompareFunction(Akt, Key);
      if (Ret) {
         if (Prev != NULL) {
            Prev->Next = Akt->Next;
         } else {
            Table->Buckets[Index] = Akt->Next;
         }

         Table->NumberOfItems--;

         break;
      }

      Prev = Akt;
      Akt = Akt->Next;
   }

   HashTableUnlock(Table, Index, Irql);

   DEBUG_EXIT_FUNCTION("0x%p", Akt);
   return Akt;
}

/** Retrieves a table item according to a given key.
 *
 *  @param Table The table where the search should be done.
 *  @param Key the key value to be used.
 *
 *  @return
 *  If an item under the given key exists, the routine returns it. Otherwise,
 *  NULL is returned.
 *
 *  @remark
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
PHASH_ITEM HashTableGet(PHASH_TABLE Table, PVOID Key)
{
   KIRQL Irql;
   BOOLEAN Ret = FALSE;
   ULONG32 Index = 0;
   PHASH_ITEM Akt = NULL;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Key=0x%p", Table, Key);
   HASH_TABLE_IRQL_VALIDATE(Table);

   Index = Table->HashFunction(Key) % Table->Size;
   HashTableLockShared(Table, Index, &Irql);
   Akt = Table->Buckets[Index];
   while (Akt != NULL) {
      Ret = Table->CompareFunction(Akt, Key);
      if (Ret) {
         break;
      }

      Akt = Akt->Next;
   }

   HashTableUnlock(Table, Index, Irql);

   DEBUG_EXIT_FUNCTION("0x%p", Akt);
   return Akt;
}

/** Performs an action with every item stored in a given hash table.
 *
 *  @param Table The table on which the action should be performed.
 *  @param Callback Address of a callback routine that represents
 *  the action.
 *  @param Context User-defined value passed as a parameter to every
 *  invocation of the callback routine.
 *
 *  @remark
 *  The callback routine representing the action is always called to
 *  every item in the table. The callback can neither alter table contents,
 *  nor can stop the traversal.
 *
 *  During invocation of the callback, the bucket to which the table item on which
 *  the callback was called, is locked exclusively.
 *
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
VOID HashTablePerform(PHASH_TABLE Table, HASH_ITEM_CALLBACK Callback, PVOID Context)
{
   KIRQL Irql;
   ULONG i = 0;
   PHASH_ITEM tmp = NULL;
   PHASH_ITEM old = NULL;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Callback=0x%p; Context=0x%p", Table, Callback, Context);
   HASH_TABLE_IRQL_VALIDATE(Table);

   for (i = 0; i < Table->Size; ++i) {
      HashTableLockExclusive(Table, i, &Irql);
      tmp = Table->Buckets[i];
      while (tmp != NULL) {
         old = tmp;
         tmp = tmp->Next;
         Callback(old, Context);
      }

      HashTableUnlock(Table, i, Irql);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Performs an action-with-feedback with every item stored in a given hash table.
 *
 *  @param Table The table on which the action should be performed.
 *  @param Callback Address of a callback routine that represents
 *  the action.
 *  @param Context User-defined value passed as a parameter to every
 *  invocation of the callback routine.
 *
 *  @remark
 *  The callback routine representing the action is always called to
 *  every item in the table. The callback routine cannot alter contents
 *  of the table, however, it can stop the traversal process by returning
 *  FALSE. 
 *
 *  During invocation of the callback, the bucket to which the table item on which
 *  the callback was called, is locked exclusively.
 *
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
VOID HashTablePerformWithFeedback(PHASH_TABLE Table, HASH_ITEM_CALLBACK_WITH_FEEDBACK *Callback, PVOID Context)
{

   KIRQL Irql;
   ULONG i = 0;
   PHASH_ITEM tmp = NULL;
   PHASH_ITEM old = NULL;
   BOOLEAN cancelled = FALSE;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Callback=0x%p; Context=0x%p", Table, Callback, Context);
   HASH_TABLE_IRQL_VALIDATE(Table);

   for (i = 0; i < Table->Size; ++i) {
      HashTableLockExclusive(Table, i, &Irql);
      tmp = Table->Buckets[i];
      while (!cancelled && tmp != NULL) {
         old = tmp;
         tmp = tmp->Next;
         cancelled = !Callback(old, Context);
      }

      HashTableUnlock(Table, i, Irql);
      if (cancelled)
         break;
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Deletes are items stored in a given general hash table
 *
 *  @param Table The hash table to be cleared.
 *  @param CallFreeFunction If set to TRUE, the free function is called
 *  to every removed item. Otherwise, every item is just removed and
 *  nothing is further done with it.
 *
 *  @remark
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
VOID HashTableClear(PHASH_TABLE Table, BOOLEAN CallFreeFunction)
{
   KIRQL Irql;
   ULONG i = 0;
   PHASH_ITEM old = NULL;
   PHASH_ITEM akt = NULL;
   DEBUG_ENTER_FUNCTION("Table=0x%p; CallFreeFunction=%u", Table, CallFreeFunction);
   HASH_TABLE_IRQL_VALIDATE(Table);

   for (i = 0; i < Table->Size; ++i) {
      HashTableLockExclusive(Table, i, &Irql);
      if (CallFreeFunction && Table->FreeFunction != NULL) {
         akt = Table->Buckets[i];
         while (akt != NULL) {
            old = akt;
            akt = akt->Next;
            Table->FreeFunction(old);
         }
      }

      Table->Buckets[i] = NULL;
      HashTableUnlock(Table, i, Irql);
   }

   Table->NumberOfItems = 0;

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Retrieves number of items stored in a given general hash table.
 *
 *  @param Table The table in question.
 *
 *  @return
 *  Returns number of items stored in the given hash table.
 *
 *  @remark
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
ULONG HashTableGetItemCount(PHASH_TABLE Table)
{
   ULONG ret = 0;
   DEBUG_ENTER_FUNCTION("Table=0x%p", Table);
   HASH_TABLE_IRQL_VALIDATE(Table);

   ret = Table->NumberOfItems;

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

/************************************************************************/
/*                         ITERATOR FUNCTIONS                           */
/************************************************************************/

/** Retrieves an iterator representing the first item in a given general
 *  hash table.
 *
 *  @param Table The hash table the first item of which should be
 *  represented by the iterator.
 *  @param Iterator Address of iterator structure that is filled by the data
 *  representing the first item stored in the table.
 *
 *  @return
 *  If the routine returns TRUE, the iterator represents its first item. Bucket of
 *  that item is locked in shared mode and the current IRQL is set appropriately.
 *  If the routine returns FALSE, the table is empty and there is no item to retrieve.
 *  In such a case, the table is not locked and the current IRQL is left unchanged.
 *
 *  @remark
 *  The @link(HASH_TABLE_IRQL_VALIDATE) macro is used to check whether
 *  the caller runs at valid IRQL.
 */
BOOLEAN HashTableGetFirst(PHASH_TABLE Table, PHASH_TABLE_ITERATOR Iterator)
{
   KIRQL irql;
   ULONG i = 0;
   BOOLEAN ret = FALSE;
   DEBUG_ENTER_FUNCTION("Table=0x%p; Iterator=0x%p", Table, Iterator);
   HASH_TABLE_IRQL_VALIDATE(Table);

   for (i = 0; i < Table->Size; ++i) {
      HashTableLockShared(Table, i, &irql);
      ret = Table->Buckets[i] != NULL;
      if (ret) {
         Iterator->PointsToEnd = FALSE;
         Iterator->CurrentIndex = i;
         Iterator->CurrentItem = Table->Buckets[i];
         Iterator->Irql = irql;
         Iterator->Table = Table;
         break;
      }

      HashTableUnlock(Table, i, irql);
   }

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

/** Given an iterator representing a hash table item , retrieves 
 *  an iterator representing the next table item.
 *
 *  @param Iterator On input, the argument must contain address of 
 *  an iterator representing a hash table item. The routine fills it
 *  with an iterator representing the next item.
 *
 *  @return
 *  Returns TRUE if the routine found the item next to the one represented by
 *  the input iterator, and successfully updated the iterator to represent it.
 *  The bucket of the next item is locked in shared mode and the current IRQL is
 *  set appropriately.
 *
 * If the item represented by the input iterator is the last item in the table, 
 * the routine returns FALSE. In this case, the table is not locked and the 
 * value of the current IRQL is restored to the state before the @link(HashTableGetFirst)
 * was called.
 *
 *  @remark
 *  The calling thread had to call HashTableGetFirst routine before this call. It
 *  is not possible to pass iterators between threads.
 */
BOOLEAN HashTableGetNext(PHASH_TABLE_ITERATOR Iterator)
{
   ULONG i = 0;
   PHASH_TABLE table = NULL;
   BOOLEAN ret = FALSE;
   DEBUG_ENTER_FUNCTION("Iterator=0x%p", Iterator);

   table = Iterator->Table;
   for (i = Iterator->CurrentIndex; i < table->Size; ++i) {
      if (i != Iterator->CurrentIndex) {
         HashTableLockShared(table, i, &Iterator->Irql);
         Iterator->CurrentItem = table->Buckets[i];
         Iterator->CurrentIndex = i;
      } else {
         Iterator->CurrentItem = Iterator->CurrentItem->Next;
      }

      ret = (Iterator->CurrentItem != NULL);
      if (ret) {
         break;
      }

      HashTableUnlock(table, i, Iterator->Irql);
   }

   Iterator->PointsToEnd = !ret;

   DEBUG_EXIT_FUNCTION("%u", ret);
   return ret;
}

/** Finalizes a given hash table iterator.
 *
 *  @param Iterator Iterator to finalize.
 *
 *  @remark
 *  Every iterator must be finalized if it ever represented any hash
 *  table item.When an iterator is set to represent certain hash item,
 *  the bucket of that item is locked in shared mode. Finalization of
 *  the iterator releases the lock.
 */
VOID HashTableIteratorFinit(PHASH_TABLE_ITERATOR Iterator)
{
   DEBUG_ENTER_FUNCTION("Iterator=0x%p", Iterator);

   if (!Iterator->PointsToEnd) {
      HashTableUnlock(Iterator->Table, Iterator->CurrentIndex, Iterator->Irql);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/** Retrieves a hash table item associated with a given iterator.
 *
 *  @param Iterator The iterator representing a hash item.
 *
 *  @return
 *  Returns a hash item represented by the given iterator.
 */
PHASH_ITEM HashTableIteratorGetData(PHASH_TABLE_ITERATOR Iterator)
{
   PHASH_ITEM ret = NULL;
   DEBUG_ENTER_FUNCTION("Iterator=0x%p", Iterator);

   ret = Iterator->CurrentItem;

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}
