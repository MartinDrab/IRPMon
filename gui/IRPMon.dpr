program IRPMon;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

{$R 'uac.res' 'uac.rc'}

uses
  WinSvc,
  Windows,
  SysUtils,
  Forms,
  MainForm in 'MainForm.pas' {MainFrm},
  IRPMonDll in 'IRPMonDll.pas',
  IRPMonRequest in 'IRPMonRequest.pas',
  Utils in 'Utils.pas',
  ListModel in 'ListModel.pas',
  RequestListModel in 'RequestListModel.pas',
  IRPRequest in 'IRPRequest.pas',
  NameTables in 'NameTables.pas',
  RequestFilter in 'RequestFilter.pas',
  TreeForm in 'TreeForm.pas' {TreeFrm},
  HookObjects in 'HookObjects.pas',
  HookProgressForm in 'HookProgressForm.pas' {HookProgressFrm},
  RequestThread in 'RequestThread.pas',
  RequestDetailsForm in 'RequestDetailsForm.pas' {RequestDetailsFrm},
  AboutForm in 'AboutForm.pas' {AboutBox},
  ClassWatch in 'ClassWatch.pas',
  ClassWatchAdd in 'ClassWatchAdd.pas' {ClassWatchAddFrm},
  DriverNameWatchAddForm in 'DriverNameWatchAddForm.pas' {DriverNameWatchAddFrm},
  WatchedDriverNames in 'WatchedDriverNames.pas',
  XXXDetectedRequests in 'XXXDetectedRequests.pas',
  LibJSON in 'LibJSON.pas',
  FastIoRequest in 'FastIoRequest.pas',
  DataParsers in 'DataParsers.pas',
  FileObjectNameXXXRequest in 'FileObjectNameXXXRequest.pas',
  FillterForm in 'FillterForm.pas' {FilterFrm},
  ProcessXXXRequests in 'ProcessXXXRequests.pas';

{$R *.res}

Const
  scmAccess = MAXIMUM_ALLOWED;
  serviceName = 'irpmndrv';
  serviceDescription = 'IRPMon Driver Service';
  driverFileName = 'irpmndrv.sys';


Var
  driverStarted : Boolean;

Function OnServiceTaskComplete(AList:TTaskOperationList; AObject:TTaskObject; AOperation:EHookObjectOperation; AStatus:Cardinal; AContext:Pointer):Cardinal;
begin
Case AOperation Of
  hooHook: ;
  hooUnhook: ;
  hooStart: driverStarted := (AStatus = ERROR_SUCCESS);
  hooStop: ;
  Else Result := ERROR_NOT_SUPPORTED;
  end;

Result := AStatus;
end;


Var
  taskList : TTaskOperationList;
  serviceTask : TDriverTaskObject;
  hScm : THandle;
  err : Cardinal;
  wow64 : LongBool;
Begin
If IsWow64Process(GetCurrentProcess, wow64) Then
  begin
  If Not wow64 Then
    begin
    driverStarted := False;
    Application.Initialize;
    Application.MainFormOnTaskbar := True;
    err := TablesInit('ntstatus.txt', 'ioctl.txt');
    If err = ERROR_SUCCESS Then
      begin
      hScm := OpenSCManagerW(Nil, Nil, scmAccess);
      If hScm <> 0 Then
        begin
        taskList := TTaskOperationList.Create;
        serviceTask := TDriverTaskObject.Create(hScm, serviceName, serviceDescription, serviceDescription, ExtractFilePath(Application.ExeName) + 'irpmndrv.sys');
        serviceTask.SetCompletionCallback(OnServiceTaskComplete, Nil);
        taskList.Add(hooHook, serviceTask);
        taskList.Add(hooStart, serviceTask);
        taskList.Add(hooLibraryInitialize, serviceTask);
        With THookProgressFrm.Create(Application, taskList) Do
          begin
          ShowModal;
          Free;
          end;

        Application.CreateForm(TMainFrm, MainFrm);
  MainFrm.TaskList := taskList;
        MainFrm.ServiceTask := serviceTask;
        Application.Run;
        IRPMonDllFinalize;

        serviceTask.Free;
        taskList.Free;
        CloseServiceHandle(hScm);
        end
      Else WinErrorMessage('Unable to access SCM database', GetLastError);

      TablesFinit;
      end
    Else WinErrorMessage('Unable to initialize name tables', err);
    end
  Else ErrorMessage('IRPMon cannot be run under WOW64. Please run the 64-bit version of the program');
  end
Else WinErrorMessage('Failed to determine whether the IRPMon process is runnin under WOW64', GetLastError);
End.

