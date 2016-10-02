object ClassWatchAddFrm: TClassWatchAddFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Add a Class Watch'
  ClientHeight = 340
  ClientWidth = 491
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
    Width = 491
    Height = 313
    Align = alTop
    TabOrder = 0
    ExplicitWidth = 381
    object FilterPositionRadioGroup: TRadioGroup
      Left = 1
      Top = 1
      Width = 489
      Height = 56
      Align = alTop
      Caption = 'Filter position'
      Columns = 2
      ItemIndex = 1
      Items.Strings = (
        'first'
        'last')
      TabOrder = 0
      ExplicitTop = -5
    end
    object GroupBox1: TGroupBox
      Left = 1
      Top = 57
      Width = 489
      Height = 255
      Align = alClient
      Caption = 'Class'
      TabOrder = 1
      ExplicitTop = 105
      ExplicitWidth = 379
      ExplicitHeight = 208
      object ClassListView: TListView
        Left = 2
        Top = 15
        Width = 485
        Height = 238
        Align = alClient
        Columns = <
          item
            Caption = 'Name'
            Width = 100
          end
          item
            Caption = 'Filter type'
            Width = 75
          end
          item
            AutoSize = True
            Caption = 'GUID'
          end>
        OwnerData = True
        ReadOnly = True
        RowSelect = True
        ShowWorkAreas = True
        TabOrder = 0
        ViewStyle = vsReport
        OnCustomDrawItem = ClassListViewCustomDrawItem
        OnData = ClassListViewData
        ExplicitHeight = 194
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
