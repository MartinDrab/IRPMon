program IRPMon;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}



uses
{$IFNDEF FPC}
  WinSvc,
{$ELSE}
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
  LibJSON in 'LibJSON.pas',
  FastIoRequest in 'FastIoRequest.pas',
  DataParsers in 'DataParsers.pas',
  FileObjectNameXXXRequest in 'FileObjectNameXXXRequest.pas',
  FillterForm in 'FillterForm.pas' {FilterFrm},
  ProcessXXXRequests in 'ProcessXXXRequests.pas',
  ConnectorSelectionForm in 'ConnectorSelectionForm.pas' {ConnectorSelectionFrm},
  BinaryLogHeader in 'BinaryLogHeader.pas',
  DLLDecider in 'DLLDecider.pas',
  ImageLoadRequest in 'ImageLoadRequest.pas';

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
  end;

Result := AStatus;
end;


Var
  connectorForm : TConnectorSelectionFrm;
  taskList : TTaskOperationList;
  serviceTask : TDriverTaskObject;
  hScm : THandle;
  err : Cardinal;
  connType : EIRPMonConnectorType;
  initInfo : IRPMON_INIT_INFO;
Begin
connType := ictNone;
driverStarted := False;
Application.Initialize;
Application.MainFormOnTaskbar := True;
err := TablesInit('ntstatus.txt', 'ioctl.txt');
If err = ERROR_SUCCESS Then
  begin
  FillChar(initInfo, SizeOf(initInfo), 0);
  initInfo.AddressFamily := 2;
  connectorForm := TConnectorSelectionFrm.Create(Application);
  With connectorForm Do
    begin
    ShowModal;
    If Not Cancelled Then
      begin
      connType := ConnectionType;
      case connType of
        ictDevice : initInfo.DeviceName := PWideChar(DeviceName);
        ictNetwork : begin
          initInfo.NetworkHost := PWideChar(NetworkAddress);
          initInfo.NetworkPort := PWideChar(NetworkPort);
          end;
        end;
      end;
    end;

  If Not connectorForm.Cancelled Then
    begin
    initInfo.ConnectionType := connType;
    hScm := 0;
    If connType = ictDevice Then
      hScm := OpenSCManagerW(Nil, Nil, scmAccess);

    taskList := TTaskOperationList.Create;
    serviceTask := TDriverTaskObject.Create(initInfo, hScm, serviceName, serviceDescription, serviceDescription, ExtractFilePath(Application.ExeName) + 'irpmndrv.sys');
    serviceTask.SetCompletionCallback(OnServiceTaskComplete, Nil);
    If (connType = ictDevice) And (hScm <> 0) Then
      begin
      taskList.Add(hooHook, serviceTask);
      taskList.Add(hooStart, serviceTask);
      end;

    taskList.Add(hooLibraryInitialize, serviceTask);
    With THookProgressFrm.Create(Application, taskList) Do
      begin
      ShowModal;
      Free;
      end;

    Application.CreateForm(TMainFrm, MainFrm);
  MainFrm.ServiceTask := serviceTask;
    MainFrm.TaskList := taskList;
    MainFrm.ConnectorType := connType;
    Application.Run;
    IRPMonDllFinalize;
    If (connType = ictDevice) And (hScm <> 0) Then
      CloseServiceHandle(hScm);

    serviceTask.Free;
    taskList.Free;
    end;

  connectorForm.Free;
  TablesFinit;
  end
Else WinErrorMessage('Unable to initialize name tables', err);
End.

