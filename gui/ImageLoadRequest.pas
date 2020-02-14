Unit ImageLoadRequest;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows,
  RequestListModel, IRPMonDll;

Type
  TImageLoadRequest = Class (TDriverRequest)
   Private
    FImageBase : Pointer;
    FImageSize : NativeUInt;
    FExtraInfo : Boolean;
    FKernelDriver : Boolean;
    FMappedToAllPids : Boolean;
    FPartialMap : Boolean;
    FSigningLevel : EImageSigningLevel;
    FSignatureType : EImageSignatureType;
   Public
    Constructor Create(Var ARequest:REQUEST_IMAGE_LOAD); Overload;
    Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Override;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;

    Property ImageBase : Pointer Read FImageBase;
    Property ImageSize : NativeUInt Read FImageSize;
    Property KernelDriver : Boolean Read FKernelDriver;
    Property MappedToAllPids : Boolean Read FMappedToAllPids;
    Property ExtraInfo : Boolean Read FExtraInfo;
    Property PartialMap : Boolean Read FPartialMap;
    Property SigningLevel : EImageSigningLevel Read FSigningLevel;
    Property SignatureType : EImageSignatureType Read FSignatureType;
  end;


Implementation

Uses
  SysUtils;

Constructor TImageLoadRequest.Create(Var ARequest:REQUEST_IMAGE_LOAD);
Var
  tmp : WideString;
  rawReq : PREQUEST_IMAGE_LOAD;
begin
Inherited Create(ARequest.Header);
rawReq := PREQUEST_IMAGE_LOAD(FRaw);
AssignData(Pointer(PByte(rawReq) + SizeOf(REQUEST_IMAGE_LOAD)), rawReq.DataSize);
SetFileObject(ARequest.FileObject);
FImageBase := ARequest.ImageBase;
FImageSize := ARequest.ImageSize;
FSigningLevel := ARequest.SignatureLevel;
FSignatureType := ARequest.SignatureType;
FKernelDriver := ARequest.KernelDriver;
FMappedToAllPids := ARequest.MappedToAllPids;
FExtraInfo := ARequest.ExtraInfo;
FPartialMap := ARequest.PartialMap;
SetLength(tmp, FDataSize Div SIzeOf(WideChar));
Move(FData^, PWideChar(tmp)^, FDataSize);
SetFileName(tmp);
end;

Function TImageLoadRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Result := '';
case AColumnType of
  rlmctArg1: Result := 'Image base';
  rlmctArg2: Result := 'Image size';
  rlmctArg3: Result := 'Signature type';
  rlmctArg4: Result := 'Signature level';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

Function TImageLoadRequest.GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctDriverObject,
  rlmctDriverName : Result := False;
  rlmctArg1 : begin
    AValue := @FImageBase;
    AValueSize := SizeOf(FImageBase);
    end;
  rlmctArg2 : begin
    AValue := @FImageSize;
    AValueSize := SizeOf(FImageSize);
    end;
  rlmctArg3 : begin
    AValue := @FSignatureType;
    AValueSize := SizeOf(FSignatureType);
    end;
  rlmctArg4 : begin
    AValue := @FSigningLevel;
    AValueSize := SizeOf(FSigningLevel);
    end;
  Else Result := Inherited GetColumnValueRaw(AColumnType, AValue, AValueSize);
  end;
end;

Function TImageLoadRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctDriverObject,
  rlmctDriverName : Result := False;
  rlmctArg1 : AResult := Format('0x%p', [FImageBase]);
  rlmctArg2 : AResult := Format('%u', [FImageSize]);
  rlmctArg3 : AResult := ImageSignatureTypeToString(FSignatureType);
  rlmctArg4 : AResult := ImageSigningLevelToString(SigningLevel);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


End.
