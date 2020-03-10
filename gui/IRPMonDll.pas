Unit IRPMonDll;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

{$Z4}
{$MINENUMSIZE 4}

Interface

Uses
  Windows;

Const
  FILE_SUPERSEDE                 = $00000000;
  FILE_OPEN                      = $00000001;
  FILE_CREATE                    = $00000002;
  FILE_OPEN_IF                   = $00000003;
  FILE_OVERWRITE                 = $00000004;
  FILE_OVERWRITE_IF              = $00000005;

  FILE_DIRECTORY_FILE                     = $00000001;
  FILE_WRITE_THROUGH                      = $00000002;
  FILE_SEQUENTIAL_ONLY                    = $00000004;
  FILE_NO_INTERMEDIATE_BUFFERING          = $00000008;
  FILE_SYNCHRONOUS_IO_ALERT               = $00000010;
  FILE_SYNCHRONOUS_IO_NONALERT            = $00000020;
  FILE_NON_DIRECTORY_FILE                 = $00000040;
  FILE_CREATE_TREE_CONNECTION             = $00000080;
  FILE_COMPLETE_IF_OPLOCKED               = $00000100;
  FILE_NO_EA_KNOWLEDGE                    = $00000200;
  FILE_OPEN_REMOTE_INSTANCE               = $00000400;
  FILE_RANDOM_ACCESS                      = $00000800;
  FILE_DELETE_ON_CLOSE                    = $00001000;
  FILE_OPEN_BY_FILE_ID                    = $00002000;
  FILE_OPEN_FOR_BACKUP_INTENT             = $00004000;
  FILE_NO_COMPRESSION                     = $00008000;
  FILE_OPEN_REQUIRING_OPLOCK              = $00010000;
  FILE_DISALLOW_EXCLUSIVE                 = $00020000;
  FILE_SESSION_AWARE                      = $00040000;
  FILE_RESERVE_OPFILTER                   = $00100000;
  FILE_OPEN_REPARSE_POINT                 = $00200000;
  FILE_OPEN_NO_RECALL                     = $00400000;
  FILE_OPEN_FOR_FREE_SPACE_QUERY          = $00800000;


  LABEL_SECURITY_INFORMATION                 = $00000010;
  ATTRIBUTE_SECURITY_INFORMATION             = $00000020;
  SCOPE_SECURITY_INFORMATION                 = $00000040;
  PROCESS_TRUST_LABEL_SECURITY_INFORMATION   = $00000080;
  ACCESS_FILTER_SECURITY_INFORMATION         = $00000100;
  BACKUP_SECURITY_INFORMATION                = $00010000;

  PROTECTED_DACL_SECURITY_INFORMATION        = $80000000;
  PROTECTED_SACL_SECURITY_INFORMATION        = $40000000;
  UNPROTECTED_DACL_SECURITY_INFORMATION      = $20000000;
  UNPROTECTED_SACL_SECURITY_INFORMATION      = $10000000;

Const
  FIleInformationClassArray : Array [0..76] Of WideString = (
    '0',
    'FileDirectoryInformation',
    'FileFullDirectoryInformation',                   // 2
    'FileBothDirectoryInformation',                   // 3
    'FileBasicInformation',                           // 4
    'FileStandardInformation',                        // 5
    'FileInternalInformation',                        // 6
    'FileEaInformation',                              // 7
    'FileAccessInformation',                          // 8
    'FileNameInformation',                            // 9
    'FileRenameInformation',                          // 10
    'FileLinkInformation',                            // 11
    'FileNamesInformation',                           // 12
    'FileDispositionInformation',                     // 13
    'FilePositionInformation',                        // 14
    'FileFullEaInformation',                          // 15
    'FileModeInformation',                            // 16
    'FileAlignmentInformation',                       // 17
    'FileAllInformation',                             // 18
    'FileAllocationInformation',                      // 19
    'FileEndOfFileInformation',                       // 20
    'FileAlternateNameInformation',                   // 21
    'FileStreamInformation',                          // 22
    'FilePipeInformation',                            // 23
    'FilePipeLocalInformation',                       // 24
    'FilePipeRemoteInformation',                      // 25
    'FileMailslotQueryInformation',                   // 26
    'FileMailslotSetInformation',                     // 27
    'FileCompressionInformation',                     // 28
    'FileObjectIdInformation',                        // 29
    'FileCompletionInformation',                      // 30
    'FileMoveClusterInformation',                     // 31
    'FileQuotaInformation',                           // 32
    'FileReparsePointInformation',                    // 33
    'FileNetworkOpenInformation',                     // 34
    'FileAttributeTagInformation',                    // 35
    'FileTrackingInformation',                        // 36
    'FileIdBothDirectoryInformation',                 // 37
    'FileIdFullDirectoryInformation',                 // 38
    'FileValidDataLengthInformation',                 // 39
    'FileShortNameInformation',                       // 40
    'FileIoCompletionNotificationInformation',        // 41
    'FileIoStatusBlockRangeInformation',              // 42
    'FileIoPriorityHintInformation',                  // 43
    'FileSfioReserveInformation',                     // 44
    'FileSfioVolumeInformation',                      // 45
    'FileHardLinkInformation',                        // 46
    'FileProcessIdsUsingFileInformation',             // 47
    'FileNormalizedNameInformation',                  // 48
    'FileNetworkPhysicalNameInformation',             // 49
    'FileIdGlobalTxDirectoryInformation',             // 50
    'FileIsRemoteDeviceInformation',                  // 51
    'FileUnusedInformation',                          // 52
    'FileNumaNodeInformation',                        // 53
    'FileStandardLinkInformation',                    // 54
    'FileRemoteProtocolInformation',                  // 55
    'FileRenameInformationBypassAccessCheck',         // 56
    'FileLinkInformationBypassAccessCheck',           // 57
    'FileVolumeNameInformation',                      // 58
    'FileIdInformation',                              // 59
    'FileIdExtdDirectoryInformation',                 // 60
    'FileReplaceCompletionInformation',               // 61
    'FileHardLinkFullIdInformation',                  // 62
    'FileIdExtdBothDirectoryInformation',             // 63
    'FileDispositionInformationEx',                   // 64
    'FileRenameInformationEx',                        // 65
    'FileRenameInformationExBypassAccessCheck',       // 66
    'FileDesiredStorageClassInformation',             // 67
    'FileStatInformation',                            // 68
    'FileMemoryPartitionInformation',                 // 69
    'FileStatLxInformation',                          // 70
    'FileCaseSensitiveInformation',                   // 71
    'FileLinkInformationEx',                          // 72
    'FileLinkInformationExBypassAccessCheck',         // 73
    'FileStorageReserveIdInformation',                // 74
    'FileCaseSensitiveInformationForceAccessCheck',   // 75
    'FileMaximumInformation'
  );


Type
  BUS_QUERY_ID_TYPE = (
    BusQueryDeviceID = 0,       // <Enumerator>\<Enumerator-specific device id>
    BusQueryHardwareIDs = 1,    // Hardware ids
    BusQueryCompatibleIDs = 2,  // compatible device ids
    BusQueryInstanceID = 3,     // persistent id for this instance of the device
    BusQueryDeviceSerialNumber = 4,   // serial number for this device
    BusQueryContainerID = 5     // unique id of the device's physical container
  );
  PBUS_QUERY_ID_TYPE = ^BUS_QUERY_ID_TYPE;

  DEVICE_TEXT_TYPE = (
    DeviceTextDescription = 0,            // DeviceDesc property
    DeviceTextLocationInformation = 1     // DeviceLocation property
  );
  PDEVICE_TEXT_TYPE = ^DEVICE_TEXT_TYPE;

  DEVICE_RELATION_TYPE = (
    BusRelations,
    EjectionRelations,
    PowerRelations,
    RemovalRelations,
    TargetDeviceRelation,
    SingleBusRelations,
    TransportRelations
  );
  PDEVICE_RELATION_TYPE = ^DEVICE_RELATION_TYPE;


  TIRPArguments = Record
    Case Integer Of
      0 : (Other : Record
        Arg1 : Pointer;
        Arg2 : Pointer;
        Arg3 : Pointer;
        Arg4 : Pointer;
        end; );
      1 : (ReadWrite : Record
        Length : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        Key : Cardinal;
        ByteOffset : UInt64;
        end; );
      2 : (QuerySetInformation : Record
        Lenth : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        FileInformationClass : Cardinal;
        end; );
      3 : (QuerySetVolume : Record
        Lenth : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        FileInformationClass : Cardinal;
        end; );
      4 : (DeviceControl : Record
        OutputBufferLength : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        InputBufferLength : Cardinal;
{$IFNDEF WIN32}
        Padding2 : Cardinal;
{$ENDIF}
        IoControlCode : Cardinal;
        Type3InputBuffer : Pointer;
        end; );
      5 : (VerifyVolume : Record
        NotUsed : Pointer;
        DeviceObject : Pointer;
        end; );
      6 : (QueryDeviceRelations : Record
        DeviceRelationType : DEVICE_RELATION_TYPE;
        end; );
      7 : (QueryInterface : Record
        InterfaceType : Pointer;
        Size : Word;
        Version : Word;
        InterfaceRoutines : Pointer;
        InterfaceSpecificData : Pointer;
        end; );
      8 : (QueryCapabilities : Record
        Capabilities : Pointer;
        end; );
      9 : (FilterResourceRequirements : Record
        end; );
      10 : (ReadWriteConfig : Record
        WhichSpace : Cardinal;
        Buffer : Pointer;
        Offset : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        Length : Cardinal;
        end; );
      11 : (SetLock : Record
        Lock : ByteBool;
        end; );
      12 : (QueryId : Record
        IdType : BUS_QUERY_ID_TYPE;
        end; );
      13 : (QueryText : Record
        TextType : DEVICE_TEXT_TYPE;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        LocaleId : Cardinal;
        end; );
      14 : (UsageNotification : Record
        InPath : ByteBool;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        NotificationType : Cardinal;
        end; );
      15 : (WaitWake : Record
        PowerState : Cardinal;
        end; );
      16 : (PowerSequence : Record
        end; );
      17 : (QuerySetPower : Record
        SystemContext : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        PowerStateType : Cardinal;
{$IFNDEF WIN32}
        Padding2 : Cardinal;
{$ENDIF}
        PowerState : Cardinal;
{$IFNDEF WIN32}
        Padding3 : Cardinal;
{$ENDIF}
        ShutdownType : Cardinal;
        end; );
      18 : (StartDevice : Record
        end; );
      19 : (Wmi : Record
        ProviderId : NativeUInt;
        DataPath : Pointer;
        BufferSize : Cardinal;
        Buffer : Pointer;
        end; );
      20 : (QueryDirectory : Record
        Length : Cardinal;
        FileName : Pointer;
        FileInformationClass : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        FlieIndex : Cardinal;
        end; );
      21 : (NotifyDirectory : Record
        Length : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        CompletionFilter : Cardinal;
        end; );
    22 : (QuerySecurity : Record
        SecurityInformation : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        Length : Cardinal;
        end; );
    23 : (SetSecurity : Record
        SecurityInformation : Cardinal;
        SecurityDescriptor : Pointer;
        end; );
    24 : (Create : Record
        SecurityContext : Pointer;
        Options : Cardinal;
{$IFNDEF WIN32}
        Padding : Cardinal;
{$ENDIF}
        FileAttributes : Word;
        ShareAccess : Word;
{$IFNDEF WIN32}
        Padding2 : Cardinal;
{$ENDIF}
        EaLength : Cardinal;
        end);
    end;

  (** Enumerates all possible types of Fast I/O operations. *)
  _EFastIoOperationType = (
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
	  FastIoMax);
  EFastIoOperationType = _EFastIoOperationType;
  PEFastIoOperationType = ^EFastIoOperationType;

  (** Type of request reported by the IRPMon driver. *)
  _ERequestType = (
	  (** Exists only for debugging purposes, should not be ever used. *)
	  ertUndefined,
	  (** I/O request packet (IRP). *)
	  ertIRP,
	  (** An IRP is completed. *)
	  ertIRPCompletion,
	  (** Driver's AddDevice routine was called in order to inform the driver about a newly detected device. *)
  	ertAddDevice,
	  (** A driver was unloaded. *)
	  ertDriverUnload,
	  (** A fast I/O request was serviced by a driver. *)
	  ertFastIo,
	  (** Driver's StartIo routine was invoked. *)
	  ertStartIo,
    (** Previously unknown or not-monitored driver has been detected. *)
    ertDriverDetected,
    (** A new device has been detected. **)
    ertDeviceDetected,
    (** An IRP_MJ_CREATE has just been performed on a file object **)
    ertFileObjectNameAssigned,
    (** Last handle to a file object has been closed (IRP_MJ_CLEANUP) **)
    ertFileObjectNameDeleted,
    ertProcessCreated,
    ertProcessExitted,
    ertImageLoad
  );
  ERequesttype = _ERequestType;
  PERequesttype = ^ERequesttype;

  (** Determines the type returned in the Result union of the @link(REQUEST_HEADER) structure. **)
  _ERequestResultType = (
	  (** The result value is either not yet initialized, or not defined for a given request type. **)
    rrtUndefined,
	  (** The type is NTSTATUS. **)
    rrtNTSTATUS,
	  (** The type is BOOLEAN. **)
    rrtBOOLEAN);
  ERequestResultType = _ERequestResultType;
  PERequestResultType = ^ERequestResultType;

Const
  REQUEST_FLAG_EMULATED            = $1;
  REQUEST_FLAG_DATA_STRIPPED       = $2;
  REQUEST_FLAG_ADMIN               = $4;
  REQUEST_FLAG_IMPERSONATED        = $8;
  REQUEST_FLAG_IMPERSONATED_ADMIN  = $10;
  REQUEST_FLAG_NEXT_AVAILABLE      = $20;
  REQUEST_FLAG_PAGED					     = $80;
  REQUEST_FLAG_NONPAGED				     = $100;

Type
  (** Header, containing information common for all request types. *)
  _REQUEST_HEADER = Record
	  Next : ^_REQUEST_HEADER;
    Nothing2 : Pointer;
	  (** Date and time of the request's detection (in 100 nanosecond
	    units from January 1 1601). *)
	  Time : UInt64;
	  (** Type of the request. *)
    RequestType : ERequesttype;
	  (** Unique identifier of the request. ID of a new request is always
	    greater than ID of already existing ones, so the ID also follows
		  the order in which the requests were created. *)
    Id : Cardinal;
	  (** Device object associated with the request. *)
	  Device : Pointer;
	  (** Driver object associated with the request. *)
	  Driver : Pointer;
    ProcessId : THandle;
    ThreadId : THandle;
    Flags : Word;
    Irql : Byte;
	  (** Result of the request servicing. The type of this field
	    differs depending the type of the request.

		 * NTSTATUS (ertIRP, ertAddDevice, ertStartIo).
		 * Not relevant to the request type (ertDriverUnload, erpUndefined).

		@todo Dopsat fast I/O
	  *)
    ResultType : ERequestResultType;
    Case ERequestResultType Of
      rrtUndefined : (Other : Pointer;);
      rrtNTSTATUS : (NTSTATUSValue : Cardinal;);
     rrtBOOLEAN : (BOOLEANValue : ByteBool;);
    end;
  REQUEST_HEADER = _REQUEST_HEADER;
  PREQUEST_HEADER = ^REQUEST_HEADER;

  (** Represents an IRP request. *)
  _REQUEST_IRP = Record
	  (** The header. *)
    Header : REQUEST_HEADER;
	  (** The major function (IRP_MJ_XXX constants). *)
	  MajorFunction : Byte;
	  (** The minor function (related to the major function), IRP_MN_XXX constants. *)
	  MinorFunction : Byte;
	  (** Value of the ExGetPreviousMode at the time of the request retrieval. *)
	  PreviousMode : Byte;
	  (** Indicates whether the creator of the request is sent by user application
	    or by a kernel component. *)
	  RequestorMode : Byte;
	  (** Address of the IRP structure representing the request. *)
	  IRPAddress : Pointer;
	  (** Irp->Flags *)
	  IrpFlags : Cardinal;
	  (** Irp->FileObject *)
	  FileObject : Pointer;
    Args : TIRPArguments;
    (** Value of Irp->IoStatus.Status at time of IRP detection. **)
    IOSBStatus : Cardinal;
    (** Value of Irp->IoStatus.Information at time of IRP detection. **)
    IOSBInformation : NativeUInt;
	  (** PID of the process originally requesting the operation. **)
    RequestorProcessId : NativeUInt;
    DataSize : NativeUInt;
    // Data
    end;
  REQUEST_IRP = _REQUEST_IRP;
  PREQUEST_IRP = ^REQUEST_IRP;

  _REQUEST_IRP_COMPLETION = Record
    Header : REQUEST_HEADER;
	  IRPAddress : Pointer;
	  CompletionStatus : Cardinal;
	  CompletionInformation : NativeUInt;
    MajorFunction : Cardinal;
    MinorFunction : Cardinal;
    Arguments : Packed Array [0..3] Of Pointer;
    FileObject : Pointer;
    RequestorProcessId : NativeUInt;
    PreviousMode : Byte;
    RequestorMode : Byte;
    DataSize : NativeUInt;
    end;
  REQUEST_IRP_COMPLETION = _REQUEST_IRP_COMPLETION;
  PREQUEST_IRP_COMPLETION = ^REQUEST_IRP_COMPLETION;

  (** Represents a fast I/O request. *)
  _REQUEST_FASTIO = Record
	  (** Request header. *)
    Header : REQUEST_HEADER;
	  (** Type of the fast I/O operation. *)
    FastIoType : EFastIoOperationType;
	  (** Indicates whether the operation was performed by a user aplication or
	    by a kernel component. *)
	  PreviousMode : Byte;
	  (** The first argument of the operation. *)
	  Arg1 : Pointer;
	(** The second argument of the operation. *)
	  Arg2 : Pointer;
	  (** The third argument of the operation. *)
	  Arg3 : Pointer;
	  (** The fourth argument of the operation. *)
	  Arg4 : Pointer;
	  Arg5 : Pointer;
	  Arg6 : Pointer;
	  Arg7 : Pointer;
	  Arg8 : Pointer;
	  Arg9 : Pointer;
	  FileObject : Pointer;
	  IOSBStatus : Cardinal;
	  IOSBInformation : NativeUInt;
    end;
  REQUEST_FASTIO = _REQUEST_FASTIO;
  PREQUEST_FASTIO = ^REQUEST_FASTIO;

  (** Represent an AddDevice event indicating that an AddDevice routine of a
    driver monitored by the IRPMon was executed. *)
  _REQUEST_ADDDEVICE = Record
	  (** Request header. *)
    Header : REQUEST_HEADER;
    end;
  REQUEST_ADDDEVICE = _REQUEST_ADDDEVICE;
  PREQUEST_ADDDEVICE = ^REQUEST_ADDDEVICE;

  (** Represents an event reporting a driver unload. *)
  _REQUEST_UNLOAD = Record
	  (** Request header. *)
    Header : REQUEST_HEADER;
    end;
  REQUEST_UNLOAD = _REQUEST_UNLOAD;
  PREQUEST_UNLOAD = ^REQUEST_UNLOAD;

  (** Represents an event reporting an invocation of driver's StartIo routine. *)
  _REQUEST_STARTIO = Record
	  (** Request header. *)
    Header : REQUEST_HEADER;
	  (** Address of an IRP structure passed to the routine. *)
    IRPAddress : Pointer;
	  (** Major type of the IRP (IRP_MJ_XXX constant). *)
    MajorFunction : Byte;
	  (** Minor type of the IRP (IRP_MN_XXX constant). *)
    MinorFunction : Byte;
	  (** The first argument of the operation. *)
	  Arg1 : Pointer;
	  (** The second argument of the operation. *)
	  Arg2 : Pointer;
	  (** The third argument of the operation. *)
	  Arg3 : Pointer;
	  (** The fourth argument of the operation. *)
	  Arg4 : Pointer;
	  IrpFlags : Cardinal;
	  FileObject : Pointer;
	  (** Value of the Irp->IoStatus.Information after calling the original
	    dispatch routine. *)
	  Information : NativeUInt;
	  (** Value of the Irp->IoStatus.Status after calling the original
	    dispatch routine. *)
	  Status : Cardinal;
    (** Length of data associated with the request. *)
    DataSize : NativeUInt;
    end;
  REQUEST_STARTIO = _REQUEST_STARTIO;
  PREQUEST_STARTIO = ^REQUEST_STARTIO;

  _REQUEST_DRIVER_DETECTED = Record
    Header : REQUEST_HEADER;
    DriverNameLength : Cardinal;
    end;
  REQUEST_DRIVER_DETECTED = _REQUEST_DRIVER_DETECTED;
  PREQUEST_DRIVER_DETECTED = ^REQUEST_DRIVER_DETECTED;

  _REQUEST_DEVICE_DETECTED = Record
    Header : REQUEST_HEADER;
    DeviceNameLength : Cardinal;
    end;
  REQUEST_DEVICE_DETECTED = _REQUEST_DEVICE_DETECTED;
  PREQUEST_DEVICE_DETECTED = ^REQUEST_DEVICE_DETECTED;

  _REQUEST_FILE_OBJECT_NAME_ASSIGNED = Record
    Header : REQUEST_HEADER;
    FileObject : Pointer;
    FileNameLength : Cardinal;
    end;
  REQUEST_FILE_OBJECT_NAME_ASSIGNED = _REQUEST_FILE_OBJECT_NAME_ASSIGNED;
  PREQUEST_FILE_OBJECT_NAME_ASSIGNED = ^REQUEST_FILE_OBJECT_NAME_ASSIGNED;

  _REQUEST_FILE_OBJECT_NAME_DELETED = Record
    Header : REQUEST_HEADER;
    FileObject : Pointer;
    end;
  REQUEST_FILE_OBJECT_NAME_DELETED = _REQUEST_FILE_OBJECT_NAME_DELETED;
  PREQUEST_FILE_OBJECT_NAME_DELETED = ^REQUEST_FILE_OBJECT_NAME_DELETED;

  _REQUEST_PROCESS_CREATED = Record
	  Header : REQUEST_HEADER;
	  ProcessId : THandle;
	  ParentId : THandle;
	  CreatorId : THandle;
	  ImageNameLength : UInt32;
	  CommandLineLength : UInt32;
    end;
  REQUEST_PROCESS_CREATED = _REQUEST_PROCESS_CREATED;
  PREQUEST_PROCESS_CREATED = ^REQUEST_PROCESS_CREATED;

  _REQUEST_PROCESS_EXITTED = Record
	  Header : REQUEST_HEADER;
	  ProcessId : THandle;
    end;
  REQUEST_PROCESS_EXITTED = _REQUEST_PROCESS_EXITTED;
  PREQUEST_PROCESS_EXITTED = ^REQUEST_PROCESS_EXITTED;

  _EImageSignatureType = (
	  istNone,
	  istEmbedded,
	  istCache,
	  istCatalogCached,
	  istCatalogNotCached,
	  istCatalogHint,
	  istPackageCatalog);
  EImageSignatureType = _EImageSignatureType;
  PEImageSignatureType = ^EImageSignatureType;

  _EImageSigningLevel = (
	  islUnchecked,
	  islUnsigned,
	  islEnterprise,
	  islDeveloper,
	  islAuthenticode,
	  islCustom2,
	  islStore,
	  islAntiMalware,
	  islMicrosoft,
	  islCustom4,
	  islCustom5,
	  islDynamicCode,
	  islWindows,
	  islCustom7,
	  islWindowsTCB,
	  islCustom6);
  EImageSigningLevel = _EImageSigningLevel;
  PEImageSigningLevel = ^EImageSigningLevel;

  _REQUEST_IMAGE_LOAD = Record
    Header : REQUEST_HEADER;
	  ImageBase : Pointer;
    ImageSize : NativeUInt;
	  FileObject : Pointer;
    SignatureLevel : EImageSigningLevel;
    SignatureType : EImageSignatureType;
    DataSize : Cardinal;
    KernelDriver : ByteBool;
    MappedToAllPids : ByteBool;
    ExtraInfo : ByteBool;
	  PartialMap : ByteBool;
    end;
  REQUEST_IMAGE_LOAD = _REQUEST_IMAGE_LOAD;
  PREQUEST_IMAGE_LOAD = ^REQUEST_IMAGE_LOAD;


  _REQUEST_GENERAL = Record
    Case ERequestType Of
      ertUndefined : ( Header : REQUEST_HEADER);
      ertIRP : (Irp : REQUEST_IRP);
		  ertIRPCompletion : (IrpComplete : REQUEST_IRP_COMPLETION);
		  ertFastIo : (FastIo : REQUEST_FASTIO);
		  ertAddDevice : (AddDevice : REQUEST_ADDDEVICE);
		  ertDriverUnload : (DriverUnload : REQUEST_UNLOAD);
		  ertStartIo : (StartIo : REQUEST_STARTIO);
      ertDriverDetected : (DriverDetected : REQUEST_DRIVER_DETECTED);
      ertDeviceDetected : (DeviceDetected : REQUEST_DEVICE_DETECTED);
      ertFileObjectNameAssigned : (FileObjectNameAssigned : REQUEST_FILE_OBJECT_NAME_ASSIGNED);
      ertFileObjectNameDeleted : (FileObjectNameDeleted : REQUEST_FILE_OBJECT_NAME_DELETED);
      ertProcessCreated : (ProcessCreated : REQUEST_PROCESS_CREATED);
      ertProcessExitted : (ProcessExitted : REQUEST_PROCESS_EXITTED);
      ertImageLoad : (ImageLoad : REQUEST_IMAGE_LOAD)
    end;
  REQUEST_GENERAL = _REQUEST_GENERAL;
  PREQUEST_GENERAL = ^REQUEST_GENERAL;


  TFastIoSettings = Packed Array [0..Ord(FastIoMax) - 1] Of Byte;
  PFastIoSettings = ^TFastIoSettings;
  TIRPSettings = Packed Array [0..$1B] Of Byte;
  PIRPSettings = ^TIRPSettings;

  (** Contains information about one device monitored by the IRPMon driver. *)
  _HOOKED_DEVICE_UMINFO = Record
	  (** ID of the object, used within the IRPMon driver. *)
    ObjectId : Pointer;
	  (** Address of device's DEVICE_OBJECT structure. *)
	  DeviceObject : Pointer;
	  (** Name of the hooked device. Can never be NULL. *)
	  DeviceName : PWideChar;
	  (** Length of the device name, in bytes. The value does not include the
	    terminating null character. *)
    DeviceNameLen : Cardinal;
	  (** Indicates which types of fast I/O requests are monitored. THe exact
	   meaning of each entry is still undefined. *)
    FastIoSettings : TFastIoSettings;
	  (** Indicates which types of IRP requests are monitored. THe exact
	   meaning of each entry is still undefined.
	   NOTE: 0x1b = IRP_MJ_MAXIMUM_FUNCTION. *)
	  IRPSettings : TIRPSettings;
	  (** Indicates whether the monitoring is active for the device. *)
    MonitoringEnabled : ByteBool;
    end;
  HOOKED_DEVICE_UMINFO = _HOOKED_DEVICE_UMINFO;
  PHOOKED_DEVICE_UMINFO = ^HOOKED_DEVICE_UMINFO;

  _IRP_SETTINGS = Record
    Settings : Array [0..$1B] Of Byte;
    end;
  IRP_SETTINGS = _IRP_SETTINGS;
  PIRP_SETTINGS = ^IRP_SETTINGS;

  _FASTIO_SETTINGS = Record
    Settings : Array [0..Ord(FastIoMax) - 1] Of Byte;
    end;
  FASTIO_SETTINGS = _FASTIO_SETTINGS;
  PFASTIO_SETTINGS = ^FASTIO_SETTINGS;

  _DRIVER_MONITOR_SETTINGS = Record
    MonitorNewDevices : ByteBool;
    MonitorAddDevice : ByteBool;
    MonitorStartIo : ByteBool;
    MonitorUnload : ByteBool;
    MonitorFastIo : ByteBool;
    MonitorIRP : ByteBool;
    MonitorIRPCompletion : ByteBool;
    MonitorData : ByteBool;
    IRPSettings : IRP_SETTINGS;
    FastIoSettings : FASTIO_SETTINGS;
    end;
  DRIVER_MONITOR_SETTINGS = _DRIVER_MONITOR_SETTINGS;
  PDRIVER_MONITOR_SETTINGS = ^DRIVER_MONITOR_SETTINGS;

  (** Contains information about one driver hooked by the IRPMon driver. *)
  _HOOKED_DRIVER_UMINFO = Record
	  (** ID of the object, used within the IRPMon driver. *)
    ObjectId : Pointer;
	  (** Address of driver's DRIVER_OBJECT structure. *)
	  DriverObject : Pointer;
	  (** Name of the driver. Cannot be NULL. *)
	  DriverName : PWideChar;
	  (* Length of the driver name, in bytes. The value does not include the
	   terminating null-character. *)
	  DriverNameLen : Cardinal;
	  (** Indicates whether the IRPMon driver monitors events related to the target
	    driver. If set to TRUE, the information about the events is stored in the
		  IRPMon Event Queue. *)
    MonitoringEnabled : ByteBool;
    DeviceExtensionHooks : ByteBool;
    MonitorSettings : DRIVER_MONITOR_SETTINGS;
	  (** Number of devices, monitored by the IRPMon driver (not including the new ones). *)
    NumberOfHookedDevices : Cardinal;
	  (** An array of @link(HOOKED_DEVICE_UMINFO) structures, each representing one
	   monitored device. *)
    HookedDevices : PHOOKED_DEVICE_UMINFO;
    end;
  HOOKED_DRIVER_UMINFO = _HOOKED_DRIVER_UMINFO;
  PHOOKED_DRIVER_UMINFO = ^HOOKED_DRIVER_UMINFO;

  (** Stores information about one device object existing in the system. *)
  _IRPMON_DEVICE_INFO = Record
	  (** Object address (address of its DEVICE_OBJECT structure). *)
    DeviceObject : Pointer;
	  (** Address of device object attached to this one. NULL value means no device
	    is attached. *)
	  AttachedDevice : Pointer;
	  (** Name of the device. If the device has no name, this field points to a null
	    character. Will never be NULL. *)
    Name : PWideChar;
    end;
  IRPMON_DEVICE_INFO = _IRPMON_DEVICE_INFO;
  PIRPMON_DEVICE_INFO = ^IRPMON_DEVICE_INFO;
  PPIRPMON_DEVICE_INFO = ^PIRPMON_DEVICE_INFO;

  (** Stores information about one driver existing in the system. *)
  _IRPMON_DRIVER_INFO = Record
	  (** Address of the driver (its DRIVER_OBJECT structure). *)
    DriverObject : Pointer;
	  (** Driver name, will never be NULL. *)
    DriverName : PWideChar;
	  (** Number of device objects owned by the driver. *)
    DeviceCount : Cardinal;
	  (** Pointer to an array of pointers to @link(IRPMON_DEVICE_INFO) each
	    containing information about one device owned by the driver. Number
		  of entries is reflected by the DeviceCount member. *)
    Devices : PPIRPMON_DEVICE_INFO;
    end;
  IRPMON_DRIVER_INFO = _IRPMON_DRIVER_INFO;
  PIRPMON_DRIVER_INFO = ^IRPMON_DRIVER_INFO;
  PPIRPMON_DRIVER_INFO = ^PIRPMON_DRIVER_INFO;

  _CLASS_WATCH_RECORD = Record
	  ClassGuid : Packed Array [0..15] Of Byte;
	  ClassGuidString : PWideChar;
	  UpperFilter : Byte;
	  Beginning : Byte;
    end;
  CLASS_WATCH_RECORD = _CLASS_WATCH_RECORD;
  PCLASS_WATCH_RECORD = ^CLASS_WATCH_RECORD;

  _DRIVER_NAME_WATCH_RECORD = Record
    MonitorSettings : DRIVER_MONITOR_SETTINGS;
	  DriverName : PWideChar;
    end;
  DRIVER_NAME_WATCH_RECORD = _DRIVER_NAME_WATCH_RECORD;
  PDRIVER_NAME_WATCH_RECORD = ^DRIVER_NAME_WATCH_RECORD;

  _EIRPMonConnectorType = (
    ictNone,
    ictDevice,
    ictNetwork
  );
  EIRPMonConnectorType = _EIRPMonConnectorType;
  PEIRPMonConnectorType = ^EIRPMonConnectorType;

  _IRPMON_INIT_INFO = Record
    ConnectionType : EIRPMonConnectorType;
    Case EIRPMonConnectorType Of
      ictDevice : (
        DeviceName : PWideChar;
        );
      ictNetwork : (
          NetworkHost : PWideChar;
          NetworkPort : PWideChar;
          AddressFamily : Word;
        );
    end;
  IRPMON_INIT_INFO = _IRPMON_INIT_INFO;
  PIRPMON_INIT_INFO = ^IRPMON_INIT_INFO;

  _IRPMNDRV_SETTINGS = Record
    ReqQueueLastRequestId : UInt32;
	  ReqQueueLength : UInt32;
    ReqQueueNonPagedLength : UInt32;
    ReqQueuePagedLength : UInt32;
	  ReqQueueConnected : ByteBool;
	  ReqQueueClearOnDisconnect : ByteBool;
	  ReqQueueCollectWhenDisconnected : ByteBool;
	  ProcessEventsCollect : ByteBool;
	  FileObjectEventsCollect : ByteBool;
	  DriverSnapshotEventsCollect : ByteBool;
	  ProcessEmulateOnConnect : ByteBool;
	  DriverSnapshotOnConnect : ByteBool;
    DataStripThreshold : Cardinal;
    StripData : ByteBool;
    end;
  IRPMNDRV_SETTINGS = _IRPMNDRV_SETTINGS;
  PIRPMNDRV_SETTINGS = ^IRPMNDRV_SETTINGS;

  ERequestFilterOperator = (
    rfoEquals,
    rfoLowerEquals,
    rfoGreaterEquals,
    rfoLower,
    rfoGreater,
    rfoContains,
    rfoBegins,
    rfoEnds,
    rfoAlwaysTrue,
    rfoDLLDecider
  );

  EFilterAction = (
    ffaHighlight, // If the request is included
    ffaInclude,
    ffaExclude,
    ffaPassToFilter
  );


Function IRPMonDllDriverHooksEnumerate(Var AHookedDrivers:PHOOKED_DRIVER_UMINFO; Var ACount:Cardinal):Cardinal; StdCall;
Procedure IRPMonDllDriverHooksFree(AHookedDrivers:PHOOKED_DRIVER_UMINFO; ACount:Cardinal); StdCall;

Function IRPMonDllHookDriver(ADriverName:PWideChar; Var AMonitorSettings:DRIVER_MONITOR_SETTINGS; ADeviceExtensionHook:ByteBool; Var ADriverHandle:THandle; Var AObjectId:Pointer):Cardinal; StdCall;
Function IRPMonDllDriverStartMonitoring(ADriverhandle:THandle):Cardinal; StdCall;
Function IRPMonDllDriverStopMonitoring(ADriverhandle:THandle):Cardinal; StdCall;
Function IRPMonDllDriverSetInfo(ADriverHandle:THandle; Var ASettings:DRIVER_MONITOR_SETTINGS):Cardinal; StdCall;
Function IRPMonDllUnhookDriver(ADriverHandle:THandle):Cardinal; StdCall;
Function IRPMonDllHookDeviceByName(ADeviceName:PWideChar; Var AHookHandle:THandle; Var AObjectId:Pointer):Cardinal; StdCall;
Function IRPMonDllHookDeviceByAddress(ADeviceObject:Pointer; Var AHookHandle:THandle; Var AObjectId:Pointer):Cardinal; StdCall;
Function IRPMonDllUnhookDevice(AHookHandle:THandle):Cardinal; StdCall;
Function IRPMonDllHookedDeviceGetInfo(AHandle:THandle; AIRPSettings:PByte; AFastIOSettings:PByte; Var AMonitoringEnabled:ByteBool):Cardinal; StdCall;
Function IRPMonDllHookedDeviceSetInfo(AHandle:THandle; AIRPSettings:PByte; AFastIOSettings:PByte; AMonitoringEnabled:ByteBool):Cardinal; StdCall;
Function IRPMonDllHookedDriverGetInfo(AHandle:THandle; Var ASettings:DRIVER_MONITOR_SETTINGS; Var AMonitoringEnabled:ByteBool):Cardinal; StdCall;

Function IRPMonDllSnapshotRetrieve(Var ADriverInfo:PPIRPMON_DRIVER_INFO; Var ACount:Cardinal):Cardinal; StdCall;
Procedure IRPMonDllSnapshotFree(ADriverInfo:PPIRPMON_DRIVER_INFO; ACount:Cardinal); StdCall;

Function IRPMonDllConnect:Cardinal; StdCall;
Function IRPMonDllDisconnect:Cardinal; StdCall;
Function IRPMonDllGetRequest(ARequest:PREQUEST_HEADER; ASize:Cardinal):Cardinal; StdCall;

Function IRPMonDllOpenHookedDriver(AObjectId:Pointer; Var AHandle:THandle):Cardinal; StdCall;
Function IRPMonDllCloseHookedDriverHandle(AHandle:THandle):Cardinal; StdCall;
Function IRPMonDllOpenHookedDevice(AObjectId:Pointer; Var AHandle:THandle):Cardinal; StdCall;
Function IRPMonDllCloseHookedDeviceHandle(AHandle:THandle):Cardinal; StdCall;

Function IRPMonDllClassWatchRegister(AClassGuid:PWideChar; AUpperFilter:ByteBool; ABeginning:ByteBool):Cardinal; StdCall;
Function IRPMonDllClassWatchUnregister(AClassGuid:PWideChar; AUpperFilter:ByteBool; ABeginning:ByteBool):Cardinal; StdCall;
Function IRPMonDllClassWatchEnum(Var AArray:PCLASS_WATCH_RECORD; Var ACount:Cardinal):Cardinal; StdCall;
Procedure IRPMonDllClassWatchEnumFree(AArray:PCLASS_WATCH_RECORD; ACount:Cardinal); StdCall;

Function IRPMonDllDriverNameWatchRegister(ADriverName:PWideChar; Var AMonitorSettings:DRIVER_MONITOR_SETTINGS):Cardinal; StdCall;
Function IRPMonDllDriverNameWatchUnregister(ADriverName:PWideChar):Cardinal; StdCall;
Function IRPMonDllDriverNameWatchEnum(Var AArray:PDRIVER_NAME_WATCH_RECORD; Var ACount:Cardinal):Cardinal; StdCall;
Procedure IRPMonDllDriverNameWatchEnumFree(AArray:PDRIVER_NAME_WATCH_RECORD; ACount:Cardinal); StdCall;

Function IRPMonDllEmulateDriverDevices:Cardinal; StdCall;
Function IRPMonDllEmulateProcesses:Cardinal; StdCall;

Function IRPMonDllSettingsQuery(Var ASettings:IRPMNDRV_SETTINGS):Cardinal; StdCall;
Function IRPMonDllSettingsSet(Var ASettings:IRPMNDRV_SETTINGS; ASave:ByteBool):Cardinal; StdCall;

Function RequestCopy(AHeader:PREQUEST_HEADER):PREQUEST_HEADER; Cdecl;
Function RequestMemoryAlloc(ASize:NativeUInt):PREQUEST_HEADER; Cdecl;
Procedure RequestMemoryFree(AHeader:PREQUEST_HEADER); Cdecl;
Function RequestGetSize(ARequest:PREQUEST_HEADER):Cardinal; Cdecl;

Function IRPMonDllInitialized:LongBool; StdCall;
Function IRPMonDllInitialize(Var AInfo:IRPMON_INIT_INFO):Cardinal; StdCall;
Procedure IRPMonDllFinalize; StdCall;

Implementation

Const
  LibraryName = 'irpmondll.dll';
  RequestsLibraryName = 'requests.dll';


Function IRPMonDllDriverHooksEnumerate(Var AHookedDrivers:PHOOKED_DRIVER_UMINFO; Var ACount:Cardinal):Cardinal; StdCall; External LibraryName;
Procedure IRPMonDllDriverHooksFree(AHookedDrivers:PHOOKED_DRIVER_UMINFO; ACount:Cardinal); StdCall; External LibraryName;

Function IRPMonDllHookDriver(ADriverName:PWideChar; Var AMonitorSettings:DRIVER_MONITOR_SETTINGS; ADeviceExtensionHook:ByteBool; Var ADriverHandle:THandle; Var AObjectId:Pointer):Cardinal; StdCall; External LibraryName;
Function IRPMonDllDriverStartMonitoring(ADriverhandle:THandle):Cardinal; StdCall; External LibraryName;
Function IRPMonDllDriverStopMonitoring(ADriverhandle:THandle):Cardinal; StdCall; External LibraryName;
Function IRPMonDllDriverSetInfo(ADriverHandle:THandle; Var ASettings:DRIVER_MONITOR_SETTINGS):Cardinal; StdCall; External LibraryName;
Function IRPMonDllUnhookDriver(ADriverHandle:THandle):Cardinal; StdCall; External LibraryName;
Function IRPMonDllHookDeviceByName(ADeviceName:PWideChar; Var AHookHandle:THandle; Var AObjectId:Pointer):Cardinal; StdCall; External LibraryName;
Function IRPMonDllHookDeviceByAddress(ADeviceObject:Pointer; Var AHookHandle:THandle; Var AObjectId:Pointer):Cardinal; StdCall; External LibraryName;
Function IRPMonDllUnhookDevice(AHookHandle:THandle):Cardinal; StdCall; External LibraryName;
Function IRPMonDllHookedDeviceGetInfo(AHandle:THandle; AIRPSettings:PByte; AFastIOSettings:PByte; Var AMonitoringEnabled:ByteBool):Cardinal; StdCall; External LibraryName;
Function IRPMonDllHookedDeviceSetInfo(AHandle:THandle; AIRPSettings:PByte; AFastIOSettings:PByte; AMonitoringEnabled:ByteBool):Cardinal; StdCall; External LibraryName;
Function IRPMonDllHookedDriverGetInfo(AHandle:THandle; Var ASettings:DRIVER_MONITOR_SETTINGS; Var AMonitoringEnabled:ByteBool):Cardinal; StdCall; External LibraryName;

Function IRPMonDllSnapshotRetrieve(Var ADriverInfo:PPIRPMON_DRIVER_INFO; Var ACount:Cardinal):Cardinal; StdCall; External LibraryName;
Procedure IRPMonDllSnapshotFree(ADriverInfo:PPIRPMON_DRIVER_INFO; ACount:Cardinal); StdCall; External LibraryName;

Function IRPMonDllConnect:Cardinal; StdCall; External LibraryName;
Function IRPMonDllDisconnect:Cardinal; StdCall; External LibraryName;
Function IRPMonDllGetRequest(ARequest:PREQUEST_HEADER; ASize:Cardinal):Cardinal; StdCall; External LibraryName;

Function IRPMonDllOpenHookedDriver(AObjectId:Pointer; Var AHandle:THandle):Cardinal; StdCall; External LibraryName;
Function IRPMonDllCloseHookedDriverHandle(AHandle:THandle):Cardinal; StdCall; External LibraryName;
Function IRPMonDllOpenHookedDevice(AObjectId:Pointer; Var AHandle:THandle):Cardinal; StdCall; External LibraryName;
Function IRPMonDllCloseHookedDeviceHandle(AHandle:THandle):Cardinal; StdCall; External LibraryName;

Function IRPMonDllClassWatchRegister(AClassGuid:PWideChar; AUpperFilter:ByteBool; ABeginning:ByteBool):Cardinal; StdCall; External LibraryName;
Function IRPMonDllClassWatchUnregister(AClassGuid:PWideChar; AUpperFilter:ByteBool; ABeginning:ByteBool):Cardinal; StdCall; External LibraryName;
Function IRPMonDllClassWatchEnum(Var AArray:PCLASS_WATCH_RECORD; Var ACount:Cardinal):Cardinal; StdCall; External LibraryName;
Procedure IRPMonDllClassWatchEnumFree(AArray:PCLASS_WATCH_RECORD; ACount:Cardinal); StdCall; External LibraryName;

Function IRPMonDllDriverNameWatchRegister(ADriverName:PWideChar; Var AMonitorSettings:DRIVER_MONITOR_SETTINGS):Cardinal; StdCall; External LibraryName;
Function IRPMonDllDriverNameWatchUnregister(ADriverName:PWideChar):Cardinal; StdCall; External LibraryName;
Function IRPMonDllDriverNameWatchEnum(Var AArray:PDRIVER_NAME_WATCH_RECORD; Var ACount:Cardinal):Cardinal; StdCall; External LibraryName;
Procedure IRPMonDllDriverNameWatchEnumFree(AArray:PDRIVER_NAME_WATCH_RECORD; ACount:Cardinal); StdCall; External LibraryName;

Function IRPMonDllEmulateDriverDevices:Cardinal; StdCall; External LibraryName;
Function IRPMonDllEmulateProcesses:Cardinal; StdCall; External LibraryName;

Function IRPMonDllSettingsQuery(Var ASettings:IRPMNDRV_SETTINGS):Cardinal; StdCall; External LibraryName;
Function IRPMonDllSettingsSet(Var ASettings:IRPMNDRV_SETTINGS; ASave:ByteBool):Cardinal; StdCall; External LibraryName;

Function RequestCopy(AHeader:PREQUEST_HEADER):PREQUEST_HEADER; Cdecl; External RequestsLibraryName;
Function RequestMemoryAlloc(ASize:NativeUInt):PREQUEST_HEADER; Cdecl; External RequestsLibraryName;
Procedure RequestMemoryFree(AHeader:PREQUEST_HEADER); Cdecl; External RequestsLibraryName;
Function RequestGetSize(ARequest:PREQUEST_HEADER):Cardinal; Cdecl; External RequestsLibraryName;

Function IRPMonDllInitialized:LongBool; StdCall; External LibraryName;
Function IRPMonDllInitialize(Var AInfo:IRPMON_INIT_INFO):Cardinal; StdCall; External LibraryName;
Procedure IRPMonDllFinalize; StdCall; External LibraryName;


End.

