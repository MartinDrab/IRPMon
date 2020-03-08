
#include <windows.h>
#include "debug.h"
#include "irpmondll-types.h"
#include "driver-com.h"
#include "request.h"
#include "irpmondll.h"



/************************************************************************/
/*                           EXPORTED FUNCTIONS                         */
/************************************************************************/


/// <summary>Removes a request from the IRPMon Event Queue and copies it to a given buffer.
/// </summary>
/// <param name="Request">
/// Address of buffer to which the request data will be copied.
/// </param>
/// <param name="Size">
/// Size of the buffer, in bytes.
/// </param>
/// <returns>
/// The function returns one of the following error codes :
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>
///   A request has been successfully removed from the queue and
///   copied to the given buffer.
///  </description>
/// </item>
/// <item>
///   <term>ERROR_INSUFFICIENT_BUFFER</term>
///   <description>
///   The given buffer is not large enough to
///   hold all the request data. The caller needs to specify a larger buffer.
///   The request remains in the queue.
///   </description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// The calling thread must be connected to the IRPMon Event Queue.Otherwise,
/// the function fails.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllGetRequest(PREQUEST_HEADER Request, DWORD Size)
{
	return DriverComGetRequest(Request, Size);
}


/// <summary>Gets size of a request, in bytes.
/// </summary>
/// <param name="Request">
/// Pointer to the request retrieved via <see cref="IRPMonDllGetRequest"/>.
/// </param>
/// <returns>
/// If successful, returns the request size, in bytes.
/// On error, zero is returned.
/// </returns>
IRPMONDLL_API size_t WINAPI IRPMonDllGetRequestSize(const REQUEST_HEADER *Request)
{
	return RequestGetSize(Request);
}


/// <summary>
/// Connects the current thread (the calling one) to the queue
/// of events detected by the IRPMon driver.
/// </summary>
/// <returns>
/// The function returns one of the following error codes :
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The thread successfully connected to the queue.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// At most one thread can be connected to the IRPMon Event Queue at any
/// moment of time.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllConnect(void)
{
	return DriverComConnect();
}


/// <summary>Disconnects the current thread from the IRPMon Event Queue.
/// </summary>
/// <returns>
/// The function returns one of the following error codes :
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The thread successfully disconnected from the queue.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllDisconnect(VOID)
{
	return DriverComDisconnect();
}


/// <summary>Discards all requests stored in the IRPMon Event Queue.
/// </summary>
/// <returns>
/// The function returns one of the following error codes :
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The queue has been successfully cleared.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllQueueClear(void)
{
	return DriverComQueueClear();
}


/// <summary>Given name of its object, the routine hooks a driver in order to monitor requests serviced by its devices.
/// </summary>
/// <param name="DriverName">
/// Name of the driver object to hook.The name usually starts with the "\Driver\" or "\FileSystem\" prefix.
/// </param>
/// <param name="MonitorSettings">
/// Defines types of events being monitored on the given driver object
/// and its devices.
/// </param>
/// <param name="DeviceExtensionHook">
/// Determines whether the IRPMon takes advantage of IRP hooks(FALSE) or device extension
/// based hooks(TRUE).
/// </param>
/// <param name="DriverHandle">
/// Address of variable that receives a handle representing the hooked driver.
/// </param>
/// <param name="ObjectId">
/// Address of variable that receives globally unique ID of the hooked driver object.
/// This parameter is optional and can be <c>NULL</c>.
/// </param>
/// <returns>
/// The function may return one of the following error codes:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The hook operation has succeeded.The hook handle is stored in the <paramref name="Driverhandle"/> parameter.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// <para>This routine instructs the IRPMon driver to prepare to monitor a given driver.The monitoring itself,
/// however, must be activated by a call to the <see cref="IRPMonDllDriverStartMonitoring"/> routine.The IRPMon driver just remembers which requests
/// will be monitored for the given driver and saves also a list of its devices in order to be able to distinguish
/// them from new ones(devices created after the IRPMonDllHookDriver returns).
/// </para>
/// <para>Driver names accepted by this function can be obtained from a list of drivers present in the system, returned by the
/// <see cref="IRPMonDllSnapshotRetrieve"/> function.
/// </para>
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDriver(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings, BOOLEAN DeviceExtensionHook, PHANDLE DriverHandle, PVOID *ObjectId)
{
	return DriverComHookDriver(DriverName, MonitorSettings, DeviceExtensionHook, DriverHandle, ObjectId);
}


/// <summary>Unhooks a given driver.
/// </summary>
/// <param name="HookHandle">Handle to the hooked driver.
/// </param>
/// <returns>
/// One of the following error codes may be returned:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The driver has been unhooked successfully.</description>
/// </item>
/// <item>
///   <term>ERROR_INVALID_HANDLE</term>
///   <description>The handle specified in the <paramref name="DriverHandle" /> parameter is invalid.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remark>
/// <para>
/// If the monitoring is active for the given driver, the routine fails. The monitoring
/// must be stopped by a call to the <see cref="IRPMonDllDriverStopMonitoring"/> functiion first.
/// </para>
/// <para>
/// The routine causes the IRPMon driver to forget all information about the given hooked
/// driver which makes it being no longer hooked. The actual unhooking (removing hooks from
///	driver's DRIVER_OBJECT structure) is done by the <see cref="IRPMonDllDriverStopMonitoring"/> routine.
/// </para>
/// <para>
///	After the successful call to this routine, the handle passed in the <paramref name="DriverHandle"/> argument to the call
///	becomes invalid.
/// </para>
/// </remark>
IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDriver(HANDLE HookHandle)
{
	return DriverComUnhookDriver(HookHandle);
}


/// <summary>Retrieves information about driver and device objects currently present in the system.
/// </summary>
/// <param name="DriverInfo">
/// Address of variable that receives address of an array of
/// pointers to <see cref="_IRPMON_DRIVER_INFO"/> structures, each containing information about
/// one driver and its devices.
/// </param>
/// <param name="InfoCount">
/// Address of variable that receives the number of structures in the array.
/// </param>
/// <returns>
/// The routine may return one of the following values:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The snapshot has been retrieved successfully.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// When the caller no longer needs the information retrieved by the routine,
/// it must free it by calling the <see cref="IRPMonDllSnapshotFree"/> procedure.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllSnapshotRetrieve(PIRPMON_DRIVER_INFO **DriverInfo, PULONG InfoCount)
{
	return DriverComSnapshotRetrieve(DriverInfo, InfoCount);
}


/// <summary> Frees a given snapshot of drivers and their devices.
/// </summary>
/// <param name="DriverInfo">
/// An array of pointers to <see cref="_IRPMON_DRIVER_INFO"/> structures, each
/// containing information about one driver and its devices. Address of the array
/// is returned in the first parameter of a call to the <see cref="IRPMonDllSnapshotRetrieve"/>
/// function.
/// </param>
/// <param name="Count">
/// Number of entries in the array. This value is returned in the second parameter
/// of the <see cref="IRPMonDllSnapshotRetrieve"/> call.
/// </param>
IRPMONDLL_API VOID WINAPI IRPMonDllSnapshotFree(PIRPMON_DRIVER_INFO *DriverInfo, ULONG Count)
{
	DriverComSnapshotFree(DriverInfo, Count);

	return;
}


/// <summary>Starts monitoring events related to a given device, identified by its object name.
/// </summary>
/// <param name="DeviceName">
/// Name of the device object to be monitored. It can be obtained from
/// a snapshot of drivers and devices currently present in the system, retrieved by a call
/// to the <see cref="IRPMonDllSnapshotRetrieve"/> function.
/// </param>
/// <param name="HookHandle">
/// Address of variable that receives a handle representing the device being monitored.
/// </param>
/// <param name="ObjectId">
/// Address of variable that receives globally unique ID of the target device object.
/// This parameter is optional and can be <c>NULL</c>.
/// </param>
/// <returns>
/// The following error codes may be returned:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The operation has succeeded. The device is now being monitored.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// Before a call to this routine, the driver to which the device belongs must
/// be hooked. Otherwise, the function fails. Drivers can be hooked by a call to the
/// <see cref="IRPMonDllHookDriver"/> routine.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByName(PWCHAR DeviceName, PHANDLE HookHandle, PVOID *ObjectId)
{
	return DriverComHookDeviceByName(DeviceName, HookHandle, ObjectId);
}


/// <summary>Starts monitoring events related to a given device, identified by its device object address.
/// </summary>
/// <param name="DeviceObject">
/// Address of the device object to be monitored. It can be obtained from
/// a snapshot of drivers and devices currently present in the system, retrieved by a call
/// to the <see cref="IRPMonDllSnapshotRetrieve"/> function.
/// </param>
/// <param name="HookHandle">
/// Address of variable that receives a handle representing the device being monitored.
/// </param>
/// <param name="ObjectId">
/// Address of variable that receives globally unique ID of the target device object.
/// This parameter is optional and can be <c>NULL</c>.
/// </param>
/// <returns>
/// The following error codes may be returned:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The operation has succeeded. The device is now being monitored.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// Before a call to this routine, the driver to which the device belongs must
/// be hooked. Otherwise, the function fails. Drivers can be hooked by a call to the
/// <see cref="IRPMonDllHookDriver"/> routine.
/// </remarks>
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


/// <summary>Stops monitoring events related to a given device object.
/// </summary>
/// <param name="HookHandle">
///  *  A handle representing the device object being monitored.
/// </param>
/// <returns>
/// The routine can return one of the following values :
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>
///   The operation has succeeded. The devices is not being
///   monitored any more. The handle given in the argument is not valid any longer.
///   </description>
/// </item>
/// <item>
///   <term>ERROR_INVALID_HANDLE</term>
///   <description>The given handle is invalid.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// If the whole driver to which the device belongs is unhooked (see <see cref="IRPMonDllUnhookDriver"/>), this
/// routine needs not to be called since the device handle is automatically invalidated.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDevice(HANDLE HookHandle)
{
	return DriverComUnhookDevice(HookHandle);
}


/// <summary>Starts monitoring of a driver.
/// </summary>
/// <param name="Driverhandle">
/// Handle to the hooked driver, returned by a call to the
/// <see cref="IRPMonDllHookDriver"/> function.
/// </param>
/// <returns>
/// The function can return the following error codes:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The monitoring has successfully started.Events detected by the IRPMon driver are being stored into the event queue.</description>
/// </item>
/// <item>
///   <term>ERROR_INVALID_HANDLE</term>
///   <description>The handle supplied in the parameter is not valid.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error has occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// <para>
/// The routine effectively hooks the driver which means it places hooks inside its DRIVER_OBJECT
/// structure. From this time, the IRPMon driver starts receiving notifications from these hooks.
/// </para>
/// <para>
/// Monitoring, enabled by this routine, can be stopped by a call to the <see cref="IRPMonDllDriverStopMonitoring"/>
/// function..
/// </para>
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStartMonitoring(HANDLE Driverhandle)
{
	return DriverComHookedDriverActivate(Driverhandle, TRUE);
}


/// <summary>Stops monitoring events related to a given driver.
/// </summary>
/// <param name="Driverhandle">
/// Handle to the hooked driver (returned by a call to the <see cref="IRPMonDllHookDriver"/> routine).
/// </param>
/// <returns>
/// The function can return the following error codes :
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description> The monitoring has been successfully stopped for the given driver.</description>
/// </item>
/// <item>
///   <term>ERROR_INVALID_HANDLE</term>
///   <description>The handle supplied in the parameter is not valid.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error has occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// <para>
/// This routine does exactly the opposite of the <see cref="IRPMonDllDriverStartMonitoring"/> function.
/// </para>
/// <para>
/// The routine removes all hooks placed on driver's DRIVER_OBJECT structure, so no events related
/// to the driver are detected any longer. Howerver, the driver is still remembered as being hooked;
/// its record remains in data structures of the IRPMon driver.To remove the "hooked" label from the
/// driver, use the <see cref="IRPMonDllUnhookDriver"/> function.
/// </para>
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStopMonitoring(HANDLE Driverhandle)
{
	return DriverComHookedDriverActivate(Driverhandle, FALSE);
}


/// <summary>Changes monitoring settings for a given driver and its devices.
/// </summary>
/// <param name="DriverHandle">
/// Handle to the hooked driver the settings of which needs to be changed.
/// </param>
/// <param name="Settings">
/// New values of the monitoring settings for the given driver.
/// </param>
/// <returns>
/// Returns one of the following error codes:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The settings for the given hooked driver has been successfully changed to the new values.</description>
/// </item>
/// <item>
///   <term>ERROR_INVALID_HANDLE</term>
///   <description>The handle supplied in the parameter is not valid.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error has occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// If the IRPMon driver is currently monitoring activity related to the given driver,
/// only the value stored in the <see cref="_DRIVER_MONITOR_SETTINGS:MonitorNewDevices"/> field
/// passed in the <paramref name="Settings"/> parameter takes effect.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverSetInfo(HANDLE DriverHandle, PDRIVER_MONITOR_SETTINGS Settings)
{
	return DriverComHookedDriverSetInfo(DriverHandle, Settings);
}


/// <summary>Open a handle to a given driver monitored by the IRPMon driver.
/// </summary>
/// <param name="ObjectId">
/// ID of the target driver. IDs can be obtained from the
/// 'ObjectId' member of the <see cref="_HOOKED_DRIVER_UMINFO"/> structure retrieved
/// by the <see cref="IRPMonDllDriverHooksEnumerate"/> function.
/// </param>
/// <param name="Handle">
/// Address of variable that receives the newly opened handle. The
/// handle can be then used to control the hooked driver.
/// </param>
/// <returns>
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The handle has been successfully created.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// When no longer needed, the handle should be closed via the <see cref="IRPMonDllCloseHookedDriverHandle"/>
/// function. Alternatively, the <see cref="IRPMonDllUnhookDriver"/> routine may also be used to close the
/// handle, however, it also unhooks the driver represented by the handle.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDriver(PVOID ObjectId, PHANDLE Handle)
{
	return DriverComDriverOpen(ObjectId, Handle);
}


/// <summary>Closes a handle to a given driver monitored by the IRPMon.
/// </summary>
/// <param name="Handle">
/// The handle to close.
/// </param>
/// <returns>
/// The routine should always return ERROR_SUCCESS.I f it does not, the bug is
/// caller's code, not in the code of the library or driver.
/// TODO: Use the table
/// </returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDriverHandle(HANDLE Handle)
{
	return DriverComDriverHandleClose(Handle);
}


/// <summary>Open a handle to a given device monitored by the IRPMon driver.
/// </summary>
/// <param name="ObjectId">
/// ID of the target device. IDs can be obtained from the
/// 'ObjectId' member of the <see cref="_HOOKED_DEVICE_UMINFO"/> structure retrieved
/// by the <see cref="IRPMonDllDriverHooksEnumerate"/> function.
/// </param>
/// <param name="Handle">
/// Address of variable that receives the newly opened handle. The
/// handle can be then used to control the hooked device.
/// </param>
/// <returns>
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The handle has been successfully created.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// When no longer needed, the handle should be closed via the <see cref="IRPMonDllCloseHookedDeviceHandle"/>
/// function. Alternatively, the <see cref="IRPMonDllUnhookDevice"/> routine may also be used to close the
/// handle, however, it also unhooks the device represented by the handle.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDevice(PVOID ObjectId, PHANDLE Handle)
{
	return DriverComDeviceOpen(ObjectId, Handle);
}


/// <summary>Closes a handle to a given device monitored by the IRPMon.
/// </summary>
/// <param name="Handle">
/// The handle to close.
/// </param>
/// <returns>
/// The routine should always return ERROR_SUCCESS.I f it does not, the bug is
/// caller's code, not in the code of the library or driver.
/// TODO: Use the table
/// </returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDeviceHandle(HANDLE Handle)
{
	return DriverComDeviceHandleClose(Handle);
}


/************************************************************************/
/*                           HOOKED OBJECTS ENUMERATION                 */
/************************************************************************/


/// <summary> Retrieves information about drivers and devices hooked by the IRPMon driver.
/// <para>
/// Individual drivers and their devices are identified by their addresses, and names.
/// </para>
/// </summary>
/// <param name="HookedDrivers">
/// Address of variable that receives an array of  @link(HOOKED_DRIVER_UMINFO) structures. Each structure represents one driver monitored by IRPMonand includes also information about its hooked devices.
/// </param>
/// <param name="Count">
/// Address of variable that receives number of structures in the HookedDrivers array.
/// </param>
/// <returns>
/// The function can return one of the following error codes:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The operation completed successfully.</description>
/// </item>
/// <item>
///   <term>ERROR_NOT_ENOUGH_MEMORY</term>
///   <description>There is not enough available free memory to perform the operation. </description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// When the caller no longer needs the retrieved information, it must free it by calling
/// the <see cref="IRPMonDllDriverHooksFree"/> procedure.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverHooksEnumerate(PHOOKED_DRIVER_UMINFO *HookedDrivers, PULONG Count)
{
	return DriverComHookedObjectsEnumerate(HookedDrivers, Count);
}


/// <summary>Frees information returned by the <see cref="IRPMonDllDriverHooksEnumerate"/> function
/// </summary>
/// <param name="HookedDrivers">
/// Address of an array of  <see cref="HOOKED_DRIVER_UMINFO"/> structures.The array is returned in the first argument in a call to the <see cref="IRPMonDllDriverHooksEnumerate"/> function.
/// </param>
/// <param name="Count">
/// Number of elements in the array.
/// </param>
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


/// <summary>Checks whether the IRPMon library is initialized.
/// </summary>
/// <returns>
/// Returns a boolean value indicating whether the library is initialized.
/// </returns>
/// <remarks>
/// The library is considered initialized if and only if a connection to the IRPMon
/// driver device object is established (locally or remotely).
/// </remarks>
IRPMONDLL_API BOOL WINAPI IRPMonDllInitialized(VOID)
{
	BOOL ret = FALSE;
	DEBUG_ENTER_FUNCTION_NO_ARGS();

	ret = DriverComDeviceConnected();

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


/// <summary>
/// Initializes the IRPMon library and connects the current process to the
/// IRPMon driver.
/// </summary>
/// <param name="Info">
/// Determines how to connect to an IRPMon driver instance. The library
/// can connect etiher to a locally installed IRPMon driver, or to an
/// IRPMon server instance running on remote machine.
/// </param>
/// <returns>
/// One of the following values may be returned:
/// <list type="table">
/// <listheader>
///   <term>Value</term>
///   <description>Description</description>
/// </listheader>
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The initialization was successful.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// This routine must be successfully called before any other routine exported
/// by the library.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllInitialize(const IRPMON_INIT_INFO *Info)
{
	DWORD ret = ERROR_GEN_FAILURE;
	DEBUG_ENTER_FUNCTION("Info=0x%p", Info);

	ret = DriverComModuleInit(Info);

	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}


/// <summary>Disconnects the current process from the IRPMon driver and cleans up
/// resources used by the library.
/// </summary>
/// <remarks>
/// After calling this routine, no other routine exported by the library can
/// be successfully invoked.
/// </remarks>
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
