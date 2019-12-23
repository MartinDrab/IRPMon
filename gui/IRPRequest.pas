Unit IRPRequest;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows,
  RequestListModel, IRPMonDll;

Type
  TIRPRequest = Class (TDriverRequest)
  Private
    FArgs : TIRPArguments;
    FIRPAddress : Pointer;
    FMajorFunction : Byte;
    FMinorFunction : Byte;
    FPreviousMode : Byte;
    FRequestorMode : Byte;
    FIRPFlags : Cardinal;
    FProcessId : THandle;
    FThreadId : THandle;
    FIOSBStatus : Cardinal;
    FIOSBInformation : NativeUInt;
    FRequestorProcessId : NativeUInt;
  Public
    Constructor Create(Var ARequest:REQUEST_IRP); Overload;

    Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Override;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    Class Function PowerStateTypeToString(AType:Cardinal):WideString;
    Class Function PowerStateToString(AType:Cardinal; AState:Cardinal):WideString;
    Class Function ShutdownTypeToString(AType:Cardinal):WideString;
    Class Function BusQueryIdTypeToString(AType:BUS_QUERY_ID_TYPE):WideString;
    Class Function DeviceTextTypeToString(AType:DEVICE_TEXT_TYPE):WideString;
    Class Function DeviceRelationTypeToString(AType:DEVICE_RELATION_TYPE):WideString;
    Class Function SecurityInformationToString(AInfo:Cardinal):WideString;
    Class Function CreateOptionsToString(AOptions:Cardinal):WideString;
    Class Function ShareAccessToString(AAccess:Cardinal):WideString;
    Class Function Build(Var ARequest:REQUEST_IRP):TIRPRequest;

    Property Args : TIRPArguments Read FArgs;
    Property Address : Pointer Read FIRPAddress;
    Property MajorFunction : Byte Read FMajorFunction;
    Property MinorFunction : Byte Read FMinorFunction;
    Property RequestorMode : Byte Read FRequestorMode;
    Property PreviousMode : Byte Read FPreviousMode;
    Property IRPFlags : Cardinal Read FIRPFlags;
    Property ProcessId : THandle Read FProcessId;
    Property ThreadId : THandle Read FThreadId;
    Property IOSBStatus : Cardinal Read FIOSBStatus;
    Property IOSBInformation : NativeUInt Read FIOSBInformation;
    Property RequestorProcessId : NativeUInt Read FRequestorProcessId;
  end;

  TZeroArgIRPRequest = Class (TIRPRequest)
    Public
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TCreateRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TDeviceControlRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TFileSystemControlRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TReadWriteRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TQuerySetRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
      Class Function InformationClassToString(AClass:Cardinal):WideString;
    end;

  TQuerySetVolumeRequest = Class (TIRPRequest)
    Public
      Class Function InformationClassToString(AInfoClass:Cardinal):WideString;
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TQuerySecurityRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TSetSecurityRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TCloseCleanupRequest = Class (TIRPRequest)
    Public
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  (** IRP_MJ_POWER **)

  TWaitWakeRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TPowerSequenceRequest = Class (TIRPRequest)
    Public
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TQuerySetPowerRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  (** IRP_MJ_PNP **)

  TQueryIdRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TQueryDeviceTextRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TQueryDeviceRelationsRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TQueryDeviceCapabilitiesRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TQueryInterfaceRequest = Class (TIRPRequest)
    Public
      Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TQueryDeviceStateRequest = Class (TZeroArgIRPRequest);
  TStartDeviceRequest = Class (TZeroArgIRPRequest);
  TEnumeratedDeviceRequest = Class (TZeroArgIRPRequest);
  TRemoveDeviceRequest = Class (TZeroArgIRPRequest);
  TQueryRemoveDeviceRequest = Class (TZeroArgIRPRequest);
  TCancelRemoveDeviceRequest = Class (TZeroArgIRPRequest);
  TSurpriseDeviceRequest = Class (TZeroArgIRPRequest);
  TstopDeviceRequest = Class (TZeroArgIRPRequest);
  TQueryStopDeviceRequest = Class (TZeroArgIRPRequest);
  TCancelStopDeviceRequest = Class (TZeroArgIRPRequest);
  TEjectDeviceRequest = Class (TZeroArgIRPRequest);


Implementation

Uses
  SysUtils,
  NameTables;

(** TIRPRequest **)

Constructor TIRPRequest.Create(Var ARequest:REQUEST_IRP);
Var
  d : Pointer;
begin
Inherited Create(ARequest.Header);
d := PByte(@ARequest) + SizeOf(ARequest);
AssignData(d, ARequest.DataSize);
FMajorFunction := ARequest.MajorFunction;
FMinorFunction := ARequest.MinorFunction;
FPreviousMode := ARequest.PreviousMode;
FRequestorMode := ARequest.RequestorMode;
FIRPAddress := ARequest.IRPAddress;
FIRPFlags := ARequest.IrpFlags;
SetFileObject(ARequest.FileObject);
FArgs := ARequest.Args;
FRequestorProcessId := ARequest.RequestorProcessId;
end;


Function TIRPRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
case AColumnType of
  rlmctSubType: Result := 'Major function';
  rlmctMinorFunction: Result := 'Minor function';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

Function TIRPRequest.GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctSubType: begin
    AValue := @FMajorFunction;
    AValueSize := SizeOf(FMajorFunction);
    end;
  rlmctMinorFunction: begin
    AValue := @FMinorFunction;
    AValueSize := SizeOf(FMinorFunction);
    end;
  rlmctIRPAddress: begin
    AValue := @FIRPAddress;
    AValueSize := SizeOf(FIRPAddress);
    end;
  rlmctIRPFlags: begin
    AValue := @FIRPFlags;
    AValueSize := SizeOf(FIRPFlags);
    end;
  rlmctArg1: begin
    AValue := @FArgs.Other.Arg1;
    AValueSize := SizeOf(FArgs.Other.Arg1);
    end;
  rlmctArg2: begin
    AValue := @FArgs.Other.Arg2;
    AValueSize := SizeOf(FArgs.Other.Arg2);
    end;
  rlmctArg3: begin
    AValue := @FArgs.Other.Arg3;
    AValueSize := SizeOf(FArgs.Other.Arg3);
    end;
  rlmctArg4: begin
    AValue := @FArgs.Other.Arg4;
    AValueSize := SizeOf(FArgs.Other.Arg4);
    end;
  rlmctPreviousMode: begin
    AValue := @FPreviousMode;
    AValueSize := SizeOf(FPreviousMode);
    end;
  rlmctRequestorMode: begin
    AValue := @FRequestorMode;
    AValueSize := SizeOf(FRequestorMode);
    end;
  rlmctIOSBStatusValue: begin
    AValue := @FIOSBStatus;
    AValueSize := SizeOf(FIOSBStatus);
    end;
  rlmctIOSBInformation: begin
    AValue := @FIOSBInformation;
    AValueSize := SizeOf(FIOSBInformation);
    end;
  rlmctRequestorPID: begin
    AValue := @FRequestorProcessId;
    AValueSize := SizeOf(FRequestorProcessId);
    end;
  Else Inherited GetColumnValueRaw(AColumnType, AValue, AValueSize);
  end;
end;

Function TIRPRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctRequestorPID : AResult := Format('%d', [FRequestorProcessId]);
  rlmctSubType: AResult := Format('%s', [MajorFunctionToString(FMajorFunction)]);
  rlmctMinorFunction: AResult := Format('%s', [MinorFunctionToString(FMajorFunction, FMinorFunction)]);
  rlmctIRPAddress: AResult := Format('0x%p', [FIRPAddress]);
  rlmctIRPFlags: AResult := Format('0x%x', [FIRPFlags]);
  rlmctArg1: AResult := Format('0x%p', [FArgs.Other.Arg1]);
  rlmctArg2: AResult := Format('0x%p', [FArgs.Other.Arg2]);
  rlmctArg3: AResult := Format('0x%p', [FArgs.Other.Arg3]);
  rlmctArg4: AResult := Format('0x%p', [FArgs.Other.Arg4]);
  rlmctPreviousMode: AResult := AccessModeToString(FPreviousMode);
  rlmctRequestorMode: AResult := AccessModeToString(FRequestorMode);
  rlmctIOSBStatusValue : AResult := Format('0x%x', [FIOSBStatus]);
  rlmctIOSBStatusConstant : AResult := Format('%s', [NTSTATUSToString(FIOSBStatus)]);
  rlmctIOSBInformation : AResult := Format('%u (0x%p)', [FIOSBInformation, Pointer(FIOSBInformation)]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Class Function TIRPRequest.PowerStateTypeToString(AType:Cardinal):WideString;
begin
Case AType Of
  0 : Result := 'System';
  1 : Result := 'Device';
  Else Result := Format('%u', [AType]);
  end;
end;

Class Function TIRPRequest.PowerStateToString(AType:Cardinal; AState:Cardinal):WideString;
begin
Result := Format('%u', [AState]);
If AType = 0 Then
  begin
  Case AState Of
    0 : Result := 'PowerSystemUnspecified';
    1 : Result := 'PowerSystemWorking';
    2 : Result := 'PowerSystemSleeping1';
    3 : Result := 'PowerSystemSleeping2';
    4 : Result := 'PowerSystemSleeping3';
    5 : Result := 'PowerSystemHibernate';
    6 : Result := 'PowerSystemShutdown';
    7 : Result := 'PowerSystemMaximum';
    end;
  end
Else If AType = 1 Then
  begin
  Case AState Of
    0 : Result := 'PowerDeviceUnspecified';
    1 : Result := 'PowerDeviceD0';
    2 : Result := 'PowerDeviceD1';
    3 : Result := 'PowerDeviceD2';
    4 : Result := 'PowerDeviceD3';
    end;
  end;
end;

Class Function TIRPRequest.ShutdownTypeToString(AType:Cardinal):WideString;
begin
Case AType Of
  0 : Result := 'PowerActionNone';
  1 : Result := ' PowerActionReserved';
  2 : Result := ' PowerActionSleep';
  3 : Result := ' PowerActionHibernate';
  4 : Result := ' PowerActionShutdown';
  5 : Result := ' PowerActionShutdownReset';
  6 : Result := ' PowerActionShutdownOff';
  7 : Result := ' PowerActionWarmEject';
  end;
end;

Class Function TIRPRequest.BusQueryIdTypeToString(AType:BUS_QUERY_ID_TYPE):WideString;
begin
Case AType Of
  BusQueryDeviceID: Result := 'DeviceID';
  BusQueryHardwareIDs: Result := 'HardwareIDs';
  BusQueryCompatibleIDs: Result := 'CompatibleIDs';
  BusQueryInstanceID: Result := 'InstanceID';
  BusQueryDeviceSerialNumber: Result := 'SerialNumber';
  BusQueryContainerID: Result := 'ContainerID';
  Else Result := Format('<unknown (%u)>', [Ord(AType)]);
  End;
end;

Class Function TIRPRequest.DeviceTextTypeToString(AType:DEVICE_TEXT_TYPE):WideString;
begin
Case AType Of
  DeviceTextDescription: Result := 'Description';
  DeviceTextLocationInformation: Result := 'Location';
  Else Result := Format('<unknown (%u)>', [Ord(AType)]);
  end;
end;

Class Function TIRPRequest.DeviceRelationTypeToString(AType:DEVICE_RELATION_TYPE):WideString;
begin
Case AType Of
  BusRelations: Result := 'Bus';
  EjectionRelations: Result := 'Ejection';
  PowerRelations: Result := 'Power';
  RemovalRelations: Result := 'Removal';
  TargetDeviceRelation: Result := 'Target';
  SingleBusRelations: Result := 'SingleBus';
  TransportRelations: Result := 'Transport';
  Else Result := Format('<unknown (%u)>', [Ord(AType)]);
  end;
end;

Class Function TIRPRequest.SecurityInformationToString(AInfo:Cardinal):WideString;
begin
Result := '';
If ((AInfo And OWNER_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' Owner';
If ((AInfo And GROUP_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' Group';
If ((AInfo And DACL_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' Dacl';
If ((AInfo And SACL_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' Sacl';
If ((AInfo And LABEL_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' Label';
If ((AInfo And ATTRIBUTE_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' Attribute';
If ((AInfo And SCOPE_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' Scope';
If ((AInfo And PROCESS_TRUST_LABEL_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' ProcessTrust';
If ((AInfo And ACCESS_FILTER_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' AccessFilter';
If ((AInfo And PROTECTED_DACL_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' ProtectedDaclt';
If ((AInfo And PROTECTED_SACL_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' ProtectedSacl';
If ((AInfo And UNPROTECTED_DACL_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' UnprotectedDaclt';
If ((AInfo And UNPROTECTED_SACL_SECURITY_INFORMATION) <> 0) Then
  Result := Result + ' UnprotectedSacl';

If Result <> '' Then
  Delete(Result, 1, 1);
end;

Class Function TIRPRequest.CreateOptionsToString(AOptions:Cardinal):WideString;
Var
  disp : Cardinal;
  opt : Cardinal;
begin
Result := '';
opt := (AOptions And $FFFFFF);
disp := (AOptions Shr 24);
Case disp Of
  FILE_SUPERSEDE : Result := 'Supersede';
  FILE_CREATE : Result := 'Supersede';
  FILE_OPEN : Result := 'Supersede';
  FILE_OPEN_IF : Result := 'Supersede';
  FILE_OVERWRITE : Result := 'Supersede';
  FILE_OVERWRITE_IF : Result := 'Supersede';
  end;

If ((opt And FILE_DIRECTORY_FILE) <> 0) THen
  Result := Result + ' Directory';
If ((opt And FILE_WRITE_THROUGH) <> 0) THen
  Result := Result + ' WriteThrough';
If ((opt And FILE_SEQUENTIAL_ONLY) <> 0) THen
  Result := Result + ' Sequential';
If ((opt And FILE_NO_INTERMEDIATE_BUFFERING) <> 0) THen
  Result := Result + ' NoBuffering';
If ((opt And FILE_SYNCHRONOUS_IO_ALERT) <> 0) THen
  Result := Result + ' SynchronousAlert';
If ((opt And FILE_SYNCHRONOUS_IO_NONALERT) <> 0) THen
  Result := Result + ' SynchronousNoAlert';
If ((opt And FILE_NON_DIRECTORY_FILE) <> 0) THen
  Result := Result + ' NonDirectory';
If ((opt And FILE_CREATE_TREE_CONNECTION) <> 0) THen
  Result := Result + ' TreeConnection';
If ((opt And FILE_COMPLETE_IF_OPLOCKED) <> 0) THen
  Result := Result + ' CompleteOplocked';
If ((opt And FILE_NO_EA_KNOWLEDGE) <> 0) THen
  Result := Result + ' NoEAKnowledge';
If ((opt And FILE_OPEN_REMOTE_INSTANCE) <> 0) THen
  Result := Result + ' RemoteInstance';
If ((opt And FILE_RANDOM_ACCESS) <> 0) THen
  Result := Result + ' RandomAccess';
If ((opt And FILE_DELETE_ON_CLOSE) <> 0) THen
  Result := Result + ' DeleteOnClose';
If ((opt And FILE_OPEN_BY_FILE_ID) <> 0) THen
  Result := Result + ' FileId';
If ((opt And FILE_OPEN_FOR_BACKUP_INTENT) <> 0) THen
  Result := Result + ' Backup';
If ((opt And FILE_NO_COMPRESSION) <> 0) THen
  Result := Result + ' NoCompression';
If ((opt And FILE_OPEN_REQUIRING_OPLOCK) <> 0) THen
  Result := Result + ' RequireOplock';
If ((opt And FILE_DISALLOW_EXCLUSIVE) <> 0) THen
  Result := Result + ' DisallowExclusive';
If ((opt And FILE_SESSION_AWARE) <> 0) THen
  Result := Result + ' SessionAware';
If ((opt And FILE_RESERVE_OPFILTER) <> 0) THen
  Result := Result + ' ReserveOpFilter';
If ((opt And FILE_OPEN_REPARSE_POINT) <> 0) THen
  Result := Result + ' ReparsePoint';
If ((opt And FILE_OPEN_NO_RECALL) <> 0) THen
  Result := Result + ' NoRecall';
If ((opt And FILE_OPEN_FOR_FREE_SPACE_QUERY) <> 0) THen
  Result := Result + ' FreeSpaceQuery';
end;

Class Function TIRPRequest.ShareAccessToString(AAccess:Cardinal):WideString;
begin
Result := '';
If ((AAccess And FILE_SHARE_READ) <> 0) Then
  Result := Result + ' Read';
If ((AAccess And FILE_SHARE_WRITE) <> 0) Then
  Result := Result + ' Write';
If ((AAccess And FILE_SHARE_DELETE) <> 0) Then
  Result := Result + ' Delete';

If Result <> '' Then
  Delete(Result, 1, 1);
end;

Class Function TIRPRequest.Build(Var ARequest:REQUEST_IRP):TIRPRequest;
begin
Result := Nil;
Case ARequest.MajorFunction Of
  0 : Result := TCreateRequest.Create(ARequest);
  2, 18 : Result := TCloseCleanupRequest.Create(ARequest);
  3, 4   : Result := TReadWriteRequest.Create(ARequest);
  5, 6   : Result := TQuerySetRequest.Create(ARequest);
  10, 11 : Result := TQuerySetVolumeRequest.Create(ARequest);
  12 : ;     // DirectoryControl
  13 : Result := TFileSystemControlRequest.Create(ARequest);
  14, 15 : Result := TDeviceControlRequest.Create(ARequest);
  20 : Result := TQuerySecurityRequest.Create(ARequest);
  21 : Result := TSetSecurityRequest.Create(ARequest);
  22 : begin
    Case ARequest.MinorFunction Of
      0 : Result := TWaitWakeRequest.Create(ARequest);
      1 : Result := TPowerSequenceRequest.Create(ARequest);
      2, 3 : Result := TQuerySetPowerRequest.Create(ARequest);
      end;
    end;
  27 : begin // PnP
    Case ARequest.MinorFunction Of
      $0 : Result := TStartDeviceRequest.Create(ARequest);
      $1 : Result := TQueryRemoveDeviceRequest.Create(ARequest);
      $2 : Result := TRemoveDeviceRequest.Create(ARequest);
      $3 : Result := TCancelRemoveDeviceRequest.Create(ARequest);
      $4 : Result := TStopDeviceRequest.Create(ARequest);
      $5 : Result := TQueryStopDeviceRequest.Create(ARequest);
      $6 : Result := TCancelStopDeviceRequest.Create(ARequest);
      $7 : Result := TQueryDeviceRelationsRequest.Create(ARequest);
      $8 : Result := TQueryInterfaceRequest.Create(ARequest);
      $9 : Result := TQueryDeviceCapabilitiesRequest.Create(ARequest);
      $C : Result := TQueryDeviceTextRequest.Create(ARequest);
      $11 : Result := TEjectDeviceRequest.Create(ARequest);
      $13 : Result := TQueryIdRequest.Create(ARequest);
      $14 : Result := TQueryDeviceStateRequest.Create(ARequest);
      $17 : Result := TSurpriseDeviceRequest.Create(ARequest);
      $19 : Result := TEnumeratedDeviceRequest.Create(ARequest);
      end;
    end;
  end;

If Not Assigned(Result) Then
  Result := TIRPRequest.Create(ARequest);
end;

(** TZeroArgIRPRequest **)

Function TZeroArgIRPRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1,
  rlmctArg2,
  rlmctArg3,
  rlmctArg4 : Result := False;
  Else Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

(** TCreateRequest **)

Function TCreateRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('0x%p', [FArgs.Create.SecurityContext]);
  rlmctArg2: AResult := CreateOptionsToString(FArgs.Create.Options);
  rlmctArg3: AResult := ShareAccessToString(FArgs.Create.ShareAccess);
  rlmctArg4: AResult := Format('%u', [FArgs.Create.EaLength]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TCreateRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Security context';
  rlmctArg2 : Result := 'Options';
  rlmctArg3 : Result := 'Access and attributes';
  rlmctArg4 : Result := 'EA length';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TDeviceControlRequest **)

Function TDeviceControlRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('%u', [FArgs.DeviceControl.OutputBufferLength]);
  rlmctArg2: AResult := Format('%u', [FArgs.DeviceControl.InputBufferLength]);
  rlmctArg3: AResult := IOCTLToString(FArgs.DeviceControl.IoControlCode);
  rlmctArg4: AResult := Format('0x%p', [FArgs.DeviceControl.Type3InputBuffer]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TDeviceControlRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Output length';
  rlmctArg2 : Result := 'Input length';
  rlmctArg3 : Result := 'IOCTL';
  rlmctArg4 : Result := 'Type3 input';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TFileSystemControlRequest **)

Function TFileSystemControlRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('%u', [FArgs.DeviceControl.OutputBufferLength]);
  rlmctArg2: AResult := Format('%u', [FArgs.DeviceControl.InputBufferLength]);
  rlmctArg3: AResult := IOCTLToString(FArgs.DeviceControl.IoControlCode);
  rlmctArg4: AResult := Format('0x%p', [FArgs.DeviceControl.Type3InputBuffer]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TFileSystemControlRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Output length';
  rlmctArg2 : Result := 'Input length';
  rlmctArg3 : Result := 'FSCTL';
  rlmctArg4 : Result := 'Type3 input';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TReadWriteRequest **)

Function TReadWriteRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('%u', [FArgs.ReadWrite.Length]);
  rlmctArg2: AResult := Format('0x%x', [FArgs.ReadWrite.Key]);
  rlmctArg3: AResult := Format('0x%x', [FArgs.ReadWrite.ByteOffset]);
  rlmctArg4: Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TReadWriteRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Length';
  rlmctArg2 : Result := 'Key';
  rlmctArg3 : Result := 'Offset';
  rlmctArg4 : Result := '';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TQuerySetRequest **)


Class Function TQuerySetRequest.InformationClassToString(AClass:Cardinal):WideString;
begin
Result := '';
If AClass < 76 Then
  Result := FIleInformationClassArray[AClass]
Else Format('%u', [AClass]);
end;

Function TQuerySetRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('%u', [FArgs.QuerySetInformation.Lenth]);
  rlmctArg2: AResult := InformationClassToString(FArgs.QuerySetInformation.FileInformationClass);
  rlmctArg3,
  rlmctArg4: Result := False
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TQuerySetRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Length';
  rlmctArg2 : Result := 'Information class';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TQuerySetVolumeRequest **)

Class Function TQuerySetVolumeRequest.InformationClassToString(AInfoClass:Cardinal):WideString;
begin
Case AInfoClass Of
  1 : Result := 'FileFsVolumeInformation';
  2 : Result := 'FileFsLabelInformation';
  3 : Result := 'FileFsSizeInformation';
  4 : Result := 'FileFsDeviceInformation';
  5 : Result := 'FileFsAttributeInformation';
  6 : Result := 'FileFsControlInformation';
  7 : Result := 'FileFsFullSizeInformation';
  8 : Result := 'FileFsObjectIdInformation';
  9 : Result := 'FileFsDriverPathInformation';
  10 : Result := 'FileFsVolumeFlagsInformation';
  11 : Result := 'FileFsSectorSizeInformation';
  12 : Result := 'FileFsDataCopyInformation';
  13 : Result := 'FileFsMetadataSizeInformation';
  Else Result := Format('%u', [AInfoClass]);
  end;
end;

Function TQuerySetVolumeRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('%u', [FArgs.QuerySetInformation.Lenth]);
  rlmctArg2: AResult := InformationClassToString(Args.QuerySetVolume.FileInformationClass);
  rlmctArg3,
  rlmctArg4: Result := False
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TQuerySetVolumeRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Length';
  rlmctArg2 : Result := 'Information class';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TQuerySecurityRequest **)

Function TQuerySecurityRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := SecurityInformationToString(FArgs.QuerySecurity.SecurityInformation);
  rlmctArg2: AResult := Format('%u', [FArgs.QuerySecurity.Length]);
  rlmctArg3,
  rlmctArg4: Result := False
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TQuerySecurityRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Security information';
  rlmctArg2 : Result := 'Length';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TSetSecurityRequest **)

Function TSetSecurityRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := SecurityInformationToString(FArgs.SetSecurity.SecurityInformation);
  rlmctArg2: AResult := Format('0x%p', [FArgs.SetSecurity.SecurityDescriptor]);
  rlmctArg3,
  rlmctArg4: Result := False
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TSetSecurityRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Security information';
  rlmctArg2 : Result := 'Descriptor';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TWaitWakeRequest **)

Function TWaitWakeRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1 : AResult := PowerStateToString(1, FArgs.WaitWake.PowerState);
  rlmctArg2,
  rlmctArg3,
  rlmctArg4 : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TWaitWakeRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Power state';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TPowerSequenceRequest **)

Function TPowerSequenceRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1,
  rlmctArg2,
  rlmctArg3,
  rlmctArg4 : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

(** TQuerySetPowerRequest **)

Function TQuerySetPowerRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1 : AResult := Format('%u', [FArgs.QuerySetPower.SystemContext]);
  rlmctArg2 : AResult := PowerStateTypeToString(FArgs.QuerySetPower.PowerStateType);
  rlmctArg3 : AResult := PowerStateToString(FArgs.QuerySetPower.PowerStateType, FArgs.QuerySetPower.PowerState);
  rlmctArg4 : AResult := ShutdownTypeToString(FArgs.QuerySetPower.ShutdownType);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TQuerySetPowerRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'System context';
  rlmctArg2 : Result := 'Power state type';
  rlmctArg3 : Result := 'Power state';
  rlmctArg4 : Result := 'Shutdown type';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TCloseCleanupRequest **)

Function TCloseCleanupRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1,
  rlmctArg2,
  rlmctArg3,
  rlmctArg4 : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

(** TQueryIdRequest **)

Function TQueryIdRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1 : AResult := BusQueryIdTypeToString(FArgs.QueryId.IdType);
  rlmctArg2,
  rlmctArg3,
  rlmctArg4 : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TQueryIdRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'ID type';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;


(** TQueryDeviceRelationsRequest **)

Function TQueryDeviceRelationsRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1 : AResult := DeviceRelationTypeToString(FArgs.QueryDeviceRelations.DeviceRelationType);
  rlmctArg2,
  rlmctArg3,
  rlmctArg4 : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TQueryDeviceRelationsRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Relation type';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TQueryDeviceTextRequest **)

Function TQueryDeviceTextRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1 : AResult := DeviceTextTypeToString(FArgs.QueryText.TextType);
  rlmctArg2 : AResult := Format('%u', [FArgs.QueryText.LocaleId]);
  rlmctArg3,
  rlmctArg4 : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Function TQueryDeviceTextRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Text type';
  rlmctArg2 : Result := 'Locale';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TQueryDeviceCapabilitiesRequest **)

Function TQueryDeviceCapabilitiesRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Capabilities';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

Function TQueryDeviceCapabilitiesRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1 : AResult := Format('0x%p', [FArgs.QueryCapabilities.Capabilities]);
  rlmctArg2,
  rlmctArg3,
  rlmctArg4 : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

(** TQueryInterfaceRequest **)

Function TQueryInterfaceRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Case AColumnType Of
  rlmctArg1 : Result := 'Interface type';
  rlmctArg2 : Result := 'Size | Version';
  rlmctArg3 : Result := 'Routines';
  rlmctArg4 : Result := 'Specific';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

Function TQueryInterfaceRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1 : AResult := Format('0x%p', [FArgs.QueryInterface.InterfaceType]);
  rlmctArg2 : AResult := Format('%u | %u', [FArgs.QueryInterface.Size, FArgs.QueryInterface.Version]);
  rlmctArg3 : AResult := Format('0x%p', [FArgs.QueryInterface.InterfaceRoutines]);
  rlmctArg4 : AResult := Format('0x%p', [FArgs.QueryInterface.InterfaceSpecificData]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


End.

