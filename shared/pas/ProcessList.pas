Unit ProcessList;

Interface

Uses
  Generics.Collections,
  RefObject;

Type
  TImageEntry = Class (TRefObject)
    Public
      Time : UInt64;
      BaseAddress : Pointer;
      ImageSize : NativeUInt;
      FiileName : WideString;
    end;

  TProcessEntry = Class (TRefObject)
    Private
      FImageName : WideString;
      FBaseName : WideString;
      FCommandLine : WideString;
      FProcessId : Cardinal;
      FCreatorProcessId : Cardinal;
      FTerminated : Boolean;
      FImageMap : TObjectList<TImageEntry>;
    Public
      Constructor Create(AProcessId:Cardinal; ACreatorProcessId:Cardinal; AFileName:WideString; ACommandLine:WideString); Reintroduce;
      Destructor Destroy; Override;

      Procedure AddImage(ATime:UInt64; ABase:Pointer; ASize:NativeUInt; AFileName:WideString);
      Procedure Terminate;
      Function ImageByAddress(AAddress:Pointer; AImageList:TList<TImageEntry>):Boolean; Overload;
      Function ImageByAddress(AAddress:Pointer):TImageEntry; Overload;
      Procedure EnumImages(AList:TObjectList<TImageEntry>);

      Property ImageName : WideString Read FImageName;
      Property BaseName : WideString Read FBaseName;
      Property CommandLine : WideString Read FCommandLine;
      Property ProcessId : Cardinal Read FProcessId;
      Property CreatorProcessId : Cardinal Read FCreatorProcessId;
      Property Terminated : Boolean Read FTerminated;
    end;

Implementation

Uses
  SysUtils,
  Generics.Defaults;

(** TProcessEntry **)

Constructor TProcessEntry.Create(AProcessId:Cardinal; ACreatorProcessId:Cardinal; AFileName:WideString; ACommandLine:WideString);
begin
Inherited Create;
FCreatorProcessId := ACreatorProcessId;
FProcessId := AProcessId;
FImageName := AFileName;
FBaseName := ExtractFileName(AFileName);
FCommandLine := ACommandLine;
FImageMap := TObjectList<TImageEntry>.Create;
end;

Destructor TProcessEntry.Destroy;
begin
FImageMap.Free;
Inherited Destroy;
end;

Procedure TProcessEntry.AddImage(ATime:UInt64; ABase:Pointer; ASize:NativeUInt; AFileName:WideString);
Var
  ie : TImageEntry;
begin
ie := TImageEntry.Create;
ie.Time := ATime;
ie.BaseAddress := ABase;
ie.ImageSize := ASize;
ie.FiileName := AFileName;
FImageMap.Add(ie);
FImageMap.Sort(
  TComparer<TImageEntry>.Construct(
    Function(const A, B: TImageEntry): Integer
    begin
    Result := 0;
    If A.Time < B.Time Then
      Result := -1
    Else If A.Time > B.Time Then
      Result := 1;
    end
  )
);
end;

Procedure TProcessEntry.Terminate;
begin
FTerminated := True;
end;

Function TProcessEntry.ImageByAddress(AAddress:Pointer; AImageList:TList<TImageEntry>):Boolean;
Var
  ie : TImageEntry;
begin
Result := False;
For ie In FImageMap Do
  begin
  If (NativeUInt(ie.BaseAddress) <= NativeUInt(AAddress)) And
    (NativeUInt(AAddress) < NativeUInt(ie.BaseAddress) + ie.ImageSize) Then
      begin
      ie.Reference;
      AImageList.Add(ie);
      Result := True;
      end;
  end;
end;

Function TProcessEntry.ImageByAddress(AAddress:Pointer):TImageEntry;
Var
  ie : TImageEntry;
begin
Result := Nil;
For ie In FImageMap Do
  begin
  If (NativeUInt(ie.BaseAddress) <= NativeUInt(AAddress)) And
    (NativeUInt(AAddress) < NativeUInt(ie.BaseAddress) + ie.ImageSize) Then
      begin
      ie.Reference;
      Result := ie;
      Break;
      end;
  end;
end;

Procedure TProcessEntry.EnumImages(AList:TObjectList<TImageEntry>);
Var
  ie : TImageEntry;
begin
For ie In FImageMap Do
  AList.Add(ie.Reference As TImageEntry);
end;


End.
