object SetSymPathFrm: TSetSymPathFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Change symbol search path'
  ClientHeight = 316
  ClientWidth = 411
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  PixelsPerInch = 96
  TextHeight = 13
  object MainPanel: TPanel
    Left = 0
    Top = 0
    Width = 411
    Height = 273
    Align = alTop
    TabOrder = 0
    ExplicitLeft = -8
    object SymListView: TListView
      Left = 1
      Top = 1
      Width = 409
      Height = 271
      Align = alClient
      Columns = <
        item
          AutoSize = True
          Caption = 'Local directory'
        end
        item
          AutoSize = True
          Caption = 'Symbol server'
        end>
      ReadOnly = True
      RowSelect = True
      PopupMenu = SymPopupMenu
      ShowWorkAreas = True
      TabOrder = 0
      ViewStyle = vsReport
      ExplicitLeft = 2
      ExplicitTop = 2
    end
  end
  object StornoButton: TButton
    Left = 344
    Top = 279
    Width = 59
    Height = 34
    Caption = 'Storno'
    TabOrder = 1
    OnClick = StornoButtonClick
  end
  object OkButton: TButton
    Left = 279
    Top = 279
    Width = 59
    Height = 34
    Caption = 'Ok'
    TabOrder = 2
    OnClick = OkButtonClick
  end
  object SymPopupMenu: TPopupMenu
    OnPopup = OnSymPopupMenuClick
    Left = 128
    Top = 104
    object AddMenuItem: TMenuItem
      Caption = 'Add...'
      OnClick = OnSymPopupMenuClick
    end
    object EditMenuItem: TMenuItem
      Caption = 'Edit...'
      OnClick = OnSymPopupMenuClick
    end
    object DeleteMenuItem: TMenuItem
      Caption = 'Delete'
      OnClick = OnSymPopupMenuClick
    end
  end
end
