
#ifndef __DATA_PARSER_TYPES_H__
#define __DATA_PARSER_TYPES_H__

#include <stdint.h>
#include <windows.h>
#include "general-types.h"


#define DP_INIT_ROUTINE_NAME 	DataParserInit

typedef struct _DP_REQUEST_EXTRA_INFO {
	PWCHAR DriverName;
	PWCHAR DeviceName;
	PWCHAR FileName;
} DP_REQUEST_EXTRA_INFO, *PDP_REQUEST_EXTRA_INFO;

typedef DWORD (cdecl DP_PARSE_ROUTINE)(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount);
typedef void (cdecl DP_FREE_ROUTINE)(wchar_t **Names, wchar_t **Values, size_t Count);

#define IRPMON_DATA_PARSER_VERSION_1			0x1

typedef struct _IRPMON_DATA_PARSER {
	uint32_t Version;
	uint32_t Size;
	const wchar_t *Name;
	const wchar_t *Description;
	uint32_t MajorVersion;
	uint32_t MinorVersion;
	uint32_t BuildVersion;
	uint32_t Priority;
	DP_PARSE_ROUTINE *ParseRoutine;
	DP_FREE_ROUTINE *FreeRoutine;
} IRPMON_DATA_PARSER, *PIRPMON_DATA_PARSER;

typedef IRPMON_DATA_PARSER IRPMON_DATA_PARSER_V1, *PIRPMON_DATA_PARSER_V1;

typedef DWORD (cdecl DP_INIT_ROUTINE)(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);



#endif
