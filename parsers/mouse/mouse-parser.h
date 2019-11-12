
#ifndef __MOUSE_PARSER_H__
#define __MOUSE_PARSER_H__



#include "general-types.h"
#include "data-parser-types.h"



#ifdef MOUSE_EXPORTS

#define MOUSE_API			__declspec(dllexport)

#else

#define MOUSE_API			__declspec(dllimport)

#endif


MOUSE_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);



#endif
