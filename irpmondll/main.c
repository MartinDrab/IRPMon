
#include <windows.h>
#include "debug.h"
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


IRPMONDLL_API DWORD WINAPI IRPMonDllConnect(HANDLE hSemaphore)
{
	return DriverComConnect(hSemaphore);
}

IRPMONDLL_API DWORD WINAPI IRPMonDllDisconnect(VOID)
{
	return DriverComDisconnect();
}


IRPMONDLL_API DWORD WINAPI IRPMonDllHookDriver(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings, PHANDLE DriverHandle)
{
	return DriverComHookDriver(DriverName, MonitorSettings, DriverHandle);
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

IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByName(PWCHAR DeviceName, PHANDLE HookHandle)
{
	return DriverComHookDeviceByName(DeviceName, HookHandle);
}

IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByAddress(PVOID DeviceObject, PHANDLE HookHandle)
{
	return DriverComHookDeviceByAddress(DeviceObject, HookHandle);
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
/*                          INITIALIZATION AND FINALIZATION             */
/************************************************************************/

IRPMONDLL_API DWORD WINAPI IRPMonDllInitialize(VOID)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = DriverComModuleInit();

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
