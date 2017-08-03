
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "utils-dym-array.h"

#undef DEBUG_TRACE_ENABLED
#define DEBUG_TRACE_ENABLED 0


/************************************************************************/
/*                           HELPER ROUTINES                            */
/************************************************************************/

static NTSTATUS _DymArraySynchronizationAlloc(PUTILS_DYM_ARRAY Array)
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Array=0x%p", Array);

   switch (Array->PoolType) {
   case PagedPool:
      Array->LockPaged = (PFAST_MUTEX)HeapMemoryAllocNonPaged(sizeof(FAST_MUTEX));
      if (Array->LockPaged != NULL) {
         ExInitializeFastMutex(Array->LockPaged);
         status = STATUS_SUCCESS;
      } else status = STATUS_INSUFFICIENT_RESOURCES;
      break;
   case NonPagedPool:
      Array->LockNonPaged = (PKSPIN_LOCK)HeapMemoryAllocNonPaged(sizeof(KSPIN_LOCK));
      if (Array->LockNonPaged != NULL) {
         KeInitializeSpinLock(Array->LockNonPaged);
         status = STATUS_SUCCESS;
      } else status = STATUS_INSUFFICIENT_RESOURCES;
      break;
   default:
      DEBUG_ERROR("Invalid pool type supplied (%u)", Array->PoolType);
      status = STATUS_NOT_SUPPORTED;
      break;
   }

   DEBUG_EXIT_FUNCTION("0x%x", status);
   return status;
}


static VOID _DymArraySynchronizationFree(PUTILS_DYM_ARRAY Array)
{
   DEBUG_ENTER_FUNCTION("Array=0x%p", Array);

   switch (Array->PoolType) {
      case PagedPool:
         HeapMemoryFree(Array->LockPaged);
         break;
      case NonPagedPool:
         HeapMemoryFree(Array->LockNonPaged);
         break;
      default:
         DEBUG_ERROR("Invalid pool type of the array (%u)", Array->PoolType);
         KeBugCheck(0);
         break;
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


/************************************************************************/
/*                             PUBLIC ROUTINES                          */
/************************************************************************/



NTSTATUS DymArrayCreate(POOL_TYPE PoolType, PUTILS_DYM_ARRAY *Array)
{
   PUTILS_DYM_ARRAY tmpArray = NULL;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("PoolType=%u; Array=0x%p", PoolType, Array);

   *Array = NULL;
  tmpArray = (PUTILS_DYM_ARRAY)HeapMemoryAllocNonPaged(sizeof(UTILS_DYM_ARRAY));
  if (tmpArray != NULL) {
      tmpArray->PoolType = PoolType;
      tmpArray->AllocatedLength = DYM_ARRAY_INITIAL_ALLOC_LENGTH;
      tmpArray->ValidLength = 0;
      status = _DymArraySynchronizationAlloc(tmpArray);
      if (NT_SUCCESS(status)) {
         tmpArray->Data = HeapMemoryAlloc(tmpArray->PoolType, DYM_ARRAY_INITIAL_ALLOC_LENGTH * sizeof(PVOID));
         if (tmpArray->Data != NULL)
            *Array = tmpArray;
         else status = STATUS_INSUFFICIENT_RESOURCES;
		 
         if (!NT_SUCCESS(status))
            _DymArraySynchronizationFree(tmpArray);
      }

      if (!NT_SUCCESS(status))
         HeapMemoryFree(tmpArray);
   } else status = STATUS_INSUFFICIENT_RESOURCES;

   DEBUG_EXIT_FUNCTION("0x%x, *Array=0x%p", status, *Array);
   return status;
}


VOID DymArrayDestroy(PUTILS_DYM_ARRAY Array)
{
   DEBUG_ENTER_FUNCTION("Array=0x%p", Array);

   HeapMemoryFree(Array->Data);
   _DymArraySynchronizationFree(Array);
   HeapMemoryFree(Array);

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}



VOID DymArrayLock(PUTILS_DYM_ARRAY Array, PKIRQL Irql)
{
   DEBUG_ENTER_FUNCTION("Array=0x%p; Irql=0x%p", Array, Irql);

   switch (Array->PoolType) {
      case PagedPool:
         ExAcquireFastMutex(Array->LockPaged);
         break;
      case NonPagedPool:
         KeAcquireSpinLock(Array->LockNonPaged, Irql);
         break;
      default:
         DEBUG_ERROR("Invalid array pool type (%u)", Array->PoolType);
         KeBugCheck(0);
         break;
   }

   DEBUG_EXIT_FUNCTION(" *Irql=%u", *Irql);
   return;
}


VOID DymArrayUnlock(PUTILS_DYM_ARRAY Array, KIRQL Irql)
{
   DEBUG_ENTER_FUNCTION("Array=0x%p; Irql=%u", Array, Irql);

   switch (Array->PoolType) {
   case PagedPool:
      ExReleaseFastMutex(Array->LockPaged);
      break;
   case NonPagedPool:
      KeReleaseSpinLock(Array->LockNonPaged, Irql);
      break;
   default:
      DEBUG_ERROR("Invalid array pool type (%u)", Array->PoolType);
      KeBugCheck(0);
      break;
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}



NTSTATUS DymArrayReserve(PUTILS_DYM_ARRAY Array, SIZE_T Length)
{
   PVOID tmpBuffer = NULL;
   SIZE_T minLength = 0;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Array=0x%p; Length=%u", Array, Length);

  tmpBuffer = HeapMemoryAlloc(Array->PoolType, Length*sizeof(PVOID));
  if (tmpBuffer != NULL) {
      minLength = min(Array->ValidLength, Length);
      RtlCopyMemory(tmpBuffer, Array->Data, minLength * sizeof(PVOID));
      tmpBuffer = InterlockedExchangePointer((volatile PVOID *)&Array->Data, tmpBuffer);
      HeapMemoryFree(tmpBuffer);
      Array->AllocatedLength = Length;
      Array->ValidLength = minLength;
      status = STATUS_SUCCESS;
   } else status = STATUS_INSUFFICIENT_RESOURCES;

   DEBUG_EXIT_FUNCTION("0x%x", status);
   return status;
}


VOID DymArrayPushBackNoAlloc(PUTILS_DYM_ARRAY Array, PVOID Value)
{
   DEBUG_ENTER_FUNCTION("Array=0x%p; Value=0x%p", Array, Value);

   if (Array->ValidLength < Array->AllocatedLength) {
      Array->Data[Array->ValidLength] = Value;
      Array->ValidLength++;
   } else {
      DEBUG_ERROR("The array is full (0x%x)", STATUS_UNSUCCESSFUL);
      KeBugCheck(0);
   }

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}


NTSTATUS DymArrayPushBack(PUTILS_DYM_ARRAY Array, PVOID Value)
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Array=0x%p; Value=0x%p", Array, Value);

   if (Array->ValidLength == Array->AllocatedLength) {
      status = DymArrayReserve(Array, Array->AllocatedLength * (100 + DYM_ARRAY_INCREASE_PER_CENTS) / 100);
      if (Array->ValidLength == Array->AllocatedLength) {
         status = DymArrayReserve(Array, Array->AllocatedLength + 1);
      }
   } else status = STATUS_SUCCESS;

   if (NT_SUCCESS(status)) {
      Array->Data[Array->ValidLength] = Value;
      Array->ValidLength++;
   }

   DEBUG_EXIT_FUNCTION("0x%x", status);
   return status;
}


PVOID DymArrayPopBack(PUTILS_DYM_ARRAY Array)
{
   PVOID ret = NULL;
   DEBUG_ENTER_FUNCTION("Array=0x%p", Array);

   if (Array->ValidLength > 0) {
      ret = Array->Data[Array->ValidLength - 1];
      Array->ValidLength--;
   } else {
      DEBUG_ERROR("Attempt to remove an item from an empty array (0x%p)", Array);
      KeBugCheck(0);
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}


NTSTATUS DymArrayPushFront(PUTILS_DYM_ARRAY Array, PVOID Value)
{
   SIZE_T i = 0;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Array=0x%p; Value=0x%p", Array, Value);

   if (Array->ValidLength == Array->AllocatedLength) {
      status = DymArrayReserve(Array, Array->AllocatedLength * (100 + DYM_ARRAY_INCREASE_PER_CENTS) / 100);
      if (Array->ValidLength == Array->AllocatedLength) {
         status = DymArrayReserve(Array, Array->AllocatedLength + 1);
      }
   } else status = STATUS_SUCCESS;

   if (NT_SUCCESS(status)) {
      for (i = Array->ValidLength; i > 0; i--)
         Array->Data[i] = Array->Data[i - 1];

      Array->Data[0] = Value;
      Array->ValidLength++;
   }

   DEBUG_EXIT_FUNCTION("0x%x", status);
   return status;
}


PVOID DymArrayPopFront(PUTILS_DYM_ARRAY Array)
{   
   SIZE_T i = 0;
   PVOID ret = NULL;
   DEBUG_ENTER_FUNCTION("Array=0x%p", Array);

   if (Array->ValidLength > 0) {
      ret = Array->Data[0];
      for (i = 0; i < Array->ValidLength - 1; i++)
         Array->Data[i] = Array->Data[i + 1];

      Array->ValidLength--;
   } else {
      DEBUG_ERROR("Attempt to remove an item from an empty array (0x%p)", Array);
      KeBugCheck(0);
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}



SIZE_T DymArrayLength(PUTILS_DYM_ARRAY Array)
{
   SIZE_T ret = 0;
   DEBUG_ENTER_FUNCTION("Array=0x%p", Array);

   ret = Array->ValidLength;

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}


SIZE_T DymArrayAllocatedLength(PUTILS_DYM_ARRAY Array)
{
   SIZE_T ret = 0;
   DEBUG_ENTER_FUNCTION("Array=0x%p", Array);

   ret = Array->AllocatedLength;

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}


PVOID DymArrayItem(PUTILS_DYM_ARRAY Array, SIZE_T Index)
{
   PVOID ret;
   DEBUG_ENTER_FUNCTION("Array=0x%p; Index=%u", Array, Index);

   if (Index < Array->ValidLength) {
      ret = Array->Data[Index];
   } else {
      DEBUG_ERROR("Index out of bounds (%u)", Index);
      KeBugCheck(0);
   }

   DEBUG_EXIT_FUNCTION("0x%p", ret);
   return ret;
}


VOID DymArrayToStaticArray(PUTILS_DYM_ARRAY Array, PVOID StaticArray)
{
   DEBUG_ENTER_FUNCTION("Array=0x%p; StaticArray=0x%p", Array, StaticArray);

   RtlCopyMemory(StaticArray, Array->Data, Array->ValidLength * sizeof(PVOID));

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

NTSTATUS DymArrayToStaticArrayAlloc(PUTILS_DYM_ARRAY Array, POOL_TYPE PoolType, PVOID **StaticArray)
{
   SIZE_T numBytes = 0;
   PVOID *tmpStaticArray = NULL;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   DEBUG_ENTER_FUNCTION("Array=0x%p; PoolType=%u; StaticArray=0x%p", Array, PoolType, StaticArray);

   *StaticArray = NULL;
   numBytes = Array->ValidLength * sizeof(PVOID);
   tmpStaticArray = HeapMemoryAlloc(PoolType, numBytes);
   if (tmpStaticArray != NULL) {
      DymArrayToStaticArray(Array, tmpStaticArray);
      *StaticArray = tmpStaticArray;
      status = STATUS_SUCCESS;
   } else status = STATUS_INSUFFICIENT_RESOURCES;

   DEBUG_EXIT_FUNCTION("0x%x, *StaticArray=0x%p", status, *StaticArray);
   return status;
}

VOID DymArrayClear(PUTILS_DYM_ARRAY Array)
{
   DEBUG_ENTER_FUNCTION("Array=0x%p", Array);

   Array->ValidLength = 0;

   DEBUG_EXIT_FUNCTION_VOID();
   return;
}

VOID DymArrayPushArrayNoAlloc(PUTILS_DYM_ARRAY DymArray, PVOID *Array, ULONG Count)
{
	ULONG i = 0;
	DEBUG_ENTER_FUNCTION("DymArray=0x%p; Array=0x%p; Count=%u", DymArray, Array, Count);

	for (i = 0; i < Count; ++i)
		DymArrayPushBackNoAlloc(DymArray, Array[i]);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

NTSTATUS DymArrayPushArray(PUTILS_DYM_ARRAY DymArray, PVOID *Array, ULONG Count)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DymArray=0x%p; Array=0x%p; Count=%u", DymArray, Array, Count);

	status = DymArrayReserve(DymArray, DymArrayLength(DymArray) + Count);
	if (NT_SUCCESS(status))
		DymArrayPushArrayNoAlloc(DymArray, Array, Count);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}