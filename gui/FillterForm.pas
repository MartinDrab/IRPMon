Unit FillterForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls,
  Generics.Collections, RequestFilter, RequestListModel, Vcl.ExtCtrls;

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
    Procedure FormCreate(Sender: TObject);
    procedure FilterTypeComboBoxChange(Sender: TObject);
    procedure FilterColumnComboBoxChange(Sender: TObject);
    procedure FilterActionComboBoxChange(Sender: TObject);
    procedure CloseButtonClick(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
  Private
    FRequest : TDriverRequest;
    FFilter : TRequestFilter;
    FCBs : Array [0..Ord(fctMax)-1] of TComboBox;
    Procedure EnableCombobox(AType:EFilterComboboxType; AEnable:Boolean);
  end;


Implementation

{$R *.DFM}

Uses
  IRPMonDll, IRPRequest, FastIoRequest,
  FileObjectNameXxxRequest, XXXDetectedRequests;

Procedure TFilterFrm.FilterActionComboBoxChange(Sender: TObject);
Var
  fa : EFilterAction;
  c : TComboBox;
begin
c := (Sender As TComboBox);
fa := EFilterAction(c.ItemIndex);
HighlightColorColorBox.Visible := (fa = ffaHighlight);
HighlightColorColorBox.Enabled := (fa = ffaHighlight);
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

Procedure TFilterFrm.FormCreate(Sender: TObject);
Var
  bm : Boolean;
  index : Integer;
  ss : TList<UInt64>;
  ts : TList<WideString>;
  tmp : TRequestFilter;
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
FFilter := TRequestFilter.NewInstance(ertUndefined);
FFilter.SetCondition(rlmctRequestType, rfoAlwaysTrue, 0);
FFilter.GetPossibleValues(ss, ts, bm);
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
end;

Procedure TFilterFrm.OkButtonClick(Sender: TObject);
begin
Close;
end;

Procedure TFilterFrm.CloseButtonClick(Sender: TObject);
begin
Close;
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
