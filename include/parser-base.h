
#ifndef __PARSER_BASE_H__
#define __PARSER_BASE_H__




typedef struct _NV_PAIR {
	wchar_t **Names;
	wchar_t **Values;
	size_t Count;
} NV_PAIR, *PNV_PAIR;




DWORD PBaseAddNameValue(PNV_PAIR Pair, const wchar_t *Name, const wchar_t *Value);
DWORD PBaseAddNameFormat(PNV_PAIR Pair, const wchar_t *Name, const wchar_t *Format, ...);
DWORD PBaseAddBooleanValue(PNV_PAIR Pair, const wchar_t *Name, BOOLEAN Value, BOOLEAN HideZeroValues);
void PBaseFreeNameValue(wchar_t **Names, wchar_t **Values, size_t Count);



#endif
