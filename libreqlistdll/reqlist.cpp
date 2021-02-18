
#include <windows.h>
#include "general-types.h"
#include "reqlist.h"


typedef DWORD (cdecl REQLISTCREATE)(PHANDLE List);
typedef void (cdecl REQLISTFREE)(HANDLE List);
typedef void (cdecl REQLISTASSIGNPARSERLIST)(HANDLE List, HANDLE Parsers);
typedef DWORD (cdecl REQLISTADD)(HANDLE List, PREQUEST_HEADER Request);
typedef void (cdecl REQLISTCLEAR)(HANDLE List);
typedef DWORD (cdecl REQLISTGETOBJECTNAME)(HANDLE List, void* Object, ERequestListObjectType Type, wchar_t** Name);
typedef void (cdecl REQLISTFREEOBJECTNAME)(wchar_t* Name);
typedef DWORD (cdecl REQLISTSETCALLBACK)(HANDLE List, REQUEST_LIST_CALLBACK* Routine, void* Context);
typedef void (cdecl REQLISTUNREGISTERCALLBACK)(HANDLE List);
typedef DWORD (cdecl REQLISTSAVE)(HANDLE List, ERequestLogFormat Format, const wchar_t* FileName);
typedef DWORD (cdecl REQLISTLOAD)(HANDLE List, const wchar_t* FileName);
typedef void (cdecl REQLISTSETSYMSTORE)(HANDLE List, HANDLE SymStore);
typedef DWORD (cdecl REQUESTTOSTREAM)(HANDLE RequestHandle, ERequestLogFormat Format, HANDLE Parsers, HANDLE SymStore, HANDLE Stream);


static HMODULE _hLibrary = NULL;
static REQLISTCREATE *_ReqListCreate = NULL;
static REQLISTFREE *_ReqListFree = NULL;
static REQLISTASSIGNPARSERLIST *_ReqListAssignParserList = NULL;
static REQLISTADD * _ReqListAdd = NULL;
static REQLISTCLEAR *_ReqListClear = NULL;
static REQLISTGETOBJECTNAME *_ReqListGetObjectName = NULL;
static REQLISTFREEOBJECTNAME *_ReqListFreeObjectName = NULL;
static REQLISTSETCALLBACK *_ReqListSetCallback = NULL;
static REQLISTUNREGISTERCALLBACK *_ReqListUnregisterCallback = NULL;
static REQLISTSAVE *_ReqListSave = NULL;
static REQLISTLOAD *_ReqListLoad = NULL;
static REQLISTSETSYMSTORE *_ReqListSetSymStore = NULL;
static REQUESTTOSTREAM *_RequestToStream = NULL;


#define PREPARE_ROUTINE(aLibrary, aRoutine)	do {\
	_##aRoutine = (decltype(_##aRoutine))GetProcAddress(aLibrary, #aRoutine);	\
	if (_##aRoutine == NULL) {	\
		FreeLibrary(aLibrary);	\
		return ERROR_PROC_NOT_FOUND;	\
	}	\
	} while (FALSE)	\



extern "C" DWORD ReqListCreate(PHANDLE List)
{
	return _ReqListCreate(List);
}


extern "C" void ReqListFree(HANDLE List)
{
	_ReqListFree(List);

	return;
}


extern "C" void ReqListAssignParserList(HANDLE List, HANDLE Parsers)
{
	_ReqListAssignParserList(List, Parsers);

	return;
}


extern "C" DWORD ReqListAdd(HANDLE List, PREQUEST_HEADER Request)
{
	return _ReqListAdd(List, Request);
}


void ReqListClear(HANDLE List)
{
	_ReqListClear(List);

	return;
}


extern "C" DWORD ReqListGetObjectName(HANDLE List, void* Object, ERequestListObjectType Type, wchar_t **Name)
{
	return _ReqListGetObjectName(List, Object, Type, Name);
}


extern "C" void ReqListFreeObjectName(wchar_t *Name)
{
	_ReqListFreeObjectName(Name);

	return;
}


extern "C" DWORD ReqListSetCallback(HANDLE List, REQUEST_LIST_CALLBACK *Routine, void *Context)
{
	return _ReqListSetCallback(List, Routine, Context);
}


extern "C" void ReqListUnregisterCallback(HANDLE List)
{
	_ReqListUnregisterCallback(List);

	return;
}


extern "C" DWORD ReqListSave(HANDLE List, ERequestLogFormat Format, const wchar_t *FileName)
{
	return _ReqListSave(List, Format, FileName);
}


extern "C" DWORD ReqListLoad(HANDLE List, const wchar_t *FileName)
{
	return _ReqListLoad(List, FileName);
}


extern "C" void ReqListSetSymStore(HANDLE List, HANDLE SymStore)
{
	_ReqListSetSymStore(List, SymStore);

	return;
}


extern "C" DWORD RequestToStream(HANDLE RequestHandle, ERequestLogFormat Format, HANDLE Parsers, HANDLE SymStore, HANDLE Stream)
{
	return _RequestToStream(RequestHandle, Format, Parsers, SymStore, Stream);
}


extern "C" DWORD ReqListModuleInit(const wchar_t *LibraryName)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = 0;
	_hLibrary = LoadLibraryW(LibraryName);
	if (_hLibrary != NULL) {
		PREPARE_ROUTINE(_hLibrary, ReqListCreate);
		PREPARE_ROUTINE(_hLibrary, ReqListFree);
		PREPARE_ROUTINE(_hLibrary, ReqListAssignParserList);
		PREPARE_ROUTINE(_hLibrary, ReqListAdd);
		PREPARE_ROUTINE(_hLibrary, ReqListClear);
		PREPARE_ROUTINE(_hLibrary, ReqListGetObjectName);
		PREPARE_ROUTINE(_hLibrary, ReqListFreeObjectName);
		PREPARE_ROUTINE(_hLibrary, ReqListSetCallback);
		PREPARE_ROUTINE(_hLibrary, ReqListUnregisterCallback);
		PREPARE_ROUTINE(_hLibrary, ReqListSave);
		PREPARE_ROUTINE(_hLibrary, ReqListLoad);
		PREPARE_ROUTINE(_hLibrary, ReqListSetSymStore);
		PREPARE_ROUTINE(_hLibrary, RequestToStream);
	} else ret = GetLastError();

	return ret;
}


extern "C" void ReqListModuleFinit(void)
{
	FreeLibrary(_hLibrary);

	return;
}
