Unit AddEditSymServerForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls;

Type
  TAddEditSymServerFrm = Class(TForm)
    MainPanel: TPanel;
    StornoButton: TButton;
    OkButton: TButton;
    LocalEdit: TEdit;
    BrowseButton: TButton;
    Label1: TLabel;
    ServerEdit: TEdit;
    Label2: TLabel;
    LocalOpenDialog: TFileOpenDialog;
    procedure StornoButtonClick(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure BrowseButtonClick(Sender: TObject);
  Private
    FCancelled : Boolean;
    FSymPath : WideString;
  Public
    Constructor Create(AOwner:TComponent; APath:WideString = ''); Reintroduce;

    Property Cancelled : Boolean Read FCancelled;
    Property OneSymPath : WideString Read FSymPath;
  end;

Implementation

Constructor TAddEditSymServerFrm.Create(AOwner:TComponent; APath:WideString = '');
begin
FSymPath := APath;
FCancelled := True;
Inherited Create(AOwner);
end;

{$R *.DFM}


Procedure TAddEditSymServerFrm.BrowseButtonClick(Sender: TObject);
begin
If LocalOpenDialog.Execute Then
  LocalEdit.Text := LocalOpenDialog.FileName;
end;

Procedure TAddEditSymServerFrm.FormCreate(Sender: TObject);
Var
  starIndex : Integer;
begin
If Pos(WideString('srv*'), FSymPath) = 1 Then
  Delete(FSymPath, 1, Length('srv*'));

starIndex := Pos(WideString('*'), FSymPath);
If starIndex > 0 Then
  begin
  LocalEdit.Text := Copy(FSymPath, 1, starIndex - 1);
  ServerEdit.Text := Copy(FSymPath, starIndex + 1, Length(FSymPath) - starIndex - 1);
  end
Else LocalEdit.Text := FSymPath;
end;

procedure TAddEditSymServerFrm.OkButtonClick(Sender: TObject);
begin
FSymPath := '';
If ServerEdit.Text <> '' Then
  FSymPath := 'srv*';

If LocalEdit.Text <> '' Then
  FSymPath := FSymPath + LocalEdit.Text;

If ServerEdit.Text <> '' Then
  FSymPath := FSymPath + '*' + ServerEdit.Text;

FCancelled := False;
Close;
end;

Procedure TAddEditSymServerFrm.StornoButtonClick(Sender: TObject);
begin
Close;
end;


End.
