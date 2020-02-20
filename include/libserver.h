
#ifndef __LIBSERVER_H__
#define __LIBSERVER_H__



#include <windows.h>



DWORD IRPMonServerStart(const char *Address, const char *Port, HANDLE ExitEvent);



#endif
