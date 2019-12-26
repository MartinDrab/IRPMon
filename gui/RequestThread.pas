Unit RequestThread;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Classes, SysUtils, Generics.Collections,
  IRPMonDll;

Type
  TRequestThread = Class (TTHread)
  Private
    FConnected : Boolean;
    FEvent : THandle;
    FMsgCode : Cardinal;
    FCurrentList : TList<PREQUEST_GENERAL>;
    Procedure PortablePostMessage;
    Procedure PostRequestList;
    Function ProcessRequest(AList:TList<PREQUEST_GENERAL>):Cardinal;
  Protected
    Procedure Execute; Override;
  Public
    Constructor Create(ACreateSuspended:Boolean; AMsgCode:Cardinal); Reintroduce;
    Destructor Destroy; Override;
    Procedure SignalTerminate;
  end;


Implementation

Uses
  Forms, MainForm;

Procedure TRequestThread.PortablePostMessage;
begin
MainFrm.OnRequest(FCurrentList);
end;

Procedure TRequestThread.PostRequestList;
begin
{$IFDEF FPC}
Synchronize(PortablePostMessage);
{$ELSE}
PostMessage(Application.Handle, FMsgCode, 0, lParam(FCurrentList));
{$ENDIF}
end;

Function TRequestThread.ProcessRequest(AList:TList<PREQUEST_GENERAL>):Cardinal;
Var
  rq : PREQUEST_GENERAL;
  bufSize : Cardinal;
begin
bufSize := 65536;
Repeat
rq := AllocMem(bufSize);
If Assigned(rq) Then
  begin
  Result := IRPMonDllGetRequest(@rq.Header, bufSize);
  If Result = ERROR_SUCCESS Then
    begin
    AList.Add(rq);
    If ((rq.Header.Flags And REQUEST_FLAG_NEXT_AVAILABLE) = 0) Or
       (AList.Count >= 200) Then
      Break;
    end;

  If Result <> ERROR_SUCCESS Then
    begin
    FreeMem(rq);
    If Result = ERROR_INSUFFICIENT_BUFFER Then
      bufSize := bufSize * 2;
    end;
  end
Else Result := ERROR_NOT_ENOUGH_MEMORY;

Until (Result <> ERROR_SUCCESS) And (Result <> ERROR_INSUFFICIENT_BUFFER);
end;

Procedure TRequestThread.Execute;
Var
  waitRes : Cardinal;
  l : TList<PREQUEST_GENERAL>;
begin
FreeOnTerminate := False;
l := TList<PREQUEST_GENERAL>.Create;
While Not Terminated  Do
  begin
  ProcessRequest(l);
  If l.Count >= 200 Then
    begin
    FCurrentList := l;
    PostRequestList;
    l := TList<PREQUEST_GENERAL>.Create;
    end;

  waitRes := WaitForSingleObject(FEvent, 1000);
  Case waitRes Of
    WAIT_TIMEOUT: begin
      If l.Count > 0 Then
        begin
        FCurrentList := l;
        PostRequestList;
        l := TList<PREQUEST_GENERAL>.Create;
        end;
      end;
    WAIT_OBJECT_0: Break;
    end;
  end;

FConnected := False;
IRPMonDllDisconnect;
l.Free;
end;


Procedure TRequestTHread.SignalTerminate;
begin
FConnected := False;
SetEvent(FEvent);
end;


Constructor TRequestThread.Create(ACreateSuspended:Boolean; AMsgCode:Cardinal);
Var
  err : Cardinal;
begin
Inherited Create(True);
FConnected := False;
FEvent := 0;
FMsgCode := AMsgCode;
FEvent := CreateEvent(Nil, False, False, Nil);
If FEvent = 0 Then
  Raise Exception.Create(Format('CreateEvent: %u', [GetLastError]));

err := IRPMonDllConnect;
If err <> ERROR_SUCCESS Then
  Raise Exception.Create(Format('IRPMonDllConnect: %u', [err]));

FConnected := True;
If Not ACreateSuspended Then
  Resume;
end;

Destructor TRequestTHread.Destroy;
begin
If FConnected Then
  IRPMonDllDisconnect;

If FEvent <> 0 Then
  CloseHandle(FEvent);

Inherited Destroy;
end;

End.
