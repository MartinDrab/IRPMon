Unit MainForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics,
  Controls, Forms, Dialogs, ComCtrls, Menus,
  Generics.Collections, RequestFilter,
  IRPMonDll, RequestListModel, ExtCtrls,
  HookObjects, RequestThread, DataParsers
{$IFNDEF FPC}
  , AppEvnts
{$ENDIF}
  ;

Type
  TMainFrm = Class (TForm)
    MainMenu1: TMainMenu;
    ActionMenuItem: TMenuItem;
    SelectDriversDevicesMenuItem: TMenuItem;
    ExitMenuItem: TMenuItem;
    MonitoringMenuItem: TMenuItem;
    ClearMenuItem: TMenuItem;
    HelpMenuItem: TMenuItem;
    AboutMenuItem: TMenuItem;
    N5: TMenuItem;
    ColumnsMenuItem: TMenuItem;
    PageControl1: TPageControl;
    RequestTabSheet: TTabSheet;
    RequestListView: TListView;
    CaptureEventsMenuItem: TMenuItem;
    N6: TMenuItem;
    RefreshNameCacheMenuItem: TMenuItem;
    RequestMenuItem: TMenuItem;
    RequestDetailsMenuItem: TMenuItem;
    SaveMenuItem: TMenuItem;
    LogSaveDialog: TSaveDialog;
    N1: TMenuItem;
    WatchClassMenuItem: TMenuItem;
    WatchedClassesMenuItem: TMenuItem;
    WatchDriverNameMenuItem: TMenuItem;
    WatchedDriversMenuItem: TMenuItem;
    DriverMenuItem: TMenuItem;
    UnloadOnExitMenuItem: TMenuItem;
    UninstallOnExitMenuItem: TMenuItem;
    Documentation1: TMenuItem;
    DataParsersTabSheet: TTabSheet;
    DataParsersListView: TListView;
    FiltersMenuItem: TMenuItem;
    LogOpenDialog: TOpenDialog;
    OpenMenuItem: TMenuItem;
    N2: TMenuItem;
    HideExcludedRequestsMenuItem: TMenuItem;
    RequestPopupMenu: TPopupMenu;
    RPDetailsMenuItem: TMenuItem;
    N3: TMenuItem;
    RPIncludeMenuItem: TMenuItem;
    RPHighlightMenuItem: TMenuItem;
    RPExcludeMenuItem: TMenuItem;
    HighlightColorDialog: TColorDialog;
    StatusBar1: TStatusBar;
    StatusTimer: TTimer;
    N4: TMenuItem;
    ReqQueueClearOnDisconnectMenuItem: TMenuItem;
    ReqQueueCollectWhenDisconnectedMenuItem: TMenuItem;
    ProcessEventsCollectMenuItem: TMenuItem;
    FileObjectEventsCollectMenuItem: TMenuItem;
    DriverSnapshotEventsCollectMenuItem: TMenuItem;
    ProcessEmulateOnConnectMenuItem: TMenuItem;
    DriverSnapshotOnConnectMenuItem: TMenuItem;
    CompressMenuItem: TMenuItem;
    IgnoreLogFileHeadersMenuItem: TMenuItem;
    StripRequestDataMenuItem: TMenuItem;
    MaxRequestDataSizeMenuItem: TMenuItem;
    RPIncludeAllMenuItem: TMenuItem;
    RPHighlightAllMenuItem: TMenuItem;
    RPExcludeAllMenuItem: TMenuItem;
    CopyMenuItem: TMenuItem;
    CopyVisibleColumnsMenuItem: TMenuItem;
    CopyWholeLineMenuItem: TMenuItem;
    Procedure ClearMenuItemClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure CaptureEventsMenuItemClick(Sender: TObject);
    procedure ExitMenuItemClick(Sender: TObject);
    procedure RefreshNameCacheMenuItemClick(Sender: TObject);
    procedure SelectDriversDevicesMenuItemClick(Sender: TObject);
    procedure RequestDetailsMenuItemClick(Sender: TObject);
    procedure AboutMenuItemClick(Sender: TObject);
    procedure SaveMenuItemClick(Sender: TObject);
    procedure WatchClassMenuItemClick(Sender: TObject);
    procedure WatchDriverNameMenuItemClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure DriverMenuItemClick(Sender: TObject);
    procedure Documentation1Click(Sender: TObject);
    procedure DataParsersListViewData(Sender: TObject; Item: TListItem);
    procedure DataParsersTabSheetShow(Sender: TObject);
    procedure FiltersMenuItemClick(Sender: TObject);
    procedure OpenMenuItemClick(Sender: TObject);
    procedure HideExcludedRequestsMenuItemClick(Sender: TObject);
    procedure RPDetailsMenuItemClick(Sender: TObject);
    procedure RequestPopupMenuPopup(Sender: TObject);
    procedure PopupFilterClick(Sender: TObject);
    procedure StatusTimerTimer(Sender: TObject);
    procedure DriverSettingsMenuItemClick(Sender: TObject);
    procedure DriverMenuItemExpand(Sender: TObject);
    procedure CompressMenuItemClick(Sender: TObject);
    procedure IgnoreLogFileHeadersMenuItemClick(Sender: TObject);
    procedure CopyVisibleColumnsMenuItemClick(Sender: TObject);
  Private
{$IFDEF FPC}
    FAppEvents: TApplicationProperties;
{$ELSE}
    FAppEvents: TApplicationEvents;
{$ENDIF}
    FModel : TRequestListModel;
    FHookedDrivers : TDictionary<Pointer, TDriverHookObject>;
    FHookedDevices : TDictionary<Pointer, TDeviceHookObject>;
    FHookedDeviceDriverMap : TDictionary<Pointer, Pointer>;
    FRequestTHread : TRequestThread;
    FRequestMsgCode : Cardinal;
    FParsers : TObjectList<TDataParser>;
    FFilters : TObjectList<TRequestFilter>;
    Procedure EnumerateHooks;
    Procedure EnumerateClassWatches;
    Procedure EnumerateDriverNameWatches;
    Procedure OnWatchedClassClick(Sender:TObject);
    Procedure OnWatchedDriverNameClick(Sender:TObject);
    procedure IrpMonAppEventsMessage(var Msg: tagMSG; var Handled: Boolean);
    Procedure IrpMonAppEventsException(Sender: TObject; E: Exception);
    Procedure WriteSettings;
    Procedure ReadSettings;
    Function GetLocalSettingsDirectory:WideString;
    Procedure OnRequestProcessed(ARequest:TDriverRequest; Var AStore:Boolean);
  Public
    ConnectorType : EIRPMonConnectorType;
    ServiceTask : TDriverTaskObject;
    TaskList : TTaskOperationList;
    Procedure OnRequest(AList:TList<PREQUEST_GENERAL>);
  end;

Var
  MainFrm: TMainFrm;

Implementation

{$R *.dfm}

Uses
{$IFNDEF FPC}
  IOUtils,
{$ELSE}
  LazFileUtils,
{$ENDIF}
  IniFiles, ShellAPI, Clipbrd,
  ListModel, HookProgressForm,
  Utils, TreeForm, RequestDetailsForm, AboutForm,
  ClassWatchAdd, ClassWatch, DriverNameWatchAddForm,
  WatchedDriverNames, FillterForm;



Procedure TMainFrm.EnumerateHooks;
Var
  phde : TPair<Pointer, TDeviceHookObject>;
  phdr : TPair<Pointer, TDriverHookObject>;
  hdr : TDriverHookObject;
  hde : TDeviceHookObject;
  err : Cardinal;
  I, J : Integer;
  count : Cardinal;
  dri : PHOOKED_DRIVER_UMINFO;
  tmp : PHOOKED_DRIVER_UMINFO;
  dei : PHOOKED_DEVICE_UMINFO;
begin
err := IRPMonDllDriverHooksEnumerate(dri, count);
If err = ERROR_SUCCESS Then
  begin
  For phde In FHookedDevices Do
    phde.Value.Free;

  FHookedDevices.Clear;
  For phdr In FHookedDrivers Do
    phdr.Value.Free;

  FHookedDrivers.Clear;
  FHookedDeviceDriverMap.Clear;
  tmp := dri;
  For I := 0 To count - 1 Do
    begin
    hdr := TDriverHookObject.Create(tmp.DriverObject, tmp.DriverName);
    hdr.ObjectId := tmp.ObjectId;
    hdr.Hooked := True;
    FHookedDrivers.Add(hdr.Address, hdr);
    dei := tmp.HookedDevices;
    For J := 0 To tmp.NumberOfHookedDevices - 1 Do
      begin
      hde := TDeviceHookObject.Create(dei.DeviceObject, tmp.DriverObject, Nil, dei.DeviceName);
      hde.ObjectId := dei.ObjectId;
      hde.Hooked := True;
      FHookedDevices.Add(hde.Address, hde);
      FHookedDeviceDriverMap.Add(hde.Address, hdr.Address);
      Inc(dei);
      end;

    Inc(tmp);
    end;

  IRPMonDllDriverHooksFree(dri, count);
  end
Else WInErrorMessage('Failed to enumerate hooked objects', err);
end;

Procedure TMainFrm.ExitMenuItemClick(Sender: TObject);
begin
Close;
end;

Procedure TMainFrm.FiltersMenuItemClick(Sender: TObject);
Var
  filtersDir : WideString;
  rf : TRequestFilter;
begin
With TFilterFrm.Create(Application, FFilters) Do
  begin
  ShowModal;
  If Not Cancelled Then
    begin
    FFilters.Clear;
    For rf In FilterList Do
      FFilters.Add(rf);

    filtersDir := ExtractFilePath(Application.ExeName);
    If Not TRequestFilter.SaveList(filtersDir + 'filters.ini', FFilters) Then
      begin
      filtersDir := GetLocalSettingsDirectory;
      If Not TRequestFilter.SaveList(filtersDir + 'filters.ini', FFilters) Then
        ErrorMessage('Unable to save request filters to a file');
      end;

    FModel.Reevaluate;
    end;

  Free;
  end;
end;

Procedure TMainFrm.FormClose(Sender: TObject; var Action: TCloseAction);
begin
FAppEvents.Free;
WriteSettings;
DataParsersListView.Items.Count := 0;
FParsers.Free;
taskList.Add(hooLibraryFinalize, serviceTask);
If ConnectorType = ictDevice Then
  begin
  If UnloadOnExitMenuItem.Checked Then
    taskList.Add(hooStop, serviceTask);

  If UninstallOnExitMenuItem.Checked Then
    taskList.Add(hooUnhook, serviceTask);
  end;

With THookProgressFrm.Create(Application, taskList) Do
  begin
  ShowModal;
  Free;
  end;

FFilters.Free;
end;

Procedure TMainFrm.FormCreate(Sender: TObject);
Var
  fileName : WideString;
  filtersLoaded : Boolean;
  begin
FFilters := TObjectList<TRequestFilter>.Create;
filtersLoaded := False;
fileName := GetLocalSettingsDirectory + 'filters.ini';
If FileExists(fileName) Then
  filtersLoaded := TRequestFilter.LoadList(fileName, FFilters);

If Not filtersLoaded Then
  begin
  fileName := ExtractFilePath(Application.ExeName) + 'filters.ini';
  filtersLoaded := TRequestFilter.LoadList(fileName, FFilters);
  If Not FiltersLoaded Then
    FFilters.Clear;
  end;

RequestListView.DoubleBuffered := True;
{$IFNDEF FPC}
FAppEvents := TApplicationEvents.Create(Self);
FAppEvents.OnMessage := IrpMonAppEventsMessage;
FRequestMsgCode := RegisterWindowMessage('IRPMON');
If FRequestMsgCode = 0 Then
  begin
  WinErrorMessage('Failed to register internal message', GetLastError);
  Exit;
  end;
{$ELSE}
FAppEvents := TApplicationProperties.Create(Self);
{$ENDIF}
FAppEvents.OnException := IrpMonAppEventsException;
FHookedDrivers := TDictionary<Pointer, TDriverHookObject>.Create;
FHookedDevices := TDictionary<Pointer, TDeviceHookObject>.Create;
FHookedDeviceDriverMap := TDictionary<Pointer, Pointer>.Create;
FModel := TRequestListModel.Create;
FModel.OnRequestProcessed := OnRequestProcessed;
FModel.ColumnUpdateBegin;
FModel.
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctId), Ord(rlmctId), False, 60).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctTime), Ord(rlmctTime)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctThreadId), Ord(rlmctThreadId), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctProcessId), Ord(rlmctProcessId), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctProcessName), Ord(rlmctProcessName)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctIRQL), Ord(rlmctIRQL)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctRequestType), Ord(rlmctRequestType)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctDeviceObject), Ord(rlmctDeviceObject)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctDeviceName), Ord(rlmctDeviceName), True).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctDriverObject), Ord(rlmctDriverObject)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctDriverName), Ord(rlmctDriverName), True).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctResultValue), Ord(rlmctResultValue), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctResultConstant), Ord(rlmctResultConstant), True).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctIRPAddress), Ord(rlmctIRPAddress), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctIOSBStatusValue), Ord(rlmctIOSBStatusValue), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctIOSBStatusConstant), Ord(rlmctIOSBStatusConstant), True).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctIOSBInformation), Ord(rlmctIOSBInformation), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctSubType), Ord(rlmctSubType), False, 100).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctMinorFunction), Ord(rlmctMinorFunction), False, 100).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctFileObject), Ord(rlmctFileObject), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctFileName), Ord(rlmctFileName), True).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctIRPFlags), Ord(rlmctIRPFlags), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctArg1), Ord(rlmctArg1), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctArg2), Ord(rlmctArg2), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctArg3), Ord(rlmctArg3), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctArg4), Ord(rlmctArg4), False, 75).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctPreviousMode), Ord(rlmctPreviousMode)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctRequestorMode), Ord(rlmctRequestorMode)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctRequestorPID), Ord(rlmctRequestorPID)).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctDataAssociated), Ord(rlmctDataAssociated), False, 50).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctEmulated), Ord(rlmctEmulated), False, 50).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctDataSize), Ord(rlmctDataSize), False, 50).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctAdmin), Ord(rlmctAdmin), False, 50).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctImpersonated), Ord(rlmctImpersonated), False, 50).
    ColumnAdd(TDriverRequest.GetBaseColumnName(rlmctImpersonatedAdmin), Ord(rlmctImpersonatedAdmin), False, 50);


FModel.ColumnUpdateEnd;
FModel.CreateColumnsMenu(ColumnsMenuItem);
FModel.SetDisplayer(RequestListView);
If IRPMonDllInitialized Then
  begin
  EnumerateClassWatches;
  EnumerateDriverNameWatches;
  end
Else begin
  SelectDriversDevicesMenuItem.Enabled := False;
  WatchClassMenuItem.Enabled := False;
  WatchDriverNameMenuItem.Enabled := False;

  CaptureEventsMenuItem.Enabled := False;
  RefreshNameCacheMenuItem.Enabled := False;
  end;

FParsers := TObjectList<TDataParser>.Create;
DataPrasersLoad(ExtractFileDir(Application.ExeName), FParsers);
FModel.Parsers := FParsers;
ReadSettings;
end;

Procedure TMainFrm.HideExcludedRequestsMenuItemClick(Sender: TObject);
Var
  M : TMenuItem;
begin
M := Sender As TMenuItem;
M.Checked := Not M.Checked;
FModel.FilterDisplayOnly := M.Checked;
end;

Procedure TMainFrm.IgnoreLogFileHeadersMenuItemClick(Sender: TObject);
Var
  M : TMenuItem;
begin
M := Sender As TMenuItem;
M.Checked := Not M.Checked;
end;

Procedure TMainFrm.IrpMonAppEventsException(Sender: TObject; E: Exception);
begin
ErrorMessage(E.Message);
end;

Procedure TMainFrm.OnRequest(AList:TList<PREQUEST_GENERAL>);
Var
  rq : PREQUEST_GENERAL;
begin
FModel.UpdateRequest := AList;
FModel.Update;
For rq In AList Do
  FreeMem(rq);

AList.Free;
end;

Procedure TMainFrm.IrpMonAppEventsMessage(var Msg: tagMSG;
  Var Handled: Boolean);
begin
If Msg.message = FRequestMsgCode Then
  begin
  OnRequest(TList<PREQUEST_GENERAL>(Msg.lParam));
  Handled := True;
  end;
end;

Procedure TMainFrm.AboutMenuItemClick(Sender: TObject);
begin
With TAboutBox.Create(Self) Do
  begin
  ShowModal;
  Free;
  end;
end;

Procedure TMainFrm.CaptureEventsMenuItemClick(Sender: TObject);
Var
  connect : Boolean;
  M : TMenuItem;
begin
M := Sender As TMenuItem;
connect := Not M.Checked;
If connect Then
  begin
  FRequestThread := TRequestTHread.Create(False, FRequestMsgCode);
  M.Checked := connect;
  end
Else begin
  FRequestThread.SignalTerminate;
  FRequestThread.WaitFor;
  FreeAndNil(FRequestTHread);
  M.Checked := connect;
  end;
end;

Procedure TMainFrm.ClearMenuItemClick(Sender: TObject);
begin
FModel.Clear;
end;

Procedure TMainFrm.CompressMenuItemClick(Sender: TObject);
Var
  M : TMenuItem;
begin
M := Sender As TMenuItem;
M.Checked := Not M.Checked;
end;

Procedure TMainFrm.CopyVisibleColumnsMenuItemClick(Sender: TObject);
Var
  t : WideString;
  value : WideString;
  selectedIndex : Integer;
  visibleOnly : Boolean;
  c : TListModelColumn;
  I : Integer;
begin
t := '';
selectedIndex := FModel.SelectedIndex;
visibleOnly := (Sender = CopyVisibleColumnsMenuItem);
For I := 0 To FModel.ColumnCount - 1 Do
  begin
  c := FModel.Columns[I];
  If (Not visibleOnly) Or (c.Visible) Then
    begin
    value := FModel.Item(selectedIndex, I);
    If value <> '' Then
      t := t + value + #9;
    end;
  end;

If t <> '' Then
  Delete(t, Length(t), 1);

Clipboard.AsText := t;
end;

Procedure TMainFrm.DataParsersListViewData(Sender: TObject; Item: TListItem);
Var
  dp : TDataParser;
begin
With Item Do
  begin
  dp := FParsers[Index];
  Caption := Format('%d', [UInt64(dp.Priority)]);
  SubItems.Add(dp.Name);
  SubItems.Add(Format('%d.%d.%d', [dp.MajorVersion, dp.MinorVersion, dp.BuildNumber]));
  SubItems.Add(dp.Description);
  SubItems.Add(dp.LibraryName);
  end;
end;

Procedure TMainFrm.DataParsersTabSheetShow(Sender: TObject);
begin
DataParsersListView.Items.Count := FParsers.Count;
end;

Procedure TMainFrm.Documentation1Click(Sender: TObject);
Var
  appDirectory : WideString;
begin
appDirectory := ExtractFileDir(Application.ExeName);
ShellExecuteW(0, 'open', 'https://github.com/MartinDrab/IRPMon/wiki', '', PWideChar(AppDirectory), SW_NORMAL);
end;

Procedure TMainFrm.DriverMenuItemClick(Sender: TObject);
Var
  M : TMenuItem;
begin
M := Sender As TMenuItem;
M.Checked := Not M.Checked;
end;

Procedure TMainFrm.DriverMenuItemExpand(Sender: TObject);
Var
  err : Cardinal;
  settings : IRPMNDRV_SETTINGS;
begin
err := IRPMonDllSettingsQuery(settings);
ReqQueueClearOnDisconnectMenuItem.Enabled := (err = ERROR_SUCCESS);
ReqQueueCollectWhenDisconnectedMenuItem.Enabled := (err = ERROR_SUCCESS);
ProcessEventsCollectMenuItem.Enabled := (err = ERROR_SUCCESS);
FileObjectEventsCollectMenuItem.Enabled := (err = ERROR_SUCCESS);
DriverSnapshotEventsCollectMenuItem.Enabled := (err = ERROR_SUCCESS);
ProcessEmulateOnConnectMenuItem.Enabled := (err = ERROR_SUCCESS);
DriverSnapshotOnConnectMenuItem.Enabled := (err = ERROR_SUCCESS);
If err = ERROR_SUCCESS Then
  begin
  ReqQueueClearOnDisconnectMenuItem.Checked := settings.ReqQueueClearOnDisconnect;
  ReqQueueCollectWhenDisconnectedMenuItem.Checked := settings.ReqQueueCollectWhenDisconnected;
  ProcessEventsCollectMenuItem.Checked := settings.ProcessEventsCollect;
  FileObjectEventsCollectMenuItem.Checked := settings.FileObjectEventsCollect;
  DriverSnapshotEventsCollectMenuItem.Checked := settings.DriverSnapshotEventsCollect;
  ProcessEmulateOnConnectMenuItem.Checked := settings.ProcessEmulateOnConnect;
  DriverSnapshotOnConnectMenuItem.Checked := settings.DriverSnapshotOnConnect;
  StripRequestDataMenuItem.Checked := settings.StripData;
  MaxRequestDataSizeMenuItem.Caption := Format('Max request data size: %u', [settings.DataStripThreshold]);
  end;
end;

Procedure TMainFrm.DriverSettingsMenuItemClick(Sender: TObject);
Var
  M : TMenuItem;
  err : Cardinal;
  pv : ^ByteBool;
  puv : PCardinal;
  puvStr : WideString;
  tmp : Cardinal;
  settings : IRPMNDRV_SETTINGS;
begin
M := Sender As TMenuItem;
err := IRPMonDllSettingsQuery(settings);
If err = ERROR_SUCCESS Then
  begin
  pv := Nil;
  puv := Nil;
  Case M.Tag Of
    0 : pv := @settings.ReqQueueClearOnDisconnect;
    1 : pv := @settings.ReqQueueCollectWhenDisconnected;
    2 : pv := @settings.ProcessEventsCollect;
    3 : pv := @settings.FileObjectEventsCollect;
    4 : pv := @settings.DriverSnapshotEventsCollect;
    5 : pv := @settings.ProcessEmulateOnConnect;
    6 : pv := @settings.DriverSnapshotOnConnect;
    7 : pv := @settings.StripData;
    8 : puv := @settings.DataStripThreshold;
    end;

  If Assigned(puv) Then
    begin
    puvStr := InputBox(M.Caption, '', IntToStr(Int64(puv^)));
    If puvStr <> '' Then
      begin
      Try
        tmp := StrToInt64(puvStr);
        puv^ := tmp;
      Except
        end;
      end;
    end
  Else If Assigned(pv) Then
    begin
    M.Checked := Not M.Checked;
    pv^ := M.Checked;
    end;

  err := IRPMonDllSettingsSet(settings, True);
  If err <> ERROR_SUCCESS THen
    WinErrorMessage('Unable to change the driver settings', err);
  end;
end;

Procedure TMainFrm.RefreshNameCacheMenuItemClick(Sender: TObject);
Var
  err : Cardinal;
begin
err := FModel.RefreshMaps;
If err <> ERROR_SUCCESS Then
  WinErrorMessage('Unable to refresh name cache', err);
end;

Procedure TMainFrm.RequestDetailsMenuItemClick(Sender: TObject);
Var
  rq : TDriverRequest;
begin
rq := FModel.Selected;
If Assigned(rq) Then
  begin
  With TRequestDetailsFrm.Create(Self, rq, FParsers) Do
    begin
    ShowModal;
    Free;
    end;
  end
Else begin
  If Sender <> RequestListView Then
    WarningMessage('No request selected');
  end;
end;

Procedure TMainFrm.RequestPopupMenuPopup(Sender: TObject);
Var
  value : WideString;
  w : Integer;
  c : TListModelColumn;
  I : Integer;
  p : TPoint;
  clientP : TPoint;
  selectedIndex : Integer;
  columnFound : Boolean;
  requestTypeString : WideString;
begin
w := 0;
value := '';
columnFound := False;
GetCursorPos(p);
clientP := RequestListView.ScreenToClient(p);
selectedIndex := FModel.SelectedIndex;
If selectedIndex <> -1 Then
  begin
  For I := 0 To FModel.ColumnCount - 1 Do
    begin
    c := FModel.Columns[I];
    If c.Visible Then
      begin
      Inc(w, c.Width);
      If clientP.X <= w Then
        begin
        requestTypeString := TDriverRequest.RequestTypeToString(FModel.Items[selectedIndex].RequestType);
        value := FModel.Item(selectedIndex, I);
        RPIncludeMenuItem.Caption := Format('Include "%s" (%s)', [value, requestTypeString]);
        RPHighlightMenuItem.Caption := Format('Highlight "%s" (%s)...', [value, requestTypeString]);
        RPExcludeMenuItem.Caption := Format('Exclude "%s" (%s)', [value, requestTypeString]);

        RPIncludeAllMenuItem.Caption := Format('Include "%s" (all requests)', [value]);
        RPHighlightAllMenuItem.Caption := Format('Highlight "%s" (all requests)...', [value]);
        RPExcludeAllMenuItem.Caption := Format('Exclude "%s" (all requests)', [value]);

        CopyMenuItem.Caption := Format('Copy "%s"', [value]);
        RPHighlightMenuItem.Tag := c.Tag;
        RPIncludeMenuItem.Tag := c.Tag;
        RPExcludeMenuItem.Tag := c.Tag;
        RPHighlightAllMenuItem.Tag := c.Tag;
        RPIncludeAllMenuItem.Tag := c.Tag;
        RPExcludeAllMenuItem.Tag := c.Tag;
        CopyMenuItem.Tag := c.Tag;        RequestPopupMenu.Tag := I;
        columnFound := True;
        Break;
        end;
      end;
    end;
  end;

RequestDetailsMenuItem.Enabled := (selectedIndex <> -1);
RPIncludeMenuItem.Enabled := columnFound;
RPHighlightMenuItem.Enabled := columnFound;
RPExcludeMenuItem.Enabled := columnFound;
RPIncludeAllMenuItem.Enabled := columnFound;
RPHighlightAllMenuItem.Enabled := columnFound;
RPExcludeAllMenuItem.Enabled := columnFound;
CopyMenuItem.Enabled := columnFound;
CopyVisibleColumnsMenuItem.Enabled := (selectedIndex <> -1);
CopyWholeLineMenuItem.Enabled := (selectedIndex <> -1);
end;

Procedure TMainFrm.RPDetailsMenuItemClick(Sender: TObject);
begin
RequestDetailsMenuItemClick(RequestDetailsMenuItem);
end;

Procedure TMainFrm.SaveMenuItemClick(Sender: TObject);
Var
  fn : WideString;
begin
If LogSaveDialog.Execute Then
  begin
  fn := LogSaveDialog.FileName;
  Case LogSaveDialog.FilterIndex Of
    1 : fn := ChangeFIleExt(fn, '.log');
    2 : fn := ChangeFIleExt(fn, '.bin');
    end;

  FModel.SaveToFile(fn, LogSaveDialog.FilterIndex = 2, CompressMenuItem.Checked);
  end;
end;

Procedure TMainFrm.SelectDriversDevicesMenuItemClick(Sender: TObject);
begin
With TTreeFrm.Create(Self) Do
  begin
  ShowModal;
  Free;
  end;

EnumerateHooks;
end;

Procedure TMainFrm.StatusTimerTimer(Sender: TObject);
Var
  err : Cardinal;
  settings : IRPMNDRV_SETTINGS;
  statusText : WideString;
begin
err := ERROR_SUCCESS;
If ConnectorType <> ictNone Then
  err := IRPMonDllSettingsQuery(settings);

If err = ERROR_SUCCESS Then
  begin
  If ConnectorType = ictNone Then
    statusText := 'Not connected | Requests: '
  Else begin
    If settings.ReqQueueConnected Then
      statusText := 'Monitoring | Requests: '
    Else statusText := 'Not monitoring | Requests: ';

    If settings.ReqQueueConnected Then
      statusText := statusText + Format('%u queued, (%u paged, %u nonpaged) ', [settings.ReqQueueLength, settings.ReqQueuePagedLength, settings.ReqQueueNonPagedLength]);
    end;

  statusText := statusText + Format('%u displayed, ', [FModel.RowCount]);
  statusText := statusText + Format('%u total', [FModel.TotalCount]);
  end
Else statusText := Format('Unable to get driver information: %s (%u)', [SysErrorMessage(err), err]);

StatusBar1.SimpleText := statusText;
end;

Procedure TMainFrm.WatchClassMenuItemClick(Sender: TObject);
Var
  M : TMenuItem;
  err : Cardinal;
  mCaption : WideString;
begin
With TClassWatchAddFrm.Create(Self) Do
  begin
  ShowModal;
  If Not Cancelled Then
    begin
    err := SelectedClass.Register(Beginning);
    If err = ERROR_SUCCESS Then
      begin
      M := TMenuItem.Create(WatchedClassesMenuItem);
      mCaption := SelectedClass.Name + ' (';
      If SelectedClass.UpperFIlter Then
        mCaption := mCaption + 'upper, '
      Else mCaption := mCaption + 'lower, ';

      If SelectedClass.Beginning Then
        mCaption := mCaption + 'first)'
      Else mCaption := mCaption + 'last)';

      M.Caption := mCaption;
      M.Tag := NativeInt(SelectedClass);
      M.OnClick := OnWatchedClassClick;
      WatchedClassesMenuItem.Add(M);
      WatchedClassesMenuItem.Visible := True;
      WatchedClassesMenuItem.Enabled := True;
      end
    Else WinErrorMessage('Failed to watch the class', err);

    If err <> ERROR_SUCCESS Then
      SelectedClass.Free;
    end;

  Free;
  end;
end;


Procedure TMainFrm.WatchDriverNameMenuItemClick(Sender: TObject);
Var
  M : TMenuItem;
  dn : TWatchableDriverName;
  err : Cardinal;
  ds : DRIVER_MONITOR_SETTINGS;
begin
With TDriverNameWatchAddFrm.Create(Self) DO
  begin
  ShowModal;
  If Not Cancelled Then
    begin
    ds := DriverSettings;
    dn := TWatchableDriverName.Create(DriverName, ds);
    err := dn.Register;
    If err = ERROR_SUCCESS Then
      begin
      M := TMenuItem.Create(WatchedClassesMenuItem);
      M.Tag := NativeInt(dn);
      M.Caption := dn.Name;
      M.OnClick := OnWatchedDriverNameClick;
      WatchedDriversMenuItem.Add(M);
      WatchedDriversMenuItem.Visible := True;
      WatchedDriversMenuItem.Enabled := True;
      end
    Else WinErrorMessage('Failed to watch for the driver name', err);

    If err <> ERROR_SUCCESS Then
      dn.Free;
    end;

  Free;
  end;
end;

Procedure TMainFrm.OnWatchedClassClick(Sender:TObject);
Var
  err : Cardinal;
  wc : TWatchableClass;
  M : TMenuItem;
begin
M := (Sender As TMenuItem);
wc := TWatchableClass(M.Tag);
err := wc.Unregister;
If err = ERROR_SUCCESS Then
  begin
  wc.Free;
  M.Free;
  If WatchedClassesMenuItem.Count = 0 Then
    begin
    WatchedClassesMenuItem.Visible := False;
    WatchedClassesMenuItem.Enabled := False;
    end;
  end
Else WinErrorMessage('Failed to unregister the watched class', err);
end;

Procedure TMainFrm.EnumerateClassWatches;
Var
  M : TMenuItem;
  mCaption : WideString;
  err : Cardinal;
  wc : TWatchableClass;
  wl : TList<TWatchableClass>;
begin
wl := TList<TWatchableClass>.Create;
err := TWatchableClass.Enumerate(wl);
If err = ERROR_SUCCESS Then
  begin
  For wc In wl DO
    begin
    If wc.Registered Then
      begin
      M := TMenuItem.Create(WatchedClassesMenuItem);
      mCaption := wc.Name + ' (';
      If wc.UpperFIlter Then
        mCaption := mCaption + 'upper, '
      Else mCaption := mCaption + 'lower, ';

      If wc.Beginning Then
        mCaption := mCaption + 'first)'
      Else mCaption := mCaption + 'last)';

      M.Caption := mCaption;
      M.Tag := NativeInt(wc);
      M.OnClick := OnWatchedClassClick;
      WatchedClassesMenuItem.Add(M);
      WatchedClassesMenuItem.Visible := True;
      WatchedClassesMenuItem.Enabled := True;
      end
    Else wc.Free;
    end;
  end
Else WinErrorMessage('Failed to enumerate watched classes', err);

wl.Free;
end;

Procedure TMainFrm.OnWatchedDriverNameClick(Sender:TObject);
Var
  err : Cardinal;
  M : TMenuItem;
  dn : TWatchableDriverName;
begin
M := (Sender As TMenuItem);
dn := TWatchableDriverName(M.Tag);
err := dn.Unregister;
If err = ERROR_SUCCESS Then
  begin
  dn.Free;
  M.Free;
  If WatchedDriversMenuItem.Count = 0 Then
    begin
    WatchedDriversMenuItem.Visible := False;
    WatchedDriversMenuItem.Enabled := False;
    end;
  end
Else WinErrorMessage('Failed to unregister the watched driver name', err);
end;

Procedure TMainFrm.OpenMenuItemClick(Sender: TObject);
Var
  fn : WideString;
begin
If LogOpenDialog.Execute Then
  begin
  fn := LogOpenDialog.FileName;
  If LogOpenDialog.FilterIndex = 1 Then
    fn := ChangeFileExt(fn, '.bin');

  FModel.LoadFromFile(fn, Not IgnoreLogFileHeadersMenuItem.Checked);
  end;
end;

procedure TMainFrm.PopupFilterClick(Sender: TObject);
Var
  copyText : Boolean;
  filterAction : EFilterAction;
  invalidButton : Boolean;
  rf : TRequestFilter;
  M : TMenuItem;
  value : WideString;
  intValue : UInt64;
  rq : TDriverRequest;
  d : Pointer;
  l : Cardinal;
  ret : Boolean;
  columnType : ERequestListModelColumnType;
  filterType : ERequestType;
begin
filterAction := ffaHighlight;
invalidButton := False;
copyText := False;
M := Sender As TMenuItem;
If (Sender = RPHighlightMenuItem) Or
   (Sender = RPHighlightAllMenuItem) Then
  filterAction := ffaHighlight
Else If (Sender = RPIncludeMenuItem) Or
        (Sender = RPIncludeAllMenuItem) Then
  filterAction := ffaInclude
Else If (Sender = RPExcludeMenuItem) Or
        (Sender = RPExcludeAllMenuItem) Then
  filterAction := ffaExclude
Else If Sender = CopyMenuItem Then
  copyText := True
Else invalidButton := True;

If Not invalidButton Then
  begin
  value := FModel.Item(FModel.SelectedIndex, RequestPopupMenu.Tag);
  If Not copyText Then
    begin
    rq := FModel.Selected;
    filterType := ertUndefined;
    If (Sender = RPIncludeMenuItem) Or
       (Sender = RPHighlightMenuItem) Or
       (Sender = RPExcludeMenuItem) Then
       filterType := rq.RequestType;

    rf := TRequestFilter.NewInstance(filterType);
    rf.Enabled := True;
    rf.Ephemeral := True;
    columnType := ERequestListModelColumnType(M.Tag);
    ret := rq.GetColumnValueRaw(columnType, d, l);
    If ret Then
      begin
      If RequestListModelColumnValueTypes[Ord(columnType)] <> rlmcvtString Then
        begin
        intValue := 0;
        Move(d^, intValue, l);
        ret := rf.SetCondition(columnType, rfoEquals, intValue);
        end
      Else ret := rf.SetCondition(columnType, rfoEquals, value);
      end;

    If ret Then
      begin
      If filterAction = ffaHighlight Then
        begin
        If highlightColorDialog.Execute Then
          begin
          rf.SetAction(filterAction, highlightColorDialog.Color);
          rf.GenerateName(FFilters);
          FFilters.Add(rf);
          end
        Else FreeAndNil(rf);
        end
      Else begin
        rf.SetAction(filterAction);
        rf.GenerateName(FFilters);
        FFilters.Insert(0, rf);
        end;

      If Assigned(rf) Then
        FModel.Reevaluate;
      end
    Else ErrorMessage('Unable to set filter condition');
    end
  Else Clipboard.AsText := value;
  end;
end;

Procedure TMainFrm.EnumerateDriverNameWatches;
Var
  M : TMenuItem;
  dn : TWatchableDriverName;
  err : Cardinal;
  dnl : TList<TWatchableDriverName>;
begin
dnl := TList<TWatchableDriverName>.Create;
err := TWatchableDriverName.Enumerate(dnl);
If err = ERROR_SUCCESS Then
  begin
  For dn In dnl Do
    begin
    M := TMenuItem.Create(WatchedClassesMenuItem);
    M.Tag := NativeInt(dn);
    M.Caption := dn.Name;
    M.OnClick := OnWatchedDriverNameClick;
    WatchedDriversMenuItem.Add(M);
    WatchedDriversMenuItem.Visible := True;
    WatchedDriversMenuItem.Enabled := True;
    end;
  end
Else WinErrorMessage('Failed to enumerate watched driver names', err);

dnl.Free;
end;

Procedure TMainFrm.WriteSettings;
Var
  I, J : Integer;
  c : TListModelColumn;
  iniCOlumnName : WideString;
  iniFile : TIniFile;
  settingsDir : WideString;
  settingsFile : WideString;
begin
settingsFile := ChangeFileExt(ExtractFileName(Application.ExeName), '.ini');
settingsDir := ExtractFilePath(Application.ExeName);
Try
  iniFile := TIniFile.Create(settingsDir + settingsFile);
Except
  iniFile := Nil;
  End;

If Not Assigned(iniFile) Then
  begin
  settingsDir := GetLocalSettingsDirectory;
  Try
    iniFile := TIniFile.Create(settingsDir + settingsFile);
  Except
    iniFile := Nil;
    End;
  end;

If Assigned(iniFile) Then
  begin
  iniFIle.WriteBool('Driver', 'unload_on_exit', UnloadOnExitMenuItem.Checked);
  iniFIle.WriteBool('Driver', 'uninstall_on_exit', UninstallOnExitMenuItem.Checked);
  iniFile.WriteBool('General', 'CaptureEvents', CaptureEventsMenuItem.Checked);
  iniFile.WriteBool('General', 'filter_display_only', HideExcludedRequestsMenuItem.Checked);
  iniFile.WriteBool('Log', 'compress_requests', CompressMenuItem.Checked);
  iniFile.WriteBool('Log', 'ignore_headers', IgnoreLogFileHeadersMenuItem.Checked);
  For I := 0 To FModel.ColumnCount - 1 Do
    begin
    c := FModel.Columns[I];
    iniColumnName := c.Caption;
    For J := 1 To Length(iniColumnName) Do
      begin
      If (iniColumnName[J] = ' ') Then
        iniColumnName[J] := '_';
      end;

    iniFile.WriteBool('Columns', iniColumnName, c.Visible);
    end;
  end;
end;

Procedure TMainFrm.ReadSettings;
Var
  I, J : Integer;
  c : TListModelColumn;
  iniCOlumnName : WideString;
  iniFile : TIniFile;
  settingsFile : WideString;
  settingsDir : WideString;
begin
iniFIle := Nil;
settingsFile := ChangeFileExt(ExtractFileName(Application.ExeName), '.ini');
settingsDir := GetLocalSettingsDirectory;
If Not FileExists(settingsDir + settingsFile) Then
  settingsDir := ExtractFilePath(Application.ExeName);

If FileExists(settingsDir + settingsFile) Then
  begin
  Try
    iniFile := TIniFile.Create(settingsDir + settingsFile);
    UnloadOnExitMenuItem.Checked := iniFIle.ReadBool('Driver', 'unload_on_exit', False);
    UninstallOnExitMenuItem.Checked := iniFIle.ReadBool('Driver', 'uninstall_on_exit', True);
    CompressMenuItem.Checked := iniFIle.ReadBool('Log', 'compress_requests', False);
    IgnoreLogFileHeadersMenuItem.Checked := iniFile.ReadBool('Log', 'ignore_headers', False);
    If IRPMonDllInitialized Then
      begin
      CaptureEventsMenuItem.Checked := Not iniFile.ReadBool('General', 'CaptureEvents', False);
      If Not CaptureEventsMenuItem.Checked Then
        CaptureEventsMenuItemClick(CaptureEventsMenuItem)
      Else CaptureEventsMenuItem.Checked := False;
      end;

    FModel.FilterDisplayOnly := iniFile.ReadBool('General', 'filter_display_only', False);
    HideExcludedRequestsMenuItem.Checked := FModel.FilterDisplayOnly;
    FModel.ColumnUpdateBegin;
    For I := 0 To FModel.ColumnCount - 1 Do
      begin
      c := FModel.Columns[I];
      iniColumnName := c.Caption;
      For J := 1 To Length(iniColumnName) Do
        begin
        If (iniColumnName[J] = ' ') Then
          iniColumnName[J] := '_';
        end;

      FModel.ColumnSetVisible(I, iniFile.ReadBool('Columns', iniColumnName, True));
      end;

    FModel.ColumnUpdateEnd;
  Finally
    iniFIle.Free;
    End;
  end;
end;

Function TMainFrm.GetLocalSettingsDirectory:WideString;
begin
{$IFNDEF FPC}
Result := Format('%s\IRPMon', [TPath.GetHomePath]);
{$ELSE}
Result := Format('%s\IRPMon', [GetUserDir]);
{$ENDIF}
ForceDirectories(Result);
Result := Result + '\';
end;

Procedure TMainFrm.OnRequestProcessed(ARequest:TDriverRequest; Var AStore:Boolean);
Var
  rf : TRequestFilter;
  matchingRF : TRequestFilter;
  allInclusive : Boolean;
  allExclusive : Boolean;
  noMatch : Boolean;
  action : EFilterAction;
  hColor : Cardinal;
begin
allInclusive := True;
allExclusive := True;
noMatch := True;
ARequest.Highlight := False;
AStore := (FFilters.Count = 0);
For rf In FFilters Do
  begin
  If rf.Action = ffaInclude Then
    allExclusive := False;

  If rf.Action = ffaExclude Then
    allInclusive := False;

  matchingRF := rf.Match(ARequest, action, hColor);
  If Assigned(matchingRF) Then
    begin
    If (noMatch) And ((action = ffaInclude) Or (action = ffaExclude)) Then
      begin
      noMatch := False;
      AStore := (action = ffaInclude);
      end;

    If (action = ffaInclude) Or (action = ffaHighlight) Then
      begin
      ARequest.Highlight := (hColor <> $FFFFFF);
      ARequest.HighlightColor := hColor;
      end;
    end;
  end;

If noMatch Then
  begin
  If allInclusive Then
    AStore := False;

  If allExclusive Then
    AStore := True;
  end;
end;


End.

