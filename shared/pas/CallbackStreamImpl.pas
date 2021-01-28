Unit CallbackStreamImpl;

Interface

Uses
  Classes, SysUtils;

Type
  TCallbackStream = Class;
  TCallbackStreamOnRead = Function (ABuffer:Pointer; ALength:Cardinal; AStream:TCallbackStream; AReadContext:Pointer):Cardinal; Cdecl;
  TCallbackStreamOnWrite = Function (ABuffer:Pointer; ALength:Cardinal; AStream:TCallbackStream; AWriteContext:Pointer):Cardinal; Cdecl;

  TCallbackStream = Class (TStream)
  Private
    FReadCallback : TCallbackStreamOnRead;
    FReadContext : Pointer;
    FWriteCallback : TCallbackStreamOnWrite;
    FWriteContext : Pointer;
  Public
    Constructor Create(AReadCallback:TCallbackStreamOnRead; AWriteCallback:TCallbackStreamOnRead; AReadContext:Pointer = Nil; AWriteContext:Pointer = Nil); Reintroduce;

    Function Read(var Buffer; Count: Longint): Longint; Override;
    Function Write(const Buffer; Count: Longint): Longint; Override;
  end;

Implementation

Constructor TCallbackStream.Create(AReadCallback:TCallbackStreamOnRead; AWriteCallback:TCallbackStreamOnRead; AReadContext:Pointer = Nil; AWriteContext:Pointer = Nil);
begin
Inherited Create;
FReadCallback := AReadCallback;
FReadContext := AReadContext;
FWriteCallback := AWriteCallback;
FWriteContext := AWriteContext;
end;

Function TCallbackStream.Read(var Buffer; Count: Longint): Longint;
begin
Result := FReadCallback(@Buffer, Count, Self, FReadContext);
end;

Function TCallbackStream.Write(const Buffer; Count: Longint): Longint;
begin
Result := FWriteCallback(@Buffer, Count, Self, FWriteContext);
end;


End.
