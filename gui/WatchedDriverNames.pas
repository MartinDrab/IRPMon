Unit WatchedDriverNames;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, IRPMonDll, Generics.Collections;

Type
  TWatchableDriverName = Class
    Private
      FName : WideString;
      FSettings : DRIVER_MONITOR_SETTINGS;
    Public
      Constructor Create(AName:WideString; ASettings:DRIVER_MONITOR_SETTINGS); Reintroduce; Overload;
      Constructor Create(Var ARecord:DRIVER_NAME_WATCH_RECORD); Reintroduce; Overload;

      Function Register:Cardinal;
      Function Unregister:Cardinal;
      Class Function Enumerate(AList:TList<TWatchableDriverName>):Cardinal;

      Property Name : WideString Read FName;
      Property Settings : DRIVER_MONITOR_SETTINGS Read FSettings;
    end;


Implementation

Uses
  SysUtils;


Constructor TWatchableDriverName.Create(AName:WideString; ASettings:DRIVER_MONITOR_SETTINGS);
begin
Inherited Create;
FName := AName;
FSettings := ASettings;
end;

Constructor TWatchableDriverName.Create(Var ARecord:DRIVER_NAME_WATCH_RECORD);
begin
Inherited Create;
FName := Copy(WideString(ARecord.DriverName), 1, StrLen(ARecord.DriverName));
FSettings := ARecord.MonitorSettings;
end;

Function TWatchableDriverName.Register:Cardinal;
begin
Result := IRPMonDllDriverNameWatchRegister(PWideChar(FName), FSettings);
end;

Function TWatchableDriverName.Unregister:Cardinal;
begin
Result := IRPMonDllDriverNameWatchUnregister(PWideChar(FName));
end;

Class Function TWatchableDriverName.Enumerate(AList:TList<TWatchableDriverName>):Cardinal;
Var
  I : Integer;
  dn : TWatchableDriverName;
  tmp : PDRIVER_NAME_WATCH_RECORD;
  dnCount : Cardinal;
  dnArray : PDRIVER_NAME_WATCH_RECORD;
begin
Result := IRPMonDllDriverNameWatchEnum(dnArray, dnCount);
If Result = ERROR_SUCCESS Then
  begin
  tmp := dnArray;
  For I := 0 To dnCount - 1 Do
    begin
    dn := TWatchableDriverName.Create(tmp^);
    AList.Add(dn);
    Inc(tmp);
    end;

  IRPMonDllDriverNameWatchEnumFree(dnArray, dnCount);
  end;
end;



End.
