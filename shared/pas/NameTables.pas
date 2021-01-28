Unit NameTables;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Function TablesInit(ANTSTATUSFile:WideString; AIOCTLFIle:WideString):Cardinal;
Procedure TablesFinit;
Function TablesNTSTATUSToString(AValue:Cardinal):WideString;
Function TablesIOCTLToString(AValue:Cardinal):WideString;

Implementation

Uses
  Windows, SysUtils, Utils, Generics.Collections;

Var
  ntstatusTable : TDictionary<Cardinal, WideString>;
  ioctlTable : TDictionary<Cardinal, WideString>;


fUNCTION _TableLoad(ATable:TDictionary<Cardinal, WideString>; AFileName:WideString):Cardinal;
Var
  c : WideString;
  vstr : WideString;
  v : Cardinal;
  index : Integer;
  F : TextFile;
  line : WideString;
begin
{$I-}
AssignFile(F, AFileName);
Reset(F);
{$I+}
Result := IOResult;
If Result = ERROR_SUCCESS Then
  begin
  While Not EOF(F) Do
    begin
    Readln(F, line);
    If Line = '' Then
      Continue;

    index := Pos(' ', line);
    If index > 0 Then
      begin
      c := Copy(line, 1, index - 1);
      vstr := Copy(line, index + 1, Length(line) - index);
      If (vstr[1] = '0') And (vstr[2] = 'x') Then
        Delete(vstr, 1, 2);

      vstr := '$' + vstr;
      While vstr[Length(vstr)] = ' ' Do
        Delete(vstr, Length(vstr), 1);

      v := StrToInt64(vstr);
      Try
        ATable.Add(v, c);
      Except
//        ErrorMessage(Format('Duplicate entry for 0x%x (%s)', [v, c]));
        end;
      end;
    end;

  CloseFile(F);
  end;
end;

Function TablesInit(ANTSTATUSFile:WideString; AIOCTLFIle:WideString):Cardinal;
begin
ntstatusTable := TDictionary<Cardinal, WideString>.Create;
Result := _TableLoad(ntstatusTable, ANTSTATUSFile);
If Result = ERROR_SUCCESS Then
  begin
  ioctlTable := TDictionary<Cardinal, WideString>.Create;
  Result := _TableLoad(ioctlTable, AIOCTLFile);
  If Result <> ERROR_SUCCESS Then
    begin
    FreeAndNil(ioctlTable);
    FreeAndNil(ntstatusTable);
    end;
  end;
end;

Procedure TablesFinit;
begin
FreeAndNil(ioctlTable);
FreeAndNil(ntstatusTable);
end;

Function TablesNTSTATUSToString(AValue:Cardinal):WideString;
Var
  str : WideString;
begin
Result := Format('0x%x', [AValue]);
If Assigned(ntstatusTable) Then
  begin
  If ntstatusTable.TryGetValue(AValue, str) Then
    Result := str;
  end;
end;

Function TablesIOCTLToString(AValue:Cardinal):WideString;
Var
  str : WideString;
begin
Result := Format('0x%x', [AValue]);
If Assigned(ioctlTable) Then
  begin
  If ioctlTable.TryGetValue(AValue, str) Then
    Result := str;
  end;
end;



End.
