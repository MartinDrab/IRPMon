Unit RequestDetailsForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils, Variants,
  Classes, Graphics,
  Controls, Forms, Dialogs, ExtCtrls, StdCtrls,
  IRPMonRequest, ComCtrls,
  Generics.Collections, DataParsers, SymTables,
  Menus;

Type
  TRequestDetailsFrm = Class (TForm)
    LowerPanel: TPanel;
    OkButton: TButton;
    PageControl1: TPageControl;
    HeadersTabSheet: TTabSheet;
    NameValueListView: TListView;
    HeaderPopupMenu: TPopupMenu;
    CopyValueMenuItem: TMenuItem;
    CopyLineMenuItem: TMenuItem;
    CopyAllMenuItem: TMenuItem;
    StackTabSheet: TTabSheet;
    StackListView: TListView;
    Procedure OkButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure CopyClick(Sender: TObject);
    procedure HeaderPopupMenuPopup(Sender: TObject);
  Private
    FRequest : TDriverRequest;
    FParsers : TObjectList<TDataParser>;
    FSymStore : TModuleSymbolStore;
    Procedure ProcessParsers;
    Procedure ProcessStack;
  Public
    Constructor Create(AOwner:TComponent; ARequest:TDriverRequest; AParsers:TObjectList<TDataParser>; ASymStore:TModuleSymbolStore); Reintroduce;
  end;


Implementation

Uses
  Clipbrd,
  Utils,
  IRPMonDll,
  RefObject,
  ProcessList;

Procedure TRequestDetailsFrm.CopyClick(Sender: TObject);
Var
  L : TListItem;
  t : WideString;
begin
t := '';
L := NameValueListView.Selected;
If Sender = CopyValueMenuItem Then
  t := L.SubItems[0]
Else If Sender = CopyLineMenuItem Then
  t := L.Caption + #9 + L.SubItems[0]
Else If Sender = CopyAllMenuItem Then
  begin
  For L In NameValueListView.Items Do
    t := t + L.Caption + #9 + L.SubItems[0] + #13#10;
  end;

If t <> '' Then
  Clipboard.AsText := t;
end;

Constructor TRequestDetailsFrm.Create(AOwner:TComponent; ARequest:TDriverRequest; AParsers:TObjectList<TDataParser>; ASymStore:TModuleSymbolStore);
begin
FRequest := ARequest;
FParsers := AParsers;
FSymStore := ASymStore;
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
{$IFDEF FPC}
  re : TMemo;
{$ELSE}
  re : TRichEdit;
{$ENDIF}
begin
names := TStringList.Create;
values := TStringList.Create;
For pd In FParsers Do
  begin
  err := pd.Parse(rlfText, FRequest, _handled, names, values);
  If (err = ERROR_SUCCESS) And (_handled) Then
    begin
    tb := TTabSheet.Create(PageControl1);
    tb.Parent := PageControl1;
    tb.Caption := pd.Name;
    tb.PageControl := PageControl1;
{$IFDEF FPC}
    re := TMemo.Create(tb);
{$ELSE}
    re := TRichEdit.Create(tb);
{$ENDIF}
    re.Parent := tb;
    re.Align := alClient;
{$IFNDEF FPC}
    re.PlainText := True;
{$ENDIF}
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

Procedure TRequestDetailsFrm.ProcessStack;
Var
  I : Integer;
  pe : TProcessEntry;
  pframe : PPointer;
  il : TRefObjectList<TImageEntry>;
  ie : TImageEntry;
  st : TSymTable;
  symbolName : WideString;
  offset : UInt64;
  ms : TModuleSymbol;
begin
pe := FRequest.Process;
il := TRefObjectList<TImageEntry>.Create;
pframe := FRequest.StackFrames;
For I := 0 To FRequest.StackFrameCount - 1 Do
  begin
  With StackListView.Items.Add Do
    begin
    Caption := Format('%d', [I]);
    SubItems.Add(Format('0x%p', [pframe^]));
    If pe.ImageByAddress(pframe^, il) Then
      begin
      symbolName := '';
      offset := NativeUInt(pframe^) - NativeUInt(ie.BaseAddress);
      ie := il[0];
      SubItems.Add(ExtractFileName(ie.FileName));
      st := FSymStore.Module[ExtractFileName(ie.FileName)];
      If Assigned(st) Then
        begin
        ms := st.FindSymbol(offset);
        If Assigned(ms) Then
          begin
          symbolName := ms.Name;
          ms.Free;
          end;
        end;

      SubItems.Add(symbolName);
      SubItems.Add(Format('0x%x', [offset]));
      il.Clear;
      end;
    end;

  Inc(pframe);
  end;

il.Free;
pe.Free;
end;

Procedure TRequestDetailsFrm.FormCreate(Sender: TObject);
Var
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

If FRequest.StackFrameCount > 0 Then
  ProcessStack;

If FRequest.DataSize > 0 Then
  ProcessParsers;
end;

Procedure TRequestDetailsFrm.HeaderPopupMenuPopup(Sender: TObject);
Var
  L : TListItem;
begin
L := NameValueListView.Selected;
CopyValueMenuItem.Enabled := Assigned(L);
CopyLineMenuItem.Enabled := Assigned(L);
end;

Procedure TRequestDetailsFrm.OkButtonClick(Sender: TObject);
begin
Close;
end;

End.

