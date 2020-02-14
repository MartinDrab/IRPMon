Unit FileObjectNameXXXRequest;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows,
  RequestListModel, IRPMonDll;

Type
  TFileObjectNameAssignedRequest = Class (TDriverRequest)
    Public
      Constructor Create(Var ARequest:REQUEST_FILE_OBJECT_NAME_ASSIGNED); Overload;
      Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;

  TFileObjectNameDeletedRequest = Class (TDriverRequest)
    Public
      Constructor Create(Var ARequest:REQUEST_FILE_OBJECT_NAME_DELETED); Overload;
      Function GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean; Override;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    end;



Implementation

Uses
  SysUtils;

(** TFileObjectNameAssignedRequest **)

Constructor TFileObjectNameAssignedRequest.Create(Var ARequest:REQUEST_FILE_OBJECT_NAME_ASSIGNED);
Var
  fn : PWideChar;
  rawReq : PREQUEST_FILE_OBJECT_NAME_ASSIGNED;
  tmpFileName : WideString;
begin
Inherited Create(ARequest.Header);
rawReq := PREQUEST_FILE_OBJECT_NAME_ASSIGNED(FRaw);
SetFileObject(rawReq.FileObject);
fn := PWideChar(PByte(rawReq) + SizeOf(REQUEST_FILE_OBJECT_NAME_ASSIGNED));
SetLength(tmpFileName, rawReq.FileNameLength Div SizeOf(WideChar));
CopyMemory(PWideChar(tmpFileName), fn, rawReq.FileNameLength);
SetFileName(tmpFileName);
end;

Function TFileObjectNameAssignedRequest.GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean;
begin
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResultValue,
  rlmctResultConstant,
  rlmctDriverObject,
  rlmctDriverName : Result := False;
  Else Result := Inherited GetColumnValueRaw(AColumnType, AValue, AValueSize);
  end;
end;

Function TFileObjectNameAssignedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResultValue,
  rlmctResultConstant,
  rlmctDriverObject,
  rlmctDriverName : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


(** TFileObjectNameDeletedRequest **)

Constructor TFileObjectNameDeletedRequest.Create(Var ARequest:REQUEST_FILE_OBJECT_NAME_DELETED);
begin
Inherited Create(ARequest.Header);
SetFileObject(ARequest.FileObject);
end;

Function TFileObjectNameDeletedRequest.GetColumnValueRaw(AColumnType:ERequestListModelColumnType; Var AValue:Pointer; Var AValueSize:Cardinal):Boolean;
begin
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResultValue,
  rlmctResultConstant,
  rlmctDriverObject,
  rlmctDriverName : Result := False;
  Else Result := Inherited GetColumnValueRaw(AColumnType, AValue, AValueSize);
  end;
end;

Function TFileObjectNameDeletedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResultValue,
  rlmctResultConstant,
  rlmctDriverObject,
  rlmctDriverName : Result := False;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


End.
