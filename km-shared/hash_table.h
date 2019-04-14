
/**
 * @file:
 *
 * Exposes data types and routine definitions for general hash tables. 
 */

#ifndef __HASH_TABLE_H_
#define __HASH_TABLE_H_

#include <ntifs.h>


/** Links hash table items stored in one bucket together. These structures
    are expected to be a members of data that the user actually wishes to
    store inside a general hash table. Some table support routines return 
    address of the HASH_ITEM structure, the user can use CONTAINING_RECORD
    macro to obtain address of her data. */
typedef struct _HASH_ITEM {
   /** Address of the next item in the bucket. */
   struct _HASH_ITEM *Next;
} HASH_ITEM, *PHASH_ITEM;


/** Prototype of a general hash table hash function.
 *
 *  Converts a given key value to index of the target
 *  bucket. Implementation of the callback should not target specific
 *  number of buckets, general hash tables perform a modulo operation
 *  automatically to ensure that the result of the callback is a valid
 *  bucket index.
 *
 *  @param Key The key value to transform into a bucket index.
 *
 *  @return
 *  The callback must return a hashed key value. 
 *
 *  @remark
 *  The callback implementation must be deterministic (must return same
 *  outputs when given the same inputs), should be fast and should distribute
 *  its outputs uniformly.
 *
 *  The callback is always invoked at the IRQL the HashXXX routine that calls it
 *  runs on. The table is not locked during execution of the callback.
 */
typedef ULONG32 (*HASH_FUNCTION) (PVOID Key);

/** Prototype of a general hash table compare function.
 *
 *  The task of the callback is to determine whether a given table item
 *  and a key matches. 
 *
 *  @param ObjectInTable The table item.
 *  @param Key The key.
 *
 *  @return
 *  If the callback returns TRUE, the hash table treats as if the item was
 *  inserted to the table under the given key. Specifying this key, the item
 *  can be retrieved or removed from the table.
 *
 *  During execution of the callback, bucket where the table item specified in
 *  the first argument is stored, is locked for shared access.
 *
 *  IRQL at which the callback is invoked follows the following rules:
 *  * If the table is a passive IRQL or no-synchronization one, the IRQL is equal
 *    to the @link(HashTableGet) or @link(HashtableDelete) call in which the callback is invoked.
 *  * If the table is a dispatch IRQL one, the IRQL is always DISPATCH_LEVEL.
 */
typedef BOOLEAN (*COMPARE_FUNCTION) (PHASH_ITEM ObjectInTable, PVOID Key);

/** Prototype of a general hash table free function.
 *
 *  The function is invoked for every item stored in the hash table
 *  in time of its destruction.
 *
 *  @param Object Address of the table item.
 *
 *  @remark *  The IRQL at which the callback is invoked follows these conditions:
 *  * If the table is a passive IRQL table or no-synchronization table,
 *    the IRQL is the same as IRQL of the @link(HashTableDestroy) or @link(HashtableClear) caller.
 *  * If the table is a dispatch IRQL table, the IRQL is equal to DISPATCH_LEVEL when the callback
 *    is invoked inside a HashTableClear call, and below DISPATCH_LEVEL when the callback is
 *    executed inside a HashTableDestroy. 
 */
typedef VOID (*FREE_ITEM_FUNCTION) (PHASH_ITEM Object);

/** Prototype of the action callback that is invoked for every item stored inside the
 *  table when a @link(HashTablePerform) is called.
 *
 *  @param Object Address of the item.
 *  @param Context User-defined variable supplied as an argument to the
 *  HashTablePerform call.
 *
 *  @remark
 *  The bucket to which the item passed to the callback routine as the first
 *  parameter, is locked for exclusive access.
 *
 *  The IRQL at which the callback is invoked follows these conditions:
 *  * If the table is a passive IRQL table or no-synchronization table,
 *    the IRQL is the same as IRQL of the HashTableperform caller.
 *  * If the table is a dispatch IRQL table, the IRQL is equal to DISPATCH_LEVEL. 
 */
typedef VOID (*HASH_ITEM_CALLBACK)(IN PHASH_ITEM Object, PVOID Context);

/** Prototype of the action-feedback callback that represents an action
 *  taken to every item found in the given hash table. The user specifies
 *  this callback when calling the @link(HashTablePerformWithFeedback) routine.
 *
 *  @param Object The hash table item.
 *  @param Context User-defined value passed as the last parameter to the
 *  HashtablePerformWithFeedback. 
 *
 *  @return
 *  If the callback returns TRUE the HashTablePerformWithFeedback finds a
 *  next table item and calls the callback on it. Otherwise, the routine stops
 *  the traversal and returns to the caller.
 *
 *  @remark
 *  The bucket to which the item passed to the callback routine as the first
 *  parameter, is locked for exclusive access.
 *
 *  The IRQL at which the callback is invoked follows these conditions:
 *  * If the table is a passive IRQL table or no-synchronization table,
 *    the IRQL is the same as IRQL of the HashTableperformWithFeedback caller.
 *  * If the table is a dispatch IRQL table, the IRQL is equal to DISPATCH_LEVEL. 
 */
typedef BOOLEAN (HASH_ITEM_CALLBACK_WITH_FEEDBACK)(PHASH_ITEM Object, PVOID Context);

/** Represents a type of general hash table. */
typedef enum {
   /** A passive IRQL table. Access to it is synchronized via an executive
       resource and must be done at IRQL below DISPATCH_LEVEL. Table items
       may be allocated from paged or nonpaged pool. */
   httPassiveLevel,
   /** A dispatch IRQL table. Access to it is synchronized via a reader-writer
       spin lock and must be done at IRQL <= DISPATCH_LEVEL. Table items must
       be allocated from nonpaged pool. */
   httDispatchLevel,
   /** A no-synchronization table. Access to the table is not synchronized, no
      limitations to IRQL and memory pool for table items are placed. */
   httNoSynchronization
} EHashTableType, *PEHashTableType;

/** Represents a general hash table. */
typedef struct _HASH_TABLE {
   /** Number of buckets (slots). */
   ULONG32 Size;
   /** type of the table. */
   EHashTableType Type;
   /** Address of the hash function. */
   HASH_FUNCTION HashFunction;
   /** Address of the compare function. */
   COMPARE_FUNCTION CompareFunction;
   /** Address of the free function. */
   FREE_ITEM_FUNCTION FreeFunction;
   /** Array of executive resources used to synchronize access to individual
       buckets. For passive IRQL tables only. */
   PERESOURCE Locks;
   /** Array of reader-writer spin locks used to synchronize access to individual
       buckets. For dispatch IRQL tables only. */
   PKSPIN_LOCK DispatchLocks;
   /** Array of boolean variables that indicate which buckets are
       locked exclusively. Used by dispatch IRQL tables only. */
   PBOOLEAN DispatchLockExclusive;
   /** Number of entries stored in the hash table. */
   volatile ULONG NumberOfItems;
   /** The buckets. */
   PHASH_ITEM Buckets[1];
} HASH_TABLE, *PHASH_TABLE;

/** Represents a general hash table iterator. The iterator can represent
    one table item. */
typedef struct {
   /** Bucket index of the item represented by the iterator. */
   ULONG CurrentIndex;
   /** Pointer to the item associated with the iterator. */
   PHASH_ITEM CurrentItem;
   /** Value of IRQL the caller was running at before this iterator
       started to represent any table item. */
   KIRQL Irql;
   /** Address of the hash table item of which the iterator represents. */
   PHASH_TABLE Table;
   /** If set to TRUE, the iterator does not represent any hash item. It
       points after the last item of the table. */
   BOOLEAN PointsToEnd;
} HASH_TABLE_ITERATOR, *PHASH_TABLE_ITERATOR;


NTSTATUS HashTableCreate(EHashTableType Type, IN ULONG32 Size, IN HASH_FUNCTION HashFunction, IN COMPARE_FUNCTION CompareFunction, IN FREE_ITEM_FUNCTION FreeFunction, OUT PHASH_TABLE *Table);
VOID HashTableDestroy(IN PHASH_TABLE Table);
VOID HashTableInsert(IN PHASH_TABLE Table, IN PHASH_ITEM Object, IN PVOID Key);
PHASH_ITEM HashTableDelete(IN PHASH_TABLE Table, IN PVOID Key);
PHASH_ITEM HashTableGet(IN PHASH_TABLE Table, IN PVOID Key);
VOID HashTablePerform(PHASH_TABLE Table, HASH_ITEM_CALLBACK Callback, PVOID Context);
VOID HashTablePerformWithFeedback(PHASH_TABLE Table, HASH_ITEM_CALLBACK_WITH_FEEDBACK *Callback, PVOID Context);
VOID HashTableClear(PHASH_TABLE Table, BOOLEAN CallFreeFunction);
ULONG HashTableGetItemCount(PHASH_TABLE Table);

BOOLEAN HashTableGetFirst(PHASH_TABLE HashTable, PHASH_TABLE_ITERATOR Iterator);
BOOLEAN HashTableGetNext(PHASH_TABLE_ITERATOR Iterator);
VOID HashTableIteratorFinit(PHASH_TABLE_ITERATOR Iterator);
PHASH_ITEM HashTableIteratorGetData(PHASH_TABLE_ITERATOR Iterator);


#endif
