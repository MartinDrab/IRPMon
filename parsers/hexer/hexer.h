
#ifndef __HEXER_DLL_H__
#define __HEXER_DLL_H__


#include "general-types.h"
#include "data-parser-types.h"



#ifdef HEXER_EXPORTS

#define HEXER_API			__declspec(dllexport)

#else

#define HEXER_API			__declspec(dllimport)

#endif


HEXER_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);



#endif
