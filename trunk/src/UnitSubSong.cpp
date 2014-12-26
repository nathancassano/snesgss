//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitSubSong.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormSubSong *FormSubSong;
//---------------------------------------------------------------------------
__fastcall TFormSubSong::TFormSubSong(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormSubSong::SpeedButtonImportClick(TObject *Sender)
{
	Import=true;
	Close();	
}
//---------------------------------------------------------------------------
void __fastcall TFormSubSong::ButtonImportClick(TObject *Sender)
{
	Import=true;
	Close();	
}
//---------------------------------------------------------------------------
