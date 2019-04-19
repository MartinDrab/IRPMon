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
    rftResultValue,
    rftResultConstant,
    rftDriverObject,
    rftDriverName,
    rftDeviceObject,
    rftDeviceName,

    rftFileObject,
    rftFileName,
    rftIRPAddress,
    rftIRPMajorFunction,
    rftIRPMinorFunction,
    rftIRPArg1,
    rftIRPArg2,
    rftIRPArg3,
    rftIRPArg4,
    rftIRPRequestorMode,
    rftIRPFlags,

    rftIOSBStatusValue,
    rftIOSBStatusConstant,
    rftIOSBInformation,
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

  EFilterAction = (
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



Implementation




End.
