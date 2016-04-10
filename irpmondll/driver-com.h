
#ifndef __IRPMONDLL_DRIVER_COM_H__
#define __IRPMONDLL_DRIVER_COM_H__

#include <windows.h>


DWORD DriverComHookDriver(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings, PHANDLE HookHandle);
DWORD DriverComHookedDriverSetInfo(HANDLE Driverhandle, PDRIVER_MONITOR_SETTINGS Settings);
DWORD DriverComHookedDriverGetInfo(HANDLE Driverhandle, PDRIVER_MONITOR_SETTINGS Settings, PBOOLEAN MonitoringEnabled);
DWORD DriverComHookedDriverActivate(HANDLE DriverHandle, BOOLEAN Activate);
DWORD DriverComUnhookDriver(HANDLE HookHandle);

DWORD DriverComConnect(HANDLE hSemaphore);
DWORD DriverComDisconnect(VOID);
DWORD DriverComGetRequest(PREQUEST_HEADER Request, DWORD Size);

DWORD DriverComHookDeviceByName(PWCHAR DeviceName, PHANDLE HookHandle);
DWORD DriverComHookDeviceByAddress(PVOID DeviceObject, PHANDLE HookHandle);
DWORD DriverComDeviceGetInfo(HANDLE DeviceHandle, PUCHAR IRPSettings, PUCHAR FastIoSettings, PBOOLEAN MonitoringEnabled);
DWORD DriverComDeviceSetInfo(HANDLE DeviceHandle, PUCHAR IRPSettings, PUCHAR FastIoSettings, BOOLEAN MonitoringEnabled);
DWORD DriverComUnhookDevice(HANDLE HookHandle);

DWORD DriverComSnapshotRetrieve(PIRPMON_DRIVER_INFO **DriverInfo, PULONG InfoCount);
VOID DriverComSnapshotFree(PIRPMON_DRIVER_INFO *DriverInfo, ULONG Count);

DWORD DriverComHookedObjectsEnumerate(PHOOKED_DRIVER_UMINFO *Info, PULONG Count);
VOID DriverComHookedObjectsFree(PHOOKED_DRIVER_UMINFO Info, ULONG Count);

DWORD DriverComDriverOpen(PVOID ID, PHANDLE Handle);
DWORD DriverComDriverHandleClose(HANDLE Handle);
DWORD DriverComDeviceOpen(PVOID ID, PHANDLE Handle);
DWORD DriverComDeviceHandleClose(HANDLE Handle);

DWORD DriverComModuleInit(VOID);
VOID DriverComModuleFinit(VOID);


#endif 
