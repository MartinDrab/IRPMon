
#ifndef __CALLBACK_STREAM_H__
#define __CALLBACK_STREAM_H__


#include "general-types.h"
#include "irpmondll-types.h"


typedef DWORD (cdecl CallbackStreamOnRead)(void *Buffer, ULONG Length, void *Stream, void *Context);
typedef DWORD (cdecl CallbackStreamOnWrite)(const void *Buffer, ULONG ALength, void *Stream, void *Context);


#ifdef __cplusplus
extern "C" {
#endif

void *CallbackStreamCreate(CallbackStreamOnRead *ReadCallback, CallbackStreamOnWrite *WriteCallback, void *ReadContext, void *WriteContext);
void CallbackStreamFree(void *Stream);

DWORD CallbackStreamModuleInit(const wchar_t *LibraryName);
void CallbackStreamModuleFinit(void);

#ifdef __cplusplus
}
#endif


#endif
