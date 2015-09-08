
#ifndef __UTILS_ALLOCATOR_H__
#define __UTILS_ALLOCATOR_H__

#include <windows.h>

typedef struct {
   LIST_ENTRY Entry;
   PVOID Address;
   SIZE_T NumberOfBytes;
   PCHAR Function;
   ULONG Line;
} DEBUG_ALLOCATION_RECORD, *PDEBUG_ALLOCATION_RECORD;

typedef VOID (ALLOCATOR_CHECK_CALLBACK)(PDEBUG_ALLOCATION_RECORD Record, PVOID Context);

/** When defined, a custom memory allocator able to detect memory leaks is 
    used rather than the standard Windows heap functions.
 */
// #define USE_MEMORY_LEAK_DETECTION

/** When defined, the ntdll!RtlAllocateHeap is hooked in order to detect when
    an out of memory conditions happen. */
// #define MEMORY_ALLOCATION_HOOK

#ifdef USE_MEMORY_LEAK_DETECTION

#define HeapMemoryAlloc(a) DebugHeapMemoryAlloc(a, __FUNCTION__, __LINE__)
#define HeapMemoryFree(a)  DebugHeapMemoryFree(a)

#else

#define HeapMemoryAlloc(a) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, a)
#define HeapMemoryFree(a)  HeapFree(GetProcessHeap(), 0, a)

#endif


PVOID DebugHeapMemoryAlloc(SIZE_T NumberOfBytes, PCHAR Function, ULONG Line);
VOID DebugHeapMemoryFree(PVOID Address);
VOID DebugAllocatorCheck(ALLOCATOR_CHECK_CALLBACK *Callback, PVOID Context);

DWORD DebugAllocatorInit(VOID);
VOID DebugAllocatorFinit(VOID);


#endif
