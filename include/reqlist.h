
#ifndef __REQLIST_H__
#define __REQLIST_H__

#include <windows.h>
#include "general-types.h"


typedef enum _ERequestListObjectType {
	rlotDriver,
	rlotDevice,
	rlotFile,
	rlotProcess,
} ERequestListObjectType, *PERequestListObjectType;


typedef void (cdecl REQUEST_LIST_CALLBACK)(PREQUEST_HEADER Request, HANDLE RequestHandle, void *Context, PBOOLEAN Store);

#ifdef __cplusplus
extern "C" {
#endif

DWORD ReqListCreate(PHANDLE List);
void ReqListFree(HANDLE List);
void ReqListAssignParserList(HANDLE List, HANDLE Parsers);
DWORD ReqListAdd(HANDLE List, PREQUEST_HEADER Request);
void ReqListClear(HANDLE List);
DWORD ReqListGetObjectName(HANDLE List, void *Object, ERequestListObjectType Type, wchar_t **Name);
void ReqListFreeObjectName(wchar_t *Name);
DWORD ReqListSetCallback(HANDLE List, REQUEST_LIST_CALLBACK *Routine, void *Context);
void ReqListUnregisterCallback(HANDLE List);
DWORD ReqListSave(HANDLE List, ERequestLogFormat Format, const wchar_t *FileName);
DWORD ReqListLoad(HANDLE List, const wchar_t *FileName);
void ReqListSetSymStore(HANDLE List, HANDLE SymStore);
DWORD RequestToStream(HANDLE RequestHandle, ERequestLogFormat Format, HANDLE Parsers, HANDLE SymStore, HANDLE Stream);

DWORD ReqListModuleInit(const wchar_t *LibraryName);
void ReqListModuleFinit(void);

#ifdef __cplusplus
}
#endif


#endif
