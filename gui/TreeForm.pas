Unit TreeForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics,
  Controls, Forms, Dialogs, ComCtrls, StdCtrls,
  CheckLst, Generics.Collections, IRPMonDll, Menus, ExtCtrls,
  HookObjects;

Type

  TTreeFrm = class(TForm)
    DeviceTreeView: TTreeView;
    IRPGroupBox: TGroupBox;
    FastIOGroupBox: TGroupBox;
    IRPCheckListBox: TCheckListBox;
    FastIOCheckListBox: TCheckListBox;
    DeviceTreeViewPopupMenu: TPopupMenu;
    HookedMenuItem: TMenuItem;
    N1: TMenuItem;
    NewDevicesMenuItem: TMenuItem;
    IRPMenuItem: TMenuItem;
    IRPCompleteMenuItem: TMenuItem;
    FastIOMenuItem: TMenuItem;
    StartIoMenuItem: TMenuItem;
    AddDeviceMenuItem: TMenuItem;
    DriverUnloadMenuItem: TMenuItem;
    LowerPanel: TPanel;
    StornoButton: TButton;
    OkButton: TButton;
    DataMenuItem: TMenuItem;
    DeviceExtensionMenuItem: TMenuItem;
    Procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure DeviceTreeViewPopupMenuPopup(Sender: TObject);
    procedure TreePopupMenuClick(Sender: TObject);
    procedure DeviceTreeViewAdvancedCustomDrawItem(Sender: TCustomTreeView;
      Node: TTreeNode; State: TCustomDrawState; Stage: TCustomDrawStage;
      var PaintImages, DefaultDraw: Boolean);
    procedure DeviceTreeViewChange(Sender: TObject; Node: TTreeNode);
    procedure StornoButtonClick(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
    procedure CheckListBoxClickCheck(Sender: TObject);
  Private
    FCancelled : Boolean;
    FDriverMap : TDictionary<Pointer, TDriverHookObject>;
    FDeviceMap : TDictionary<Pointer, TDeviceHookObject>;
    FCurrentlyHookedDrivers : TList<TDriverHookObject>;
    FCurrentlyHookedDevices : TList<TDeviceHookObject>;
    Procedure BuildTree;
    Procedure BuildDeviceStack(ALowest:TDeviceHookObject);
    Procedure EnumerateHooks;
  Public
    Property Cancelled : Boolean Read FCancelled;
  end;


Implementation

{$R *.dfm}

Uses
  Utils, RequestListModel, HookProgressForm,
  FastIoRequest;

Procedure TTreeFrm.BuildDeviceStack(ALowest:TDeviceHookObject);
Var
  tn : TTreeNode;
  tnName : WideString;
  I, J : Integer;
  hde : TDeviceHookObject;
  hdr : TDriverHookObject;
  driverList : TList<TDriverHookObject>;
begin
driverList := TList<TDriverHookObject>.Create;
hde := ALowest;
hdr := FDriverMap.Items[hde.Driverobject];
driverList.Add(hdr);
While FDeviceMap.TryGetValue(hde.AttachedDevice, hde) Do
  begin
  hdr := FDriverMap.Items[hde.Driverobject];
  driverList.Add(hdr);
  end;

For I := 0 To driverList.Count - 1 Do
  begin
  tn := driverList[I].TreeNode;
  hde := ALowest;
  For J := 0 To driverList.Count - 1 Do
    begin
    hdr := driverList[J];
    tnName := Format('%s (0x%p) - (%s)', [hde.Name, hde.Address, hdr.Name]);
    If J < I Then
      tnName := 'LOW: ' + tnName
    Else If J > I Then
      tnName := 'UPP: ' + tnName;

    tn := DeviceTreeView.Items.AddChild(tn, tnName);
    tn.Data := hde;
    If J = I Then
      hde.TreeNode := tn;

    If J < driverList.Count - 1 Then
      hde := FDeviceMap.Items[hde.AttachedDevice];
    end;
  end;

driverList.Free;
end;

Procedure TTreeFrm.BuildTree;
Var
  I, J : Integer;
  err : Cardinal;
  count : Cardinal;
  pdri : PPIRPMON_DRIVER_INFO;
  dri : PIRPMON_DRIVER_INFO;
  tmp : PPIRPMON_DRIVER_INFO;
  pdei : PPIRPMON_DEVICE_INFO;
  dei : PIRPMON_DEVICE_INFO;
  tnDriver : TTreeNode;
  hdr : TDriverHookObject;
  hde : TDeviceHookObject;
  p, p2 : TPair<Pointer, TDeviceHookObject>;
  isLowest : Boolean;
begin
err := IRPMonDllSnapshotRetrieve(pdri, count);
If err = ERROR_SUCCESS Then
  begin
  DeviceTreeView.Items.BeginUpdate;
  tmp := pdri;
  For I := 0 To count - 1 Do
    begin
    dri := tmp^;
    hdr := TDriverHookObject.Create(dri.DriverObject, dri.DriverName);
    tnDriver := DeviceTreeView.Items.AddCHild(Nil, hdr.Name);
    tnDriver.Data := hdr;
    hdr.TreeNode := tnDriver;
    FDriverMap.Add(dri.DriverObject, hdr);
    pdei := dri.Devices;
    For J := 0 To dri.DeviceCount - 1 Do
      begin
      dei := pdei^;
      hde := TDeviceHookObject.Create(dei.DeviceObject, dri.DriverObject, dei.AttachedDevice, dei.Name);
      FDeviceMap.Add(dei.DeviceObject, hde);
      Inc(pdei);
      end;

    Inc(tmp);
    end;

  For p In FDeviceMap Do
    begin
    isLowest := True;
    For p2 In FDevicemap Do
      begin
      If p.Key = p2.Key Then
        Continue;

      isLowest := (Not FDeviceMap.TryGetValue(p2.Value.AttachedDevice, hde)) Or (hde.Address <> p.Key);
      If Not isLowest Then
        Break
      end;

    If isLowest Then
      BuildDeviceStack(p.Value);
    end;

  DeviceTreeView.SortType := stText;
  DeviceTreeView.Items.EndUpdate;
  IRPMonDllSnapshotFree(pdri, count);
  end;

EnumerateHooks;
end;

Procedure TTreeFrm.CheckListBoxClickCheck(Sender: TObject);
Var
  dr : TDriverHookObject;
  de : TDeviceHookObject;
  tn : TTreeNode;
  I : Integer;
  chl : TCheckListBox;
begin
tn := DeviceTreeView.Selected;
If Assigned(tn) Then
  begin
  If Assigned(tn.Parent) Then
    begin
    de := tn.Data;
    chl := (Sender As TCheckListBox);
    If chl = IRPCheckListBox Then
      begin
      For I := 0 To chl.Count - 1 Do
        de.IRPSettings[I] := Ord(chl.Checked[I]);
      end
    Else If chl = FastIoCheckListBox Then
      begin
      For I := 0 To chl.Count - 1 Do
        de.FastIoSettings[I] := Ord(chl.Checked[I]);
      end;
    end
  Else begin
    dr := tn.Data;
    chl := (Sender As TCheckListBox);
    If chl = IRPCheckListBox Then
      begin
      For I := 0 To chl.Count - 1 Do
        dr.Settings.IRPSettings.Settings[I] := Ord(chl.Checked[I]);
      end
    Else If chl = FastIoCheckListBox Then
      begin
      For I := 0 To chl.Count - 1 Do
        dr.Settings.FastIoSettings.Settings[I] := Ord(chl.Checked[I]);
      end;
    end;
  end;
end;

Procedure TTreeFrm.EnumerateHooks;
Var
  cdr : TDriverHookObject;
  cde : TDeviceHookObject;
  drh : TDriverHookObject;
  deh : TDeviceHookObject;
  I, J : Integer;
  err : Cardinal;
  count : Cardinal;
  dri : PHOOKED_DRIVER_UMINFO;
  dei : PHOOKED_DEVICE_UMINFO;
  tmp : PHOOKED_DRIVER_UMINFO;
begin
err := IRPMonDllDriverHooksEnumerate(dri, count);
If err = ERROR_SUCCESS Then
  begin
  tmp := dri;
  For I := 0 To count - 1 Do
    begin
    If FDriverMap.TryGetValue(tmp.DriverObject, drh) THen
      begin
      drh.ObjectId := tmp.ObjectId;
      drh.Hooked := True;
      drh.Settings := tmp.MonitorSettings;
      drh.DeviceExtensionHook := tmp.DeviceExtensionHooks;
      cdr := TDriverHookObject.Create(drh.Address, PWideChar(drh.Name));
      cdr.Hooked := True;
      cdr.Settings := drh.Settings;
      cdr.DeviceExtensionHook := drh.DeviceExtensionHook;
      FCurrentlyHookedDrivers.Add(cdr);

      dei := tmp.HookedDevices;
      For J := 0 To tmp.NumberOfHookedDevices - 1 Do
        begin
        If FDeviceMap.TryGetValue(dei.DeviceObject, deh) Then
          begin
          deh.ObjectId := dei.ObjectId;
          deh.Hooked := True;
          deh.FastIOSettings := dei.FastIoSettings;
          deh.IRPSettings := dei.IRPSettings;

          cde := TDeviceHookObject.Create(deh.Address, deh.AttachedDevice, deh.Driverobject, PWideChar(deh.Name));
          cde.Hooked := True;
          cde.IRPSettings := deh.IRPSettings;
          cde.FastIoSettings := deh.FastIoSettings;
          FCurrentlyHookedDevices.Add(cde);
          end;

        Inc(dei);
        end;
      end;

    Inc(tmp);
    end;

  IRPMonDllDriverHooksFree(dri, count);
  end;
end;

Procedure TTreeFrm.DeviceTreeViewAdvancedCustomDrawItem(Sender: TCustomTreeView;
  Node: TTreeNode; State: TCustomDrawState; Stage: TCustomDrawStage;
  var PaintImages, DefaultDraw: Boolean);
Var
  hooked : Boolean;
  deh : TDeviceHookObject;
  drh : TDriverHookObject;
begin
DefaultDraw := True;
{$IFDEF FPC}
DeviceTreeView.Canvas.Brush.Color := ClBlack;
{$ELSE}
DeviceTreeView.Canvas.Font.Color := clBlack;
{$ENDIF}
DeviceTreeView.Canvas.Font.Style := [];
If Not Assigned(Node.Parent) Then
  begin
  drh := Node.Data;
  If Assigned(drh) Then
    begin
    If drh.Hooked Then
      begin
{$IFDEF FPC}
      DeviceTreeView.Canvas.Brush.Color := ClRed;
{$ELSE}
      DeviceTreeView.Canvas.Font.Color := clRed;
{$ENDIF}
      DeviceTreeView.Canvas.Font.Style := [fsBold];
      end;
    end;
  end
Else begin
  deh := Node.Data;
  If Assigned(deh) THen
    begin
    hooked := deh.Hooked;
    If hooked Then
      begin
{$IFDEF FPC}
      DeviceTreeView.Canvas.Brush.Color := ClRed;
{$ELSE}
      DeviceTreeView.Canvas.Font.Color := clRed;
{$ENDIF}
      DeviceTreeView.Canvas.Font.Style := [fsBold];
      end;

    While (Not hooked) And (FDeviceMap.TryGetValue(deh.AttachedDevice, deh)) Do
      begin
      hooked := deh.Hooked;
      If hooked Then
        begin
{$IFDEF FPC}
        DeviceTreeView.Canvas.Brush.Color := ClRed;
{$ELSE}
        DeviceTreeView.Canvas.Font.Color := clGray;
{$ENDIF}
        DeviceTreeView.Canvas.Font.Style := [fsBold];
        end;
      end;
    end;
  end;
end;

Procedure TTreeFrm.DeviceTreeViewChange(Sender: TObject; Node: TTreeNode);
Var
  I : Integer;
  drh : TDriverHookObject;
  deh : TDeviceHookObject;
begin
If Node.Selected Then
  begin
  If Not Assigned(Node.Parent) THen
    begin
    drh := Node.Data;
    For I := 0 To $1B Do
      begin
      IRPCheckListBOx.State[I] := cbUnchecked;
      IRPCheckListBox.Checked[I] := drh.Settings.IRPSettings.Settings[I] <> 0;
      end;

    For I := 0 To Ord(FastIoMax) - 1 Do
      begin
      FastIoCheckListBOx.State[I] := cbUnchecked;
      FastIoCheckListBox.Checked[I] := drh.Settings.FastIoSettings.Settings[I] <> 0;
      end;
    end
  Else begin
    deh := Node.Data;
    For I := 0 To $1B Do
      begin
      IRPCheckListBOx.State[I] := cbUnchecked;
      IRPCheckListBox.Checked[I] := deh.IRPSettings[I] <> 0;
      end;

    For I := 0 To Ord(FastIoMax) - 1 Do
      begin
      FastIoCheckListBOx.State[I] := cbUnchecked;
      FastIoCheckListBox.Checked[I] := deh.FastIoSettings[I] <> 0;
      end;
    end;
  end;
end;

Procedure TTreeFrm.DeviceTreeViewPopupMenuPopup(Sender: TObject);
Var
  hdr : TDriverHookObject;
  hde : TDeviceHookObject;
  tn : TTreeNode;
begin
tn := DeviceTreeView.Selected;
If Assigned(tn) Then
  begin
  hde := Nil;
  hdr := Nil;
  If Assigned(tn.Parent) Then
    begin
    hde := tn.Data;
    FDriverMap.TryGetValue(hde.Driverobject, hdr);
    end
  Else hdr := tn.Data;

  If Not Assigned(hde) Then
    HookedMenuItem.Checked := hdr.Hooked
  Else HookedMenuItem.Checked := hde.Hooked;

  DeviceExtensionMenuItem.Checked := hdr.DeviceExtensionHook;
  IRPMenuItem.Checked := hdr.Settings.MonitorIRP;
  IRPCompleteMenuItem.Checked := hdr.Settings.MonitorIRPCompletion;
  FastIOMenuItem.Checked := hdr.Settings.MonitorFastIo;
  StartIOMenuItem.Checked := hdr.Settings.MonitorStartIo;
  AddDeviceMenuItem.Checked := hdr.Settings.MonitorAddDevice;
  DriverUnloadMenuItem.Checked := hdr.Settings.MonitorUnload;
  NewDevicesMenuItem.Checked := hdr.Settings.MonitorNewDevices;
  DataMenuItem.Checked := hdr.Settings.MonitorData;
  end;
end;

Procedure TTreeFrm.FormClose(Sender: TObject; var Action: TCloseAction);
Var
  de : TDeviceHookObject;
  dr : TDriverHookObject;
  pdr : TPair<Pointer, TDriverHookObject>;
  pde : TPair<Pointer, TDeviceHookObject>;
begin
For de In FCurrentlyHookedDevices Do
  de.Free;

FCurrentlyHookedDevices.Free;
For dr In FCurrentlyHookedDrivers Do
  dr.Free;

FCurrentlyHookedDrivers.Free;
For pde In FDeviceMap Do
  pde.Value.Free;

FDeviceMap.Free;
For pdr In FDriverMap Do
  pdr.Value.Free;

FDriverMap.Free;
end;

Procedure TTreeFrm.FormCreate(Sender: TObject);
Var
  I : Byte;
begin
FCancelled := True;
For I := 0 To $1B Do
  IRPCheckListBox.AddItem(TDriverRequest.MajorFunctionToString(I), Nil);

For I := 0 To Ord(FastIoMax) - 1 Do
  FastIoCheckListBox.AddItem(TFastIoRequest.FastIoTypeToString(EFastIoOperationType(I)), Nil);

FDriverMap := TDictionary<Pointer, TDriverHookObject>.Create;
FDeviceMap := TDictionary<Pointer, TDeviceHookObject>.Create;
FCurrentlyHookedDrivers := TList<TDriverHookObject>.Create;
FCurrentlyHookedDevices := TList<TDeviceHookObject>.Create;
BuildTree;
end;

Procedure TTreeFrm.OkButtonClick(Sender: TObject);
Var
  cdr : TDriverHookObject;
  cde : TDeviceHookObject;
  dr : TDriverHookObject;
  de : TDeviceHookObject;
  pdr : TPair<Pointer, TDriverHookObject>;
  pde : TPair<Pointer, TDeviceHookObject>;
  opList : TTaskOperationList;
begin
FCancelled := False;
opList := TTaskOperationList.Create;
For cde In FCurrentlyHookedDevices DO
  begin
  If FDeviceMap.TryGetValue(cde.Address, de) THen
    begin
    If Not de.Hooked THen
      opList.Add(hooUnhook, de)
    Else If (Not CompareMem(@cde.IRPSettings, @de.IRPSettings, SizeOf(cde.IRPSettings))) Or
            (Not CompareMem(@cde.FastIoSettings, @de.FastIoSettings, SizeOf(cde.FastIoSettings))) Then
      opList.Add(hooChange, de);
    end;
  end;

For cdr In FCurrentlyHookedDrivers Do
  begin
  If FDriverMap.TryGetValue(cdr.Address, dr) Then
    begin
    If Not dr.Hooked Then
      begin
      opList
        .Add(hooStop, dr)
        .Add(hooUnhook, dr);
      end
    Else If
      (dr.Settings.MonitorNewDevices <> cdr.Settings.MonitorNewDevices) Or
      (dr.Settings.MonitorAddDevice <> cdr.Settings.MonitorAddDevice) Or
      (dr.Settings.MonitorStartIo <> cdr.Settings.MonitorStartIo) Or
      (dr.Settings.MonitorUnload <> cdr.Settings.MonitorUnload) Or
      (dr.Settings.MonitorFastIo <> cdr.Settings.MonitorFastIo) Or
      (dr.Settings.MonitorIRP <> cdr.Settings.MonitorIRP) Or
      (dr.Settings.MonitorIRPCompletion <> cdr.Settings.MonitorIRPCompletion) Then
      opList
        .Add(hooStop, dr)
        .Add(hooChange, dr)
        .Add(hooStart, dr);
    end;
  end;

For pdr In FDriverMap Do
  begin
  dr := pdr.Value;
  If (dr.Hooked) And (Not Assigned(dr.ObjectId)) Then
    begin
    opList
      .Add(hooHook, dr)
      .Add(hooStart, dr);
    end;
  end;

For pde In FDeviceMap Do
  begin
  de := pde.Value;
  If (de.Hooked) And (Not Assigned(de.ObjectId)) Then
    opList.Add(hooHook, de);
  end;

If (opList.Count > 0) Then
  begin
  With THookProgressFrm.Create(Self, opList) Do
    begin
    ShowModal;
    Free;
    end;
  end;

opList.Free;
Close;
end;

Procedure TTreeFrm.StornoButtonClick(Sender: TObject);
begin
FCancelled := True;
Close;
end;

Procedure TTreeFrm.TreePopupMenuClick(Sender: TObject);
Var
  M : TMenuItem;
  tn : TTreeNode;
  ho : THookObject;
  hde : TDeviceHookObject;
  hdr : TDriverHookObject;
begin
hdr := Nil;
hde := Nil;
tn := DeviceTreeView.Selected;
If Assigned(tn) Then
  begin
  M := Sender As TMenuItem;
  M.Checked := Not M.Checked;
  ho := tn.Data;
  If Assigned(tn.Parent) Then
    begin
    hde := tn.Data;
    FDriverMap.TryGetValue(hde.Driverobject, hdr);
    end
  Else hdr := tn.Data;

  If (M = HookedMenuItem) Then
    begin
    ho.Hooked := M.Checked;
    If Assigned(hde) Then
      begin
      If ho.Hooked Then
        begin
        Inc(hdr.NumberOfHookedDevices);
        hdr.Hooked := True
        end
      Else begin
        Dec(hdr.NumberOfHookedDevices);
        if (hdr.NumberOfHookedDevices = 0) And
          (Not hdr.Settings.MonitorNewDevices) And
          (Not hdr.Settings.MonitorAddDevice) And
          (Not hdr.Settings.MonitorUnload) THen
          hdr.Hooked := False;
        end;
      end;
    end
  Else If M = DeviceExtensionMenuItem Then
    hdr.DeviceExtensionHook := M.Checked
  Else If M = NewDevicesMenuItem Then
    hdr.Settings.MonitorNewDevices := M.Checked
  Else If M = IRPMenuItem Then
    hdr.Settings.MonitorIRP := M.Checked
  Else If M = IRPCompleteMenuItem Then
    hdr.Settings.MonitorIRPCompletion := M.Checked
  Else If M = FastIoMenuItem Then
    hdr.Settings.MonitorFastIo := M.Checked
  Else If M = StartIoMenuItem Then
    hdr.Settings.MonitorStartIo := M.Checked
  Else If M = AddDeviceMenuItem Then
    hdr.Settings.MonitorAddDevice := M.Checked
  Else If M = DriverUnloadMenuItem Then
    hdr.Settings.MonitorUnload := M.Checked
  Else If M = DataMenuItem Then
    hdr.Settings.MonitorData := M.Checked;
  end;

DeviceTreeView.Invalidate;
end;

end.

