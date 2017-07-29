Unit FastIoRequest;

Interface

Uses
  Windows,
  RequestListModel, IRPMonDll;

Type
  TFastIoRequest = Class (TDriverRequest)
  Private
    FFileObject : Pointer;
    FPreviousMode : Byte;
    FIOSBStatus : Cardinal;
    FIOSBInformation : NativeUInt;
    FFastIoType : EFastIoOperationType;
  Public
    Constructor Create(Var ARequest:REQUEST_FASTIO); Reintroduce;

    Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;
    Class Function FastIoTypeToString(AFastIoType:EFastIoOperationType):WideString;

    Property PreviousMode : Byte Read FPreviousMode;
    Property IOSBStatus : Cardinal Read FIOSBStatus;
    Property IOSBInformation : NativeUInt Read FIOSBInformation;
    Property FastIoType : EFastIoOperationType Read FFastIoType;
  end;



Implementation

Uses
  SysUtils;


Constructor TFastIoRequest.Create(Var ARequest:REQUEST_FASTIO);
begin
Inherited Create(ARequest.Header);
FFastIoType := ARequest.FastIoType;
FPreviousMode := ARequest.PreviousMode;
FIOSBStatus := ARequest.IOSBStatus;
FIOSBInformation := ARequest.IOSBInformation;
FFileObject := ARequest.FileObject;
end;

Function TFastIoRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctSubType : AResult := FastIoTypeToString(FFastIoType);
  rlmctFileObject : AResult := Format('0x%p', [FFileObject]);
  rlmctPreviousMode : AResult := AccessModeToString(FPreviousMode);
  rlmctIOSBStatus : begin
    Result := False;
    Case FFastIoType Of
      FastIoCheckIfPossible,
      FastIoRead,
      FastIoWrite,
      FastIoQueryBasicInfo,
      FastIoQueryStandardInfo,
      FastIoLock,
      FastIoUnlockSingle,
      FastIoUnlockAll,
      FastIoUnlockAllByKey,
      FastIoDeviceControl,
      FastIoQueryNetworkOpenInfo,
      MdlRead,
      PrepareMdlWrite,
      FastIoReadCompressed,
      FastIoWriteCompressed : AResult := Format('0x%x (%s)', [FIOSBStatus, NTSTATUSToString(FIOSBStatus)]);
      end;
    end;
  rlmctIOSBInformation : begin
    Result := False;
    Case FFastIoType Of
      FastIoCheckIfPossible,
      FastIoRead,
      FastIoWrite,
      FastIoQueryBasicInfo,
      FastIoQueryStandardInfo,
      FastIoLock,
      FastIoUnlockSingle,
      FastIoUnlockAll,
      FastIoUnlockAllByKey,
      FastIoDeviceControl,
      FastIoQueryNetworkOpenInfo,
      MdlRead,
      PrepareMdlWrite,
      FastIoReadCompressed,
      FastIoWriteCompressed : AResult := Format('%u (0x%p)', [FIOSBInformation, Pointer(FIOSBInformation)]);
      end;
    end;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;

Class Function TFastIoRequest.FastIoTypeToString(AFastIoType:EFastIoOperationType):WideString;
begin
Case AFastIoType Of
  FastIoCheckIfPossible : Result := 'CheckIfPossible';
  FastIoRead : Result := 'Read';
  FastIoWrite : Result := 'Write';
  FastIoQueryBasicInfo : Result := 'QueryBasicInfo';
  FastIoQueryStandardInfo : Result := 'QueryStandardInfo';
  FastIoLock : Result := 'Lock';
  FastIoUnlockSingle : Result := 'UnlockSingle';
  FastIoUnlockAll : Result := 'UnlockAll';
  FastIoUnlockAllByKey : Result := 'UnlockAllByKey';
  FastIoDeviceControl : Result := 'DeviceControl';
  AcquireFileForNtCreateSection : Result := 'AcquireFileForNtCreateSection';
  ReleaseFileForNtCreateSection : Result := 'ReleaseFileForNtCreateSection';
  FastIoDetachDevice : Result := 'DetachDevice';
  FastIoQueryNetworkOpenInfo : Result := 'QueryNetworkOpenInfo';
  AcquireForModWrite : Result := 'AcquireForModWrite';
  MdlRead : Result := 'MdlRead';
  MdlReadComplete : Result := 'MdlReadComplete';
  PrepareMdlWrite : Result := 'PrepareMdlWrite';
  MdlWriteComplete : Result := 'MdlWriteComplete';
  FastIoReadCompressed : Result := 'FastIoReadCompressed';
  FastIoWriteCompressed : Result := 'FastIoWriteCompressed';
  MdlReadCompleteCompressed : Result := 'MdlReadCompleteCompressed';
  MdlWriteCompleteCompressed : Result := 'MdlWriteCompleteCompressed';
  FastIoQueryOpen : Result := 'FastIoQueryOpen';
  ReleaseForModWrite : Result := 'ReleaseForModWrite';
  AcquireForCcFlush : Result := 'AcquireForCcFlush';
  ReleaseForCcFlush : Result := 'ReleaseForCcFlush';
  Else Result := Format('%d', [Ord(AFastIoType)]);
  end;
end;


End.

