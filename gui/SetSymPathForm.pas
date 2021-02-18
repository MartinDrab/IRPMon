Unit SetSymPathForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls, Vcl.ComCtrls,
  Vcl.Menus;

Type
  TSetSymPathFrm = Class (TForm)
    MainPanel: TPanel;
    StornoButton: TButton;
    OkButton: TButton;
    SymListView: TListView;
    SymPopupMenu: TPopupMenu;
    AddMenuItem: TMenuItem;
    EditMenuItem: TMenuItem;
    DeleteMenuItem: TMenuItem;
    procedure StornoButtonClick(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
    procedure OnSymPopupMenuClick(Sender: TObject);
  Private
    FCancelled : Boolean;
    FSymPath : WideString;
    Function ListItemToSymPath(AItem:TListItem):WideString;
    Procedure SymPathToListItem(ASymPath:WideString; AItem:TListItem);
  Public
    Constructor Create(AOwner:TComponent; ASymPath:WideString); Reintroduce;

    Property Cancelled : Boolean Read FCancelled;
    Property SymPath : WideString Read FSymPath;
  end;

Implementation

Uses
  AddEditSymServerForm;

Constructor TSetSymPathFrm.Create(AOwner:TComponent; ASymPath:WideString);
begin
FCancelled := True;
FSymPath := ASymPath;
Inherited Create(AOwner);
end;

{$R *.DFM}

Function TSetSymPathFrm.ListItemToSymPath(AItem:TListItem):WideString;
begin
If AItem.SubItems[0] <> '' Then
  Result := Format('srv*%s*%s', [AItem.Caption, AItem.SubItems[0]])
Else Result := AItem.Caption;
end;

Procedure TSetSymPathFrm.SymPathToListItem(ASymPath:WideString; AItem:TListItem);
Var
  starIndex : Integer;
begin
If Pos(WideString('srv*'), ASymPath) = 1 Then
  Delete(ASymPath, 1, Length('srv*'));

starIndex := Pos(WideString('*'), ASymPath);
If starIndex > 0 Then
  begin
  AItem.Caption := Copy(ASymPath, 1, starIndex - 1);
  AItem.SubItems[0] := Copy(ASymPath, starIndex + 1, Length(ASymPath) - starIndex - 1);
  end
Else AItem.Caption := ASymPath;
end;

Procedure TSetSymPathFrm.OkButtonClick(Sender: TObject);
Var
  L : TListItem;
  onePart : WideString;
begin
FSymPath := '';
For L In SymListView.Items Do
  begin
  onePart := ListItemToSymPath(L);
  If FSymPath <> '' Then
    FSymPath := FSymPath + ';';

  FSymPath := FSymPath + onePart;
  end;

FCancelled := False;
Close;
end;

Procedure TSetSymPathFrm.OnSymPopupMenuClick(Sender: TObject);
Var
   L : TListItem;
   sp : WideString;
begin
L := SymListView.Selected;
If Sender = SymPopupMenu Then
  begin
  EditMenuItem.Enabled := Assigned(L);
  DeleteMenuItem.Enabled := Assigned(L);
  end
Else If (Sender = AddMenuItem) Or
  (Sender = EditMenuItem) Then
  begin
  sp := '';
  If Sender = EditMenuItem Then
    sp := ListItemToSymPath(L)
  Else L := Nil;

  With TAddEditSymServerFrm.Create(Application, sp) Do
    begin
    ShowModal;
    If Not Cancelled Then
      begin
      sp := OneSymPath;
      If Not Assigned(L) Then
        begin
        L := SymListView.Items.Add;
        L.SubItems.Add('');
        end;

      SymPathToListItem(sp, L);
      end;

    Free;
    end;
  end
Else If Sender = DeleteMenuItem Then
  L.Delete;
end;

Procedure TSetSymPathFrm.StornoButtonClick(Sender: TObject);
begin
Close;
end;


End.
