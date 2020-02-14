Program IRPLauncher;

{$R *.RES}

Uses
  Windows,
  Vcl.Forms,
  SysUtils;

Var
  wow64 : LongBool;
  err : Cardinal;
  si : STARTUPINFOW;
  pi : PROCESS_INFORMATION;
  I : Integer;
  bitness : Cardinal;
  targetFile : WideString;
  targetCmdLine : WideString;
Begin
wow64 := False;
bitness := 0;
If IsWow64Process(GetCurrentProcess, wow64) Then
  begin
  If Not wow64 Then
    bitness := 32;

  If bitness = 0 Then
    begin
    If ParamCount > 0 Then
      begin
      Try
        bitness := StrToInt(ParamStr(1));
      Except
        end;
      end;

    If bitness = 0 Then
      begin
      Case MessageBoxW(0, 'Which version of IRPMon do you wish to run?'#13#10'Yes - 64-bit'#13#10'No - 32-bit', 'Question', MB_YESNOCANCEL Or MB_ICONINFORMATION) Of
        IDYES : bitness := 64;
        IDNO : bitness := 32;
        end;
      end;
    end;

  If bitness <> 0 Then
    begin
    targetFile := ExtractFilePath(Application.ExeName);
    Case bitness Of
      32 : targetFile := targetFile + 'x86\IRPMon.exe';
      64 : targetFile := targetFile + 'x64\IRPMon.exe';
      end;

    targetCmdLine := Format('"%s"', [targetFile]);
    For I := 2 To ParamCount Do
      targetCmdLine := Format('%s "%s"', [targetCmdLine, ParamStr(I)]);

    si.cb := SizeOf(si);
    FillChar(pi, SizeOf(pi), 0);
    If CreateProcessW(PWideChar(targetFile), PWideChar(targetCmdLine), Nil, Nil, False, 0, Nil, PWideChar(ExtractFileDir(targetFile)), si, pi) Then
      begin
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      end
    Else begin
      err := GetLastError;
      MessageBoxW(0, PWIdeChar(Format('%s (%u)', [SysErrorMessage(err), err])), 'Error', MB_OK Or MB_ICONERROR);
      end;
    end
  Else begin
    err := GetLastError;
    MessageBoxW(0, PWIdeChar(Format('%s (%u)', [SysErrorMessage(err), err])), 'Error', MB_OK Or MB_ICONERROR);
    end;
  end;
End.
