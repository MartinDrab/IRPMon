
#include <stdint.h>
#include <windows.h>
#include <strsafe.h>
#include "general-types.h"
#include "data-parser-types.h"
#include "parser-base.h"



DWORD PBaseAddNameValue(PNV_PAIR Pair, const wchar_t *Name, const wchar_t *Value)
{
	DWORD ret = ERROR_GEN_FAILURE;
	wchar_t *tmpName = NULL;
	wchar_t *tmpValue = NULL;
	size_t nameLen = 0;
	size_t valueLen = 0;
	size_t totalLen = 0;
	wchar_t **tmp = NULL;

	ret = StringCbLengthW(Name, STRSAFE_MAX_CCH, &nameLen);
	if (ret == S_OK) {
		ret = StringCbLengthW(Value, STRSAFE_MAX_CCH, &valueLen);
		if (ret == S_OK) {
			totalLen = nameLen + 1 + valueLen;
			tmpName = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (totalLen + 1) * sizeof(wchar_t));
			if (tmpName != NULL) {
				tmpValue = tmpName + nameLen + 1;
				CopyMemory(tmpName, Name, nameLen * sizeof(wchar_t));
				CopyMemory(tmpValue, Value, valueLen * sizeof(wchar_t));
				tmp = (wchar_t **)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2 * (Pair->Count + 1) * sizeof(wchar_t *));
				if (tmp != NULL) {
					CopyMemory(tmp, Pair->Names, Pair->Count * sizeof(wchar_t *));
					tmp[Pair->Count] = tmpName;
					CopyMemory(tmp + Pair->Count + 1, Pair->Values, Pair->Count * sizeof(wchar_t *));
					tmp[Pair->Count * 2 + 1] = tmpValue;
					if (Pair->Count > 0)
						HeapFree(GetProcessHeap(), 0, Pair->Names);

					Pair->Names = tmp;
					Pair->Values = tmp + Pair->Count + 1;
					++Pair->Count;
				}
				else ret = GetLastError();

				if (ret != ERROR_SUCCESS)
					HeapFree(GetProcessHeap(), 0, tmpName);
			}
			else ret = GetLastError();
		}
	}

	return ret;
}


DWORD PBaseAddNameFormat(PNV_PAIR Pair, const wchar_t *Name, const wchar_t *Format, ...)
{
	wchar_t buf[1024];
	DWORD ret = ERROR_GEN_FAILURE;
	va_list args;

	va_start(args, Format);
	RtlSecureZeroMemory(buf, sizeof(buf));
	ret = StringCbVPrintf(buf, sizeof(buf) / sizeof(buf[0]), Format, args);
	if (ret == S_OK)
		ret = PBaseAddNameValue(Pair, Name, buf);

	va_end(args);

	return ret;
}


DWORD PBaseAddBooleanValue(PNV_PAIR Pair, const wchar_t *Name, BOOLEAN Value, BOOLEAN HideZeroValues)
{
	DWORD ret = ERROR_GEN_FAILURE;
	const wchar_t *boolValues[] = { L"false", L"true" };

	ret = ERROR_SUCCESS;
	if (Value)
		Value = TRUE;

	if (!HideZeroValues || Value)
		ret = PBaseAddNameValue(Pair, Name, boolValues[Value]);

	return ret;
}


DWORD PBaseAddFlags(PNV_PAIR Pair, uint32_t Flags, const uint32_t *FlagBits, const wchar_t **FlagNames, size_t FlagCount, BOOLEAN HideZeroValues)
{
	DWORD ret = ERROR_SUCCESS;

	for (size_t i = 0; i < FlagCount; ++i) {
		ret = PBaseAddBooleanValue(Pair, FlagNames[i], (Flags & FlagBits[i]), HideZeroValues);
		if (ret != ERROR_SUCCESS)
			break;
	}

	return ret;
}


void PBaseFreeNameValue(wchar_t **Names, wchar_t **Values, size_t Count)
{
	for (size_t i = 0; i < Count; ++i)
		HeapFree(GetProcessHeap(), 0, Names[i]);

	HeapFree(GetProcessHeap(), 0, Names);

	return;
}



DWORD PBaseDataParserAlloc(uint32_t Version, PIRPMON_DATA_PARSER *Parser)
{
	uint32_t parserSize = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	PIRPMON_DATA_PARSER tmpParser = NULL;

	switch (Version) {
		case IRPMON_DATA_PARSER_VERSION_1:
			parserSize = sizeof(IRPMON_DATA_PARSER_V1);
			ret = ERROR_SUCCESS;
			break;
		case IRPMON_DATA_PARSER_VERSION_2:
			parserSize = sizeof(IRPMON_DATA_PARSER_V2);
			ret = ERROR_SUCCESS;
			break;
		default:
			ret = ERROR_INVALID_PARAMETER;
			break;
	}

	if (ret == ERROR_SUCCESS) {
		tmpParser = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, parserSize);
		if (tmpParser != NULL) {
			tmpParser->Version = Version;
			tmpParser->Size = parserSize;
			*Parser = tmpParser;
		} else ret = GetLastError();
	}

	return ret;
}


void PBaseDataParserFree(PIRPMON_DATA_PARSER Parser)
{
	HeapFree(GetProcessHeap(), 0, Parser);

	return;
}
