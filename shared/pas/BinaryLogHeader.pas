Unit BinaryLogHeader;

Interface

Const
  LOGHEADER_SIGNATURE           = $474f4c4e4d505249;
  LOGHEADER_VERSION             = 1;
  LOGHEADER_ARCHITECTURE_X86    = 1;
  LOGHEADER_ARCHITECTURE_X64    = 2;

Type
  TBinaryLogHeader = Packed Record
    Signature : UInt64;
    Version : Cardinal;
    Architecture : Cardinal;
    Class Function ArchitectureSupported(Var AHeader:TBinaryLogHeader):Boolean; Static;
    Class Function SignatureValid(Var AHeader:TBinaryLogHeader):Boolean; Static;
    Class Function VersionSupported(Var AHeader:TBinaryLogHeader):Boolean; Static;
    Class Procedure Fill(Var AHeader:TBinaryLogHeader); Static;
    end;
  PBinaryLogHeader = ^TBinaryLogHeader;


Implementation

Class Function TBinaryLogHeader.ArchitectureSupported(Var AHeader:TBinaryLogHeader):Boolean;
Var
  arch : Cardinal;
begin
{$IFDEF WIN32}
arch := LOGHEADER_ARCHITECTURE_X86;
{$ELSE}
arch := LOGHEADER_ARCHITECTURE_X64;
{$ENDIF}
Result := (arch = AHeader.Architecture);
end;

Class Function TBinaryLogHeader.SignatureValid(Var AHeader:TBinaryLogHeader):Boolean;
begin
Result := (AHeader.Signature = LOGHEADER_SIGNATURE);
end;

Class Procedure TBinaryLogHeader.Fill(Var AHeader:TBinaryLogHeader);
Var
  arch : Cardinal;
begin
AHeader.Signature := LOGHEADER_SIGNATURE;
AHeader.Version := LOGHEADER_VERSION;
{$IFDEF WIN32}
arch := LOGHEADER_ARCHITECTURE_X86;
{$ELSE}
arch := LOGHEADER_ARCHITECTURE_X64;
{$ENDIF}
AHeader.Architecture := arch;
end;

Class Function TBinaryLogHeader.VersionSupported(Var AHeader:TBinaryLogHeader):Boolean;
begin
Result := (AHeader.Version = LOGHEADER_VERSION);
end;



End.
