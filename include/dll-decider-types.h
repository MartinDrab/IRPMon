
#ifndef __DLL_DECIDER_TYPES_H__
#define __DLL_DECIDER_TYPES_H__


#ifdef _KERNEL_MODE
#include <ntifs.h>
#else
#include <windows.h>
#endif
#include "general-types.h"
#include "data-parser-types.h"



typedef enum _EFilterAction {
	ffaHighlight,
	ffaInclude,
	ffaExclude,
	ffaPassToFilter,
} EFilterAction, *PEFilterAction;

typedef struct _DLL_DECIDER_DECISION {
	EFilterAction Action;
	ULONG HighlightColor;
	BOOLEAN Decided;
	BOOLEAN OverrideFilter;
} DLL_DECIDER_DECISION, *PDLL_DECIDER_DECISION;



#define DLL_DECIDER_ROUTINE_DEFAULT_NAME			DecideRoutine


typedef ULONG(cdecl DLL_DECIDER_DECIDE_ROUTINE)(PREQUEST_GENERAL Request, PDP_REQUEST_EXTRA_INFO ExtraInfo, PDLL_DECIDER_DECISION Decision);




#endif
