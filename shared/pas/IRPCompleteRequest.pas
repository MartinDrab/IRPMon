Unit IRPCompleteRequest;

{$IFDEF FPC}
{$MODE Delphi}
{$ENDIF}

Interface

Uses
  IRPMonDll,
  AbstractRequest,
  IRPMonRequest;

Type
  TIRPCompleteRequest = Class (TDriverRequest)
  Private
    FIRPAddress : Pointer;
    FMajorFunction : Cardinal;
    FMinorFunction : Cardinal;
    FRequestorProcessId : NativeUInt;
    FPreviousMode : Byte;
    FRequestorMode : Byte;
  Public
    Constructor Create(Var ARequest:REQUEST_IRP_COMPLETION); Overload;

    Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Override;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;

    Property Address : Pointer Read FIRPAddress;
    Property MajorFunction : Cardinal Read FMajorFunction;
    Property MinorFunction : Cardinal Read FMinorFunction;
    Property RequestorProcessId : NativeUInt Read FRequestorProcessId;
    Property PreviousMode : Byte Read FPreviousMode;
    Property RequestorMode : Byte Read FRequestorMode;
  end;


Implementation

Uses
  SysUtils;

Constructor TIRPCompleteRequest.Create(Var ARequest:REQUEST_IRP_COMPLETION);
Var
  d : Pointer;
begin
Inherited Create(ARequest.Header);
d := PByte(@ARequest) + SizeOf(ARequest);
AssignData(d, ARequest.DataSize);
FIRPAddress := ARequest.IRPAddress;
FMajorFunction := ARequest.MajorFunction;
FMinorFunction := ARequest.MinorFunction;
FPreviousMode := ARequest.PreviousMode;
FRequestorMode := ARequest.RequestorMode;
FRequestorProcessId := ARequest.RequestorProcessId;
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
  rlmctPreviousMode: begin
    AValue := @FPreviousMode;
    AValueSize := SizeOf(FPreviousMode);
    end;
  rlmctRequestorMode: begin
    AValue := @FRequestorMode;
    AValueSize := SizeOf(FRequestorMode);
    end;
  rlmctRequestorPID: begin
    AValue := @FRequestorProcessId;
    AValueSize := SizeOf(FRequestorProcessId);
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
  rlmctPreviousMode: AResult := AccessModeToString(FPreviousMode);
  rlmctRequestorMode: AResult := AccessModeToString(FRequestorMode);
  rlmctRequestorPID : AResult := Format('%d', [FRequestorProcessId]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


End.
