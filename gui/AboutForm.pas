Unit AboutForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.Imaging.jpeg,
  Vcl.ComCtrls;

Type
  TAboutBox = class(TForm)
    AboutImage: TImage;
    AboutListView: TListView;
  end;

Implementation

{$R *.DFM}

end.

