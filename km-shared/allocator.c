
/**
 * @file
 *
 * Implementation of a special memory allocator. 
 * 
 * The allocator records information
 * about every memory allocation and deallocation, attempts to check validity of
 * a heap, which should catch writing too many data to small buffers, and keeps
 * an eye on memory leaks.
 *
 * Structure of every memory block allocated by the allocator looks as follows:
 * - THE HEADER (allocator-specific information are stored here)
 * - THE BLOCK (user gets pointer to beginning of this member)
 * - THE FOOTER (contains signature in order to detect writing to small buffers)
 *
 * The allocator records the following information about every allocated block:
 * - Type of memory pool
 * - size
 * - name of function that allocated it
 * - line of code that allocated it
 */

#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"


/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

#if _MSC_VER < 1700
volatile LONG __security_cookie = __LINE__;
#endif

static const ULONG _poolTag = (ULONG)'MPRI';

/** Synchronizes access to list of paged memory blocks. */
static ERESOURCE _pagedListLock;
/** synchronizes access to list of nonpaged memory blocks. */
static KSPIN_LOCK _nonPagedListLock;
/** Lists of memory blocks. One list for one memory pool. */
static LIST_ENTRY _poolLists [2];


/************************************************************************/
/*                            HELPER MACROS                             */
/************************************************************************/


/** Initializes header of an allocated block.
 *
 *  @param header Address of the header.
 *  @param PoolType Type of the memory pool the block is allocated from.
 *  @param NumberOfbytes Size of the block requested by the caller, in bytes.
 *  @param Function Name of function where the allocation was performed.
 *  @param Line Number of source line that performed the allocation.
 */
#define BlockHeaderInitialize(Header, PoolType, NumberOfBytes, Function, Line) \
   InitializeListHead(&Header->Entry);                \
   Header->PoolType = PoolType;                       \
   Header->NumberOfBytes = NumberOfBytes;             \
   Header->Function = Function;                       \
   Header->Line = Line;                               \
   Header->Signature = BLOCK_HEADER_SIGNATURE         


/** Initializes footer of an allocated block.
 *
 *  @param Footer Address of the footer.
 */
#define BlockFooterInitialize(Footer) \
   Footer->Signature = BLOCK_FOOTER_SIGNATURE 
 
/************************************************************************/
/*                        HELPER ROUTINES                               */
/************************************************************************/

/** Locks list of allocated blocks of given memory pool.
 *
 *  @param PoolType Type of memory pool.
 *  @param Irql Address of variable that, when locking list of nonpaged memory blocks,
 *  receives value of IRQL before the locking operation. The parameter is ignored when
 *  locking list of paged memory blocks.
 */
static VOID _PoolListLock(POOL_TYPE PoolType, PKIRQL Irql)
{
   switch (PoolType) {
      case NonPagedPool:
         KeAcquireSpinLock(&_nonPagedListLock, Irql);
         break;
      case PagedPool:
		  KeEnterCriticalRegion();
         ExAcquireResourceExclusiveLite(&_pagedListLock, TRUE);
         break;
      default:
         DEBUG_ERROR("Invalid memory pool type: %u", PoolType);
         break;
   }

   return;
}


/** Unlocks list of allocated blocks of given pool type.
 *
 *  @param PoolType Type of memory pool.
 *  @param Irql Value of IRQL which should be restored after the unlock operation is
 *  finished. The parameter is ignored when unlocking list of nonpaged memory blocks.
 */
static VOID _PoolUnlock(POOL_TYPE PoolType, KIRQL Irql)
{
   switch (PoolType) {
      case NonPagedPool:
         KeReleaseSpinLock(&_nonPagedListLock, Irql);
         break;
      case PagedPool:
         ExReleaseResourceLite(&_pagedListLock);
		 KeLeaveCriticalRegion();
         break;
      default:
         DEBUG_ERROR("Invalid memory pool type: %u", PoolType);
         break;
   }

   return;
}


/** Checks whether given allocated block of memory is valid.
 *
 *  @param Header Header of the block to check.
 *
 *  @remark
 *  The routine does not return any value indicating whether the block is valid or not.
 *  Instead, it prints a debug message and issues a breakpoint when the block is invalid.
 */
static VOID _BlockValidityCheck(PDEBUG_BLOCK_HEADER Header)
{
   BOOLEAN valid = FALSE;
   PDEBUG_BLOCK_FOOTER footer = (PDEBUG_BLOCK_FOOTER)((PUCHAR)Header + sizeof(DEBUG_BLOCK_HEADER) + Header->NumberOfBytes);
//   DEBUG_ENTER_FUNCTION("Header=0x%p", Header);

   valid = 
      ((Header->PoolType == PagedPool || Header->PoolType == NonPagedPool) &&
      (Header->Signature == BLOCK_HEADER_SIGNATURE) &&
      (footer->Signature == BLOCK_FOOTER_SIGNATURE));

   if (!valid) {
      DEBUG_ERROR("Block of memory is invalid: header=0x%p", Header);
   }

//   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/** Checks validity of all blocks in one list.
 *
 *  @param PoolType Type of memory pool which list of allocated blocks should be
 *  checked.
 */
static VOID _PoolValidityCheck(POOL_TYPE PoolType)
{
   KIRQL Irql;
   PDEBUG_BLOCK_HEADER header = NULL;
//   DEBUG_ENTER_FUNCTION("PoolType=%u", PoolType);

   _PoolListLock(PoolType, &Irql);
   header = CONTAINING_RECORD(_poolLists[PoolType].Flink, DEBUG_BLOCK_HEADER, Entry);
   while (&header->Entry != &_poolLists[PoolType]) {
      _BlockValidityCheck(header);
      header = CONTAINING_RECORD(header->Entry.Flink, DEBUG_BLOCK_HEADER, Entry);
   }

   _PoolUnlock(PoolType, Irql);

//   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/** Checks validity of blocks in all currently available lists of 
 *  allocated memory blocks. 
 *
 *  @remark
 *  If called at IRQL >= DISPATCH_LEVEL, only the list of nonpaged memory
 *  blocks is checked. Otherwise, the paged one is also examined.
 */
static VOID _HeapValidityCheck(VOID)
{
//   DEBUG_ENTER_FUNCTION_NO_ARGS();

   _PoolValidityCheck(NonPagedPool);
   // Check the paged pool heap only when we are running at IRQL
   // lower enough.
   if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
      _PoolValidityCheck(PagedPool);
   }

//   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/** Finds memory blocks allocated in given pool, that were not freed and hence are
 *  a memory leaks.
 *
 *  @param PoolType type of the memory pool.
 *
 *  @remark
 *  The routine does not return these blocks. Instead, it prints debug messages and
 *  issues a breakpoint for every block found not to be freed.
 */
static VOID _PoolFindUnfreedMemory(POOL_TYPE PoolType)
{
   PDEBUG_BLOCK_HEADER header = NULL;
//   DEBUG_ENTER_FUNCTION("PoolType=%u", PoolType);

   header = CONTAINING_RECORD(_poolLists[PoolType].Flink, DEBUG_BLOCK_HEADER, Entry);
   while (&header->Entry != &_poolLists[PoolType]) {
      DEBUG_PRINT_LOCATION("A block of allocated memory has been found: 0x%p", header + 1);
      DEBUG_PRINT_LOCATION("Pool type: %s", header->PoolType == NonPagedPool ? "nonpaged" : "paged");
      DEBUG_PRINT_LOCATION("Size:      %u", header->NumberOfBytes);
      DEBUG_PRINT_LOCATION("Allocated in function %s at line %u", header->Function, header->Line);
//      __debugbreak();
      header = CONTAINING_RECORD(header->Entry.Flink, DEBUG_BLOCK_HEADER, Entry);
   }

//   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/** Finds allocated memory blocks that were not freed, hence they are part of a memory
 *  leak.
 */
static VOID _FindUnfreedMemory(VOID)
{
//   DEBUG_ENTER_FUNCTION_NO_ARGS();

   _PoolFindUnfreedMemory(NonPagedPool);
   _PoolFindUnfreedMemory(PagedPool);

//   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/** Frees all memory used by the debug allocator for given pool type, including
 *  allocated memory blocks.
 *
 *  @param PoolType Memory pool type.
 */
static VOID _PoolFree(POOL_TYPE PoolType)
{
   PDEBUG_BLOCK_HEADER old = NULL;
   PDEBUG_BLOCK_HEADER header = NULL;
   DEBUG_ENTER_FUNCTION("PoolType=%u", PoolType);

   header = CONTAINING_RECORD(_poolLists[PoolType].Flink, DEBUG_BLOCK_HEADER, Entry);
   while (&header->Entry != &_poolLists[PoolType]) {
      old = header;
      header = CONTAINING_RECORD(header->Entry.Flink, DEBUG_BLOCK_HEADER, Entry);
      ExFreePool(old);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/** Frees all memory used by the debug allocator, including
 *  allocated memory blocks.
 */
static VOID _PoolsFree(VOID)
{
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   _PoolFree(NonPagedPool);
   _PoolFree(PagedPool);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/************************************************************************/
/*                      PUBLIC ROUTINES                                 */
/************************************************************************/


/** Allocates a block of memory and records the operation to the structures of the allocator.
 *  
 *  @param PoolType Type of memory pool from where the block should be allocated.
 *  @param NumberOfBytes Size of the block, in bytes.
 *  @param Function Name of routine that is calling the allocator.
 *  @param Line Line of code where the call occurred.
 *
 *  @return
 *  Returns address of newly allocated block of memory. If the allocation fails, 
 *  the function returns NULL.
 */
PVOID DebugAllocatorAlloc(POOL_TYPE PoolType, SIZE_T NumberOfBytes, PCHAR Function, ULONG Line)
{
   KIRQL Irql;
   PVOID ret = NULL;
   PDEBUG_BLOCK_HEADER header = NULL;
   PDEBUG_BLOCK_FOOTER footer = NULL;
   SIZE_T wholeSize = sizeof(DEBUG_BLOCK_HEADER) + NumberOfBytes + sizeof(DEBUG_BLOCK_FOOTER);
//   DEBUG_ENTER_FUNCTION("PoolType=%u; NumberOfBytes=%u; Function=%s; Line=%u", PoolType, NumberOfBytes, Function, Line);

   _HeapValidityCheck();
   ret = ExAllocatePoolWithTag(PoolType, wholeSize, _poolTag);
   if (ret != NULL) {
      header = (PDEBUG_BLOCK_HEADER)ret;
      footer = (PDEBUG_BLOCK_FOOTER)((PUCHAR)ret + sizeof(DEBUG_BLOCK_HEADER) + NumberOfBytes);
      BlockHeaderInitialize(header, PoolType, NumberOfBytes, Function, Line);
      BlockFooterInitialize(footer);
      _PoolListLock(PoolType, &Irql);
      InsertTailList(&_poolLists[PoolType], &header->Entry);
      _PoolUnlock(PoolType, Irql);
      ret = (PVOID)((PUCHAR)ret + sizeof(DEBUG_BLOCK_HEADER));
   }

//   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}


/** Frees a block of memory.
 *
 *  @param Address of the block, returned by DebugAllocatorAlloc routine.
 */
VOID DebugAllocatorFree(PVOID Address)
{
   KIRQL Irql;
   PDEBUG_BLOCK_HEADER header = (PDEBUG_BLOCK_HEADER)((PUCHAR)Address - sizeof(DEBUG_BLOCK_HEADER));
//   DEBUG_ENTER_FUNCTION("Address=0x%p", Address);

   _BlockValidityCheck(header);
   _HeapValidityCheck();
   _PoolListLock(header->PoolType, &Irql);
   RemoveEntryList(&header->Entry);
   _PoolUnlock(header->PoolType, Irql);
   ExFreePoolWithTag(header, _poolTag);

//   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/************************************************************************/
/*                     INITIALIZATION AND FINALIZACTION                 */
/************************************************************************/


/** Initializes the allocator.
 *
 *  @return
 *  Always returns STATUS_SUCCESS.
 */
NTSTATUS DebugAllocatorModuleInit(VOID)
{
#if _MSC_VER < 1700
	ULONG seed = 0xbadf00d;
#endif 
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION_NO_ARGS();

#if _MSC_VER < 1700
   // The __security_cookie must not be optimized out by the compiler because that causes
   // an unresolved external error on 32-bit platforms.
   __security_cookie = RtlRandom(&seed);
#endif 
   status = ExInitializeResourceLite(&_pagedListLock);
   if (NT_SUCCESS(status)) {
	   KeInitializeSpinLock(&_nonPagedListLock);
	   InitializeListHead(&_poolLists[0]);
	   InitializeListHead(&_poolLists[1]);
   }

   DEBUG_EXIT_FUNCTION("0x%x", status);
   return status;
}


/** Frees memory allocated by the allocator. The routine also attempts to find
 *  blocks of memory that were not freed. The routine is expected to be called
 *  just before the driver unloads.
 */
VOID DebugAllocatorModuleFinit(VOID)
{
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   _HeapValidityCheck();
   _FindUnfreedMemory();
   _PoolsFree();
   ExDeleteResourceLite(&_pagedListLock);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}
