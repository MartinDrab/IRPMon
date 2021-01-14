Library ReqList;

{$Z4}
{$MINENUMSIZE 4}

uses
  Windows,
  System.SysUtils,
  System.Classes,
  Generics.Collections,
  IRPMonDll in '..\shared\pas\IRPMonDll.pas',
  AbstractRequest in '..\shared\pas\AbstractRequest.pas',
  AddDeviceRequest in '..\shared\pas\AddDeviceRequest.pas',
  DriverUnloadRequest in '..\shared\pas\DriverUnloadRequest.pas',
  FastIoRequest in '..\shared\pas\FastIoRequest.pas',
  FileObjectNameXXXRequest in '..\shared\pas\FileObjectNameXXXRequest.pas',
  ImageLoadRequest in '..\shared\pas\ImageLoadRequest.pas',
  IRPCompleteRequest in '..\shared\pas\IRPCompleteRequest.pas',
  IRPMonRequest in '..\shared\pas\IRPMonRequest.pas',
  IRPRequest in '..\shared\pas\IRPRequest.pas',
  ProcessXXXRequests in '..\shared\pas\ProcessXXXRequests.pas',
  RequestList in '..\shared\pas\RequestList.pas',
  StartIoRequest in '..\shared\pas\StartIoRequest.pas',
  XXXDetectedRequests in '..\shared\pas\XXXDetectedRequests.pas',
  Utils in '..\shared\pas\Utils.pas',
  NameTables in '..\shared\pas\NameTables.pas',
  DataParsers in '..\shared\pas\DataParsers.pas',
  BinaryLogHeader in '..\shared\pas\BinaryLogHeader.pas';

Type
  ERequestListObjectType = (
    rlotDriver,
    rlotDevice,
    rlotFile,
    rlotProcess
  );

  REQUEST_LIST_CALLBACK = Procedure (ARequest:PREQUEST_HEADER; AContext:Pointer; Var AStore:ByteBool); Cdecl;

  _REQUEST_LIST_CALLBACK_RECORD = Record
    Routine : REQUEST_LIST_CALLBACK;
    Context : Pointer;
    end;
  REQUEST_LIST_CALLBACK_RECORD = _REQUEST_LIST_CALLBACK_RECORD;
  PREQUEST_LIST_CALLBACK_RECORD = ^REQUEST_LIST_CALLBACK_RECORD;

Var
  callbacks : TDictionary<Pointer, REQUEST_LIST_CALLBACK_RECORD>;

{$R *.RES}

Procedure OnRequest(ARequestList:TRequestList; ARequest:TDriverRequest; Var AStore:Boolean);
Var
  r : REQUEST_LIST_CALLBACK_RECORD;
  tmp : ByteBool;
begin
AStore := True;
If callbacks.TryGetValue(ARequestList, r) Then
  begin
  tmp := True;
  r.Routine(ARequest.Raw, r.Context, tmp);
  AStore := tmp;
  end;
end;

Procedure DllMain(reason: Integer);
begin
Case Reason Of
  DLL_PROCESS_ATTACH : begin
    TablesInit('ntstatus.txt', 'ioctl.txt');
    end;
  DLL_PROCESS_DETACH : begin
    TablesFinit;
    end;
  end;
end;



Function ReqListCreate(Var AList:Pointer):Cardinal; Cdecl;
Var
  l : TRequestList;
begin
Try
  l := TRequestList.Create;
  l.OnRequestProcessed := OnRequest;
  Result := 0;
Except
  Result := ERROR_NOT_ENOUGH_MEMORY;
  end;

If Result = 0 Then
  AList := l;
end;

Procedure ReqListFree(AList:Pointer); Cdecl;
Var
  l : TRequestList;
begin
l := AList;
l.Free;
end;

Procedure ReqListAssignParserList(AList:Pointer; AParsers:Pointer); Cdecl;
Var
  l : TRequestList;
  dp : TObjectList<TDataParser>;
begin
l := AList;
dp := AParsers;
l.Parsers := dp;
end;

Function ReqListAdd(AList:Pointer; ABuffer:PREQUEST_GENERAL):Cardinal; Cdecl;
Var
  l : TRequestList;
begin
l := AList;
Result := l.ProcessBuffer(ABuffer);
end;

Procedure ReqListClear(AList:Pointer); Cdecl;
Var
  l : TRequestList;
begin
l := AList;
l.Clear;
end;

Function ReqListGetObjectName(AList:Pointer; AObject:Pointer; AType:ERequestListObjectType; Var AName:PWideChar):Cardinal; Cdecl;
Var
  ret : Boolean;
  l : TRequestList;
  nameString : WideString;
begin
ret := False;
Result := ERROR_FILE_NOT_FOUND;
AName := Nil;
l := AList;
Case AType Of
  rlotDriver : ret := l.GetDriverName(AObject, nameString);
  rlotDevice : ret := l.GetDeviceName(AObject, nameString);
  rlotFile : ret := l.GetFileName(AObject, nameString);
  rlotProcess : ret := l.GetProcessName(Cardinal(AObject), nameString);
  Else Result := ERROR_INVALID_PARAMETER;
  end;

If ret Then
  begin
  AName := StrAlloc(Length(nameString));
  If Assigned(AName) Then
    begin
    StringToWideChar(nameString, AName, Length(nameString));
    Result := 0;
    end
  Else Result := ERROR_NOT_ENOUGH_MEMORY;
  end;
end;

Procedure ReqListFreeObjectName(AName:PWideChar); Cdecl;
begin
FreeMem(AName);
end;

Function ReqListSetCallback(AList:Pointer; ARoutine:REQUEST_LIST_CALLBACK; AContext:Pointer):Cardinal; Cdecl;
Var
  r : REQUEST_LIST_CALLBACK_RECORD;
begin
r.Routine := ARoutine;
r.Context := AContext;
Try
  callbacks.Add(AList, r);
  Result := 0;
Except
  Result := ERROR_NOT_ENOUGH_MEMORY;
  end;
end;

Procedure ReqListUnregisterCallback(AList:Pointer); Cdecl;
begin
callbacks.Remove(AList);
end;

Function ReqListSave(AList:Pointer; AFormat:ERequestLogFormat; AFileName:PWideChar):Cardinal; Cdecl;
Var
  l : TRequestList;
  fn : WideString;
begin
l := AList;
Try
  fn := WideCharToString(AFileName);
  l.SaveToFile(fn, AFormat);
  Result := 0;
Except
  Result := ERROR_GEN_FAILURE;
  end;
end;

Function ReqListLoad(AList:Pointer; AFileName:PWideChar):Cardinal; Cdecl;
Var
  l : TRequestList;
  fn : WideString;
begin
l := AList;
Try
  fn := WideCharToString(AFileName);
  l.LoadFromFile(fn);
  Result := 0;
Except
  Result := ERROR_GEN_FAILURE;
  end;
end;


Exports
  ReqListCreate,
  ReqListFree,
  ReqListAssignParserList,
  ReqListAdd,
  ReqListClear,
  ReqListGetObjectName,
  ReqListFreeObjectName,
  ReqListSetCallback,
  ReqListUnregisterCallback,
  ReqListSave,
  ReqListLoad;

begin
DLLProc := DllMain;
DllMain(DLL_PROCESS_ATTACH);
end.