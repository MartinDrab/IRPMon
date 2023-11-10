
#ifndef __KEYBOARD_PARSER_H__
#define __KEYBOARD_PARSER_H__



#include "general-types.h"
#include "data-parser-types.h"



#ifdef KEYBOARD_EXPORTS

#define KEYBOARD_API			__declspec(dllexport)

#else

#define KEYBOARD_API			__declspec(dllimport)

#endif


KEYBOARD_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);










#endif
