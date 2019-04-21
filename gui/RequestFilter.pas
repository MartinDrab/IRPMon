Unit RequestFilter;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Generics.Collections,
  RequestListModel, IRPMonDll;

Type
  ERequestFilterOperator = (
    rfoEquals,
    rfoNotEquals,
    rfoLowerEquals,
    rfoGreaterEquals,
    rfoLower,
    rfoGreater,
    rfoContains,
    rfoDoesNotContain,
    rfoBegins,
    rfoDoesNotBegin,
    rfoEnds,
    rfoDoesNotEnd,
    rfoAlwaysTrue
  );

  RequestFilterOperatorSet = Set Of ERequestFilterOperator;

Const
  RequestFilterIntegerOperators : RequestFilterOperatorSet = [
    rfoEquals,
    rfoNotEquals,
    rfoLowerEquals,
    rfoGreaterEquals,
    rfoLower,
    rfoGreater,
    rfoAlwaysTrue
  ];

  RequestFilterStringOperators : RequestFilterOperatorSet = [
    rfoEquals,
    rfoNotEquals,
    rfoContains,
    rfoDoesNotContain,
    rfoBegins,
    rfoDoesNotBegin,
    rfoEnds,
    rfoDoesNotEnd,
    rfoAlwaysTrue
  ];

Type
  EFilterAction = (
    ffaUndefined,
    ffaInclude,
    ffaNoExclude,
    ffaHighlight,
    ffaPassToFilter
  );

  TRequestFilter = Class
  Private
    FHighlightColor : Cardinal;
    FNextFilter : TRequestFilter;
    FPreviousFilter : TRequestFilter;
    FName : WideString;
    FField : ERequestListModelColumnType;
    FOp : ERequestFilterOperator;
    FStringValue : WideString;
    FIntValue : UInt64;
    FRequestType : ERequestType;
    FEnabled : Boolean;
    FAction : EFilterAction;
  Protected
    Procedure SetEnable(AValue:Boolean);
    Function AddNext(AFilter:TRequestFilter):Cardinal;
    Procedure RemoveFromChain;
    Procedure AddMapping(ASources:TList<UInt64>; ATargets:TList<WideString>; AIntValue:UInt64; AStringValue:WideString);
  Public
    Constructor Create(AName:WideString; ARequestType:ERequestType = ertUndefined); Reintroduce;

    Function GetPossibleValues(ASources:TList<UInt64>; ATargets:TList<WideString>; Var ABitmask:Boolean):Boolean; Virtual;

    Function Match(ARequest:TDriverRequest; AChainStart:Boolean = True):TRequestFilter;
    Function SetAction(AAction:EFilterAction; AHighlightColor:Cardinal = 0; ANextFilter:TRequestFilter = Nil):Cardinal;
    Function SetCondition(AColumn:ERequestListModelColumnType; AOperator:ERequestFilterOperator; AValue:UInt64):Boolean; Overload;
    Function SetCondition(AColumn:ERequestListModelColumnType; AOperator:ERequestFilterOperator; AValue:WideString):Boolean; Overload;

    Property Name : WideString Read FName Write FName;
    Property Field : ERequestListModelColumnType Read FField;
    Property Op : ERequestFilterOperator Read FOp;
    Property StringValue : WideString Read FStringValue;
    Property IntValue : UInt64 Read FIntValue;
    Property RequestType : ERequesttype Read FRequestType;
    Property Enabled : Boolean Read FEnabled Write SetEnable;
    Property Action : EFilterAction Read FAction;
    Property HighlightColor : Cardinal Read FHighlightColor;
  end;

  TIRPRequestFilter = Class (TRequestFilter)
  Public
    Constructor Create(AName:WideString); Reintroduce;

    Function GetPossibleValues(ASources:TList<UInt64>; ATargets:TList<WideString>; Var ABitmask:Boolean):Boolean; Override;
  end;

Implementation

Uses
  SysUtils;

(** TRequestFilter **)

Constructor TRequestFilter.Create(AName:WideString; ARequestType:ERequestType = ertUndefined);
begin
Inherited Create;
FName := AName;
FRequestType := ARequestType;
FOp := rfoAlwaysTrue;
FAction := ffaInclude;
FNextFilter := Nil;
FPreviousFilter := Nil;
end;

Function TRequestFilter.Match(ARequest:TDriverRequest; AChainStart:Boolean = True):TRequestFilter;
Var
  ret : Boolean;
  d : Pointer;
  l : Cardinal;
  iValue : UInt64;
  sValue : WideString;
begin
Result := Nil;
If (FEnabled) And
    ((Not AChainStart) Or (Not Assigned(FPreviousFIlter))) And
   ((FRequestType = ertUndefined) Or (ARequest.RequestType = FRequestType)) Then
  begin
  ret := ARequest.GetColumnValueRaw(FField, d, l);
  If ret Then
    begin
    iValue := 0;
    sValue := '';
    ret := False;
    Case RequestListModelColumnValueTypes[Ord(FField)] Of
      rlmcvtInteger,
      rlmcvtTime,
      rlmcvtMajorFunction,
      rlmcvtMinorFunction,
      rlmcvtProcessorMode,
      rlmcvtRequestType,
      rlmcvtIRQL : begin
        Move(d^, iValue, l);
        Case FOp Of
          rfoEquals: ret := (iValue = FIntValue);
          rfoNotEquals: ret := (iValue <> FIntValue);
          rfoLowerEquals: ret := (iValue <= FIntValue);
          rfoGreaterEquals: ret := (iValue >= FIntValue);
          rfoLower: ret := (iValue < FIntValue);
          rfoGreater: ret := (iValue > FIntValue);
          rfoAlwaysTrue: ret := True;
          end;
        end;
      rlmcvtString : begin
        sValue := WideCharToString(d);
        Case FOp Of
          rfoEquals: ret := (WideCompareText(sValue, FStringValue) = 0);
          rfoNotEquals: ret := (WideCompareText(sValue, FStringValue) <> 0);
          rfoLowerEquals: ret := (WideCompareText(sValue, FStringValue) <= 0);
          rfoGreaterEquals: ret := (WideCompareText(sValue, FStringValue) >= 0);
          rfoLower: ret := (WideCompareText(sValue, FStringValue) < 0);
          rfoGreater: ret := (WideCompareText(sValue, FStringValue) > 0);
          rfoContains: ret := (Pos(sValue, FStringValue) > 0);
          rfoDoesNotContain: ret := (Pos(sValue, FStringValue) <= 0);
          rfoBegins: ret := (Pos(sValue, FStringValue) = 1);
          rfoDoesNotBegin: ret := Pos(sValue, FStringValue) <> 1;
          rfoEnds: ret := (Pos(sValue, FStringValue) = Length(FStringValue) - Length(sValue) + 1);
          rfoDoesNotEnd: ret := (Pos(sValue, FStringValue) <> Length(FStringValue) - Length(sValue) + 1);
          rfoAlwaysTrue: ret := True;
          end;
        end;
      end;

    If ret Then
      begin
      Result := Self;
      If (FAction = ffaPassToFilter) And (Assigned(FNextFilter)) Then
        Result := FNextFilter.Match(ARequest, False);
      end;
    end;
  end;
end;


Procedure TRequestFilter.SetEnable(AValue:Boolean);
Var
  tmp : TRequestFilter;
begin
FEnabled := AValue;
tmp := FPreviousFilter;
While Assigned(tmp) Do
  begin
  tmp.FEnabled := AValue;
  tmp := tmp.FPreviousFilter;
  end;

tmp := FNextFilter;
While Assigned(tmp) Do
  begin
  tmp.FEnabled := AValue;
  tmp := tmp.FNextFilter;
  end;
end;

Function TRequestFilter.AddNext(AFilter:TRequestFilter):Cardinal;
begin
Result := 0;
If (FRequestType = ertUndefined) Or (FRequestType = AFilter.FRequestType) Then
  begin
  If (Not Assigned(AFilter.FNextFilter)) And
     (Not Assigned(AFilter.FPreviousFilter)) Then
    begin
    FAction := ffaPassToFilter;
    AFilter.FNextFilter := FNextFilter;
    AFilter.FPreviousFilter := Self;
    FNextFilter := AFIlter;
    end
  Else Result := 2;
  end
Else Result := 1;
end;

Procedure TRequestFilter.RemoveFromChain;
begin
If (Assigned(FNextFilter)) Or (Assigned(FPreviousFilter)) Then
  begin
  FAction := ffaInclude;
  If Assigned(FPreviousFilter) Then
    FPreviousFilter.FNextFilter := FNextFilter;

  If Assigned(FNextFilter) Then
    FNextFilter.FPreviousFilter := FPreviousFilter;

  FNextFilter := Nil;
  FPreviousFilter := Nil;
  end;
end;


Function TRequestFilter.SetAction(AAction:EFilterAction; AHighlightColor:Cardinal = 0; ANextFilter:TRequestFilter = Nil):Cardinal;
begin
Result := 0;
If FAction <> AAction Then
  begin
  If AAction = ffaPassToFilter Then
    begin
    If Assigned(ANextFilter) Then
      Result := AddNext(ANextFilter)
    Else Result := 3;
    end
  Else begin
    FAction := AAction;
    If FAction = ffaHighlight Then
      FHighlightColor := AHighlightColor;
    end;
  end
Else If (FAction = ffaHighlight) Then
  FHighlightColor := AHighlightColor;
end;

Function TRequestFilter.SetCondition(AColumn:ERequestListModelColumnType; AOperator:ERequestFilterOperator; AValue:UInt64):Boolean;
begin
Case RequestListModelColumnValueTypes[Ord(AColumn)] Of
  rlmcvtInteger,
  rlmcvtTime,
  rlmcvtMajorFunction,
  rlmcvtMinorFunction,
  rlmcvtProcessorMode,
  rlmcvtRequestType,
  rlmcvtIRQL : begin
    Result := (AOperator In RequestFilterIntegerOperators);
    If Result Then
      begin
      FField := AColumn;
      FOp := AOperator;
      FIntValue := AValue;
      end;
    end;
  Else Result := False;
  end;
end;

Function TRequestFilter.SetCondition(AColumn:ERequestListModelColumnType; AOperator:ERequestFilterOperator; AValue:WideString):Boolean;
begin
Result := (RequestListModelColumnValueTypes[Ord(AColumn)] = rlmcvtString);
If Result Then
  begin
  Result := (AOperator In RequestFilterStringOperators);
  If Result Then
    begin
    FField := AColumn;
    FOp := AOperator;
    FStringValue := AValue;
    end;
  end;
end;

Procedure TRequestFilter.AddMapping(ASources:TList<UInt64>; ATargets:TList<WideString>; AIntValue:UInt64; AStringValue:WideString);
begin
ASources.Add(AIntValue);
ATargets.Add(AStringValue);
end;

Function TRequestFIlter.GetPossibleValues(ASources:TList<UInt64>; ATargets:TList<WideString>; Var ABitmask:Boolean):Boolean;
begin
ABitmask := False;
Result := True;
Case RequestListModelColumnValueTypes[Ord(FField)] Of
  rlmcvtProcessorMode : begin
    AddMapping(ASources, ATargets, 0, 'KernelMode');
    AddMapping(ASources, ATargets, 0, 'UserMode');
    end;
  rlmcvtIRQL : begin
    AddMapping(ASources, ATargets, 0, 'Passive');
    AddMapping(ASources, ATargets, 1, 'APC');
    AddMapping(ASources, ATargets, 2, 'Dispatch');
{$IFDEF WIN32}
    AddMapping(ASources, ATargets, 27, 'Profile');
    AddMapping(ASources, ATargets, 28, 'Clock');
    AddMapping(ASources, ATargets, 29, 'IPI');
    AddMapping(ASources, ATargets, 30, 'Power');
    AddMapping(ASources, ATargets, 31, 'High');
{$ELSE}
    AddMapping(ASources, ATargets, 13, 'Clock');
    AddMapping(ASources, ATargets, 14, 'Profile, IPI, Power');
    AddMapping(ASources, ATargets, 15, 'High');
{$ENDIF}
    end;
  rlmcvtRequestType : begin
    AddMapping(ASources, ATargets, Ord(ertIRP), 'IRP');
    AddMapping(ASources, ATargets, Ord(ertIRPCompletion), 'IRPComp');
    AddMapping(ASources, ATargets, Ord(ertAddDevice), 'AddDevice');
    AddMapping(ASources, ATargets, Ord(ertDriverUnload), 'Unload');
    AddMapping(ASources, ATargets, Ord(ertFastIo), 'FastIo');
    AddMapping(ASources, ATargets, Ord(ertStartIo), 'StartIo');
    AddMapping(ASources, ATargets, Ord(ertDriverDetected), 'DriverDetected');
    AddMapping(ASources, ATargets, Ord(ertDeviceDetected), 'DeviceDetected');
    AddMapping(ASources, ATargets, Ord(ertFileObjectNameAssigned), 'FONameAssigned');
    AddMapping(ASources, ATargets, Ord(ertFileObjectNameDeleted), 'FONameDeleted');
    end;
  Else begin
    Result := False;
    end;
  end;
end;

(** TIRPRequestFilter **)

Constructor TIRPRequestFilter.Create(AName:WideString);
begin
Inherited Create(AName, ertIRP);
end;

Function TIRPRequestFilter.GetPossibleValues(ASources:TList<UInt64>; ATargets:TList<WideString>; Var ABitmask:Boolean):Boolean;
begin
ABitmask := False;
Result := True;
Case RequestListModelColumnValueTypes[Ord(FField)] Of
  rlmcvtMajorFunction : begin
    AddMapping(ASources, ATargets, 0, 'Create');
    AddMapping(ASources, ATargets, 1, 'CreateNamedPipe');
    AddMapping(ASources, ATargets, 2, 'Close');
    AddMapping(ASources, ATargets, 3, 'Read');
    AddMapping(ASources, ATargets, 4, 'Write');
    AddMapping(ASources, ATargets, 5, 'Query');
    AddMapping(ASources, ATargets, 6, 'Set');
    AddMapping(ASources, ATargets, 7, 'QueryEA');
    AddMapping(ASources, ATargets, 8, 'SetEA');
    AddMapping(ASources, ATargets, 9, 'Flush');
    AddMapping(ASources, ATargets, 10, 'QueryVolume');
    AddMapping(ASources, ATargets, 11, 'SetVolume');
    AddMapping(ASources, ATargets, 12, 'DirectoryControl');
    AddMapping(ASources, ATargets, 13, 'FSControl');
    AddMapping(ASources, ATargets, 14, 'DeviceControl');
    AddMapping(ASources, ATargets, 15, 'InternalDeviceControl');
    AddMapping(ASources, ATargets, 16, 'Shutdown');
    AddMapping(ASources, ATargets, 17, 'Lock');
    AddMapping(ASources, ATargets, 18, 'Cleanup');
    AddMapping(ASources, ATargets, 19, 'CreateMailslot');
    AddMapping(ASources, ATargets, 20, 'QuerySecurity');
    AddMapping(ASources, ATargets, 21, 'SetSecurity');
    AddMapping(ASources, ATargets, 22, 'Power');
    AddMapping(ASources, ATargets, 23, 'SystemControl');
    AddMapping(ASources, ATargets, 24, 'DeviceChange');
    AddMapping(ASources, ATargets, 25, 'QueryQuota');
    AddMapping(ASources, ATargets, 26, 'SetQuota');
    AddMapping(ASources, ATargets, 27, 'PnP');
    end;
  Else Result := Inherited GetPossibleValues(ASources, ATargets, ABitmask);
  end;
end;


End.

