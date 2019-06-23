
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



/** Retrieves information about drivers and devices hooked by the IRPMon driver.
 *
 *  Individual drivers and their devices are identified by their addresses, and names.
 *
 *  @param HookedDrivers Address of variable that receives an array of  @link(HOOKED_DRIVER_UMINFO) structures.
 *  Each structure represents one driver monitored by IRPMon and includes also information about its hooked devices.
 *  @param Count Address of variable that receives number of structures in the HookedDrivers array.
 *
 *  @return
 *  The function can return one of the following error codes:
 *  @value ERROR_SUCCESS The operation completed successfully.
 *  @value ERROR_NOT_ENOUGH_MEMORY There is not enough available free memory to
 *  perform the operation.
 *
 *  @remark
 *  When the caller no longer needs the retrieved information, it must free it by calling
 *  the @link(IRPMonDllDriverHooksFree) procedure.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverHooksEnumerate(PHOOKED_DRIVER_UMINFO *HookedDrivers, PULONG Count);


/** Frees information returned by the @link(IRPMonDllDriverHooksEnumerate) function.
 *
 *  @param HookedDrivers Address of an array of  @link(HOOKED_DRIVER_UMINFO) structures. The array is returned
 *  in the first argument in a call to the @link(IRPMonDllDriverHooksEnumerate) function.
 *  @param Count Number of elements in the array.
 */
IRPMONDLL_API VOID WINAPI IRPMonDllDriverHooksFree(PHOOKED_DRIVER_UMINFO HookedDrivers, ULONG Count);


/** Given name of its object, the routine hooks a driver in order to monitor requests serviced by
 *  its devices.
 *
 *  @param DriverName Name of the driver object to hook. The name usually starts with the "\Driver\" or
 *  "\FileSystem\" prefix.
 *  @param MonitorNewDevices Indicates whether requests directed at device objects created by the driver
 *  some time after the call to this function will be automatically monitored by IRPMon. 
 *  @param AddDevice Determine whether invocations of driver's AddDevice routine will be monitored.
 *  @param Unload Instructs the IRPMon driver to detect driver unload.
 *  @param StartIo Indicates whether driver's StartIo routine will be monitored.
 *  @param Driverhandle Address of variable that receives a handle representing the hooked driver.
 *  
 *  @return
 *  The function may return one of the following error codes:
 *  @value ERROR_SUCCESS The hook operation has succeeded. The hook handle is stored in the 
 *  Driverhandle parameter.
 *  @value Other An error occurred.
 *
 *  @remark
 *  This routine instructs the IRPMon driver to prepare to monitor a given driver. The monitoring itself,
 *  however, must be activated by a call to the @link(IRPMonDllDriverStartMonitoring) routine. The IRPMon driver just remembers which requests
 *  will be monitored for the given driver and saves also a list of its devices in order to be able to distinguish
 *  them from new ones (devices created after the IRPMonDllHookDriver returns).
 *
 *  Driver names accepted by this function can be obtained from a list of drivers present in the system, returned by the
 *  @link(IRPMonDllSnapshotRetrieve) function.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDriver(PWCHAR DriverName, PDRIVER_MONITOR_SETTINGS MonitorSettings, PHANDLE DriverHandle, PVOID *ObjectId);


/** Starts monitoring of a driver. 
 *
 *  @param DriverHandle Handle to the hooked driver, returned by a call to the
 *  @link(IRPMonDllHookDriver) function. 
 *
 *  @remark
 *  The function can return the following error codes:
 *  @value ERROR_SUCCESS The monitoring has successfully started. Events detected by the
 *  IRPMon driver are being stored into the event queue.
 *  @value ERROR_INVALID_HANDLE The handle supplied in the parameter is not valid.
 *  @value Other An error has occurred.
 *
 *  @remark
 *  The routine effectively hooks the driver which means it places hooks inside its DRIVER_OBJECT
 *  structure. From this time, the IRPMon driver starts receiving notifications from these hooks.
 *
 *  Monitoring, enabled by this routine, can be stopped by a call to the @link(IRPMonDllDriverStopMonitoring)
 *  function..
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStartMonitoring(HANDLE Driverhandle);


/** Stops monitoring events related to a given driver.
 *
 *  @param DriverHandle Handle to the hooked driver (returned by a call to the @Link(IRPMonDllHookDriver) routine).
 *
 *  @return
 *  The function can return the following error codes:
 *  @value ERROR_SUCCESS The monitoring has been successfully stopped for the given
 *  driver. 
 *  @value ERROR_INVALID_HANDLE The handle supplied in the parameter is not valid.
 *  @value Other An error occurred.
 *
 *  @remark
 *  This routine does exactly the opposite of the @Link(IRPMonDllDriverStartMonitoring) function.
 *
 *  The routine removes all hooks placed on driver's DRIVER_OBJECT structure, so no events related
 *  to the driver are detected any longer. Howerver, the driver is still remembered as being hooked;
 *  its record remains in data structures of the IRPMon driver. To remove the "hooked" label from the
 *  driver, use the @Link(IRPMonDllUnhookDriver) function.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverStopMonitoring(HANDLE Driverhandle);


/** Changes monitoring settings for a given driver and its devices.
 *
 *  @param DriverHandle Handle to the hooked driver the settings of which needs to
 *  be changed. 
 *  @param Settings New values of the monitoring settings for the given driver.
 *
 *  @return
 *  Returns one of the following error codes:
 *  @value ERROR_SUCCESS The settings for the given hooked driver has been successfully
 *  changed to the new values.
 *  @value ERROR_INVALID_HANDLE The handle supplied in the first parameter is not valid.
 *  @value Other An error has occurred.
 *
 *  @remark
 *  If the IRPMon driver is currently monitoring activity related to the given driver,
 *  only the value stored in the MonitorNewDevices member of the DRIVER_MONITOR_SETTINGS
 *  structure passed in the Settings parameter takes effect.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllDriverSetInfo(HANDLE DriverHandle, PDRIVER_MONITOR_SETTINGS Settings);


/** Unhooks a given driver.
 *
 *  @param DriverHandle Handle to the hooked driver.
 *
 *  @return
 *  One of the following error codes may be returned:
 *  @value ERROR_SUCCESS The driver has been unhooked successfully.
 *  @value ERROR_INVALID_HANDLE The handle supplied in the parameter is not valid.
 *  @value Other An error has occurred.
 *
 *  @remark
 *  If the monitoring is active for the given driver, the routine fails. The monitoring
 *  must be stopped by a call to the @Link(IRPMonDllDriverStopMonitoring) functiion first.
 *
 *  The routine causes the IRPMon driver to forget all information about the given hooked
 *  driver which makes it being no longer hooked. The actual unhooking (removing hooks from
 *  driver's DRIVER_OBJECT structure) is done by the @link(IRPMonDllDriverStopMonitoring) routine.
 *
 *  After the successful call to this routine, the handle passed in the first argument to the call
 *  becomes invalid.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDriver(HANDLE DriverHandle);


/** Starts monitoring events related to a given device, identified by its object name.
 *
 *  @param DeviceName Name of the device object to be monitored. It can be obtained from
 *  a snapshot of drivers and devices currently present in the system, retrieved by a call
 *  to the @Link(IRPMonDllSnapshotRetrieve) function.
 *  @param HookHandle Address of variable that receives a handle representing the device
 *  being monitored.
 *
 *  @return
 *  The following error codes may be returned:
 *  @value ERROR_SUCCESS The operation has succeeded. The device is now being
 *  monitored.
 *  @value Other An error occurred.
 *
 *  @remark
 *  Before a call to this routine, the driver to which the device belongs must
 *  be hooked. Otherwise, the function fails. Drivers can be hooked by a call to the
 *  @link(IRPMonDllHookDriver) routine.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByName(PWCHAR DeviceName, PHANDLE HookHandle, PVOID *ObjectId);


/** Starts monitoring events related to a given device, identified by a kernel address of its object.
 *
 *  @param DeviceObject Address of the device object to be monitored. It can be obtained from
 *  a snapshot of drivers and devices currently present in the system, retrieved by a call
 *  to the @Link(IRPMonDllSnapshotRetrieve) function.
 *  @param HookHandle Address of variable that receives a handle representing the device
 *  being monitored.
 *
 *  @return
 *  The following error codes may be returned:
 *  @value ERROR_SUCCESS The operation has succeeded. The device is now being
 *  monitored.
 *  @value Other An error occurred.
 *
 *  @remark
 *  Before a call to this routine, the driver to which the device belongs must
 *  be hooked. Otherwise, the function fails. Drivers can be hooked by a call to the
 *  @link(IRPMonDllHookDriver) routine.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllHookDeviceByAddress(PVOID DeviceObject, PHANDLE HookHandle, PVOID *ObjectId);


/** Stops monitoring events related to a given device object.
 *
 *  @param HookHandle A handle representing the device object being monitored.
 *
 *  @return
 *  The routine can return one of the following values:
 *  @value ERROR_SUCCESS The operation has succeeded. The devices is not being
 *  monitored any more. The handle given in the argument is not valid any longer.
 *  @value ERROR_INVALID_HANDLE The given handle is invalid.
 *
 *  @remark
 *  If the whole driver to which the device belongs is unhooked (see @link(IRPMonDllUnhookDriver)), this
 *  routine needs not to be called since the device handle is automatically invalidated.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllUnhookDevice(HANDLE HookHandle);


/** Retrieves information about driver and device objects currently present
 *  in the system.
 *
 *  @param DriverInfo Address of variable that receives address of an array of
 *  pointers to IRPMON_DRIVER_INFO structures, each containing information about
 *  one driver and its devices.
 *  @param Count Address of variable that receives the number of structures in the 
 *  array.
 *
 *  @return
 *  The routine may return one of the following values:
 *  @value ERROR_SUCCESS The snapshot has been retrieved successfully.
 *  @value Other An error occurred.
 *
 *  @remark
 *  When the caller no longer needs the information retrieved by the routine,
 *  it must free it by calling the @link(IRPMonDllSnapshotFree) procedure.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllSnapshotRetrieve(PIRPMON_DRIVER_INFO **DriverInfo, PULONG Count);


/** Frees a given snapshot of drivers and their devices.
 *
 *  @param DriverInfo Array of pointers to IRPMON_DRIVER_INFO structures, each
 *  containing information about one driver and its devices. Address of the array
 *  is returned in the first parameter of a call to the @link(IRPMonDllSnapshotRetrieve)
 *  function.
 *  @param Count Number of entries in the array. This value is returned in the second parameter
 *  of the @link(IRPMonDllSnapshotRetrieve) call.
 */
IRPMONDLL_API VOID WINAPI IRPMonDllSnapshotFree(PIRPMON_DRIVER_INFO *DriverInfo, ULONG Count);


/** Connects the current thread (the calling one) to the queue
 *  of events detected by the IRPMon driver.
 *
 *  @param Semaphore Handle to a semaphore the counter of which is increased
 *  by one every time a new event is inserted into the queue.
 *
 *  @return
 *  The function returns one of the following error codes:
 *  @value ERROR_SUCCESS The thread successfully connected to the queue.
 *  @value Other An error occurred.
 *
 *  @remark
 *  At most one thread can be connected to the IRPMon Event Queue at any
 *  moment of time.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllConnect(HANDLE hSemaphore);


/** Disconnects the current thread from the IRPMon Event Queue.
 *
 *  @return
 *  ne of the following error codes may be returned:
 *  @value ERROR_SUCCESS The thread successfully disconnected from the queue.
 *  @value Other An error occurred.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllDisconnect(VOID);


/** Removes a request from the IRPMon Event Queue and copies it to a given
 *  buffer.
 *
 *  @param Request Address of buffer to which the request data will be copied.
 *  @param Size Size of the buffer, in bytes.
 *
 *  @return
 *  Returns one of the following values:
 *  @value ERROR_SUCCESS The request was successfully removed from the queue and
 *  copied to the given buffer. 
 *  @value ERROR_INSUFFICIENT_BUFFER The given buffer is not large enough to 
 *  hold all the request data. Because all types of requests are of fixed size,
 *  hence using a buffer large enough to hold the largest request type will avoid
 *  this error completely.
 *
 *  @remark
 *  The calling thread must be connected to the IRPMon Event Queue. Otherwise,
 *  the function fails.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllGetRequest(PREQUEST_HEADER Request, DWORD Size);


/** Get size of a request, in bytes.
 *
 *  @param Request Pointer to the request retrieved via @link(IRPMonDllGetRequest).
 *
 *  @returns
 *  If successful, returns the request size, in bytes.
 *  On error, zero is returned.
 *
 *  @remark
 *  The routine just looks at request type and performs necessary
 *  calculcations to obtain the total size, including general data
 *  associated with the request.
 */
IRPMONDLL_API size_t WINAPI IRPMonDllGetRequestSize(const REQUEST_HEADER *Request);

/** Open a handle to a given driver monitored by the IRPMon driver.
 *
 *  @param ObjectId ID of the target driver. IDs can be obtained from the
 *  'ObjectId' member of the @link(HOOKED_DRIVER_UMINFO) structure retrieved
 *  by the @link(IRPMonDllDriverHooksEnumerate) function.
 *  @param Handle Address of variable that receives the newly opened handle. The
 *  handle can be then used to control the hooked driver.
 *
 *  @return
 *  The function may return one of the following values:
 *    @value ERROR-SUCCESS The handle has been created successfully.
 *    @value Other An error occurred.
 *
 *  @remark
 *  When no longer needed, the handle should be closed via the @link(IRPMonDllCloseHookedDriverHandle)
 *  function. Alternatively, the @link(IRPMonDllUnhookDriver) routine may also be used to close the
 *  handle, however, it also unhooks the driver represented by the handle.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDriver(PVOID ObjectId, PHANDLE Handle);


/** Closes a handle to a given driver monitored by the IRPMon.
 *
 *  @param Handle The handle to close.
 *
 *  @return
 *  The routine should always return ERROR-SUCCESS. If it does not, the bug is
 9  in caller's code, not in the code of the library or driver.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllCloseHookedDriverHandle(HANDLE Handle);


/** Open a handle to a given device monitored by the IRPMon driver.
 *
 *  @param ObjectId ID of the target device. IDs can be obtained from the
 *  'ObjectId' member of the @link(HOOKED_DEVICE_UMINFO) structure retrieved
 *  by the @link(IRPMonDllDriverHooksEnumerate) function.
 *  @param Handle Address of variable that receives the newly opened handle. The
 *  handle can be then used to control the hooked device.
 *
 *  @return
 *  The function may return one of the following values:
 *    @value ERROR-SUCCESS The handle has been created successfully.
 *    @value Other An error occurred.
 *
 *  @remark
 *  When no longer needed, the handle should be closed via the @link(IRPMonDllCloseHookedDeviceHandle)
 *  function. Alternatively, the @link(IRPMonDllUnhookDevice) routine may also be used to close the
 *  handle, however, it also unhooks the device represented by the handle.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllOpenHookedDevice(PVOID ObjectId, PHANDLE Handle);


/** Closes a handle to a given device monitored by the IRPMon.
 *
 *  @param Handle The handle to close.
 *
 *  @return
 *  The routine should always return ERROR-SUCCESS. If it does not, the bug is
 9  in caller's code, not in the code of the library or driver.
 */
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
IRPMONDLL_API void WINAPI IRPMonDllRequestEmulatedFree(PREQUEST_HEADER Header);


/************************************************************************/
/*           INITIALIZATION AND FINALIZATION                            */
/************************************************************************/


/** @brief
 *  Checks whether the IRPMon library is initialized.
 *
 *  @return
 *  Returns a boolean value indicating whether the library is initialized.
 *
 *  @remark
 *  The library is considered initialized if and only if a connection to the IRPMon
 *  driver device is established.
 */
IRPMONDLL_API BOOL WINAPI IRPMonDllInitialized(VOID);


/** Initializes the IRPMon library and connects the current process to the
 *  IRPMon driver.
 *
 *  @return
 *  Returns one of the following error codes:
 *  @value ERROR_SUCCESS The operation succeeded.
 *  @value Other The initialization failed. No other library functions may be
 *  called.
 *
 *  @remark
 *  This routine must be successfully called before any other routine exported
 *  by the library.
 */
IRPMONDLL_API DWORD WINAPI IRPMonDllInitialize(VOID);


/** Disconnects the current process from the IRPMon driver and cleans up
 *  resources used by the library.
 *
 *  @remark
 *  After calling this routine, no other routine exported by the library can
 *  be successfully invoked.
 */
IRPMONDLL_API VOID WINAPI IRPMonDllFinalize(VOID);


#endif 
