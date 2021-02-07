Unit RefObject;

Interface

Uses
  Generics.Collections;

Type
  TRefObject = Class
  Private
    FReferenceCount : Integer;
  Public
    Constructor Create; Reintroduce;

    Function Reference:TRefObject;
    Procedure Free; Reintroduce;
    Procedure DisposeOf; Reintroduce;

    Property ReferenceCount : Integer Read FReferenceCount;
  end;

  TRefObjectList<T: TRefObject> = Class (TObjectList<T>)
  Protected
    Procedure Notify(const Value: T; Action: TCollectionNotification); Override;
  Public
  end;

  TRefObjectDictionary<K;V:TRefObject> = Class (TObjectDictionary<K, V>)
  Protected
    Procedure ValueNotify(const Value: V; Action: TCollectionNotification); Override;
  Public
    Constructor Create; Reintroduce;
  end;

Implementation

Uses
  Windows;

(** TRefObject **)

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

Procedure TRefObject.DisposeOf;
begin
Free;
end;

(** TRefObjectList **)

Procedure TRefObjectList<T>.Notify(const Value: T; Action: TCollectionNotification);
begin
If Assigned(Value) THen
  begin
  Case action Of
    cnAdded: Value.Reference;
    cnRemoved: Value.Free;
    end;
  end;

If Assigned(OnNotify) Then
  OnNotify(Self, Value, Action);
end;


(** TRefObjectDictionary **)

Constructor TRefObjectDictionary<K,V>.Create;
begin
Inherited Create([doOwnsValues]);
end;

Procedure TRefObjectDictionary<K,V>.ValueNotify(const Value: V; Action: TCollectionNotification);
begin
Case Action Of
  cnAdded : Value.Reference;
  cnRemoved : Value.Free;
  end;

If Assigned(OnValueNotify) Then
  OnValueNotify(Self, Value, Action);
end;


End.

