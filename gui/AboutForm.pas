Unit AboutForm;

{$IFDEF FPC}
  {$MODE Delphi}
{$ENDIF}

Interface

Uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics,
  Controls, Forms, Dialogs, ExtCtrls,
  ComCtrls, Vcl.Imaging.jpeg;

Type
  TAboutBox = class(TForm)
    AboutImage: TImage;
    AboutListView: TListView;
  end;

Implementation

{$R *.dfm}

end.

