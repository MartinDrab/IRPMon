object FilterFrm: TFilterFrm
  Left = 0
  Top = 0
  Caption = 'Filters'
  ClientHeight = 331
  ClientWidth = 648
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
    Width = 648
    Height = 145
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
    object Label6: TLabel
      Left = 0
      Top = 59
      Width = 27
      Height = 13
      Caption = 'Name'
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
      Selected = clWhite
      TabOrder = 5
    end
    object FilterActionComboBox: TComboBox
      Left = 468
      Top = 32
      Width = 81
      Height = 21
      Style = csDropDownList
      TabOrder = 6
      OnChange = FilterActionComboBoxChange
      Items.Strings = (
        'Highlight'
        'Include'
        'Exclude'
        'Pass')
    end
    object AddButton: TButton
      Left = 0
      Top = 114
      Width = 57
      Height = 25
      Caption = 'Add'
      TabOrder = 7
      OnClick = AddButtonClick
    end
    object DeleteButton: TButton
      Left = 120
      Top = 114
      Width = 57
      Height = 25
      Caption = 'Delete'
      TabOrder = 8
      OnClick = DeleteButtonClick
    end
    object EnabledCheckBox: TCheckBox
      Left = 346
      Top = 59
      Width = 73
      Height = 17
      Caption = 'Enabled'
      Checked = True
      State = cbChecked
      TabOrder = 9
    end
    object NextFilterComboBox: TComboBox
      Left = 464
      Top = 85
      Width = 85
      Height = 21
      Style = csDropDownList
      TabOrder = 10
      Visible = False
    end
    object UpButton: TButton
      Left = 183
      Top = 114
      Width = 57
      Height = 25
      Caption = 'Up'
      TabOrder = 11
      OnClick = UpDownButtonClick
    end
    object DownButton: TButton
      Left = 246
      Top = 114
      Width = 57
      Height = 25
      Caption = 'Down'
      TabOrder = 12
      OnClick = UpDownButtonClick
    end
    object NameEdit: TEdit
      Left = 0
      Top = 78
      Width = 253
      Height = 21
      TabOrder = 13
    end
    object ApplyButton: TButton
      Left = 57
      Top = 114
      Width = 57
      Height = 25
      Caption = 'Apply'
      TabOrder = 14
      OnClick = AddButtonClick
    end
    object EphemeralCheckBox: TCheckBox
      Left = 346
      Top = 80
      Width = 73
      Height = 17
      Caption = 'Ephemeral'
      TabOrder = 15
    end
  end
  object LowerPanel: TPanel
    Left = 0
    Top = 282
    Width = 648
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
      Top = 6
      Width = 57
      Height = 33
      Caption = 'Ok'
      TabOrder = 1
      OnClick = OkButtonClick
    end
  end
  object FilterListView: TListView
    Left = 0
    Top = 145
    Width = 648
    Height = 137
    Align = alClient
    Checkboxes = True
    Columns = <
      item
        AutoSize = True
        Caption = 'Name'
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
      end
      item
        AutoSize = True
        Caption = 'Next'
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
    OnSelectItem = FilterListViewSelectItem
    OnItemChecked = FilterListViewItemChecked
  end
end
