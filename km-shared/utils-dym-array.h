
#ifndef __UTILS_DYM_ARRAY_H_
#define __UTILS_DYM_ARRAY_H_

#include <ntifs.h>
#include "kbase-exports.h"
#include "utils-dym-array-types.h"



KBASE_API
NTSTATUS DymArrayCreate(POOL_TYPE PoolType, PUTILS_DYM_ARRAY *Array);
KBASE_API
VOID DymArrayDestroy(PUTILS_DYM_ARRAY Array);
KBASE_API
NTSTATUS DymArrayReserve(PUTILS_DYM_ARRAY Array, SIZE_T Length);
KBASE_API
NTSTATUS DymArrayPushBack(PUTILS_DYM_ARRAY Array, PVOID Value);
KBASE_API
PVOID DymArrayPopBack(PUTILS_DYM_ARRAY Array);
KBASE_API
NTSTATUS DymArrayPushFront(PUTILS_DYM_ARRAY Array, PVOID Value);
KBASE_API
PVOID DymArrayPopFront(PUTILS_DYM_ARRAY Array);
KBASE_API
SIZE_T DymArrayLength(PUTILS_DYM_ARRAY Array);
KBASE_API
SIZE_T DymArrayAllocatedLength(PUTILS_DYM_ARRAY Array);
KBASE_API
PVOID DymArrayItem(PUTILS_DYM_ARRAY Array, SIZE_T Index);
KBASE_API
VOID DymArrayLock(PUTILS_DYM_ARRAY Array, PKIRQL Irql);
KBASE_API
VOID DymArrayUnlock(PUTILS_DYM_ARRAY Array, KIRQL Irql);
KBASE_API
VOID DymArrayToStaticArray(PUTILS_DYM_ARRAY Array, PVOID StaticArray);
KBASE_API
NTSTATUS DymArrayToStaticArrayAlloc(PUTILS_DYM_ARRAY Array, POOL_TYPE PoolType, PVOID **StaticArray);
KBASE_API
VOID DymArrayPushBackNoAlloc(PUTILS_DYM_ARRAY Array, PVOID Value);
KBASE_API
VOID DymArrayClear(PUTILS_DYM_ARRAY Array);
KBASE_API
VOID DymArrayPushArrayNoAlloc(PUTILS_DYM_ARRAY DymArray, PVOID *Array, ULONG Count);
KBASE_API
NTSTATUS DymArrayPushArray(PUTILS_DYM_ARRAY DymArray, PVOID *Array, ULONG Count);


#endif
