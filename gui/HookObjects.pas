Unit HookObjects;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
{$IFDEF FPC}
  JwaWinSvc,
{$ELSE}
  WinSvc,
{$ENDIF}
  Windows, IRPMonDll, ComCtrls, Generics.Collections;

Type
  EHookObjectOperation = (
    hooNone,
    hooHook,
    hooUnhook,
    hooChange,
    hooStart,
    hooStop,
    hooOpenSCM,
    hooCreateDriver,
    hooDeleteDriver,
    hooWatchClass,
    hooUnwatchClass,
    hooWatchDriver,
    hooUnwatchDriver,
    hooMax
  );
  THookObjectOperations = Set Of EHookObjectOperation;

  TTaskObject = Class
  Private
    FName : WideString;
    FObjectType : WideString;
    FSupportedOperations : THookObjectOperations;
  Public
    Constructor Create(AType:WideString; AName:WideString; ASupportedOperations:THookObjectOperations = []); Reintroduce;
    Function Operation(AOperationType:EHookObjectOperation):Cardinal; Virtual; Abstract;
    Class Function OperationString(AOpType:EHookObjectOperation):WideString;
    Property SupportedOperations : THookObjectOperations Read FSupportedOperations;
    Property ObjectTpye : WideString Read FObjectType;
    Property Name : WideString Read FName;
  end;

  TDriverTaskObject = Class (TTaskObject)
  Private
    FServiceName : WideString;
    FServiceBinary : WideString;
    FServiceDescription : WideString;
    FServiceDisplayName : WideString;
    FhSCM : SC_HANDLE;
    Function Install:Cardinal;
    Function Uninstall:Cardinal;
    Function Load:Cardinal;
    Function Unload:Cardinal;
  Public
    Constructor Create(ASCHandle:SC_HANDLE; AServiceName:WideString; AServiceDisplayName:WideString = ''; AServiceDescription:WideString = ''; AFileName:WideString = ''); Reintroduce;
    Function Operation(AOperationType:EHookObjectOperation):Cardinal; Override;

    Property ServiceName : WideString Read FServiceName;
    Property ServiceBinary : WideString Read FServiceBinary;
    Property ServiceDisplayName : WideString Read FServiceDisplayName;
    Property ServiceDescription : WideString Read FServiceDescription;
  end;

  THookObject = Class (TTaskObject)
  Protected
    FAddress : Pointer;
    FObjectId : Pointer;
    FHooked : Boolean;
    FTreeNode : TTreeNode;
  Public
    Constructor Create(AType:WideString; AAddress:Pointer; AName:PWideChar; ASupportedOperations:THookObjectOperations = []); Reintroduce;

    Property Address : Pointer Read FAddress;
    Property ObjectId : Pointer Read FObjectId Write FObjectId;
    Property Hooked : Boolean Read FHooked Write FHooked;
    Property TreeNode : TTreeNode Read FTreenode Write FTreeNode;
  end;

  TDriverHookObject = Class (THookObject)
  Private
    Function Unhook:Cardinal;
    Function Hook:Cardinal;
    Function Change:Cardinal;
    Function Start:Cardinal;
    Function Stop:Cardinal;
  Public
    NumberOfHookedDevices : Cardinal;
    Settings : DRIVER_MONITOR_SETTINGS;
    Constructor Create(AAddress:Pointer; AName:PWideChar); Reintroduce;
    Function Operation(AOperationType:EHookObjectOperation):Cardinal; Override;
  end;

  TDeviceHookObject = Class (THookObject)
  Private
    FDriverObject : Pointer;
    FAttachedDevice : Pointer;
    Function Unhook:Cardinal;
    Function Hook:Cardinal;
    Function Change:Cardinal;
  Public
    IRPSettings : TIRPSettings;
    FastIOSettings : TFastIoSettings;
    Constructor Create(AAddress:Pointer; ADriverObject:Pointer; AAttachedDevice:Pointer; AName:PWideChar); Reintroduce;
    Function Operation(AOperationType:EHookObjectOperation):Cardinal; Override;

    Property Driverobject : Pointer Read FDriverObject;
    Property AttachedDevice : Pointer Read FAttachedDevice;
  end;

  TTaskOperationList = Class
    Private
      FCount : Cardinal;
      FObjectList : TList<TTaskObject>;
      FOpList : TList<EHookObjectOperation>;
    Protected
      Function GetItem(AIndex:Integer):TPair<EHookObjectOperation, TTaskObject>;
    Public
      Constructor Create;
      Destructor Destroy; Override;

      Function Add(AOp:EHookObjectOperation; AObject:TTaskObject):TTaskOperationList;

      Property Items [AIndex:Integer] : TPair<EHookObjectOperation, TTaskObject> Read GetItem;
      Property Count : Cardinal Read FCount;
    end;

Implementation

Uses
  SysUtils;

(** TTaskObject **)

Constructor TTaskObject.Create(AType:WideString; AName:WideString; ASupportedOperations:THookObjectOperations = []);
begin
Inherited Create;
FName := AName;
FSupportedOperations := ASupportedOperations;
FObjectType := AType;
end;

Class Function TTaskObject.OperationString(AOpType:EHookObjectOperation):WideString;
begin
Case AOpType Of
  hooNone: Result := 'None';
  hooHook: Result := 'Hook';
  hooUnhook: Result := 'Unhook';
  hooChange: Result := 'Change';
  hooStart: Result := 'Start';
  hooStop: Result := 'Stop';
  hooCreateDriver: Result := 'Install';
  hooDeleteDriver: Result := 'Delete';
  hooWatchClass: Result := 'WatchClass';
  hooUnwatchClass: Result := 'UnwatchClass';
  hooWatchDriver: Result := 'WatchDriver';
  hooUnwatchDriver: Result := 'UnwatchDriver';
  hooMax: Result := 'Max';
  Else Result := Format('<unknown (%u)>', [Ord(AOpType)]);
  end;
end;

(** THookObject **)

Constructor THookObject.Create(AType:WideString; AAddress:Pointer; AName:PWideChar; ASupportedOperations:THookObjectOperations = []);
Var
  len : Cardinal;
begin
Inherited Create(AType, AName, ASupportedOperations);
len := strlen(AName);
FAddress := AAddress;
FObjectId := Nil;
FHooked := False;
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
FillChar(Settings.IRPSettings, SizeOf(Settings.IRPSettings), Ord(True));
FillChar(Settings.FastIoSettings, SizeOf(Settings.FastIoSettings), Ord(True));
end;

Function TDriverHookObject.Operation(AOperationType:EHookObjectOperation):Cardinal;
begin
Case AOperationType Of
  hooHook: Result := Hook;
  hooUnhook: Result := Unhook;
  hooChange: Result := Change;
  hooStart: Result := Start;
  hooStop: Result := Stop;
  Else Result := ERROR_NOT_SUPPORTED;
  end;
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

Function TDeviceHookObject.Operation(AOperationType:EHookObjectOperation):Cardinal;
begin
Case AOperationType Of
  hooHook: Result := Hook;
  hooUnhook: Result := Unhook;
  hooChange: Result := Change;
  Else Result := ERROR_NOT_SUPPORTED;
  end;
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

(** TDriverTaskObject **)

Constructor TDriverTaskObject.Create(ASCHandle:SC_HANDLE; AServiceName:WideString; AServiceDisplayName:WideString = ''; AServiceDescription:WideString = ''; AFileName:WideString = '');
begin
Inherited Create('Service', AServiceName, [hooCreateDriver, hooDeleteDriver, hooStart, hooStop]);
FServiceName := AServiceName;
FServiceDisplayName := AServiceDisplayName;
FServiceDescription := AServiceDescription;
FServiceBinary := AFileName;
FhSCM := ASCHandle;
end;

Function TDriverTaskObject.Operation(AOperationType:EHookObjectOperation):Cardinal;
begin
Case AOperationType Of
  hooCreateDriver: Result := Install;
  hooStart : Result := Load;
  hooStop : Result := Unload;
  hooDeleteDriver : Result := Uninstall;
  Else Result := ERROR_NOT_SUPPORTED;
  end;
end;

Function TDriverTaskObject.Install:Cardinal;
Var
  hService : THandle;
  sd : SERVICE_DESCRIPTIONW;
begin
Result := ERROR_SUCCESS;
hService := CreateServiceW(FhSCM, PWideChar(FServiceName), PWideChar(FServiceDisplayName), SERVICE_CHANGE_CONFIG , SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, PWideChar(FServiceBinary), Nil, Nil, Nil, Nil, Nil);
If hService <> 0 Then
  begin
  sd.lpDescription := PWideChar(FServiceDescription);
  If Not ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, @sd) Then
    begin
    Result := GetLastError;
    DeleteService(hService);
    end;

  CloseServiceHandle(hService);
  end
Else Result := GetLastError;
end;

Function TDriverTaskObject.Uninstall:Cardinal;
Var
  hService : THandle;
begin
Result := ERROR_SUCCESS;
hService := OpenServiceW(FhSCM, PWideChar(FServiceName), SERVICE_STOP);
If hService <> 0 Then
  begin
  If Not DeleteService(hService) Then
    Result := GetLastError;

  CloseServiceHandle(hService);
  end
Else Result := GetLastError;
end;

Function TDriverTaskObject.Load:Cardinal;
Var
  hService : SC_HANDLE;
  dummy : PWideChar;
begin
Result := ERROR_SUCCESS;
hService := OpenServiceW(FhSCM, PWideChar(FServiceName), SERVICE_START);
If hService <> 0 Then
  begin
  dummy := Nil;
  If Not StartServiceW(hService, 0, dummy) Then
    Result := GetLastError;

  CloseServiceHandle(hService);
  end
Else Result := GetLastError;
end;

Function TDriverTaskObject.Unload:Cardinal;
Var
  hService : THandle;
  ss : SERVICE_STATUS;
begin
Result := ERROR_SUCCESS;
hService := OpenServiceW(FhSCM, PWideChar(FServiceName), SERVICE_STOP);
If hService <> 0 Then
  begin
  If Not ControlService(hService, SERVICE_CONTROL_STOP, ss) Then
    Result := GetLastError;

  CloseServiceHandle(hService);
  end
Else Result := GetLastError;
end;

(** TTaskOperationList **)

Constructor TTaskOperationList.Create;
begin
Inherited Create;
FOpList := TList<EHookObjectOperation>.Create;
FObjectList := TList<TTaskObject>.Create;
end;

Destructor TTaskOperationList.Destroy;
begin
FObjectList.Free;
FOpList.Free;
Inherited Destroy;
end;

Function TTaskOperationList.Add(AOp:EHookObjectOperation; AObject:TTaskObject):TTaskOperationList;
begin
If Not (AOp In AObject.SupportedOperations) Then
  Raise Exception.Create('Attempt to add an unsupported operation');

FOpList.Add(AOp);
FObjectList.Add(AObject);
Inc(FCount);
Result := Self;
end;


Function TTaskOperationList.GetItem(AIndex:Integer):TPair<EHookObjectOperation, TTaskObject>;
begin
Result.Key := FOpList[AIndex];
Result.Value := FObjectList[AIndex];
end;


End.

