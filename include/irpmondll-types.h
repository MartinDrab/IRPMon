
#ifndef __IRPMONDLL_TYPES_H__
#define __IRPMONDLL_TYPES_H__

#include <windows.h>
#include "general-types.h"

/************************************************************************/
/*                         SNAPSHOT RETRIEVAL                           */
/************************************************************************/

/** Stores information about one device object existing in the system. */
typedef struct _IRPMON_DEVICE_INFO {
	/** Object address (address of its DEVICE_OBJECT structure). */
	PVOID DeviceObject;
	/** Address of device object attached to this one. NULL value means no device
	    is attached. */
	PVOID AttachedDevice;
	/** Name of the device. If the device has no name, this field points to a null
	    character. Will never be NULL. */
	PWCHAR Name;
} IRPMON_DEVICE_INFO, *PIRPMON_DEVICE_INFO;

/** Stores information about one driver existing in the system. */
typedef struct _IRPMON_DRIVER_INFO {
	/** Address of the driver (its DRIVER_OBJECT structure). */
	PVOID DriverObject;
	/** Driver name, will never be NULL. */
	PWCHAR DriverName;
	/** Number of device objects owned by the driver. */
	ULONG DeviceCount;
	/** Pointer to an array of pointers to @link(IRPMON_DEVICE_INFO) each
	    containing information about one device owned by the driver. Number
		of entries is reflected by the DeviceCount member. */
	PIRPMON_DEVICE_INFO *Devices;
} IRPMON_DRIVER_INFO, *PIRPMON_DRIVER_INFO;

/************************************************************************/
/*                  CLASS WATCHES                                       */
/************************************************************************/

typedef struct _CLASS_WATCH_RECORD {
	GUID ClassGuid;
	PWCHAR ClassGuidString;
	BOOLEAN UpperFilter;
	BOOLEAN Beginning;
} CLASS_WATCH_RECORD, *PCLASS_WATCH_RECORD;

/************************************************************************/
/*                DRIVER NAME WATCHES                                   */
/************************************************************************/

typedef struct _DRIVER_NAME_WATCH_RECORD {
	DRIVER_MONITOR_SETTINGS MonitorSettings;
	PWCHAR DriverName;
} DRIVER_NAME_WATCH_RECORD, *PDRIVER_NAME_WATCH_RECORD;



#endif 
