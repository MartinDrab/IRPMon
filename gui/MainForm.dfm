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
      object RequestListView: TListView
        Left = 0
        Top = 0
        Width = 475
        Height = 276
        Align = alClient
        Columns = <>
        DoubleBuffered = True
        OwnerData = True
        ReadOnly = True
        RowSelect = True
        ParentDoubleBuffered = False
        TabOrder = 0
        ViewStyle = vsReport
        OnDblClick = RequestDetailsMenuItemClick
      end
    end
    object Hooks: TTabSheet
      Caption = 'Hooks'
      ImageIndex = 1
    end
  end
  object MainMenu1: TMainMenu
    Left = 168
    Top = 176
    object ActionMenuItem: TMenuItem
      Caption = 'Action'
      object SelectDriversDevicesMenuItem: TMenuItem
        Caption = 'Select drivers / devices...'
        OnClick = SelectDriversDevicesMenuItemClick
      end
      object WatchClassMenuItem: TMenuItem
        Caption = 'Watch class...'
        OnClick = WatchClassMenuItemClick
      end
      object WatchedClassesMenuItem: TMenuItem
        Caption = 'Watched classes'
        Enabled = False
        Visible = False
      end
      object WatchDriverNameMenuItem: TMenuItem
        Caption = 'Watch driver...'
        OnClick = WatchDriverNameMenuItemClick
      end
      object WatchedDriversMenuItem: TMenuItem
        Caption = 'Watched drivers'
        Enabled = False
        Visible = False
      end
      object N1: TMenuItem
        Caption = '-'
      end
      object SaveMenuItem: TMenuItem
        Caption = 'Save...'
        OnClick = SaveMenuItemClick
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
      object CaptureEventsMenuItem: TMenuItem
        Caption = 'Capture events'
        OnClick = CaptureEventsMenuItemClick
      end
      object RefreshNameCacheMenuItem: TMenuItem
        Caption = 'Refresh name cache'
        OnClick = RefreshNameCacheMenuItemClick
      end
      object N6: TMenuItem
        Caption = '-'
      end
      object ClearMenuItem: TMenuItem
        Caption = 'Clear'
        OnClick = ClearMenuItemClick
      end
    end
    object RequestMenuItem: TMenuItem
      Caption = 'Request'
      object RequestDetailsMenuItem: TMenuItem
        Caption = 'Details...'
        OnClick = RequestDetailsMenuItemClick
      end
    end
    object ColumnsMenuItem: TMenuItem
      Caption = 'Columns'
    end
    object HelpMenuItem: TMenuItem
      Caption = 'Help'
      object AboutMenuItem: TMenuItem
        Caption = 'About IRPMon...'
        OnClick = AboutMenuItemClick
      end
    end
  end
  object IrpMonAppEvents: TApplicationEvents
    OnException = IrpMonAppEventsException
    OnMessage = IrpMonAppEventsMessage
    Left = 164
    Top = 128
  end
  object LogSaveDialog: TSaveDialog
    Filter = 'Log files [*.log]|*.log|All files [*.*]|*.*'
    Left = 160
    Top = 88
  end
end
