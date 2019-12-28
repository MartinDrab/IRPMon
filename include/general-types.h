
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
	ertDriverDetected,
	ertDeviceDetected,
	ertFileObjectNameAssigned,
	ertFileObjectNameDeleted,
	ertProcessCreated,
	ertProcessExitted,
} ERequesttype, *PERequestPype;

/** Determines the type returned in the Result union of the @link(REQUEST_HEADER) structure. */
typedef enum _ERequestResultType {
	/** The result value is either not yet initialized, or not defined for a given request type. */
	rrtUndefined,
	/** The type is NTSTATUS. */
	rrtNTSTATUS,
	/** The type is BOOLEAN. */
	rrtBOOLEAN,
} ERequestResultType, *PERequstResultType;

#define REQUEST_FLAG_EMULATED				0x1
#define REQUEST_FLAG_DATA_STRIPPED			0x2
#define REQUEST_FLAG_ADMIN					0x4
#define REQUEST_FLAG_IMPERSONATED			0x8
#define REQUEST_FLAG_IMPERSONATED_ADMIN		0x10
#define REQUEST_FLAG_NEXT_AVAILABLE			0x20
#define REQUEST_FLAG_COMPRESSED				0x40
#define REQUEST_FLAG_PAGED					0x80
#define REQUEST_FLAG_NONPAGED				0x100


/** Header, containing information common for all request types. */
typedef struct _REQUEST_HEADER {
	LIST_ENTRY Entry;
	/** Date and time of the request's detection (in 100 nanosecond 
	    units from January 1 1601). */
	LARGE_INTEGER Time;
	/** Type of the request. */
	ERequesttype Type;
	/** Unique identifier of the request. ID of a new request is always
	    greater than ID of already existing ones, so the ID also follows
		the order in which the requests were created. */
	ULONG Id;
	/** Device object associated with the request. */
	PVOID Device;
	/** Driver object associated with the request. */
	PVOID Driver;
	HANDLE ProcessId;
	HANDLE ThreadId;
	/** Various flags related to the request. */
	USHORT Flags;
	UCHAR Irql;
	/** Result of the request servicing. The type of this field
	    differs depending the type of the request. 
		
		 * NTSTATUS (ertIRP, ertAddDevice, ertStartIo).
		 * Not relevant to the request type (ertDriverUnload, erpUndefined).
		 * BOOLEAN for most Fast I/Os
		@todo Dopsat fast I/O
	  */
	ERequestResultType ResultType;
	union {
		NTSTATUS NTSTATUSValue;
		BOOLEAN BOOLEANValue;
		PVOID Other;
	} Result;
} REQUEST_HEADER, *PREQUEST_HEADER;

/** @brief
 *  Sets results of a given request, both its value and type. The result is written to the header.
 *  
 *  @param aHeader Header of the request. 
 *  @param aRequestType Data type of the request (BOOLEAN or NTSTATUS).
 *  @param aRequestValue Value of the result.
 */
#define RequestHeaderSetResult(aHeader, aResultType, aResultValue)			\
	{																		\
		(aHeader).ResultType = rrt##aResultType;								\
		(aHeader).Result.aResultType##Value = (aResultValue);							\
	}																		\

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
	/** Irp->Flags */
	ULONG IrpFlags;
	/** Irp->FileObject*/
	PVOID FileObject;
	/** The first argument of the request. */
	PVOID Arg1;
	/** The second argument of the request. */
	PVOID Arg2;
	/** The third argument of the request. */
	PVOID Arg3;
	/** The fourth argument of the request. */
	PVOID Arg4;
	/** Value of the Irp->IoStatus.Status at time of IRP detection. */
	NTSTATUS IOSBStatus;
	/** Value of the Irp->IoStatus.Information at time of IRP detection. */
	ULONG_PTR IOSBInformation;
	/** PID of the process originally requesting the operation. */
	ULONG_PTR RequestorProcessId;
	/** Number of data bytes associated with the request. */
	SIZE_T DataSize;
	// Data bytes
} REQUEST_IRP, *PREQUEST_IRP;

typedef struct _REQUEST_IRP_COMPLETION {
	REQUEST_HEADER Header;
	PVOID IRPAddress;
	NTSTATUS CompletionStatus;
	ULONG_PTR CompletionInformation;
	ULONG MajorFunction;
	ULONG MinorFunction;
	PVOID Arguments[4];
	PVOID FileObject;
	ULONG_PTR DataSize;
	// Data
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
	/** Length of data associated with the request. */
	SIZE_T DataSize;
	// Data
} REQUEST_STARTIO, *PREQUEST_STARTIO;

typedef struct _REQUEST_DRIVER_DETECTED {
	/** Request header. */
	REQUEST_HEADER Header;
	/** Length of the detected driver object name, in bytes. */
	ULONG DriverNameLength;
} REQUEST_DRIVER_DETECTED, *PREQUEST_DRIVER_DETECTED;

typedef struct _REQUEST_DEVICE_DETECTED {
	/** Request header. */
	REQUEST_HEADER Header;
	/** Length of the detected device object name, in bytes. */
	ULONG DeviceNameLength;
} REQUEST_DEVICE_DETECTED, *PREQUEST_DEVICE_DETECTED;

typedef struct _REQUEST_PROCESS_CREATED {
	REQUEST_HEADER Header;
	HANDLE ProcessId;
	HANDLE ParentId;
	HANDLE CreatorId;
	ULONG ImageNameLength;
	ULONG CommandLineLength;
} REQUEST_PROCESS_CREATED, *PREQUEST_PROCESS_CREATED;

typedef struct _REQUEST_PROCESS_EXITTED {
	REQUEST_HEADER Header;
	HANDLE ProcessId;
} REQUEST_PROCESS_EXITTED, *PREQUEST_PROCESS_EXITTED;

typedef struct _REQUEST_FILE_OBJECT_NAME_ASSIGNED {
	REQUEST_HEADER Header;
	void *FileObject;
	ULONG NameLength;
} REQUEST_FILE_OBJECT_NAME_ASSIGNED, *PREQUEST_FILE_OBJECT_NAME_ASSIGNED;

typedef struct _REQUEST_FILE_OBJECT_NAME_DELETED {
	REQUEST_HEADER Header;
	void *FileObject;
} REQUEST_FILE_OBJECT_NAME_DELETED, *PREQUEST_FILE_OBJECT_NAME_DELETED;

typedef struct _REQUEST_GENERAL {
	union {
		REQUEST_HEADER Other;
		REQUEST_IRP Irp;
		REQUEST_IRP_COMPLETION IrpComplete;
		REQUEST_FASTIO FastIo;
		REQUEST_ADDDEVICE AddDevice;
		REQUEST_UNLOAD DriverUnload;
		REQUEST_STARTIO StartIo;
		REQUEST_DRIVER_DETECTED DriverDetected;
		REQUEST_DEVICE_DETECTED DeviceDetected;
		REQUEST_PROCESS_CREATED ProcessCreated;
		REQUEST_PROCESS_EXITTED ProcessExitted;
		REQUEST_FILE_OBJECT_NAME_ASSIGNED FileObjectNameAssigned;
		REQUEST_FILE_OBJECT_NAME_DELETED FileObjectNameDeleted;
	} RequestTypes;
} REQUEST_GENERAL, *PREQUEST_GENERAL;

typedef struct _REQUEST_CREATE_IRP_ETRA_PARAMETERS {
	BOOLEAN Admin;
	BOOLEAN Impersonated;
	BOOLEAN EffectiveOnly;
	BOOLEAN CopyOnOpen;
	SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
} REQUEST_CREATE_IRP_ETRA_PARAMETERS, *PREQUEST_CREATE_IRP_ETRA_PARAMETERS;

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
	BOOLEAN MonitorData;
	/** IRPSettings for newly hooked devices. */
	UCHAR IRPSettings[0x1b + 1];
	/** FastIoSettings for newly hooked devices. */
	UCHAR FastIoSettings[FastIoMax];
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


/************************************************************************/
/*                GLOBAL DRIVER SETTINGS                                */
/************************************************************************/


typedef struct _IRPMNDRV_SETTINGS {
	volatile LONG ReqQueueLastRequestId;
	volatile LONG ReqQueueLength;
	volatile LONG ReqQueueNonPagedLength;
	volatile LONG ReqQueuePagedLength;
	BOOLEAN ReqQueueConnected;
	BOOLEAN ReqQueueClearOnDisconnect;
	BOOLEAN ReqQueueCollectWhenDisconnected;
	BOOLEAN ProcessEventsCollect;
	BOOLEAN FileObjectEventsCollect;
	BOOLEAN DriverSnapshotEventsCollect;
	BOOLEAN ProcessEmulateOnConnect;
	BOOLEAN DriverSnapshotOnConnect;
} IRPMNDRV_SETTINGS, *PIRPMNDRV_SETTINGS;



#endif 
