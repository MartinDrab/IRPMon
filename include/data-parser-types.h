
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
	PWCHAR ProcessName;
	ERequestLogFormat Format;
} DP_REQUEST_EXTRA_INFO, *PDP_REQUEST_EXTRA_INFO;

typedef DWORD (cdecl DP_PARSE_ROUTINE)(const REQUEST_HEADER *Request, const DP_REQUEST_EXTRA_INFO *ExtraInfo, PBOOLEAN Handled, wchar_t ***Names, wchar_t ***Values, size_t *RowCount);
typedef void (cdecl DP_FREE_ROUTINE)(wchar_t **Names, wchar_t **Values, size_t Count);

#define DP_OPTION_TYPE_BOOLEAN					0x1
#define DP_OPTION_TYPE_STRING					0x2
#define DP_OPTION_TYPE_BINARY					0x3
#define DP_OPTION_TYPE_INT						0x4

typedef struct _DP_OPTION_RECORD {
	wchar_t *Name;
	uint32_t Type;
} DP_OPTION_RECORD, *PDP_OPTION_RECORD;

typedef DWORD(cdecl DP_QUERY_OPTION)(const wchar_t *Name, const void **Data, size_t *Length);
typedef DWORD(cdecl DP_SET_OPTION)(const wchar_t *Name, const void *Data, size_t Length);
typedef DWORD(cdecl DP_ENUM_OPTION)(PDP_OPTION_RECORD *Options, size_t *Count);
typedef void(cdecl DP_FREE_OPTION)(PDP_OPTION_RECORD Options, size_t Count);


#define IRPMON_DATA_PARSER_VERSION_1			0x1
#define IRPMON_DATA_PARSER_VERSION_2			0x2

typedef struct _IRPMON_DATA_PARSER_V1 {
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
} IRPMON_DATA_PARSER_V1, *PIRPMON_DATA_PARSER_V1;

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
	DP_ENUM_OPTION *OptionEnumRoutine;
	DP_FREE_OPTION *OptionEnumFreeRoutine;
	DP_QUERY_OPTION *OptionQueryRoutine;
	DP_SET_OPTION *OptionSetRoutine;
} IRPMON_DATA_PARSER, *PIRPMON_DATA_PARSER;

typedef IRPMON_DATA_PARSER IRPMON_DATA_PARSER_V2, *PIRPMON_DATA_PARSER_V2;

typedef DWORD (cdecl DP_INIT_ROUTINE)(uint32_t RequestedVersion, PIRPMON_DATA_PARSER *Parser);



#endif
