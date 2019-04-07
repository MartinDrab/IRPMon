Unit DataParsers;

Interface

Uses
  Classes, Windows, Generics.Collections,
  IRPMonDll, RequestListModel;

Const
  DATA_PARSER_INIT_ROUTINE = 'DataParserInit';

Type
  TDataParserParseRoutine = Function (AHeader:PREQUEST_HEADER; ADriverName:PWideChar; ADeviceName:PWideChar; Var AHandled:ByteBool; Var ANames:PPWideChar; Var AValues:PPWideChar; Var ARowCount:Cardinal):Cardinal; Cdecl;
  TDataParserFreeRoutine = Procedure (ANames:PPWideChar; AValues:PPWideChar; ACount:Cardinal); Cdecl;

  _IRPMON_DATA_PARSER = Record
    Name : PWideChar;
    ParseRoutine : TDataParserParseRoutine;
    FreeRoutine : TDataParserFreeRoutine;
    MajorVersion : Cardinal;
    MinorVersion : Cardinal;
    BuildVersion : Cardinal;
    end;
  IRPMON_DATA_PARSER = _IRPMON_DATA_PARSER;
  PIRPMON_DATA_PARSER = ^IRPMON_DATA_PARSER;

  TDataParserInitRoutine = Function (Var AParser:IRPMON_DATA_PARSER):Cardinal; Cdecl;

  TDataParser = Class
    Private
      FLibraryHandle : THandle;
      FLibraryName : WideString;
      FName : WideString;
      FParseRoutine : TDataParserParseRoutine;
      FFreeRoutine : TDataParserFreeRoutine;
    Public
      Constructor Create(ALibraryHandle:THandle; ALibraryName:WideString; Var ARaw:IRPMON_DATA_PARSER); Reintroduce;
      Destructor Destroy; Override;
      Class Function CreateForLibrary(ALibraryName:WideString; Var AError:Cardinal):TDataParser;

      Function Parse(ARequest:TDriverRequest; ADriverName:WideString; ADeviceName:WideString; Var AHandled:Boolean; ANames:TStrings; AValues:TStrings):Cardinal;

      Property Name : WideString Read FName;
      Property LibraryName : WideString Read FLibraryName;
    end;


Function DataParserLoad(AFileName:WideString; Var AError:Cardinal; Var AParser:TDataParser):Boolean;

Implementation

Uses
  SysUtils;

Constructor TDataParser.Create(ALibraryHandle:THandle; ALibraryName:WideString; Var ARaw:IRPMON_DATA_PARSER);
begin
Inherited Create;
FParseRoutine := ARaw.ParseRoutine;
FFreeRoutine := ARaw.FreeRoutine;
FName := WideCharToString(ARaw.Name);
FLibraryName := ALibraryName;
FLibraryHandle := ALibraryHandle;
end;

Destructor TDataParser.Destroy;
begin
If FLibraryHandle <> 0 Then
  FreeLibrary(FLibraryHandle);

Inherited Destroy;
end;

Function TDataParser.Parse(ARequest:TDriverRequest; ADriverName:WideString; ADeviceName:WideString; Var AHandled:Boolean; ANames:TStrings; AValues:TStrings):Cardinal;
Var
  I : Integer;
  _handled : ByteBool;
  _ns : PPWideChar;
  _vs : PPWideChar;
  _rsCount : Cardinal;
  n : PPWideChar;
  v : PPWideChar;
begin
AHandled := False;
Result := FParseRoutine(ARequest.Raw, PWideChar(ADriverName), PWideChar(ADeviceName), _handled, _ns, _vs, _rsCount);
If (Result = 0) And (_handled) Then
  begin
  AHandled := True;
  n := _ns;
  v := _vs;
  For I := 0 To _rsCount - 1 Do
    begin
    ANames.Add(WideCharToString(n^));
    AValues.Add(WideCharToString(v^));
    Inc(n);
    Inc(v);
    end;

  FFreeRoutine(_ns, _vs, _rsCount);
  end;
end;

Class Function TDataParser.CreateForLibrary(ALibraryName:WideString; Var AError:Cardinal):TDataParser;
Var
  p : IRPMON_DATA_PARSER;
  initRoutine : TDataParserInitRoutine;
  lh : THandle;
begin
Result := Nil;
lh := LoadLibraryW(PWideChar(ALibraryName));
If lh <> 0 Then
  begin
  initRoutine := GetProcAddress(lh, DATA_PARSER_INIT_ROUTINE);
  If Assigned(initRoutine) Then
    begin
    AError := initRoutine(p);
    If AError = ERROR_SUCCESS Then
      begin
      Try
        Result := TDataParser.Create(lh, ALibraryName, p);
      Except
        Result := Nil;
        end;
      end;
    end;

  If Not Assigned(Result) Then
    FreeLibrary(lh);
  end;
end;

Function DataParserLoad(AFileName:WideString; Var AError:Cardinal; Var AParser:TDataParser):Boolean;
begin
AParser := TDataParser.CreateForLibrary(AFileName, AError);
Result := Assigned(AParser);
end;


End.
