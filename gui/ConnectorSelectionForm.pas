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
    Function IsWOW64:Boolean;
  Public
    Property Cancelled : Boolean Read FCancelled;
    property ConnectionType : EIRPMonConnectorType Read FConnectionType;
    Property DeviceName : WideString Read FDeviceName;
    Property NetworkAddress : WideString Read FNetworkAddress;
    Property NetworkPort : WideString Read FNetworkPort;
  end;

Implementation

Uses
  Utils;

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

Procedure TConnectorSelectionFrm.FormCreate(Sender: TObject);
begin
FCancelled := True;
If (IsWOW64) Or (Not IsAdmin) Then
  begin
  DeviceTabSheet.Enabled := False;
  DeviceTabSheet.TabVisible := False;
  end;
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
