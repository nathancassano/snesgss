//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitTranspose.h"
#include "UnitMain.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormTranspose *FormTranspose;
//---------------------------------------------------------------------------
__fastcall TFormTranspose::TFormTranspose(TComponent* Owner)
: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormTranspose::SpeedButtonTransposeClick(TObject *Sender)
{
	Transpose=true;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormTranspose::SpeedButtonCancelClick(TObject *Sender)
{
	Transpose=false;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormTranspose::FormCreate(TObject *Sender)
{
	Transpose=false;
}
//---------------------------------------------------------------------------
void __fastcall TFormTranspose::EditTransposeKeyPress(TObject *Sender, char &Key)
{
	if(!((Key>='0'&&Key<='9')||Key=='-'||Key==VK_BACK||Key==VK_DELETE)) Key=0;
}
//---------------------------------------------------------------------------
void __fastcall TFormTranspose::FormShow(TObject *Sender)
{
	RadioButtonCurrentInstrument->Caption="Current instrument ("+IntToStr(FormMain->InsCur+1)+")";
}
//---------------------------------------------------------------------------

