Unit SymTables;

Interface

Uses
  Generics.Collections,
  RefObject,
  DbgHelpDll;

Type
  TSymTable = Class (TRefObject)
    Private
      FName : WideString;
      FNames : TDictionary<UInt64, WideString>;
    Protected
      Function GetSymbolCount:Integer;
    Public
      Constructor Create(AProcess:THandle; AFileName:WideString); Reintroduce;
      Destructor Destroy; Override;

      Function FindSymbol(Var AOffset:UInt64; Var AName:WideString):Boolean;

      Property Name : WideString Read FName;
      Property Count : Integer Read GetSymbolCount;
    end;

  TModuleSymbolStore = Class
    Private
      FhProcess : THandle;
      FKeys : TList<WideString>;
      FSymTables : TObjectDictionary<WideString, TSymTable>;
    Protected
      Function GetModule(ABaseName:WideString):TSymTable;
      Function GetModuleCount:Integer;
      Function GetModuleByIndex(AIndex:Integer):TSymTable;
    Public
      Constructor Create(AProcess:THandle); Reintroduce;
      Destructor Destroy;

      Function AddFile(AFileName:WideString):Boolean;
      Function AddDirectory(ADirName:WideString):Boolean;

      Property Module [ABaseName:WideString] : TSymTable Read GetModule;
      Property ModuleByIndex [AIndex:Integer] : TSymTable Read GetModuleByIndex;
      Property ModuleCount : Integer Read GetModuleCount;
    end;

Implementation

Uses
  Windows,
  SysUtils;

(** TSymTable **)

Function _EnumCallback(pSymInfo:PSYMBOL_INFOW; SymbolSize:Cardinal; UserContext:Pointer):BOOL; StdCall;
Var
  symName : WideString;
  table : TSymTable;
  offset : UInt64;
begin
table := TSymTable(UserContext);
If (pSymInfo.Address <> 0) Then
  begin
  SetLength(symName, pSymInfo.NameLen - 1);
  Move(pSymInfo.Name, PWideChar(symName)^, Length(symName)*SizeOf(WideChar));
  offset := pSymInfo.Address - pSymInfo.ModBase;
  Try
    table.FNames.Add(offset, symName);
  Except
    end;
  end;

Result := True;
end;

Constructor TSymTable.Create(AProcess:THandle; AFileName:WideString);
Var
  hLib : THandle;
  err : Cardinal;
  hFile : THandle;
  fileSize : Int64;
  pdbFileBuffer : Packed Array [0..MAX_PATH - 1] Of WideChar;
  dbgFileBuffer : Packed Array [0..MAX_PATH - 1] Of WideChar;
  i1 : TGuid;
  i2 : Cardinal;
  i3 : Cardinal;
  found : Packed Array [0..MAX_PATH - 1] Of WideChar;
begin
Inherited Create;
FName := AFileName;
FNames := TDictionary<UInt64, WideString>.Create;
(*
FillChar(i1, SizeOf(i1), 0);
FillChar(i2, SizeOf(i2), 0);
FillChar(i3, SizeOf(i3), 0);
If Not SymSrvGetFileIndexesW(PWideChar(AFileName), i1, i2, i3, 0) Then
  begin
  err := GetLastError;
  Raise Exception.Create(Format('Unable to get the PDB file IDs: %u', [err]));
  end;

FillChar(found, SizeOf(found), 0);
If Not SymFindFileInPathW(AProcess, Nil, PWideChar(ChangeFileExt(ExtractFileName(AFileName), '.pdb')), @i1, i2, i3, SSRVOPT_GUIDPTR, found, Nil, Nil) Then
  begin
  err := GetLastError;
  Raise Exception.Create(Format('Unable to fetch the PDB file: %u', [err]));
  end;
*)
hFile := CreateFileW(PWideChar(AFileName), GENERIC_READ, FILE_SHARE_READ, Nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
If hFile = INVALID_HANDLE_VALUE Then
  begin
  err := GetLastError;
  Raise Exception.Create(Format('Unable to open the file: %u', [err]));
  end;

If Not GetFileSizeEx(hFile, fileSize) Then
  begin
  err := GetLastError;
  CloseHandle(hFile);
  Raise Exception.Create(Format('Unable to get the file size: %u', [err]));
  end;

hLib := LoadLibraryExW(PWideChar(AFileName), 0, DONT_RESOLVE_DLL_REFERENCES);
If hLib = 0 Then
  begin
  err := GetLastError;
  CloseHandle(hFile);
  Raise Exception.Create(Format('Unable to load library: %u', [err]));
  end;

If SymLoadModuleExW(AProcess, hFile, PWideChar(AFileName), Nil, hLib, fileSize, Nil, 0) = 0 Then
  begin
  err := GetLastError;
  FreeLibrary(hLib);
  CloseHandle(hFile);
  Raise Exception.Create(Format('Unable to load symbols: %u', [err]));
  end;

If Not SymEnumSymbolsW(AProcess, hLib, PWideChar(ExtractFileName(AFileName) + '!*'), _EnumCallback, Self) Then
  begin
  err := GetLastError;
  SymUnloadModule64(AProcess, hLib);
  FreeLibrary(hLib);
  CloseHandle(hFile);
  Raise Exception.Create(Format('Unable to enumerate symbols: %u', [err]));
  end;

SymUnloadModule64(AProcess, hLib);
FreeLibrary(hLib);
CloseHandle(hFile);
end;

Destructor TSymTable.Destroy;
begin
FNames.Free;
Inherited Destroy;
end;

Function TSymTable.GetSymbolCount:Integer;
begin
Result := FNames.Count;
end;

Function TSymTable.FindSymbol(Var AOffset:UInt64; Var AName:WideString):Boolean;
Var
  p : TPair<UInt64, WideString>;
  minOffset : UInt64;
  minName : WideString;
begin
Result := False;
For p In FNames Do
  begin
  If p.Key < AOffset Then
    begin
    Result := True;
    If AOffset - p.Key < minOffset Then
      begin
      minOffset := AOffset - p.Key;
      minName := p.Value;
      end;
    end;
  end;

If Result Then
  begin
  AOffset := minOffset;
  AName := minName;
  end;
end;

(** TModuleSymbolStore **)

Constructor TModuleSymbolStore.Create(AProcess:THandle);
begin
Inherited Create;
FhProcess := AProcess;
FSymTables := TObjectDictionary<WideString, TSymTable>.Create;
FKeys := TList<WideString>.Create;
end;

Destructor TModuleSymbolStore.Destroy;
begin
FKeys.Free;
FSymTables.Free;
Inherited Destroy;
end;

Function TModuleSymbolStore.AddFile(AFileName:WideString):Boolean;
Var
  k : WideString;
  index : Integer;
  st : TSymTable;
begin
Try
  st := TSymTable.Create(FhProcess, AFileName);
  Result := True;
Except
  Result := False;
  end;

If Result Then
  begin
  k := WideUpperCase(ExtractFileName(AFileName));
  index := FKeys.IndexOf(k);
  If index <> -1 Then
    FKeys[index] := k
  Else FKeys.Add(k);

  FSymTables.AddOrSetValue(k, st);
  end;
end;

Function TModuleSymbolStore.AddDirectory(ADirName:WideString):Boolean;
Var
  d : TSearchRec;
begin
Result := True;
If FindFirst(ADirName + '\*', FaAnyFile, d) = 0 Then
  begin
  Repeat
  If (d.Attr And FaDirectory) <> 0 Then
    Continue;

  AddFile(ADirName + '\' + d.Name);
  Until FindNext(d) <> 0;
  SysUtils.FindClose(d);
  end;
end;

Function TModuleSymbolStore.GetModule(ABaseName:WideString):TSymTable;
begin
Result := Nil;
If FSymTables.TryGetValue(WideUpperCase(ABaseName), Result) Then
  Result.Reference;
end;

Function TModuleSymbolStore.GetModuleCount:Integer;
begin
Result := FKeys.Count;
end;

Function TModuleSymbolStore.GetModuleByIndex(AIndex:Integer):TSymTable;
Var
  k : WideString;
begin
k := FKeys[AIndex];
Result := GetModule(k);
end;

End.

