Unit RequestDetailsForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils, Variants,
  Classes, Graphics,
  Controls, Forms, Dialogs, ExtCtrls, StdCtrls,
  RequestListModel, ComCtrls;

Type
  TRequestDetailsFrm = Class (TForm)
    LowerPanel: TPanel;
    OkButton: TButton;
    PageControl1: TPageControl;
    HeadersTabSheet: TTabSheet;
    RawDataTabSheet: TTabSheet;
    NameValueListView: TListView;
    DataRichEdit: TRichEdit;
    Procedure OkButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  Private
    FRequest : TDriverRequest;
  Public
    Constructor Create(AOwner:TComponent; ARequest:TDriverRequest); Reintroduce;
  end;


Implementation

Uses
  Utils;

Constructor TRequestDetailsFrm.Create(AOwner:TComponent; ARequest:TDriverRequest);
begin
FRequest := ARequest;
Inherited Create(AOwner);
end;

{$R *.dfm}

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
  DataRichEdit.Text := BufferToHex(FRequest.Data, FRequest.DataSize);
end;

Procedure TRequestDetailsFrm.OkButtonClick(Sender: TObject);
begin
Close;
end;

End.

