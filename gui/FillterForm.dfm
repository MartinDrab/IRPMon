object FilterFrm: TFilterFrm
  Left = 0
  Top = 0
  Caption = 'Filters'
  ClientHeight = 271
  ClientWidth = 473
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 0
    Top = 13
    Width = 24
    Height = 13
    Caption = 'Type'
  end
  object Label2: TLabel
    Left = 87
    Top = 13
    Width = 35
    Height = 13
    Caption = 'Column'
  end
  object Label3: TLabel
    Left = 174
    Top = 13
    Width = 44
    Height = 13
    Caption = 'Operator'
  end
  object Label4: TLabel
    Left = 261
    Top = 13
    Width = 26
    Height = 13
    Caption = 'Value'
  end
  object Label5: TLabel
    Left = 348
    Top = 13
    Width = 30
    Height = 13
    Caption = 'Action'
  end
  object FilterTypeComboBox: TComboBox
    Left = 0
    Top = 32
    Width = 81
    Height = 21
    Style = csDropDownList
    TabOrder = 0
    OnChange = FilterTypeComboBoxChange
  end
  object FilterColumnComboBox: TComboBox
    Left = 87
    Top = 32
    Width = 81
    Height = 21
    Style = csDropDownList
    TabOrder = 1
    OnChange = FilterColumnComboBoxChange
  end
  object FilterOperatorComboBox: TComboBox
    Left = 174
    Top = 32
    Width = 81
    Height = 21
    Style = csDropDownList
    TabOrder = 2
  end
  object FilterValueComboBox: TComboBox
    Left = 261
    Top = 32
    Width = 81
    Height = 21
    Style = csDropDownList
    TabOrder = 3
  end
  object FilterActionComboBox: TComboBox
    Left = 348
    Top = 32
    Width = 81
    Height = 21
    Style = csDropDownList
    ItemIndex = 0
    TabOrder = 4
    Text = 'None'
    OnChange = FilterActionComboBoxChange
    Items.Strings = (
      'None'
      'Include'
      'Exclude'
      'Highlight'
      'Pass')
  end
  object HighlightColorColorBox: TColorBox
    Left = 348
    Top = 59
    Width = 85
    Height = 22
    Enabled = False
    TabOrder = 5
    Visible = False
  end
end
