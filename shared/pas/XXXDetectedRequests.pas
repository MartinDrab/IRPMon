Unit XXXDetectedRequests;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows,
  IRPMonRequest, IRPMonDll;

Type
  TDriverDetectedRequest = Class (TDriverRequest)
    Private
      FDriverObject : Pointer;
      FDriverName : WideString;
    Public
      Constructor Create(Var ARequest:REQUEST_DRIVER_DETECTED); Overload;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;

      Property DriverObject : Pointer Read FDriverObject;
      Property DriverName : WideString Read FDriverName;
    end;

  TDeviceDetectedRequest = Class (TDriverRequest)
    Private
      FDriverObject : Pointer;
      FDeviceObject : Pointer;
      FDeviceName : WideString;
    Public
      Constructor Create(Var ARequest:REQUEST_DEVICE_DETECTED); Overload;
      Function GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean; Override;

      Property DriverObject : Pointer Read FDriverObject;
      Property DeviceObject : Pointer Read FDeviceObject;
      Property DeviceName : WideString Read FDeviceName;
    end;


Implementation

Uses
  SysUtils;

(** TDroverDetectedRequest **)

Constructor TDriverDetectedRequest.Create(Var ARequest:REQUEST_DRIVER_DETECTED);
Var
  dn : PWideChar;
  rawRequest : PREQUEST_DRIVER_DETECTED;
begin
Inherited Create(ARequest.Header);
rawRequest := PREQUEST_DRIVER_DETECTED(Raw);
FDriverObject := rawRequest.Header.Driver;
dn := PWideChar(PByte(rawRequest) + SizeOf(REQUEST_DRIVER_DETECTED));
SetLength(FDriverName, rawRequest.DriverNameLength Div SizeOf(WideChar));
CopyMemory(PWideChar(FDriverName), dn, rawRequest.DriverNameLength);
SetDriverName(FDriverName);
end;

Function TDriverDetectedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResultValue,
  rlmctIOSBStatusValue,
  rlmctIOSBStatusConstant,
  rlmctIOSBInformation,
  rlmctResultConstant : Result := False;
  rlmctDriverObject : AResult := Format('0x%p', [FDriverObject]);
  rlmctDriverName : AResult := FDriverName;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;


(** TDeviceDetectedRequest **)

Constructor TDeviceDetectedRequest.Create(Var ARequest:REQUEST_DEVICE_DETECTED);
Var
  dn : PWideChar;
  rawRequest : PREQUEST_DEVICE_DETECTED;
begin
Inherited Create(ARequest.Header);
rawRequest := PREQUEST_DEVICE_DETECTED(Raw);
FDriverObject := rawRequest.Header.Driver;
FDeviceObject := rawRequest.Header.Device;
dn := PWideChar(PByte(rawRequest) + SizeOf(REQUEST_DEVICE_DETECTED));
SetLength(FDeviceName, rawRequest.DeviceNameLength Div SizeOf(WideChar));
CopyMemory(PWideChar(FDeviceName), dn, rawRequest.DeviceNameLength);
SetDeviceName(FDeviceName);
end;

Function TDeviceDetectedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctResultValue,
  rlmctIOSBStatusValue,
  rlmctIOSBStatusConstant,
  rlmctIOSBInformation,
  rlmctResultConstant : Result := False;
  rlmctDriverObject : AResult := Format('0x%p', [FDriverObject]);
  rlmctDeviceObject : AResult := Format('0x%p', [FDeviceObject]);
  rlmctDeviceName : AResult := FDeviceName;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;



End.
