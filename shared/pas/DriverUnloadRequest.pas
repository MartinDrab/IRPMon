unit DriverUnloadRequest;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  IRPMonDll,
  AbstractRequest,
  IRPMonRequest;

Type
  TDriverUnloadRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_UNLOAD); Overload;

    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
  end;


Implementation

Constructor TDriverUnloadRequest.Create(Var ARequest:REQUEST_UNLOAD);
begin
Inherited Create(ARequest.Header);
end;

Function TDriverUnloadRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResultValue,
  rlmctFileObject,
  rlmctFileName,
  rlmctIOSBStatusValue,
  rlmctIOSBStatusConstant,
  rlmctIOSBInformation,
  rlmctResultConstant  : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


End.
