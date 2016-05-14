program IRPMon;

uses
  Windows,
  Vcl.Forms,
  MainForm in 'MainForm.pas' {MainFrm},
  IRPMonDll in 'IRPMonDll.pas',
  IRPMonRequest in 'IRPMonRequest.pas',
  HookObjects in 'HookObjects.pas',
  Utils in 'Utils.pas',
  ListModel in 'ListModel.pas',
  RequestListModel in 'RequestListModel.pas',
  DriverRequest in 'DriverRequest.pas',
  IRPRequest in 'IRPRequest.pas',
  NameTables in 'NameTables.pas';

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
