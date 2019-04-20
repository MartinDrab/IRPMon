Unit RequestFilter;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Generics.Collections,
  RequestListModel;

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
  Public
    Procedure GetPossibleValues(AValues:TDictionary<UInt64, WideString>); Virtual; Abstract;

    Property Name : WideString Read FName;
    Property Field : ERequestListModelColumnType Read FField;
    Property Op : ERequestFilterOperator Read FOp;
    Property StringValue : WideString Read FStringValue;
    Property IntValue : UInt64 Read FIntValue;
  end;



Implementation




End.
