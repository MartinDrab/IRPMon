Unit Utils;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Procedure ErrorMessage(AMsg:WideString);
Procedure WarningMessage(AMsg:WideString);
Procedure InformationMessage(AMsg:WideString);
Procedure WinErrorMessage(AMsg:WideString; ACode:Cardinal);

Implementation

Uses
  Windows, SysUtils;

Procedure ErrorMessage(AMsg:WideString);
begin
MessageBoxW(0, PWideChar(AMsg), 'Error', MB_OK Or MB_ICONERROR);
end;

Procedure WarningMessage(AMsg:WideString);
begin
MessageBoxW(0, PWideChar(AMsg), 'Warning', MB_OK Or MB_ICONERROR);
end;

Procedure InformationMessage(AMsg:WideString);
begin
MessageBoxW(0, PWideChar(AMsg), 'Information', MB_OK Or MB_ICONINFORMATION);
end;

Procedure WinErrorMessage(AMsg:WideString; ACode:Cardinal);
Var
  msg : WideString;
begin
msg := Format('%s'#13#10'Error code: %u'#13#10'Error message: %s', [AMsg, ACode, SysErrorMessage(ACode)]);
ErrorMessage(msg);
end;



End.
