
#ifndef __MULTISTRING_H__
#define __MULTISTRING_H__



#include <ntifs.h>



size_t _MultiStringSize(const wchar_t *Data);
BOOLEAN _MultiStringExists(wchar_t *Data, PUNICODE_STRING Name, wchar_t **Position);
BOOLEAN _MultiStringInsert(wchar_t *Data, PUNICODE_STRING Name, BOOLEAN Beginning, size_t *NewLength);
BOOLEAN _MultiStringRemove(wchar_t *Data, PUNICODE_STRING Name, size_t *NewSize);
BOOLEAN _MultiStringFirst(wchar_t *MultiString, wchar_t **Item);
BOOLEAN _MultiStringNext(wchar_t *Item, wchar_t **Next);



#endif
