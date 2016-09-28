object ClassWatchAddFrm: TClassWatchAddFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Add a Class Watch'
  ClientHeight = 340
  ClientWidth = 381
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
  object MainPanel: TPanel
    Left = 0
    Top = 0
    Width = 381
    Height = 313
    Align = alTop
    TabOrder = 0
    object FilterPositionRadioGroup: TRadioGroup
      Left = 1
      Top = 49
      Width = 379
      Height = 56
      Align = alTop
      Caption = 'Filter position'
      Columns = 2
      ItemIndex = 1
      Items.Strings = (
        'first'
        'last')
      TabOrder = 0
    end
    object FilterTypeRadioGroup: TRadioGroup
      Left = 1
      Top = 1
      Width = 379
      Height = 48
      Align = alTop
      Caption = 'Filter type'
      Columns = 2
      ItemIndex = 1
      Items.Strings = (
        'lower'
        'upper')
      TabOrder = 1
    end
    object GroupBox1: TGroupBox
      Left = 1
      Top = 105
      Width = 379
      Height = 208
      Align = alTop
      Caption = 'Class'
      TabOrder = 2
      object ClassListView: TListView
        Left = 2
        Top = 15
        Width = 375
        Height = 191
        Align = alClient
        Columns = <
          item
            Caption = 'Name'
            Width = 100
          end
          item
            AutoSize = True
            Caption = 'GUID'
          end>
        OwnerData = True
        ReadOnly = True
        RowSelect = True
        TabOrder = 0
        ViewStyle = vsReport
        OnCustomDrawItem = ClassListViewCustomDrawItem
        OnData = ClassListViewData
      end
    end
  end
  object CancelButton: TButton
    Left = 256
    Top = 319
    Width = 49
    Height = 25
    Caption = 'Cancel'
    TabOrder = 1
    OnClick = CancelButtonClick
  end
  object OkButton: TButton
    Left = 201
    Top = 319
    Width = 49
    Height = 25
    Caption = 'Ok'
    TabOrder = 2
    OnClick = OkButtonClick
  end
end
