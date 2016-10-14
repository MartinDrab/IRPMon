object TreeFrm: TTreeFrm
  Left = 175
  Height = 375
  Top = 66
  Width = 585
  BorderIcons = [biSystemMenu]
  Caption = 'Hook Drivers and Devices'
  ClientHeight = 375
  ClientWidth = 585
  Color = clBtnFace
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  OnClose = FormClose
  OnCreate = FormCreate
  Position = poScreenCenter
  LCLVersion = '1.6.0.4'
  object DeviceTreeView: TTreeView
    Left = 0
    Height = 336
    Top = 0
    Width = 249
    Align = alClient
    DefaultItemHeight = 16
    Indent = 19
    PopupMenu = DeviceTreeViewPopupMenu
    ReadOnly = True
    TabOrder = 0
    OnAdvancedCustomDrawItem = DeviceTreeViewAdvancedCustomDrawItem
    OnChange = DeviceTreeViewChange
    Options = [tvoAutoItemHeight, tvoHideSelection, tvoKeepCollapsedNodes, tvoReadOnly, tvoShowButtons, tvoShowLines, tvoShowRoot, tvoToolTips, tvoThemedDraw]
  end
  object IRPGroupBox: TGroupBox
    Left = 249
    Height = 336
    Top = 0
    Width = 160
    Align = alRight
    Caption = 'IRP'
    ClientHeight = 318
    ClientWidth = 156
    TabOrder = 1
    object IRPCheckListBox: TCheckListBox
      Left = 0
      Height = 318
      Top = 0
      Width = 156
      Align = alClient
      ItemHeight = 0
      OnClickCheck = CheckListBoxClickCheck
      TabOrder = 0
    end
  end
  object FastIOGroupBox: TGroupBox
    Left = 409
    Height = 336
    Top = 0
    Width = 176
    Align = alRight
    Caption = 'Fast I/O'
    ClientHeight = 318
    ClientWidth = 172
    TabOrder = 2
    object FastIOCheckListBox: TCheckListBox
      Left = 0
      Height = 318
      Top = 0
      Width = 172
      Align = alClient
      ItemHeight = 0
      OnClickCheck = CheckListBoxClickCheck
      TabOrder = 0
    end
  end
  object LowerPanel: TPanel
    Left = 0
    Height = 39
    Top = 336
    Width = 585
    Align = alBottom
    ClientHeight = 39
    ClientWidth = 585
    TabOrder = 3
    object StornoButton: TButton
      Left = 520
      Height = 25
      Top = 6
      Width = 49
      Caption = 'Storno'
      OnClick = StornoButtonClick
      TabOrder = 0
    end
    object OkButton: TButton
      Left = 465
      Height = 25
      Top = 6
      Width = 49
      Caption = 'Ok'
      OnClick = OkButtonClick
      TabOrder = 1
    end
  end
  object DeviceTreeViewPopupMenu: TPopupMenu
    OnPopup = DeviceTreeViewPopupMenuPopup
    left = 72
    top = 144
    object HookedMenuItem: TMenuItem
      Caption = 'Hooked'
      Default = True
      OnClick = TreePopupMenuClick
    end
    object N1: TMenuItem
      Caption = '-'
    end
    object NewDevicesMenuItem: TMenuItem
      Caption = 'New devices'
      OnClick = TreePopupMenuClick
    end
    object IRPMenuItem: TMenuItem
      Caption = 'IRP'
      OnClick = TreePopupMenuClick
    end
    object IRPCompleteMenuItem: TMenuItem
      Caption = 'IRP complete'
      OnClick = TreePopupMenuClick
    end
    object FastIOMenuItem: TMenuItem
      Caption = 'Fast IO'
      OnClick = TreePopupMenuClick
    end
    object StartIoMenuItem: TMenuItem
      Caption = 'StartIo'
      OnClick = TreePopupMenuClick
    end
    object AddDeviceMenuItem: TMenuItem
      Caption = 'AddDevice'
      OnClick = TreePopupMenuClick
    end
    object DriverUnloadMenuItem: TMenuItem
      Caption = 'DriverUnload'
      OnClick = TreePopupMenuClick
    end
  end
end
