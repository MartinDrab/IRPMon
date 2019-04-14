
#ifndef __DATA_PARSER_TYPES_H__
#define __DATA_PARSER_TYPES_H__

#include <windows.h>
#include "general-types.h"


#define DP_INIT_ROUTINE_NAME 	DataParserInit

typedef DWORD (cdecl DP_PARSE_ROUTINE)(const REQUEST_HEADER *Request, const wchar_t *DriverName, const wchar_t *DeviceName, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount);
typedef void (cdecl DP_FREE_ROUTINE)(wchar_t **Names, wchar_t **Values, size_t Count);

typedef struct _IRPMON_DATA_PARSER {
	const wchar_t *Name;
	DP_PARSE_ROUTINE *ParseRoutine;
	DP_FREE_ROUTINE *FreeRoutine;
	DWORD MajorVersion;
	DWORD MinorVersion;
	DWORD BuildVersion;
	DWORD Priority;
} IRPMON_DATA_PARSER, *PIRPMON_DATA_PARSER;

typedef DWORD (cdecl DP_INIT_ROUTINE)(PIRPMON_DATA_PARSER Parser);



#endif
