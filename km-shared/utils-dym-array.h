
#ifndef __UTILS_DYM_ARRAY_H_
#define __UTILS_DYM_ARRAY_H_

#include <ntifs.h>
#include "utils-dym-array-types.h"



NTSTATUS DymArrayCreate(POOL_TYPE PoolType, PUTILS_DYM_ARRAY *Array);
VOID DymArrayDestroy(PUTILS_DYM_ARRAY Array);
NTSTATUS DymArrayReserve(PUTILS_DYM_ARRAY Array, SIZE_T Length);
NTSTATUS DymArrayPushBack(PUTILS_DYM_ARRAY Array, PVOID Value);
PVOID DymArrayPopBack(PUTILS_DYM_ARRAY Array);
NTSTATUS DymArrayPushFront(PUTILS_DYM_ARRAY Array, PVOID Value);
PVOID DymArrayPopFront(PUTILS_DYM_ARRAY Array);
SIZE_T DymArrayLength(PUTILS_DYM_ARRAY Array);
SIZE_T DymArrayAllocatedLength(PUTILS_DYM_ARRAY Array);
PVOID DymArrayItem(PUTILS_DYM_ARRAY Array, SIZE_T Index);
VOID DymArrayLock(PUTILS_DYM_ARRAY Array, PKIRQL Irql);
VOID DymArrayUnlock(PUTILS_DYM_ARRAY Array, KIRQL Irql);
VOID DymArrayToStaticArray(PUTILS_DYM_ARRAY Array, PVOID StaticArray);
NTSTATUS DymArrayToStaticArrayAlloc(PUTILS_DYM_ARRAY Array, POOL_TYPE PoolType, PVOID **StaticArray);
VOID DymArrayPushBackNoAlloc(PUTILS_DYM_ARRAY Array, PVOID Value);
VOID DymArrayClear(PUTILS_DYM_ARRAY Array);
VOID DymArrayPushArrayNoAlloc(PUTILS_DYM_ARRAY DymArray, PVOID *Array, ULONG Count);
NTSTATUS DymArrayPushArray(PUTILS_DYM_ARRAY DymArray, PVOID *Array, ULONG Count);



#endif
