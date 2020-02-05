
#ifndef __IRPMON_HOOK_H__
#define __IRPMON_HOOK_H__

#include <ntifs.h>
#include "hash_table.h"
#include "kernel-shared.h"

typedef VOID (VOID_FUNCTION)(VOID);
typedef VOID_FUNCTION *PVOID_FUNCTION;

/** Determines why a given device hook record exists. */
typedef enum _EDeviceRecordCreateReason {
	/** Nobody knows. This value should never appear anywhere. */
	edrcrUnknown,
	/** The user requested to monitor the given device. The record has been created
	    in order to represent this activity. */
	edrcrUserRequest,
	/** The driver to which the device belongs has been hooked. The device record
	    has been created to indicate that the given device is not a new device. */
	edrcrDriverHooked,
} EDeviceRecordCreateReason, *PEDeviceRecordCreateReason;

/** Stores monitoring settings specific to a certain device. */
typedef struct _DEVICE_HOOK_RECORD {
	/** Number of references pointing to this record. */
	LONG ReferenceCount;
	/** Links the record to the hash table (stored in driver monitoring
	    settings). */
	HASH_ITEM HashItem;
	/** Links the record to the hash table mapping record addresses to them.
	    The table is used to validate whether a given address is an address
		of any device hook record. */
	HASH_ITEM ValidationHashItem;
	/** Address of the hook record for the driver owning the device */
	struct _DRIVER_HOOK_RECORD *DriverRecord;
	/** Address of device's DEVICE_OBJECT structure. */
	PDEVICE_OBJECT DeviceObject;
	/** Device name, for enumeration purposes mainly. */
	UNICODE_STRING DeviceName;
	/** Determines which IRP-based operations to monitor. */
	UCHAR IRPMonitorSettings[IRP_MJ_MAXIMUM_FUNCTION + 1]; 
	/** Determines which Fast I/O Operations to monitor. */
	UCHAR FastIoMonitorSettings[FastIoMax];
	/** Indicates whether a communication going through the device should be monitored. */
	BOOLEAN MonitoringEnabled;
	/** Determines why the device hook record was created. */
	EDeviceRecordCreateReason CreateReason;
	/** Proxy device object used to hook via device extension */
	PDEVICE_OBJECT ProxyDevice;
} DEVICE_HOOK_RECORD, *PDEVICE_HOOK_RECORD;

/** Contains information about a hook done on a given driver. */
typedef struct _DRIVER_HOOK_RECORD {
	/** Number of references pointing to this record. */
	LONG ReferenceCount;
	/** Stores the record in a hash table mapping addresses of DRIVER_OBJECT structures 
	    to these records. */
	HASH_ITEM HashItem;
	/** Links the record to the hash table mapping record addresses to them.
	    The table is used to validate whether a given address is an address
		of any driver hook record. */
	HASH_ITEM ValidationHashItem;
	/** Address of the driver's DRIVER_OBJECT structure. */
	PDRIVER_OBJECT DriverObject;
	/** Driver name, for enumeration purposes mainly. */
	UNICODE_STRING DriverName;
	/** Old pointers from driver's MajorFunction array. */
	PDRIVER_DISPATCH OldMajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
	/** Old Fast IO Dispatch structure of the driver. */
	FAST_IO_DISPATCH OldFastIoDisptach;
	/** TRUE if the target driver has Fast I/O dispatch, FALSE otherwise. */
	BOOLEAN FastIoPresent;
	/** Address of driver's AddDevice routine. */
	PDRIVER_ADD_DEVICE OldAddDevice;
	/** Determines whether the driver has its AddDevice routine set. */
	BOOLEAN AddDevicePresent;
	/** Determines whether driver's AddDevice routine is being monitored (and thus, hooked). */
	BOOLEAN MonitorAddDevice;
	/** Address of driver's DriverUnload routine. */
	PDRIVER_UNLOAD OldDriverUnload;
	/** Determines whether the driver has its DriverUnload routine set. */
	BOOLEAN DriverUnloadPresent;
	/** Stores information whether the DriverUnload routine of the driver should be monitored. */
	BOOLEAN MonitorDriverUnload;
	/** Determines whether the driver has a StartIo routine. */
	BOOLEAN StartIoPresent;
	/** Determines whether to monitor driver's StartIo routine. */
	BOOLEAN MonitorStartIo;
	/** Stores driver's original StartIo routine. */
	PDRIVER_STARTIO OldStartIo;
	/** Determines whether a new devices created by the target driver should
	    be automatically included into the monitoring. */
	BOOLEAN MonitorNewDevices;
	BOOLEAN MonitorFastIo;
	BOOLEAN MonitorIRP;
	BOOLEAN MonitorIRPCompletion;
	BOOLEAN MonitorData;
	BOOLEAN DeviceExtensionHook;
	UCHAR IRPSettings[IRP_MJ_MAXIMUM_FUNCTION + 1];
	UCHAR FastIoSettings[FastIoMax];
	/** Indicates whether the driver actively monitors incoming requests. */
	BOOLEAN MonitoringEnabled;
	/** Synchronizes access to the device table. */
	KSPIN_LOCK SelectedDevicesLock;
	/** Contains information about monitoring of individual devices. */
	PHASH_TABLE SelectedDevices;
} DRIVER_HOOK_RECORD, *PDRIVER_HOOK_RECORD;

VOID DriverHookRecordReference(PDRIVER_HOOK_RECORD Record);
VOID DriverHookRecordDereference(PDRIVER_HOOK_RECORD Record);
BOOLEAN DriverHookRecordValid(PDRIVER_HOOK_RECORD DriverRecord);
VOID DeviceHookRecordReference(PDEVICE_HOOK_RECORD Record);
VOID DeviceHookRecordDereference(PDEVICE_HOOK_RECORD Record);
BOOLEAN DeviceHookRecordValid(PDEVICE_HOOK_RECORD DeviceRecord);

NTSTATUS HookDriverObject(PDRIVER_OBJECT DriverObject, PDRIVER_MONITOR_SETTINGS MonitorSettings, BOOLEAN DeviceExtensionHook, PDRIVER_HOOK_RECORD *DriverRecord);
NTSTATUS UnhookDriverObject(PDRIVER_HOOK_RECORD DriverRecord);
NTSTATUS DriverHookRecordSetInfo(PDRIVER_HOOK_RECORD Record, PDRIVER_MONITOR_SETTINGS DriverSettings);
VOID DriverHookRecordGetInfo(PDRIVER_HOOK_RECORD Record, PDRIVER_MONITOR_SETTINGS DriverSettings, PBOOLEAN Enabled);
NTSTATUS DriverHookRecordEnable(PDRIVER_HOOK_RECORD Record, BOOLEAN Enable);
PDRIVER_HOOK_RECORD DriverHookRecordGet(PDRIVER_OBJECT DriverObject);

NTSTATUS DriverHookRecordAddDevice(PDRIVER_HOOK_RECORD Record, PDEVICE_OBJECT DeviceObject, PUCHAR IRPSettings, PUCHAR FastIoSettings, BOOLEAN MonitoringEanbled, PDEVICE_HOOK_RECORD *DeviceRecord);
NTSTATUS DriverHookRecordDeleteDevice(PDEVICE_HOOK_RECORD DeviceRecord);
PDEVICE_HOOK_RECORD DriverHookRecordGetDevice(PDRIVER_HOOK_RECORD Record, PDEVICE_OBJECT DeviceObject);
NTSTATUS DeviceHookRecordSetInfo(PDEVICE_HOOK_RECORD Record, PUCHAR IRPSettings, PUCHAR FastIoSettings, BOOLEAN MonitoringEnabled);
VOID DeviceHookRecordGetInfo(PDEVICE_HOOK_RECORD Record, PUCHAR IRPSettings, PUCHAR FastIoSettings, PBOOLEAN MonitoringEnabled);

NTSTATUS HookObjectsEnumerate(PVOID Buffer, ULONG BufferLength, PULONG ReturnLength);

NTSTATUS HookModuleInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);
VOID HookModuleFinit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, PVOID Context);


#endif 
