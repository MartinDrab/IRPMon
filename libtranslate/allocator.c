
#include <stdio.h>
#include <windows.h>
#include "dlists.h"
#include "debug.h"
#include "allocator.h"


/************************************************************************/
/*                               LOCAL VARIABLES                        */
/************************************************************************/

static LIST_ENTRY _allocationRecordsList;
static CRITICAL_SECTION _allocationRecordsListLock;


/************************************************************************/
/*                                HELPER FUNCTIONS                      */
/************************************************************************/

static VOID _Lock(VOID)
{
   EnterCriticalSection(&_allocationRecordsListLock);
}

static VOID _Unlock(VOID)
{
   LeaveCriticalSection(&_allocationRecordsListLock);
}

static PDEBUG_ALLOCATION_RECORD _RecordAlloc(PVOID Address, SIZE_T NumberOfBytes, PCHAR Function, ULONG Line)
{
   PDEBUG_ALLOCATION_RECORD ret = NULL;
   DEBUG_ENTER_FUNCTION("Address=0x%p; NumberOfBytes=%u; Function=%s; Line=%u", Address, NumberOfBytes, Function, Line);

   ret = (PDEBUG_ALLOCATION_RECORD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEBUG_ALLOCATION_RECORD));
   if (ret != NULL) {
      ret->Address = Address;
      _InitializeListHead(&ret->Entry);
      ret->Function = Function;
      ret->Line = Line;
      ret->NumberOfBytes = NumberOfBytes;
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}

static VOID _RecordFree(PDEBUG_ALLOCATION_RECORD Record)
{
   DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

   HeapFree(GetProcessHeap(), 0, Record);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

static VOID _RecordInsert(PDEBUG_ALLOCATION_RECORD Record)
{
   DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

   _InsertTailList(&_allocationRecordsList, &Record->Entry);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

static VOID _RecordInsertLock(PDEBUG_ALLOCATION_RECORD Record)
{
   DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

   _Lock();
   _RecordInsert(Record);
   _Unlock();

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

static PDEBUG_ALLOCATION_RECORD _RecordFind(PVOID Address)
{
   PDEBUG_ALLOCATION_RECORD ret = NULL;
   DEBUG_ENTER_FUNCTION("Address=0x%p", Address);

   ret = CONTAINING_RECORD(_allocationRecordsList.Flink, DEBUG_ALLOCATION_RECORD, Entry);
   while (&_allocationRecordsList != &ret->Entry) {
      if (ret->Address == Address)
         break;

      ret = CONTAINING_RECORD(ret->Entry.Flink, DEBUG_ALLOCATION_RECORD, Entry);
   }

   if (&ret->Entry == &_allocationRecordsList)
      ret = NULL;

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}

static PDEBUG_ALLOCATION_RECORD _RecordFindLock(PVOID Address)
{
   PDEBUG_ALLOCATION_RECORD ret = NULL;
   DEBUG_ENTER_FUNCTION("Address=0x%p", Address);

   _Lock();
   ret = _RecordFind(Address);
   _Unlock();

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}

static VOID _RecordRemove(PDEBUG_ALLOCATION_RECORD Record)
{
   DEBUG_ENTER_FUNCTION("Record=0x%p");

   _RemoveEntryList(&Record->Entry);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

static VOID _RecordRemoveLock(PDEBUG_ALLOCATION_RECORD Record)
{
   DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

   _Lock();
   _RecordRemove(Record);
   _Unlock();

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

static VOID _CheckCallback(PDEBUG_ALLOCATION_RECORD Record, PVOID Context)
{
   DEBUG_ENTER_FUNCTION("Record=0x%p; Context=0x%p", Record, Context);

   DEBUG_PRINT_LOCATION("Address: 0x%p, size %u bytes", Record->Address, Record->NumberOfBytes);
   DEBUG_PRINT_LOCATION("Allocated in function %s at line %u", Record->Function, Record->Line);
   __debugbreak();

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

static VOID _AllocatorCheck(ALLOCATOR_CHECK_CALLBACK *Callback, PVOID Context)
{
   PDEBUG_ALLOCATION_RECORD tmp = NULL;
   DEBUG_ENTER_FUNCTION("Callaback=0x%p; Context=0x%p", Callback, Context);

   tmp = CONTAINING_RECORD(_allocationRecordsList.Flink, DEBUG_ALLOCATION_RECORD, Entry);
   while (&tmp->Entry != &_allocationRecordsList) {
      Callback(tmp, Context);
      tmp = CONTAINING_RECORD(tmp->Entry.Flink, DEBUG_ALLOCATION_RECORD, Entry);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/************************************************************************/
/*                               PUBLIC ROUTINES                        */
/************************************************************************/

PVOID DebugHeapMemoryAlloc(SIZE_T NumberOfBytes, PCHAR Function, ULONG Line)
{
   PVOID ret = NULL;
   PDEBUG_ALLOCATION_RECORD record = NULL;
   DEBUG_ENTER_FUNCTION("NumberOfBytes=%u; Function=%s; Lines=%u", NumberOfBytes, Function, Line);

   ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NumberOfBytes);
   if (ret != NULL) {
      if (!HeapValidate(GetProcessHeap(), 0, NULL)) {
         DEBUG_PRINT_LOCATION("Last operation occurred at function %s at line %d", Function, Line);
         __debugbreak();
      }

      record = _RecordAlloc(ret, NumberOfBytes, Function, Line);
      if (record != NULL)
         _RecordInsertLock(record);

      if (record == NULL) {
         HeapFree(GetProcessHeap(), 0, ret);
         ret = NULL;
      }
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}

VOID DebugHeapMemoryFree(PVOID Address)
{
   PDEBUG_ALLOCATION_RECORD record = NULL;
   DEBUG_ENTER_FUNCTION("Address=0x%p", Address);

   _Lock();
   record = _RecordFind(Address);
   if (record != NULL) {
      _RecordRemove(record);
      HeapFree(GetProcessHeap(), 0, Address);
      if (!HeapValidate(GetProcessHeap(), 0, NULL)) {
         DEBUG_PRINT_LOCATION("Last operation occurred at function %s at line %d", record->Function, record->Line);
         __debugbreak();
      }

      _RecordFree(record);
   } else {
      DEBUG_PRINT_LOCATION("ERROR: Allocation record for address 0x%p not found", Address);
   }

   _Unlock();

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

VOID DebugAllocatorCheck(ALLOCATOR_CHECK_CALLBACK *Callback, PVOID Context)
{
   DEBUG_ENTER_FUNCTION("Callkback=0x%p; Context=0x%p", Callback, Context);
    
   if (Callback == NULL)
      Callback = _CheckCallback;

   _Lock();
   _AllocatorCheck(Callback, Context);
   _Unlock();

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

/************************************************************************/
/*                             INITIALIZATION AND FINALIZATION          */
/************************************************************************/

DWORD DebugAllocatorInit(VOID)
{
   DWORD ret = ERROR_GEN_FAILURE;
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   if (InitializeCriticalSectionAndSpinCount(&_allocationRecordsListLock, 0x1000)) {
      _InitializeListHead(&_allocationRecordsList);
      ret = ERROR_SUCCESS;
   } else {
      ret = GetLastError();
   }

   DEBUG_EXIT_FUNCTION("%d", ret);
   return ret;
}

VOID DebugAllocatorFinit(VOID)
{
   DEBUG_ENTER_FUNCTION_NO_ARGS();

   DebugAllocatorCheck(NULL, NULL);
   DeleteCriticalSection(&_allocationRecordsListLock);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}
