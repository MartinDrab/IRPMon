Unit DbgHelpDll;

{$Z4}
{$MINENUMSIZE 4}

Interface

Uses
  Windows;

Const
  SYMFLAG_EXPORT          = $200;
  SYMFLAG_FUNCTION        = $800;
  SYMFLAG_VIRTUAL         = $1000;

  SLMFLAG_VIRTUAL         = $1;
  SLMFLAG_NO_SYMBOLS      = $4;

  sfImage                 = 0;
  sfDbg                   = 1;
  sfPdb                   = 2;
  sfMpd                   = 3;

Type
  _SYMBOL_INFOW = Record
    SizeOfStruct : Cardinal;
    TypeIndex : Cardinal;
    Reserved : Packed Array [0..1] Of UInt64;
    Index : Cardinal;
    Size : Cardinal;
    ModBase : UInt64;
    Flags : Cardinal;
    Value : UInt64;
    Address : UInt64;
    Register : Cardinal;
    Scope : Cardinal;
    Tag : Cardinal;
    NameLen : Cardinal;
    MaxNameLen : Cardinal;
    Name : WideChar;
    end;
  SYMBOL_INFOW = _SYMBOL_INFOW;
  PSYMBOL_INFOW = ^SYMBOL_INFOW;

  SymEnumeratesymbolsCallback = Function (pSymInfo:PSYMBOL_INFOW; SymbolSize:Cardinal; UserContext:Pointer):BOOL; StdCall;

Function SymInitializeW(hProcess:THandle; UserSearchPath:PWideChar; fInvadeProcess:BOOL):BOOL; StdCall;
Function SymCleanup(hProcess:THandle):BOOL; StdCall;
Function SymEnumSymbolsW(hProcess:THandle; BaseOfDll:NativeUInt; Mask:PWideChar; EnumSymbolsCallback:SymEnumeratesymbolsCallback; UserContext:Pointer):BOOL; StdCall;
Function SymLoadModuleExW(hProcess:THandle; hFile:THandle; ImageName:PWideChar; ModuleName:PWideChar; BaseOfDll:UInt64; DllSize:Cardinal; Data:Pointer; Flags:Cardinal):Cardinal; StdCall;
Function SymUnloadModule64(hProcess:THandle; BaseOfDll:UInt64):BOOL; StdCall;
Function SymGetSymbolFileW(hProcess:THandle; SymPath:PWideChar; ImageFile:PWideChar; FileType:Cardinal; SymbolFile:PWideChar; cSymbolFile:NativeUInt; DbgFile:PWideChar; cDebugFile:NativeUInt):BOOL; StdCall;

Implementation

Const
  LibraryName = 'dbghelp.dll';

Function SymInitializeW(hProcess:THandle; UserSearchPath:PWideChar; fInvadeProcess:BOOL):BOOL; StdCall; External LibraryName;
Function SymCleanup(hProcess:THandle):BOOL; StdCall; External LibraryName;
Function SymEnumSymbolsW(hProcess:THandle; BaseOfDll:NativeUInt; Mask:PWideChar; EnumSymbolsCallback:SymEnumeratesymbolsCallback; UserContext:Pointer):BOOL; StdCall; External LibraryName;
Function SymLoadModuleExW(hProcess:THandle; hFile:THandle; ImageName:PWideChar; ModuleName:PWideChar; BaseOfDll:UInt64; DllSize:Cardinal; Data:Pointer; Flags:Cardinal):Cardinal; StdCall; External LibraryName;
Function SymUnloadModule64(hProcess:THandle; BaseOfDll:UInt64):BOOL; StdCall; External LibraryName;
Function SymGetSymbolFileW(hProcess:THandle; SymPath:PWideChar; ImageFile:PWideChar; FileType:Cardinal; SymbolFile:PWideChar; cSymbolFile:NativeUInt; DbgFile:PWideChar; cDebugFile:NativeUInt):BOOL; StdCall; External LibraryName;


End.
