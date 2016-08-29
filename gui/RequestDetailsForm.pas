Unit RequestDetailsForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls,
  RequestListModel, Vcl.ComCtrls;

Type
  TRequestDetailsFrm = Class (TForm)
    LowerPanel: TPanel;
    OkButton: TButton;
    NameValueListView: TListView;
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

{$R *.DFM}

procedure TRequestDetailsFrm.FormCreate(Sender: TObject);
Var
  value : WIdeString;
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
end;

Procedure TRequestDetailsFrm.OkButtonClick(Sender: TObject);
begin
Close;
end;

End.
