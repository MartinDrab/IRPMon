Unit HookProgressForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls,
  Vcl.StdCtrls, Vcl.ComCtrls,
  Generics.Collections, HookObjects;

Type
  THookProgressFrm = Class;
  THookProgressThread = Class (TThread)
    Private
      FForm : THookProgressFrm;
      FHookList : TList<THookObject>;
      FChangeList : TList<THookObject>;
      FUnhookList : TList<THookObject>;
      FCurrentObject : THookObject;
      FCurrentOp : WideString;
      FStatus : Cardinal;
      Procedure UpdateGUI;
    Protected
      Procedure Execute; Override;
    Public
      Constructor Create(ACreateSuspended:Boolean; AForm:THookProgressFrm; AHookList:TList<THookObject>; AUnhookList:TList<THookObject>; AChangeList:TList<THookObject>); Reintroduce;
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
    FHookList : TList<THookObject>;
    FChangeList : TList<THookObject>;
    FUnhookList : TList<THookObject>;
    FThread : THookProgressThread;
  Public
    Constructor Create(AOwner:TComponent; AHookList:TList<THookObject>; AUnhookList:TList<THookObject>; AChangeList:TList<THookObject>); Reintroduce;
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
  SubItems.Add(Format('0x%p', [FCurrentObject.Address]));
  SubItems.Add(Format('%u', [FStatus]));
  end;
end;


Procedure THookProgressThread.Execute;
Var
  ho : THookObject;
begin
FreeOnTerminate := False;
For ho In FChangeList Do
  begin
  FCurrentObject := ho;
  If (hooStop In ho.SupportedOperations) Then
    begin
    FCurrentOp := 'Stop';
    FStatus := ho.Stop;
    Synchronize(UpdateGUI);
    end;

  If (hooChange In ho.SupportedOperations) Then
    begin
    FCurrentOp := 'Change';
    FStatus := ho.Change;
    Synchronize(UpdateGUI);
    end;

  If (hooStart In ho.SupportedOperations) Then
    begin
    FCurrentOp := 'Start';
    FStatus := ho.Start;
    Synchronize(UpdateGUI);
    end;
  end;

For ho In FUnhookList Do
  begin
  FCurrentObject := ho;
  If (hooStop In ho.SupportedOperations) Then
    begin
    FCurrentOp := 'Stop';
    FStatus := ho.Stop;
    Synchronize(UpdateGUI);
    end;

  If (hooUnhook In ho.SupportedOperations) Then
    begin
    FCurrentOp := 'Unhook';
    FStatus := ho.Unhook;
    Synchronize(UpdateGUI);
    end;
  end;

For ho In FHookList Do
  begin
  FCurrentObject := ho;
  If (hooHook In ho.SupportedOperations) Then
    begin
    FCurrentOp := 'Hook';
    FStatus := ho.Hook;
    Synchronize(UpdateGUI);
    end;

  If (hooStart In ho.SupportedOperations) Then
    begin
    FCurrentOp := 'Start';
    FStatus := ho.Start;
    Synchronize(UpdateGUI);
    end;
  end;
end;

Constructor THookProgressThread.Create(ACreateSuspended:Boolean; AForm:THookProgressFrm; AHookList:TList<THookObject>; AUnhookList:TList<THookObject>; AChangeList:TList<THookObject>);
begin
FForm := AForm;
FHookList := AHookList;
FUnhookList := AUnhookList;
FChangeList := AChangeList;
Inherited Create(ACreateSuspended);
end;


{$R *.DFM}

Constructor THookPRogressFrm.Create(AOwner:TComponent; AHookList:TList<THookObject>; AUnhookList:TList<THookObject>; AChangeList:TList<THookObject>);
begin
FHookList := AHookList;
FUnhookList := AUnhookList;
FChangeList := AChangeList;
FThread := THookProgressThread.Create(True, Self, FHookList, FUnhookList, FChangeList);
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
