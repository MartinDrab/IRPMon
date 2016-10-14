program IRPMon;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

{$R 'uac.res' 'uac.rc'}

uses
{$IFnDEF FPC}
  WinSvc,
{$ELSE}
  jwaWinSvc,
  Interfaces,
{$ENDIF}
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
  LibJSON in 'LibJSON.pas';

{$R *.res}

Const
  serviceAccess = MAXIMUM_ALLOWED;
  scmAccess = MAXIMUM_ALLOWED;
  serviceName = 'irpmndrv';
  serviceDescription = 'IRPMon Driver Service';
  driverFileName = 'irpmndrv.sys';

Var
  startArgs : PWideChar;
  hScm : THandle;
  hService : THandle;
  err : Cardinal;
  serviceStatus : SERVICE_STATUS;
Begin
Application.Initialize;
Application.MainFormOnTaskbar := True;
err := TablesInit('ntstatus.txt', 'ioctl.txt');
If err = ERROR_SUCCESS Then
  begin
  hScm := OpenSCManagerW(Nil, Nil, scmAccess);
  If hScm <> 0 Then
    begin
    hService := CreateServiceW(hScm, serviceName, serviceDescription, serviceAccess, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, PWideChar(WideString(ExtractFilePath(Application.ExeName) + driverFileName)), Nil, Nil, Nil, Nil, Nil);
    If hService = 0 Then
      err := GetLastError;

    If (hService <> 0) Or (err = ERROR_SERVICE_EXISTS) Or
       (err = ERROR_SERVICE_MARKED_FOR_DELETE) Then
      begin
      If err = ERROR_SERVICE_EXISTS Then
        begin
        err := ERROR_SUCCESS;
        hService := OpenServiceW(hScm, serviceName, serviceAccess);
        If hService = 0 Then
          err := GetLastError;
        end;

      If (err = ERROR_SUCCESS) Or (err = ERROR_SERVICE_MARKED_FOR_DELETE) Then
        begin
        err := ERROR_SUCCESS;
        startArgs := Nil;
        If hService <> 0 Then
          begin
          If Not StartServiceW(hService, 0, @startArgs) Then
            begin
            err := GetLastError;
            If err = ERROR_SERVICE_ALREADY_RUNNING THen
              err := ERROR_SUCCESS;
            end;
          end;

        If err = ERROR_SUCCESS Then
          begin
          err := IRPMonDllInitialize;
          If err = ERROR_SUCCESS Then
            begin
            Application.CreateForm(TMainFrm, MainFrm);
  Application.Run;
            IRPMonDllFinalize;
            end
          Else WinErrorMessage('Unable to initialize irpmondll.dll', err);

          If (err <> ERROR_SUCCESS) And (hService <> 0) Then
            ControlService(hService, SERVICE_CONTROL_STOP, serviceStatus);
          end
        Else WinErrorMessage('Failed to start the IRPMon driver', err);

        If hService <> 0 Then
          ;begin
          DeleteService(hService);
          CloseServiceHandle(hService);
          end;
        end
      Else WinErrorMessage('Failed to open IRPMon driver service', err);
      end
    Else WinErrorMessage('Failed to create a service entry for the IRPMon driver', err);

    CloseServiceHandle(hScm);
    end
  Else WinErrorMessage('Unable to access SCM database', GetLastError);

  TablesFinit;
  end
Else WinErrorMessage('Unable to initialize name tables', err);
End.

