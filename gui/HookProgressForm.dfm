object HookProgressFrm: THookProgressFrm
  Left = 0
  Top = 0
  BorderIcons = []
  Caption = 'Progress'
  ClientHeight = 244
  ClientWidth = 589
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object LowerPanel: TPanel
    Left = 0
    Top = 195
    Width = 589
    Height = 49
    Align = alBottom
    TabOrder = 0
    object CloseButton: TButton
      Left = 144
      Top = 6
      Width = 65
      Height = 33
      Caption = 'Close'
      Enabled = False
      TabOrder = 0
      OnClick = CloseButtonClick
    end
  end
  object ProgressListView: TListView
    Left = 0
    Top = 0
    Width = 589
    Height = 195
    Align = alClient
    Columns = <
      item
        Caption = 'Operation'
        Width = 75
      end
      item
        Caption = 'Type'
        Width = 55
      end
      item
        Caption = 'Name'
        Width = 150
      end
      item
        Caption = 'Status'
      end
      item
        Caption = 'Description'
        Width = 300
      end>
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ReadOnly = True
    RowSelect = True
    ParentFont = False
    TabOrder = 1
    ViewStyle = vsReport
    OnAdvancedCustomDrawItem = ProgressListViewAdvancedCustomDrawItem
  end
end
