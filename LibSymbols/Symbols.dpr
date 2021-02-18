Library Symbols;


uses
  Windows,
  System.SysUtils,
  System.Classes,
  SymTables in '..\shared\pas\SymTables.pas',
  DbgHelpDll in '..\shared\pas\DbgHelpDll.pas',
  ProcessList in '..\shared\pas\ProcessList.pas',
  RefObject in '..\shared\pas\RefObject.pas';


Type
  _SYM_TRANSLATION = Record
    Address : Pointer;
    ModuleName : PWideChar;
    FunctionName : PWideChar;
    Offset : NativeUInt;
    end;
  SYM_TRANSLATION = _SYM_TRANSLATION;
  PSYM_TRANSLATION = ^SYM_TRANSLATION;

{$R *.RES}


Function SymStoreCreate(ASymPath:PWideChar; Var AHandle:Pointer):Cardinal; Cdecl;
Var
  mss : TModuleSymbolStore;
  sp : WideString;
begin
sp := WideCharToString(ASymPath);
Try
  mss := TModuleSymbolStore.Create(GetCurrentProcess, sp);
  AHandle := mss;
  Result := 0;
Except
  Result := GetLastError;
  end;
end;

Procedure SymStoreFree(AHandle:Pointer); Cdecl;
Var
  mss : TModuleSymbolStore;
begin
mss := AHandle;
mss.Free;
end;

Function SymStoreAddFile(AHandle:Pointer; AFileName:PWideChar):Cardinal; Cdecl;
Var
  fn : WideString;
  mss : TModuleSymbolStore;
begin
Result := 0;
mss := TModuleSymbolStore(AHandle);
fn := WideCharToString(AFileName);
If Not mss.AddFile(fn) Then
  Result := GetLastError;
end;

Function SymStoreAddDirectory(AHandle:Pointer; ADirName:PWideChar; AMask:PWideChar):Cardinal; Cdecl;
Var
  dn : WideString;
  m : WideString;
  mss : TModuleSymbolStore;
begin
Result := 0;
mss := TModuleSymbolStore(AHandle);
dn := WideCharToString(ADirName);
If Assigned(AMask) Then
  m := WideCharToString(AMask)
Else m := '*';

If Not mss.AddDirectory(dn, m) Then
  Result := GetLastError;
end;

Function SymPathSetSymPath(AHandle:Pointer; APath:PWideChar):Cardinal; Cdecl;
Var
  p : WideString;
  mss : TModuleSymbolStore;
begin
Result := 0;
p := WideCharToString(APath);
mss := TModuleSymbolStore(AHandle);
If Not mss.SetSymPath(p) Then
  Result := GetLastError;
end;


Procedure SymTranslationFree(Var ATranslation:SYM_TRANSLATION); Cdecl;
begin
If Assigned(ATranslation.FunctionName) Then
  StrDispose(ATranslation.FunctionName);

If Assigned(ATranslation.ModuleName) Then
  StrDispose(ATranslation.ModuleName);
end;


Exports
  SymStoreCreate,
  SymStoreFree,
  SymStoreAddFile,
  SymStoreAddDirectory,
  SymPathSetSymPath,
  SymTranslationFree;

begin
end.
