
#ifndef __LIBFILTERS_INTERNAL_H__
#define __LIBFILTERS_INTERNAL_H__


#include <ntifs.h>
#include "string-ref-table.h"
#include "libfilters.h"


typedef struct _FILTER_DRIVER_RECORD {
	STRING_REF_ITEM Item;
	volatile LONG ReferenceCount;
	EFilterDriverType Type;
	BOOLEAN First;
	UNICODE_STRING ServiceName;
	HANDLE ValueRecord;
	HANDLE KeyRecord;
	HANDLE CallbackHandle;
} FILTER_DRIVER_RECORD, *PFILTER_DRIVER_RECORD;







#endif
