Unit Utils;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Graphics;

Procedure ErrorMessage(AMsg:WideString);
Procedure WarningMessage(AMsg:WideString);
Procedure InformationMessage(AMsg:WideString);
Procedure WinErrorMessage(AMsg:WideString; ACode:Cardinal);
Function BufferToHex(ABuffer:Pointer; ASize:NativeUInt):WideString;
Procedure ColorToComponents(AColor:TColor; Var AR,AG,AB:Cardinal);
Function ColorLuminance(AColor:TColor):Cardinal;
Function ColorLuminanceHeur(AColor:TColor):Cardinal;
{$IFDEF FPC}
Function UIntToStr(AValue:UInt64):WideString;
{$ENDIF}

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


Procedure ColorToComponents(AColor:TColor; Var AR,AG,AB:Cardinal);
begin
AR := (AColor And $0000FF);
AG := (AColor Shr 8) ANd $FF;
AB := (AColor Shr 16) And $FF;
end;

Function ColorLuminance(AColor:TColor):Cardinal;
Const
  weights : Array [0..2] Of Double = (
    0.2126,
    0.7152,
    0.0722
  );
Var
  cd : Double;
  I : Integer;
  components : Array [0..2] Of Cardinal;
begin
Result := 0;
ColorToComponents(AColor, components[0], components[1], components[2]);
For I := Low(components) To High(components) Do
  begin
  cd := components[I] / 255.0;
  If (cd <= 0.03928) Then
    cd := cd /12.92
  Else cd := Exp(ln((cd + 0.055) / 1.055)*2.4);

  Inc(Result, Trunc(weights[I]*cd*10000.0));
  end;
end;

Function ColorLuminanceHeur(AColor:TColor):Cardinal;
Var
  r, g, b : Cardinal;
begin
ColorToComponents(AColor, r, g, b);
Result := (r*2990 + g*5870 + b*1140) Div 1000;
end;

{$IFDEF FPC}

Function UIntToStr(AValue:UInt64):WideString;
Var
  digit : Cardinal;
begin
Result := '';
If AValue > 0 Then
  begin
  Repeat
  digit := AValue Mod 10;
  AValue := AValue Div 10;
  Result := Chr(digit + Ord('0')) + Result;
  Until AValue = 0;
  end
Else Result := '0';
end;

{$ENDIF}



End.

