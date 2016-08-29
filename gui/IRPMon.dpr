program IRPMon;

uses
  Windows,
  Vcl.Forms,
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
  AboutForm in 'AboutForm.pas' {AboutBox};

{$R *.RES}

Var
  err : Cardinal;
Begin
Application.Initialize;
Application.MainFormOnTaskbar := True;
err := TablesInit('ntstatus.txt', 'ioctl.txt');
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

  TablesFinit;
  end
Else WinErrorMessage('Unable to initialize name tables', err);
End.
