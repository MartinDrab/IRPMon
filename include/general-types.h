
#ifndef __IRPMONDLL_GENERAL_TYPES_H__
#define __IRPMONDLL_GENERAL_TYPES_H__


/************************************************************************/
/*                     OPERATION TYPES                                  */
/************************************************************************/

/** Enumerates all possible types of Fast I/O operations. */
typedef enum _EFastIoOperationType {
	FastIoCheckIfPossible = 0,
	FastIoRead,
	FastIoWrite,
	FastIoQueryBasicInfo,
	FastIoQueryStandardInfo,
	FastIoLock,
	FastIoUnlockSingle,
	FastIoUnlockAll,
	FastIoUnlockAllByKey,
	FastIoDeviceControl,
	AcquireFileForNtCreateSection,
	ReleaseFileForNtCreateSection,
	FastIoDetachDevice,
	FastIoQueryNetworkOpenInfo,
	AcquireForModWrite,
	MdlRead,
	MdlReadComplete,
	PrepareMdlWrite,
	MdlWriteComplete,
	FastIoReadCompressed,
	FastIoWriteCompressed,
	MdlReadCompleteCompressed,
	MdlWriteCompleteCompressed,
	FastIoQueryOpen,
	ReleaseForModWrite,
	AcquireForCcFlush,
	ReleaseForCcFlush,
	FastIoMax
} EFastIoOperationType, *PEFastIoOperationType;

/************************************************************************/
/*                     REQUEST TYPES                                    */
/************************************************************************/

/** Type of request reported by the IRPMon driver. */
typedef enum _ERequestType {
	/** Exists only for debugging purposes, should not be ever used. */
	erpUndefined,
	/** I/O request packet (IRP). */
	ertIRP,
	/** An IRP is completed. */
	ertIRPCompletion,
	/** Driver's AddDevice routine was called in order to inform the driver about a newly detected device. */
	ertAddDevice,
	/** A driver was unloaded. */
	ertDriverUnload,
	/** A fast I/O request was serviced by a driver. */
	ertFastIo,
	/** Driver's StartIo routine was invoked. */
	ertStartIo,
} ERequesttype, *PERequestPype;

/** Header, containing information common for all request types. */
typedef struct _REQUEST_HEADER {
	LIST_ENTRY Entry;
	/** Date and time of the request's detection (in 100 nanosecond 
	    units from January 1 1601). */
	LARGE_INTEGER Time;
	/** Type of the request. */
	ERequesttype Type;
	/** Device object associated with the request. */
	PVOID Device;
	/** Driver object associated with the request. */
	PVOID Driver;
	/** Result of the request servicing. The type of this field 
	    differs depending the type of the request. 
		
		 * NTSTATUS (ertIRP, ertAddDevice, ertStartIo).
		 * Not relevant to the request type (ertDriverUnload, erpUndefined).
		
		@todo Dopsat fast I/O
	  */
	PVOID Result;
} REQUEST_HEADER, *PREQUEST_HEADER;

/** Represents an IRP request. */
typedef struct _REQUEST_IRP {
	/** The header. */
	REQUEST_HEADER Header;
	/** The major function (IRP_MJ_XXX constants). */
	UCHAR MajorFunction;
	/** The minor function (related to the major function), IRP_MN_XXX constants. */
	UCHAR MinorFunction;
	/** Value of the ExGetPreviousMode at the time of the request retrieval. */
	UCHAR PreviousMode;
	/** Indicates whether the creator of the request is sent by user application
	    or by a kernel component. */
	UCHAR RequestorMode;
	/** Address of the IRP structure representing the request. */
	PVOID IRPAddress;
	/** The first argument of the request. */
	PVOID Arg1;
	/** The second argument of the request. */
	PVOID Arg2;
	/** The third argument of the request. */
	PVOID Arg3;
	/** The fourth argument of the request. */
	PVOID Arg4;
	/** PID of the process in context of which the request was received. */
	PVOID ProcessId;
	/** TID of the thread in context of which the request was received. */
	PVOID ThreadId;
	/** Irp->Flags */
	ULONG IrpFlags;
	/** Irp->FileObject*/
	PVOID FileObject;
} REQUEST_IRP, *PREQUEST_IRP;

typedef struct _REQUEST_IRP_COMPLETION {
	REQUEST_HEADER Header;
	PVOID IRPAddress;
	NTSTATUS CompletionStatus;
	ULONG_PTR CompletionInformation;
	HANDLE ProcessId;
	HANDLE ThreadId;
} REQUEST_IRP_COMPLETION, *PREQUEST_IRP_COMPLETION;

/** Represents a fast I/O request. */
typedef struct _REQUEST_FASTIO {
	/** Request header. */
	REQUEST_HEADER Header;
	/** Type of the fast I/O operation. */
	EFastIoOperationType FastIoType;
	/** Indicates whether the operation was performed by a user aplication or
	    by a kernel component. */
	UCHAR PreviousMode;
	/** The first argument of the operation. */
	PVOID Arg1;
	/** The second argument of the operation. */
	PVOID Arg2;
	/** The third argument of the operation. */
	PVOID Arg3;
	/** The fourth argument of the operation. */
	PVOID Arg4;
	PVOID Arg5;
	PVOID Arg6;
	PVOID Arg7;
	PVOID Arg8;
	PVOID Arg9;
	/** PID of the process in context of which the operation was requested. */
	PVOID ProcessId;
	/** TID of the thread in context of which the operation was requested. */
	PVOID ThreadId;
	PVOID FileObject;
	LONG IOSBStatus;
	ULONG_PTR IOSBInformation;
} REQUEST_FASTIO, *PREQUEST_FASTIO;

/** Represent an AddDevice event indicating that an AddDevice routine of a
    driver monitored by the IRPMon was executed. */
typedef struct _REQUEST_ADDDEVICE {
	/** Request header. */
	REQUEST_HEADER Header;
} REQUEST_ADDDEVICE, *PREQUEST_ADDDEVICE;

/** Represents an event reporting a driver unload. */
typedef struct _REQUEST_UNLOAD {
	/** Request header. */
	REQUEST_HEADER Header;
} REQUEST_UNLOAD, *PREQUEST_UNLOAD;

/** Represents an event reporting an invocation of driver's StartIo routine. */
typedef struct _REQUEST_STARTIO {
	/** Request header. */
	REQUEST_HEADER Header;
	/** Address of an IRP structure passed to the routine. */
	PVOID IRPAddress;
	/** Major type of the IRP (IRP_MJ_XXX constant). */
	UCHAR MajorFunction;
	/** Minor type of the IRP (IRP_MN_XXX constant). */
	UCHAR MinorFunction;
	/** The first argument of the operation. */
	PVOID Arg1;
	/** The second argument of the operation. */
	PVOID Arg2;
	/** The third argument of the operation. */
	PVOID Arg3;
	/** The fourth argument of the operation. */
	PVOID Arg4;
	ULONG IrpFlags;
	PVOID FileObject;
	/** Value of the Irp->IoStatus.Information after calling the original
	    dispatch routine. */
	ULONG_PTR Information;
	/** Value of the Irp->IoStatus.Status after calling the original
	    dispatch routine. */
	LONG Status;
} REQUEST_STARTIO, *PREQUEST_STARTIO;

typedef struct _REQUEST_GENERAL {
	union {
		REQUEST_HEADER Header;
		REQUEST_IRP Irp;
		REQUEST_IRP_COMPLETION IrpComplete;
		REQUEST_FASTIO FastIo;
		REQUEST_ADDDEVICE AddDevice;
		REQUEST_UNLOAD DriverUnload;
		REQUEST_STARTIO StartIo;
	} RequestTypes;
} REQUEST_GENERAL, *PREQUEST_GENERAL;

/************************************************************************/
/*                     HOOKED DRIVERS AND DEVICES                       */
/************************************************************************/


/** Contains information about one device monitored by the IRPMon driver. */
typedef struct _HOOKED_DEVICE_UMINFO {
	/** ID of the object, used within the IRPMon driver. */
	PVOID ObjectId;
	/** Address of device's DEVICE_OBJECT structure. */
	PVOID DeviceObject;
	/** Name of the hooked device. Can never be NULL. */
	PWCHAR DeviceName;
	/** Length of the device name, in bytes. The value does not include the
	    terminating null character. */
	ULONG DeviceNameLen;
	/** Indicates which types of fast I/O requests are monitored. THe exact
	   meaning of each entry is still undefined. */
	UCHAR FastIoSettings[FastIoMax];
	/** Indicates which types of IRP requests are monitored. THe exact
	   meaning of each entry is still undefined. 
	   NOTE: 0x1b = IRP_MJ_MAXIMUM_FUNCTION. */
	UCHAR IRPSettings[0x1b + 1];
	/** Indicates whether the monitoring is active for the device. */
	BOOLEAN MonitoringEnabled;
} HOOKED_DEVICE_UMINFO, *PHOOKED_DEVICE_UMINFO;

typedef struct _DRIVER_MONITOR_SETTINGS {
	BOOLEAN MonitorNewDevices;
	BOOLEAN MonitorAddDevice;
	BOOLEAN MonitorStartIo;
	BOOLEAN MonitorUnload;
	BOOLEAN MonitorFastIo;
	BOOLEAN MonitorIRP;
	BOOLEAN MonitorIRPCompletion;
} DRIVER_MONITOR_SETTINGS, *PDRIVER_MONITOR_SETTINGS;

/** Contains information about one driver hooked by the IRPMon driver. */
typedef struct _HOOKED_DRIVER_UMINFO {
	/** ID of the object, used within the IRPMon driver. */
	PVOID ObjectId;
	/** Address of driver's DRIVER_OBJECT structure. */
	PVOID DriverObject;
	/** Name of the driver. Cannot be NULL. */
	PWCHAR DriverName;
	/* Length of the driver name, in bytes. The value does not include the
	   terminating null-character. */
	ULONG DriverNameLen;
	/** Indicates whether the IRPMon driver monitors events related to the target
	    driver. If set to TRUE, the information about the events is stored in the
		IRPMon Event Queue. */
	BOOLEAN MonitoringEnabled;
	DRIVER_MONITOR_SETTINGS MonitorSettings;
	/** Number of devices, monitored by the IRPMon driver (not including the new ones). */
	ULONG NumberOfHookedDevices;
	/** An array of @link(HOOKED_DEVICE_UMINFO) structures, each representing one
	   monitored device. */
	PHOOKED_DEVICE_UMINFO HookedDevices;
} HOOKED_DRIVER_UMINFO, *PHOOKED_DRIVER_UMINFO;




#endif 
