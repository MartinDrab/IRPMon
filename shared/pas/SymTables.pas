Unit SymTables;

Interface

Uses
  Generics.Collections,
  RefObject,
  DbgHelpDll;

Type
  TModuleSymbol = Class (TRefObject)
  Private
    FOffset : Uint64;
    FName : WideString;
  Public
    Constructor Create(AOffset:UInt64; AName:WideString); Reintroduce;

    Property Offset : UInt64 Read FOffset;
    Property Name : WideString Read FName;
  end;

  TSymTable = Class (TRefObject)
    Private
      FName : WideString;
      FNames : TRefObjectDictionary<UInt64, TModuleSymbol>;
      FImageSize : Cardinal;
      FTimeDateStamp : Cardinal;
      FChecksum : Cardinal;
      FSymType : SYM_TYPE;
    Protected
      Function GetSymbolCount:Integer;
    Public
      Constructor Create(AProcess:THandle; AFileName:WideString); Reintroduce;
      Destructor Destroy; Override;

      Function FindSymbol(Var AOffset:UInt64):TModuleSymbol;

      Class Function SymTypeToString(AType:SYM_TYPE):WideString;

      Property Name : WideString Read FName;
      Property Count : Integer Read GetSymbolCount;
      Property TimeDateStamp : Cardinal Read FTimeDateStamp;
      Property CheckSum : Cardinal Read FCheckSum;
      Property SymType:SYM_TYPE Read FSymType;
      Property ImageSize : Cardinal Read FImageSize;
    end;

  TModuleSymbolStore = Class
    Private
      FhProcess : THandle;
      FKeys : TList<WideString>;
      FSymTables : TRefObjectDictionary<WideString, TSymTable>;
    Protected
      Function GetModule(ABaseName:WideString):TSymTable;
      Function GetModuleCount:Integer;
      Function GetModuleByIndex(AIndex:Integer):TSymTable;
    Public
      Constructor Create(AProcess:THandle); Reintroduce;
      Destructor Destroy; Override;

      Function AddFile(AFileName:WideString):Boolean;
      Function AddDirectory(ADirName:WideString):Boolean;
      Function Delete(AModuleName:WideString):Boolean;

      Property Module [ABaseName:WideString] : TSymTable Read GetModule;
      Property ModuleByIndex [AIndex:Integer] : TSymTable Read GetModuleByIndex;
      Property ModuleCount : Integer Read GetModuleCount;
    end;

Implementation

Uses
  Windows,
  SysUtils;

(** TModuleSymbol **)

Constructor TModuleSymbol.Create(AOffset:UInt64; AName:WideString);
begin
Inherited Create;
FName := AName;
FOffset := AOffset;
end;

(** TSymTable **)

Function _EnumCallback(pSymInfo:PSYMBOL_INFOW; SymbolSize:Cardinal; UserContext:Pointer):BOOL; StdCall;
Var
  symName : WideString;
  table : TSymTable;
  offset : UInt64;
  ms : TModuleSymbol;
begin
table := TSymTable(UserContext);
If (pSymInfo.Address <> 0) Then
  begin
  SetLength(symName, pSymInfo.NameLen - 1);
  Move(pSymInfo.Name, PWideChar(symName)^, Length(symName)*SizeOf(WideChar));
  offset := pSymInfo.Address - pSymInfo.ModBase;
  Try
    Try
      ms := TModuleSymbol.Create(offset, symName);
      table.FNames.Add(offset, ms);
    Finally
      ms.Free;
      end;
  Except
    end;
  end;

Result := True;
end;

Constructor TSymTable.Create(AProcess:THandle; AFileName:WideString);
Var
  hLib : THandle;
  hFile : THandle;
  fileSize : Int64;
  symBase : Cardinal;
  info : IMAGEHLP_MODULE64W;
begin
Inherited Create;
hLib := 0;
symBase := 0;
hFile := INVALID_HANDLE_VALUE;
FName := AFileName;
FNames := TRefObjectDictionary<UInt64, TModuleSymbol>.Create;
Try
  hFile := CreateFileW(PWideChar(AFileName), GENERIC_READ, FILE_SHARE_READ, Nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  If hFile = INVALID_HANDLE_VALUE Then
    Raise Exception.Create(Format('Unable to open the file: %u', [GetLastError]));

  If Not GetFileSizeEx(hFile, fileSize) Then
    Raise Exception.Create(Format('Unable to get the file size: %u', [GetLastError]));

  hLib := LoadLibraryExW(PWideChar(AFileName), 0, DONT_RESOLVE_DLL_REFERENCES);
  If hLib = 0 Then
    Raise Exception.Create(Format('Unable to load library: %u', [GetLastError]));

  symBase := SymLoadModuleExW(AProcess, hFile, PWideChar(AFileName), Nil, hLib, fileSize, Nil, 0);
  If symBase = 0 Then
    Raise Exception.Create(Format('Unable to load symbols: %u', [GetLastError]));

  If Not SymEnumSymbolsW(AProcess, hLib, PWideChar(ExtractFileName(AFileName) + '!*'), _EnumCallback, Self) Then
    Raise Exception.Create(Format('Unable to enumerate symbols: %u', [GetLastError]));

  info.SizeOfStruct := SizeOf(info);
  If Not SymGetModuleInfoW64(AProcess, hLib, info) Then
    Raise Exception.Create(Format('Unable to get module info: %u', [GetLastError]));

  FImageSize := info.ImageSize;
  FTimeDateStamp := info.TimeDateStamp;
  FCheckSum := info.CheckSum;
  FSymType := info.SymType;
Finally
  If symBase <> 0 THen
    SymUnloadModule64(AProcess, hLib);

  If hLib <> 0 Then
    FreeLibrary(hLib);

  If hFile <> INVALID_HANDLE_VALUE Then
    CloseHandle(hFile);
  end;
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

Function TSymTable.FindSymbol(Var AOffset:UInt64):TModuleSymbol;
Var
  p : TPair<UInt64, TModuleSymbol>;
  minOffset : UInt64;
begin
Result := Nil;
For p In FNames Do
  begin
  If p.Key < AOffset Then
    begin
    If AOffset - p.Key < minOffset Then
      begin
      minOffset := AOffset - p.Key;
      Result := p.Value;
      end;
    end;
  end;

If Assigned(Result) Then
  begin
  AOffset := minOffset;
  Result.Reference;
  end;
end;

Class Function TSymTable.SymTypeToString(AType:SYM_TYPE):WideString;
begin
Case AType Of
  SymCoff : Result := 'COFF';
  SymCv : Result := 'CodeView';
  SymDeferred : Result := 'Deferred';
  SymExport : Result := 'Exports';
  SymNone : Result := 'None';
  SymPdb : Result := 'PDB';
  SymSym : Result := 'SYM';
  Else Result := Format('%u', [Ord(AType)]);
  end;
end;

(** TModuleSymbolStore **)

Constructor TModuleSymbolStore.Create(AProcess:THandle);
begin
Inherited Create;
FhProcess := AProcess;
FSymTables := TRefObjectDictionary<WideString, TSymTable>.Create;
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
  st.Free;
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

Function TModuleSymbolStore.Delete(AModuleName:WideString):Boolean;
Var
  index : Integer;
  baseName : WideString;
begin
baseName := WideUpperCase(ExtractFileName(AModuleName));
index := FKeys.IndexOf(baseName);
Result := index <> -1;
If Result Then
  begin
  FKeys.Delete(index);
  FSymTables.Remove(baseName);
  end;
end;

End.

