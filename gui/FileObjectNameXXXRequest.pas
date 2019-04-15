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
    Private
      FFileObject : Pointer;
      FFileName : WideString;
    Public
      Constructor Create(Var ARequest:REQUEST_FILE_OBJECT_NAME_ASSIGNED); Reintroduce;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;

      Property FileObject : Pointer Read FFileObject;
      Property FileName : WideString Read FFileName;
    end;

  TFileObjectNameDeletedRequest = Class (TDriverRequest)
    Private
      FFileObject : Pointer;
    Public
      Constructor Create(Var ARequest:REQUEST_FILE_OBJECT_NAME_DELETED); Reintroduce;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;

      Property FileObject : Pointer Read FFileObject;
    end;



Implementation

Uses
  SysUtils;

(** TFileObjectNameAssignedRequest **)

Constructor TFileObjectNameAssignedRequest.Create(Var ARequest:REQUEST_FILE_OBJECT_NAME_ASSIGNED);
Var
  fn : PWideChar;
begin
Inherited Create(ARequest.Header);
FFileObject := ARequest.FileObject;
fn := PWideChar(PByte(@ARequest) + SizeOf(REQUEST_FILE_OBJECT_NAME_ASSIGNED));
SetLength(FFileName, ARequest.FileNameLength Div SizeOf(WideChar));
CopyMemory(PWideChar(FFileName), fn, ARequest.FileNameLength);
SetFileName(FFileName);
end;

Function TFileObjectNameAssignedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResult,
  rlmctDriverObject,
  rlmctDriverName : Result := False;
  rlmctFileObject : AResult := Format('0x%p', [FFileObject]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


(** TFileObjectNameDeletedRequest **)

Constructor TFileObjectNameDeletedRequest.Create(Var ARequest:REQUEST_FILE_OBJECT_NAME_DELETED);
begin
Inherited Create(ARequest.Header);
FFileObject := ARequest.FileObject;
end;

Function TFileObjectNameDeletedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResult,
  rlmctDriverObject,
  rlmctDriverName : Result := False;
  rlmctFileObject : AResult := Format('0x%p', [FFileObject]);
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


End.
