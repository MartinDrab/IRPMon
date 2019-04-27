Unit XXXDetectedRequests;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows,
  RequestListModel, IRPMonDll;

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
begin
Inherited Create(ARequest.Header);
FDriverObject := ARequest.Header.Driver;
dn := PWideChar(PByte(@ARequest) + SizeOf(REQUEST_DRIVER_DETECTED));
SetLength(FDriverName, ARequest.DriverNameLength Div SizeOf(WideChar));
CopyMemory(PWideChar(FDriverName), dn, ARequest.DriverNameLength);
SetDriverName(FDriverName);
end;

Function TDriverDetectedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctDeviceObject,
  rlmctDeviceName,
  rlmctResultValue,
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
begin
Inherited Create(ARequest.Header);
FDriverObject := ARequest.Header.Driver;
FDeviceObject := ARequest.Header.Device;
dn := PWideChar(PByte(@ARequest) + SizeOf(REQUEST_DEVICE_DETECTED));
SetLength(FDeviceName, ARequest.DeviceNameLength Div SizeOf(WideChar));
CopyMemory(PWideChar(FDeviceName), dn, ARequest.DeviceNameLength);
SetDeviceName(FDeviceName);
end;

Function TDeviceDetectedRequest.GetColumnValue(AColumnType:ERequestListModelColumnType; Var AResult:WideString):Boolean;
begin
Result := True;
Case AColumnType Of
  rlmctResultValue,
  rlmctResultConstant : Result := False;
  rlmctDriverObject : AResult := Format('0x%p', [FDriverObject]);
  rlmctDeviceObject : AResult := Format('0x%p', [FDeviceObject]);
  rlmctDeviceName : AResult := FDeviceName;
  Else Result := Inherited GetColumnValue(AColumnType, AResult);
  end;
end;



End.
