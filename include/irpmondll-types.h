
#ifndef __IRPMONDLL_TYPES_H__
#define __IRPMONDLL_TYPES_H__

#include <stdint.h>
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


/// Contains information about one device setup class watched by the
/// IRPMon driver.
typedef struct _CLASS_WATCH_RECORD {
	/// CLass GUID, in its binary form.
	GUID ClassGuid;
	/// Class GUID, in its string form.
	PWCHAR ClassGuidString;
	/// Indicates whether the driver installed itself as an upper filter
	/// (<c>TRUE</c>) or lower filter (<c>FALSE</c>) for the class.
	BOOLEAN UpperFilter;
	/// Determines whether the driver is installed as the first (<c>TRUE</c>)
	/// or last (<c>FALSE</c>) filter driver in the filter list.
	BOOLEAN Beginning;
} CLASS_WATCH_RECORD, *PCLASS_WATCH_RECORD;

/************************************************************************/
/*                DRIVER NAME WATCHES                                   */
/************************************************************************/

/// Contains information about one watched driver name.
typedef struct _DRIVER_NAME_WATCH_RECORD {
	/// Monitoring settings to apply when the driver with the specified
	/// name is detected.
	DRIVER_MONITOR_SETTINGS MonitorSettings;
	/// Name of the driver object, in absolute format (Object Manager namespace).
	PWCHAR DriverName;
} DRIVER_NAME_WATCH_RECORD, *PDRIVER_NAME_WATCH_RECORD;

/************************************************************************/
/*              LIBRARY INITIALIZATION DATA TYPES                       */
/************************************************************************/

/// Defines how the IRPMon library should connect
/// to the driver instance.
typedef enum _EIRPMonConnectorType {
	/// This value should never be used.
	ictNone,
	/// Connect to device object of the locally installed IRPMon driver.
	ictDevice,
	/// Connect to an IRPMon instance running on a remote computer.
	ictNetwork,
	/// Connect via VMWare vSockets
	ictVSockets,
	/// Hyper-V
	ictHyperV,
} EConnectorType, *PEConnectorType;

/// Defines parameter necessary for connecting to IRPMon driver's control device.
typedef struct _IRPMON_DEVICE_INIT_INFO {
	/// Specifies name of the device object belonging to the
	/// local IRPMon driver.
	wchar_t *DeviceName;
} IRPMON_DEVICE_INIT_INFO, *PIRPMON_DEVICE_INIT_INFO;

/// Defines parameter necessary for connecting to IRPMon instance over
/// a network.
typedef struct _IRPMON_NETWORK_INIT_INFO {
	/// Domain or IP address where the remote IRPMon instance
	/// is running.
	wchar_t *Address;
	/// Port number (in its string form) at which the instance listens.
	wchar_t *Service;
	/// Version of IP protocol used to establish the connection.
	/// <list type="bullets">
	/// <item>
	///   <term>AF_UNSPEC</term>
	/// </item>
	/// <item>
	///   <term>AF_INET</term>
	/// </item>
	/// <item>
	///   <term>AF_INET6</term>
	/// </item>
	/// </list>
	uint16_t AddressFamily;
} IRPMON_NETWORK_INIT_INFO, *PIRPMON_NETWORK_INIT_INFO;

typedef struct _IRPMON_VSOCKETS_INIT_INFO {
	unsigned int CID;
	unsigned int Port;
} IRPMON_VSOCKETS_INIT_INFO, *PIRPMON_VSOCKETS_INIT_INFO;

typedef struct _IRPMON_HYPERV_INIT_INFO {
	GUID VMId;
	GUID AppId;
} IRPMON_HYPERV_INIT_INFO, *PIRPMON_HYPERV_INIT_INFO;

/// Defines data specific to individual connection methods.
typedef union _IRPMON_INIT_INFO_DATA {
	/// Parameters for connecting to a local IRPMon driver instance.
	IRPMON_DEVICE_INIT_INFO Device;
	/// Parameters for making network connections.
	IRPMON_NETWORK_INIT_INFO Network;
	/// Parameters for making a connection through vSockets
	IRPMON_VSOCKETS_INIT_INFO VSockets;
	/// Parameters for making a connection using Hyper-V
	IRPMON_HYPERV_INIT_INFO HyperV;
} IRPMON_INIT_INFO_DATA, *PIRPMON_INIT_INFO_DATA;

/// Information required to initialize the IRPMon library.
typedef struct _IRPMON_INIT_INFO {
	/// Defines how to connect to the driver instance.
	EConnectorType ConnectorType;
	/// Data specific to the selected connection type.
	IRPMON_INIT_INFO_DATA Data;
} IRPMON_INIT_INFO, *PIRPMON_INIT_INFO;

typedef DWORD (DRIVERCOMM_SYNC_IOCTL)(DWORD Code, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
typedef DWORD(DRIVERCOMM_CONNECT)(const IRPMON_INIT_INFO *Info);
typedef void(DRIVERCOMM_DISCONNECT)(void);
typedef BOOL(DRIVERCOMM_CONNECTED)(void);

typedef struct _IRPMON_DRIVER_COMM_INTERFACE {
	EConnectorType InterfaceType;
	DRIVERCOMM_SYNC_IOCTL *SynchronousIoctl;
	DRIVERCOMM_CONNECT *Connect;
	DRIVERCOMM_DISCONNECT *Disconnect;
	DRIVERCOMM_CONNECTED *Connected;
} IRPMON_DRIVER_COMM_INTERFACE, *PIRPMON_DRIVER_COMM_INTERFACE;




#endif 
