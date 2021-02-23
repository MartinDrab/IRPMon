object ColumnFrm: TColumnFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Manage columns'
  ClientHeight = 255
  ClientWidth = 345
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
    Top = 216
    Width = 345
    Height = 39
    Align = alBottom
    TabOrder = 0
    object StornoButton: TButton
      Left = 288
      Top = 5
      Width = 57
      Height = 33
      Caption = 'Storno'
      TabOrder = 0
      OnClick = StornoButtonClick
    end
    object OkButton: TButton
      Left = 225
      Top = 6
      Width = 57
      Height = 33
      Caption = 'Ok'
      TabOrder = 1
      OnClick = OkButtonClick
    end
  end
  object MainPanel: TPanel
    Left = 0
    Top = 0
    Width = 345
    Height = 216
    Align = alClient
    TabOrder = 1
    object ColumnListView: TListView
      Left = 1
      Top = 1
      Width = 343
      Height = 214
      Align = alClient
      Checkboxes = True
      Columns = <>
      ReadOnly = True
      ShowWorkAreas = True
      TabOrder = 0
      ViewStyle = vsList
    end
  end
end
