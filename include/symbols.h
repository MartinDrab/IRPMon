
#ifndef __PDB_SYMBOLS_H__
#define __PDB_SYMBOLS_H__


#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _SYM_TRANSLATION {
	void *Address;
	wchar_t *ModuleName;
	wchar_t *FunctionName;
	size_t Offset;
} SYM_TRANSLATION, *PSYM_TRANSLATION;


DWORD SymStoreCreate(const wchar_t *SymPath, PHANDLE Handle);
void SymStoreFree(HANDLE Handle);
DWORD SymStoreAddFile(HANDLE Handle, const wchar_t *FileName);
DWORD SymStoreAddDirectory(HANDLE Handle, const wchar_t *DirName, const wchar_t *Mask);
DWORD SymStoreSetSymPath(HANDLE Handle, const wchar_t *SymPath);
DWORD SymStoreTranslate(HANDLE Handle, const wchar_t *ModuleName, size_t Offset, PSYM_TRANSLATION Translation);
void SymStoreTranslationFree(PSYM_TRANSLATION Translation);

DWORD SymbolsModuleInit(const wchar_t *LibraryName);
void SymbolsModuleFinit(void);

#ifdef __cplusplus
}
#endif



#endif
