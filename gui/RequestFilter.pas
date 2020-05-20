Unit RequestFilter;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Generics.Collections, INIFiles,
  RequestListModel, IRPMonDll, DLLDecider;

Type
  RequestFilterOperatorSet = Set Of ERequestFilterOperator;

Const
  RequestFilterOperatorNames : Array [0..Ord(rfoDLLDecider)] Of WideString = (
    '==',
    '<=',
    '>=',
    '<',
    '>',
    'contains',
    'begins',
    'ends',
    'true',
    'DLL'
  );

  RequestFilterIntegerOperators : RequestFilterOperatorSet = [
    rfoEquals,
    rfoLowerEquals,
    rfoGreaterEquals,
    rfoLower,
    rfoGreater,
    rfoAlwaysTrue,
    rfoDLLDecider
  ];

  RequestFilterStringOperators : RequestFilterOperatorSet = [
    rfoEquals,
    rfoContains,
    rfoBegins,
    rfoEnds,
    rfoAlwaysTrue,
    rfoDLLDecider
  ];

Const
  RequestFilterActionNames : Array [0..Ord(ffaPassToFilter)] Of WideString = (
    'Highlight',
    'Include',
    'Exclude',
    'Pass'
  );


Type
  TRequestFilter = Class
  Private
    FDLLDecider : TDLLDecider;
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
    FEphemeral : Boolean;
    FAction : EFilterAction;
    FNegate : Boolean;
    FColumnName : WideString;
    FRequestPrototype : TDriverRequest;
  Protected
    Procedure SetEnable(AValue:Boolean);
    Procedure RemoveFromChain;
    Procedure AddMapping(ASources:TList<UInt64>; ATargets:TList<WideString>; AIntValue:UInt64; AStringValue:WideString);
  Public
    Constructor Create(AName:WideString; ARequestType:ERequestType = ertUndefined); Reintroduce;
    Destructor Destroy; Override;

    Procedure GenerateName(AList:TObjectList<TRequestFilter> = Nil);
    Function GetPossibleValues(ASources:TList<UInt64>; ATargets:TList<WideString>; Var ABitmask:Boolean):Boolean; Virtual;
    Function SupportedOperators:RequestFilterOperatorSet; Virtual;
    Function Copy:TRequestFilter;
    Function Match(ARequest:TDriverRequest; Var AResultAction:EFilterAction; Var AHiglightColor:Cardinal; AChainStart:Boolean = True):TRequestFilter;
    Function SetAction(AAction:EFilterAction; AHighlightColor:Cardinal = $FFFFFF):Cardinal;
    Function AddNext(AFilter:TRequestFilter):Cardinal;
    Function SetCondition(AColumn:ERequestListModelColumnType; AOperator:ERequestFilterOperator; AValue:UInt64):Boolean; Overload;
    Function SetCondition(AColumn:ERequestListModelColumnType; AOperator:ERequestFilterOperator; AValue:WideString):Boolean; Overload;
    Function HasPredecessor:Boolean;

    Class Function NewInstance(ARequestType:ERequestType):TRequestFilter; Reintroduce; Overload;
    Class Function LoadList(AFileName:WideString; AList:TList<TRequestFilter>):Boolean;
    Class Function SaveList(AFileName:WideString; AList:TList<TRequestFilter>):Boolean;
    Class Function GetByName(AName:WideString; AList:TList<TRequestFilter>):TRequestFilter;
    Function Save(AFile:TIniFile):Boolean;

    Property Name : WideString Read FName Write FName;
    Property Field : ERequestListModelColumnType Read FField;
    Property Op : ERequestFilterOperator Read FOp;
    Property StringValue : WideString Read FStringValue;
    Property IntValue : UInt64 Read FIntValue;
    Property RequestType : ERequesttype Read FRequestType;
    Property Enabled : Boolean Read FEnabled Write SetEnable;
    Property Ephemeral : Boolean Read FEphemeral Write FEphemeral;
    Property Action : EFilterAction Read FAction;
    Property HighlightColor : Cardinal Read FHighlightColor;
    Property Negate : Boolean Read FNegate Write FNegate;
    Property ColumnName : WideString Read FColumnName;
    Property NextFilter : TRequestFilter Read FNextFilter;
  end;

  TIRPRequestFilter = Class (TRequestFilter)
  Public
    Constructor Create(AName:WideString); Reintroduce;

    Function GetPossibleValues(ASources:TList<UInt64>; ATargets:TList<WideString>; Var ABitmask:Boolean):Boolean; Override;
  end;

  TIRPCompletionRequestFilter = Class (TRequestFilter)
  Public
    Constructor Create(AName:WideString); Reintroduce;

    Function GetPossibleValues(ASources:TList<UInt64>; ATargets:TList<WideString>; Var ABitmask:Boolean):Boolean; Override;
  end;

Implementation

Uses
  Classes, SysUtils, IRPRequest, FastIoRequest, FileObjectNameXXXRequest,
  XXXDetectedRequests, ProcessXXXRequests,
  Utils;

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
FHighlightColor := $FFFFFF;
FRequestPrototype := TDriverRequest.CreatePrototype(FRequestType);
end;

Destructor TRequestFilter.Destroy;
begin
RemoveFromChain;
If Assigned(FRequestPrototype) Then
  FRequestPrototype.Free;

If Assigned(FDLLDecider) Then
  FDLLDecider.Free;

Inherited Destroy;
end;

Function TRequestFilter.Save(AFile:TIniFile):Boolean;
begin
Try
  AFile.WriteInteger(FName, 'RequestType', Ord(FRequestType));
  AFile.WriteInteger(FName, 'Column', Ord(FField));
  AFile.WriteInteger(FName, 'Operator', Ord(FOp));
  AFile.WriteString(FName, 'Value', FStringValue);
  AFile.WriteInteger(FName, 'Action', Ord(FAction));
  AFile.WriteBool(FName, 'Enabled', FEnabled);
  AFIle.WriteBool(FName, 'Negate', FNegate);
  AFile.WriteInteger(FName, 'Color', FHighlightCOlor);
  If Assigned(FNextFilter) Then
    AFile.WriteString(FName, 'Next', FNextFilter.Name);

  Result := True;
Except
  Result := False;
  end;
end;

Procedure TRequestFilter.GenerateName(AList:TObjectList<TRequestFilter> = Nil);
Var
  I : Integer;
  tmpName : WideString;
begin
I := 0;
tmpName := FName;
While (tmpName = '') Or (Assigned(AList) And Assigned(GetByName(tmpName, AList))) Do
  begin
  tmpName := TDriverRequest.RequestTypeToString(FRequestType) + '-' +
    FColumnName + '-' +
    RequestFilterOperatorNames[Ord(FOp)] + '-' +
    FStringValue + '-' +
    BoolToStr(FNegate) + '-' +
    RequestFilterActionNames[Ord(FAction)];

  If I <> 0 Then
    tmpName := tmpName + '-' + IntToStr(Int64(I));

  Inc(I);
  end;

FName := tmpName;
end;

Class Function TRequestFilter.LoadList(AFileName:WideString; AList:TList<TRequestFilter>):Boolean;
Var
  names : TStringList;
  rf : TRequestFilter;
  tmp : TRequestFilter;
  I : Integer;
  _name : WideString;
  _enabled : Boolean;
  _negate : Boolean;
  _type : ERequestType;
  _column : ERequestListModelColumnType;
  _op : ERequestFilterOperator;
  _value : WideString;
  _action : EFilterAction;
  _color : Cardinal;
  _next : WideString;
  nextNames : TStringList;
  iniFile : TIniFile;
begin
Result := True;
iniFile := Nil;
nextNames := Nil;
names := Nil;
Try
  iniFile := TIniFile.Create(AFileName);
  nextNames := TStringList.Create;
  names := TStringList.Create;
  Try
    iniFile.ReadSections(names);
    For _name In names Do
      begin
      Try
        _type := ERequestType(iniFile.ReadInteger(_name, 'RequestType', -1));
        _column := ERequestListModelColumnType(iniFile.ReadInteger(_name, 'Column', -1));
        _op := ERequestFilterOperator(iniFile.ReadInteger(_name, 'Operator', -1));
        _value := iniFile.ReadString(_name, 'Value', '');
        _action := EFilterAction(iniFile.ReadInteger(_name, 'Action', -1));
        _enabled := iniFile.ReadBool(_name, 'Enabled', True);
        _negate := iniFile.ReadBool(_name, 'Negate', False);
        _color := iniFile.ReadInteger(_name, 'Color', $FFFFFF);
        _next := iniFile.ReadString(_name, 'Next', '');
        rf := GetByName(_name, AList);
        If Assigned(rf) Then
          Continue;

        rf := TRequestFilter.NewInstance(_type);
        rf.Name := _name;
        rf.Enabled := _enabled;
        rf.Negate := _negate;
        rf.SetCondition(_column, _op, _value);
        rf.SetAction(_action, _color);
        AList.Add(rf);
        nextNames.Add(_next);
      Except
        Result := False;
        end;
      end;

    If Result Then
      begin
      For I := 0 To AList.Count - 1 Do
        begin
        _next := nextNames[I];
        If _next <> '' Then
          begin
          rf := AList[I];
          tmp := GetByName(_next, AList);
          If (Assigned(tmp)) And (rf.Action = ffaPassToFilter) THen
            rf.AddNext(tmp);
          end;
        end;
      end;
  Except
    Result := False;
    end;
Finally
  names.Free;
  nextNames.Free;
  iniFile.Free;
  end;
end;

Class Function TRequestFilter.SaveList(AFileName:WideString; AList:TList<TRequestFilter>):Boolean;
Var
  section : WideString;
  names : TStringList;
  rf : TRequestFilter;
  iniFile : TIniFile;
begin
Result := True;
iniFile := Nil;
names := Nil;
Try
  iniFile := TIniFile.Create(AFileName);
  names := TStringList.Create;
  Try
    iniFile.ReadSections(names);
    For section In names Do
      iniFile.EraseSection(section);

    For rf  In AList Do
      begin
      If Not rf.Ephemeral Then
        begin
        Result := rf.Save(iniFile);
        If Not Result Then
          Break;
        end;
      end;
  Except
    Result := False;
    end;
Finally
  names.Free;
  iniFile.Free;
  end;
end;

Class Function TRequestFilter.GetByName(AName:WideString; AList:TList<TRequestFilter>):TRequestFilter;
Var
  tmp : TRequestFilter;
begin
Result := Nil;
For tmp In AList Do
  begin
  If tmp.Name = AName Then
    begin
    Result := tmp;
    Break;
    end;
  end;
end;

Function TRequestFilter.HasPredecessor:Boolean;
begin
Result := Assigned(FPreviousFilter);
end;

Function TRequestFilter.Match(ARequest:TDriverRequest; Var AResultAction:EFilterAction; Var AHiglightColor:Cardinal; AChainStart:Boolean = True):TRequestFilter;
Var
  ret : Boolean;
  d : Pointer;
  l : Cardinal;
  iValue : UInt64;
  sValue : WideString;
  stringConstant : WideString;
  dllDecision : DLL_DECIDER_DECISION;
begin
Result := Nil;
If (FEnabled) And
    ((Not AChainStart) Or (Not Assigned(FPreviousFIlter))) And
   ((FRequestType = ertUndefined) Or (ARequest.RequestType = FRequestType)) Then
  begin
  If FOp <> rfoDLLDecider Then
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
            rfoLowerEquals: ret := (iValue <= FIntValue);
            rfoGreaterEquals: ret := (iValue >= FIntValue);
            rfoLower: ret := (iValue < FIntValue);
            rfoGreater: ret := (iValue > FIntValue);
            rfoAlwaysTrue: ret := True;
            end;
          end;
        rlmcvtString : begin
          sValue := WideUpperCase(WideCharToString(d));
          stringConstant := WideUpperCase(FStringValue);
          Case FOp Of
            rfoEquals: ret := (WideCompareText(sValue, stringConstant) = 0);
            rfoContains: ret := (Pos(stringConstant, sValue) > 0);
            rfoBegins: ret := (Pos(stringConstant, sValue) = 1);
            rfoEnds: ret := (Pos(stringConstant, sValue) = Length(sValue) - Length(stringConstant) + 1);
            rfoAlwaysTrue: ret := True;
            end;
          end;
        end;

      If FNegate Then
        ret := Not ret;

      If ret Then
        begin
        AResultAction := FAction;
        AHiglightColor := FHighlightColor;
        end;
      end;
    end
  Else begin
    dllDecision.Action := FAction;
    dllDecision.HiglightColor := FHighlightColor;
    dllDecision.Decided := False;
    dllDecision.OverrideFilter := False;
    ret := (FDLLDecider.Decide(ARequest, dllDecision) = 0);
    If ret Then
      ret := dllDecision.Decided;

    If ret Then
      begin
      AResultAction := FAction;
      AHiglightColor := FHighlightColor;
      If dllDecision.OverrideFilter Then
        begin
        AResultAction := dllDecision.Action;
        AHiglightColor := dllDecision.HiglightColor;
        end;
      end;
    end;

  If ret Then
    begin
    Result := Self;
    If (AResultAction = ffaPassToFilter) And (Assigned(FNextFilter)) Then
      Result := FNextFilter.Match(ARequest, AResultAction, AHiglightColor, False);
    end;
  end;
end;

Procedure TRequestFilter.SetEnable(AValue:Boolean);
Var
  tmp : TRequestFilter;
begin
FEnabled := AValue;
tmp := FPreviousFilter;
While Assigned(tmp) And (tmp <> Self) Do
  begin
  tmp.FEnabled := AValue;
  tmp := tmp.FPreviousFilter;
  end;

tmp := FNextFilter;
While Assigned(tmp) And (tmp <> Self) Do
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
  If Assigned(AFilter.FPreviousFilter) Then
    begin
    AFilter.FPreviousFilter.FNextFilter := Nil;
    AFilter.FPreviousFilter := Nil;
    end;

  If Assigned(FNextFilter) Then
    begin
    FNextFilter.FPreviousFilter := Nil;
    FNextFilter := Nil;
    end;

  FAction := ffaPassToFilter;
  AFilter.FPreviousFilter := Self;
  FNextFilter := AFIlter;
  end
Else Result := 1;
end;

Procedure TRequestFilter.RemoveFromChain;
begin
If (Assigned(FNextFilter)) Or (Assigned(FPreviousFilter)) Then
  begin
  If Assigned(FPreviousFilter) Then
    begin
    FPreviousFilter.FNextFilter := FNextFilter;
    If Not Assigned(FNextFilter) Then
      FPreviousFilter.FAction := ffaInclude;
    end;

  If Assigned(FNextFilter) Then
    begin
    FNextFilter.FPreviousFilter := FPreviousFilter;
    FAction := ffaInclude;
    end;

  FNextFilter := Nil;
  FPreviousFilter := Nil;
  end;
end;


Function TRequestFilter.SetAction(AAction:EFilterAction; AHighlightColor:Cardinal = $FFFFFF):Cardinal;
begin
Result := 0;
If (FAction = ffaPassToFilter) And (FAction <> AAction) Then
  begin
  If Assigned(FNextFilter) Then
    begin
    FNextFilter.FPreviousFilter := Nil;
    FNextFilter := Nil;
    end;
  end;

FAction := AAction;
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
      FstringValue := UIntToStr(AValue);
      end;
    end;
  Else Result := False;
  end;

If Result Then
  FColumnName := FRequestPrototype.GetColumnName(AColumn);
end;

Function TRequestFilter.SetCondition(AColumn:ERequestListModelColumnType; AOperator:ERequestFilterOperator; AValue:WideString):Boolean;
Var
  tmpDecider : TDLLDecider;
  dllSeparatorIndex : Integer;
  modName : WideString;
  routineName : WideString;
begin
Result := False;
If AOperator <> rfoDLLDecider Then
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
        Try
          FIntValue := StrToInt64(AValue);
          FstringValue := AValue;
          FField := AColumn;
          FOp := AOperator;
        Except
          Result := False;
          end;
        end;
      end;
    rlmcvtString : begin
      Result := (AOperator In RequestFilterStringOperators);
      If Result Then
        begin
        FField := AColumn;
        FOp := AOperator;
        FStringValue := AValue;
        end;
      end;
    end;
  end
Else begin
  FField := AColumn;
  FOp := AOperator;
  FStringValue := AValue;
  dllSeparatorIndex := Pos('!', FStringValue);
  If dllSeparatorIndex >= 1 Then
    begin
    modName := System.Copy(FStringValue, 1, dllSeparatorIndex - 1);
    routineName := System.Copy(FStringValue, dllSeparatorIndex + 1, Length(FStringValue) - dllSeparatorIndex);
    tmpDecider := TDLLDecider.NewInstance(modName, routineName);
    end
  Else tmpDecider := TDLLDecider.NewInstance(FStringValue);

  Result := Assigned(tmpDecider);
  If Result Then
    begin
    If Assigned(FDLLDecider) Then
      FDLLDecider.Free;

    FDLLDecider := tmpDecider;
    end;
  end;

If Result Then
  FColumnName := FRequestPrototype.GetColumnName(AColumn);
end;

Function TRequestFilter.SupportedOperators:RequestFilterOperatorSet;
begin
Case RequestListModelColumnValueTypes[Ord(FField)] Of
  rlmcvtInteger,
  rlmcvtTime,
  rlmcvtMajorFunction,
  rlmcvtMinorFunction,
  rlmcvtProcessorMode,
  rlmcvtIRQL,
  rlmcvtRequestType : Result := RequestFilterIntegerOperators;
  rlmcvtString : Result := RequestFilterStringOperators;
  Else Result := [rfoAlwaysTrue];
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
    AddMapping(ASources, ATargets, 1, 'UserMode');
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
    AddMapping(ASources, ATargets, Ord(ertUndefined), '<all>');
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
    AddMapping(ASources, ATargets, Ord(ertProcessCreated), 'ProcessCreate');
    AddMapping(ASources, ATargets, Ord(ertProcessExitted), 'ProcessExit');
    AddMapping(ASources, ATargets, Ord(ertImageLoad), 'ImageLoad');
    end;
  Else Result := False;
  end;
end;

Class Function TRequestFilter.NewInstance(ARequestType:ERequestType):TRequestFilter;
begin
Case ARequestType Of
  ertUndefined: Result := TRequestFilter.Create('', ARequestType);
  ertIRP: Result := TIRPRequestFilter.Create('');
  ertIRPCompletion : Result := TIRPCompletionRequestFilter.Create('');
  Else Result := TRequestFilter.Create('', ARequestType);
  end;
end;

Function TRequestFilter.Copy:TRequestFilter;
begin
Result := NewInstance(FRequestType);
If Assigned(Result) Then
  begin
  Result.FName := FName;
  Result.FHighlightColor := FHighlightColor;
  Result.FField := FField;
  Result.FOp := FOp;
  Result.FStringValue := FStringValue;
  Result.FIntValue := FIntValue;
  Result.FEnabled := FEnabled;
  Result.FEphemeral := FEphemeral;
  Result.FAction := FAction;
  Result.FNegate := FNegate;
  Result.FColumnName := FColumnName;
  If Assigned(FDLLDecider) Then
    Result.FDLLDecider := TDLLDecider.NewInstance(FDLLDecider.ModuleName, FDLLDecider.DecideRoutineName);
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


(** TIRPCompletionRequestFilter **)

Constructor TIRPCompletionRequestFilter.Create(AName:WideString);
begin
Inherited Create(AName, ertIRPCompletion);
end;

Function TIRPCompletionRequestFilter.GetPossibleValues(ASources:TList<UInt64>; ATargets:TList<WideString>; Var ABitmask:Boolean):Boolean;
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

