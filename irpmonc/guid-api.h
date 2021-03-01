
#ifndef __GUID_API_H__
#define __GUID_API_H__


#include <windows.h>



#ifdef __cplusplus
extern "C" {
#endif

int GAGUIDToStringW(const GUID *G, wchar_t *Buffer, size_t MaxCount);
int GAGUIDToStringA(const GUID *G, char *Buffer, size_t MaxCount);
int GAStringToGUIDW(const wchar_t *S, GUID *G);
int GAStringToGUIDA(const char *S, GUID *G);

int GUIDApiInit(void);

#ifdef __cplusplus
}
#endif



#endif
