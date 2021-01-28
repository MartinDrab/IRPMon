
#include <windows.h>
#include "general-types.h"
#include "data-parser-types.h"
#include "dparser.h"


typedef DWORD(cdecl DPLISTCREATE)(PHANDLE List);
typedef void (cdecl DPLISTFREE)(HANDLE List);
typedef ULONG(cdecl DPLISTGETCOUNT)(HANDLE List);
typedef DWORD(cdecl DPLISTADDFILE)(HANDLE List, const wchar_t* FileName);
typedef DWORD(cdecl DPLISTADDDIRECTORY)(HANDLE List, const wchar_t* Directory);
typedef DWORD(cdecl DPLISTUNLOADITEM)(HANDLE List, ULONG Index);
typedef DWORD(cdecl DPLISTGETITEMINFO)(HANDLE List, ULONG Index, PIRPMON_DATA_PARSER Info);
typedef void (cdecl DPLISTITEMINFOFREE)(PIRPMON_DATA_PARSER Info);


static HMODULE _hLibrary = NULL;
static DPLISTCREATE *_DPListCreate = NULL;
static DPLISTFREE *_DPListFree = NULL;
static DPLISTGETCOUNT *_DPListGetCount = NULL;
static DPLISTADDFILE *_DPListAddFile = NULL;
static DPLISTADDDIRECTORY *_DPListAddDirectory = NULL;
static DPLISTUNLOADITEM *_DPListUnloadItem = NULL;
static DPLISTGETITEMINFO *_DPListGetItemInfo = NULL;
static DPLISTITEMINFOFREE *_DPListItemInfoFree = NULL;

#define PREPARE_ROUTINE(aLibrary, aRoutine)	do {\
	_##aRoutine = (decltype(_##aRoutine))GetProcAddress(aLibrary, #aRoutine);	\
	if (_##aRoutine == NULL) {	\
		FreeLibrary(aLibrary);	\
		return ERROR_PROC_NOT_FOUND;	\
	}	\
	} while (FALSE)	\



extern "C" DWORD DPListCreate(PHANDLE List)
{
	return _DPListCreate(List);
}


extern "C" void DPListFree(HANDLE List)
{
	_DPListFree(List);
	
	return;
}


extern "C" ULONG DPListGetCount(HANDLE List)
{
	return _DPListGetCount(List);
}


extern "C" DWORD DPListAddFile(HANDLE List, const wchar_t* FileName)
{
	return _DPListAddFile(List, FileName);
}


extern "C" DWORD DPListAddDirectory(HANDLE List, const wchar_t* Directory)
{
	return _DPListAddDirectory(List, Directory);
}


extern "C" DWORD DPListUnloadItem(HANDLE List, ULONG Index)
{
	return _DPListUnloadItem(List, Index);
}


extern "C" DWORD DPListGetItemInfo(HANDLE List, ULONG Index, PIRPMON_DATA_PARSER Info)
{
	return _DPListGetItemInfo(List, Index, Info);
}


extern "C" void DPListItemInfoFree(PIRPMON_DATA_PARSER Info)
{
	_DPListItemInfoFree(Info);

	return;
}




extern "C" DWORD DPListModuleInit(const wchar_t *LibraryName)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = 0;
	_hLibrary = LoadLibraryW(LibraryName);
	if (_hLibrary != NULL) {
		PREPARE_ROUTINE(_hLibrary, DPListCreate);
		PREPARE_ROUTINE(_hLibrary, DPListFree);
		PREPARE_ROUTINE(_hLibrary, DPListGetCount);
		PREPARE_ROUTINE(_hLibrary, DPListAddFile);
		PREPARE_ROUTINE(_hLibrary, DPListAddDirectory);
		PREPARE_ROUTINE(_hLibrary, DPListUnloadItem);
		PREPARE_ROUTINE(_hLibrary, DPListGetItemInfo);
		PREPARE_ROUTINE(_hLibrary, DPListItemInfoFree);
	} else ret = GetLastError();

	return ret;
}



extern "C" void DPListModuleFinit(void)
{
	FreeLibrary(_hLibrary);

	return;
}
