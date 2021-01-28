
#include <windows.h>
#include "general-types.h"
#include "irpmondll-types.h"
#include "callback-stream.h"



typedef void* (cdecl CALLBACKSTREAMCREATE)(CallbackStreamOnRead* ReadCallback, CallbackStreamOnWrite* WriteCallback, void *ReadContext, void *WriteContext);
typedef void (cdecl CALLBACKSTREAMFREE)(void* Stream);

static HMODULE _hLibrary = NULL;
static CALLBACKSTREAMCREATE *_CallbackStreamCreate = nullptr;
static CALLBACKSTREAMFREE *_CallbackStreamFree = nullptr;


#define PREPARE_ROUTINE(aLibrary, aRoutine)	do {\
	_##aRoutine = (decltype(_##aRoutine))GetProcAddress(aLibrary, #aRoutine);	\
	if (_##aRoutine == NULL) {	\
		FreeLibrary(aLibrary);	\
		return ERROR_PROC_NOT_FOUND;	\
	}	\
	} while (FALSE)	\


extern "C" void *CallbackStreamCreate(CallbackStreamOnRead * ReadCallback, CallbackStreamOnWrite * WriteCallback, void *ReadContext, void *WriteContext)
{
	return _CallbackStreamCreate(ReadCallback, WriteCallback, ReadContext, WriteContext);
}


extern "C" void CallbackStreamFree(void* Stream)
{
	_CallbackStreamFree(Stream);

	return;
}



extern "C" DWORD CallbackStreamModuleInit(const wchar_t *LibraryName)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = 0;
	_hLibrary = LoadLibraryW(LibraryName);
	if (_hLibrary != nullptr) {
		PREPARE_ROUTINE(_hLibrary, CallbackStreamCreate);
		PREPARE_ROUTINE(_hLibrary, CallbackStreamFree);
	} else ret = GetLastError();

	return ret;
}


extern "C" void CallbackStreamModuleFinit(void)
{
	FreeLibrary(_hLibrary);

	return;
}
