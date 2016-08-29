object RequestDetailsFrm: TRequestDetailsFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Request Details'
  ClientHeight = 322
  ClientWidth = 279
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object LowerPanel: TPanel
    Left = 0
    Top = 281
    Width = 279
    Height = 41
    Align = alBottom
    TabOrder = 0
    object OkButton: TButton
      Left = 96
      Top = 8
      Width = 65
      Height = 25
      Caption = 'Ok'
      TabOrder = 0
      OnClick = OkButtonClick
    end
  end
  object NameValueListView: TListView
    Left = 0
    Top = 0
    Width = 279
    Height = 281
    Align = alClient
    Columns = <
      item
        Caption = 'Name'
        Width = 125
      end
      item
        AutoSize = True
        Caption = 'Value'
      end>
    DoubleBuffered = True
    ReadOnly = True
    RowSelect = True
    ParentDoubleBuffered = False
    ShowColumnHeaders = False
    ShowWorkAreas = True
    TabOrder = 1
    ViewStyle = vsReport
    ExplicitLeft = 48
    ExplicitWidth = 231
  end
end
