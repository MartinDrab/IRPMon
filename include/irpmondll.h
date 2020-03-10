 
#ifndef __IRPMONDLL_H__
#define __IRPMONDLL_H__

#include <windows.h>
#include "irpmondll-types.h"




#ifdef IRPMONDLL_EXPORTS

#define IRPMONDLL_API                  EXTERN_C __declspec(dllexport)

#else 

#define IRPMONDLL_API                  EXTERN_C __declspec(dllimport)

#endif



IRPMONDLL_API DWORD WINAPI IRPMonDllDriverHooksEnumerate(PHOOKED_DRIVER_UMINFO *HookedDrivers, PULONG Count);
IRPMONDLL_API VOID WINAPI IRPMonDllDriverHooksFree(PHOOKED_DRIVER_UMINFO HookedDrivers, ULONG Count);
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDriver(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings, BOOLEAN DeviceExtensionHook, PHANDLE DriverHandle, PVOID *ObjectId);
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStartMonitoring(HANDLE Driverhandle);
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStopMonitoring(HANDLE Driverhandle);
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverSetInfo(HANDLE DriverHandle, PDRIVER_MONITOR_SETTINGS Settings);
IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDriver(HANDLE DriverHandle);
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByName(PWCHAR DeviceName, PHANDLE HookHandle, PVOID *ObjectId);
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByAddress(PVOID DeviceObject, PHANDLE HookHandle, PVOID *ObjectId);
IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDevice(HANDLE HookHandle);
IRPMONDLL_API DWORD WINAPI IRPMonDllSnapshotRetrieve(PIRPMON_DRIVER_INFO **DriverInfo, PULONG Count);
IRPMONDLL_API VOID WINAPI IRPMonDllSnapshotFree(PIRPMON_DRIVER_INFO *DriverInfo, ULONG Count);
IRPMONDLL_API DWORD WINAPI IRPMonDllConnect(void);
IRPMONDLL_API DWORD WINAPI IRPMonDllDisconnect(VOID);
IRPMONDLL_API DWORD WINAPI IRPMonDllQueueClear(void);
IRPMONDLL_API DWORD WINAPI IRPMonDllGetRequest(PREQUEST_HEADER Request, DWORD Size);
IRPMONDLL_API size_t WINAPI IRPMonDllGetRequestSize(const REQUEST_HEADER *Request);
IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDriver(PVOID ObjectId, PHANDLE Handle);
IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDriverHandle(HANDLE Handle);
IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDevice(PVOID ObjectId, PHANDLE Handle);
IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDeviceHandle(HANDLE Handle);
IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDeviceGetInfo(HANDLE Handle, PUCHAR IRPSettings, PUCHAR FastIOSettings, PBOOLEAN MonitoringEnabled);
IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDeviceSetInfo(HANDLE Handle, PUCHAR IRPSettings, PUCHAR FastIOSettings, BOOLEAN MonitoringEnabled);
IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDriverGetInfo(HANDLE Handle, PDRIVER_MONITOR_SETTINGS Settings, PBOOLEAN MonitoringEnabled);
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchRegister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning);
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchUnregister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning);
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchEnum(PCLASS_WATCH_RECORD *Array, PULONG Count);
IRPMONDLL_API VOID WINAPI IRPMonDllClassWatchEnumFree(PCLASS_WATCH_RECORD Array, ULONG Count);
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchRegister(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings);
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchUnregister(PWCHAR DriverName);
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchEnum(PDRIVER_NAME_WATCH_RECORD *Array, PULONG Count);
IRPMONDLL_API VOID WINAPI IRPMonDllDriverNameWatchEnumFree(PDRIVER_NAME_WATCH_RECORD Array, ULONG Count);
IRPMONDLL_API DWORD WINAPI IRPMonDllEmulateDriverDevices(void);
IRPMONDLL_API DWORD WINAPI IRPMonDllEmulateProcesses(void);
IRPMONDLL_API DWORD WINAPI IRPMonDllSettingsQuery(PIRPMNDRV_SETTINGS Settings);
IRPMONDLL_API DWORD WINAPI IRPMonDllSettingsSet(PIRPMNDRV_SETTINGS Settings, BOOLEAN Save);

/************************************************************************/
/*           INITIALIZATION AND FINALIZATION                            */
/************************************************************************/

IRPMONDLL_API BOOL WINAPI IRPMonDllInitialized(VOID);
IRPMONDLL_API DWORD WINAPI IRPMonDllInitialize(const IRPMON_INIT_INFO *Info);
IRPMONDLL_API VOID WINAPI IRPMonDllFinalize(VOID);



#endif 
