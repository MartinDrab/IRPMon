
#ifndef __DPARSER_H__
#define __DPARSER_H__

#include <windows.h>
#include "general-types.h"
#include "data-parser-types.h"


#ifdef __cplusplus
extern "C" {
#endif

DWORD DPListCreate(PHANDLE List);
void DPListFree(HANDLE List);
ULONG DPListGetCount(HANDLE List);
DWORD DPListAddFile(HANDLE List, const wchar_t *FileName);
DWORD DPListAddDirectory(HANDLE List, const wchar_t *Directory);
DWORD DPListUnloadItem(HANDLE List, ULONG Index);
DWORD DPListGetItemInfo(HANDLE List, ULONG Index, PIRPMON_DATA_PARSER Info);
void DPListItemInfoFree(PIRPMON_DATA_PARSER Info);

DWORD DPListModuleInit(const wchar_t* LibraryName);
void DPListModuleFinit(void);

#ifdef __cplusplus
}
#endif


#endif
