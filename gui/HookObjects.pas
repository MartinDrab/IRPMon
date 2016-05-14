Unit HookObjects;

interface

Uses
  Generics.Collections,
  Windows, IRPMonDll;

Type
  THookedObject = Class
  Private
    FObjectId : Pointer;
  Protected
    FHandle : THandle;
    Function OpenHandle:Cardinal; Virtual; Abstract;
    Function CloseHandle:Cardinal; Virtual; Abstract;
  Public
    Constructor Create(AObjectId:Pointer); Reintroduce;
    Destructor Destroy; Override;
  end;

  THookedDevice = Class (THookedObject)
  Private
    FDeviceObject : Pointer;
    FDeviceName : WideString;
    FIRPSettings : TIRPSettings;
    FFastIoSettings : TFastIoSettings;
    FMonitoringEnabled : Boolean;
  Protected
    Function OpenHandle:Cardinal; Override;
    Function CloseHandle:Cardinal; Override;
  Public
    Constructor Create(Var ARecord:HOOKED_DEVICE_UMINFO); Reintroduce;
    Function Unhook:Cardinal;
    Function SetInfo(AMonitoringEnabled:Boolean; AIRPSettings:PIRPSettings = Nil; AFastIoSettings:PFastIoSettings = Nil):Cardinal;
    Function GetInfo:Cardinal;

    Class Function Hook(AName:WideString):Cardinal; Overload;
    Class Function Hook(AAddress:Pointer):Cardinal; Overload;

    Property DeviceObject : Pointer Read FDeviceObject;
    Property DeviceName : WideString Read FDeviceName;
    Property IRPSettings : TIRPSettings Read FIRPSettings;
    Property FastIoSettings : TFastIoSettings Read FFastIoSettings;
    Property MonitoringEnabled : Boolean Read FMonitoringEnabled;
  end;

  THookedDriver = Class (THookedObject)
    Private
      FDriverObject : Pointer;
      FDriverName : WideString;
      FMonitoringEnabled : Boolean;
      FMonitorSettings : DRIVER_MONITOR_SETTINGS;
      FHookedDevices : TList<THookedDevice>;
    Protected
      Function OpenHandle:Cardinal; Override;
      Function CloseHandle:Cardinal; Override;
    Public
      Constructor Create(Var ARecord:HOOKED_DRIVER_UMINFO); Reintroduce;
      Destructor Destroy; Override;

      Function Unhook:Cardinal;
      Function StartMonitoring:Cardinal;
      Function StopMonitoring:Cardinal;
      Function SetInfo(ASettings:DRIVER_MONITOR_SETTINGS):Cardinal;
      Function GetInfo:Cardinal;

      Class Function Enumerate(AList:TList<THookedDriver>):Cardinal;

      Property DriverObject : Pointer Read FDriverObject;
      Property DriverName : WideString Read FDriverName;
      Property MonitoringEnabled : Boolean Read FMonitoringEnabled;
      Property MonitorSettings : DRIVER_MONITOR_SETTINGS Read FMonitorSettings;
      Property HookedDevices : TList<THookedDevice> Read FHookedDevices;
    end;


implementation

Uses
  SysUtils;

(** THookedObject **)

Constructor THookedObject.Create(AObjectId:Pointer);
Var
  err : Cardinal;
begin
FObjectId := AObjectId;
FHandle := 0;
err := OpenHandle;
If err <> ERROR_SUCCESS Then
  Raise Exception.Create(Format('Failed to obtain handle: %s (%d)', [SysErrorMessage(err), err]));

Inherited Create;
end;

Destructor THookedObject.Destroy;
begin
If FHandle <> 0 Then
  CloseHandle;

Inherited Destroy;
end;

(** THookedDevice **)

Constructor THookedDevice.Create(Var ARecord:HOOKED_DEVICE_UMINFO);
begin
Inherited Create(ARecord.ObjectId);
FObjectId := ARecord.ObjectId;
FDeviceObject := ARecord.DeviceObject;
SetLength(FDeviceName, ARecord.DeviceNameLen Div SizeOf(WideChar));
CopyMemory(PWideChar(FDeviceName), ARecord.DeviceName, ARecord.DeviceNameLen);
FIRPSettings := ARecord.IRPSettings;
FFastIoSettings := ARecord.FastIoSettings;
FMonitoringEnabled := ARecord.MonitoringEnabled;
end;

Function THookedDevice.OpenHandle:Cardinal;
begin
Result := IRPMonDllOpenHookedDevice(FObjectId, FHandle);
end;

Function THookedDevice.CloseHandle:Cardinal;
begin
Result := IRPMonDllCloseHookedDeviceHandle(FHandle);
If Result = 0 Then
  FHandle := 0;
end;

Function THookedDevice.Unhook:Cardinal;
begin
Result := IRPMonDllUnhookDevice(FHandle);
end;

Function THookedDevice.SetInfo(AMonitoringEnabled:Boolean; AIRPSettings:PIRPSettings = Nil; AFastIoSettings:PFastIoSettings = Nil):Cardinal;
begin
Result := IRPMonDllHookedDeviceSetInfo(FHandle, @AIRPSettings, @AFastIoSettings, AMonitoringEnabled);
If Result = ERROR_SUCCESS Then
  Result := GetInfo;
end;

Function THookedDevice.GetInfo:Cardinal;
Var
  me : ByteBool;
  irs : Packed Array [0..$1B] Of Byte;
  fs : Packed Array [0..Ord(FastIoMax) - 1] Of Byte;
begin
Result := IRPMonDllHookedDeviceGetInfo(FHandle, @irs, @fs, me);
If Result = ERROR_SUCCESS Then
  begin
  CopyMemory(@FIRPSettings, @irs, SizeOf(irs));
  CopyMemory(@FFastIoSettings, @fs, SizeOf(fs));
  FMonitoringEnabled := me;
  end;
end;

Class Function THookedDevice.Hook(AName:WideString):Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllHookDeviceByName(PWideChar(AName), h);
If Result = ERROR_SUCCESS Then
  IRPMonDllCloseHookedDeviceHandle(h);
end;

Class Function THookedDevice.Hook(AAddress:Pointer):Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllHookDeviceByAddress(AAddress, h);
If Result = ERROR_SUCCESS Then
  IRPMonDllCloseHookedDeviceHandle(h);
end;


(** THookedDriver **)


Constructor THookedDriver.Create(Var ARecord:HOOKED_DRIVER_UMINFO);
Var
  I : Integer;
  hd : THookedDevice;
  phdr : PHOOKED_DEVICE_UMINFO;
begin
Inherited Create(ARecord.ObjectId);
FHookedDevices := TList<THookedDevice>.Create;
FObjectId := ARecord.ObjectId;
FDriverObject := ARecord.DriverObject;
SetLength(FDriverName, ARecord.DriverNameLen Div SizeOf(WideChar));
CopyMemory(PWideChar(FDriverName), ARecord.DriverName, ARecord.DriverNameLen);
FMonitoringEnabled := ARecord.MonitoringEnabled;
phdr := ARecord.HookedDevices;
For I := 0 To ARecord.NumberOfHookedDevices - 1 Do
  begin
  hd := THookedDevice.Create(phdr^);
  FHookedDevices.Add(hd);
  Inc(phdr);
  end;
end;

Destructor THookedDriver.Destroy;
Var
  hd : THookedDevice;
begin
For hd In FHookedDevices Do
  hd.Free;

FHookedDevices.Free;
Inherited Destroy;
end;

Function THookedDriver.OpenHandle:Cardinal;
begin
Result := IRPMonDllOpenHookedDriver(FObjectId, FHandle);
end;

Function THookedDriver.CloseHandle:Cardinal;
begin
Result := IRPMonDllCloseHookedDriverHandle(FHandle);
If Result = ERROR_SUCCESS Then
  FHandle := 0;
end;

Function THookedDriver.Unhook:Cardinal;
begin
Result := IRPMonDllUnhookDriver(FHandle);
end;

Function THookedDriver.StartMonitoring:Cardinal;
begin
Result := IRPMonDllDriverStartMonitoring(FHandle);
end;

Function THookedDriver.StopMonitoring:Cardinal;
begin
Result := IRPMonDllDriverStopMonitoring(FHandle);
end;

Function THookedDriver.SetInfo(ASettings:DRIVER_MONITOR_SETTINGS):Cardinal;
begin
Result := IRPMonDllDriverSetInfo(FHandle, ASettings);
If Result = ERROR_SUCCESS Then
  Result := GetInfo;
end;

Function THookedDriver.GetInfo:Cardinal;
Var
  ds : DRIVER_MONITOR_SETTINGS;
  me : ByteBool;
begin
Result := IRPMonDllHookedDriverGetInfo(FHandle, ds, me);
If Result = ERROR_SUCCESS Then
  begin
  FMonitorSettings := ds;
  FMonitoringEnabled := me;
  end;
end;


Class Function THookedDriver.Enumerate(AList:TList<THookedDriver>):Cardinal;
Var
  I : Integer;
  hd : THookedDriver;
  hdi : PHOOKED_DRIVER_UMINFO;
  tmp : PHOOKED_DRIVER_UMINFO;
  count : Cardinal;
begin
Result := IRPMonDllDriverHooksEnumerate(hdi, count);
If Result = ERROR_SUCCESS Then
  begin
  tmp := hdi;
  For I := 0 To count - 1 Do
    begin
    hd := THookedDriver.Create(tmp^);
    AList.Add(hd);
    Inc(tmp);
    end;

  IRPMonDllDriverHooksFree(hdi, count);
  end;
end;



end.

