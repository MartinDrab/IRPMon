Unit RequestDetailsForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils, Variants,
  Classes, Graphics,
  Controls, Forms, Dialogs, ExtCtrls, StdCtrls,
  RequestListModel, ComCtrls,
  Generics.Collections, DataParsers,
  Menus;

Type
  TRequestDetailsFrm = Class (TForm)
    LowerPanel: TPanel;
    OkButton: TButton;
    PageControl1: TPageControl;
    HeadersTabSheet: TTabSheet;
    NameValueListView: TListView;
    HeaderPopupMenu: TPopupMenu;
    CopyValueMenuItem: TMenuItem;
    CopyLineMenuItem: TMenuItem;
    CopyAllMenuItem: TMenuItem;
    Procedure OkButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure CopyClick(Sender: TObject);
    procedure HeaderPopupMenuPopup(Sender: TObject);
  Private
    FRequest : TDriverRequest;
    FParsers : TObjectList<TDataParser>;
    Procedure ProcessParsers;
  Public
    Constructor Create(AOwner:TComponent; ARequest:TDriverRequest; AParsers:TObjectList<TDataParser>); Reintroduce;
  end;


Implementation

Uses
  Clipbrd, Utils;

Procedure TRequestDetailsFrm.CopyClick(Sender: TObject);
Var
  L : TListItem;
  t : WideString;
begin
t := '';
L := NameValueListView.Selected;
If Sender = CopyValueMenuItem Then
  t := L.SubItems[0]
Else If Sender = CopyLineMenuItem Then
  t := L.Caption + #9 + L.SubItems[0]
Else If Sender = CopyAllMenuItem Then
  begin
  For L In NameValueListView.Items Do
    t := t + L.Caption + #9 + L.SubItems[0] + #13#10;
  end;

If t <> '' Then
  Clipboard.AsText := t;
end;

Constructor TRequestDetailsFrm.Create(AOwner:TComponent; ARequest:TDriverRequest; AParsers:TObjectList<TDataParser>);
begin
FRequest := ARequest;
FParsers := AParsers;
Inherited Create(AOwner);
end;

{$R *.dfm}

Procedure TRequestDetailsFrm.ProcessParsers;
Var
  I : Integer;
  err : Cardinal;
  _handled : ByteBool;
  names : TStringList;
  values : TStringList;
  pd : TDataParser;
  tb : TTabSheet;
{$IFDEF FPC}
  re : TMemo;
{$ELSE}
  re : TRichEdit;
{$ENDIF}
begin
names := TStringList.Create;
values := TStringList.Create;
For pd In FParsers Do
  begin
  err := pd.Parse(FRequest, _handled, names, values);
  If (err = ERROR_SUCCESS) And (_handled) Then
    begin
    tb := TTabSheet.Create(PageControl1);
    tb.Parent := PageControl1;
    tb.Caption := pd.Name;
    tb.PageControl := PageControl1;
{$IFDEF FPC}
    re := TMemo.Create(tb);
{$ELSE}
    re := TRichEdit.Create(tb);
{$ENDIF}
    re.Parent := tb;
    re.Align := alClient;
{$IFNDEF FPC}
    re.PlainText := True;
{$ENDIF}
    re.Font.Name := 'Courier New';
    For I := 0 To values.Count - 1 Do
      begin
      If names.Count > 0 Then
        re.Lines.Add(Format('%s: %s', [names[I], values[I]]))
      Else re.Lines.Add(values[I]);
      end;

    values.Clear;
    names.Clear;
    end;
  end;

values.Free;
names.Free;
end;

Procedure TRequestDetailsFrm.FormCreate(Sender: TObject);
Var
  value : WideString;
  ct : ERequestListModelColumnType;
begin
For ct := Low(ERequestListModelColumnType) To High(ERequestListModelColumnType) Do
  begin
  If FRequest.GetColumnValue(ct, value) Then
    begin
    With NameValueListVIew.Items.Add Do
      begin
      Caption := FRequest.GetColumnName(ct);
      SubItems.Add(value);
      end;
    end;
  end;

With NameValueListVIew.Items.Add Do
  begin
  Caption := 'Data size';
  SubItems.Add(Format('%d', [FRequest.DataSize]));
  end;

If FRequest.DataSize > 0 Then
  ProcessParsers;
end;

Procedure TRequestDetailsFrm.HeaderPopupMenuPopup(Sender: TObject);
Var
  L : TListItem;
begin
L := NameValueListView.Selected;
CopyValueMenuItem.Enabled := Assigned(L);
CopyLineMenuItem.Enabled := Assigned(L);
end;

Procedure TRequestDetailsFrm.OkButtonClick(Sender: TObject);
begin
Close;
end;

End.

