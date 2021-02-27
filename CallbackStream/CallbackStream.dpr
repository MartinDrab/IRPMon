Library CallbackStream;

uses
  ShareMem,
  System.SysUtils,
  System.Classes,
  CallbackStreamImpl in '..\shared\pas\CallbackStreamImpl.pas';

{$R *.RES}

Function CallbackStreamCreate(AReadCallback:TCallbackStreamOnRead; AWriteCallback:TCallbackStreamOnWrite; AReadContext:Pointer; AWriteContext:Pointer):Pointer; Cdecl;
begin
Try
  Result := TCallbackStream.Create(AReadCallback, AWriteCallback, AReadContext, AWriteContext);
Except
  Result := Nil;
  end;
end;

Procedure CallbackStreamFree(AStream:Pointer); Cdecl;
Var
  cs : TCallbackStream;
begin
cs := AStream;
cs.Free;
end;


Exports
  CallbackStreamCreate,
  CallbackStreamFree;


Begin
End.
