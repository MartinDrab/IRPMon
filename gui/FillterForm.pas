Unit FillterForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls,
  Generics.Collections, RequestFilter, RequestListModel, Vcl.ExtCtrls,
  Vcl.ComCtrls;

Type
  EFilterComboboxType = (
    fctType,
    fctColumn,
    fctOperator,
    fctValue,
    fctAction,
    fctMax
  );

  TFilterFrm = Class (TForm)
    UpperPanel: TPanel;
    FilterTypeComboBox: TComboBox;
    FilterColumnComboBox: TComboBox;
    FilterOperatorComboBox: TComboBox;
    NegateCheckBox: TCheckBox;
    FilterValueComboBox: TComboBox;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    HighlightColorColorBox: TColorBox;
    FilterActionComboBox: TComboBox;
    Label5: TLabel;
    Label4: TLabel;
    LowerPanel: TPanel;
    CloseButton: TButton;
    OkButton: TButton;
    FilterListView: TListView;
    AddButton: TButton;
    DeleteButton: TButton;
    EnabledCheckBox: TCheckBox;
    NextFilterComboBox: TComboBox;
    UpButton: TButton;
    DownButton: TButton;
    Procedure FormCreate(Sender: TObject);
    procedure FilterTypeComboBoxChange(Sender: TObject);
    procedure FilterColumnComboBoxChange(Sender: TObject);
    procedure FilterActionComboBoxChange(Sender: TObject);
    procedure CloseButtonClick(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
    procedure FilterListViewData(Sender: TObject; Item: TListItem);
    procedure DeleteButtonClick(Sender: TObject);
    procedure AddButtonClick(Sender: TObject);
    procedure FilterListViewAdvancedCustomDrawItem(Sender: TCustomListView;
      Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage;
      var DefaultDraw: Boolean);
    procedure FilterListViewDblClick(Sender: TObject);
    procedure FilterListViewDeletion(Sender: TObject; Item: TListItem);
    procedure FilterListViewItemChecked(Sender: TObject; Item: TListItem);
    procedure FormDestroy(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure UpDownButtonClick(Sender: TObject);
    procedure FilterListViewSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
  Private
    FFilterList : TObjectList<TRequestFilter>;
    FCBs : Array [0..Ord(fctMax)-1] of TComboBox;
    FCancelled : Boolean;
    Procedure EnableCombobox(AType:EFilterComboboxType; AEnable:Boolean);
    Procedure ShowNextFilters;
  Public
    Constructor Create(AOwner:TApplication; AFilterList:TObjectList<TRequestFilter>); Reintroduce;

    Property FilterList : TObjectList<TRequestFilter> Read FFilterList;
    Property Cancelled : Boolean Read FCancelled;
  end;


Implementation

{$R *.DFM}

Uses
  IniFiles,
  IRPMonDll, IRPRequest, FastIoRequest, Utils,
  FileObjectNameXxxRequest, XXXDetectedRequests;

Constructor TFilterFrm.Create(AOwner:TApplication; AFilterList:TObjectList<TRequestFilter>);
Var
  I : Integer;
  n : WideString;
  names : TStringList;
  rf : TRequestFilter;
  tmp : TRequestFilter;
begin
names := TStringList.Create;
FFilterList := TObjectList<TRequestFilter>.Create;
For rf In AFilterList Do
  begin
  n := '';
  FFilterList.Add(rf.Copy);
  If Assigned(rf.NextFilter) Then
    n := rf.NextFilter.Name;

  names.Add(n);
  end;

For I := 0 To names.Count - 1 Do
  begin
  n := names[I];
  rf := FFilterList[I];
  tmp := TRequestFilter.GetByName(n, FFilterList);
  If Assigned(tmp) Then
    rf.AddNext(tmp);
  end;

names.Free;
Inherited Create(Application);
end;

Procedure TFilterFrm.ShowNextFilters;
Var
  L : TListItem;
  rf : TRequestFilter;
  selectedIndex : Integer;
  currentRF : TRequestFilter;
  selectedRF : TRequestFilter;
begin
selectedIndex := -1;
selectedRF := Nil;
If NextFilterComboBox.ItemIndex <> -1 Then
  selectedRF := NextFilterComboBox.Items.Objects[NextFilterComboBox.ItemIndex] As TRequestFilter;

NextFilterComboBox.Clear;
If NextFilterComboBox.Visible Then
  begin
  currentRF := Nil;
  L := FilterListView.Selected;
  If Assigned(L) Then
    currentRF := FFilterList[L.Index];

  For rf In FFilterList Do
    begin
    If (Assigned(currentRF)) And (rf.Name = currentRF.Name) Then
      Continue;

    If rf.HasPredecessor Then
      Continue;

    If (Assigned(selectedRF)) And (selectedRF.Name = rf.Name) Then
      selectedIndex := NextFilterComboBox.Items.Count;

    NextFilterComboBox.Items.AddObject(rf.Name, rf);
    end;

  NextFilterComboBox.ItemIndex := selectedIndex;
  end;
end;

Procedure TFilterFrm.UpDownButtonClick(Sender: TObject);
Var
  L : TListItem;
  index2 : Integer;
begin
L := FilterListView.Selected;
If Assigned(L) Then
  begin
  If Sender = UpButton Then
    index2 := L.Index - 1
  Else If Sender = DownButton Then
    index2 := L.Index + 1;

  FFilterList.Exchange(L.Index, index2);
  FilterListViewData(FilterListView, L);
  FilterListViewData(FilterListView, FilterListView.Items[index2]);
  FilterListView.Selected := FilterListView.Items[index2];
  end;
end;

Procedure TFilterFrm.FilterActionComboBoxChange(Sender: TObject);
Var
  fa : EFilterAction;
  c : TComboBox;
begin
c := (Sender As TComboBox);
fa := EFilterAction(c.ItemIndex);
NextFilterComboBox.Visible := (fa = ffaPassToFilter);
NextFilterComboBox.Enabled := (fa = ffaPassToFilter);
ShowNextFilters;
end;

Procedure TFilterFrm.FilterColumnComboBoxChange(Sender: TObject);
Var
  I : Integer;
  rt : ERequestType;
  ct : ERequestListModelColumnType;
  c : TComboBox;
  tmp : TRequestFilter;
  ss : TList<UInt64>;
  st : TList<WideString>;
  bm : Boolean;
  ops : RequestFilterOperatorSet;
  op : ERequestFilterOperator;
begin
c := (Sender As TComboBox);
EnableComboBox(fctOperator, c.ItemIndex <> -1);
EnableComboBox(fctValue, c.ItemIndex <> -1);
If FilterOperatorComboBox.Enabled Then
  begin
  rt := ERequestType(FilterTypeComboBox.Items.Objects[FilterTypeComboBox.ItemIndex]);
  ct := ERequestListModelColumnType(FilterColumnComboBox.Items.Objects[FilterColumnComboBox.ItemIndex]);
  FilterValueComboBox.Clear;
  tmp := TRequestFilter.NewInstance(rt);
  If Not tmp.SetCondition(ct, rfoAlwaysTrue, 0) Then
    tmp.SetCondition(ct, rfoAlwaysTrue, '');

  ss := TList<UInt64>.Create;
  st := TList<WideString>.Create;
  If tmp.GetPossibleValues(ss, st, bm) Then
    begin
    FilterValueComboBox.Style := csDropDown;
    For I := 0 To ss.Count - 1 Do
      FilterValueComboBox.AddItem(st[I], Pointer(ss[I]));
    end
  Else FilterValueComboBox.Style := csSimple;

  FilterOperatorComboBox.Clear;
  ops := tmp.SupportedOperators;
  For op In ops Do
    FilterOperatorComboBox.AddItem(RequestFilterOperatorNames[Ord(op)], Pointer(Ord(op)));

  st.Free;
  ss.Free;
  tmp.Free;
  end;
end;

Procedure TFilterFrm.FilterListViewAdvancedCustomDrawItem(
  Sender: TCustomListView; Item: TListItem; State: TCustomDrawState;
  Stage: TCustomDrawStage; var DefaultDraw: Boolean);
Var
  f : TRequestFilter;
begin
With FilterListView.Canvas Do
  begin
  f := FFilterList[Item.Index];
  If Item.Selected Then
    begin
    Brush.Color := clHighLight;
    Font.Color := clHighLightText;
    Font.Style := [fsBold];
    end
  Else begin
    Font.Style := [];
    If Not f.HasPredecessor Then
      Font.Style := [fsBold];

    Brush.Color := f.HighlightColor;
    If ColorLuminanceHeur(f.HighlightColor) >= 1490 Then
       Font.Color := ClBlack
    Else Font.Color := ClWhite;
    end;
  end;

DefaultDraw := True;
end;

Procedure TFilterFrm.FilterListViewData(Sender: TObject; Item: TListItem);
Var
  f : TRequestFilter;
  nextFilterName : WideString;
begin
With Item  Do
  begin
  f := FFilterList[Index];
  Caption := f.Name;
  SubItems.Clear;
  SubItems.Add(FilterTypeComboBox.Items[Ord(f.RequestType)]);
  SubItems.Add(f.ColumnName);
  SubItems.Add(RequestFilterOperatorNames[Ord(f.Op)]);
  If f.Negate Then
    SubItems.Add('Yes')
  Else SubItems.Add('No');

  SubItems.Add(f.StringValue);
  SubItems.Add(FilterActionComboBox.Items[Ord(f.Action)]);
  nextFilterName := '<not applicable>';
  If Assigned(f.NextFilter) Then
    nextFilterName := f.NextFilter.Name;

  SubItems.Add(nextFilterName);
  Checked := f.Enabled;
  end;
end;

Procedure TFilterFrm.FilterListViewDblClick(Sender: TObject);
Var
  I : Integer;
  f : TRequestFilter;
  L : TListItem;
begin
L := FilterListView.Selected;
If Assigned(L) Then
  begin
  f := FFilterList[L.Index];
  FilterTypeComboBox.ItemIndex := Ord(f.RequestType);
  FilterTypeComboBoxChange(FilterTypeComboBox);
  FilterColumnComboBox.ItemIndex := FilterColumnComboBox.Items.IndexOf(f.ColumnName);
  FilterColumnComboBoxChange(FilterColumnComboBox);
  For I := 0 To FilterOperatorComboBox.Items.Count - 1 Do
    begin
    If ERequestFilterOperator(NativeUInt(FilterOperatorComboBox.Items.Objects[i])) = f.Op Then
      begin
      FilterOperatorComboBox.ItemIndex := I;
      Break;
      end;
    end;

  If FilterValueComboBox.Style <> csSimple Then
    begin
    For I := 0 To FilterValueComboBox.Items.Count - 1 Do
      begin
      If UInt64(FilterValueComboBox.Items.Objects[I]) = f.IntValue Then
        begin
        FilterValueComboBox.ItemIndex := I;
        Break;
        end;
      end;
    end
  Else FilterValueComboBox.Text := f.StringValue;

  FilterActionComboBox.ItemIndex := Ord(f.Action);
  FilterActionComboBoxChange(FilterActionComboBox);
  HighlightColorColorBox.Selected := f.HighlightColor;
  NegateCheckBox.Checked := f.Negate;
  EnabledCheckBox.Checked := f.Enabled;
  end;
end;

Procedure TFilterFrm.FilterListViewDeletion(Sender: TObject; Item: TListItem);
begin
FFilterList.Delete(Item.Index);
end;

Procedure TFilterFrm.FilterListViewItemChecked(Sender: TObject;
  Item: TListItem);
Var
  I : Integer;
  f : TRequestFilter;
begin
f := FFilterList[Item.Index];
f.Enabled := Item.Checked;
If f.Action = ffaPassToFilter Then
  begin
  For I := 0 To FilterListVIew.Items.Count - 1 Do
    begin
    If I = Item.Index Then
      Continue;

    FilterListView.Items[I].Checked := FFilterList[I].Enabled;
    end;
  end;
end;

Procedure TFilterFrm.FilterListViewSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
begin
DeleteButton.Enabled := (Assigned(Item)) And (Selected);
UpButton.Enabled := ((Assigned(Item)) And (Selected) And (Item.Index > 0));
DownButton.Enabled := ((Assigned(Item)) And (Selected) And (Item.Index < FilterListView.Items.Count - 1));
end;

Procedure TFilterFrm.FilterTypeComboBoxChange(Sender: TObject);
Var
  c : TComboBox;
  dr : TDriverRequest;
  rt : ERequestType;
  ct : ERequestListModelColumnType;
  v : WideString;
begin
c := (Sender As TComboBox);
EnableComboBox(fctColumn, c.ItemIndex <> -1);
If FilterColumnComboBox.Enabled Then
  begin
  dr := Nil;
  rt := ERequestType(c.Items.Objects[c.ItemIndex]);
  Case rt Of
    ertUndefined: dr := TDriverRequest.Create;
    ertIRP: dr := TIRPRequest.Create;
    ertIRPCompletion: dr := TIRPCompleteRequest.Create;
    ertAddDevice: dr := TAddDeviceRequest.Create;
    ertDriverUnload: dr := TDriverUnloadRequest.Create;
    ertFastIo: dr := TFastIoRequest.Create;
    ertStartIo: dr := TStartIoRequest.Create;
    ertDriverDetected: dr := TDriverDetectedRequest.Create;
    ertDeviceDetected: dr := TDeviceDetectedRequest.Create;
    ertFileObjectNameAssigned: dr := TFileObjectNameAssignedRequest.Create;
    ertFileObjectNameDeleted: dr := TFileObjectNameDeletedRequest.Create;
    end;

  FilterColumnComboBox.Clear;
  If Assigned(dr) Then
    begin
    For ct := Low(ERequestListModelColumnType) To High(ERequestListModelColumnType) Do
      begin
      If dr.GetColumnValue(ct, v) Then
        FilterColumnComboBox.AddItem(dr.GetColumnName(ct), Pointer(Ord(ct)));
      end;

    dr.Free;
    end;
  end;
end;

Procedure TFilterFrm.FormClose(Sender: TObject; var Action: TCloseAction);
begin
FilterListView.OnDeletion := Nil;
end;

Procedure TFilterFrm.FormCreate(Sender: TObject);
Var
  L : TListItem;
  bm : Boolean;
  index : Integer;
  ss : TList<UInt64>;
  rf : TRequestFilter;
  ts : TList<WideString>;
  tmp : TRequestFilter;
  tmpEnabled : Boolean;
  I : ERequestType;
begin
FCBs[Ord(fctType)] := FilterTypeComboBox;
FCBs[Ord(fctColumn)] := FilterColumnComboBox;
FCBs[Ord(fctOperator)] := FilterOperatorComboBox;
FCBs[Ord(fctValue)] := FilterValueComboBox;
FCBs[Ord(fctAction)] := FilterActionComboBox;
EnableComboBox(fctColumn, False);
EnableComboBox(fctOperator, False);
EnableComboBox(fctValue, False);

ss := TList<UInt64>.Create;
ts := TList<WideString>.Create;
tmp := TRequestFilter.NewInstance(ertUndefined);
tmp.SetCondition(rlmctRequestType, rfoAlwaysTrue, 0);
tmp.GetPossibleValues(ss, ts, bm);
tmp.Free;
For I := Low(ERequestType) To High(ERequestType) Do
  begin
  tmp := TRequestFilter.NewInstance(I);
  If Assigned(tmp) Then
    begin
    index := ss.IndexOf(Ord(I));
    FilterTypeComboBox.AddItem(ts[Index], Pointer(ss[Index]));
    tmp.Free;
    end;
  end;

ts.Free;
ss.Free;
FCancelled := True;
For rf In FFilterList Do
  begin
  tmpEnabled := rf.Enabled;
  L := FilterListView.Items.Add;
  FilterListViewData(FilterListView, L);
  rf.Enabled := tmpEnabled;
  L.Checked := rf.Enabled;
  end;

FilterListViewSelectItem(FilterListView, Nil, False);
end;

Procedure TFilterFrm.FormDestroy(Sender: TObject);
begin
FFilterList.Free;
end;

Procedure TFilterFrm.OkButtonClick(Sender: TObject);
Var
  iniFile : TIniFile;
  fileName : WideString;
begin
fileName := ExtractFilePath(Application.ExeName) + 'filters.ini';
iniFile := TIniFIle.Create(fileName);
FCancelled := Not TRequestFilter.SaveList(iniFile, FFilterList);
iniFile.Free;
If Not FCancelled Then
  begin
  FFilterList.OwnsObjects := False;
  Close;
  end;
end;

Procedure TFilterFrm.AddButtonClick(Sender: TObject);
Var
  I : Integer;
  L : TListItem;
  rt : ERequestType;
  ct : ERequestListModelColumnType;
  op : ERequestFilterOperator;
  f : TRequestFilter;
  v : WideString;
  fa : EFilterAction;
  hc : TColor;
  err : Cardinal;
  passTarget : TRequestFilter;
  newName : WideString;
begin
f := Nil;
passTarget := Nil;
Try
  If FilterTypeComboBox.ItemIndex = -1 Then
    begin
    ErrorMessage('Filter type is not selected');
    Exit;
    end;

  If FilterColumnComboBox.ItemIndex = -1 Then
    begin
    ErrorMessage('Filtered column not selected');
    Exit;
    end;

  If FilterOperatorComboBox.ItemIndex = -1 Then
    begin
    ErrorMessage('Filter condition operator not selected');
    Exit;
    end;

  rt := ERequestType(FilterTypeComboBox.Items.Objects[FilterTypeComboBox.ItemIndex]);
  ct := ERequestListModelColumnType(FilterColumnComboBox.Items.Objects[FilterColumnComboBox.ItemIndex]);
  op := ERequestFilterOperator(FilterOperatorComboBox.Items.Objects[FilterOperatorComboBox.ItemIndex]);
  fa := EFilterAction(FilterActionComboBox.ItemIndex);
  If (fa = ffaPassToFilter) Then
    begin
    If (NextFilterComboBox.ItemIndex = -1) Then
      begin
      ErrorMessage('Target of the pass-to-filter action not selected');
      Exit;
      end;

    passTarget := NextFilterComboBox.Items.Objects[NextFilterComboBox.ItemIndex] As TRequestFilter;
    end;

  hc := HighlightColorColorBox.Selected;
  v := FilterValueComboBox.Text;
  For I := 0 To FilterValueComboBox.Items.Count - 1 Do
    begin
    If (v = FilterValueComboBox.Items[I]) Then
      begin
      v := UIntToStr(UInt64(FilterValueComboBox.Items.Objects[I]));
      Break;
      end;
    end;

  newName :=
    FilterTypeComboBox.Text + '-' +
    FilterColumnComboBox.Text + '-' +
    FilterOperatorComboBox.Text + '-' +
    FilterValueComboBox.Text + '-' +
    FilterActionComboBox.Text;

  f := TRequestFilter.GetByName(newName, FFilterList);
  If Assigned(f) Then
    begin
    ErrorMessage('The filter is already present in the list');
    f := Nil;
    Exit;
    end;

  f := TRequestFilter.NewInstance(rt);
  If Not Assigned(f) Then
    Exit;

  f.Name := newName;
  If Not f.SetCondition(ct, op, v) Then
    begin
    ErrorMessage('Unable to set filter condition, bad value of the constant');
    Exit;
    end;

  f.Negate := NegateCheckBox.Checked;
  err := f.SetAction(fa, hc);
  If err <> 0 Then
    begin
    ErrorMessage(IntToStr(err));
    Exit;
    end;

  If fa = ffaPassToFilter Then
    begin
    err := f.AddNext(passTarget);
    If err <> 0 Then
      begin
      ErrorMessage(IntToStr(err));
      Exit;
      end;
    end;

  FFilterList.Add(f);
  L := FilterListVIew.Items.Add;
  f.Enabled := EnabledCheckBox.Checked;
  FilterListViewData(FilterListView, L);
  f := Nil;
Finally
  If Assigned(f) Then
    f.Free;
  end;
end;

Procedure TFilterFrm.CloseButtonClick(Sender: TObject);
begin
Close;
end;

Procedure TFilterFrm.DeleteButtonClick(Sender: TObject);
Var
  L : TListItem;
begin
L := FilterListView.Selected;
If Assigned(L) Then
  L.Delete;
end;

Procedure TFilterFrm.EnableCombobox(AType:EFilterComboboxType; AEnable:Boolean);
Var
  c : TComboBox;
begin
c := FCBs[Ord(AType)];
c.Enabled := AEnable;
Case c.Enabled Of
  False : c.Color := clBtnFace;
  True : c.Color := clWindow;
  end;
end;


End.

