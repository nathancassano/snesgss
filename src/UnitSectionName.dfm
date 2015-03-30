object FormName: TFormName
  Left = 0
  Top = 0
  BorderStyle = bsToolWindow
  Caption = 'Change song section name'
  ClientHeight = 25
  ClientWidth = 294
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  PixelsPerInch = 96
  TextHeight = 16
  object EditName: TEdit
    Left = 0
    Top = 0
    Width = 297
    Height = 24
    TabOrder = 0
    OnKeyPress = EditNameKeyPress
  end
end
