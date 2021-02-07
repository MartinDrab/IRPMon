
/**
 * @file
 *
 * Header file exporting routines of our special memory allocator capable of
 * detecting memory leaks and buffer overflows.
 */

#ifndef __PNPMON_ALLOCATOR_H__
#define __PNPMON_ALLOCATOR_H__

#include <ntifs.h>


 /** Magic signature of block header, used to detect overrides. */
#define BLOCK_HEADER_SIGNATURE         0xfeadefdf
/** Magic signature of block footer, used to detect overrides. */
#define BLOCK_FOOTER_SIGNATURE         0xf00defdf


/// Structure of the header of memory block allocated by the allocator.
typedef struct {
	/// Used to store the block within list of allocated blocks.
	LIST_ENTRY Entry;
	/// Name of function that allocated the block. */
	PCHAR Function;
	/// Line of code where the allocation occurred.
	ULONG Line;
	/// Type of memory pool the block is allocated from.
	POOL_TYPE PoolType;
	/// Size of the block, in bytes (without the header and the footer).
	SIZE_T NumberOfBytes;
	/// Header signature
	ULONG Signature;
} DEBUG_BLOCK_HEADER, * PDEBUG_BLOCK_HEADER;

/// Structure of the footer of memory block allocated by the allocator.
typedef struct {
	/// Signature of the footer.
	ULONG Signature;
} DEBUG_BLOCK_FOOTER, * PDEBUG_BLOCK_FOOTER;

#ifdef __cplusplus
extern "C" {
#endif

PVOID DebugAllocatorAlloc(POOL_TYPE PoolType, SIZE_T NumberOfBytes, PCHAR Function, ULONG Line);
void DebugAllocatorFree(PVOID Address);

#if defined(_DEBUG) || defined(DBG)
// #define MEMORY_LEAK_DETECTION
#endif

#ifdef MEMORY_LEAK_DETECTION

#define HeapMemoryAlloc(PoolType,NumberOfBytes)                DebugAllocatorAlloc(PoolType, NumberOfBytes, __FUNCTION__, __LINE__)
#define HeapMemoryFree(Buffer)                                 DebugAllocatorFree(Buffer)

#else

#define HeapMemoryAlloc(PoolType,NumberOfBytes)                ExAllocatePoolWithTag(PoolType,NumberOfBytes, 'MPRI')
#define HeapMemoryFree(Buffer)                                 ExFreePoolWithTag(Buffer, 'MPRI')

#endif

#define HeapMemoryAllocPaged(NumberOfBytes)                    HeapMemoryAlloc(PagedPool, NumberOfBytes)
#define HeapMemoryAllocNonPaged(NumberOfBytes)                 HeapMemoryAlloc(NonPagedPool, NumberOfBytes)


NTSTATUS DebugAllocatorModuleInit(void);
void DebugAllocatorModuleFinit(void);

#ifdef __cplusplus
}
#endif


#endif
