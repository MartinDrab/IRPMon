object TreeFrm: TTreeFrm
  Left = 175
  Top = 66
  BorderIcons = [biSystemMenu]
  Caption = 'Hook Drivers and Devices'
  ClientHeight = 375
  ClientWidth = 585
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = True
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object DeviceTreeView: TTreeView
    Left = 0
    Top = 0
    Width = 249
    Height = 336
    Align = alClient
    Indent = 19
    PopupMenu = DeviceTreeViewPopupMenu
    ReadOnly = True
    TabOrder = 0
    OnAdvancedCustomDrawItem = DeviceTreeViewAdvancedCustomDrawItem
    OnChange = DeviceTreeViewChange
  end
  object IRPGroupBox: TGroupBox
    Left = 249
    Top = 0
    Width = 160
    Height = 336
    Align = alRight
    Caption = 'IRP'
    TabOrder = 1
    object IRPCheckListBox: TCheckListBox
      Left = 2
      Top = 15
      Width = 156
      Height = 319
      OnClickCheck = CheckListBoxClickCheck
      Align = alClient
      ItemHeight = 13
      TabOrder = 0
    end
  end
  object FastIOGroupBox: TGroupBox
    Left = 409
    Top = 0
    Width = 176
    Height = 336
    Align = alRight
    Caption = 'Fast I/O'
    TabOrder = 2
    object FastIOCheckListBox: TCheckListBox
      Left = 2
      Top = 15
      Width = 172
      Height = 319
      OnClickCheck = CheckListBoxClickCheck
      Align = alClient
      ItemHeight = 13
      TabOrder = 0
    end
  end
  object LowerPanel: TPanel
    Left = 0
    Top = 336
    Width = 585
    Height = 39
    Align = alBottom
    TabOrder = 3
    object StornoButton: TButton
      Left = 520
      Top = 6
      Width = 49
      Height = 25
      Caption = 'Storno'
      TabOrder = 0
      OnClick = StornoButtonClick
    end
    object OkButton: TButton
      Left = 465
      Top = 6
      Width = 49
      Height = 25
      Caption = 'Ok'
      TabOrder = 1
      OnClick = OkButtonClick
    end
  end
  object DeviceTreeViewPopupMenu: TPopupMenu
    OnPopup = DeviceTreeViewPopupMenuPopup
    Left = 72
    Top = 144
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
