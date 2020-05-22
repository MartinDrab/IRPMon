Unit AboutForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics,
  Controls, Forms, Dialogs, ExtCtrls,
  ComCtrls, StdCtrls;

Type
  TAboutBox = class(TForm)
    AboutImage: TImage;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    VersionEdit: TEdit;
    LicenseEdit: TEdit;
    WebEdit: TEdit;
    AuthorEdit: TEdit;
    Label5: TLabel;
    ContributorsMemo: TMemo;
  end;

Implementation

{$R *.dfm}

end.

