
#ifndef __PIPES_PARSER_H__
#define __PIPES_PARSER_H__


#include "general-types.h"
#include "data-parser-types.h"



#ifdef PIPES_EXPORTS

#define PIPES_API			__declspec(dllexport)

#else

#define PIPES_API			__declspec(dllimport)

#endif


PIPES_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER* Parser);



#endif
