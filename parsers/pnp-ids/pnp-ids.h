
#ifndef __PNP_IDS_H__
#define __PNP_IDS_H__


#include "general-types.h"
#include "data-parser-types.h"



#ifdef PNPIDS_EXPORTS

#define PNPIDS_API			__declspec(dllexport)

#else

#define PNPIDS_API			__declspec(dllimport)

#endif


PNPIDS_API
DWORD cdecl DP_INIT_ROUTINE_NAME(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);



#endif
