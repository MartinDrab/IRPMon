[Code]
Const
  EnvironmentKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

Procedure EnvAddPath(Path:string);
Var
    Paths: string;
begin
If Not RegQueryStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths) Then
    Paths := '';

If Pos(';' + Uppercase(Path) + ';', ';' + Uppercase(Paths) + ';') > 0 Then
  Exit;

Paths := Paths + ';'+ Path +';'
If RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths) Then
  Log(Format('The [%s] added to PATH: [%s]', [Path, Paths]))
Else Log(Format('Error while adding the [%s] to PATH: [%s]', [Path, Paths]));
end;

Procedure EnvRemovePath(Path:string);
Var
  Paths: string;
  P: Integer;
begin
If Not RegQueryStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths) then
  Exit;

P := Pos(';' + Uppercase(Path) + ';', ';' + Uppercase(Paths) + ';');
If P = 0 Then
  Exit;

Delete(Paths, P - 1, Length(Path) + 1);
If RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', Paths) Then
  Log(Format('The [%s] removed from PATH: [%s]', [Path, Paths]))
Else Log(Format('Error while removing the [%s] from PATH: [%s]', [Path, Paths]));
end;
