
#ifndef __UTILS_DYMARRAY_TYPES_H_
#define __UTILS_DYMARRAY_TYPES_H_

#include <ntifs.h>


#define DYM_ARRAY_INITIAL_ALLOC_LENGTH         16
#define DYM_ARRAY_INCREASE_PER_CENTS           20

typedef struct {
   POOL_TYPE PoolType;
   SIZE_T ValidLength;
   SIZE_T AllocatedLength;
   PFAST_MUTEX LockPaged;
   PKSPIN_LOCK LockNonPaged;
   PVOID *Data;
} UTILS_DYM_ARRAY, *PUTILS_DYM_ARRAY;


#endif
