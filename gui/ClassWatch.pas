Unit ClassWatch;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, IRPMonDll, Classes,
  Generics.Collections, Generics.Defaults;

Type
  TWatchableClass = Class
    Private
      FName : WideString;
      FGUid : WideString;
      FBeginning : Boolean;
      FUpperFilter : Boolean;
      FRegistered : Boolean;
    Public
      Constructor Create(AGuid:WideString; AName:WideString; AUpperFilter:Boolean); Reintroduce; Overload;
      Constructor Create(Var ARecord:CLASS_WATCH_RECORD); Reintroduce; Overload;

      Function Register(ABeginning:Boolean):Cardinal;
      Function Unregister:Cardinal;

      Class Function Enumerate(AList:TList<TWatchableClass>):Cardinal;

      Property Registered : Boolean Read FRegistered;
      Property Beginning : Boolean Read FBeginning;
      Property UpperFIlter : Boolean Read FUpperFilter;
      Property Name : WideString Read FName;
      Property GUid : WideString Read FGuid;
    end;

  TWatchableClassComparer = Class (TComparer<TWatchableClass>)
{$IFDEF FPC}
    Function Compare(constref Left, Right: TWatchableClass): Integer; Override;
{$ELSE}
    Function Compare(const Left, Right: TWatchableClass): Integer; Override;
{$ENDIF}
  end;

Implementation

Uses
  SysUtils, Utils;


(** TWatchableClass **)

Constructor TWatchableClass.Create(AGuid:WideString; AName:WideString; AUpperFilter:Boolean);
begin
Inherited Create;
FGUid := AGuid;
FName := AName;
FRegistered := False;
FUpperFilter := AUpperFilter;
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


Function TWatchableClass.Register(ABeginning:Boolean):Cardinal;
begin
Result := IRPMonDllClassWatchRegister(PWideChar(FGuid), FUpperFilter, ABeginning);
If Result = ERROR_SUCCESS Then
  begin
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
  strclassGuid : WideString;
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
  key : WideString;
  wcComparer : TWatchableClassComparer;
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
      Result := RegOpenKeyExW(classesKey, classGuid, 0, KEY_READ, cKey);
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
            strclassGuid := Copy(WideString(classGuid), 1, StrLen(classGuid));
            wc := TWatchableClass.Create(
              strclassGuid,
              Copy(WideString(className), 1, StrLen(className)),
              False);

            key := WideUpperCase(strClassGuid) + 'L';
            dict.Add(key, wc);
            wc := TWatchableClass.Create(
              strclassGuid,
              Copy(WideString(className), 1, StrLen(className)),
              True);

            key := WideUpperCase(strClassGuid) + 'U';
            dict.Add(key, wc);
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
    key := WideUpperCase(COpy(WideString(tmp.ClassGuidString), 1, StrLen(tmp.ClassGuidString)));
    If tmp.UpperFilter <> 0 Then
      key := Key + 'U'
    Else key := key + 'L';

    If dict.TryGetValue(key, wc) Then
      begin
      wc.FRegistered := True;
      wc.FBeginning := tmp.Beginning <> 0;
      end;

    Inc(tmp);
    end;

  IRPMonDllClassWatchEnumFree(cwArray, cwCount);
  end;

If Result = ERROR_SUCCESS Then
  begin
  For p In dict Do
    AList.Add(p.Value);

  wcComparer := TWatchableClassComparer.Create;
  AList.Sort(wcComparer);
  wcComparer.Free;
  end;

dict.Free;
end;

{$IFDEF FPC}
Function TWatchableClassComparer.Compare(constref Left, Right: TWatchableClass): Integer;
{$ELSE}
Function TWatchableClassComparer.Compare(const Left, Right: TWatchableClass): Integer;
{$ENDIF}
begin
Result := WideCompareText(Left.Name, Right.Name);
If Result = 0 Then
  begin
  If Left.UpperFilter <> Right.UpperFilter Then
    begin
    If Not Left.UpperFIlter Then
      Result := -1
    Else Result := 1;
    end;
  end;
end;

End.

