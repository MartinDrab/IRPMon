
#ifndef __MULTISTRING_H__
#define __MULTISTRING_H__



#include <ntifs.h>
#include "kbase-exports.h"



KBASE_API
size_t _MultiStringSize(const wchar_t *Data);
KBASE_API
BOOLEAN _MultiStringExists(wchar_t *Data, PUNICODE_STRING Name, wchar_t **Position);
KBASE_API
BOOLEAN _MultiStringInsert(wchar_t *Data, PUNICODE_STRING Name, BOOLEAN Beginning, size_t *NewLength);
KBASE_API
BOOLEAN _MultiStringRemove(wchar_t *Data, PUNICODE_STRING Name, size_t *NewSize);
KBASE_API
BOOLEAN _MultiStringFirst(wchar_t *MultiString, wchar_t **Item);
KBASE_API
BOOLEAN _MultiStringNext(wchar_t *Item, wchar_t **Next);



#endif
