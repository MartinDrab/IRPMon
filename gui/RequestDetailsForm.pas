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
    NameValueListView: TListView;
    DataPanel: TPanel;
    DataRichEdit: TRichEdit;
    Procedure OkButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  Private
    FRequest : TDriverRequest;
  Public
    Constructor Create(AOwner:TComponent; ARequest:TDriverRequest); Reintroduce;
  end;


Implementation

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
  begin
  hexLine := '';
  dispLine := '';
  index := 0;
  d := FRequest.Data;
  For I := 0 To FRequest.DataSize - 1 Do
    begin
    hexLine := hexLine + ' ' + IntToHex(d^, 2);
    If d^ >= Ord(' ') Then
      dispLine := dispLine + Chr(d^)
    Else dispLine := dispLine + '.';

    Inc(Index);
    Inc(d);
    If index = 16 Then
      begin
      DataRichEdit.Lines.Add(Format('%s  %s', [hexLine, dispLine]));
      hexLine := '';
      dispLine := '';
      index := 0;
      end;
    end;

  If index > 0 Then
    begin
    For I := index To 16 - 1 Do
      hexLine := hexLine + '   ';

    DataRichEdit.Lines.Add(Format('%s  %s', [hexLine, dispLine]));
    end;
  end;
end;

Procedure TRequestDetailsFrm.OkButtonClick(Sender: TObject);
begin
Close;
end;

End.
