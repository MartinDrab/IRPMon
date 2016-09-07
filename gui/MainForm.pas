Unit MainForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ComCtrls, Vcl.Menus,
  Generics.Collections,
  IRPMonDll, RequestListModel, Vcl.ExtCtrls,
  HookObjects, RequestThread, Vcl.AppEvnts;

Type
  TMainFrm = Class (TForm)
    MainMenu1: TMainMenu;
    ActionMenuItem: TMenuItem;
    TreeMenuItem: TMenuItem;
    ExitMenuItem: TMenuItem;
    MonitoringMenuItem: TMenuItem;
    ClearMenuItem: TMenuItem;
    HelpMenuItem: TMenuItem;
    AboutMenuItem: TMenuItem;
    N5: TMenuItem;
    ColumnsMenuItem: TMenuItem;
    PageControl1: TPageControl;
    RequestTabSheet: TTabSheet;
    Hooks: TTabSheet;
    RequestListView: TListView;
    CaptureEventsMenuItem: TMenuItem;
    N6: TMenuItem;
    RefreshNameCacheMenuItem: TMenuItem;
    IrpMonAppEvents: TApplicationEvents;
    RequestMenuItem: TMenuItem;
    RequestDetailsMenuItem: TMenuItem;
    SaveMenuItem: TMenuItem;
    LogSaveDialog: TSaveDialog;
    N1: TMenuItem;
    Procedure ClearMenuItemClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure CaptureEventsMenuItemClick(Sender: TObject);
    procedure ExitMenuItemClick(Sender: TObject);
    procedure RefreshNameCacheMenuItemClick(Sender: TObject);
    procedure TreeMenuItemClick(Sender: TObject);
    procedure IrpMonAppEventsMessage(var Msg: tagMSG; var Handled: Boolean);
    procedure IrpMonAppEventsException(Sender: TObject; E: Exception);
    procedure RequestDetailsMenuItemClick(Sender: TObject);
    procedure AboutMenuItemClick(Sender: TObject);
    procedure SaveMenuItemClick(Sender: TObject);
  Private
    FModel : TRequestListModel;
    FHookedDrivers : TDictionary<Pointer, TDriverHookObject>;
    FHookedDevices : TDictionary<Pointer, TDeviceHookObject>;
    FHookedDeviceDriverMap : TDictionary<Pointer, Pointer>;
    FRequestTHread : TRequestThread;
    FRequestMsgCode : Cardinal;
    Procedure EnumerateHooks;
  end;

Var
  MainFrm: TMainFrm;

Implementation

{$R *.DFM}

Uses
  Utils, TreeForm, RequestDetailsForm, AboutForm;



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

Procedure TMainFrm.FormCreate(Sender: TObject);
begin
FRequestMsgCode := RegisterWindowMessage('IRPMON');
If FRequestMsgCode = 0 Then
  begin
  WinErrorMessage('Failed to register internal message', GetLastError);
  Exit;
  end;

FHookedDrivers := TDictionary<Pointer, TDriverHookObject>.Create;
FHookedDevices := TDictionary<Pointer, TDeviceHookObject>.Create;
FHookedDeviceDriverMap := TDictionary<Pointer, Pointer>.Create;
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
end;

Procedure TMainFrm.IrpMonAppEventsException(Sender: TObject; E: Exception);
begin
ErrorMessage(E.Message);
end;

Procedure TMainFrm.IrpMonAppEventsMessage(var Msg: tagMSG;
  Var Handled: Boolean);
Var
  rq : PREQUEST_GENERAL;
begin
If Msg.message = FRequestMsgCode Then
  begin
  rq := PREQUEST_GENERAL(Msg.lParam);
  FModel.UpdateRequest := rq;
  FModel.Update;
  FreeMem(rq);
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
  With TRequestDetailsFrm.Create(Self, rq) Do
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

Procedure TMainFrm.SaveMenuItemClick(Sender: TObject);
Var
  fn : WideString;
begin
If LogSaveDialog.Execute Then
  begin
  fn := LogSaveDialog.FileName;
  If LogSaveDialog.FilterIndex = 0 Then
    fn := ChangeFIleExt(fn, '.log');

  FModel.SaveToFile(fn);
  end;
end;

Procedure TMainFrm.TreeMenuItemClick(Sender: TObject);
begin
With TTreeFrm.Create(Self) Do
  begin
  ShowModal;
  Free;
  end;

EnumerateHooks;
end;



End.

