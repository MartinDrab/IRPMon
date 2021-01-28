unit AddDeviceRequest;

{$IFDEF FPC}
{$MODE Delphi}
{$ENDIF}

Interface

Uses
  IRPMonDLl,
  AbstractRequest,
  IRPMonRequest;

Type
  TAddDeviceRequest = Class (TDriverRequest)
  Public
    Constructor Create(Var ARequest:REQUEST_ADDDEVICE); Overload;
  end;

Implementation

Constructor TAddDeviceRequest.Create(Var ARequest:REQUEST_ADDDEVICE);
begin
Inherited Create(ARequest.Header);
end;


End.
