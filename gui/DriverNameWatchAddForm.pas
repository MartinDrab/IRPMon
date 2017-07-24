Unit DriverNameWatchAddForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils,
  Variants, Classes, Graphics,
  Controls, Forms, Dialogs, ExtCtrls,
  StdCtrls, IRPMonDll, CheckLst;

Type
  TDriverNameWatchAddFrm = Class(TForm)
    Panel1: TPanel;
    CancelButton: TButton;
    OkButton: TButton;
    DriverNameEdit: TEdit;
    Label1: TLabel;
    MonitorSettingsCheckListBox: TCheckListBox;
    Label2: TLabel;
    procedure CancelButtonClick(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  Private
    FCancelled : Boolean;
    FDriverName : WideString;
    FDriverSettings : DRIVER_MONITOR_SETTINGS;
  Public
    Property Cancelled : Boolean Read FCancelled;
    Property DriverName : WideString Read FDriverName;
    Property DriverSettings : DRIVER_MONITOR_SETTINGS Read FDriverSettings;
  end;


Implementation

{$R *.dfm}

Uses
  Utils;

Procedure TDriverNameWatchAddFrm.CancelButtonClick(Sender: TObject);
begin
Close;
end;

Procedure TDriverNameWatchAddFrm.FormCreate(Sender: TObject);
begin
FCancelled := True;
MonitorSettingsCheckListBox.Checked[0] := True;
MonitorSettingsCheckListBox.Checked[1] := True;
MonitorSettingsCheckListBox.Checked[2] := True;
MonitorSettingsCheckListBox.Checked[5] := True;
MonitorSettingsCheckListBox.Checked[6] := True;
end;

Procedure TDriverNameWatchAddFrm.OkButtonClick(Sender: TObject);
begin
If DriverNameEdit.Text <> '' Then
  begin
  FDriverName := DriverNameEdit.Text;
  FDriverSettings.MonitorNewDevices := MonitorSettingsCheckListBox.Checked[0];
  FDriverSettings.MonitorIRP := MonitorSettingsCheckListBox.Checked[1];
  FDriverSettings.MonitorIRPCompletion := MonitorSettingsCheckListBox.Checked[2];
  FDriverSettings.MonitorFastIo := MonitorSettingsCheckListBox.Checked[3];
  FDriverSettings.MonitorStartIo := MonitorSettingsCheckListBox.Checked[4];
  FDriverSettings.MonitorAddDevice := MonitorSettingsCheckListBox.Checked[5];
  FDriverSettings.MonitorUnload := MonitorSettingsCheckListBox.Checked[6];
  FillChar(FDriverSettings.IRPSettings, SizeOf(FDriverSettings.IRPSettings), Ord(True));
  FillChar(FDriverSettings.FastIoSettings, SizeOf(FDriverSettings.FastIoSettings), Ord(True));
  FCancelled := False;
  Close;
  end
Else WarningMessage('No driver name specified');
end;



End.
