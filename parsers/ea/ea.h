
#ifndef __EA_PARSER_H__
#define __EA_PARSER_H__



#include "general-types.h"
#include "data-parser-types.h"



#ifdef EA_EXPORTS

#define EA_API			__declspec(dllexport)

#else

#define EA_API			__declspec(dllimport)

#endif


EA_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);










#endif
