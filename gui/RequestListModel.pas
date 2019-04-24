Unit RequestListModel;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Classes, Generics.Collections, Generics.Defaults,
  IRPMonDll, ListModel, IRPMonRequest, DataParsers;


Type
  ERequestListModelColumnValueType = (
    rlmcvtInteger,
    rlmcvtString,
    rlmcvtTime,
    rlmcvtMajorFunction,
    rlmcvtMinorFunction,
    rlmcvtProcessorMode,
    rlmcvtIRQL,
    rlmcvtRequestType
  );

  ERequestListModelColumnType = (
    rlmctId,
    rlmctTime,
    rlmctRequestType,
    rlmctDeviceObject,
    rlmctDeviceName,
    rlmctDriverObject,
    rlmctDriverName,
    rlmctResultValue,
    rlmctResultConstant,
    rlmctSubType,
    rlmctMinorFunction,
    rlmctIRPAddress,
    rlmctFileObject,
    rlmctFileName,
    rlmctIRPFlags,
    rlmctArg1,
    rlmctArg2,
    rlmctArg3,
    rlmctArg4,
    rlmctThreadId,
    rlmctProcessId,
    rlmctIRQL,
    rlmctPreviousMode,
    rlmctRequestorMode,
    rlmctIOSBStatusValue,
    rlmctIOSBStatusConstant,
    rlmctIOSBInformation,
    rlmctRequestorPID);
  PERequestListModelColumnType = ^ERequestListModelColumnType;

  RequestListModelColumnSet = Set Of ERequestListModelColumnType;

Const
  RequestListModelColumnNames : Array [0..Ord(rlmctRequestorPID)] Of String = (
    'ID',
    'Time',
    'Type',
    'Device object',
    'Device name',
    'Driver object',
    'Driver name',
    'Result value',
    'Result constant',
    'Subtype',
    'Minor function',
    'IRP address',
    'File object',
    'File name',
    'IRP flags',
    'Argument1',
    'Argument2',
    'Argument3',
    'Argument4',
    'Thread ID',
    'Process ID',
    'IRQL',
    'Previous mode',
    'Requestor mode',
    'IOSB.Status value',
    'IOSB.Status constant',
    'IOSB.Information',
    'Requestor PID'
  );

  RequestListModelColumnValueTypes : Array [0..Ord(rlmctRequestorPID)] Of ERequestListModelColumnValueType = (
    rlmcvtInteger,
    rlmcvtTime,
    rlmcvtRequestType,
    rlmcvtInteger,
    rlmcvtString,
    rlmcvtInteger,
    rlmcvtString,
    rlmcvtInteger,
    rlmcvtString,
    rlmcvtMajorFunction,
    rlmcvtMinorFunction,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtString,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtIRQL,
    rlmcvtProcessorMode,
    rlmcvtProcessorMode,
    rlmcvtInteger,
    rlmcvtString,
    rlmcvtInteger,
    rlmcvtInteger
  );

Type
  TDriverRequest = Class (TGeneralRequest)
  Private
    Procedure ProcessParsers(AParsers:TObjectList<TDataParser>; ALines:TStrings);
  Public
    Class Function GetBaseColumnName(AColumnType:ERequestListModelColumnType):WideString;
    Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Virtual;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Virtual;
    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Virtual;
    Procedure SaveToStream(AStream:TStream; AParsers:TObjectList<TDataParser>); Virtual;
    Procedure SaveToFile(AFileName:WideString; AParsers:TObjectList<TDataParser>); Virtual;
  end;

  TDriverRequestComparer = Class (TComparer<TDriverRequest>)
  Public
{$IFDEF FPC}
    Function Compare(Constref Left, Right:TDriverRequest):Integer; Override;
{$ELSE}
    Function Compare(Const Left, Right:TDriverRequest):Integer; Override;
{$ENDIF}
  end;

  TDriverUnloadRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_UNLOAD); Reintroduce;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
  end;

  TAddDeviceRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_ADDDEVICE); Reintroduce;
  end;


  TIRPCompleteRequest = Class (TDriverRequest)
  Private
    FIRPAddress : Pointer;
    FIOSBStatus : Cardinal;
    FIOSBInformation : NativeUInt;
    FProcessId : THandle;
    FThreadId : THandle;
  Public
    Constructor Create(Var ARequest:REQUEST_IRP_COMPLETION); Reintroduce;

    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    Property Address : Pointer Read FIRPAddress;
    Property IOSBStatus : Cardinal Read FIOSBStatus;
    Property IOSBInformation : NativeUInt Read FIOSBInformation;
    Property ProcessId : THandle Read FProcessId;
    Property ThreadId : THandle Read FThreadId;
  end;

  TStartIoRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_STARTIO); Reintroduce;
  end;

  TRequestListModel = Class (TListModel<TDriverRequest>)
    Private
      FRequests : TList<TDriverRequest>;
      FDriverMap : TDictionary<Pointer, WideString>;
      FDeviceMap : TDictionary<Pointer, WideString>;
      FFileMap : TDictionary<Pointer, WideString>;
      FParsers : TObjectList<TDataParser>;
    Protected
      Function GetColumn(AItem:TDriverRequest; ATag:NativeUInt):WideString; Override;
      Procedure FreeItem(AItem:TDriverRequest); Override;
      Function _Item(AIndex:Integer):TDriverRequest; Override;
    Public
      UpdateRequest : TList<PREQUEST_GENERAL>;
      Constructor Create; Reintroduce;
      Destructor Destroy; Override;
      Function RefreshMaps:Cardinal;
      Procedure Sort;

      Procedure Clear; Override;
      Function RowCount : Cardinal; Override;
      Function Update:Cardinal; Override;
      Procedure SaveToStream(AStream:TStream);
      Procedure SaveToFile(AFileName:WideString);

      Property Parsers : TObjectList<TDataParser> Read FParsers Write FParsers;
    end;

Implementation

Uses
  SysUtils, NameTables, IRPRequest, FastIoRequest,
  XXXDetectedRequests, FileObjectNameXXXRequest, Utils;

(** TDriverRequestComparer **)

{$IFDEF FPC}
Function TDriverRequestComparer.Compare(Constref Left, Right:TDriverRequest):Integer;
{$ELSE}
Function TDriverRequestComparer.Compare(Const Left, Right:TDriverRequest):Integer;
{$ENDIF}
begin
Result := Integer(Left.Id - Right.Id);
end;

(** TDriverRequest **)

Procedure TDriverRequest.ProcessParsers(AParsers:TObjectList<TDataParser>; ALines:TStrings);
Var
  I : Integer;
  err : Cardinal;
  _handled : ByteBool;
  names : TStringList;
  values : TStringList;
  pd : TDataParser;
begin
If Assigned(AParsers) Then
  begin
  names := TStringList.Create;
  values := TStringList.Create;
  For pd In AParsers Do
    begin
    err := pd.Parse(Self, _handled, names, values);
    If (err = ERROR_SUCCESS) And (_handled) Then
      begin
      ALines.Add(Format('Data (%s)', [pd.Name]));
      For I := 0 To values.Count - 1 Do
        begin
        If names.Count > 0 Then
          ALines.Add(Format('  %s: %s', [names[I], values[I]]))
        Else ALines.Add('  ' + values[I]);
        end;

      values.Clear;
      names.Clear;
      end;
    end;

  values.Free;
  names.Free;
  end;
end;


Procedure TDriverRequest.SaveToStream(AStream: TStream; AParsers:TObjectList<TDataParser>);
Var
  s : TStringList;
  value : WideString;
  ct : ERequestListModelColumnType;
begin
s := TStringList.Create;
For ct := Low(ERequestListModelColumnType) To High(ERequestListModelColumnType) Do
  begin
  If GetColumnValue(ct, value) Then
    s.Add(Format('%s = %s', [GetColumnName(ct), value]));
  end;

If DataSize > 0 Then
  begin
  S.Add(Format('Data size = %d', [DataSize]));
  ProcessParsers(AParsers, s);
  end;

s.Add('');
s.SaveToStream(AStream);
s.Free;
end;

Procedure TDriverRequest.SaveToFile(AFileName: WideString; AParsers:TObjectList<TDataParser>);
Var
  F : TFileStream;
begin
F := TFileStream.Create(AFileName, fmCreate Or fmOpenWrite);
Try
  SaveToStream(F, AParsers);
Finally
  F.Free;
  end;
end;

Function TDriverRequest.GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctId: begin
    AValue := @FId;
    AValueSIze := SizeOf(FId);
    end;
  rlmctTime: begin
    AValue := @FTime;
    AValueSIze := SizeOf(FTime);
    end;
  rlmctRequestType: begin
    AValue := @FRequestType;
    AValueSIze := SizeOf(FRequestType);
    end;
  rlmctDeviceObject: begin
    AValue := @FDeviceObject;
    AValueSIze := SizeOf(FDeviceObject);
    end;
  rlmctDeviceName: begin
    AValue := PWideChar(FDeviceName);
    AValueSize := 0;
    end;
  rlmctDriverObject: begin
    AValue := @FDriverObject;
    AValueSize := SizeOf(FDriverObject);
    end;
  rlmctDriverName: begin
    AValue := PWideChar(FDriverName);
    AValueSize := 0;
    end;
  rlmctResultValue: begin
    AValue := @FResultValue;
    AValueSIze := SizeOf(FTime);
    end;
  rlmctThreadId: begin
    AValue := @FThreadId;
    AValueSIze := SizeOf(FThreadId);
    end;
  rlmctProcessId: begin
    AValue := @FProcessId;
    AValueSIze := SizeOf(FProcessId);
    end;
  rlmctIRQL: begin
    AValue := @FIrql;
    AValueSIze := SizeOf(FIrql);
    end;
  Else Result := False;
  end;
end;

Function TDriverRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
Var
  s : SYSTEMTIME;
begin
Result := True;
AResult := '';
Case AColumnType Of
  rlmctId : AResult := Format('%u', [FId]);
  rlmctTime : begin
    FileTimeToSystemTime(FILETIME(FTime), s);
    AResult := DateTimeToStr(SystemTimeToDateTime(s));
    end;
  rlmctRequestType: AResult := RequestTypeToString(FRequestType);
  rlmctDeviceObject: AResult := Format('0x%p', [FDeviceObject]);
  rlmctDeviceName: AResult := FDeviceName;
  rlmctDriverObject: AResult := Format('0x%p', [FDriverObject]);
  rlmctDriverName: AResult := FDriverName;
  rlmctFileObject : AResult := Format('0x%p', [FFileObject]);
  rlmctFileName: AResult := FFileName;
  rlmctResultValue: AResult := RequestResultToString(FResultValue, FResultType);
  rlmctResultConstant: AResult := RequestResultToString(FResultValue, FResultType, True);
  rlmctProcessId : AResult := Format('%u', [FProcessId]);
  rlmctThreadId :  AResult := Format('%u', [FThreadId]);
  rlmctIRQL : AResult := IRQLToString(FIRQL);
  Else Result := False;
  end;
end;

Function TDriverRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Result := GetBaseColumnName(AColumnType);
end;

Class Function TDriverRequest.GetBaseColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Result := RequestListModelColumnNames[Ord(AColumnType)];
end;



(** TDriverUnloadRequest **)

Constructor TDriverUnloadRequest.Create(Var ARequest:REQUEST_UNLOAD);
begin
Inherited Create(ARequest.Header);
end;

Function TDriverUnloadRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResultValue,
  rlmctResultConstant  : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

(** TAddDeviceRequest **)

Constructor TAddDeviceRequest.Create(Var ARequest:REQUEST_ADDDEVICE);
begin
Inherited Create(ARequest.Header);
end;


(** TIRPCompleteRequest **)

Constructor TIRPCompleteRequest.Create(Var ARequest:REQUEST_IRP_COMPLETION);
Var
  d : Pointer;
begin
Inherited Create(ARequest.Header);
d := PByte(@ARequest) + SizeOf(ARequest);
AssignData(d, ARequest.DataSize);
FIRPAddress := ARequest.IRPAddress;
FIOSBStatus := ARequest.CompletionStatus;
FIOSBInformation := ARequest.CompletionInformation;
end;

Function TIRPCompleteRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctIRPAddress: AResult := Format('0x%p', [FIRPAddress]);
  rlmctIOSBStatusValue : AResult := Format('0x%x', [FIOSBStatus]);
  rlmctIOSBStatusConstant : AResult := Format('%s', [NTSTATUSToString(FIOSBStatus)]);
  rlmctIOSBInformation : AResult := Format('0x%p', [Pointer(IOSBInformation)]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

(** TStartIoRequest **)

Constructor TStartIoRequest.Create(Var ARequest:REQUEST_STARTIO);
Var
  d : Pointer;
begin
Inherited Create(ARequest.Header);
d := PByte(@ARequest) + aRequest.DataSize;
AssignData(d, ARequest.DataSize);
end;

(** TRequestListModel **)

Function TRequestListModel.GetColumn(AItem:TDriverRequest; ATag:NativeUInt):WideString;
begin
Result := '';
AItem.GetColumnValue(ERequestListModelColumnType(ATag), Result);
end;

Procedure TRequestListModel.FreeItem(AItem:TDriverRequest);
begin
AItem.Free;
end;

Function TRequestListModel._Item(AIndex:Integer):TDriverRequest;
begin
Result := FRequests[AIndex];
end;

Function TRequestListModel.RowCount : Cardinal;
begin
Result := FRequests.Count;
end;

Function TRequestListModel.Update:Cardinal;
Var
  ur : PREQUEST_GENERAL;
  dr : TDriverRequest;
  deviceName : WideString;
  driverName : WideString;
  fileName : WideString;
begin
Result := ERROR_SUCCESS;
If Assigned(UpdateRequest) Then
  begin
  For ur In UpdateRequest Do
    begin
    Case ur.Header.RequestType Of
      ertIRP: dr := TIRPRequest.Build(ur.Irp);
      ertIRPCompletion: dr := TIRPCompleteRequest.Create(ur.IrpComplete);
      ertAddDevice: dr := TAddDeviceRequest.Create(ur.AddDevice);
      ertDriverUnload: dr := TDriverUnloadRequest.Create(ur.DriverUnload);
      ertFastIo: dr := TFastIoRequest.Create(ur.FastIo);
      ertStartIo: dr := TStartIoRequest.Create(ur.StartIo);
      ertDriverDetected : begin
        dr := TDriverDetectedRequest.Create(ur.DriverDetected);
        If FDriverMap.ContainsKey(dr.DriverObject) Then
          FDriverMap.Remove(dr.DriverObject);

        FDriverMap.Add(dr.DriverObject, dr.DriverName);
        end;
      ertDeviceDetected : begin
        dr := TDeviceDetectedRequest.Create(ur.DeviceDetected);
        If FDeviceMap.ContainsKey(dr.DeviceObject) Then
          FDeviceMap.Remove(dr.DeviceObject);

        FDeviceMap.Add(dr.DeviceObject, dr.DeviceName);
        end;
      ertFileObjectNameAssigned : begin
        dr := TFileObjectNameAssignedRequest.Create(ur.FileObjectNameAssigned);
        If FFileMap.ContainsKey(dr.FileObject) Then
          FFileMap.Remove(dr.FileObject);

        FFileMap.Add(dr.FileObject, dr.FileName);
        end;
      ertFileObjectNameDeleted : begin
        dr := TFileObjectNameDeletedRequest.Create(ur.FileObjectNameDeleted);
        If FFileMap.ContainsKey(dr.FileObject) Then
          FFileMap.Remove(dr.FileObject);
        end
      Else dr := TDriverRequest.Create(ur.Header);
      end;

    If FDriverMap.TryGetValue(dr.DriverObject, driverName) Then
      dr.DriverName := driverName;

    If FDeviceMap.TryGetValue(dr.DeviceObject, deviceName) Then
      dr.DeviceName := deviceName;

    If FFileMap.TryGetValue(dr.FileObject, fileName) Then
      dr.SetFileName(fileName);

    FRequests.Add(dr);
    end;

  UpdateRequest := Nil;
  end;

Inherited Update;
end;

Function TRequestListModel.RefreshMaps:Cardinal;
Var
  I, J : Integer;
  count : Cardinal;
  pdri : PPIRPMON_DRIVER_INFO;
  dri : PIRPMON_DRIVER_INFO;
  tmp : PPIRPMON_DRIVER_INFO;
  pdei : PPIRPMON_DEVICE_INFO;
  dei : PIRPMON_DEVICE_INFO;
begin
Result := IRPMonDllSnapshotRetrieve(pdri, count);
If Result = ERROR_SUCCESS Then
  begin
  FDriverMap.Clear;
  FDeviceMap.Clear;
  tmp := pdri;
  For I := 0 To count - 1 Do
    begin
    dri := tmp^;
    FDriverMap.Add(dri.DriverObject, Copy(WideString(dri.DriverName), 1, Length(WideString(dri.DriverName))));
    pdei := dri.Devices;
    For J := 0 To dri.DeviceCount - 1 Do
      begin
      dei := pdei^;
      FDeviceMap.Add(dei.DeviceObject, Copy(WideString(dei.Name), 1, Length(WideString(dei.Name))));
      Inc(pdei);
      end;

    Inc(tmp);
    end;

  IRPMonDllSnapshotFree(pdri, count);
  end;
end;

Procedure TRequestListModel.Clear;
Var
  dr : TDriverRequest;
begin
Inherited Clear;
For dr In FRequests Do
  dr.Free;

FRequests.Clear;
end;


Constructor TRequestListModel.Create;
begin
Inherited Create(Nil);
UpdateRequest := Nil;
FRequests := TList<TDriverRequest>.Create;
FDriverMap := TDictionary<Pointer, WideString>.Create;
FDeviceMap := TDictionary<Pointer, WideString>.Create;
FFileMap := TDictionary<Pointer, WideString>.Create;
RefreshMaps;
end;

Destructor TRequestListModel.Destroy;
begin
FFileMap.Free;
FDriverMap.Free;
FDeviceMap.Free;
Clear;
FRequests.Free;
Inherited Destroy;
end;

Procedure TRequestListModel.SaveToStream(AStream:TStream);
Var
  I : Integer;
  dr : TDriverRequest;
begin
For I := 0 To RowCount - 1 Do
  begin
  dr := _Item(I);
  dr.SaveToStream(AStream, FParsers);
  end;
end;

Procedure TRequestListModel.SaveToFile(AFileName:WideString);
Var
  F : TFileStream;
begin
F := TFileStream.Create(AFileName, fmCreate Or fmOpenWrite);
Try
  SaveToStream(F);
Finally
  F.Free;
  end;
end;

Procedure TRequestListModel.Sort;
Var
  c : TDriverRequestComparer;
begin
c := TDriverRequestComparer.Create;
FRequests.Sort(c);
c.Free;
If Assigned(FDisplayer) Then
  FDisplayer.Invalidate;
end;


End.

