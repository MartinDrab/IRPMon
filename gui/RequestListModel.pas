Unit RequestListModel;

{$IFDEF FPC}
{$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Classes, Generics.Collections, Generics.Defaults,
  IRPMonDll, ListModel, IRPMonRequest, DataParsers, ComCtrls,
  Graphics, RequestList, SymTables;

Type
  TRequestListModel = Class (TListModel<TDriverRequest>)
    Private
      FList : TRequestList;
      FFilterDisplayOnly : Boolean;
    Protected
      Procedure OnAdvancedCustomDrawItemCallback(Sender: TCustomListView; Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage; var DefaultDraw: Boolean); Override;
      Function GetColumn(AItem:TDriverRequest; ATag:NativeUInt):WideString; Override;
      Procedure FreeItem(AItem:TDriverRequest); Override;
      Function _Item(AIndex:Integer):TDriverRequest; Override;
      Procedure SetFilterDisplayOnly(AValue:Boolean);
      Function GetTotalCount:Integer;
      Procedure SetOnRequestProcessed(ARoutine:TRequestListOnRequestProcessed);
      Function GetOnRequestProcessed:TRequestListOnRequestProcessed;
      Procedure SetParsers(AValue:TObjectList<TDataParser>);
      Function GetParsers:TObjectList<TDataParser>;
      Procedure SetSymStore(AStore:TModuleSymbolStore);
      Function GetSymStore:TModuleSymbolStore;
    Public
      UpdateRequest : TList<PREQUEST_GENERAL>;
      Constructor Create; Reintroduce;
      Destructor Destroy; Override;
      Function RefreshMaps:Cardinal;

      Procedure Clear; Override;
      Function RowCount : Cardinal; Override;
      Function Update:Cardinal; Override;
      Procedure SaveToStream(AStream:TStream; AFormat:ERequestLogFormat);
      Procedure SaveToFile(AFileName:WideString; AFormat:ERequestLogFormat);
      Procedure LoadFromStream(AStream:TStream; ARequireHeader:Boolean = True);
      Procedure LoadFromFile(AFileName:WideString; ARequireHeader:Boolean = True);
      Procedure Reevaluate;

      Property FilterDisplayOnly : Boolean Read FFilterDisplayOnly Write SetFilterDisplayOnly;
      Property OnRequestProcessed : TRequestListOnRequestProcessed Read GetOnRequestProcessed Write SetOnRequestProcessed;
      Property TotalCount : Integer Read GetTotalCount;
      Property Parsers : TObjectList<TDataParser> Read GetParsers Write SetParsers;
      Property SymStore : TModuleSymbolStore Read GetSymStore Write SetSymStore;
      Property List : TRequestList Read FList;
    end;

Implementation

Uses
  SysUtils,
  NameTables,
  Utils;

(** TRequestListModel **)

Function TRequestListModel.GetColumn(AItem:TDriverRequest; ATag:NativeUInt):WideString;
begin
Result := '';
AItem.GetColumnValue(ERequestListModelColumnType(ATag), Result);
end;

Procedure TRequestListModel.FreeItem(AItem:TDriverRequest);
begin
AItem.Free;
end;

Function TRequestListModel._Item(AIndex:Integer):TDriverRequest;
begin
Result := FList[AIndex];
end;

Procedure TRequestListModel.SetFilterDisplayOnly(AValue:Boolean);
begin
FList.FilterDisplayOnly := AValue;
end;

Function TRequestListModel.RowCount : Cardinal;
begin
Result := FList.Count;
end;

Function TRequestListModel.Update:Cardinal;
Var
  requestBuffer : PREQUEST_GENERAL;
begin
Result := ERROR_SUCCESS;
If Assigned(UpdateRequest) Then
  begin
  For requestBuffer In UpdateRequest Do
      Result := FList.ProcessBuffer(requestBuffer);

  UpdateRequest := Nil;
  end;

Inherited Update;
end;

Function TRequestListModel.RefreshMaps:Cardinal;
begin
Result := FList.RefreshMaps;
end;

Procedure TRequestListModel.Clear;
begin
Inherited Clear;
FList.Clear;
end;

Constructor TRequestListModel.Create;
begin
Inherited Create(Nil);
UpdateRequest := Nil;
FList := TRequestList.Create;
RefreshMaps;
end;

Destructor TRequestListModel.Destroy;
begin
FList.Free;
Inherited Destroy;
end;

Procedure TRequestListModel.SaveToStream(AStream:TStream; AFormat:ERequestLogFormat);
begin
FList.SaveToStream(AStream, AFormat);
end;

Procedure TRequestListModel.SaveToFile(AFileName:WideString; AFormat:ERequestLogFormat);
begin
FList.SaveToFile(AFilename, AFormat);
end;

Procedure TRequestListModel.LoadFromStream(AStream:TStream; ARequireHeader:Boolean = True);
begin
FList.LoadFromStream(AStream, ARequireHeader);
Update;
end;

Procedure TRequestListModel.LoadFromFile(AFileName:WideString; ARequireHeader:Boolean = True);
begin
FList.LoadFromFile(AFilename, ARequireHeader);
Update;
end;

Procedure TRequestListModel.OnAdvancedCustomDrawItemCallback(Sender: TCustomListView; Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage; var DefaultDraw: Boolean);
Var
  dr : TDriverRequest;
begin
dr := FList[Item.Index];
With Sender.Canvas Do
  begin
  If Item.Selected Then
    begin
    Brush.Color := clHighLight;
    Font.Color := clHighLightText;
    Font.Style := [fsBold];
    end
  Else If dr.Highlight Then
    begin
    Brush.Color := dr.HighlightColor;
    If Utils.ColorLuminanceHeur(dr.HighlightColor) >= 1490 Then
       Font.Color := ClBlack
    Else Font.Color := ClWhite;
    end;
  end;

DefaultDraw := True;
end;

Procedure TRequestListModel.Reevaluate;
begin
FList.Reevaluate;
If Assigned(Displayer) Then
  begin
  Displayer.Items.Count := FList.Count;
  Displayer.Invalidate;
  end;
end;

Function TRequestListModel.GetTotalCount:Integer;
begin
Result := FList.GetTotalCount;
end;

Procedure TRequestListModel.SetOnRequestProcessed(ARoutine:TRequestListOnRequestProcessed);
begin
FList.OnRequestProcessed := ARoutine;
end;

Function TRequestListModel.GetOnRequestProcessed:TRequestListOnRequestProcessed;
begin
Result := FList.OnRequestProcessed;
end;

Procedure TRequestListModel.SetParsers(AValue:TObjectList<TDataParser>);
begin
FList.Parsers := AValue;
end;

Function TRequestListModel.GetParsers:TObjectList<TDataParser>;
begin
Result := FList.Parsers;
end;

Procedure TRequestListModel.SetSymStore(AStore:TModuleSymbolStore);
begin
FList.SymStore := AStore;
end;

Function TRequestListModel.GetSymStore:TModuleSymbolStore;
begin
Result := FList.SymStore;
end;



End.

