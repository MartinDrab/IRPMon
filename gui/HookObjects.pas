Unit HookObjects;

Interface

Uses
  Windows, IRPMonDll, Vcl.ComCtrls, Generics.Collections;

Type
  EHookObjectOperation = (hooHook, hooUnhook, hooChange, hooStart, hooStop);
  THookObjectOperations = Set Of EHookObjectOperation;

  THookObject = Class
  Private
    FObjectType : WIdeString;
    FSupportedOperations : THookObjectOperations;
  Protected
    FAddress : Pointer;
    FName : WideString;
    FObjectId : Pointer;
    FHooked : Boolean;
    FTreeNode : TTreeNode;
  Public
    Constructor Create(AType:WideString; AAddress:Pointer; AName:PWideChar; ASupportedOperations:THookObjectOperations = []); Reintroduce;
    Function Unhook:Cardinal; Virtual; Abstract;
    Function Hook:Cardinal; Virtual; Abstract;
    Function Change:Cardinal; Virtual; Abstract;
    Function Start:Cardinal; Virtual; Abstract;
    Function Stop:Cardinal; Virtual; Abstract;

    Property SupportedOperations : THookObjectOperations Read FSupportedOperations;
    Property Address : Pointer Read FAddress;
    Property Name : WideString Read FName;
    Property ObjectId : Pointer Read FObjectId Write FObjectId;
    Property Hooked : Boolean Read FHooked Write FHooked;
    Property TreeNode : TTreeNode Read FTreenode Write FTreeNode;
    Property ObjectTpye : WideString Read FObjectType;
  end;

  TDriverHookObject = Class (THookObject)
  Private
  Public
    NumberOfHookedDevices : Cardinal;
    Settings : DRIVER_MONITOR_SETTINGS;
    Constructor Create(AAddress:Pointer; AName:PWideChar); Reintroduce;
    Function Unhook:Cardinal; Override;
    Function Hook:Cardinal; Override;
    Function Change:Cardinal; Override;
    Function Start:Cardinal; Override;
    Function Stop:Cardinal; Override;
  end;

  TDeviceHookObject = Class (THookObject)
  Private
    FDriverObject : Pointer;
    FAttachedDevice : Pointer;
  Public
    IRPSettings : TIRPSettings;
    FastIOSettings : TFastIoSettings;
    Constructor Create(AAddress:Pointer; ADriverObject:Pointer; AAttachedDevice:Pointer; AName:PWideChar); Reintroduce;

    Function Unhook:Cardinal; Override;
    Function Hook:Cardinal; Override;
    Function Change:Cardinal; Override;

    Property Driverobject : Pointer Read FDriverObject;
    Property AttachedDevice : Pointer Read FAttachedDevice;
  end;


Implementation

Uses
  SysUtils;

(** THookObject **)

Constructor THookObject.Create(AType:WideString; AAddress:Pointer; AName:PWideChar; ASupportedOperations:THookObjectOperations = []);
Var
  len : Cardinal;
begin
Inherited Create;
FSupportedOperations := ASupportedOperations;
FObjectType := AType;
len := strlen(AName);
FAddress := AAddress;
FObjectId := Nil;
FHooked := False;
SetLength(FName, len);
CopyMemory(PWideChar(FName), AName, len*SizeOf(WideChar));
end;

(** TDriverHookObject **)

Constructor TDriverHookObject.Create(AAddress:Pointer; AName:PWideChar);
begin
Inherited Create('Driver', AAddress, AName, [hooHook, hooUnhook, hooChange, hooStart, hooStop]);
NumberOfHookedDevices := 0;
Settings.MonitorNewDevices := False;
Settings.MonitorAddDevice := False;
Settings.MonitorStartIo := False;
Settings.MonitorFastIo := True;
Settings.MonitorIRP := True;
Settings.MonitorIRPCompletion := True;
Settings.MonitorUnload := False;
end;

Function TDriverHookObject.Unhook:Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllOpenHookedDriver(FObjectId, h);
If Result = ERROR_SUCCESS Then
  begin
  Result := IRPMonDllUnhookDriver(h);
  IRPMonDllCloseHookedDriverHandle(h);
  end;
end;

Function TDriverHookObject.Hook:Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllHookDriver(PWideChar(FName), Settings, h, FObjectId);
If Result = ERROR_SUCCESS Then
  IRPMonDllCloseHookedDriverHandle(h);
end;

Function TDriverHookObject.Change:Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllOpenHookedDriver(FObjectId, h);
If Result = ERROR_SUCCESS Then
  begin
  Result := IRPMonDllDriverSetInfo(h, Settings);
  IRPMonDllCloseHookedDriverHandle(h);
  end;
end;

Function TDriverHookObject.Start:Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllOpenHookedDriver(FObjectId, h);
If Result = ERROR_SUCCESS Then
  begin
  Result := IRPMonDllDriverStartMonitoring(h);
  IRPMonDllCloseHookedDriverHandle(h);
  end;
end;

Function TDriverHookObject.Stop:Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllOpenHookedDriver(FObjectId, h);
If Result = ERROR_SUCCESS Then
  begin
  Result := IRPMonDllDriverStopMonitoring(h);
  IRPMonDllCloseHookedDriverHandle(h);
  end;
end;


(** TDeviceHookObject **)

Constructor TDeviceHookObject.Create(AAddress:Pointer; ADriverObject:Pointer; AAttachedDevice:Pointer; AName:PWideChar);
begin
Inherited Create('Device', AAddress, AName, [hooHook, hooUnhook, hooChange]);
FDriverObject := ADriverObject;
FAttachedDevice := AAttachedDevice;
FillChar(IRPSettings, SizeOf(IRPSettings), Ord(True));
FillChar(FastIoSettings, SizeOf(FastIoSettings), Ord(True));
end;

Function TDeviceHookObject.Unhook:Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllOpenHookedDevice(FObjectId, h);
If Result = ERROR_SUCCESS Then
  begin
  Result := IRPMonDllUnhookDevice(h);
  IRPMonDllCloseHookedDeviceHandle(h);
  end;
end;

Function TDeviceHookObject.Hook:Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllHookDeviceByAddress(FAddress, h, FObjectId);
If Result = ERROR_SUCCESS Then
  begin
  Result := IRPMonDllHookedDeviceSetInfo(h, @IRPSettings, @FastIoSettings, True);
  If Result <> ERROR_SUCCESS Then
    IRPMonDllUnhookDevice(h);

  IRPMonDllCloseHookedDeviceHandle(h);
  end;
end;

Function TDeviceHookObject.Change:Cardinal;
Var
  h : THandle;
begin
Result := IRPMonDllOpenHookedDevice(FObjectId, h);
If Result = ERROR_SUCCESS Then
  begin
  Result := IRPMonDllHookedDeviceSetInfo(h, @IRPSettings, @FastIoSettings, True);
  IRPMonDllCloseHookedDeviceHandle(h);
  end;
end;



End.
