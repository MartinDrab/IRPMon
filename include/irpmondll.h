
/**
 * @file
 *
 * Exports an interface to the irpmondll.dll library. The library forms an user
 * mode part of the system for monitoring various driver and device-related events.
 *
 *  HOW TO USE
 *
 *  *  Initialize the library by a call to the @link(IRPMonDllInitialize) routine.
 *     The current process is connected to the IRPMon driver and the library is now 
 *     ready to work.
 *  *  Retrieve information about drivers and devices currently present in the system
 *     (@link(IRPMonDllSnapshotRetrieve)). The retrieved data must be released by the
 *     @link(IRPMonDllSnapshotFree) procedure when no longer needed.
 *  *  Retrieve the list of drivers and devices currently monitored (and hooked) by the
 *     IRPMon driver. The @link(IRPMonDllDriverHooksEnumerate) does this. Free the returned
 *     information by using the @link(IRPMonDllDriverHooksFree) procedure. The "HookHandle"
 *     member of the structures describing the hooked objects can be used in library functions
 *     described below.
 *  *  Hook a new driver by specifying its name to the @link(IRPMonDllHookDriver) function. This
 *     call instructs the IRPMon driver to store information about the target driver within its
 *     data structures. The actual monitoring must be started by the @link(IRPMonDllDriverStartMonitoring) 
 *     function. @link(IRPMonDllDriverStopMonitoring) stops the monitoring. 
 *  *  Unhook a given driver by passing its hook handle to the @link(IRPMonDllUnhookDriver) function.
 *     The monitoring must not be active. Otherwise, the function fails. 
 *  *  Use the @link(IRPMonDllDriverSetInfo) to change monitoring settings of a hooked driver. If the monitoring
 *     is active, only the value of the MonitorNewDevices setting is propagated to the IRPMon driver.
 *  *  Determine which device objects of a driver should be monitored. Use @link(IRPMonDllHookDeviceByName),
 *     @link(IRPMonDllHookDeviceByAddress) and @link(IRPMonDllUnhookDevice) to tell this to the IRPMon driver.
 * 
 *   COLLECTIONG EVENTS
 *
 *   * Use the @link(IRPMonDllConnect) to connect the current process to the IRPMon Event Queue. You can supply
 *     a semaphore object the counter of which is increased by the IRPMon driver every time an event is added to
 *     the queue. During initialization of the connection, the driver increments the counter by a number of events
 *     currently stored in the queue. 
 *     At most one process can be connected to the queue at any moment of time.
 *   * Disconnect the process by calling the @link(IRPMonDllDisconnect) function.
 *   * Retrieve individual events from the queue via the @link(IRPMonDllGetRequest)
 *     function.
 */

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

IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateDriverDetected(void *DriverObject, const wchar_t *DriverName, PREQUEST_DRIVER_DETECTED *Request);
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateDeviceDetected(void *DriverObject, void *DeviceObject, const wchar_t *DeviceName, PREQUEST_DEVICE_DETECTED *Request);
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateFileNameAssigned(void *FileObject, const wchar_t *FileName, PREQUEST_FILE_OBJECT_NAME_ASSIGNED *Request);
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateFileNameDeleted(void *FileObject, PREQUEST_FILE_OBJECT_NAME_DELETED *Request);
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateProcessCreated(HANDLE ProcessId, HANDLE ParentId, const wchar_t *ImageName, const wchar_t *CommandLine, PREQUEST_PROCESS_CREATED *Request);
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateProcessExitted(HANDLE ProcessId, PREQUEST_PROCESS_EXITTED *Request);
IRPMONDLL_API PREQUEST_HEADER WINAPI IRPMonDllRequestCopy(const REQUEST_HEADER *Header);
IRPMONDLL_API PREQUEST_HEADER WINAPI IRPMonDllRequestMemoryAlloc(size_t Size);
IRPMONDLL_API void WINAPI IRPMonDllRequestMemoryFree(PREQUEST_HEADER Header);

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
