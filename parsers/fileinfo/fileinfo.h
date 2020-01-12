
#ifndef __FILEINFO_PARSER_H__
#define __FILEINFO_PARSER_H__



#include "general-types.h"
#include "data-parser-types.h"



#ifdef FILEINFO_EXPORTS

#define FILEINFO_API			__declspec(dllexport)

#else

#define FILEINFO_API			__declspec(dllimport)

#endif


FILEINFO_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);










#endif
