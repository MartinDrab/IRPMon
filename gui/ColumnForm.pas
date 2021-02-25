Unit ColumnForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils,
  System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs,
  Vcl.ExtCtrls, Vcl.StdCtrls, Vcl.ComCtrls,
  Generics.Collections,
  ListModel, IRPMonRequest;

Type
  TColumnFrm = Class (TForm)
    LowerPanel: TPanel;
    MainPanel: TPanel;
    StornoButton: TButton;
    OkButton: TButton;
    ColumnListView: TListView;
    procedure StornoButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
  Private
    FModel : TListModel<TDriverRequest>;
    FCancelled : Boolean;
    FChecked : TList<Boolean>;
  Public
    Constructor Create(AOwner:TComponent; AModel:TListModel<TDriverRequest>); Reintroduce;
    Destructor Destroy; Override;

    Property Checked : TList<Boolean> Read FChecked;
    Property Cancelled : Boolean Read FCancelled;
  end;

Implementation

Constructor TColumnFrm.Create(AOwner:TComponent; AModel:TListModel<TDriverRequest>);
begin
FCancelled := True;
FModel := AModel;
FChecked := TList<Boolean>.Create;
Inherited Create(AOwner);
end;

Destructor TColumnFrm.Destroy;
begin
FChecked.Free;
Inherited Destroy;
end;

{$R *.DFM}

Procedure TColumnFrm.FormCreate(Sender: TObject);
Var
  I : Integer;
  c : TListModelColumn;
begin
For I := 0 To FModel.ColumnCount - 1 Do
  begin
  c := FModel.Columns[I];
  With ColumnListView.Items.Add Do
    begin
    Caption := c.Caption;
    Checked := c.Visible;
    FChecked.Add(Checked);
    end;
  end;
end;

Procedure TColumnFrm.OkButtonClick(Sender: TObject);
Var
  I : Integer;
begin
For I := 0 To COlumnListView.Items.Count - 1 Do
  FChecked[I] := ColumnListView.Items[I].Checked;

FCancelled := False;
Close;
end;

Procedure TColumnFrm.StornoButtonClick(Sender: TObject);
begin
Close;
end;


End.

