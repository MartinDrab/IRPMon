object MainFrm: TMainFrm
  Left = 0
  Top = 0
  Caption = 'IRPMon'
  ClientHeight = 304
  ClientWidth = 483
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Menu = MainMenu1
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object PageControl1: TPageControl
    Left = 0
    Top = 0
    Width = 483
    Height = 304
    ActivePage = RequestTabSheet
    Align = alClient
    TabOrder = 0
    object RequestTabSheet: TTabSheet
      Caption = 'Requests'
      ExplicitLeft = 8
      ExplicitTop = 28
      object RequestListView: TListView
        Left = 0
        Top = 0
        Width = 475
        Height = 276
        Align = alClient
        Columns = <>
        OwnerData = True
        ReadOnly = True
        RowSelect = True
        TabOrder = 0
        ViewStyle = vsReport
        ExplicitLeft = 96
        ExplicitTop = 104
        ExplicitWidth = 105
        ExplicitHeight = 73
      end
    end
    object Hooks: TTabSheet
      Caption = 'Hooks'
      ImageIndex = 1
    end
  end
  object MainMenu1: TMainMenu
    Left = 232
    Top = 96
    object ActionMenuItem: TMenuItem
      Caption = 'Action'
      object HookDriverMenuItem: TMenuItem
        Caption = 'Hook driver...'
        OnClick = HookDriverMenuItemClick
      end
      object HookDriverNDMenuItem: TMenuItem
        Caption = 'Hook driver (new devices)...'
        OnClick = HookDriverMenuItemClick
      end
      object UnhookDriverMenuItem: TMenuItem
        Caption = 'Unhook driver'
      end
      object N3: TMenuItem
        Caption = '-'
      end
      object HookDeviceNameMenuItem: TMenuItem
        Caption = 'Hook device (name)...'
        OnClick = HookDeviceNameMenuItemClick
      end
      object HookDeviceAddressMenuItem: TMenuItem
        Caption = 'Hook device (address)...'
        OnClick = HookDeviceNameMenuItemClick
      end
      object UnhookDeviceMenuItem: TMenuItem
        Caption = 'Unhook device...'
      end
      object N1: TMenuItem
        Caption = '-'
      end
      object TreeMenuItem: TMenuItem
        Caption = 'Tree...'
      end
      object N2: TMenuItem
        Caption = '-'
      end
      object SaveMenuItem: TMenuItem
        Caption = 'Save...'
      end
      object N5: TMenuItem
        Caption = '-'
      end
      object ExitMenuItem: TMenuItem
        Caption = 'Exit'
        OnClick = ExitMenuItemClick
      end
    end
    object MonitoringMenuItem: TMenuItem
      Caption = 'Monitoring'
      object StartMenuItem: TMenuItem
        Caption = 'Start'
        OnClick = MonitoringMenuItemClick
      end
      object StopMenuItem: TMenuItem
        Caption = 'Stop'
        OnClick = MonitoringMenuItemClick
      end
      object N4: TMenuItem
        Caption = '-'
      end
      object CaptureEventsMenuItem: TMenuItem
        Caption = 'Capture events'
        OnClick = CaptureEventsMenuItemClick
      end
      object MonitorMenuItem: TMenuItem
        Caption = 'Monitor'
        object ViewIRPMenuItem: TMenuItem
          Caption = 'IRP'
          Checked = True
          OnClick = ViewMenuItemClick
        end
        object ViewIRPCompleteMenuItem: TMenuItem
          Caption = 'IRP completion'
          Checked = True
          OnClick = ViewMenuItemClick
        end
        object ViewFastIoMenuItem: TMenuItem
          Caption = 'Fast I/O'
          Checked = True
          OnClick = ViewMenuItemClick
        end
        object ViewAddDeviceMenuItem: TMenuItem
          Caption = 'AddDevice'
          Checked = True
          OnClick = ViewMenuItemClick
        end
        object ViewUnloadMenuItem: TMenuItem
          Caption = 'Unload'
          Checked = True
          OnClick = ViewMenuItemClick
        end
        object ViewStartIoMenuItem: TMenuItem
          Caption = 'StartIo'
          Checked = True
          OnClick = ViewMenuItemClick
        end
      end
      object N6: TMenuItem
        Caption = '-'
      end
      object ClearMenuItem: TMenuItem
        Caption = 'Clear'
        OnClick = ClearMenuItemClick
      end
    end
    object ColumnsMenuItem: TMenuItem
      Caption = 'Columns'
    end
    object HelpMenuItem: TMenuItem
      Caption = 'Help'
      object AboutMenuItem: TMenuItem
        Caption = 'About IRPMon...'
      end
    end
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 200
    Top = 96
  end
end
