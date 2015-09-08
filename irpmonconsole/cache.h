
#ifndef __IRPMON_CACHE_H__
#define __IRPMON_CACHE_H__



#include <string>
#include <windows.h>


std::wstring CacheDeviceNameGet(PVOID DeviceObject);
std::wstring CacheDriverNameGet(PVOID DriverObject);
DWORD CacheInit(VOID);
VOID CacheFinit(VOID);


#endif 
