
#ifndef __PNP_INTERFACE_PARSER_H__
#define __PNP_INTERFACE_PARSER_H__



#include "general-types.h"
#include "data-parser-types.h"



#ifdef PNPINTERFACE_EXPORTS

#define PNPINTERFACE_API			__declspec(dllexport)

#else

#define PNPINTERFACE_API			__declspec(dllimport)

#endif


PNPINTERFACE_API
DWORD cdecl DP_INIT_ROUTINE_NAME(PIRPMON_DATA_PARSER Parser);










#endif
