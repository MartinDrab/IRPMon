Unit ConnectorSelectionForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils,
  System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ComCtrls, Vcl.StdCtrls,
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
    Label2: TLabel;
    Label3: TLabel;
    NetworkDomainEdit: TEdit;
    NetworkPortEdit: TEdit;
    procedure StornoButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
  Private
    FConnectionType : EIRPMonConnectorType;
    FCancelled : Boolean;
    FDeviceName : WideString;
    FNetworkAddress : WideString;
    FNetworkPort : WideString;
  Public
    Property Cancelled : Boolean Read FCancelled;
    property ConnectionType : EIRPMonConnectorType Read FConnectionType;
    Property DeviceName : WideString Read FDeviceName;
    Property NetworkAddress : WideString Read FNetworkAddress;
    Property NetworkPort : WideString Read FNetworkPort;
  end;

Implementation

{$R *.DFM}

Procedure TConnectorSelectionFrm.FormCreate(Sender: TObject);
begin
FCancelled := True;
end;

Procedure TConnectorSelectionFrm.OkButtonClick(Sender: TObject);
begin
FConnectionType := EIRPMonConnectorType(PageControl1.ActivePageIndex);
Case FConnectionType Of
  ictNone: ;
  ictDevice: FDeviceName := DeviceNameEdit.Text;
  ictNetwork: begin
    FNetworkAddress := NetworkDomainEdit.Text;
    FNetworkPort := NetworkPortEdit.Text;
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
