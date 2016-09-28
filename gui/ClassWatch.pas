Unit ClassWatch;

Interface

Uses
  Windows, IRPMonDll, Generics.Collections;

Type
  TWatchableClass = Class
    Private
      FName : WideString;
      FGUid : WideString;
      FBeginning : Boolean;
      FUpperFilter : Boolean;
      FRegistered : Boolean;
    Public
      Constructor Create(AGuid:WideString; AName:WideString); Reintroduce; Overload;
      Constructor Create(Var ARecord:CLASS_WATCH_RECORD); Reintroduce; Overload;

      Function Register(AUpperFilter:Boolean; ABeginning:Boolean):Cardinal;
      Function Unregister:Cardinal;

      Class Function Enumerate(AList:TList<TWatchableClass>):Cardinal;

      Property Registered : Boolean Read FRegistered;
      Property Beginning : Boolean Read FBeginning;
      Property UpperFIlter : Boolean Read FUpperFilter;
      Property Name : WideString Read FName;
      Property GUid : WideString Read FGuid;
    end;



Implementation

Uses
  SysUtils, Utils;

Constructor TWatchableClass.Create(AGuid:WideString; AName:WideString);
begin
Inherited Create;
FGUid := AGuid;
FName := AName;
FRegistered := False;
FUpperFilter := False;
FBeginning := False;
end;

Constructor TWatchableClass.Create(Var ARecord:CLASS_WATCH_RECORD);
begin
Inherited Create;
FGuid := Copy(WideString(ARecord.ClassGuidString), 1, StrLen(ARecord.ClassGuidString));
FRegistered := True;
FBeginning := ARecord.Beginning <> 0;
FUpperFilter := ARecord.UpperFilter <> 0;
end;


Function TWatchableClass.Register(AUpperFilter:Boolean; ABeginning:Boolean):Cardinal;
begin
Result := IRPMonDllClassWatchRegister(PWideChar(FGuid), AUpperFilter, ABeginning);
If Result = ERROR_SUCCESS Then
  begin
  FUpperFilter := AUpperFilter;
  FBeginning := ABeginning;
  FRegistered := True;
  end;
end;


Function TWatchableClass.Unregister:Cardinal;
begin
Result := IRPMonDllClassWatchUnregister(PWIdeChar(FGuid), FUpperFIlter, FBeginning);
If Result = ERROR_SUCCESS THen
  FRegistered := False;
end;


Class Function TWatchableClass.Enumerate(AList:TList<TWatchableClass>):Cardinal;
Var
  strGuid : WideString;
  I : Integer;
  p : TPair<WideString, TWatchableClass>;
  wc : TWatchableClass;
  classNameLen : Cardinal;
  className : Packed Array [0..MAX_PATH] Of WideChar;
  dwValue : Cardinal;
  dwValueLen : Cardinal;
  cKey : HKEY;
  classGuidLen : Cardinal;
  classGuid : Packed Array [0..38] Of WideChar;
  classIndex : Integer;
  classesKey : HKEY;
  cwCount : Cardinal;
  tmp : PCLASS_WATCH_RECORD;
  cwArray : PCLASS_WATCH_RECORD;
  dict : TDictionary<WideString, TWatchableClass>;
begin
dict := TDictionary<WideString, TWatchableClass>.Create;
Result := IRPMonDllClassWatchEnum(cwArray, cwCount);
If Result = ERROR_SUCCESS THen
  begin
  Result := RegOpenKeyEx(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\Class', 0, KEY_READ, classesKey);
  If Result = ERROR_SUCCESS THen
    begin
    classIndex := 0;
    Repeat
    classGuidLen := SizeOf(classGuid) Div SizeOf(WideChar);
    FillChar(classGuid, SizeOf(classGUid), 0);
    Result := RegEnumKeyExW(classesKey, classIndex, classGuid, classGuidLen, Nil, Nil, Nil, Nil);
    If Result = ERROR_SUCCESS THen
      begin
      Result := RegOpenKeyEx(classesKey, classGuid, 0, KEY_READ, cKey);
      If Result = ERROR_SUCCESS THen
        begin
        dwValueLen := SizeOf(dwValue);
        Result := RegQueryValueExW(cKey, 'NoUseClass', Nil, Nil, @dwValue, @dwValueLen);
        If (Result <> ERROR_SUCCESS) Or (dwValue = 0) Then
          begin
          classNameLen := SizeOf(className);
          FillChar(className, SizeOf(className), 0);
          Result := RegQueryValueExW(cKey, 'Class', Nil, Nil, @className, @classNameLen);
          If Result = ERROR_SUCCESS Then
            begin
            wc := TWatchableClass.Create(
              Copy(WideString(classGuid), 1, StrLen(classGuid)),
              Copy(WideString(className), 1, StrLen(className)));
            dict.Add(WideUpperCase(wc.Guid), wc);
            end;
          end;

        If Result = ERROR_FILE_NOT_FOUND Then
          Result := ERROR_SUCCESS;

        RegCloseKey(cKey);
        end;
      end;

    Inc(classIndex);
    Until Result <> ERROR_SUCCESS;

    If Result = ERROR_NO_MORE_ITEMS Then
      Result := ERROR_SUCCESS;

    RegCloseKey(classesKey);
    end;

  tmp := cwArray;
  For I := 0 To cwCount - 1 Do
    begin
    strGuid := WideUpperCase(COpy(WideString(tmp.ClassGuidString), 1, StrLen(tmp.ClassGuidString)));
    If dict.TryGetValue(strGuid, wc) Then
      begin
      wc.FRegistered := True;
      wc.FBeginning := tmp.Beginning <> 0;
      wc.FUpperFilter := tmp.UpperFilter <> 0;
      end;

    Inc(tmp);
    end;

  IRPMonDllClassWatchEnumFree(cwArray, cwCount);
  end;

If Result = ERROR_SUCCESS Then
  begin
  For p In dict Do
    AList.Add(p.Value);
  end;

dict.Free;
end;

End.
