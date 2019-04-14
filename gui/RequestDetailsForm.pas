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
  Generics.Collections, DataParsers;

Type
  TRequestDetailsFrm = Class (TForm)
    LowerPanel: TPanel;
    OkButton: TButton;
    PageControl1: TPageControl;
    HeadersTabSheet: TTabSheet;
    NameValueListView: TListView;
    Procedure OkButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  Private
    FRequest : TDriverRequest;
    FParsers : TObjectList<TDataParser>;
    Procedure ProcessParsers;
  Public
    Constructor Create(AOwner:TComponent; ARequest:TDriverRequest; AParsers:TObjectList<TDataParser>); Reintroduce;
  end;


Implementation

Uses
  Utils;

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
  re : TRichEdit;
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
    re := TRichEdit.Create(tb);
    re.Parent := tb;
    re.Align := alClient;
    re.PlainText := True;
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
  d : PByte;
  hexLine : WideString;
  dispLine : WideString;
  I : Integer;
  index : Integer;
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

Procedure TRequestDetailsFrm.OkButtonClick(Sender: TObject);
begin
Close;
end;

End.

