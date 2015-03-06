object FormTranspose: TFormTranspose
  Left = 0
  Top = 0
  BorderStyle = bsToolWindow
  Caption = 'Transpose'
  ClientHeight = 139
  ClientWidth = 507
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
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 16
  object SpeedButtonOK: TSpeedButton
    Left = 432
    Top = 87
    Width = 73
    Height = 22
    Caption = 'OK'
    OnClick = SpeedButtonOKClick
  end
  object SpeedButtonCancel: TSpeedButton
    Left = 432
    Top = 115
    Width = 73
    Height = 22
    Caption = 'Cancel'
    OnClick = SpeedButtonCancelClick
  end
  object LabelHint: TLabel
    Left = 240
    Top = 100
    Width = 65
    Height = 16
    Caption = 'Semitones:'
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
  object GroupBox1: TGroupBox
    Left = 191
    Top = 0
    Width = 314
    Height = 72
    Caption = 'Instrument'
    TabOrder = 1
    object RadioButtonAllInstruments: TRadioButton
      Left = 16
      Top = 32
      Width = 137
      Height = 17
      Caption = 'All instruments'
      Checked = True
      TabOrder = 0
      TabStop = True
    end
    object RadioButtonCurrentInstrument: TRadioButton
      Left = 144
      Top = 32
      Width = 161
      Height = 17
      Caption = 'Current instrument'
      TabOrder = 1
    end
  end
  object EditValue: TEdit
    Left = 320
    Top = 97
    Width = 40
    Height = 24
    TabOrder = 2
    Text = '0'
    OnKeyPress = EditValueKeyPress
  end
  object UpDownValue: TUpDown
    Left = 360
    Top = 97
    Width = 19
    Height = 24
    Associate = EditValue
    Min = -96
    Max = 96
    TabOrder = 3
  end
end
