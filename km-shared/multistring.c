
#include <ntifs.h>
#include "preprocessor.h"
#include "multistring.h"



/************************************************************************/
/*                 PUBLIC FUNCTIONS                                     */
/************************************************************************/


size_t _MultiStringSize(const wchar_t *Data)
{
	SIZE_T ret = sizeof(wchar_t);
	DEBUG_ENTER_FUNCTION("Data=0x%p", Data);

	while (*Data != L'\0') {
		SIZE_T len = wcslen(Data);

		ret += (len + 1) * sizeof(wchar_t);
		Data += (len + 1);
	}

	DEBUG_EXIT_FUNCTION("%Iu", ret);
	return ret;
}


BOOLEAN _MultiStringExists(wchar_t *Data, PUNICODE_STRING Name, wchar_t **Position)
{
	UNICODE_STRING uData;
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("Data=0x%p; Name=\"%wZ\"", Data, Name);

	while (!ret && *Data != L'\0') {
		RtlInitUnicodeString(&uData, Data);
		ret = RtlEqualUnicodeString(&uData, Name, TRUE);
		if (ret && Position != NULL)
			*Position = Data;

		Data += ((uData.Length / sizeof(wchar_t)) + 1);
	}

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


BOOLEAN _MultiStringInsert(wchar_t *Data, PUNICODE_STRING Name, BOOLEAN Beginning, size_t *NewLength)
{
	SIZE_T dataSize = 0;
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("Data=0x%p; Name=\"%wZ\"; Beginning=%u", Data, Name, Beginning);

	ret = !_MultiStringExists(Data, Name, NULL);
	if (ret) {
		dataSize = _MultiStringSize(Data);
		if (Beginning) {
			memmove(Data + (Name->Length / sizeof(wchar_t)) + 1, Data, dataSize);
			memcpy(Data, Name->Buffer, Name->Length);
			Data[Name->Length / sizeof(wchar_t)] = L'\0';
		}
		else memcpy(Data + (dataSize / sizeof(wchar_t)) - 1, Name->Buffer, Name->Length);

		dataSize += (Name->Length + sizeof(wchar_t));
		*NewLength = dataSize;
	}

	DEBUG_EXIT_FUNCTION("%u, *NewLength=%Iu", ret, *NewLength);
	return ret;
}


BOOLEAN _MultiStringRemove(wchar_t *Data, PUNICODE_STRING Name, size_t *NewSize)
{
	SIZE_T len = 0;
	SIZE_T dataSize = 0;
	wchar_t *pos = NULL;
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("Data=0x%p; Name=\"%wZ\"", Data, Name);

	ret = _MultiStringExists(Data, Name, &pos);
	if (ret) {
		dataSize = _MultiStringSize(Data);
		len = Name->Length + sizeof(wchar_t);
		dataSize -= len;
		memmove(pos, pos + len / sizeof(wchar_t), dataSize - (pos - Data) * sizeof(wchar_t));
		Data[dataSize / sizeof(wchar_t) - 1] = L'\0';
		*NewSize = dataSize;
	}

	DEBUG_EXIT_FUNCTION("%u, *NewSize=%Iu", ret, *NewSize);
	return ret;
}


BOOLEAN _MultiStringFirst(wchar_t *MultiString, wchar_t **Item)
{
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("MultiString=0x%p; Item=0x%p", MultiString, Item);

	*Item = NULL;
	ret = *MultiString != L'\0';
	if (ret)
		*Item = MultiString;

	DEBUG_EXIT_FUNCTION("%u, *Item=\"%S\"", ret, *Item);
	return ret;
}


BOOLEAN _MultiStringNext(wchar_t *Item, wchar_t **Next)
{
	size_t len = 0;
	BOOLEAN ret = FALSE;
	DEBUG_ENTER_FUNCTION("Item=0x%p; Next=0x%p", Item, Next);

	*Next = NULL;
	len = wcslen(Item);
	Item += (len + 1);
	ret = *Item != L'\0';
	if (ret)
		*Next = Item;

	DEBUG_EXIT_FUNCTION("%u, *Next=\"%S\"", ret, *Next);
	return ret;
}
