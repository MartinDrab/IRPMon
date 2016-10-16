Unit HookProgressForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics,
  Controls, Forms, Dialogs, ExtCtrls,
  StdCtrls, ComCtrls,
  Generics.Collections, HookObjects;

Type
  THookProgressFrm = Class;
  THookProgressThread = Class (TThread)
    Private
      FForm : THookProgressFrm;
      FOpList : TTaskOperationList;
      FCurrentObject : TTaskObject;
      FCurrentOp : WideString;
      FStatus : Cardinal;
      Procedure UpdateGUI;
    Protected
      Procedure Execute; Override;
    Public
      Constructor Create(ACreateSuspended:Boolean; AForm:THookProgressFrm; AOpList:TTaskOperationList); Reintroduce;
    end;

  THookProgressFrm = Class (TForm)
    LowerPanel: TPanel;
    CloseButton: TButton;
    ProgressListView: TListView;
    procedure CloseButtonClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormCreate(Sender: TObject);
    procedure ProgressListViewAdvancedCustomDrawItem(Sender: TCustomListView;
      Item: TListItem; State: TCustomDrawState; Stage: TCustomDrawStage;
      var DefaultDraw: Boolean);
  Private
    FThread : THookProgressThread;
  Public
    Constructor Create(AOwner:TComponent; AOpList:TTaskOperationList); Reintroduce;
  end;


Implementation


Procedure THookProgressThread.UpdateGUI;
Var
  lw : TListView;
begin
lw := FForm.ProgressListView;
With lw.Items.Add Do
  begin
  Data := Pointer(FStatus);
  Caption := FCurrentOp;
  SubItems.Add(FCurrentObject.ObjectTpye);
  SubItems.Add(FCurrentObject.Name);
  If FCurrentObject Is THookObject Then
     SubItems.Add(Format('0x%p', [(FCurrentObject As THookObject).Address]))
  Else SubItems.Add('');

  SubItems.Add(Format('%u', [FStatus]));
  end;
end;


Procedure THookProgressThread.Execute;
Var
  I : Integer;
  p : TPair<EHookObjectOperation, TTaskObject>;
begin
FreeOnTerminate := False;
I := 0;
While I < FOpList.Count Do
  begin
  p := FOpList.Items[I];
  FCurrentObject := p.Value;
  FCurrentOp := TTaskObject.OperationString(p.Key);
  FStatus := FCurrentObject.Operation(p.Key);
  Synchronize(UpdateGUI);
  Inc(I);
  end;

FOpList.Clear;
end;

Constructor THookProgressThread.Create(ACreateSuspended:Boolean; AForm:THookProgressFrm; AOpList:TTaskOperationList);
begin
FForm := AForm;
FOpList := AOpList;
Inherited Create(ACreateSuspended);
end;


{$R *.dfm}

Constructor THookProgressFrm.Create(AOwner:TComponent; AOpList:TTaskOperationList);
begin
FThread := THookProgressThread.Create(True, Self, AOpList);
Inherited Create(AOwner);
end;

Procedure THookProgressFrm.FormClose(Sender: TObject; var Action: TCloseAction);
begin
FThread.WaitFor;
FThread.Free;
end;

Procedure THookProgressFrm.FormCreate(Sender: TObject);
begin
FThread.Resume;
end;

Procedure THookProgressFrm.ProgressListViewAdvancedCustomDrawItem(
  Sender: TCustomListView; Item: TListItem; State: TCustomDrawState;
  Stage: TCustomDrawStage; var DefaultDraw: Boolean);
Var
  lw : TListView;
begin
lw := (Sender As TListView);
If Assigned(Item.Data) Then
  lw.Canvas.Font.Color := clRed
Else lw.Canvas.Font.Color := clGreen;
end;

Procedure THookProgressFrm.CloseButtonClick(Sender: TObject);
begin
Close;
end;

end.

