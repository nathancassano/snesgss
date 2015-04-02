object FormReplace: TFormReplace
  Left = 0
  Top = 0
  BorderStyle = bsToolWindow
  Caption = 'Replace instrument'
  ClientHeight = 141
  ClientWidth = 320
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 16
  object Label1: TLabel
    Left = 208
    Top = 16
    Width = 35
    Height = 16
    Caption = 'From:'
  end
  object SpeedButtonReplace: TSpeedButton
    Left = 242
    Top = 87
    Width = 73
    Height = 22
    Caption = 'Replace'
    OnClick = SpeedButtonReplaceClick
  end
  object SpeedButtonCancel: TSpeedButton
    Left = 242
    Top = 115
    Width = 73
    Height = 22
    Caption = 'Cancel'
    OnClick = SpeedButtonCancelClick
  end
  object Label2: TLabel
    Left = 208
    Top = 46
    Width = 20
    Height = 16
    Caption = 'To:'
  end
  object GroupBoxWhere: TGroupBox
    Left = 0
    Top = 0
    Width = 185
    Height = 137
    Caption = 'Location'
    TabOrder = 0
    object RadioButtonAllSongs: TRadioButton
      Left = 16
      Top = 32
      Width = 145
      Height = 17
      Caption = 'All songs'
      TabOrder = 0
    end
    object RadioButtonCurrentSong: TRadioButton
      Left = 16
      Top = 55
      Width = 145
      Height = 17
      Caption = 'Current song only'
      TabOrder = 1
    end
    object RadioButtonCurrentChannel: TRadioButton
      Left = 16
      Top = 78
      Width = 145
      Height = 17
      Caption = 'Current channel only'
      TabOrder = 2
    end
    object RadioButtonBlock: TRadioButton
      Left = 16
      Top = 101
      Width = 145
      Height = 17
      Caption = 'Block selection only'
      Checked = True
      TabOrder = 3
      TabStop = True
    end
  end
  object EditFrom: TEdit
    Left = 256
    Top = 13
    Width = 40
    Height = 24
    TabOrder = 1
    Text = '1'
    OnKeyPress = EditFromKeyPress
  end
  object UpDownFrom: TUpDown
    Left = 296
    Top = 13
    Width = 19
    Height = 24
    Associate = EditFrom
    Min = 1
    Max = 99
    Position = 1
    TabOrder = 2
  end
  object EditTo: TEdit
    Left = 256
    Top = 43
    Width = 40
    Height = 24
    TabOrder = 3
    Text = '1'
    OnKeyPress = EditFromKeyPress
  end
  object UpDownTo: TUpDown
    Left = 296
    Top = 43
    Width = 19
    Height = 24
    Associate = EditTo
    Min = 1
    Max = 99
    Position = 1
    TabOrder = 4
  end
end
