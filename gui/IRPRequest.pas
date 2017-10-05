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
    FFileObject : Pointer;
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
    Constructor Create(Var ARequest:REQUEST_IRP); Reintroduce;

    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    Class Function PowerStateTypeToString(AType:Cardinal):WideString;
    Class Function PowerStateToString(AType:Cardinal; AState:Cardinal):WideString;
    Class Function ShutdownTypeToString(AType:Cardinal):WideString;
    Class Function BusQueryIdTypeToString(AType:BUS_QUERY_ID_TYPE):WideString;
    Class Function DeviceTextTypeToString(AType:DEVICE_TEXT_TYPE):WideString;
    Class Function DeviceRelationTypeToString(AType:DEVICE_RELATION_TYPE):WideString;
    Class Function Build(Var ARequest:REQUEST_IRP):TIRPRequest;

    Property FileObject : Pointer Read FFileObject;
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

  TDeviceControlRequest = Class (TIRPRequest)
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
begin
Inherited Create(ARequest.Header);
FMajorFunction := ARequest.MajorFunction;
FMinorFunction := ARequest.MinorFunction;
FPreviousMode := ARequest.PreviousMode;
FRequestorMode := ARequest.RequestorMode;
FIRPAddress := ARequest.IRPAddress;
FIRPFlags := ARequest.IrpFlags;
FFileObject := ARequest.FileObject;
FArgs := ARequest.Args;
FRequestorProcessId := ARequest.RequestorProcessId;
end;


Function TIRPRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctRequestorPID : AResult := Format('%d', [FRequestorProcessId]);
  rlmctSubType: AResult := Format('%s:%s', [MajorFunctionToString(FMajorFunction), MinorFunctionToString(FMajorFunction, FMinorFunction)]);
  rlmctIRPAddress: AResult := Format('0x%p', [FIRPAddress]);
  rlmctFileObject: AResult := Format('0x%p', [FFileObject]);
  rlmctIRPFlags: AResult := Format('0x%x', [FIRPFlags]);
  rlmctArg1: AResult := Format('0x%p', [FArgs.Other.Arg1]);
  rlmctArg2: AResult := Format('0x%p', [FArgs.Other.Arg2]);
  rlmctArg3: AResult := Format('0x%p', [FArgs.Other.Arg3]);
  rlmctArg4: AResult := Format('0x%p', [FArgs.Other.Arg4]);
  rlmctPreviousMode: AResult := AccessModeToString(FPreviousMode);
  rlmctRequestorMode: AResult := AccessModeToString(FRequestorMode);
  rlmctIOSBStatus : AResult := Format('0x%x (%s)', [FIOSBStatus, NTSTATUSToString(FIOSBStatus)]);
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

Class Function TIRPRequest.Build(Var ARequest:REQUEST_IRP):TIRPRequest;
begin
Result := Nil;
Case ARequest.MajorFunction Of
  2, 18 : Result := TCloseCleanupRequest.Create(ARequest);
  3, 4   : Result := TReadWriteRequest.Create(ARequest);
  5, 6   : Result := TQuerySetRequest.Create(ARequest);
  10, 11 : ; // QuerySetVolume
  12 : ;     // DirectoryControl
  13 : ;     // FsControl
  14, 15 : Result := TDeviceControlRequest.Create(ARequest);
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

(** TDeviceControlRequest **)

Function TDeviceControlRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('O: %u (0x%p)', [FArgs.DeviceControl.OutputBufferLength, Pointer(Args.DeviceControl.OutputBufferLength)]);
  rlmctArg2: AResult := Format('I: %u (0x%p)', [FArgs.DeviceControl.InputBufferLength, Pointer(Args.DeviceControl.InputBufferLength)]);
  rlmctArg3: AResult := IOCTLToString(FArgs.DeviceControl.IoControlCode);
  rlmctArg4: Result := False;
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

(** TReadWriteRequest **)

Function TReadWriteRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('L: %u', [FArgs.ReadWrite.Length]);
  rlmctArg2: AResult := Format('K: 0x%x', [FArgs.ReadWrite.Key]);
  rlmctArg3: AResult := Format('O: 0x%x', [FArgs.ReadWrite.ByteOffset]);
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

Function TQuerySetRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctArg1: AResult := Format('L: %u', [FArgs.QuerySetInformation.Lenth]);
  rlmctArg2: AResult := Format('I: %u', [FArgs.QuerySetInformation.FileInformationClass]);
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

