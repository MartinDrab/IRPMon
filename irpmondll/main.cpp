
/// <summary>
/// Exports an interface to the irpmondll.dll library. The library forms an user
/// mode part of the system for monitoring various driver and device-related events.
///
/// <h2>How to Use</h2>
///
/// <list type="bullets">
/// <item>
///   Initialize the library by a call to the <see cref="IRPMonDllInitialize"/> routine.
/// </item>
/// <item> 
///   The current process is connected to the IRPMon driver and the library is now
///   ready to work.
/// </item>
/// <item>
///   Retrieve information about drivers and devices currently present in the system
///   (<see cref="IRPMonDllSnapshotRetrieve"/>). The retrieved data must be released by the
///   <see cref="IRPMonDllSnapshotFree"/> procedure when no longer needed.
/// </item>
/// <item> 
///   Retrieve the list of drivers and devices currently monitored (and hooked) by the
///   IRPMon driver. The <see cref="IRPMonDllDriverHooksEnumerate"/> does this. Free the returned
///   information by using the <see cref="IRPMonDllDriverHooksFree"/> procedure. The "HookHandle"
///   member of the structures describing the hooked objects can be used in library functions
///   described below.
/// </item>
/// <item>
///   Hook a new driver by specifying its name to the <see cref="IRPMonDllHookDriver"/> function. This
///   call instructs the IRPMon driver to store information about the target driver within its
///   data structures. The actual monitoring must be started by the <see cref="IRPMonDllDriverStartMonitoring"/>
///   function. <see cref="IRPMonDllDriverStopMonitoring"/> stops the monitoring.
/// </item>
/// <item>
///   Unhook a given driver by passing its hook handle to the <see cref="IRPMonDllUnhookDriver"/> function.
///   The monitoring must not be active. Otherwise, the function fails.
/// </item>
/// <item>
///   Use the <see cref="IRPMonDllDriverSetInfo"/> to change monitoring settings of a hooked driver. If the monitoring
///   is active, only the value of the MonitorNewDevices setting is propagated to the IRPMon driver.
/// </item>
/// <item>
///   Determine which device objects of a driver should be monitored. Use <see cref="IRPMonDllHookDeviceByName"/>,
///   <see cref="IRPMonDllHookDeviceByAddress"/> and <see cref="IRPMonDllUnhookDevice"/> to tell this to the IRPMon driver.
/// </item>
/// </list>
///
/// <h2>Collecting Events</h2>
///
/// <list type="bullets">
/// <item>
///   Use the <see cref="IRPMonDllConnect"/> to connect the current process to the IRPMon Event Queue. You can supply
///   a semaphore object the counter of which is increased by the IRPMon driver every time an event is added to
///   the queue. During initialization of the connection, the driver increments the counter by a number of events
///   currently stored in the queue.
/// </item>
/// <item>
///   At most one process can be connected to the queue at any moment of time.
/// </item>
/// <item>
///   Disconnect the process by calling the <see cref="IRPMonDllDisconnect"/> function.
/// </item>
/// <item>
///   Retrieve individual events from the queue via the <see cref="IRPMonDllGetRequest"/>.
///   function.
/// </item>
/// </list>
/// </summary>

#include <windows.h>
#include "debug.h"
#include "general-types.h"
#include "irpmondll-types.h"
#include "driver-com.h"
#include "irpmondll.h"



/************************************************************************/
/*                           EXPORTED FUNCTIONS                         */
/************************************************************************/


IRPMONDLL_API DWORD WINAPI IRPMonDllGetRequest(PREQUEST_HEADER Request, DWORD Size)
{
	return DriverComGetRequest(Request, Size);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllConnect(void)
{
	return DriverComConnect();
}


IRPMONDLL_API DWORD WINAPI IRPMonDllDisconnect(VOID)
{
	return DriverComDisconnect();
}


IRPMONDLL_API DWORD WINAPI IRPMonDllQueueClear(void)
{
	return DriverComQueueClear();
}


IRPMONDLL_API DWORD WINAPI IRPMonDllHookDriver(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings, BOOLEAN DeviceExtensionHook, PHANDLE DriverHandle, PVOID *ObjectId)
{
	return DriverComHookDriver(DriverName, MonitorSettings, DeviceExtensionHook, DriverHandle, ObjectId);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDriver(HANDLE HookHandle)
{
	return DriverComUnhookDriver(HookHandle);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllSnapshotRetrieve(PIRPMON_DRIVER_INFO **DriverInfo, PULONG InfoCount)
{
	return DriverComSnapshotRetrieve(DriverInfo, InfoCount);
}


IRPMONDLL_API VOID WINAPI IRPMonDllSnapshotFree(PIRPMON_DRIVER_INFO *DriverInfo, ULONG Count)
{
	DriverComSnapshotFree(DriverInfo, Count);

	return;
}


IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByName(PWCHAR DeviceName, PHANDLE HookHandle, PVOID *ObjectId)
{
	return DriverComHookDeviceByName(DeviceName, HookHandle, ObjectId);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByAddress(PVOID DeviceObject, PHANDLE HookHandle, PVOID *ObjectId)
{
	return DriverComHookDeviceByAddress(DeviceObject, HookHandle, ObjectId);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDeviceGetInfo(HANDLE Handle, PUCHAR IRPSettings, PUCHAR FastIOSettings, PBOOLEAN MonitoringEnabled)
{
	return DriverComDeviceGetInfo(Handle, IRPSettings, FastIOSettings, MonitoringEnabled);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDeviceSetInfo(HANDLE Handle, PUCHAR IRPSettings, PUCHAR FastIOSettings, BOOLEAN MonitoringEnabled)
{
	return DriverComDeviceSetInfo(Handle, IRPSettings, FastIOSettings, MonitoringEnabled);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDriverGetInfo(HANDLE Handle, PDRIVER_MONITOR_SETTINGS Settings, PBOOLEAN MonitoringEnabled)
{
	return DriverComHookedDriverGetInfo(Handle, Settings, MonitoringEnabled);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDevice(HANDLE HookHandle)
{
	return DriverComUnhookDevice(HookHandle);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStartMonitoring(HANDLE Driverhandle)
{
	return DriverComHookedDriverActivate(Driverhandle, TRUE);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStopMonitoring(HANDLE Driverhandle)
{
	return DriverComHookedDriverActivate(Driverhandle, FALSE);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllDriverSetInfo(HANDLE DriverHandle, PDRIVER_MONITOR_SETTINGS Settings)
{
	return DriverComHookedDriverSetInfo(DriverHandle, Settings);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDriver(PVOID ObjectId, PHANDLE Handle)
{
	return DriverComDriverOpen(ObjectId, Handle);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDriverHandle(HANDLE Handle)
{
	return DriverComDriverHandleClose(Handle);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDevice(PVOID ObjectId, PHANDLE Handle)
{
	return DriverComDeviceOpen(ObjectId, Handle);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDeviceHandle(HANDLE Handle)
{
	return DriverComDeviceHandleClose(Handle);
}


/************************************************************************/
/*                           HOOKED OBJECTS ENUMERATION                 */
/************************************************************************/


IRPMONDLL_API DWORD WINAPI IRPMonDllDriverHooksEnumerate(PHOOKED_DRIVER_UMINFO *HookedDrivers, PULONG Count)
{
	return DriverComHookedObjectsEnumerate(HookedDrivers, Count);
}


IRPMONDLL_API VOID WINAPI IRPMonDllDriverHooksFree(PHOOKED_DRIVER_UMINFO HookedDrivers, ULONG Count)
{

	DriverComHookedObjectsFree(HookedDrivers, Count);

	return;
}


/************************************************************************/
/*                  CLASS WATCHES                                       */
/************************************************************************/


IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchRegister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	return DriverComClassWatchRegister(ClassGuid, UpperFilter, Beginning);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchUnregister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	return DriverComClassWatchUnregister(ClassGuid, UpperFilter, Beginning);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchEnum(PCLASS_WATCH_RECORD *Array, PULONG Count)
{
	return DriverComClassWatchEnum(Array, Count);
}

IRPMONDLL_API VOID WINAPI IRPMonDllClassWatchEnumFree(PCLASS_WATCH_RECORD Array, ULONG Count)
{
	DriverComClassWatchEnumFree(Array, Count);

	return;
}


/************************************************************************/
/*                  DRIVER NAME WATCHES                                 */
/************************************************************************/


IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchRegister(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings)
{
	return DriverComDriverNameWatchRegister(DriverName, MonitorSettings);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchUnregister(PWCHAR DriverName)
{
	return DriverComDriverNameWatchUnregister(DriverName);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchEnum(PDRIVER_NAME_WATCH_RECORD *Array, PULONG Count)
{
	return DriverComDriverNameWatchEnum(Array, Count);
}


IRPMONDLL_API VOID WINAPI IRPMonDllDriverNameWatchEnumFree(PDRIVER_NAME_WATCH_RECORD Array, ULONG Count)
{
	DriverComDriverNameWatchEnumFree(Array, Count);

	return;
}


IRPMONDLL_API DWORD WINAPI IRPMonDllEmulateDriverDevices(void)
{
	return DriverComEmulateDriverDevices();
}


IRPMONDLL_API DWORD WINAPI IRPMonDllEmulateProcesses(void)
{
	return DriverComEmulateProcesses();
}


IRPMONDLL_API DWORD WINAPI IRPMonDllSettingsQuery(PIRPMNDRV_SETTINGS Settings)
{
	return DriverComSettingsQuery(Settings);
}


IRPMONDLL_API DWORD WINAPI IRPMonDllSettingsSet(PIRPMNDRV_SETTINGS Settings, BOOLEAN Save)
{
	return DriverComSettingsSet(Settings, Save);
}


/************************************************************************/
/*                          INITIALIZATION AND FINALIZATION             */
/************************************************************************/


IRPMONDLL_API BOOL WINAPI IRPMonDllInitialized(VOID)
{
	BOOL ret = FALSE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = DriverComDeviceConnected();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


IRPMONDLL_API DWORD WINAPI IRPMonDllInitialize(const IRPMON_INIT_INFO *Info)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);

	ret = DriverComModuleInit(Info);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


IRPMONDLL_API VOID WINAPI IRPMonDllFinalize(VOID)
{
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	DriverComModuleFinit();

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                          DLL MAIN                                    */
/************************************************************************/


BOOL WINAPI DllMain(_In_  HINSTANCE hinstDLL, _In_  DWORD fdwReason, _In_  LPVOID lpvReserved)
{
	BOOL ret = FALSE;
	DEBUG_ENTER_FUNCTION("hinstDLL=0x%p; fdwReason=%u; lpReserved=0x%p", hinstDLL, fdwReason, lpvReserved);

	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			ret = DisableThreadLibraryCalls(hinstDLL);
			break;
	}

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}
