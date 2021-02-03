Unit RefObject;

Interface

Type
  TRefObject = Class
  Private
    FReferenceCount : Integer;
  Public
    Constructor Create; Reintroduce;

    Function Reference:TRefObject;
    Procedure Free; Reintroduce;

    Property ReferenceCount : Integer Read FReferenceCount;
  end;

Implementation

Uses
  Windows;

Constructor TRefObject.Create;
begin
Inherited Create;
InterlockedExchange(FReferenceCount, 1);
end;

Function TRefObject.Reference:TRefObject;
begin
InterlockedIncrement(FReferenceCount);
Result := Self;
end;

Procedure TRefObject.Free;
begin
If (Assigned(Self)) And
  (InterlockedDecrement(FReferenceCount) = 0) Then
  Inherited Free;
end;


End.
