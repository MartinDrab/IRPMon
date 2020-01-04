
#include <windows.h>
#include "debug.h"
#include "general-types.h"
#include "data-parser-types.h"
#include "dll-decider-types.h"



#define IRP_MJ_QUERY_SECURITY		0x14
#define IRP_MJ_SET_SECURITY			0x15



ULONG cdecl DLL_DECIDER_ROUTINE_DEFAULT_NAME(PREQUEST_GENERAL Request, PDP_REQUEST_EXTRA_INFO ExtraInfo, PDLL_DECIDER_DECISION Decision)
{
	ULONG ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Request=0x%p; ExtraInfo=0x%p; Decision=0x%p", Request, ExtraInfo, Decision);

	ret = ERROR_SUCCESS;
	memset(Decision, 0, sizeof(DLL_DECIDER_DECISION));
	if (Request->RequestTypes.Other.Type == ertIRP &&
		(Request->RequestTypes.Irp.MajorFunction == IRP_MJ_QUERY_SECURITY ||
		Request->RequestTypes.Irp.MajorFunction == IRP_MJ_SET_SECURITY)) {
		Decision->Action = ffaInclude;
		Decision->HighlightColor = 0x00ff00;
		Decision->Decided = TRUE;
		Decision->OverrideFilter = TRUE;
	}

	if (Request->RequestTypes.Other.Type == ertIRPCompletion &&
		(Request->RequestTypes.IrpComplete.MajorFunction == IRP_MJ_QUERY_SECURITY ||
			Request->RequestTypes.IrpComplete.MajorFunction == IRP_MJ_SET_SECURITY)) {
		Decision->Action = ffaInclude;
		Decision->HighlightColor = 0x00ff00;
		Decision->Decided = TRUE;
		Decision->OverrideFilter = TRUE;
	}

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}
