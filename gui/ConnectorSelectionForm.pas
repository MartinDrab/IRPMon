Unit ConnectorSelectionForm;

Interface

Uses
  Windows, Messages, SysUtils,
  Variants, Classes, Graphics,
  Controls, Forms, Dialogs, ComCtrls, StdCtrls,
  IRPMonDll;

Type
  TConnectorSelectionFrm = Class(TForm)
    PageControl1: TPageControl;
    NoneTabSheet: TTabSheet;
    DeviceTabSheet: TTabSheet;
    NetworkTabSheet: TTabSheet;
    OkButton: TButton;
    StornoButton: TButton;
    Label1: TLabel;
    DeviceNameEdit: TEdit;
    DomainLabel: TLabel;
    PortLabel: TLabel;
    NetworkDomainEdit: TEdit;
    NetworkPortEdit: TEdit;
    VSockVersionEdit: TEdit;
    VSockAddressEdit: TEdit;
    VSockVersionLabel: TLabel;
    VSockAddressLabel: TLabel;
    NetworkTypeComboBox: TComboBox;
    HyperVVMIdEdit: TEdit;
    HyperVAppIdEdit: TEdit;
    HyperVVMLabel: TLabel;
    HyperVAppLabel: TLabel;
    procedure StornoButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
    procedure NetworkTypeComboBoxClick(Sender: TObject);
  Private
    FConnectionType : EIRPMonConnectorType;
    FCancelled : Boolean;
    FDeviceName : WideString;
    FNetworkAddress : WideString;
    FNetworkPort : WideString;
    FVSockTargetAddress : Cardinal;
    FVSockTargetPort : Cardinal;
    FHyperVVMId : TGuid;
    FHyperVAppId : TGuid;
    Function IsWOW64:Boolean;
  Public
    Property Cancelled : Boolean Read FCancelled;
    property ConnectionType : EIRPMonConnectorType Read FConnectionType;
    Property DeviceName : WideString Read FDeviceName;
    Property NetworkAddress : WideString Read FNetworkAddress;
    Property NetworkPort : WideString Read FNetworkPort;
    Property VSockTargetAddress : Cardinal Read FVSockTargetAddress;
    Property VSockTargetPort : Cardinal Read FVSockTargetPort;
    Property HyperVVMId : TGuid Read FHyperVVMId;
    Property HyperVAppId : TGuid Read FHyperVAppId;
  end;

Implementation

Uses
  Utils,
  VSockConnector;

{$R *.DFM}


{$IFDEF FPC}
Function IsWow64Process(hProcess:THandle; Var Wow64:LongBool):LongBool; StdCall; External 'kernel32.dll';
{$ENDIF}

Function TConnectorSelectionFrm.IsWOW64:Boolean;
Var
  b : BOOL;
begin
Result := False;
If IsWow64Process(GetCurrentProcess, b) Then
  Result := b;
end;

Procedure TConnectorSelectionFrm.NetworkTypeComboBoxClick(Sender: TObject);
begin
Case NetworkTypeComboBox.ItemIndex Of
  0 : begin
    DomainLabel.Caption := 'Domain/IP';
    PortLabel.Caption := 'Port';
    end;
  1 : begin
    DomainLabel.Caption := 'CID';
    PortLabel.Caption := 'Port';
    end;
  2 : begin
    DomainLabel.Caption := 'VM GUID';
    PortLabel.Caption := 'App GUID';
    end;
  end;
end;

Procedure TConnectorSelectionFrm.FormCreate(Sender: TObject);
Var
  vnciVersion : Cardinal;
  vnciAddress : Cardinal;
begin
FCancelled := True;
If (IsWOW64) Or (Not IsAdmin) Then
  begin
  DeviceTabSheet.Enabled := False;
  DeviceTabSheet.TabVisible := False;
  end;

vnciVersion := VSockConn_VMCIVersion;
If vnciVersion <> VNCI_VERSION_INVALID Then
  begin
  vnciAddress := VSockConn_LocalId;
  VSockVersionEdit.Text := Format('%u.%u', [vnciVersion And $FFFF, vnciVersion Shr 16]);
  VSockAddressEdit.Text := Format('0x%x', [vnciAddress]);
  end
Else begin
  VSockVersionEdit.Text := '<not installed>';
  VSockAddressEdit.Text := '<not installed>';
  end;
end;

Procedure TConnectorSelectionFrm.OkButtonClick(Sender: TObject);
begin
FConnectionType := EIRPMonConnectorType(PageControl1.ActivePageIndex);
Case FConnectionType Of
  ictNone: ;
  ictDevice: FDeviceName := DeviceNameEdit.Text;
  ictNetwork: begin
    Case NetworkTypeComboBox.ItemIndex Of
      0 : begin
        FNetworkAddress := NetworkDomainEdit.Text;
        FNetworkPort := NetworkPortEdit.Text;
        end;
      1 : begin
        FConnectionType := ictVSockets;
        FVSockTargetAddress := StrToUInt(NetworkDomainEdit.Text);
        FVSockTargetPort := StrToUInt(NetworkPortEdit.Text);
        end;
      2 : begin
        FConnectionType := ictHyperV;
        FHyperVVMId := StringToGuid(NetworkDomainEdit.Text);
        FHyperVAppId := StringToGuid(NetworkPortEdit.Text);
        end;
      end;
    end;
  end;

FCancelled := False;
Close;
end;

Procedure TConnectorSelectionFrm.StornoButtonClick(Sender: TObject);
begin
Close;
end;

End.

