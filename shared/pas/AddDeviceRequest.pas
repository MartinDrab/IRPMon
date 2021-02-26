unit AddDeviceRequest;

{$IFDEF FPC}
{$MODE Delphi}
{$ENDIF}

Interface

Uses
  IRPMonDLl,
  AbstractRequest,
  IRPMonRequest;

Type
  TAddDeviceRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_ADDDEVICE); Overload;

    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Override;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
  end;

Implementation

Constructor TAddDeviceRequest.Create(Var ARequest:REQUEST_ADDDEVICE);
begin
Inherited Create(ARequest.Header);
end;

Function TAddDeviceRequest.GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean;
begin
Case AColumnType Of
  rlmctFileObject,
  rlmctIOSBStatusValue,
  rlmctIOSBStatusConstant,
  rlmctIOSBInformation,
  rlmctResultValue,
  rlmctResultConstant,
  rlmctFileName : Result := False;
  Else Result := Inherited GetColumnValueRaw(AColumnType, AValue, AValueSize);
  end;
end;

Function TAddDeviceRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Case AColumnType Of
  rlmctFileObject,
  rlmctIOSBStatusValue,
  rlmctIOSBStatusConstant,
  rlmctIOSBInformation,
  rlmctResultValue,
  rlmctResultConstant,
  rlmctFileName : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


End.
