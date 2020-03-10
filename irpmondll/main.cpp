
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
#include "irpmondll-types.h"
#include "driver-com.h"
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


/// <summary>Starts watching for devices (and their drivers) belonging to a given device setup class.
/// <para>
/// The IRPMon driver install itself as a lower or upper filter to the specified class. Thus,
/// its AddDevice routine will be invoked when a new device of that class appears in the system.
/// </para>
/// </summary>
/// <param name="ClassGuid">
/// GUID of the device setup class to monitor, in its string form.
/// </param>
/// <param name="UpperFilter">
/// A boolean valu indicating whether the IRPMon driver should be installed
/// as a lower or upper filter.
/// </param>
/// <param name="Beginning">
/// Determines whether the IRPMon driver is installed as the first or the last
/// of the class filters.
/// </param>
/// <returns>
/// Returns one of the following values:
/// <list type="table">
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The registration was successful</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// The ability to watch for new devices and drivers of certain device setup class
/// can be useful when tracking requests sent to a device shortly after its appearance.
/// Use <see cref="IRPMonDllDriverNameWatchRegister"/> to automatically start monitoring a
/// driver with specific name immediately after one of its devices is created and detected
/// by the class watching capability of the IRPMon driver.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchRegister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	return DriverComClassWatchRegister(ClassGuid, UpperFilter, Beginning);
}


/// <summary>Stops watching a given device setup class from specific filter position.
/// </summary>
/// <param name="ClassGuid">
/// GUID of the device setup class, in its string form. Must match the value specified during registration.
/// </param>
/// <param name="UpperFilter">
/// Type of the filter to uninstall. Must match the value specified during registration.
/// </param>
/// <param name="Beginning">
/// Position of the filter to uninstall. Must match the value specified during registration.
/// </param>
/// <returns>
/// Returns one of the following values:
/// <list type="table">
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The specified filter has been uninstalled successfully.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// Look at the Remarks section for <see cref="IRPMonDllClassWatchRegister"/> to get more details
/// about the usefulness of class watching. 
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchUnregister(PWCHAR ClassGuid, BOOLEAN UpperFilter, BOOLEAN Beginning)
{
	return DriverComClassWatchUnregister(ClassGuid, UpperFilter, Beginning);
}


/// <summary>Enumerates device setup classes watched by the IRPMon driver.
/// </summary>
/// <param name="Array">
/// Address of variable that receives an array of <see cref="_CLASS_WATCH_RECORD"/> structures.
/// </param>
/// <param name="Count">
/// Address of variable that receives number of elements returned 
/// in the *<paramref name="Array"/> array.
/// </param>
/// <returns>
/// One of the following values may be returned:
/// <list type="table">
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The <paramref name="Array"/> and <paramref name="Count"/> arguments contain information about watched classes.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// Use <see cref="IRPMonDllClassWatchEnumFree"/> to release the array retrieved by this routine.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllClassWatchEnum(PCLASS_WATCH_RECORD *Array, PULONG Count)
{
	return DriverComClassWatchEnum(Array, Count);
}


/// <summary>Releases array of watched classes returned by the <see cref="IRPMonDllClassWatchEnum"/> routine.
/// </summary>
/// <param name="Array">
/// Address of the array returned in the first argument of the <see cref="IRPMonDllClassWatchEnum"/> function.
/// </param>
/// <param name="Count">
/// Number of elements in the array.
/// </param>
IRPMONDLL_API VOID WINAPI IRPMonDllClassWatchEnumFree(PCLASS_WATCH_RECORD Array, ULONG Count)
{
	DriverComClassWatchEnumFree(Array, Count);

	return;
}


/************************************************************************/
/*                  DRIVER NAME WATCHES                                 */
/************************************************************************/


/// <summary>Instructs the IRPMon driver to automatically start monitoring
/// a driver with the given name when its presence is detected.
/// </summary>
/// <param name="DriverName">
/// Full name of the target driver object.
/// </param>
/// <param name="MonitorSettings">
/// Defines which types of requests should be monitored and logged.
/// </param>
/// <returns>
/// One of the following values may be returned:
/// <list type="table">
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The IRPMon driver is now watching for the given driver.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// <para>
/// The driver name specified in the <paramref name="DriverName"/> must be an absolute name
/// in the Object Manager namespace. In other words, it should usually contain the \Driver\ or
/// \FileSystem\ prefix.
/// </para>
/// <para>
/// Watching for drivers with specific names makes sense especially with
/// device setup class watching, see <see cref="IRPMonDllClassWatchRegister"/> for more details.
/// </para>
/// <para>
/// Use <see cref="IRPMonDllDriverNameWatchUnregister"/> to stop watching for the given driver.
/// </para>
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchRegister(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings)
{
	return DriverComDriverNameWatchRegister(DriverName, MonitorSettings);
}


/// <summary>Stops watching for driver with given name.
/// </summary>
/// <param name="DriverName">
/// Name of the driver that should not be watched for anymore.
/// </param>
/// <returns>
/// One of the following values may be returned:
/// <list type="table">
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The IRPMon driver is no longer watching for the given driver.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// <para>
/// The driver name specified in the <paramref name="DriverName"/> must be an absolute name
/// in the Object Manager namespace. In other words, it should usually contain the \Driver\ or
/// \FileSystem\ prefix.
/// </para>
/// <para>
/// This routine is exact opposite to <see cref="IRPMonDllDriverNameWatchRegister"/> one.
/// </para>
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchUnregister(PWCHAR DriverName)
{
	return DriverComDriverNameWatchUnregister(DriverName);
}


/// <summary>Enumerates names of drivers on the watch list.
/// </summary>
/// <param name="Array">
/// Address of variable that receives an array of <see cref="_DRIVER_NAME_WATCH_RECORD"/> structures,
/// each containing information about one watched driver name.
/// </param>
/// <param name="Count">
/// Address of variable that receives number of elements in the array.
/// </param>
/// <returns>
/// One of the following values may be returned:
/// <list type="table">
/// <item>
///   <term>ERROR_SUCCESS</term>
///   <description>The <paramref name="Array"/> and <paramref name="Count"/> arguments contain information about watched driver names.</description>
/// </item>
/// <item>
///   <term>Other</term>
///   <description>An error occurred.</description>
/// </item>
/// </list>
/// </returns>
/// <remarks>
/// Release the array returned by this routine by calling
/// the <see cref="IRPMonDllDriverNameWatchEnumFree"/> routine.
/// </remarks>
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverNameWatchEnum(PDRIVER_NAME_WATCH_RECORD *Array, PULONG Count)
{
	return DriverComDriverNameWatchEnum(Array, Count);
}


/// <summary>Releases array of watched classes returned by the <see cref="IRPMonDllDriverNameWatchEnum"/> routine.
/// </summary>
/// <param name="Array">
/// Address of the array returned in the first argument of the <see cref="IRPMonDllDriverNameWatchEnum"/> function.
/// </param>
/// <param name="Count">
/// Number of elements in the array.
/// </param>
IRPMONDLL_API VOID WINAPI IRPMonDllDriverNameWatchEnumFree(PDRIVER_NAME_WATCH_RECORD Array, ULONG Count)
{
	DriverComDriverNameWatchEnumFree(Array, Count);

	return;
}


/// <summary>
/// 
/// </summary>
/// <returns></returns>
IRPMONDLL_API DWORD WINAPI IRPMonDllEmulateDriverDevices(void)
{
	return DriverComEmulateDriverDevices();
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
