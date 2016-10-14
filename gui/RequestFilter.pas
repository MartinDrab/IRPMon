Unit RequestFilter;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Generics.Collections,
  RequestListModel;

Type
  ERequestFieldType = (
    rftTime,
    rftType,
    rftPID,
    rftTID,
    rftIRQL,
    rftResult,
    rftDriverObject,
    rftDriverName,
    rftDeviceObject,
    rftDeviceName,

    rftIRPAddress,
    rftIRPMajorFunction,
    rftIRPMinorFunction,
    rftIRPArg1,
    rftIRPArg2,
    rftIRPArg3,
    rftIRPArg4,
    rftIRPRequestorMode,
    rftIRPFlags,

    rftIOSBStatus,
    rftIOSBInformation,
    rftFileObject,
    rftPreviousMode,

    rftFastIOType
  );

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
    rfoDoesNotEnd
  );

  EFilterFormulaOperator = (
    ffoAND,
    ffoOR,
    ffoNOT
  );

  EFilterFormulaAction = (
    ffaUndefined,
    ffaShow,
    ffaNoShow,
    ffaHighlight
  );

  TRequestFilter = Class
  Private
    FName : WideString;
    FField : ERequestFieldType;
    FOp : ERequestFilterOperator;
    FValue : WideString;
  Public

    Property Name : WideString Read FName;
    Property Field : ERequestFieldType Read FField;
    Property Op : ERequestFilterOperator Read FOp;
    Property Value : WideString Read FValue;
  end;

  TFilterFormula = Class
  Private
    FAction : EFilterFormulaAction;
    FOp : EFilterFormulaOperator;
    FArgs : TList<TFilterFormula>;
  Protected
    Function GetArgCount:Cardinal;
    Function GetArg(AIndex:Integer):TFilterFormula;
  Public
    Property Action : EFilterFormulaAction Read FAction;
    Property Op : EFilterFormulaOperator Read FOp;
    Property Args [Index:Integer] : TFilterFormula Read GetArg;
    Property ArgCount : Cardinal Read GetArgCount;
  end;


Implementation


(** TFilterFormula **)

Function TFilterFormula.GetArgCount:Cardinal;
begin
Result := FArgs.Count;
end;

Function TFilterFormula.GetArg(AIndex:Integer):TFilterFormula;
begin
Result := FArgs[AIndex];
end;


End.
