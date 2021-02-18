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


Function SymStoreCreate(ASymPath:PWideChar; Var AHandle:Pointer):BOOL; Cdecl;
Var
  mss : TModuleSymbolStore;
  sp : WideString;
begin
sp := WideCharToString(ASymPath);
Try
  mss := TModuleSymbolStore.Create(GetCurrentProcess, sp);
  AHandle := mss;
  Result := True;
Except
  Result := False;
  end;
end;

Procedure SymStoreFree(AHandle:Pointer); Cdecl;
Var
  mss : TModuleSymbolStore;
begin
mss := TModuleSymbolStore(AHandle);
mss.Free;
end;

Function SymStoreAddFile(AHandle:Pointer; AFileName:PWideChar):BOOL; Cdecl;
Var
  fn : WideString;
  mss : TModuleSymbolStore;
begin
mss := TModuleSymbolStore(AHandle);
fn := WideCharToString(AFileName);
Result := mss.AddFile(fn);
end;

Function SymStoreAddDirectory(AHandle:Pointer; ADirName:PWideChar; AMask:PWideChar):BOOL; Cdecl;
Var
  dn : WideString;
  m : WideString;
  mss : TModuleSymbolStore;
begin
mss := TModuleSymbolStore(AHandle);
dn := WideCharToString(ADirName);
If Assigned(AMask) Then
  m := WideCharToString(AMask)
Else m := '*';

Result := mss.AddDirectory(dn, m);
end;

Function SymPathSetSymPath(AHandle:Pointer; APath:PWideChar):BOOL; Cdecl;
Var
  p : WideString;
  mss : TModuleSymbolStore;
begin
p := WideCharToString(APath);
mss := TModuleSymbolStore(AHandle);
Result := mss.SetSymPath(p);
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
