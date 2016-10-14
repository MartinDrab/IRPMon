Unit LibJSON;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Const
  ERROR_JSON_SUCCESS             = 0;
  ERROR_JSON_UNTERMINATED_STRING = 1;
  ERROR_JSON_UNRECOGNIZED_CHARACTER = 2;
  ERROR_JSON_UNKNOWN_ESCAPE_SEQUENCE = 3;
  ERROR_JSON_INVALID_DIGIT      = 4;

Type
  EJSONTokenType = (
    jttString,
    jttInteger,
    jttFloat,
    jttBoolean,
    jttIdentifier,
    jttDelimiter,
    jttAttributeDelimiter,
    jttObjectStart,
    jttObjectEnd,
    jttArrayStart,
    jttArrayEnd,
    jttNull,
    jttInputEnd
  );

  TJSONToken = Record
    Line : Cardinal;
    TokenType : EJSONTokenType;
    Value : WideString;
    end;


Implementation

Uses
  SysUtils;

Function _ParseNumber(Var Buffer:PWideChar; Var AToken:TJSONToken):Cardinal;
Var
  tmp : Byte;
  upch : WideChar;
  base : Cardinal;
  negative : Boolean;
  intValue : UInt64;
  fValue : Extended;
  done : Boolean;
begin
intValue := 0;
fValue := 0.0;
negative := False;
Base := 10;
Dec(Buffer);
Result := ERROR_JSON_SUCCESS;
AToken.TokenType := jttInteger;
If (Buffer^ = '-') Or (Buffer^ = '+') Then
  begin
  negative := Buffer^ = '-';
  Inc(Buffer);
  end;

If (Buffer^ = '0') Then
  begin
  Inc(Buffer);
  upch := UpCase(Buffer^);
  Inc(Buffer);
  Case upch Of
    'b', 'B' : base := 2;
    'o', 'O' : base := 8;
    'x', 'X' : base := 16;
    Else begin
      Dec(Buffer);
      Dec(Buffer);
      end;
    end;
  end;

done := False;
While Not done Do
  begin
  Case Buffer^ Of
    '0'..'9',
    'a'..'f',
    'A'..'F' : begin
      upch := UpCase(Buffer^);
      If
        ((base = 2) And (upch >= '0') And (upch <= '1')) Or
        ((base = 8) And (upch >= '0') And (upch <= '7')) Or
        ((base = 10) And (upch >= '0') And (upch <= '9')) Or
        ((base = 16) And ((upch >= '0') And (upch <= '9') Or ((upch >= 'A') And (upch <= 'F')))) Then
        begin
        If upch >= 'A' Then
          tmp := Ord(upch) - Ord('A')
        Else tmp := Ord(upch) - Ord('0');

        intValue := intValue*base + tmp;
        fValue := fValue*base + tmp;
        Inc(Buffer);
        end
      Else Result := ERROR_JSON_INVALID_DIGIT;
      end;
    '.' : begin
      done := ATOken.TokenType = jttFloat;
      If Not done Then
        begin
        AToken.TokenType := jttFloat;
        Inc(Buffer);
        end;
      end;
    'g'..'z',
    'G'..'Z' : Result := ERROR_JSON_INVALID_DIGIT;
    Else done := True;
    end;

  done := (done) Or (Result <> ERROR_JSON_SUCCESS);
  end;

If Result = ERROR_JSON_SUCCESS Then
  begin
  Case AToken.TokenType Of
    jttInteger : AToken.Value := IntToStr(intValue);
    jttFloat : AToken.Value := FloatToStr(fValue);
    end;

  If negative Then
    AToken.Value := '-' + AToken.Value;
  end;
end;

Function _NextToken(Var Buffer:PWideChar; Var AToken:TJSONToken):Cardinal;
Var
  upId : WideString;
  ch : WideChar;
  found : Boolean;
begin
Result := ERROR_JSON_SUCCESS;
Repeat
found := True;
ch := Buffer^;
Inc(Buffer);
ATOken.Value := ch;
Case ch Of
  ':' : AToken.TokenType := jttAttributeDelimiter;
  ',' : AToken.TokenType := jttDelimiter;
  '{' : AToken.TokenType := jttObjectStart;
  '}' : AToken.TokenType := jttObjectEnd;
  '[' : AToken.TokenType := jttArrayStart;
  ']' : AToken.TokenType := jttArrayEnd;
  '#' : begin
    found := False;
    While (Buffer^ <> #0) And (Buffer <> #13) And (Buffer <> #10) Do
      Inc(Buffer);

    While (Buffer = #13) Or (Buffer = #10) Do
      Inc(Buffer);
    end;
  '"', '''' : begin
    AToken.TokenType := jttString;
    AToken.Value := '';
    While (Buffer^ <> #13) And (Buffer^ <> #10) And (Buffer^ <> ch) Do
      begin
      AToken.Value := AToken.Value + Buffer^;
      Inc(Buffer);
      end;

    If Buffer^ = ch Then
      Inc(Buffer)
    Else Result := ERROR_JSON_UNTERMINATED_STRING;
    end;
  #8, ' ' : found := False;
  #10 : begin
    found := False;
    Inc(AToken.Line);
    end;
  #13 : begin
    found := False;
    Inc(AToken.Line);
    If Buffer^ = #10 Then
      Inc(Buffer);
    end;
  '0'..'9', '-', '+' : begin
    AToken.Value := ch;
    Result := _ParseNumber(Buffer, AToken);
    end;
  'a'..'z', 'A'..'Z' : begin
    AToken.TokenType := jttIdentifier;
    AToken.Value := ch;
    While ((Buffer^ >= 'a') And (Buffer^ <= 'z')) Or
          ((Buffer^ >= 'A') And (Buffer^ <= 'Z')) Or
          ((Buffer^ >= '0') And (Buffer^ <= '9')) Or
          (Buffer^ = '_') Do
      begin
      AToken.Value := AToken.Value + Buffer^;
      Inc(Buffer);
      end;

    upId := WideUpperCase(AToken.Value);
    If (upId = 'TRUE') Or (upId = 'FALSE') Then
      AToken.TokenType := jttBoolean
    Else If (upId = 'NULL') Then
      AToken.TokenType := jttNull;
    end;
  #0 : AToken.TokenType := jttInputEnd;
  Else Result := ERROR_JSON_UNRECOGNIZED_CHARACTER;
  end;
Until found;
end;


End.
