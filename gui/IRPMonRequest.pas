unit IRPMonRequest;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

interface

Uses
  Windows,
  IRPMonDll;


Type
  TGeneralRequest = Class
  Private
    FDataBufferAllocated : Boolean;
  Protected
    FId : Cardinal;
    FDriverName : WideString;
    FDeviceName : WideString;
    FFileName : WideString;
    FProcessName : WideString;
    FDriverObject : Pointer;
    FDeviceObject : Pointer;
    FFileObject : Pointer;
    FRequestType : ERequestType;
    FResultType : ERequestResultType;
    FResultValue : NativeUInt;
    FTime : UInt64;
    FThreadId : THandle;
    FProcessId : THandle;
    FIRQL : Byte;
    FDataSize : NativeUInt;
    FData : Pointer;
    FRaw : PREQUEST_HEADER;
    FRawSize : Cardinal;
    FEmulated : Boolean;
    FDataPresent : Boolean;
    FDataStripped : Boolean;
    FAdmin : Boolean;
    FImpersonated : Boolean;
    FImpersonatedAdmin : Boolean;
    Procedure SetDriverName(AName:WideString);
    Procedure SetDeviceName(AName:WideString);
    Procedure SetFileName(AName:WideString);
    Procedure SetProcessName(AName:WideString);
    Procedure SetFileObject(AObject:Pointer);
  Public
    Constructor Create(Var ARequest:REQUEST_HEADER); Overload;
    Destructor Destroy; Override;

    Function AssignData(AData:Pointer; ASize:NativeUInt; AWithinRequest:Boolean = True):Boolean;

    Class Function IOCTLToString(AControlCode:Cardinal):WideString;
    Class Function RequestTypeToString(ARequestType:ERequestType):WideString;
    Class Function RequestResultToString(AResult:NativeUInt; AResultType:ERequestResultType; AConstant:Boolean = False):WideString;
    Class Function NTSTATUSToString(AValue:Cardinal):WideString;
    Class Function AccessModeToString(AMode:Byte):WideString;
    Class Function IRQLToString(AValue:Byte):WideString;
    Class Function MajorFunctionToString(AMajor:Byte):WideString;
    Class Function MinorFunctionToString(AMajor:Byte; AMinor:Byte):WideString;
    Class Function ImageSignatureTypeToString(AType:EImageSignatureType):WideString;
    Class Function ImageSigningLevelToString(ALevel:EImageSigningLevel):WideString;

    Property Id : Cardinal Read FId;
    Property DriverName : WideString Read FDriverName Write SetDriverName;
    Property DeviceName : WideString Read FDeviceName Write SetDeviceName;
    Property FileName : WideString Read FFileName Write SetFileName;
    Property DriverObject : Pointer Read FDriverObject;
    Property DeviceObject : Pointer Read FDeviceObject;
    Property FileObject : Pointer Read FFileObject Write SetFileObject;
    Property RequestType : ERequestType Read FRequestType;
    Property ResultType : ERequestResultType Read FResultType;
    Property ResultValueRaw : NativeUInt Read FResultValue;
    Property TimeRaw : UInt64 Read FTime;
    Property ThreadId : THandle Read FThreadId;
    Property ProcessId : THandle Read FProcessId;
    Property IRQL : Byte Read FIRQL;
    Property DataSize : NativeUInt Read FDataSize;
    Property Data : Pointer Read FData;
    Property Raw : PREQUEST_HEADER Read FRaw;
    Property RawSize : Cardinal Read FRawSize;
    Property Emulated : Boolean Read FEmulated;
    Property Admin : Boolean Read FAdmin;
    Property Impersonated : Boolean Read FImpersonated;
    Property DataPresent : Boolean Read FDataPresent;
    Property DataStripped : Boolean Read FDataStripped;
    Property ProcessName : WideString Read FProcessName;
    Property ImpersonatedAdmin : Boolean Read FImpersonatedAdmin;
  end;

implementation

Uses
  SysUtils, NameTables;

Constructor TGeneralRequest.Create(Var ARequest:REQUEST_HEADER);
begin
Inherited Create;
FId := ARequest.Id;
FDriverObject := ARequest.Driver;
FDriverName := '';
FDeviceObject := ARequest.Device;
FDeviceName := '';
FTime := ARequest.Time;
FRequestType := ARequest.RequestType;
FResultType := ARequest.ResultType;
FResultValue := NativeUInt(ARequest.Other);
FProcessId := ARequest.ProcessId;
FThreadId := ARequest.ThreadId;
FIRQL := ARequest.Irql;
FData := Nil;
FDataSize := 0;
FRaw := RequestCopy(@ARequest);
If Not Assigned(FRaw) THen
  Raise Exception.Create('Not enough memory');

FRawSize := RequestGetSize(FRaw);
If FRawSize = 0 Then
  Raise Exception.Create('Request raw size is zero');

FRaw.Flags := (FRaw.Flags And Not (REQUEST_FLAG_NEXT_AVAILABLE));
FRaw.Next := Nil;
FEmulated := (ARequest.Flags And REQUEST_FLAG_EMULATED) <> 0;
FDataStripped := (ARequest.Flags And REQUEST_FLAG_DATA_STRIPPED) <> 0;
FAdmin := (ARequest.Flags And REQUEST_FLAG_ADMIN) <> 0;
FImpersonated := (ARequest.Flags And REQUEST_FLAG_IMPERSONATED) <> 0;
FImpersonatedAdmin := (ARequest.Flags And REQUEST_FLAG_IMPERSONATED_ADMIN) <> 0;
end;

Destructor TGeneralRequest.Destroy;
begin
If Assigned(FRaw) Then
  RequestMemoryFree(FRaw);

If FDataBufferAllocated Then
  FreeMem(FData);

Inherited Destroy;
end;

Function TGeneralRequest.AssignData(AData:Pointer; ASize:NativeUInt; AWithinRequest:Boolean = True):Boolean;
begin
Result := True;
If ASize > 0 Then
  begin
  FDataPresent := True;
  If Not AWithinRequest Then
    begin
    Result := Not Assigned(FData);
    If Result Then
      begin
      FData := AllocMem(ASize);
      Result := Assigned(FData);
      If Result Then
        begin
        Move(AData^, FData^, ASize);
        FDataSize := ASize;
        FDataBufferAllocated := True;
        end;
      end;
    end
  Else begin
    FDataSize := ASize;
    FData := PByte(FRaw) + FRawSize - FDataSize;
    end;
  end;
end;

Procedure TGeneralRequest.SetDriverName(AName:WideString);
begin
FDriverName := AName;
end;

Procedure TGeneralRequest.SetDeviceName(AName:WideString);
begin
FDeviceName := AName;
end;

Procedure TGeneralRequest.SetFileName(AName:WideString);
begin
FFileName := AName;
end;

Procedure TGeneralRequest.SetProcessName(AName:WideString);
begin
FProcessName := AName;
end;

Procedure TGeneralRequest.SetFileObject(AObject:Pointer);
begin
FFileObject := AObject;
end;


Class Function TGeneralRequest.IOCTLToString(AControlCode:Cardinal):WideString;
begin
Result := TablesIOCTLToString(AControlCode);
end;

Class Function TGeneralRequest.MajorFunctionToString(AMajor:Byte):WideString;
begin
Case AMajor Of
  0 : Result := 'Create';
  1 : Result := 'CreateNamedPipe';
  2 : Result := 'Close';
  3 : Result := 'Read';
  4 : Result := 'Write';
  5 : Result := 'Query';
  6 : Result := 'Set';
  7 : Result := 'QueryEA';
  8 : Result := 'SetEA';
  9 : Result := 'Flush';
  10 : Result := 'QueryVolume';
  11 : Result := 'SetVolume';
  12 : Result := 'DirectoryControl';
  13 : Result := 'FSControl';
  14 : Result := 'DeviceControl';
  15 : Result := 'InternalDeviceControl';
  16 : Result := 'Shutdown';
  17 : Result := 'Lock';
  18 : Result := 'Cleanup';
  19 : Result := 'CreateMailslot';
  20 : Result := 'QuerySecurity';
  21 : Result := 'SetSecurity';
  22 : Result := 'Power';
  23 : Result := 'SystemControl';
  24 : Result := 'DeviceChange';
  25 : Result := 'QueryQuota';
  26 : Result := 'SetQuota';
  27 : Result := 'PnP';
  Else Result := Format('0x%x>', [AMajor]);
  end;
end;

Class Function TGeneralRequest.MinorFunctionToString(AMajor:Byte; AMinor:Byte):WideString;
begin
Result := '';
Case AMajor Of
  27 : begin
    Case AMinor Of
      0 : Result := 'Start';
      1 : Result := 'QueryRemove';
      2 : Result := 'Remove';
      3 : Result := 'CancelRemove';
      4 : Result := 'Stop';
      5 : Result := 'QueryStop';
      6 : Result := 'CancelStop ';
      7 : Result := 'QueryRelations';
      8 : Result := 'QueryInterface';
      9 : Result := 'QueryCapabilities';
      10 : Result := 'QueryResources';
      11 : Result := 'QueryResourceRequirements';
      12 : Result := 'QueryDeviceText';
      13 : Result := 'FilterResourceRequirements';
      15 : Result := 'ReadConfig';
      16 : Result := 'WriteConfig';
      17 : Result := 'Eject';
      18 : Result := 'SetLock';
      19 : Result := 'QueryId';
      20 : Result := 'QueryState';
      21 : Result := 'QueryBusInfo';
      22 : Result := 'UsageNotifications';
      23 : Result := 'SurpriseRemoval';
      25 : Result := 'Enumerated';
      Else Result := Format('0x%x', [AMinor]);
      end;
    end;
  22 : begin
    Case AMinor Of
      0 : Result := 'WaitWake';
      1 : Result := 'PowerSequence';
      2 : Result := 'SetPower';
      3 : Result := 'QueryPower';
      end;
    end;
  23 : begin
    Case AMinor Of
      0 : Result := 'QueryAllData';
      1 : Result := 'QuerySingleInstance';
      2 : Result := 'ChangeSingleInstance';
      3 : Result := 'ChangeSingleItem';
      4 : Result := 'EnableEvents';
      5 : Result := 'DisableEvents';
      6 : Result := 'EnableCollection';
      7 : Result := 'DisableCollection';
      8 : Result := 'RegInfo';
      9 : Result := 'Execute';
      11 : Result := 'RegInfoEx';
      Else Result := Format('0x%x', [AMinor]);
      end;
    end;
  12 : begin
    Case AMinor Of
      1 : Result := 'QueryDirectory';
      2 : Result := 'ChangeNotify';
      Else Result := Format('0x%x', [AMinor]);
      end;
    end;
  13 : begin
    Case AMinor Of
      0 : Result := 'UserRequest';
      1 : Result := 'MountVolume';
      2 : Result := 'VerifyVolume';
      3 : Result := 'LoadFS';
      4 : Result := 'KernelCall';
      Else Result := Format('0x%x', [AMinor]);
      end;
    end;
  17 : begin
    Case AMinor Of
      1 : Result := 'Lock';
      2 : Result := 'UnlockSingle';
      3 : Result := 'UnlockAll';
      4 : Result := 'UnlockAllByKey';
      Else Result := Format('0x%x', [AMinor]);
      end;
    end;
  9 : begin
    Case AMinor Of
      1 : Result := 'FlushAndPurge';
      2 : Result := 'DataOnly';
      3 : Result := 'NoSync';
      Else Result := Format('0x%x', [AMinor]);
      end;
    end;
  3,
  4 : begin
    Case AMinor Of
      0 : Result := 'Normal';
      1 : Result := 'Dpc';
      2 : Result := 'Mdl';
      3 : Result := 'Dpc,Mdl';
      4 : Result := 'Complete';
      5 : Result := 'Complete,Dpc';
      6 : Result := 'Complete,Mdl';
      7 : Result := 'Complete,Dpc,Mdl';
      8 : Result := 'Compressed';
      9 : Result := 'Compressed,Dpc';
      10 : Result := 'Compressed,Mdl';
      11 : Result := 'Compressed,Dpc,Mdl';
      12 : Result := 'Complete,Compressed';
      13 : Result := 'Complete,Compressed,Dpc';
      14 : Result := 'Complete,Compressed,Mdl';
      15 : Result := 'Complete,Compressed,Dpc,Mdl';
      end;
    end;
  end;
end;

Class Function TGeneralRequest.RequestTypeToString(ARequestType:ERequestType):WideString;
begin
Case ARequestType Of
  ertIRP: Result := 'IRP';
  ertIRPCompletion: Result := 'IRPComp';
  ertAddDevice: Result := 'AddDevice';
  ertDriverUnload: Result := 'Unload';
  ertFastIo: Result := 'FastIo';
  ertStartIo: Result := 'StartIo';
  ertDriverDetected : Result := 'DriverDetected';
  ertDeviceDetected : Result := 'DeviceDetected';
  ertFileObjectNameAssigned : Result := 'FONameAssigned';
  ertFileObjectNameDeleted : Result := 'FONameDeleted';
  ertProcessCreated : Result := 'ProcessCreate';
  ertProcessExitted : Result := 'ProcessExit';
  ertImageLoad : Result := 'ImageLoad';
  Else Result := Format('<unknown> (%u)', [Ord(ARequestType)]);
  end;
end;

Class Function TGeneralRequest.RequestResultToString(AResult:NativeUInt; AResultType:ERequestResultType; AConstant:Boolean = False):WideString;
begin
Case AResultType Of
  rrtUndefined: Result := 'None';
  rrtNTSTATUS: begin
    If AConstant Then
      Result := Format('%s', [NTSTATUSToString(AResult)])
    Else Result := Format('0x%x', [AResult]);
    end;
  rrtBOOLEAN: begin
    If AResult <> 0 Then
      Result := 'TRUE'
    Else Result := 'FALSE';
    end;
  Else Result := '';
  end;
end;

Class Function TGeneralRequest.NTSTATUSToString(AValue:Cardinal):WideString;
begin
Result := TablesNTSTATUSToString(AValue);
end;

Class Function TGeneralRequest.IRQLToString(AValue:Byte):WideString;
begin
Result := Format('%u', [AValue]);
Case AValue Of
  0 : Result := 'Passive';
  1 : Result := 'APC';
  2 : Result := 'Dispatch';
{$IFDEF WIN32}
  27 : Result := 'Profile';
  28 : Result := 'Clock';
  29 : Result := 'IPI';
  30 : Result := 'Power';
  31 : Result := 'High';
{$ELSE}
  13 : Result := 'Clock';
  14 : Result := 'Profile, IPI, Power';
  15 : Result := 'High';
{$ENDIF}
 Else Result := 'Interrupt';
  end;
end;

Class Function TGeneralRequest.AccessModeToString(AMode:Byte):WideString;
begin
Case AMode Of
  0 : Result := 'KernelMode';
  1 : Result := 'UserMode';
  Else Result := Format('<unknown> (%u)', [AMode]);
  end;
end;

Class Function TGeneralRequest.ImageSignatureTypeToString(AType:EImageSignatureType):WideString;
begin
Result := '';
Case AType Of
  istNone: Result := 'None';
  istEmbedded: Result := 'Embedded';
  istCache: Result := 'Cached';
  istCatalogCached: Result := 'Catalog cached';
  istCatalogNotCached: Result := 'Catalog';
  istCatalogHint: Result := 'Catalog hint';
  istPackageCatalog: Result := 'Catalog package';
  end;
end;

Class Function TGeneralRequest.ImageSigningLevelToString(ALevel:EImageSigningLevel):WideString;
begin
Result := '';
Case ALevel Of
  islUnchecked: Result := 'Unchecked';
  islUnsigned: Result := 'Unsigned';
  islEnterprise: Result := 'Enterprise';
  islDeveloper: Result := 'Developer';
  islAuthenticode: Result := 'Authenticode';
  islCustom2: Result := 'Custom2';
  islStore: Result := 'Store';
  islAntiMalware: Result := 'Anti-malware';
  islMicrosoft: Result := 'Microsoft';
  islCustom4: Result := 'Custom4';
  islCustom5: Result := 'Custom5';
  islDynamicCode: Result := 'DynamicCode';
  islWindows: Result := 'Windows';
  islCustom7: Result := 'Custom7';
  islWindowsTCB: Result := 'Windows TCB';
  islCustom6: Result := 'Custom6';
  end;
end;


end.
