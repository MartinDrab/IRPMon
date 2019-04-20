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
  Public
    Procedure GetPossibleValues(AValues:TDictionary<UInt64, WideString>); Virtual; Abstract;

    Function Match(ARequest:TDriverRequest; AChainStart:Boolean = True):TRequestFilter;

    Property Name : WideString Read FName;
    Property Field : ERequestListModelColumnType Read FField;
    Property Op : ERequestFilterOperator Read FOp;
    Property StringValue : WideString Read FStringValue;
    Property IntValue : UInt64 Read FIntValue;
    Property RequestType : ERequesttype Read FRequestType;
    Property Enabled : Boolean Read FEnabled;
    Property Action : EFilterAction Read FAction;
  end;


Implementation

Uses
  SysUtils;

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
   (ARequest.RequestType = FRequestType) Then
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


End.
