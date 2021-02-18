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

  SSRVOPT_DWORD           = $2;
  SSRVOPT_DWORDPTR        = $4;
  SSRVOPT_GUIDPTR         = $8;

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
  PfindfileinpathcallbackW = Function (FileName:PWideChar; Context:Pointer):BOOL; StdCall;

  _SYM_TYPE = (
    SymNone,
    SymCoff,
    SymCv,
    SymPdb,
    SymExport,
    SymDeferred,
	  SymSym);
  SYM_TYPE = _SYM_TYPE;
  PSYM_TYPE = ^SYM_TYPE;

  _IMAGEHLP_MODULE64W = Record
    SizeOfStruct : Cardinal;
    BaseOfImage : UInt64;
    ImageSize : Cardinal;
    TimeDateStamp : Cardinal;
    CheckSum : Cardinal;
    NumSyms : Cardinal;
    SymType : SYM_TYPE;
    ModuleName : Packed Array [0..31] Of WideChar;
    ImageName : Packed Array [0..255] Of WideChar;
    LoadedImageName : Packed Array [0..255] Of WideChar;
    end;
  IMAGEHLP_MODULE64W  = _IMAGEHLP_MODULE64W;
  PIMAGEHLP_MODULE64W  = ^IMAGEHLP_MODULE64W;

Function SymInitializeW(hProcess:THandle; UserSearchPath:PWideChar; fInvadeProcess:BOOL):BOOL; StdCall;
Function SymCleanup(hProcess:THandle):BOOL; StdCall;
Function SymEnumSymbolsW(hProcess:THandle; BaseOfDll:NativeUInt; Mask:PWideChar; EnumSymbolsCallback:SymEnumeratesymbolsCallback; UserContext:Pointer):BOOL; StdCall;
Function SymLoadModuleExW(hProcess:THandle; hFile:THandle; ImageName:PWideChar; ModuleName:PWideChar; BaseOfDll:UInt64; DllSize:Cardinal; Data:Pointer; Flags:Cardinal):Cardinal; StdCall;
Function SymUnloadModule64(hProcess:THandle; BaseOfDll:UInt64):BOOL; StdCall;
Function SymGetSymbolFileW(hProcess:THandle; SymPath:PWideChar; ImageFile:PWideChar; FileType:Cardinal; SymbolFile:PWideChar; cSymbolFile:NativeUInt; DbgFile:PWideChar; cDebugFile:NativeUInt):BOOL; StdCall;
Function SymSrvGetFileIndexesW(AFile:PWideChar; Var Id:TGuid; Var Val1:Cardinal; Var Val2:Cardinal; flags:Cardinal):BOOL; StdCall;
Function SymFindFileInPathW(hProcess:THandle; SearchPath:PWideChar; FileName:PWideChar; Id:Pointer; two:Cardinal; three:Cardinal; flags:Cardinal; Found:PWideChar; callback:PfindfileinpathcallbackW; context:Pointer):BOOL; StdCall;
Function SymGetModuleInfoW64(hProcess:THandle; Addr:UInt64; Var ModInfo:IMAGEHLP_MODULE64W):BOOL; StdCall;
Function SymSetSearchPathW(hProcess:THandle; SearchPath:PWideChar):BOOL; StdCall;

Implementation

Const
  LibraryName = 'dbghelp.dll';

Function SymInitializeW(hProcess:THandle; UserSearchPath:PWideChar; fInvadeProcess:BOOL):BOOL; StdCall; External LibraryName;
Function SymCleanup(hProcess:THandle):BOOL; StdCall; External LibraryName;
Function SymEnumSymbolsW(hProcess:THandle; BaseOfDll:NativeUInt; Mask:PWideChar; EnumSymbolsCallback:SymEnumeratesymbolsCallback; UserContext:Pointer):BOOL; StdCall; External LibraryName;
Function SymLoadModuleExW(hProcess:THandle; hFile:THandle; ImageName:PWideChar; ModuleName:PWideChar; BaseOfDll:UInt64; DllSize:Cardinal; Data:Pointer; Flags:Cardinal):Cardinal; StdCall; External LibraryName;
Function SymUnloadModule64(hProcess:THandle; BaseOfDll:UInt64):BOOL; StdCall; External LibraryName;
Function SymGetSymbolFileW(hProcess:THandle; SymPath:PWideChar; ImageFile:PWideChar; FileType:Cardinal; SymbolFile:PWideChar; cSymbolFile:NativeUInt; DbgFile:PWideChar; cDebugFile:NativeUInt):BOOL; StdCall; External LibraryName;
Function SymSrvGetFileIndexesW(AFile:PWideChar; Var Id:TGuid; Var Val1:Cardinal; Var Val2:Cardinal; flags:Cardinal):BOOL; StdCall; External LibraryName;
Function SymFindFileInPathW(hProcess:THandle; SearchPath:PWideChar; FileName:PWideChar; Id:Pointer; two:Cardinal; three:Cardinal; flags:Cardinal; Found:PWideChar; callback:PfindfileinpathcallbackW; context:Pointer):BOOL; StdCall; External LibraryName;
Function SymGetModuleInfoW64(hProcess:THandle; Addr:UInt64; Var ModInfo:IMAGEHLP_MODULE64W):BOOL; StdCall; External LibraryName;
Function SymSetSearchPathW(hProcess:THandle; SearchPath:PWideChar):BOOL; StdCall; External LibraryName;


End.

