
#ifndef __IRPMONDLL_DRIVER_COM_H__
#define __IRPMONDLL_DRIVER_COM_H__

#include <windows.h>
#include "general-types.h"
#include "kernel-shared.h"
#include "irpmondll-types.h"


DWORD DriverComHookDriver(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings, BOOLEAN DeviceExtensionHook, PHANDLE HookHandle, PVOID *ObjectId);
DWORD DriverComHookedDriverSetInfo(HANDLE Driverhandle, PDRIVER_MONITOR_SETTINGS Settings);
DWORD DriverComHookedDriverGetInfo(HANDLE Driverhandle, PDRIVER_MONITOR_SETTINGS Settings, PBOOLEAN MonitoringEnabled);
DWORD DriverComHookedDriverActivate(HANDLE DriverHandle, BOOLEAN Activate);
DWORD DriverComUnhookDriver(HANDLE HookHandle);

DWORD DriverComConnect(void);
DWORD DriverComDisconnect(VOID);
DWORD DriverComGetRequest(PREQUEST_HEADER Request, DWORD Size);
DWORD DriverComQueueClear(void);

DWORD DriverComHookDeviceByName(PWCHAR DeviceName, PHANDLE HookHandle, PVOID *ObjectId);
DWORD DriverComHookDeviceByAddress(PVOID DeviceObject, PHANDLE HookHandle, PVOID *ObjectId);
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

DWORD DriverComClassWatchRegister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning);
DWORD DriverComClassWatchUnregister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning);
DWORD DriverComClassWatchEnum(PCLASS_WATCH_RECORD *Array, PULONG Count);
VOID DriverComClassWatchEnumFree(PCLASS_WATCH_RECORD Array, ULONG Count);

DWORD DriverComDriverNameWatchRegister(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings);
DWORD DriverComDriverNameWatchUnregister(PWCHAR DriverName);
DWORD DriverComDriverNameWatchEnum(PDRIVER_NAME_WATCH_RECORD *Array, PULONG Count);
VOID DriverComDriverNameWatchEnumFree(PDRIVER_NAME_WATCH_RECORD Array, ULONG Count);

DWORD _SynchronousNoIOIOCTL(DWORD Code);
DWORD _SynchronousWriteIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength);
DWORD _SynchronousReadIOCTL(DWORD Code, PVOID OutputBuffer, ULONG OutputBufferLength);
DWORD _SynchronousOtherIOCTL(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
DWORD _SynchronousVariableOutputIOCTL(ULONG Code, PVOID InputBuffer, ULONG InputBufferLength, ULONG InitialSize, PVOID *OutputBuffer, PULONG OutputBufferLength);

DWORD DriverComEmulateDriverDevices(void);
DWORD DriverComEmulateProcesses(void);

DWORD DriverComSettingsQuery(PIRPMNDRV_SETTINGS Settings);
DWORD DriverComSettingsSet(PIRPMNDRV_SETTINGS Settings, BOOLEAN Save);

DWORD DriverComModuleInit(const IRPMON_INIT_INFO *Info);
VOID DriverComModuleFinit(VOID);
BOOL DriverComDeviceConnected(VOID);




#endif 
