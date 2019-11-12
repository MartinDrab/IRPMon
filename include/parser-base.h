
#ifndef __PARSER_BASE_H__
#define __PARSER_BASE_H__


#include <stdint.h>
#include <windows.h>
#include "data-parser-types.h"



typedef struct _NV_PAIR {
	wchar_t **Names;
	wchar_t **Values;
	size_t Count;
} NV_PAIR, *PNV_PAIR;




DWORD PBaseAddNameValue(PNV_PAIR Pair, const wchar_t *Name, const wchar_t *Value);
DWORD PBaseAddNameFormat(PNV_PAIR Pair, const wchar_t *Name, const wchar_t *Format, ...);
DWORD PBaseAddBooleanValue(PNV_PAIR Pair, const wchar_t *Name, BOOLEAN Value, BOOLEAN HideZeroValues);
DWORD PBaseAddFlags(PNV_PAIR Pair, uint32_t Flags, const uint32_t *FlagBits, const wchar_t **FlagNames, size_t FlagCount, BOOLEAN HideZeroValues);
void PBaseFreeNameValue(wchar_t **Names, wchar_t **Values, size_t Count);

DWORD PBaseDataParserAlloc(uint32_t Version, PIRPMON_DATA_PARSER *Parser);
void PBaseDataParserFree(PIRPMON_DATA_PARSER Parser);



#endif
