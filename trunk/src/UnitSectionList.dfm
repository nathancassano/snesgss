object FormSectionList: TFormSectionList
  Left = 0
  Top = 0
  BorderStyle = bsSizeToolWin
  Caption = 'Song sections list'
  ClientHeight = 301
  ClientWidth = 284
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object ListBoxSections: TListBox
    Left = 0
    Top = 0
    Width = 284
    Height = 301
    Align = alClient
    ItemHeight = 13
    TabOrder = 0
    OnClick = ListBoxSectionsClick
    OnDblClick = ListBoxSectionsDblClick
    OnKeyPress = ListBoxSectionsKeyPress
    ExplicitWidth = 318
  end
end
