
#ifndef __MAILSLOTS_PARSER_H__
#define __MAILSLOTS_PARSER_H__


#include "general-types.h"
#include "data-parser-types.h"



#ifdef MAILSLOTS_EXPORTS

#define MAILlOSTS_API			__declspec(dllexport)

#else

#define MAILlOSTS_API			__declspec(dllimport)

#endif


MAILlOSTS_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER* Parser);



#endif
