object AddEditSymServerFrm: TAddEditSymServerFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Symbol server'
  ClientHeight = 123
  ClientWidth = 296
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
  object MainPanel: TPanel
    Left = 0
    Top = 0
    Width = 296
    Height = 81
    Align = alTop
    TabOrder = 0
    object Label1: TLabel
      Left = 0
      Top = 11
      Width = 24
      Height = 13
      Caption = 'Local'
    end
    object Label2: TLabel
      Left = 0
      Top = 38
      Width = 32
      Height = 13
      Caption = 'Server'
    end
    object LocalEdit: TEdit
      Left = 48
      Top = 8
      Width = 177
      Height = 21
      TabOrder = 0
    end
    object BrowseButton: TButton
      Left = 231
      Top = 8
      Width = 57
      Height = 21
      Caption = 'Browse...'
      TabOrder = 1
      OnClick = BrowseButtonClick
    end
    object ServerEdit: TEdit
      Left = 48
      Top = 35
      Width = 240
      Height = 21
      TabOrder = 2
    end
  end
  object StornoButton: TButton
    Left = 223
    Top = 87
    Width = 65
    Height = 33
    Caption = 'Storno'
    TabOrder = 1
    OnClick = StornoButtonClick
  end
  object OkButton: TButton
    Left = 152
    Top = 87
    Width = 65
    Height = 33
    Caption = 'Ok'
    TabOrder = 2
    OnClick = OkButtonClick
  end
  object LocalOpenDialog: TFileOpenDialog
    ClientGuid = '{9dead5a8-4817-4f81-a6e5-794b102f3ab1}'
    FavoriteLinks = <>
    FileTypes = <>
    OkButtonLabel = 'Select'
    Options = [fdoPickFolders, fdoPathMustExist]
    Title = 'Select a directory for caching symbols'
    Left = 72
    Top = 96
  end
end
