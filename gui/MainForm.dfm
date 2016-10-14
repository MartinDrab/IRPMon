object MainFrm: TMainFrm
  Left = 0
  Height = 304
  Top = 0
  Width = 483
  Caption = 'IRPMon'
  ClientHeight = 284
  ClientWidth = 483
  Color = clBtnFace
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Menu = MainMenu1
  OnCreate = FormCreate
  LCLVersion = '1.6.0.4'
  object PageControl1: TPageControl
    Left = 0
    Height = 284
    Top = 0
    Width = 483
    ActivePage = RequestTabSheet
    Align = alClient
    TabIndex = 0
    TabOrder = 0
    object RequestTabSheet: TTabSheet
      Caption = 'Requests'
      ClientHeight = 258
      ClientWidth = 475
      object RequestListView: TListView
        Left = 0
        Height = 258
        Top = 0
        Width = 475
        Align = alClient
        Columns = <>
        OwnerData = True
        ReadOnly = True
        RowSelect = True
        TabOrder = 0
        ViewStyle = vsReport
        OnDblClick = RequestDetailsMenuItemClick
      end
    end
  end
  object MainMenu1: TMainMenu
    left = 168
    top = 176
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
      object SortbyIDMenuItem: TMenuItem
        Caption = 'Sort by ID'
        OnClick = SortbyIDMenuItemClick
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
  object LogSaveDialog: TSaveDialog
    Filter = 'Log files [*.log]|*.log|All files [*.*]|*.*'
    left = 160
    top = 88
  end
end
