Unit ProcessList;

Interface

Uses
  Generics.Collections,
  ProcessXXXRequests,
  ImageLoadRequest,
  RefObject;

Type
  TImageEntry = Class (TRefObject)
    Public
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
      Constructor Create(ARequest:TProcessCreatedRequest); Reintroduce;
      Destructor Destroy; Override;

      Procedure AddImage(ARequest:TImageLoadRequest);
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

(** TProcessEntry **)

Constructor TProcessEntry.Create(ARequest:TProcessCreatedRequest);
begin
Inherited Create;
FCreatorProcessId := ARequest.ProcessId;
FProcessId := Cardinal(ARequest.DriverObject);
FImageName := ARequest.FileName;
FBaseName := ARequest.DriverName;
FCommandLine := ARequest.DeviceName;
FImageMap := TObjectList<TImageEntry>.Create;
end;

Destructor TProcessEntry.Destroy;
begin
FImageMap.Free;
Inherited Destroy;
end;

Procedure TProcessEntry.AddImage(ARequest:TImageLoadRequest);
Var
  ie : TImageEntry;
begin
If Not ARequest.KernelDriver Then
  begin
  ie := TImageEntry.Create;
  ie.BaseAddress := ARequest.ImageBase;
  ie.ImageSize := ARequest.ImageSize;
  ie.FiileName := ARequest.FileName;
  FImageMap.Add(ie);
  end;
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
