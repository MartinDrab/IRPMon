unit IRPMonRequest;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

interface

Uses
  Windows, Classes, Generics.Collections,
  IRPMonDll,
  DataParsers,
  AbstractRequest,
  SymTables;


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
    Procedure ProcessParsers(AParsers:TObjectList<TDataParser>; AFormat:ERequestLogFormat; ALines:TStrings);
    Procedure ProcessStack(ASymStore:TModuleSymbolStore; AFormat:ERequestLogFormat; ALines:TStrings);
  Public
    Class Function GetBaseColumnName(AColumnType:ERequestListModelColumnType):WideString;
    Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Virtual;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Virtual;
    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Virtual;
    Procedure SaveToStream(AStream:TStream; AParsers:TObjectList<TDataParser>; AFormat:ERequestLogFormat; ASymStore:TModuleSymbolStore); Virtual;
    Procedure SaveToFile(AFileName:WideString; AParsers:TObjectList<TDataParser>; AFormat:ERequestLogFormat; ASymStore:TModuleSymbolStore); Virtual;

    Class Function CreatePrototype(AType:ERequestType):TDriverRequest;

    Property Highlight : Boolean Read FHighlight Write FHighlight;
    Property HighlightColor : Cardinal Read FHighlightColor Write FHighlightColor;
  end;

implementation

Uses
  SysUtils,
  Utils,
  IRPRequest,
  FastIoRequest,
  XXXDetectedRequests,
  FileObjectNameXXXRequest,
  ProcessXXXRequests,
  ImageLoadRequest,
  DriverUnloadRequest,
  AddDeviceRequest,
  IRPCompleteRequest,
  StartIoRequest;


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
  ertImageLoad : Result := TImageLoadRequest.Create;
  Else Result := TDriverRequest.Create;
  end;
end;

Procedure TDriverRequest.ProcessParsers(AParsers:TObjectList<TDataParser>; AFormat:ERequestLogFormat; ALines:TStrings);
Var
  I : Integer;
  err : Cardinal;
  _handled : ByteBool;
  names : TStringList;
  values : TStringList;
  pd : TDataParser;
  tmp : WideString;
  jsonString : WideString;
  firstParsed : Boolean;
begin
If Assigned(AParsers) Then
  begin
  firstParsed := False;
  jsonString := '';
  names := TStringList.Create;
  values := TStringList.Create;
  For pd In AParsers Do
    begin
    err := pd.Parse(AFormat, Self, _handled, names, values);
    If (err = ERROR_SUCCESS) And (_handled) And (values.Count > 0) Then
      begin
      If (firstParsed) And
        ((AFormat = rlfJSONArray) Or (AFormat = rlfJSONLines)) Then
        jsonString := jsonString + ', ';

      Case AFormat Of
        rlfText : ALines.Add(Format('Data (%s)', [pd.Name]));
        rlfJSONArray,
        rlfJSONLines : jsonString := jsonString + Format('"%s" : {', [pd.Name]);
        end;

      For I := 0 To values.Count - 1 Do
        begin
        Case AFormat Of
          rlfText : begin
            If names.Count > 0 Then
              ALines.Add(Format('  %s: %s', [names[I], values[I]]))
            Else ALines.Add('  ' + values[I]);
            end;
          rlfJSONArray,
          rlfJSONLines : begin
            If names.Count > 0 Then
              tmp := Format('"%s" : "%s"', [names[I], StringEscape(values[I])])
            Else tmp := Format('"Data%u" : "%s"', [I, StringEscape(values[I])]);

            jsonString := jsonString + tmp;
            If I < values.Count - 1 Then
              jsonString := jsonString + ', ';

            end;
          end;
        end;

      values.Clear;
      names.Clear;
      firstParsed := True;
      jsonString := jsonString + '}';
      end;
    end;

  values.Free;
  names.Free;
  If jsonString <> '' Then
    ALines.Add(jsonString);
  end;
end;


Procedure TDriverRequest.ProcessStack(ASymStore:TModuleSymbolStore; AFormat:ERequestLogFormat; ALines:TStrings);
Var
  I : Integer;
  err : Cardinal;
  tmp : WideString;
  paddress : PPointer;
  jsonString : WideString;
  moduleName : WideString;
  functionName : WideString;
  offset : NativeUInt;
begin
jsonString := '';
If FStackFrameCount > 0 Then
  begin
  paddress := FStackFrames;
  For I := 0 To FStackFrameCount - 1 Do
    begin
    moduleName := '';
    functionName := '';
    offset := 0;
    If (Assigned(ASymStore)) And
      (Assigned(FProcess)) And
      (ASymStore.TranslateAddress(FProcess, paddress^, moduleName, functionName, offset)) Then
      begin
      Case AFormat Of
        rlfText : begin
          If functionName <> '' Then
            ALines.Add(Format('  %u: 0x%p %s!%s+%u', [I, paddress^, moduleName, functionName, offset]))
          Else ALines.Add(Format('  %u: 0x%p %s+%u', [I, paddress^, moduleName, offset]));
          end;
        rlfJSONArray,
        rlfJSONLines : begin
          If I <> 0 Then
            jsonString := jsonString + ', ';

          jsonString := jsonString + '{';
          jsonString := jsonString + Format('"Address" : 0x%p, ', [paddress^]);
          jsonString := jsonString + Format('"Module" : "%s", ', [moduleName]);
          If functionName <> '' Then
            jsonString := jsonString + Format('"Function" : "%s", ', [functionName]);

          jsonString := jsonString + Format('"Offset" : %u', [offset]);
          jsonString := jsonString + ')';
          end;
        end;
      end
    Else begin
      Case AFormat Of
        rlfText : ALines.Add(Format('  %u: 0x%p', [I, paddress^]));
        rlfJSONArray,
        rlfJSONLines : begin
          If I <> 0 Then
            jsonString := jsonString + ', ';

          jsonString := jsonString + '{';
          jsonString := jsonString + Format('"Address" : 0x%p, ', [paddress^]);
          jsonString := jsonString + '), ';
          end;
        end;
      end;

    Inc(paddress);
    end;
  end;

If jsonString <> '' Then
  ALines.Add(jsonString);
end;


Procedure TDriverRequest.SaveToStream(AStream: TStream; AParsers:TObjectList<TDataParser>; AFormat:ERequestLogFormat; ASymStore:TModuleSymbolStore);
Var
  tmpJson : WideString;
  jsonString : WideString;
  tmp : PREQUEST_HEADER;
  reqSize : Cardinal;
  s : TStringList;
  value : WideString;
  ct : ERequestListModelColumnType;
  rbs : RawByteString;
begin
Case AFormat Of
  rlfText : begin
    s := TStringList.Create;
    For ct := Low(ERequestListModelColumnType) To High(ERequestListModelColumnType) Do
      begin
      If GetColumnValue(ct, value) Then
        begin
        If value <> '' Then
          s.Add(Format('%s = %s', [GetColumnName(ct), value]));
        end;
      end;

    If FDataSize > 0 Then
      ProcessParsers(AParsers, AFormat, s);

    If FStackFrameCount > 0 Then
      begin
      s.Add('Stack:');
      ProcessStack(ASymStore, AFormat, s);
      end;

    s.SaveToStream(AStream);
    s.Free;
    end;
  rlfBinary : begin
    tmp := FRaw;
    Try
      reqSize := RequestGetSize(tmp);
      AStream.Write(reqSize, SizeOf(reqSize));
      AStream.Write(tmp^, reqSize);
    Finally
      end;
    end;
  rlfJSONArray,
  rlfJSONLines : begin
    s := TStringList.Create;
    jsonString := '{';
    For ct := Low(ERequestListModelColumnType) To High(ERequestListModelColumnType) Do
      begin
      If GetColumnValue(ct, value) Then
        begin
        If value <> '' Then
          begin
          jsonString := jsonString + Format('"%s" : "%s"', [GetColumnName(ct), StringEscape(value)]);
          If ct < High(ERequestListModelColumnType) Then
            jsonString := jsonString + ', ';
          end;
        end;
      end;

    If DataSize > 0 Then
      begin
      ProcessParsers(AParsers, AFormat, s);
      If s.Count > 0 Then
        begin
        jsonString := jsonString + ', "Parsers" : {';
        jsonString := jsonString + s[0] + '}';
        s.Clear;
        end;
      end;

    If FStackFrameCount > 0 Then
      begin
      ProcessStack(ASymStore, AFormat, s);
      If s.Count > 0 Then
        begin
        jsonString := jsonString + ', "Stack" : [';
        jsonString := jsonString + s[0] + ']';
        end;
      end;

    jsonString := jsonString + '}';
    rbs := AnsiToUtf8(jsonString);
    AStream.Write(PAnsiChar(rbs)^, Length(rbs));
    s.Free;
    end;
  end;
end;

Procedure TDriverRequest.SaveToFile(AFileName: WideString; AParsers:TObjectList<TDataParser>; AFormat:ERequestLogFormat; ASymStore:TModuleSymbolStore);
Var
  F : TFileStream;
begin
F := TFileStream.Create(AFileName, fmCreate Or fmOpenWrite);
Try
  SaveToStream(F, AParsers, AFormat, ASymStore);
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
    end;
  rlmctIOSBStatusValue : begin
    AValue := @FIOSBStatus;
    AValueSize := SizeOf(FIOSBStatus);
    end;
  rlmctIOSBInformation : begin
    AValue := @FIOSBInformation;
    AValueSize := SizeOf(FIOSBInformation);
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
  rlmctProcessName : AResult := FProcessName;
  rlmctIRQL : AResult := IRQLToString(FIRQL);
  rlmctIOSBStatusValue : AResult := Format('0x%x', [FIOSBStatus]);
  rlmctIOSBStatusConstant : AResult := Format('%s', [NTSTATUSToString(FIOSBStatus)]);
  rlmctIOSBInformation : AResult := Format('0x%p', [Pointer(IOSBInformation)]);
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


end.

