object ConnectorSelectionFrm: TConnectorSelectionFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Connect to the driver'
  ClientHeight = 291
  ClientWidth = 320
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
  object PageControl1: TPageControl
    Left = 0
    Top = 0
    Width = 320
    Height = 255
    ActivePage = DeviceTabSheet
    Align = alTop
    TabOrder = 0
    object NoneTabSheet: TTabSheet
      Caption = 'None'
    end
    object DeviceTabSheet: TTabSheet
      Caption = 'Device'
      ImageIndex = 1
      object Label1: TLabel
        Left = 8
        Top = 8
        Width = 61
        Height = 13
        Caption = 'Device name'
      end
      object DeviceNameEdit: TEdit
        Left = 75
        Top = 8
        Width = 153
        Height = 21
        TabOrder = 0
      end
    end
    object NetworkTabSheet: TTabSheet
      Caption = 'Network'
      ImageIndex = 2
      object DomainLabel: TLabel
        Left = 3
        Top = 8
        Width = 49
        Height = 13
        Caption = 'Domain/IP'
      end
      object PortLabel: TLabel
        Left = 3
        Top = 35
        Width = 20
        Height = 13
        Caption = 'Port'
      end
      object VSockVersionLabel: TLabel
        Left = 0
        Top = 85
        Width = 66
        Height = 13
        Caption = 'vSock version'
      end
      object VSockAddressLabel: TLabel
        Left = -4
        Top = 112
        Width = 69
        Height = 13
        Caption = 'vSock address'
      end
      object HyperVVMLabel: TLabel
        Left = 3
        Top = 139
        Width = 56
        Height = 13
        Caption = 'Hyper-V VM'
      end
      object HyperVAppLabel: TLabel
        Left = 3
        Top = 166
        Width = 61
        Height = 13
        Caption = 'Hyper-V App'
      end
      object NetworkDomainEdit: TEdit
        Left = 80
        Top = 8
        Width = 129
        Height = 21
        TabOrder = 0
      end
      object NetworkPortEdit: TEdit
        Left = 80
        Top = 35
        Width = 129
        Height = 21
        TabOrder = 1
      end
      object VSockVersionEdit: TEdit
        Left = 80
        Top = 85
        Width = 129
        Height = 21
        ReadOnly = True
        TabOrder = 2
      end
      object VSockAddressEdit: TEdit
        Left = 80
        Top = 112
        Width = 129
        Height = 21
        ReadOnly = True
        TabOrder = 3
      end
      object NetworkTypeComboBox: TComboBox
        Left = 80
        Top = 58
        Width = 129
        Height = 21
        Style = csDropDownList
        ItemIndex = 0
        TabOrder = 4
        Text = 'Classic (Ethernet)'
        OnClick = NetworkTypeComboBoxClick
        Items.Strings = (
          'Classic (Ethernet)'
          'VMWare vSockets'
          'Microsoft Hyper-V')
      end
      object HyperVVMIdEdit: TEdit
        Left = 80
        Top = 139
        Width = 129
        Height = 21
        Enabled = False
        ReadOnly = True
        TabOrder = 5
        Text = '<not implemented>'
      end
      object HyperVAppIdEdit: TEdit
        Left = 80
        Top = 166
        Width = 129
        Height = 21
        Enabled = False
        ReadOnly = True
        TabOrder = 6
        Text = '<not implemented>'
      end
    end
  end
  object OkButton: TButton
    Left = 200
    Top = 261
    Width = 57
    Height = 30
    Caption = 'Ok'
    TabOrder = 1
    OnClick = OkButtonClick
  end
  object StornoButton: TButton
    Left = 263
    Top = 261
    Width = 57
    Height = 30
    Caption = 'Storno'
    TabOrder = 2
    OnClick = StornoButtonClick
  end
end
