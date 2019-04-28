object FilterFrm: TFilterFrm
  Left = 0
  Top = 0
  Caption = 'Filters'
  ClientHeight = 331
  ClientWidth = 549
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object UpperPanel: TPanel
    Left = 0
    Top = 0
    Width = 549
    Height = 111
    Align = alTop
    TabOrder = 0
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
      Left = 259
      Top = 13
      Width = 44
      Height = 13
      Caption = 'Operator'
    end
    object Label5: TLabel
      Left = 468
      Top = 13
      Width = 30
      Height = 13
      Caption = 'Action'
    end
    object Label4: TLabel
      Left = 346
      Top = 8
      Width = 26
      Height = 13
      Caption = 'Value'
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
      Width = 166
      Height = 21
      Style = csDropDownList
      TabOrder = 1
      OnChange = FilterColumnComboBoxChange
    end
    object FilterOperatorComboBox: TComboBox
      Left = 259
      Top = 32
      Width = 81
      Height = 21
      Style = csDropDownList
      TabOrder = 2
    end
    object NegateCheckBox: TCheckBox
      Left = 259
      Top = 59
      Width = 73
      Height = 17
      Caption = 'Negate'
      TabOrder = 3
    end
    object FilterValueComboBox: TComboBox
      Left = 346
      Top = 32
      Width = 116
      Height = 21
      Style = csDropDownList
      TabOrder = 4
    end
    object HighlightColorColorBox: TColorBox
      Left = 464
      Top = 57
      Width = 85
      Height = 22
      Enabled = False
      TabOrder = 5
      Visible = False
    end
    object FilterActionComboBox: TComboBox
      Left = 468
      Top = 32
      Width = 81
      Height = 21
      Style = csDropDownList
      ItemIndex = 0
      TabOrder = 6
      Text = 'None'
      OnChange = FilterActionComboBoxChange
      Items.Strings = (
        'None'
        'Include'
        'Exclude'
        'Highlight'
        'Pass')
    end
    object AddButton: TButton
      Left = 0
      Top = 72
      Width = 57
      Height = 25
      Caption = 'Add'
      TabOrder = 7
      OnClick = AddButtonClick
    end
    object DeleteButton: TButton
      Left = 63
      Top = 72
      Width = 57
      Height = 25
      Caption = 'Delete'
      TabOrder = 8
      OnClick = DeleteButtonClick
    end
    object EnabledCheckBox: TCheckBox
      Left = 139
      Top = 59
      Width = 73
      Height = 17
      Caption = 'Enabled'
      Checked = True
      State = cbChecked
      TabOrder = 9
    end
  end
  object LowerPanel: TPanel
    Left = 0
    Top = 282
    Width = 549
    Height = 49
    Align = alBottom
    TabOrder = 1
    object CloseButton: TButton
      Left = 480
      Top = 8
      Width = 57
      Height = 33
      Caption = 'Close'
      TabOrder = 0
      OnClick = CloseButtonClick
    end
    object OkButton: TButton
      Left = 417
      Top = 8
      Width = 57
      Height = 33
      Caption = 'Ok'
      TabOrder = 1
      OnClick = OkButtonClick
    end
  end
  object FilterListView: TListView
    Left = 0
    Top = 111
    Width = 549
    Height = 171
    Align = alClient
    Checkboxes = True
    Columns = <
      item
        Caption = '#'
      end
      item
        Caption = 'Type'
        Width = 75
      end
      item
        AutoSize = True
        Caption = 'Column'
      end
      item
        Caption = 'Operator'
        Width = 75
      end
      item
        Caption = 'Negate'
      end
      item
        AutoSize = True
        Caption = 'Value'
      end
      item
        Caption = 'Action'
        Width = 75
      end>
    DoubleBuffered = True
    HideSelection = False
    ReadOnly = True
    RowSelect = True
    ParentDoubleBuffered = False
    ShowWorkAreas = True
    TabOrder = 2
    ViewStyle = vsReport
    OnAdvancedCustomDrawItem = FilterListViewAdvancedCustomDrawItem
    OnDblClick = FilterListViewDblClick
    OnDeletion = FilterListViewDeletion
    OnItemChecked = FilterListViewItemChecked
    ExplicitTop = 113
  end
end
