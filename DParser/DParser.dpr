Library DParser;


{$R *.RES}

uses
  ShareMem,
  Windows,
  Generics.Collections,
  DataParsers in '..\shared\pas\DataParsers.pas',
  IRPMonDll in '..\shared\pas\IRPMonDll.pas',
  AbstractRequest in '..\shared\pas\AbstractRequest.pas',
  NameTables in '..\shared\pas\NameTables.pas',
  Utils in '..\shared\pas\Utils.pas',
  ProcessList in '..\shared\pas\ProcessList.pas',
  RefObject in '..\shared\pas\RefObject.pas';

Procedure DllMain(reason: Integer);
begin
Case Reason Of
  DLL_PROCESS_ATTACH : begin
    TablesInit('ntstatus.txt', 'ioctl.txt');
    end;
  DLL_PROCESS_DETACH : begin
    TablesFinit;
    end;
  end;
end;


Function DPListCreate(Var AList:Pointer):Cardinal; Cdecl;
Var
  l : TObjectList<TDataParser>;
begin
Try
  l := TObjectList<TDataParser>.Create;
  Result := 0;
Except
  Result := ERROR_NOT_ENOUGH_MEMORY;
  end;

If Result = 0 Then
    AList := l;
end;

Procedure DPListFree(AList:Pointer); Cdecl;
Var
  l : TObjectList<TDataParser>;
begin
l := AList;
l.Free;
end;

Function DPListGetCount(AList:Pointer):Cardinal;
Var
  l : TObjectList<TDataParser>;
begin
l := AList;
Result := l.Count;
end;

Function DPListAddFile(AList:Pointer; AFileName:PWideChar):Cardinal; Cdecl;
Var
  fn : WideString;
  l : TObjectList<TDataParser>;
begin
l := AList;
Try
  fn := WideCharToString(AFileName);
  Result := 0;
Except
  Result := ERROR_NOT_ENOUGH_MEMORY;
  end;

If Result = 0 Then
  Result := TDataParser.AddFromFile(fn, l);
end;

Function DPListAddDirectory(AList:Pointer; ADirectoryName:PWideChar):Cardinal; Cdecl;
Var
  dn : WideString;
  l : TObjectList<TDataParser>;
begin
l := AList;
Try
  dn := WideCharToString(ADirectoryName);
  Result := 0;
Except
  Result := ERROR_NOT_ENOUGH_MEMORY;
  end;

If Result = 0 Then
  TDataParser.AddFromDirectory(dn, l);
end;

Function DPListUnloadItem(AList:Pointer; AIndex:Cardinal):Cardinal;
Var
  l : TObjectList<TDataParser>;
begin
l := AList;
If AIndex < l.Count Then
  begin
  l.Delete(AIndex);
  Result := 0;
  end
Else Result := ERROR_INVALID_PARAMETER;
end;


Function DPListGetItemInfo(AList:Pointer; AIndex:Integer; Var AInfo:IRPMON_DATA_PARSER):Cardinal; Cdecl;
Var
  dp : TDataParser;
  l : TObjectList<TDataParser>;
begin
l := AList;
If AIndex < l.Count Then
  begin
  dp := l[AIndex];
  Result := dp.QueryInfo(AInfo);
  end
Else Result := ERROR_INVALID_PARAMETER;
end;

Procedure DPListItemInfoFree(Var AInfo:IRPMON_DATA_PARSER); Cdecl;
begin
TDataParser.FreeInfo(AInfo);
end;



Exports
  DPListCreate,
  DPListFree,
  DPListAddFile,
  DPListGetCount,
  DPListAddDirectory,
  DPListUnloadItem,
  DPListGetItemInfo,
  DPListItemInfoFree;


Begin
DLLProc := DllMain;
DllMain(DLL_PROCESS_ATTACH);
End.
