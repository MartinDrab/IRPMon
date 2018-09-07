
#ifndef __PARSER_BASE_H__
#define __PARSER_BASE_H__


#include <stdint.h>
#include <windows.h>
#include "general-types.h"



typedef struct _PARSER_CONDITION {
	LIST_ENTRY Entry;
	wchar_t *DriverName;
	void *DriverAddress;
	wchar_t *DeviceName;
	void *DeviceAddress;
	ERequesttype RequestType;
	void *ResultValue;
	union {
		struct {
			uint32_t UseDriverName : 1;
			uint32_t UseDriverObject : 1;
			uint32_t UseDeviceName : 1;
			uint32_t UseDeviceObject : 1;
			uint32_t UseRequestType : 1;
			uint32_t SuccessOnly : 1;
			uint32_t UseResultValue : 1;
			uint32_t SpecialCondition : 1;
		} Items;
		uint32_t Value;
	} Flags;
} PARSER_CONDITION, *PPARSER_CONDITION;

typedef struct _PARSER_REQUEST_INFOMRATION {
	wchar_t *DriverName;
	wchar_t *DeviceName;
	union {
		struct {
			uint32_t DriverNamePresent : 1;
			uint32_t DeviceNamePresent : 1;
		} Items;
		uint32_t Value;
	} Values;
} PARSER_REQUEST_INFOMRATION, *PPARSER_REQUEST_INFOMRATION;

typedef DWORD(PARSER_CALLBACK)(const REQUEST_GENERAL *Request, const PARSER_REQUEST_INFOMRATION *Information, HANDLE FieldsHandle, void *Context);

typedef struct _PARSER_PLUGIN_REGISTRATION {
	wchar_t *Name;
	wchar_t *Description;
	void *Context;
} PARSER_PLUGIN_REGISTRATION, *PPARSER_PLUGIN_REGISTRATION;

typedef enum _EParsedFieldType {
	pftUnknown,
	pftInteger,
	pftUnsignedInteger,
	pftPointer,
	pftData,
	pftString,
	pftMax,
} EParsedFieldType, *PEParsedFieldType;

typedef struct _PARSED_FIELD {
	EParsedFieldType ValueType;
	uint32_t ValueOffset;
	uint32_t ValueSize;
	wchar_t *Name;
	wchar_t *description;
} PARSED_FIELD, *PPARSED_FIELD;



#ifdef PARSERBASE_EXPORTS

#define PARSERBASE_API		__declspec(dllexport)

#else

#define PARSERBASE_API		__declspec(dllimport)

#endif


DWORD ParserPluginRegister(const PARSER_PLUGIN_REGISTRATION *Registration, PHANDLE Handle);
void ParserPluginUnregister(HANDLE Handle);
DWORD ParserConditionAdd(HANDLE Handle, const PARSER_CONDITION *Condition);
DWORD ParserParse(const REQUEST_GENERAL *Request, const PARSER_REQUEST_INFOMRATION *Info, PPARSED_FIELD *Fields, size_t *FieldCount);

DWORD ParserFieldsInit(size_t Count, PHANDLE Handle);
DWORD ParserFieldsAdd(HANDLE Handle, const wchar_t *Name, const wchar_t *Description, EParsedFieldType ValueType, uint32_t ValueOffset, size_t ValueSize);
void ParserFieldsDestroy(HANDLE Handle);


#endif
