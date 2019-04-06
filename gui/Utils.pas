Unit Utils;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Procedure ErrorMessage(AMsg:WideString);
Procedure WarningMessage(AMsg:WideString);
Procedure InformationMessage(AMsg:WideString);
Procedure WinErrorMessage(AMsg:WideString; ACode:Cardinal);
Function BufferToHex(ABuffer:Pointer; ASize:NativeUInt):WideString;

Implementation

Uses
  Windows, SysUtils, Classes;

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

Function BufferToHex(ABuffer:Pointer; ASize:NativeUInt):WideString;
Var
  l : TStringList;
  d : PByte;
  hexLine : WideString;
  dispLine : WideString;
  index : Integer;
  I : Integer;
begin
l := TStringList.Create;
hexLine := '';
dispLine := '';
index := 0;
d := ABuffer;
For I := 0 To ASize - 1 Do
  begin
  hexLine := hexLine + ' ' + IntToHex(d^, 2);
  If d^ >= Ord(' ') Then
    dispLine := dispLine + Chr(d^)
  Else dispLine := dispLine + '.';

  Inc(Index);
  Inc(d);
  If index = 16 Then
    begin
    l.Add(Format('%s  %s', [hexLine, dispLine]));
    hexLine := '';
    dispLine := '';
    index := 0;
    end;
  end;

If index > 0 Then
  begin
  For I := index To 16 - 1 Do
    hexLine := hexLine + '   ';

  l.Add(Format('%s  %s', [hexLine, dispLine]));
  end;

Result := l.Text;
l.Free;
end;



End.
