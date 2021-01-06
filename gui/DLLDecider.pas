Unit DLLDecider;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

{$Z4}
{$MINENUMSIZE 4}

Interface


Uses
  Windows, SysUtils, DataParsers,
  IRPMonDll, IRPMonRequest;

Type
  _DLL_DECIDER_DECISION = Record
    Action : EFilterAction;
    HiglightColor : Cardinal;
    Decided : ByteBool;
    OverrideFilter : ByteBool;
    end;
  DLL_DECIDER_DECISION = _DLL_DECIDER_DECISION;
  PDLL_DECIDER_DECISION = ^DLL_DECIDER_DECISION;

  TDLL_DECIDER_DECIDE_ROUTINE = Function(ARequest:PREQUEST_GENERAL; Var AExtraInfo:DP_REQUEST_EXTRA_INFO; Var ADecision:DLL_DECIDER_DECISION):Cardinal; Cdecl;

  TDLLDecider = Class
  Private
    FModuleName : WideString;
    FModuleHandle : THandle;
    FDecideRoutineName : WideString;
    FDecideRoutine : TDLL_DECIDER_DECIDE_ROUTINE;
  Public
    Class Function NewInstance(ALibraryName:WideString; ARoutineName:WideString = 'DecideRoutine'):TDLLDecider; Reintroduce;

    Destructor Destroy; Override;

    Function Decide(ARequest:TDriverRequest; Var ADecision:DLL_DECIDER_DECISION):Cardinal;

    Property ModuleName : WideString Read FModuleName;
    Property DecideRoutineName : WideString Read FDecideRoutineName;
  end;


Implementation

Class Function TDLLDecider.NewInstance(ALibraryName:WideString; ARoutineName:WideString = 'DecideRoutine'):TDLLDecider;
begin
Result := TDLLDecider.Create;
Try
  Result.FModuleName := ALibraryName;
  Result.FDecideRoutineName := ARoutineName;
  Result.FModuleHandle := LoadLibraryW(PWideChar(ALibraryName));
  If Result.FModuleHandle = 0 Then
    Raise Exception.Create('Decider DLL failed to load');

  Result.FDecideRoutine := GetProcAddress(Result.FModuleHandle, PAnsiChar(AnsiString(Result.FDecideRoutineName)));
  If Not Assigned(Result.FDecideRoutine) Then
    Raise Exception.Create('Decider routine not found');
Except
  FreeAndNil(Result);
  end;
end;


Destructor TDLLDecider.Destroy;
begin
If FModuleHandle <> 0 Then
  FreeLibrary(FModuleHandle);

Inherited Destroy;
end;


Function TDLLDecider.Decide(ARequest:TDriverRequest; Var ADecision:DLL_DECIDER_DECISION):Cardinal;
Var
  ei : DP_REQUEST_EXTRA_INFO;
begin
ei.DriverName := PWideChar(ARequest.DriverName);
ei.DeviceName := PWideChar(ARequest.DeviceName);
ei.FileName := PWideChar(ARequest.FileName);
ei.ProcessName := PWideChar(ARequest.ProcessName);
Result := FDecideRoutine(PREQUEST_GENERAL(ARequest.Raw), ei, ADecision);
end;



End.
