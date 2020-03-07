
#include <windows.h>
#include "debug.h"
#include "irpmondll-types.h"
#include "driver-com.h"
#include "request.h"
#include "irpmondll.h"



/************************************************************************/
/*                           EXPORTED FUNCTIONS                         */
/************************************************************************/


/// <summary>
/// 
/// </summary>
/// <param name="Request"></param>
/// <param name="Size"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllGetRequest(PREQUEST_HEADER Request, DWORD Size)
{
	return DriverComGetRequest(Request, Size);
}


/// <summary>
/// 
/// </summary>
/// <param name="Request"></param>
/// <returns></returns>
IRPMONDLL_API size_t WINAPI IRPMonDllGetRequestSize(const REQUEST_HEADER *Request)
{
	return RequestGetSize(Request);
}


/// <summary>
/// 
/// </summary>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllConnect(void)
{
	return DriverComConnect();
}


/// <summary>
/// 
/// </summary>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDisconnect(VOID)
{
	return DriverComDisconnect();
}


/// <summary>
/// 
/// </summary>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllQueueClear(void)
{
	return DriverComQueueClear();
}


/// <summary>
/// 
/// </summary>
/// <param name="DriverName"></param>
/// <param name="MonitorSettings"></param>
/// <param name="DeviceExtensionHook"></param>
/// <param name="DriverHandle"></param>
/// <param name="ObjectId"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDriver(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings, BOOLEAN DeviceExtensionHook, PHANDLE DriverHandle, PVOID *ObjectId)
{
	return DriverComHookDriver(DriverName, MonitorSettings, DeviceExtensionHook, DriverHandle, ObjectId);
}


/// <summary>
/// 
/// </summary>
/// <param name="HookHandle"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDriver(HANDLE HookHandle)
{
	return DriverComUnhookDriver(HookHandle);
}


/// <summary>
/// 
/// </summary>
/// <param name="DriverInfo"></param>
/// <param name="InfoCount"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllSnapshotRetrieve(PIRPMON_DRIVER_INFO **DriverInfo, PULONG InfoCount)
{
	return DriverComSnapshotRetrieve(DriverInfo, InfoCount);
}


/// <summary>
/// 
/// </summary>
/// <param name="DriverInfo"></param>
/// <param name="Count"></param>
/// <returns></returns>
IRPMONDLL_API VOID WINAPI IRPMonDllSnapshotFree(PIRPMON_DRIVER_INFO *DriverInfo, ULONG Count)
{
	DriverComSnapshotFree(DriverInfo, Count);

	return;
}


/// <summary>
/// 
/// </summary>
/// <param name="DeviceName"></param>
/// <param name="HookHandle"></param>
/// <param name="ObjectId"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByName(PWCHAR DeviceName, PHANDLE HookHandle, PVOID *ObjectId)
{
	return DriverComHookDeviceByName(DeviceName, HookHandle, ObjectId);
}


/// <summary>
/// 
/// </summary>
/// <param name="DeviceObject"></param>
/// <param name="HookHandle"></param>
/// <param name="ObjectId"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByAddress(PVOID DeviceObject, PHANDLE HookHandle, PVOID *ObjectId)
{
	return DriverComHookDeviceByAddress(DeviceObject, HookHandle, ObjectId);
}


/// <summary>
/// 
/// </summary>
/// <param name="Handle"></param>
/// <param name="IRPSettings"></param>
/// <param name="FastIOSettings"></param>
/// <param name="MonitoringEnabled"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDeviceGetInfo(HANDLE Handle, PUCHAR IRPSettings, PUCHAR FastIOSettings, PBOOLEAN MonitoringEnabled)
{
	return DriverComDeviceGetInfo(Handle, IRPSettings, FastIOSettings, MonitoringEnabled);
}


/// <summary>
/// 
/// </summary>
/// <param name="Handle"></param>
/// <param name="IRPSettings"></param>
/// <param name="FastIOSettings"></param>
/// <param name="MonitoringEnabled"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDeviceSetInfo(HANDLE Handle, PUCHAR IRPSettings, PUCHAR FastIOSettings, BOOLEAN MonitoringEnabled)
{
	return DriverComDeviceSetInfo(Handle, IRPSettings, FastIOSettings, MonitoringEnabled);
}


/// <summary>
/// 
/// </summary>
/// <param name="Handle"></param>
/// <param name="Settings"></param>
/// <param name="MonitoringEnabled"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllHookedDriverGetInfo(HANDLE Handle, PDRIVER_MONITOR_SETTINGS Settings, PBOOLEAN MonitoringEnabled)
{
	return DriverComHookedDriverGetInfo(Handle, Settings, MonitoringEnabled);
}


/// <summary>
/// 
/// </summary>
/// <param name="HookHandle"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDevice(HANDLE HookHandle)
{
	return DriverComUnhookDevice(HookHandle);
}


/// <summary>
/// 
/// </summary>
/// <param name="Driverhandle"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStartMonitoring(HANDLE Driverhandle)
{
	return DriverComHookedDriverActivate(Driverhandle, TRUE);
}


/// <summary>
/// 
/// </summary>
/// <param name="Driverhandle"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStopMonitoring(HANDLE Driverhandle)
{
	return DriverComHookedDriverActivate(Driverhandle, FALSE);
}


/// <summary>
/// 
/// </summary>
/// <param name="DriverHandle"></param>
/// <param name="Settings"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverSetInfo(HANDLE DriverHandle, PDRIVER_MONITOR_SETTINGS Settings)
{
	return DriverComHookedDriverSetInfo(DriverHandle, Settings);
}


/// <summary>
/// 
/// </summary>
/// <param name="ObjectId"></param>
/// <param name="Handle"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDriver(PVOID ObjectId, PHANDLE Handle)
{
	return DriverComDriverOpen(ObjectId, Handle);
}


/// <summary>
/// 
/// </summary>
/// <param name="Handle"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDriverHandle(HANDLE Handle)
{
	return DriverComDriverHandleClose(Handle);
}


/// <summary>
/// 
/// </summary>
/// <param name="ObjectId"></param>
/// <param name="Handle"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDevice(PVOID ObjectId, PHANDLE Handle)
{
	return DriverComDeviceOpen(ObjectId, Handle);
}


/// <summary>
/// 
/// </summary>
/// <param name="Handle"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDeviceHandle(HANDLE Handle)
{
	return DriverComDeviceHandleClose(Handle);
}


/************************************************************************/
/*                           HOOKED OBJECTS ENUMERATION                 */
/************************************************************************/


/// <summary>
/// 
/// </summary>
/// <param name="HookedDrivers"></param>
/// <param name="Count"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverHooksEnumerate(PHOOKED_DRIVER_UMINFO *HookedDrivers, PULONG Count)
{
	return DriverComHookedObjectsEnumerate(HookedDrivers, Count);
}


/// <summary>
/// 
/// </summary>
/// <param name="HookedDrivers"></param>
/// <param name="Count"></param>
/// <returns></returns>
IRPMONDLL_API VOID WINAPI IRPMonDllDriverHooksFree(PHOOKED_DRIVER_UMINFO HookedDrivers, ULONG Count)
{

	DriverComHookedObjectsFree(HookedDrivers, Count);

	return;
}


/************************************************************************/
/*                  CLASS WATCHES                                       */
/************************************************************************/


/// <summary>
/// 
/// </summary>
/// <param name="ClassGuid"></param>
/// <param name="UpperFilter"></param>
/// <param name="Beginning"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchRegister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	return DriverComClassWatchRegister(ClassGuid, UpperFilter, Beginning);
}


/// <summary>
/// 
/// </summary>
/// <param name="ClassGuid"></param>
/// <param name="UpperFilter"></param>
/// <param name="Beginning"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchUnregister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	return DriverComClassWatchUnregister(ClassGuid, UpperFilter, Beginning);
}


/// <summary>
/// 
/// </summary>
/// <param name="Array"></param>
/// <param name="Count"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchEnum(PCLASS_WATCH_RECORD *Array, PULONG Count)
{
	return DriverComClassWatchEnum(Array, Count);
}


/// <summary>
/// 
/// </summary>
/// <param name="Array"></param>
/// <param name="Count"></param>
/// <returns></returns>
IRPMONDLL_API VOID WINAPI IRPMonDllClassWatchEnumFree(PCLASS_WATCH_RECORD Array, ULONG Count)
{
	DriverComClassWatchEnumFree(Array, Count);

	return;
}


/************************************************************************/
/*                  DRIVER NAME WATCHES                                 */
/************************************************************************/


/// <summary>
/// 
/// </summary>
/// <param name="DriverName"></param>
/// <param name="MonitorSettings"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchRegister(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings)
{
	return DriverComDriverNameWatchRegister(DriverName, MonitorSettings);
}


/// <summary>
/// 
/// </summary>
/// <param name="DriverName"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchUnregister(PWCHAR DriverName)
{
	return DriverComDriverNameWatchUnregister(DriverName);
}


/// <summary>
/// 
/// </summary>
/// <param name="Array"></param>
/// <param name="Count"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchEnum(PDRIVER_NAME_WATCH_RECORD *Array, PULONG Count)
{
	return DriverComDriverNameWatchEnum(Array, Count);
}


/// <summary>
/// 
/// </summary>
/// <param name="Array"></param>
/// <param name="Count"></param>
/// <returns></returns>
IRPMONDLL_API VOID WINAPI IRPMonDllDriverNameWatchEnumFree(PDRIVER_NAME_WATCH_RECORD Array, ULONG Count)
{
	DriverComDriverNameWatchEnumFree(Array, Count);

	return;
}


/// <summary>
/// 
/// </summary>
/// <param name="DriverObject"></param>
/// <param name="DriverName"></param>
/// <param name="Request"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateDriverDetected(void *DriverObject, const wchar_t *DriverName, PREQUEST_DRIVER_DETECTED *Request)
{
	return RequestEmulateDriverDetected(DriverObject, DriverName, Request);
}


/// <summary>
/// 
/// </summary>
/// <param name="DriverObject"></param>
/// <param name="DeviceObject"></param>
/// <param name="DeviceName"></param>
/// <param name="Request"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateDeviceDetected(void *DriverObject, void *DeviceObject, const wchar_t *DeviceName, PREQUEST_DEVICE_DETECTED *Request)
{
	return RequestEmulateDeviceDetected(DriverObject, DeviceObject, DeviceName, Request);
}


/// <summary>
/// 
/// </summary>
/// <param name="FileObject"></param>
/// <param name="FileName"></param>
/// <param name="Request"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateFileNameAssigned(void *FileObject, const wchar_t *FileName, PREQUEST_FILE_OBJECT_NAME_ASSIGNED *Request)
{
	return RequestEmulateFileNameAssigned(FileObject, FileName, Request);
}


/// <summary>
/// 
/// </summary>
/// <param name="FileObject"></param>
/// <param name="Request"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateFileNameDeleted(void *FileObject, PREQUEST_FILE_OBJECT_NAME_DELETED *Request)
{
	return RequestEmulateFileNameDeleted(FileObject, Request);
}


/// <summary>
/// 
/// </summary>
/// <param name="ProcessId"></param>
/// <param name="ParentId"></param>
/// <param name="ImageName"></param>
/// <param name="CommandLine"></param>
/// <param name="Request"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateProcessCreated(HANDLE ProcessId, HANDLE ParentId, const wchar_t *ImageName, const wchar_t *CommandLine, PREQUEST_PROCESS_CREATED *Request)
{
	return RequestEmulateProcessCreated(ProcessId, ParentId, ImageName, CommandLine, Request);
}


/// <summary>
/// 
/// </summary>
/// <param name="ProcessId"></param>
/// <param name="Request"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllRequestEmulateProcessExitted(HANDLE ProcessId, PREQUEST_PROCESS_EXITTED *Request)
{
	return RequestEmulateProcessExitted(ProcessId, Request);
}


/// <summary>
/// 
/// </summary>
/// <param name="Header"></param>
/// <returns></returns>
IRPMONDLL_API PREQUEST_HEADER WINAPI IRPMonDllRequestCopy(const REQUEST_HEADER *Header)
{
	return RequestCopy(Header);
}


/// <summary>
/// 
/// </summary>
/// <param name="Size"></param>
/// <returns></returns>
IRPMONDLL_API PREQUEST_HEADER WINAPI IRPMonDllRequestMemoryAlloc(size_t Size)
{
	return RequestMemoryAlloc(Size);
}


/// <summary>
/// 
/// </summary>
/// <param name="Header"></param>
/// <returns></returns>
IRPMONDLL_API void WINAPI IRPMonDllRequestMemoryFree(PREQUEST_HEADER Header)
{
	RequestMemoryFree(Header);

	return;
}


/// <summary>
/// 
/// </summary>
/// <param name="Header"></param>
/// <returns></returns>
IRPMONDLL_API BOOLEAN WINAPI IRPMonDllRequestCompress(PREQUEST_HEADER Header)
{
	return RequestCompress(Header);
}


/// <summary>
/// 
/// </summary>
/// <param name="Header"></param>
/// <returns></returns>
IRPMONDLL_API PREQUEST_HEADER WINAPI IRPMonDllRequestDecompress(const REQUEST_HEADER *Header)
{
	return RequestDecompress(Header);
}


/// <summary>
/// 
/// </summary>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllEmulateDriverDevices(void)
{
	return DriverComEmulateProcesses();
}


/// <summary>
/// 
/// </summary>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllEmulateProcesses(void)
{
	return DriverComEmulateProcesses();
}


/// <summary>
/// 
/// </summary>
/// <param name="Settings"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllSettingsQuery(PIRPMNDRV_SETTINGS Settings)
{
	return DriverComSettingsQuery(Settings);
}


/// <summary>
/// 
/// </summary>
/// <param name="Settings"></param>
/// <param name="Save"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllSettingsSet(PIRPMNDRV_SETTINGS Settings, BOOLEAN Save)
{
	return DriverComSettingsSet(Settings, Save);
}


/************************************************************************/
/*                          INITIALIZATION AND FINALIZATION             */
/************************************************************************/


/// <summary>
/// 
/// </summary>
/// <returns></returns>
IRPMONDLL_API BOOL WINAPI IRPMonDllInitialized(VOID)
{
	BOOL ret = FALSE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = DriverComDeviceConnected();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


/// <summary>
/// 
/// </summary>
/// <param name="Info"></param>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllInitialize(const IRPMON_INIT_INFO *Info)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);

	ret = DriverComModuleInit(Info);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


/// <summary>
/// 
/// </summary>
/// <returns></returns>
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
