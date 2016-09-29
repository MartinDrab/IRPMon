object DriverNameWatchAddFrm: TDriverNameWatchAddFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Watch for a driver'
  ClientHeight = 203
  ClientWidth = 311
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 311
    Height = 161
    Align = alTop
    TabOrder = 0
    object Label1: TLabel
      Left = 16
      Top = 11
      Width = 58
      Height = 13
      Caption = 'Driver name'
    end
    object Label2: TLabel
      Left = 16
      Top = 40
      Width = 77
      Height = 13
      Caption = 'Monitor settings'
    end
    object DriverNameEdit: TEdit
      Left = 99
      Top = 8
      Width = 198
      Height = 21
      TabOrder = 0
      Text = '\Driver\'
    end
    object MonitorSettingsCheckListBox: TCheckListBox
      Left = 99
      Top = 40
      Width = 198
      Height = 105
      ItemHeight = 13
      Items.Strings = (
        'New devices'
        'IRP'
        'IRP completion'
        'Fast I/O'
        'Start IO'
        'AddDevice'
        'Unload')
      TabOrder = 1
    end
  end
  object CancelButton: TButton
    Left = 238
    Top = 167
    Width = 65
    Height = 33
    Caption = 'Cancel'
    TabOrder = 1
    OnClick = CancelButtonClick
  end
  object OkButton: TButton
    Left = 167
    Top = 167
    Width = 65
    Height = 33
    Caption = 'Ok'
    TabOrder = 2
    OnClick = OkButtonClick
  end
end
