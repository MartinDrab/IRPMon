Unit DataParsers;

Interface

Uses
  Classes, Windows, Generics.Collections,
  IRPMonDll, IRPMonRequest;

Const
  DATA_PARSER_INIT_ROUTINE     = 'DataParserInit';
  IRPMON_DATA_PARSER_VERSION_1 = 1;

Type
  _DP_REQUEST_EXTRA_INFO = Record
    DriverName : PWideChar;
    DeviceName : PWideChar;
    FileName : PWideChar;
    ProcessName : PWideChar;
    end;
  DP_REQUEST_EXTRA_INFO = _DP_REQUEST_EXTRA_INFO;
  PDP_REQUEST_EXTRA_INFO = ^DP_REQUEST_EXTRA_INFO;

  TDataParserParseRoutine = Function (AHeader:PREQUEST_HEADER; Var AExtraInfo:DP_REQUEST_EXTRA_INFO; Var AHandled:ByteBool; Var ANames:PPWideChar; Var AValues:PPWideChar; Var ARowCount:NativeUInt):Cardinal; Cdecl;
  TDataParserFreeRoutine = Procedure (ANames:PPWideChar; AValues:PPWideChar; ACount:NativeUInt); Cdecl;

  _IRPMON_DATA_PARSER = Record
    Version : Cardinal;
    Size : Cardinal;
    Name : PWideChar;
    Description : PWideChar;
    MajorVersion : Cardinal;
    MinorVersion : Cardinal;
    BuildVersion : Cardinal;
    Priority : Cardinal;
    ParseRoutine : TDataParserParseRoutine;
    FreeRoutine : TDataParserFreeRoutine;
    end;
  IRPMON_DATA_PARSER = _IRPMON_DATA_PARSER;
  PIRPMON_DATA_PARSER = ^IRPMON_DATA_PARSER;

  TDataParserInitRoutine = Function (ARequestedVersion:Cardinal; Var AParser:PIRPMON_DATA_PARSER):Cardinal; Cdecl;

  TDataParser = Class
    Private
      FLibraryHandle : THandle;
      FLibraryName : WideString;
      FName : WideString;
      FDescription : WideString;
      FParseRoutine : TDataParserParseRoutine;
      FFreeRoutine : TDataParserFreeRoutine;
      FPriority : Cardinal;
      FMajorVersion : Cardinal;
      FMinorVersion : Cardinal;
      FBuildNumber : Cardinal;
    Public
      Constructor Create(ALibraryHandle:THandle; ALibraryName:WideString; Var ARaw:IRPMON_DATA_PARSER); Reintroduce;
      Destructor Destroy; Override;
      Class Function CreateForLibrary(ALibraryName:WideString; Var AError:Cardinal):TDataParser;

      Function Parse(ARequest:TGeneralRequest; Var AHandled:ByteBool; ANames:TStrings; AValues:TStrings):Cardinal;

      Property Name : WideString Read FName;
      Property Description : WideString Read FDescription;
      Property LibraryName : WideString Read FLibraryName;
      Property Priority : Cardinal Read FPriority;
      Property MajorVersion : Cardinal Read FMajorVersion;
      Property MinorVersion : Cardinal Read FMinorVersion;
      Property BuildNumber : Cardinal Read FBuildNumber;
    end;


Procedure DataPrasersLoad(ADirectory:WideString; AList:TObjectList<TDataParser>);

Implementation

Uses
  SysUtils, RequestListModel;

Constructor TDataParser.Create(ALibraryHandle:THandle; ALibraryName:WideString; Var ARaw:IRPMON_DATA_PARSER);
begin
Inherited Create;
FParseRoutine := ARaw.ParseRoutine;
FFreeRoutine := ARaw.FreeRoutine;
FName := WideCharToString(ARaw.Name);
FDescription := WideCharToString(ARaw.Description);
FPriority := ARaw.Priority;
FMajorVersion := ARaw.MajorVersion;
FMinorVersion := ARaw.MinorVersion;
FBuildNumber := ARaw.BuildVersion;
FLibraryName := ALibraryName;
FLibraryHandle := ALibraryHandle;
end;

Destructor TDataParser.Destroy;
begin
If FLibraryHandle <> 0 Then
  FreeLibrary(FLibraryHandle);

Inherited Destroy;
end;

Function TDataParser.Parse(ARequest:TGeneralRequest; Var AHandled:ByteBool; ANames:TStrings; AValues:TStrings):Cardinal;
Var
  I : Integer;
  _handled : ByteBool;
  _ns : PPWideChar;
  _vs : PPWideChar;
  _rsCount : NativeUInt;
  n : PPWideChar;
  v : PPWideChar;
  ei : DP_REQUEST_EXTRA_INFO;
begin
AHandled := False;
ei.DriverName := PWideChar(ARequest.DriverName);
ei.DeviceName := PWideChar(ARequest.DeviceName);
ei.FileName := PWideChar(ARequest.FileName);
ei.ProcessName := PWideChar(ARequest.ProcessName);
Result := FParseRoutine(ARequest.Raw, ei, _handled, _ns, _vs, _rsCount);
If (Result = 0) And (_handled) Then
  begin
  AHandled := True;
  n := _ns;
  v := _vs;
  For I := 0 To _rsCount - 1 Do
    begin
    If Assigned(n) Then
      begin
      ANames.Add(WideCharToString(n^));
      Inc(n);
      end;

    AValues.Add(WideCharToString(v^));
    Inc(v);
    end;

  FFreeRoutine(_ns, _vs, _rsCount);
  end;
end;

Class Function TDataParser.CreateForLibrary(ALibraryName:WideString; Var AError:Cardinal):TDataParser;
Var
  p : PIRPMON_DATA_PARSER;
  initRoutine : TDataParserInitRoutine;
  lh : THandle;
begin
p := Nil;
Result := Nil;
initRoutine := Nil;
lh := LoadLibraryExW(PWideChar(ALibraryName), 0, DONT_RESOLVE_DLL_REFERENCES);
If lh <> 0 Then
  begin
  initRoutine := GetProcAddress(lh, DATA_PARSER_INIT_ROUTINE);
  FreeLibrary(lh);
  end;

If Assigned(initRoutine) Then
  begin
  lh := LoadLibraryW(PWideChar(ALibraryName));
  If lh <> 0 Then
    begin
    initRoutine := GetProcAddress(lh, DATA_PARSER_INIT_ROUTINE);
    If Assigned(initRoutine) Then
      begin
      AError := initRoutine(IRPMON_DATA_PARSER_VERSION_1, p);
      If AError = ERROR_SUCCESS Then
        begin
        Try
          Result := TDataParser.Create(lh, ALibraryName, p^);
        Except
          Result := Nil;
          end;

        HeapFree(GetProcessHeap, 0, p);
        end;
      end;
    end;

  If Not Assigned(Result) Then
    FreeLibrary(lh);
  end;
end;


Procedure DataPrasersLoad(ADirectory:WideString; AList:TObjectList<TDataParser>);
Var
  mask : WideString;
  hSearch : THandle;
  d : WIN32_FIND_DATAW;
  dp : TDataParser;
  e : Cardinal;
begin
If ADirectory[Length(ADirectory)] = '\' Then
  Delete(ADirectory, Length(ADirectory), 1);

mask := ADirectory + '\*.dll';
hSearch := FIndFirstFileW(PWideChar(mask), d);
If hSearch <> INVALID_HANDLE_VALUE THen
  begin
  Repeat
  If ((d.dwFileAttributes And FILE_ATTRIBUTE_DIRECTORY) = 0) Then
    begin
    dp := TDataParser.CreateForLibrary(ADirectory + '\' + d.cFileName, e);
    If Assigned(dp) THen
      AList.Add(dp);
    end;
  Until Not FindNextFileW(hSearch, d);

  Windows.FindClose(hSearch);
  end;
end;


End.

