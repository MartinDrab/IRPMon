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
      FCurrentOpType : EHookObjectOperation;
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
    FOperationCount : Cardinal;
    FSuccessCount : Cardinal;
    FErrorCount : Cardinal;
    FWarningCount : Cardinal;
    Procedure OnThreadTerminated(Sender:TObject);
  Public
    Constructor Create(AOwner:TComponent; AOpList:TTaskOperationList); Reintroduce;
  end;


Implementation


Procedure THookProgressThread.UpdateGUI;
Var
  r : EHookObjectOperationResult;
  addrStr : WideString;
  lw : TListView;
begin
r := FCurrentObject.OperationResult(FCurrentOpType, FStatus);
Inc(FForm.FOperationCount);
Case r Of
  hoorSuccess: Inc(FForm.FSuccessCount);
  hoorWarning: Inc(FForm.FWarningCount);
  hoorError: Inc(FForm.FErrorCount);
  end;

lw := FForm.ProgressListView;
With lw.Items.Add Do
  begin
  Data := Pointer(r);
  Caption := FCurrentOp;
  SubItems.Add(FCurrentObject.ObjectTpye);
  addrStr := '';
  If FCurrentObject Is THookObject Then
     addrStr := Format(' (0x%p)', [(FCurrentObject As THookObject).Address]);

  SubItems.Add(Format('%s%s', [FCurrentObject.Name, addrStr]));
  SubItems.Add(Format('%u', [FStatus]));
  SubItems.Add(FCurrentObject.StatusDescription(FCurrentOpType, FStatus))
  end;
end;


Procedure THookProgressThread.Execute;
Var
  I : Cardinal;
  p : TPair<EHookObjectOperation, TTaskObject>;
begin
FreeOnTerminate := False;
I := 0;
While I < FOpList.Count Do
  begin
  p := FOpList.Items[I];
  FCurrentObject := p.Value;
  FCurrentOp := FCurrentObject.OperationString(p.Key);
  FCurrentOpType := p.Key;
  FStatus := FCurrentObject.Operation(p.Key);
  Synchronize(UpdateGUI);
  Inc(I);
  end;

FOpList.Clear;
FForm.CloseButton.Enabled := True;
end;

Constructor THookProgressThread.Create(ACreateSuspended:Boolean; AForm:THookProgressFrm; AOpList:TTaskOperationList);
begin
FForm := AForm;
FOpList := AOpList;
Inherited Create(ACreateSuspended);
end;


{$R *.DFM}

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

Procedure THookProgressFrm.OnThreadTerminated(Sender:TObject);
begin
If (FErrorCount = 0) Then
  Close;
end;

Procedure THookProgressFrm.FormCreate(Sender: TObject);
begin
FOperationCount := 0;
FSuccessCount := 0;
FErrorCount := 0;
FWarningCount := 0;
FThread.OnTerminate := OnThreadTerminated;
FThread.Start;
end;

Procedure THookProgressFrm.ProgressListViewAdvancedCustomDrawItem(
  Sender: TCustomListView; Item: TListItem; State: TCustomDrawState;
  Stage: TCustomDrawStage; var DefaultDraw: Boolean);
Var
  lw : TListView;
  r : EHookObjectOperationResult;
begin
lw := (Sender As TListView);
r := EHookObjectOperationResult(Item.Data);
Case r Of
  hoorSuccess: lw.Canvas.Font.Color := clGreen;
  hoorWarning: lw.Canvas.Font.Color := clBlue;
  hoorError: lw.Canvas.Font.Color := clRed;
  end;
end;

Procedure THookProgressFrm.CloseButtonClick(Sender: TObject);
begin
Close;
end;

end.

