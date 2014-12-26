object FormSubSong: TFormSubSong
  Left = 0
  Top = 0
  BorderStyle = bsToolWindow
  Caption = 'FamiTracker text import'
  ClientHeight = 50
  ClientWidth = 258
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  PixelsPerInch = 120
  TextHeight = 16
  object Label1: TLabel
    Left = 8
    Top = 16
    Width = 106
    Height = 16
    Caption = 'Sub song number:'
  end
  object EditSubSong: TEdit
    Left = 120
    Top = 13
    Width = 41
    Height = 24
    ReadOnly = True
    TabOrder = 1
    Text = '0'
  end
  object UpDownSubSong: TUpDown
    Left = 161
    Top = 13
    Width = 19
    Height = 24
    Associate = EditSubSong
    Min = 1
    Max = 255
    Position = 1
    TabOrder = 2
  end
  object ButtonImport: TButton
    Left = 191
    Top = 12
    Width = 58
    Height = 25
    Caption = 'Import'
    TabOrder = 0
    OnClick = ButtonImportClick
  end
end
