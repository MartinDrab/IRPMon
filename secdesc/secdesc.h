
#ifndef __SECDESC_DLL_H__
#define __SECDESC_DLL_H__


#include "general-types.h"
#include "data-parser-types.h"



#ifdef SECDESC_EXPORTS

#define SECDESC_API			__declspec(dllexport)

#else

#define SECDESC_API			__declspec(dllimport)

#endif


SECDESC_API
DWORD cdecl DP_INIT_ROUTINE_NAME(PIRPMON_DATA_PARSER Parser);



#endif
