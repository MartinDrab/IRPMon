
#ifndef __PDB_SYMBOLS_H__
#define __PDB_SYMBOLS_H__


#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif


DWORD SymStoreCreate(const wchar_t *SymPath, PHANDLE Handle);
void SymStoreFree(HANDLE Handle);
DWORD SymStoreAddFile(HANDLE Handle, const wchar_t *FileName);
DWORD SymStoreAddDirectory(HANDLE Handle, const wchar_t *DirName, const wchar_t *Mask);
DWORD SymStoreSetSymPath(HANDLE Handle, const wchar_t *SymPath);

DWORD SymbolsModuleInit(const wchar_t *LibraryName);
void SymbolsModuleFinit(void);

#ifdef __cplusplus
}
#endif



#endif
