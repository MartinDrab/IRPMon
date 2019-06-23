Unit ProcessXXXRequests;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows,
  RequestListModel, IRPMonDll;

Type
  TProcessCreatedRequest = Class (TDriverRequest)
   Public
    Constructor Create(Var ARequest:REQUEST_PROCESS_CREATED); Overload;
    Function GetColumnName(AColumnType:ERequestListModelColumnType):WideString; Override;
  end;

  TProcessExittedRequest = Class (TDriverRequest)
   Public
    Constructor Create(Var ARequest:REQUEST_PROCESS_EXITTED); Overload;
    Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Override;
    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
  end;


Implementation

Uses
  SysUtils;

(** TProcessCreatedRequest **)

Constructor TProcessCreatedRequest.Create(Var ARequest:REQUEST_PROCESS_CREATED);
Var
  tmp : WideString;
begin
Inherited Create(ARequest.Header);
tmp := '';
SetLength(tmp, ARequest.ImageNameLength Div SizeOf(WideChar));
CopyMemory(PWideChar(tmp), PByte(@ARequest) + SizeOf(ARequest), ARequest.ImageNameLength);
SetFileName(tmp);
SetDriverName(ExtractFileName(tmp));
SetLength(tmp, ARequest.CommandLineLength Div SizeOf(WideChar));
CopyMemory(PWideChar(tmp), PByte(@ARequest) + SizeOf(ARequest) + ARequest.ImageNameLength, ARequest.CommandLineLength);
SetDeviceName(tmp);
FDriverObject := Pointer(ARequest.ProcessId);
FDeviceObject := Pointer(ARequest.ParentId);
end;

Function TProcessCreatedRequest.GetColumnName(AColumnType:ERequestListModelColumnType):WideString;
begin
Result := '';
case AColumnType of
  rlmctDeviceObject: Result := 'Parent PID';
  rlmctDeviceName: Result := 'Command-line';
  rlmctDriverObject: Result := 'Process ID';
  rlmctDriverName: Result := 'Process name';
  rlmctFileName: Result := 'File name';
  Else Result := Inherited GetColumnName(AColumnType);
  end;
end;

(** TProcessExittedRequest **)

Constructor TProcessExittedRequest.Create(Var ARequest:REQUEST_PROCESS_EXITTED);
begin
Inherited Create(ARequest.Header);
end;

Function TProcessExittedRequest.GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean;
begin
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctDriverObject,
  rlmctDriverName,
  rlmctFileObject,
  rlmctFileName : Result := False;
  Else Result := Inherited GetColumnValueRaw(AColumnType, AValue, AValueSize);
  end;
end;

Function TProcessExittedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctDriverObject,
  rlmctDriverName,
  rlmctFileObject,
  rlmctFileName : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

End.
