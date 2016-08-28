Unit RequestThread;

Interface

Uses
  Windows, Classes, SysUtils,
  IRPMonDll;

Type
  TRequestThread = Class (TTHread)
  Private
    FConnected : Boolean;
    FEvent : THandle;
    FSemaphore : THandle;
    FMsgCode : Cardinal;
  Protected
    Procedure Execute; Override;
  Public
    Constructor Create(ACreateSuspended:Boolean; AMsgCode:Cardinal); Reintroduce;
    Destructor Destroy; Override;
    Procedure SignalTerminate;
  end;


Implementation

Uses
  Forms;

Procedure TRequestThread.Execute;
Var
  rq : PREQUEST_GENERAL;
  err : Cardinal;
  otw : Packed Array [0..1] Of THandle;
begin
FreeOnTerminate := False;
otw[0] := FSemaphore;
otw[1] := FEvent;
While Not Terminated  Do
  begin
  err := WaitForMultipleObjects(2, @otw, False, INFINITE);
  Case err Of
    WAIT_OBJECT_0 : begin
      rq := AllocMem(SizeOf(REQUEST_GENERAL));
      If Assigned(rq) Then
        begin
        err := IRPMonDllGetRequest(@rq.Header, SizeOf(REQUEST_GENERAL));
        If err = ERROR_SUCCESS Then
          PostMessage(Application.Handle, FMsgCode, 0, lParam(rq));

        If err <> ERROR_SUCCESS Then
          FreeMem(rq);
        end;
      end;
    WAIT_OBJECT_0 + 1 : Terminate;
    end;
  end;
end;


Procedure TRequestTHread.SignalTerminate;
begin
SetEvent(FEvent);
end;


Constructor TRequestThread.Create(ACreateSuspended:Boolean; AMsgCode:Cardinal);
Var
  err : Cardinal;
begin
Inherited Create(True);
FConnected := False;
FEvent := 0;
FSemaphore := 0;
FMsgCode := AMsgCode;
FSemaphore := CreateSemaphore(Nil, 0, $7FFFFFFF, Nil);
If FSemaphore = 0 Then
  Raise Exception.Create(Format('CreateSemaphore: %u', [GetLastError]));

FEvent := CreateEvent(Nil, False, False, Nil);
If FEvent = 0 Then
  Raise Exception.Create(Format('CreateEvent: %u', [GetLastError]));

err := IRPMonDllConnect(FSemaphore);
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

If FSemaphore <> 0 Then
  CloseHandle(FSemaphore);

Inherited Destroy;
end;

End.
