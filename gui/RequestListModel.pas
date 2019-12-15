Unit RequestListModel;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Classes, Generics.Collections, Generics.Defaults,
  IRPMonDll, ListModel, IRPMonRequest, DataParsers, ComCtrls,
  Graphics;


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
    rlmctProcessName,
    rlmctIRQL,
    rlmctPreviousMode,
    rlmctRequestorMode,
    rlmctIOSBStatusValue,
    rlmctIOSBStatusConstant,
    rlmctIOSBInformation,
    rlmctRequestorPID,
    rlmctEmulated,
    rlmctDataAssociated,
    rlmctDataStripped,
    rlmctDataSize,
    rlmctAdmin,
    rlmctImpersonated,
    rlmctImpersonatedAdmin);
  PERequestListModelColumnType = ^ERequestListModelColumnType;

  RequestListModelColumnSet = Set Of ERequestListModelColumnType;

Const
  RequestListModelColumnNames : Array [0..Ord(rlmctImpersonatedAdmin)] Of String = (
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
    'Process name',
    'IRQL',
    'Previous mode',
    'Requestor mode',
    'IOSB.Status value',
    'IOSB.Status constant',
    'IOSB.Information',
    'Requestor PID',
    'Emulated',
    'Associated data',
    'Data stripped',
    'Data size',
    'Admin',
    'Impersonated',
    'ImpAdmin'
  );

  RequestListModelColumnValueTypes : Array [0..Ord(rlmctImpersonatedAdmin)] Of ERequestListModelColumnValueType = (
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
    rlmcvtString,
    rlmcvtIRQL,
    rlmcvtProcessorMode,
    rlmcvtProcessorMode,
    rlmcvtInteger,
    rlmcvtString,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger,
    rlmcvtInteger
  );

Type
  TDriverRequest = Class (TGeneralRequest)
  Private
    FHighlight : Boolean;
    FHighlightColor : Cardinal;
    Procedure ProcessParsers(AParsers:TObjectList<TDataParser>; ALines:TStrings);
  Public
    Class Function GetBaseColumnName(AColumnType:ERequestListModelColumnType):WideString;
    Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Virtual;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Virtual;
    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Virtual;
    Procedure SaveToStream(AStream:TStream; AParsers:TObjectList<TDataParser>; ABinary:Boolean = False); Virtual;
    Procedure SaveToFile(AFileName:WideString; AParsers:TObjectList<TDataParser>; ABinary:Boolean = False); Virtual;

    Class Function CreatePrototype(AType:ERequestType):TDriverRequest;

    Property Highlight : Boolean Read FHighlight Write FHighlight;
    Property HighlightColor : Cardinal Read FHighlightColor Write FHighlightColor;
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
    Constructor Create(Var ARequest:REQUEST_UNLOAD); Overload;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
  end;

  TAddDeviceRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_ADDDEVICE); Overload;
  end;


  TIRPCompleteRequest = Class (TDriverRequest)
  Private
    FIRPAddress : Pointer;
    FIOSBStatus : Cardinal;
    FIOSBInformation : NativeUInt;
    FMajorFunction : Cardinal;
    FMinorFunction : Cardinal;
  Public
    Constructor Create(Var ARequest:REQUEST_IRP_COMPLETION); Overload;

    Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Override;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;

    Property Address : Pointer Read FIRPAddress;
    Property IOSBStatus : Cardinal Read FIOSBStatus;
    Property IOSBInformation : NativeUInt Read FIOSBInformation;
    Property MajorFunction : Cardinal Read FMajorFunction;
    Property MinorFunction : Cardinal Read FMinorFunction;
  end;

  TStartIoRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_STARTIO); Overload;
  end;

  TRequestListModelOnRequestProcessed = Procedure (ARequest:TDriverRequest; Var AStore:Boolean) Of Object;

  TRequestListModel = Class (TListModel<TDriverRequest>)
    Private
      FFilterDisplayOnly : Boolean;
      FAllRequests : TList<TDriverRequest>;
      FRequests : TList<TDriverRequest>;
      FDriverMap : TDictionary<Pointer, WideString>;
      FDeviceMap : TDictionary<Pointer, WideString>;
      FFileMap : TDictionary<Pointer, WideString>;
      FProcessMap : TDictionary<Cardinal, WideString>;
      FParsers : TObjectList<TDataParser>;
      FOnRequestProcessed : TRequestListModelOnRequestProcessed;
    Protected
      Procedure OnAdvancedCustomDrawItemCallback(Sender: TCustomListView; Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage; var DefaultDraw: Boolean); Override;
      Function GetColumn(AItem:TDriverRequest; ATag:NativeUInt):WideString; Override;
      Procedure FreeItem(AItem:TDriverRequest); Override;
      Function _Item(AIndex:Integer):TDriverRequest; Override;
      Procedure SetFilterDisplayOnly(AValue:Boolean);
    Public
      UpdateRequest : TList<PREQUEST_GENERAL>;
      Constructor Create; Reintroduce;
      Destructor Destroy; Override;
      Function RefreshMaps:Cardinal;
      Procedure Sort;

      Procedure Clear; Override;
      Function RowCount : Cardinal; Override;
      Function Update:Cardinal; Override;
      Procedure SaveToStream(AStream:TStream; ABinary:Boolean = False);
      Procedure SaveToFile(AFileName:WideString; ABinary:Boolean = False);
      Procedure LoadFromStream(AStream:TStream);
      Procedure LoadFromFile(AFileName:WideString);
      Procedure Reevaluate;

      Property FilterDisplayOnly : Boolean Read FFilterDisplayOnly Write SetFilterDisplayOnly;
      Property Parsers : TObjectList<TDataParser> Read FParsers Write FParsers;
      Property OnRequestProcessed : TRequestListModelOnRequestProcessed Read FOnRequestProcessed Write FOnRequestProcessed;
    end;

Implementation

Uses
  SysUtils, NameTables, IRPRequest, FastIoRequest,
  XXXDetectedRequests, FileObjectNameXXXRequest,
  ProcessXXXRequests, Utils;

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

Class Function TDriverRequest.CreatePrototype(AType:ERequestType):TDriverRequest;
begin
Case AType Of
  ertIRP: Result := TIRPRequest.Create;
  ertIRPCompletion: Result := TIRPCompleteRequest.Create;
  ertAddDevice: Result := TAddDeviceRequest.Create;
  ertDriverUnload: Result := TDriverUnloadRequest.Create;
  ertFastIo: Result := TFastIoRequest.Create;
  ertStartIo: Result := TStartIoRequest.Create;
  ertDriverDetected : Result := TDriverDetectedRequest.Create;
  ertDeviceDetected : Result := TDeviceDetectedRequest.Create;
  ertFileObjectNameAssigned : Result := TFileObjectNameAssignedRequest.Create;
  ertFileObjectNameDeleted : Result := TFileObjectNameDeletedRequest.Create;
  ertProcessCreated : Result := TProcessCreatedRequest.Create;
  ertProcessExitted : Result := TProcessExittedRequest.Create;
  Else Result := TDriverRequest.Create;
  end;
end;

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


Procedure TDriverRequest.SaveToStream(AStream: TStream; AParsers:TObjectList<TDataParser>; ABinary:Boolean = False);
Var
  reqSize : Cardinal;
  s : TStringList;
  value : WideString;
  ct : ERequestListModelColumnType;
begin
If Not ABinary Then
  begin
  s := TStringList.Create;
  For ct := Low(ERequestListModelColumnType) To High(ERequestListModelColumnType) Do
    begin
    If GetColumnValue(ct, value) Then
      begin
      If value <> '' Then
        s.Add(Format('%s = %s', [GetColumnName(ct), value]));
      end;
    end;

  If DataSize > 0 Then
    ProcessParsers(AParsers, s);

  s.Add('');
  s.SaveToStream(AStream);
  s.Free;
  end
Else begin
  reqSize := IRPMonDllGetRequestSize(FRaw);
  AStream.Write(reqSize, SizeOf(reqSize));
  AStream.Write(FRaw^, reqSize);
  end;
end;

Procedure TDriverRequest.SaveToFile(AFileName: WideString; AParsers:TObjectList<TDataParser>; ABinary:Boolean = False);
Var
  F : TFileStream;
begin
F := TFileStream.Create(AFileName, fmCreate Or fmOpenWrite);
Try
  SaveToStream(F, AParsers, ABinary);
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
  rlmctProcessName : begin
    AValue := PWideChar(FProcessName);
    AValueSize := 0;
    end;
  rlmctIRQL: begin
    AValue := @FIrql;
    AValueSIze := SizeOf(FIrql);
    end;
  rlmctDataAssociated : begin
    AValue := @FDataPresent;
    AValueSize := SizeOf(FDataPresent);
    end;
  rlmctDataStripped : begin
    AValue := @FDataStripped;
    AValueSize := SizeOf(FDataStripped);
    end;
  rlmctEmulated : begin
    AValue := @FEmulated;
    AValueSize := SizeOf(FEmulated);
    end;
  rlmctDataSize : begin
    AValue := @FDataSize;
    AValueSize := SizeOf(FDataSize);
    end;
  rlmctAdmin : begin
    AValue := @FAdmin;
    AValueSize := SizeOf(FAdmin);
    end;
  rlmctImpersonated : begin
    AValue := @FImpersonated;
    AValueSize := SizeOf(FImpersonated);
    end;
  rlmctImpersonatedAdmin : begin
    AValue := @FImpersonatedAdmin;
    AValueSize := SizeOf(FImpersonatedAdmin);
    end;
  rlmctFileObject: begin
    AValue := @FileObject;
    AValueSize := SizeOf(FileObject);
    end;
  rlmctFileName: begin
    AValue := PWideChar(FileName);
    AValueSize := 0;
    end
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
  rlmctDeviceObject: begin
    Result := Self.ClassType <> TDriverRequest;
    If Result Then
      AResult := Format('0x%p', [FDeviceObject]);
    end;
  rlmctDeviceName: begin
    Result := Self.ClassType <> TDriverRequest;
    If Result Then
      AResult := FDeviceName;
    end;
  rlmctDriverObject: begin
    Result := Self.ClassType <> TDriverRequest;
    If Result Then
      AResult := Format('0x%p', [FDriverObject]);
    end;
  rlmctDriverName: begin
    Result := Self.ClassType <> TDriverRequest;
    If Result Then
      AResult := FDriverName;
    end;
  rlmctFileObject : begin
    Result := Self.ClassType <> TDriverRequest;
    If Result Then
      AResult := Format('0x%p', [FFileObject]);
    end;
  rlmctFileName: begin
    Result := Self.ClassType <> TDriverRequest;
    If Result Then
      AResult := FFileName;
    end;
  rlmctResultValue: begin
    Result := Self.ClassType <> TDriverRequest;
    If Result Then
      AResult := RequestResultToString(FResultValue, FResultType);
    end;
  rlmctResultConstant: begin
    Result := Self.ClassType <> TDriverRequest;
    If Result Then
      AResult := RequestResultToString(FResultValue, FResultType, True);
    end;
  rlmctProcessId : AResult := Format('%u', [FProcessId]);
  rlmctThreadId :  AResult := Format('%u', [FThreadId]);
  rlmctProcessName : AResult := FProcessName;
  rlmctIRQL : AResult := IRQLToString(FIRQL);
  rlmctEmulated : AResult := BoolToStr(FEmulated, True);
  rlmctDataAssociated : AResult := BoolToStr(FDataPresent, True);
  rlmctDataStripped : AResult := BoolToStr(FDataStripped, True);
  rlmctDataSize : AResult := Format('%d', [FDataSize]);
  rlmctAdmin : AResult := BoolToStr(FAdmin, True);
  rlmctImpersonated : AResult := BoolToStr(FImpersonated, True);
  rlmctImpersonatedAdmin : AResult := BoolToStr(FImpersonatedAdmin, True);
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
FMajorFunction := ARequest.MajorFunction;
FMinorFunction := ARequest.MinorFunction;
SetFileObject(ARequest.FileObject);
end;

Function TIRPCompleteRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Result := '';
Case AColumnType Of
  rlmctSubType : Result := 'Major function';
  rlmctMinorFunction : Result := 'Minor function';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

Function TIRPCompleteRequest.GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctSubType : begin
    AValue := @FMajorFunction;
    AValueSize := SizeOf(FMajorFunction);
    end;
  rlmctMinorFunction : begin
    AValue := @FMinorFunction;
    AValueSize := SizeOf(FMinorFunction);
    end;
  Else Result := Inherited GetColumnValueRaw(AColumnType, AValue, AValueSize);
  end;
end;

Function TIRPCompleteRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctSubType : AResult := MajorFunctionToString(FMajorFunction);
  rlmctMinorFunction : AResult := MinorFunctionToString(FMajorFunction, FMinorFunction);
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
d := PByte(@ARequest) + SizeOf(aRequest);
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

Procedure TRequestListModel.SetFilterDisplayOnly(AValue:Boolean);
Var
  dr : TDriverRequest;
begin
If FFilterDisplayOnly <> AValue Then
  begin
  FFilterDisplayOnly := AValue;
  FAllRequests.Clear;
  If FFilterDisplayOnly Then
    begin
    For dr In FRequests Do
      FAllRequests.Add(dr);
    end;
  end;
end;

Function TRequestListModel.RowCount : Cardinal;
begin
Result := FRequests.Count;
end;

Function TRequestListModel.Update:Cardinal;
Var
  keepRequest : Boolean;
  ur : PREQUEST_GENERAL;
  dr : TDriverRequest;
  deviceName : WideString;
  driverName : WideString;
  fileName : WideString;
  processName : WideString;
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
        end;
      ertProcessCreated : begin
        dr := TProcessCreatedRequest.Create(ur.ProcessCreated);
        If FProcessMap.ContainsKey(Cardinal(dr.DriverObject)) Then
          FProcessMap.Remove(Cardinal(dr.DriverObject));

        FProcessMap.Add(Cardinal(dr.DriverObject), dr.DriverName);
        end;
      ertProcessExitted : begin
        dr := TProcessExittedRequest.Create(ur.ProcessExitted);
        end;
      Else dr := TDriverRequest.Create(ur.Header);
      end;

    If FDriverMap.TryGetValue(dr.DriverObject, driverName) Then
      dr.DriverName := driverName;

    If FDeviceMap.TryGetValue(dr.DeviceObject, deviceName) Then
      dr.DeviceName := deviceName;

    If FFileMap.TryGetValue(dr.FileObject, fileName) Then
      dr.SetFileName(fileName);

    If FProcessMap.TryGetValue(dr.ProcessId, processName) Then
      dr.SetProcessName(processName);

    keepRequest := True;
    If Assigned(FOnRequestProcessed) Then
      FOnRequestProcessed(dr, keepRequest);

    If FFilterDisplayOnly Then
      FAllRequests.Add(dr);

    If keepRequest Then
      FRequests.Add(dr)
    Else If Not FFilterDisplayOnly Then
      dr.Free;
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
FAllRequests.Clear;
end;


Constructor TRequestListModel.Create;
begin
Inherited Create(Nil);
UpdateRequest := Nil;
FRequests := TList<TDriverRequest>.Create;
FAllRequests := TList<TDriverRequest>.Create;
FDriverMap := TDictionary<Pointer, WideString>.Create;
FDeviceMap := TDictionary<Pointer, WideString>.Create;
FFileMap := TDictionary<Pointer, WideString>.Create;
FProcessMap := TDictionary<Cardinal, WideString>.Create;
RefreshMaps;
end;

Destructor TRequestListModel.Destroy;
begin
FProcessMap.Free;
FFileMap.Free;
FDriverMap.Free;
FDeviceMap.Free;
Clear;
FAllRequests.Free;
FRequests.Free;
Inherited Destroy;
end;

Procedure TRequestListModel.SaveToStream(AStream:TStream; ABinary:Boolean = False);
Var
  I : Integer;
  dr : TDriverRequest;
begin
For I := 0 To RowCount - 1 Do
  begin
  dr := _Item(I);
  dr.SaveToStream(AStream, FParsers, ABinary);
  end;
end;

Procedure TRequestListModel.SaveToFile(AFileName:WideString; ABinary:Boolean = False);
Var
  F : TFileStream;
begin
F := TFileStream.Create(AFileName, fmCreate Or fmOpenWrite);
Try
  SaveToStream(F, ABinary);
Finally
  F.Free;
  end;
end;

Procedure TRequestListModel.LoadFromStream(AStream:TStream);
Var
  reqSize : Cardinal;
  rg : PREQUEST_GENERAL;
  l : TList<PREQUEST_GENERAL>;
begin
l := TList<PREQUEST_GENERAL>.Create;
While AStream.Position < AStream.Size Do
  begin
  AStream.Read(reqSize, SizeOf(reqSize));
  rg := AllocMem(reqSize);
  AStream.Read(rg^, reqSize);
  l.Add(rg);
  end;

UpdateRequest := l;
Update;
For rg In l Do
  FreeMem(rg);

l.Free;
end;

Procedure TRequestListModel.LoadFromFile(AFileName:WideString);
Var
  F : TFileStream;
begin
F := TFileStream.Create(AFileName, fmOpenRead);
Try
  LoadFromStream(F);
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
If Assigned(Displayer) Then
  Displayer.Invalidate;
end;

Procedure TRequestListModel.OnAdvancedCustomDrawItemCallback(Sender: TCustomListView; Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage; var DefaultDraw: Boolean);
Var
  dr : TDriverRequest;
begin
dr := FRequests[Item.Index];
With Sender.Canvas Do
  begin
  If Item.Selected Then
    begin
    Brush.Color := clHighLight;
    Font.Color := clHighLightText;
    Font.Style := [fsBold];
    end
  Else If dr.Highlight Then
    begin
    Brush.Color := dr.HighlightColor;
    If Utils.ColorLuminanceHeur(dr.HighlightColor) >= 1490 Then
       Font.Color := ClBlack
    Else Font.Color := ClWhite;
    end;
  end;

DefaultDraw := True;
end;

Procedure TRequestListModel.Reevaluate;
Var
  store : Boolean;
  I : Integer;
  dr : TDriverRequest;
begin
If Not FFilterDisplayOnly Then
  begin
  I := 0;
  While (I < FRequests.Count) Do
    begin
    If Assigned(FOnRequestProcessed) Then
      begin
      store := True;
      FOnRequestProcessed(FRequests[I], store);
      If Not store THen
        begin
        FRequests[I].Free;
        FRequests.Delete(I);
        Continue;
        end;
      end;

    Inc(I);
    end;
  end
Else begin
  FRequests.Clear;
  For dr In FAllRequests Do
    begin
    store := True;
    If Assigned(FOnRequestProcessed) Then
      FOnRequestProcessed(dr, store);

    If store Then
      FRequests.Add(dr);
    end;
  end;

If Assigned(Displayer) Then
  begin
  Displayer.Items.Count := FRequests.Count;
  Displayer.Invalidate;
  end;
end;


End.

