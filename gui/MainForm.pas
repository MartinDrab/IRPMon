Unit MainForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ComCtrls, Vcl.Menus,
  Generics.Collections,
  IRPMonDll, RequestListModel, Vcl.ExtCtrls;

Type
  TMainFrm = Class (TForm)
    MainMenu1: TMainMenu;
    ActionMenuItem: TMenuItem;
    HookDriverMenuItem: TMenuItem;
    HookDeviceNameMenuItem: TMenuItem;
    HookDeviceAddressMenuItem: TMenuItem;
    TreeMenuItem: TMenuItem;
    N1: TMenuItem;
    UnhookDriverMenuItem: TMenuItem;
    UnhookDeviceMenuItem: TMenuItem;
    N2: TMenuItem;
    ExitMenuItem: TMenuItem;
    N3: TMenuItem;
    ViewIRPMenuItem: TMenuItem;
    ViewFastIoMenuItem: TMenuItem;
    ViewIRPCompleteMenuItem: TMenuItem;
    ViewAddDeviceMenuItem: TMenuItem;
    ViewUnloadMenuItem: TMenuItem;
    ViewStartIoMenuItem: TMenuItem;
    MonitoringMenuItem: TMenuItem;
    ClearMenuItem: TMenuItem;
    N4: TMenuItem;
    StartMenuItem: TMenuItem;
    StopMenuItem: TMenuItem;
    HelpMenuItem: TMenuItem;
    AboutMenuItem: TMenuItem;
    N5: TMenuItem;
    SaveMenuItem: TMenuItem;
    ColumnsMenuItem: TMenuItem;
    PageControl1: TPageControl;
    RequestTabSheet: TTabSheet;
    Hooks: TTabSheet;
    HookDriverNDMenuItem: TMenuItem;
    RequestListView: TListView;
    Timer1: TTimer;
    CaptureEventsMenuItem: TMenuItem;
    MonitorMenuItem: TMenuItem;
    N6: TMenuItem;
    Procedure HookDriverMenuItemClick(Sender: TObject);
    Procedure MonitoringMenuItemClick(Sender: TObject);
    Procedure ViewMenuItemClick(Sender: TObject);
    Procedure ClearMenuItemClick(Sender: TObject);
    procedure UnhookDriverMenuItemClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
    procedure CaptureEventsMenuItemClick(Sender: TObject);
    procedure ExitMenuItemClick(Sender: TObject);
    procedure HookDeviceNameMenuItemClick(Sender: TObject);
  Private
    FModel : TRequestListModel;
    FMS : DRIVER_MONITOR_SETTINGS;
    FHookedDrivers : TDictionary<THandle, WideString>;
    Procedure DriverMonitorSettingsFromGUI;
  end;

Var
  MainFrm: TMainFrm;

Implementation

{$R *.DFM}

Uses
  Utils;

Procedure TMainFrm.DriverMonitorSettingsFromGUI;
begin
FMS.MonitorAddDevice := ViewAddDeviceMenuItem.Checked;
FMS.MonitorStartIo := ViewStartIoMenuItem.Checked;
FMS.MonitorUnload := ViewUnloadMenuItem.Checked;
FMS.MonitorFastIo := ViewFastIoMenuItem.Checked;
FMS.MonitorIRP := ViewIRPMenuItem.Checked;
FMS.MonitorIRPCompletion := ViewIRPCompleteMenuItem.Checked;
end;

Procedure TMainFrm.ExitMenuItemClick(Sender: TObject);
begin
Close;
end;

Procedure TMainFrm.FormCreate(Sender: TObject);
begin
FHookedDrivers := TDictionary<THandle, WideString>.Create;
FModel := TRequestListModel.Create;
FModel.ColumnUpdateBegin;
FModel.
    ColumnAdd('Time', Ord(rlmctTime)).
    ColumnAdd('TID', Ord(rlmctThreadId), False, 75).
    ColumnAdd('PID', Ord(rlmctProcessId), False, 75).
    ColumnAdd('IRQL', Ord(rlmctIRQL)).
    ColumnAdd('Type', Ord(rlmctRequestType)).
    ColumnAdd('Device', Ord(rlmctDeviceObject)).
    ColumnAdd('Device name', Ord(rlmctDeviceName), True).
    ColumnAdd('Driver', Ord(rlmctDriverObject)).
    ColumnAdd('Driver name', Ord(rlmctDriverName), True).
    ColumnAdd('Result', Ord(rlmctResult), False, 75).
    ColumnAdd('IRP', Ord(rlmctIRPAddress), False, 75).
    ColumnAdd('Status', Ord(rlmctIOSBStatus), False, 75).
    ColumnAdd('Information', Ord(rlmctIOSBInformation), False, 75).
    ColumnAdd('SubType', Ord(rlmctSubType), False, 100).
    ColumnAdd('File object', Ord(rlmctFileObject), False, 75).
    ColumnAdd('IRP Flags', Ord(rlmctIRPFlags), False, 75).
    ColumnAdd('Arg1', Ord(rlmctArg1), False, 75).
    ColumnAdd('Arg2', Ord(rlmctArg2), False, 75).
    ColumnAdd('Arg3', Ord(rlmctArg3), False, 75).
    ColumnAdd('Arg4', Ord(rlmctArg4), False, 75).
    ColumnAdd('P.Mode', Ord(rlmctPreviousMode)).
    ColumnAdd('R.Mode', Ord(rlmctRequestorMode));
FModel.ColumnUpdateEnd;
FModel.CreateColumnsMenu(ColumnsMenuItem);
FModel.SetDisplayer(RequestListView);
DriverMonitorSettingsFromGUI;
end;

Procedure TMainFrm.CaptureEventsMenuItemClick(Sender: TObject);
Var
  err : Cardinal;
  connect : Boolean;
  M : TMenuItem;
begin
M := Sender As TMenuItem;
connect := Not M.Checked;
If connect Then
  begin
  err := IRPMonDllConnect(0);
  Timer1.Enabled := (err = ERROR_SUCCESS);
  If err <> ERROR_SUCCESS Then
    WinErrorMessage('Unable to capture events', err);
  end
Else begin
  err := IRPMonDllDisconnect;
  Timer1.Enabled := Not (err = ERROR_SUCCESS);
  If err <> ERROR_SUCCESS Then
    WinErrorMessage('Unable to stop capturing events', err);
  end;

If err = ERROR_SUCCESS Then
  M.Checked := connect;
end;

Procedure TMainFrm.ClearMenuItemClick(Sender: TObject);
begin
FModel.Clear;
end;

Procedure TMainFrm.HookDeviceNameMenuItemClick(Sender: TObject);
Var
  err : Cardinal;
  deviceName : WideString;
  deviceHandle : THandle;
  strValue : WideString;
  deviceAddress : Pointer;
begin
If Sender = HookDeviceNameMenuItem Then
  begin
  deviceName := InputBox('Device Name', 'Enter name of a device to monitor', '\Device\');
  If deviceName <> '' Then
    begin
    err := IRPMonDllHookDeviceByName(PWideChar(deviceName), deviceHandle);
    If err = ERROR_SUCCESS Then
    Else WinErrorMessage('Unable to hook the given device', err);
    end;
  end
Else If Sender = HookDeviceAddressMenuItem Then
  begin
  strValue := InputBox('Device Address', 'Enter address of a device object to monitor', '\Device\');
  If strValue <> '' Then
    begin
    deviceAddress := Pointer(StrToInt64(strValue));
    err := IRPMonDllHookDeviceByAddress(deviceAddress, deviceHandle);
    If err = ERROR_SUCCESS Then
    Else WinErrorMessage('Unable to hook the given device', err);
    end;
  end;
end;

Procedure TMainFrm.HookDriverMenuItemClick(Sender: TObject);
Var
  err : Cardinal;
  driverHandle : THandle;
  driverName : WideString;
  M : TMenuItem;
begin
driverName := InputBox('Driver Name', 'Enter name of the driver object to hook', '\Driver\');
If driverName <> '' Then
  begin
  FMS.MonitorNewDevices := (Sender <> HookDriverMenuItem);
  err := IRPMonDllHookDriver(PWideChar(driverName), FMS, driverHandle);
  If err = ERROR_SUCCESS Then
    begin
    FHookedDrivers.Add(driverHandle, driverName);
    M := TMenuItem.Create(UnhookDriverMenuItem);
    M.Tag := driverHandle;
    M.Caption := driverName;
    M.OnClick := UnhookDriverMenuItemClick;
    UnhookDriverMenuItem.Add(M);
    end
  Else Utils.WinErrorMessage('Unable to hook the dríver', err);
  end;
end;

Procedure TMainFrm.MonitoringMenuItemClick(Sender: TObject);
Var
  err : Cardinal;
  p : TPair<THandle, WideString>;
begin
If Sender = StartMenuItem Then
  begin
  For p In FHookedDrivers Do
    begin
    err := IRPMonDllDriverStartMonitoring(p.Key);
    If err <> ERROR_SUCCESS Then
      WinErrorMessage('Failed to start monitoring ' + p.Value, err);
    end;
  end
Else If Sender = StopMenuItem Then
  begin
  For p In FHookedDrivers Do
    begin
    err := IRPMonDllDriverStopMonitoring(p.Key);
    If err <> ERROR_SUCCESS Then
      WinErrorMessage('Failed to stop monitoring ' + p.Value, err);
    end;
  end;
end;

Procedure TMainFrm.Timer1Timer(Sender: TObject);
Var
  err : Cardinal;
begin
err := FModel.Update;
If err <> ERROR_SUCCESS Then
  begin
  IRPMonDllDisconnect;
  Timer1.Enabled := False;
  CaptureEventsMenuItem.Checked := False;
  WinErrorMessage('Unable to obtain data from the driver', err);
  end;
end;

Procedure TMainFrm.UnhookDriverMenuItemClick(Sender: TObject);
Var
  err : Cardinal;
  p : TPair<THandle, WideString>;
  child : TMenuItem;
  M : TMenuItem;
begin
M := Sender As TMenuItem;
If M.Parent = UnhookDriverMenuItem Then
  begin
  err := IRPMonDllUnhookDriver(M.Tag);
  If err = ERROR_SUCCESS Then
    begin
    FHookedDrivers.Remove(M.Tag);
    M.Free;
    end
  Else WinErrorMessage('Unable to unhook the driver', err);
  end;
end;

Procedure TMainFrm.ViewMenuItemClick(Sender: TObject);
Var
  M : TMenuItem;
begin
M := Sender As TMenuItem;
M.Checked := Not M.Checked;
DriverMonitorSettingsFromGUI;
end;



End.

