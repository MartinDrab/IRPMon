
#ifndef __WPD_PARSER_H__
#define __WPD_PARSER_H__



#include "general-types.h"
#include "data-parser-types.h"



#ifdef WPD_EXPORTS

#define WPD_API			__declspec(dllexport)

#else

#define WPD_API			__declspec(dllimport)

#endif


#ifdef __cplusplus
extern "C" {
#endif

WPD_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);

#ifdef __cplusplus
}
#endif


#endif
