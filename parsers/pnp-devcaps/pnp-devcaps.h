
#ifndef __PNP_DEVCAPS_H__
#define __PNP_DEVCAPS_H__



#include "general-types.h"
#include "data-parser-types.h"



#ifdef PNPDEVCAPS_EXPORTS

#define PNPDEVCAPS_API			__declspec(dllexport)

#else

#define PNPDEVCAPS_API			__declspec(dllimport)

#endif


PNPDEVCAPS_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);










#endif
