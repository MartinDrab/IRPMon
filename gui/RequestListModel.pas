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
      Function GetTotalCount:Cardinal;
    Public
      UpdateRequest : TList<PREQUEST_GENERAL>;
      Constructor Create; Reintroduce;
      Destructor Destroy; Override;
      Function RefreshMaps:Cardinal;

      Procedure Clear; Override;
      Function RowCount : Cardinal; Override;
      Function Update:Cardinal; Override;
      Procedure SaveToStream(AStream:TStream; AFormat:ERequestLogFormat; ACompress:Boolean = False);
      Procedure SaveToFile(AFileName:WideString; AFormat:ERequestLogFormat; ACompress:Boolean = False);
      Procedure LoadFromStream(AStream:TStream; ARequireHeader:Boolean = True);
      Procedure LoadFromFile(AFileName:WideString; ARequireHeader:Boolean = True);
      Procedure Reevaluate;

      Property FilterDisplayOnly : Boolean Read FFilterDisplayOnly Write SetFilterDisplayOnly;
      Property Parsers : TObjectList<TDataParser> Read FParsers Write FParsers;
      Property OnRequestProcessed : TRequestListModelOnRequestProcessed Read FOnRequestProcessed Write FOnRequestProcessed;
      Property TotalCount : Cardinal Read GetTotalCount;
    end;

Implementation

Uses
  SysUtils, NameTables, IRPRequest, FastIoRequest,
  XXXDetectedRequests, FileObjectNameXXXRequest,
  ProcessXXXRequests, Utils, BinaryLogHeader, ImageLoadRequest,
  DriverUnloadRequest, AddDeviceRequest, IRPCompleteRequest,
  StartIoRequest;


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
  requestBuffer : PREQUEST_GENERAL;
  tmpUR : PREQUEST_GENERAL;
  dr : TDriverRequest;
  deviceName : WideString;
  driverName : WideString;
  fileName : WideString;
  processName : WideString;
begin
Result := ERROR_SUCCESS;
If Assigned(UpdateRequest) Then
  begin
  For requestBuffer In UpdateRequest Do
    begin
    tmpUR := requestBuffer;
    While Assigned(tmpUR) Do
      begin
      Case tmpUR.Header.RequestType Of
        ertIRP: dr := TIRPRequest.Build(tmpUR.Irp);
        ertIRPCompletion: dr := TIRPCompleteRequest.Create(tmpUR.IrpComplete);
        ertAddDevice: dr := TAddDeviceRequest.Create(tmpUR.AddDevice);
        ertDriverUnload: dr := TDriverUnloadRequest.Create(tmpUR.DriverUnload);
        ertFastIo: dr := TFastIoRequest.Create(tmpUR.FastIo);
        ertStartIo: dr := TStartIoRequest.Create(tmpUR.StartIo);
        ertDriverDetected : begin
          dr := TDriverDetectedRequest.Create(tmpUR.DriverDetected);
          If FDriverMap.ContainsKey(dr.DriverObject) Then
            FDriverMap.Remove(dr.DriverObject);

          FDriverMap.Add(dr.DriverObject, dr.DriverName);
          end;
        ertDeviceDetected : begin
          dr := TDeviceDetectedRequest.Create(tmpUR.DeviceDetected);
          If FDeviceMap.ContainsKey(dr.DeviceObject) Then
            FDeviceMap.Remove(dr.DeviceObject);

          FDeviceMap.Add(dr.DeviceObject, dr.DeviceName);
          end;
        ertFileObjectNameAssigned : begin
          dr := TFileObjectNameAssignedRequest.Create(tmpUR.FileObjectNameAssigned);
          If FFileMap.ContainsKey(dr.FileObject) Then
            FFileMap.Remove(dr.FileObject);

          FFileMap.Add(dr.FileObject, dr.FileName);
          end;
        ertFileObjectNameDeleted : begin
          dr := TFileObjectNameDeletedRequest.Create(tmpUR.FileObjectNameDeleted);
          If FFileMap.ContainsKey(dr.FileObject) Then
            begin
            dr.SetFileName(FFileMap.Items[dr.FileObject]);
            FFileMap.Remove(dr.FileObject);
            end;
          end;
        ertProcessCreated : begin
          dr := TProcessCreatedRequest.Create(tmpUR.ProcessCreated);
          If FProcessMap.ContainsKey(Cardinal(dr.DriverObject)) Then
            FProcessMap.Remove(Cardinal(dr.DriverObject));

          FProcessMap.Add(Cardinal(dr.DriverObject), dr.DriverName);
          end;
        ertProcessExitted : dr := TProcessExittedRequest.Create(tmpUR.ProcessExitted);
        ertImageLoad : begin
          dr := TImageLoadRequest.Create(tmpUR.ImageLoad);
          If FFileMap.ContainsKey(dr.FileObject) Then
            FFileMap.Remove(dr.FileObject);

          FFileMap.Add(dr.FileObject, dr.FileName);
          end;
        Else dr := TDriverRequest.Create(tmpUR.Header);
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

      If Not Assigned(tmpUR.Header.Next) Then
        Break;

      tmpUR := PREQUEST_GENERAL(NativeUInt(tmpUR) + RequestGetSize(@tmpUR.Header));
      end;
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

Procedure TRequestListModel.SaveToStream(AStream:TStream; AFormat:ERequestLogFormat; ACompress:Boolean = False);
Var
  bh : TBinaryLogHeader;
  I : Integer;
  dr : TDriverRequest;
  comma : AnsiChar;
  arrayChar : AnsiChar;
  newLine : Packed Array [0..1] Of AnsiChar;
begin
comma := ',';
newLine[0] := #13;
newLine[1] := #10;
Case AFormat Of
  rlfBinary: begin
    TBinaryLogHeader.Fill(bh);
    AStream.Write(bh, SizeOf(bh));
    end;
  rlfJSONArray : begin
    arrayChar := '[';
    AStream.Write(arrayChar, SizeOf(arrayChar));
    end;
  end;

For I := 0 To RowCount - 1 Do
  begin
  dr := _Item(I);
  dr.SaveToStream(AStream, FParsers, AFormat, ACompress);
  If I < RowCount - 1 Then
    begin
    Case AFormat Of
      rlfJSONArray : AStream.Write(comma, SizeOf(comma));
      rlfText,
      rlfJSONLines : AStream.Write(newLine, SizeOf(newLine));
      end;
    end;
  end;

If AFormat = rlfJSONArray Then
  begin
  arrayChar := ']';
  AStream.Write(arrayChar, SizeOf(arrayChar));
  end;
end;

Procedure TRequestListModel.SaveToFile(AFileName:WideString; AFormat:ERequestLogFormat; ACompress:Boolean = False);
Var
  F : TFileStream;
begin
F := TFileStream.Create(AFileName, fmCreate Or fmOpenWrite);
Try
  SaveToStream(F, AFormat, ACompress);
Finally
  F.Free;
  end;
end;

Procedure TRequestListModel.LoadFromStream(AStream:TStream; ARequireHeader:Boolean = True);
Var
  reqSize : Cardinal;
  rg : PREQUEST_GENERAL;
  l : TList<PREQUEST_GENERAL>;
  bh : TBinaryLogHeader;
  oldPos : Int64;
  invalidHeader : Boolean;
begin
invalidHeader := False;
oldPos := AStream.Position;
AStream.Read(bh, SizeOf(bh));
If Not TBinaryLogHeader.SignatureValid(bh) Then
  begin
  invalidHeader := True;
  If ARequireHeader Then
    Raise Exception.Create('Invalid log file signature');
  end;

If Not TBinaryLogHeader.VersionSupported(bh) Then
  begin
  invalidHeader := True;
  If ARequireHeader Then
    Raise Exception.Create('Log file version not supported');
  end;

If Not TBinaryLogHeader.ArchitectureSupported(bh) Then
  begin
  invalidHeader := True;
  If ARequireHeader Then
    Raise Exception.Create('The log file and application "bitness"  differ.'#13#10'Use other application version');
  end;

If invalidHeader Then
  AStream.Position := oldPos;

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

Procedure TRequestListModel.LoadFromFile(AFileName:WideString; ARequireHeader:Boolean = True);
Var
  F : TFileStream;
begin
F := TFileStream.Create(AFileName, fmOpenRead);
Try
  LoadFromStream(F, ARequireHeader);
Finally
  F.Free;
  end;
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

Function TRequestListModel.GetTotalCount:Cardinal;
begin
Result := FRequests.Count;
If FFilterDisplayOnly Then
  Result := FAllRequests.Count;
end;



End.

