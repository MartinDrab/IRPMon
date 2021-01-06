Unit StartIoRequest;

{$IFDEF FPC}
{$MODE Delphi}
{$ENDIF}

Interface

Uses
  IRPMonDll,
  AbstractRequest,
  IRPMonRequest;

Type
  TStartIoRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_STARTIO); Overload;
  end;

Implementation

Constructor TStartIoRequest.Create(Var ARequest:REQUEST_STARTIO);
Var
  d : Pointer;
begin
Inherited Create(ARequest.Header);
d := PByte(@ARequest) + SizeOf(aRequest);
AssignData(d, ARequest.DataSize);
end;


End.
