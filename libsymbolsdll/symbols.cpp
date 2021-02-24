
#include <windows.h>
#include "general-types.h"
#include "symbols.h"



typedef DWORD (cdecl SYMSTORECREATE)(const wchar_t *SymPath, PHANDLE Handle);
typedef void (cdecl SYMSTOREFREE)(HANDLE Handle);
typedef DWORD (cdecl SYMSTOREADDFILE)(HANDLE Handle, const wchar_t *FileName);
typedef DWORD(cdecl SYMSTOREADDDIRECTORY)(HANDLE Handle, const wchar_t *DirName, const wchar_t *Mask);
typedef DWORD (cdecl SYMSTORESETSYMPATH)(HANDLE Handle, const wchar_t *SymPath);
typedef DWORD (cdecl SYMSTORETRANSLATE)(HANDLE Handle, const wchar_t* ModuleName, size_t Offset, PSYM_TRANSLATION Translation);
typedef void (cdecl SYMSTORETRANSLATIONFREE)(PSYM_TRANSLATION Translation);


static HMODULE _hLibrary = NULL;
static SYMSTORECREATE *_SymStoreCreate = NULL;
static SYMSTOREFREE *_SymStoreFree = NULL;
static SYMSTOREADDFILE *_SymStoreAddFile = NULL;
static SYMSTOREADDDIRECTORY *_SymStoreAddDirectory = NULL;
static SYMSTORESETSYMPATH *_SymStoreSetSymPath = NULL;
static SYMSTORETRANSLATE *_SymStoreTranslate = NULL;
static SYMSTORETRANSLATIONFREE *_SymStoreTranslationFree = NULL;


#define PREPARE_ROUTINE(aLibrary, aRoutine)	do {\
	_##aRoutine = (decltype(_##aRoutine))GetProcAddress(aLibrary, #aRoutine);	\
	if (_##aRoutine == NULL) {	\
		FreeLibrary(aLibrary);	\
		return ERROR_PROC_NOT_FOUND;	\
	}	\
	} while (FALSE)	\



extern "C" DWORD SymStoreCreate(const wchar_t *SymPath, PHANDLE Handle)
{
	return _SymStoreCreate(SymPath, Handle);
}


extern "C" void SymStoreFree(HANDLE Handle)
{
	_SymStoreFree(Handle);

	return;
}


extern "C" DWORD SymStoreAddFile(HANDLE Handle, const wchar_t *FileName)
{
	return _SymStoreAddFile(Handle, FileName);
}


extern "C" DWORD SymStoreAddDirectory(HANDLE Handle, const wchar_t *DirName, const wchar_t *Mask)
{
	return _SymStoreAddDirectory(Handle, DirName, Mask);
}


extern "C" DWORD SymStoreSetSymPath(HANDLE Handle, const wchar_t *SymPath)
{
	return _SymStoreSetSymPath(Handle, SymPath);
}


extern "C" DWORD SymStoreTranslate(HANDLE Handle, const wchar_t* ModuleName, size_t Offset, PSYM_TRANSLATION Translation)
{
	return _SymStoreTranslate(Handle, ModuleName, Offset, Translation);
}


extern "C" void SymStoreTranslationFree(PSYM_TRANSLATION Translation)
{
	_SymStoreTranslationFree(Translation);

	return;
}


extern "C" DWORD SymbolsModuleInit(const wchar_t *LibraryName)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = 0;
	_hLibrary = LoadLibraryW(LibraryName);
	if (_hLibrary != NULL) {
		PREPARE_ROUTINE(_hLibrary, SymStoreCreate);
		PREPARE_ROUTINE(_hLibrary, SymStoreFree);
		PREPARE_ROUTINE(_hLibrary, SymStoreAddFile);
		PREPARE_ROUTINE(_hLibrary, SymStoreAddDirectory);
		PREPARE_ROUTINE(_hLibrary, SymStoreSetSymPath);
		PREPARE_ROUTINE(_hLibrary, SymStoreTranslate);
		PREPARE_ROUTINE(_hLibrary, SymStoreTranslationFree);
	} else ret = GetLastError();

	return ret;
}


extern "C" void SymbolsModuleFinit(void)
{
	FreeLibrary(_hLibrary);

	return;
}
