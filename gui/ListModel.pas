Unit ListModel;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
{$IFDEF FPC}
  Windows,
{$ENDIF}
  Classes, Menus, ComCtrls, Generics.Collections;

Type
  TListModelColumn = Class
  Private
    FCaption : WideString;
    FWidth : Cardinal;
    FAUtoSize : Boolean;
    FTag : NativeUInt;
    FColumn : TListColumn;
    FVisible : Boolean;
    FMenuItem : TMenuItem;
  Protected
    Function GetWidth:Cardinal;
  Public
    Constructor Create(ACaption:WideString; ATag:NativeUInt; AAutoSize:Boolean = False; AWidth:Cardinal = 50); Reintroduce;

    Property Caption : WideString Read FCaption;
    Property Width : Cardinal Read GetWidth;
    Property AutoSize : Boolean Read FAutoSize;
    Property Tag : NativeUInt Read FTag;
    Property Visible : Boolean Read FVisible Write FVisible;
    Property MenuItem : TMenuItem Read FMenuItem Write FMenuItem;
    Property Column : TListColumn Read FColumn Write FColumn;
  end;

  TListModel<T> = Class
  Private
    FDisplayer : TListView;
    FColumns : TList<TListModelColumn>;
    Function CSVEscape(AElement:WideString):WideString;
  Protected
    Procedure _OnAdvancedCustomDrawItemCallback(Sender: TCustomListView; Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage; var DefaultDraw: Boolean);
    Procedure OnAdvancedCustomDrawItemCallback(Sender: TCustomListView; Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage; var DefaultDraw: Boolean); Virtual; Abstract;
    Procedure OnDataCallback(Sender:TObject; Item:TListItem);
    Function GetColumn(AItem:T; ATag:NativeUInt):WideString; Virtual; Abstract;
    Function GetImageIndex(AItem:T):Integer; Virtual;
    Procedure RefreshColumns;
    Procedure FreeItem(AItem:T); Virtual; Abstract;
    Function _Item(AIndex:Integer):T; Virtual; Abstract;
    Function _Column(AIndex:Integer):TListModelColumn;
    Procedure OnColumnNemuItemClick(Sender:TObject);
    Function GetSelected:T;
    Function GetSelectedIndex:Integer;
  Public
    Constructor Create(ADisplayer:TListView); Reintroduce;
    Destructor Destroy; Override;

    Function RowCount : Cardinal; Virtual; Abstract;
    Function ColumnCount : Cardinal;
    Function Item(ARow:Cardinal; AColumn:cardinal):WideString;
    Function Update:Cardinal; Virtual;
    Procedure SetDisplayer(AObject:TListView);

    Procedure ColumnUpdateBegin;
    Procedure ColumnClear;
    Function ColumnAdd(AColumn:TListModelColumn):TListModel<T>; Overload;
    Function ColumnAdd(ACaption:WideString; ATag:NativeUInt; AAutoSize:Boolean = False; AWidth:Cardinal = 50):TListModel<T>; Overload;
    Procedure ColumnSetVisible(AIndex:Integer; AVisible:Boolean);
    Procedure ColumnUpdateEnd;
    Procedure CreateColumnsMenu(AParent:TMenuItem);

    Procedure Clear; Virtual;
    Procedure ToCSV(AStrings:TStrings);

    Property Displayer : TListView Read FDisplayer;
    Property Items [Index:Integer] : T Read _Item;
    Property Columns [Index:Integer] : TListModelColumn Read _Column;
    Property Selected:T Read GetSelected;
    Property SelectedIndex:Integer Read GetSelectedIndex;
  end;

Implementation

Uses
  SysUtils;


Constructor TListModel<T>.Create(ADisplayer:TListView);
begin
Inherited Create;
FColumns := TList<TListModelColumn>.Create;
If Assigned(ADisplayer) Then
  SetDisplayer(ADisplayer)
Else Update;
end;

Destructor TListModel<T>.Destroy;
begin
If Assigned(FDisplayer) Then
  begin
  FDisplayer.OnData := Nil;
  FDisplayer := Nil;
  end;

FColumns.Free;
Inherited Destroy;
end;

Function TListModel<T>.CSVEscape(AElement:WideString):WideString;
Var
  ch : WideChar;
begin
Result := '';
For ch In AElement Do
  begin
  Case ch Of
    '\',
    '"' : Result := Result + '\';
    end;

  Result := Result + ch;
  end;
end;

Function TListModel<T>.GetImageIndex(AItem:T):Integer;
begin
Result := -1;
end;

Function TListModel<T>._Column(AIndex:Integer):TListModelColumn;
begin
Result := FColumns[AIndex];
end;

Procedure TListModel<T>.OnDataCallback(Sender:TObject; Item:TListItem);
Var
  first : Boolean;
  colIndex : Cardinal;
  c : TListModelColumn;
begin
first := True;
With Item Do
  begin
  ImageIndex := GetImageIndex(_Item(Index));
  colIndex := 0;
  For c In FColumns Do
    begin
    If c.Visible Then
      begin
      If first Then
        begin
        Caption := Self.Item(Index, colIndex);
        first := False;
        end
      Else SubItems.Add(Self.Item(Index, colIndex));
      end;

    Inc(colIndex);
    end;
  end;
end;

Procedure TListModel<T>.SetDisplayer(AObject:TListView);
begin
FDisplayer := AObject;
FDisplayer.ViewStyle := vsReport;
FDisplayer.OnData := OnDataCallback;
FDisplayer.OnAdvancedCustomDrawItem := _OnAdvancedCustomDrawItemCallback;
FDisplayer.OwnerData := True;
FDisplayer.Items.Count := 0;
RefreshColumns;
Update;
end;

Function TListModel<T>.ColumnCount:Cardinal;
begin
Result := FColumns.Count;
end;

Function TListModel<T>.Item(ARow:Cardinal; AColumn:Cardinal):WideString;
Var
  rd : T;
begin
Result := '';
rd := _Item(ARow);
Result := GetColumn(rd, FColumns[AColumn].Tag);
end;

Procedure TListModel<T>.ColumnUpdateBegin;
begin
If Assigned(FDisplayer) Then
  FDisplayer.Columns.BeginUpdate;
end;

Procedure TListModel<T>.ColumnClear;
Var
  c : TListModelColumn;
begin
For c In FColumns Do
  c.Free;

FColumns.Clear;
end;

Function TListModel<T>.ColumnAdd(AColumn:TListModelColumn):TListModel<T>;
begin
FColumns.Add(AColumn);
Result := Self;
end;

Function TListModel<T>.ColumnAdd(ACaption:WideString; ATag:NativeUInt; AAutoSize:Boolean = False; AWidth:Cardinal = 50):TListModel<T>;
begin
FColumns.Add(TListModelColumn.Create(ACaption, ATag, AAutoSize,AWidth));
Result := Self;
end;

Procedure TListModel<T>.ColumnUpdateEnd;
begin
If Assigned(FDisplayer) Then
  begin
  RefreshColumns;
  FDisplayer.Columns.EndUpdate;
  end;
end;

Procedure TListModel<T>.ColumnSetVisible(AIndex:Integer; AVisible:Boolean);
begin
FColumns.Items[AIndex].Visible := AVisible;
end;

Procedure TListModel<T>.RefreshColumns;
Var
  c : TListColumn;
  I : Integer;
  m : TListModelColumn;
begin
FDisplayer.Columns.BeginUpdate;
FDisplayer.Columns.Clear;
For I := 0 To FColumns.Count - 1 Do
  begin
  m := FColumns[I];
  If Assigned(m.MenuItem) Then
    m.MenuItem.Checked := m.Visible;

  If m.Visible Then
    begin
    c := FDisplayer.Columns.Add;
    m.Column := c;
    c.Caption := m.Caption;
    c.AutoSize := m.AutoSize;
    c.Tag := m.Tag;
    c.Width := m.Width;
    end;
  end;

FDisplayer.Columns.EndUpdate;
end;

Procedure TListModel<T>.OnColumnNemuItemClick(Sender:TObject);
Var
  M : TMenuItem;
  c : TListModelColumn;
begin
M := (Sender As TMenuItem);
M.Checked := Not M.Checked;
c := TListModelColumn(M.Tag);
ColumnUpdateBegin;
c.Visible := M.Checked;
ColumnUpdateEnd;
end;

Procedure TListModel<T>.CreateColumnsMenu(AParent:TMenuItem);
Var
  I : Integer;
  M : TMenuItem;
  c : TListModelColumn;
begin
For c In FColumns Do
  begin
  M := TMenuItem.Create(AParent);
  M.Caption := c.Caption;
  M.Checked := c.Visible;
  M.OnClick := OnColumnNemuItemClick;
  M.Tag := NativeInt(c);
  c.MenuItem := M;
  AParent.Add(M);
  end;
end;

Function TListModel<T>.GetSelected:T;
Var
  L : TListItem;
begin
Result := T(Nil);
If Assigned(Displayer) Then
  begin
  L := Displayer.Selected;
  If Assigned(L) Then
    Result := _Item(L.Index);
  end;
end;

Function TListModel<T>.GetSelectedIndex:Integer;
Var
  L : TListItem;
begin
Result := -1;
If Assigned(Displayer) Then
  begin
  L := Displayer.Selected;
  If Assigned(L) Then
    Result := L.Index;
  end;
end;

Procedure TListModel<T>.Clear;
begin
If Assigned(FDisplayer) Then
  begin
  FDisplayer.Items.Count := 0;
  FDisplayer.Invalidate;
  end;
end;

Function TListModel<T>.Update:Cardinal;
begin
Result := 0;
If Assigned(FDisplayer) Then
  begin
{$IFDEF FPC}
  LockWindowUpdate(FDisplayer.Handle);
{$ENDIF}
  FDisplayer.Items.Count := RowCount;
{$IFDEF FPC}
  LockWindowUpdate(0);
{$ELSE}
  FDisplayer.Invalidate;
{$ENDIF}
  end;
end;

Procedure TListModel<T>.ToCSV(AStrings:TStrings);
Var
  I : Integer;
  item : T;
  c : TListModelColumn;
  line : WideString;
  elem : WideString;
begin
line := '';
For c In FColumns Do
  begin
  If Not c.Visible Then
    Continue;

  elem := c.Caption;
  If line <> '' Then
    line := line + ',';

  line := line + Format('"%s"', [CSVEscape(elem)]);
  end;

AStrings.Add(line);
For I := 0 To RowCount - 1 Do
  begin
  item := _Item(I);
  line := '';
  For c In FColumns Do
    begin
    If Not c.Visible Then
      Continue;

    elem := GetColumn(item, c.Tag);
    If line <> '' Then
      line := line + ',';

    line := line + Format('"%s"', [CSVEscape(elem)]);
    end;

  AStrings.Add(line);
  end;
end;

Procedure TListModel<T>._OnAdvancedCustomDrawItemCallback(Sender: TCustomListView; Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage; var DefaultDraw: Boolean);
begin
OnAdvancedCustomDrawItemCallback(Sender, Item, State, Stage, DefaultDraw);
end;

(*** TListModelColumn ***)

Constructor TListModelColumn.Create(ACaption:WideString; ATag:NativeUInt; AAutoSize:Boolean = False; AWidth:Cardinal = 50);
begin
Inherited Create;
FCaption := ACaption;
FWidth := AWidth;
FTag := ATag;
FAutoSize := AAutoSize;
FColumn := Nil;
FMenuItem := Nil;
FVisible := True;
end;

Function TListModelColumn.GetWidth:Cardinal;
begin
Result := FWidth;
If FAutoSize Then
  Result := FColumn.Width;
end;


End.

