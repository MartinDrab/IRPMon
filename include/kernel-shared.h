
#ifndef __KERNEL_SHARED_H__
#define __KERNEL_SHARED_H__


#include "general-types.h"

/************************************************************************/
/*                     REQUEST-RELATED STUFF                            */
/************************************************************************/


/************************************************************************/
/*                   OTHER DEFINITIONS                                  */
/************************************************************************/

/** Name of the communication device visible for user mode applications. */
#define IRPMNDRV_USER_DEVICE_NAME      L"\\\\.\\IRPMnDrv"

/************************************************************************/
/*                   HOOKED DRIVERS AND DEVICES                         */
/************************************************************************/

/** Contains information about one device monitored by the IRPMon driver. */
typedef struct _HOOKED_DEVICE_INFO {
	/** Size of this record, in bytes */
	ULONG EntrySize;
	/** ID of the object, used within the IRPMon driver. */
	PVOID ObjectId;
	/** Address of device's DEVICE_OBJECT structure. */
	PVOID DeviceObject;
	/** Indicates which types of fast I/O requests are monitored. THe exact
	   meaning of each entry is still undefined. */
	UCHAR FastIoSettings[FastIoMax];
	/** Indicates which types of IRP requests are monitored. THe exact
	   meaning of each entry is still undefined. 
	   NOTE: 0x1b = IRP_MJ_MAXIMUM_FUNCTION. */
	UCHAR IRPSettings[0x1b + 1];
	/** Indicates whether the monitoring is active for the device. */
	BOOLEAN MonitoringEnabled;
	/** Length of the device name, in bytes. The terminating null character is
	    included in the value. */
	ULONG DeviceNameLen;
	/** The first character of the name. */
	WCHAR DeviceName[1];
} HOOKED_DEVICE_INFO, *PHOOKED_DEVICE_INFO;

/** Contains information about one driver hooked by the IRPMon driver. */
typedef struct _HOOKED_DRIVER_INFO {
	/** Size of this structure. */
	ULONG EntrySize;
	/** ID of the object, used within the IRPMon driver. */
	PVOID ObjectId;
	/** Address of driver's DRIVER_OBJECT structure. */
	PVOID DriverObject;
	/** Indicates whether the IRPMon driver monitors events related to the target
	    driver. If set to TRUE, the information about the events is stored in the
		IRPMon Event Queue. */
	BOOLEAN MonitoringEnabled;
	DRIVER_MONITOR_SETTINGS MonitorSettings;
	/** Number of devices, monitored by the IRPMon driver (not including the new ones).
	 *  Each is described by a single @link(HOOKED_DEVICE_INFO) structure. The first
	 *  structure is placed just after his record.
	 */
	ULONG NumberOfHookedDevices;
	/** Length of the driver name, in bytes. The terminating null character is
	    included in the value. */
	ULONG DriverNameLen;
	/** The first character of the name. */
	WCHAR DriverName[1];
	// ... Name ...
	// HOOKED_DEVICE_INFO
} HOOKED_DRIVER_INFO, *PHOOKED_DRIVER_INFO;

/** Stores information about hooked devices and drivers. */
typedef struct _HOOKED_OBJECTS_INFO {
	/** Number of hooked drivers. Each is represented by a single @link(DRIVER_HOOK_INFO) record. 
	    The first record immediately follows this structure. */
	ULONG NumberOfHookedDrivers;
	/** Number of hooked devices. */
	ULONG NumberOfHookedDevices;
	// HOOKED_DRIVER_INFO
} HOOKED_OBJECTS_INFO, *PHOOKED_OBJECTS_INFO;

/************************************************************************/
/*                    WATCHES                                           */
/************************************************************************/

#define CLASS_WATCH_FLAG_BINARY					0x1
#define CLASS_WATCH_FLAG_UPPERFILTER			0x2
#define CLASS_WATCH_FLAG_BEGINNING				0x4
#define CLASS_WATCH_VALID_FLAGS					(CLASS_WATCH_FLAG_BINARY | CLASS_WATCH_FLAG_UPPERFILTER | CLASS_WATCH_FLAG_BEGINNING)

typedef struct _CLASS_WATCH_ENTRY {
	GUID ClassGuid;
	ULONG Flags;
} CLASS_WATCH_ENTRY, *PCLASS_WATCH_ENTRY;

typedef struct _DRIVER_NAME_WATCH_ENTRY {
	DRIVER_MONITOR_SETTINGS MonitorSettings;
	ULONG NameLength;
} DRIVER_NAME_WATCH_ENTRY, *PDRIVER_NAME_WATCH_ENTRY;


#endif 
