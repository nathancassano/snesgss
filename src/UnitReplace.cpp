//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitReplace.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormReplace *FormReplace;
//---------------------------------------------------------------------------
__fastcall TFormReplace::TFormReplace(TComponent* Owner)
: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormReplace::FormCreate(TObject *Sender)
{
	Replace=false;
}
//---------------------------------------------------------------------------
void __fastcall TFormReplace::SpeedButtonCancelClick(TObject *Sender)
{
	Replace=false;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormReplace::SpeedButtonReplaceClick(TObject *Sender)
{
	Replace=true;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormReplace::EditFromKeyPress(TObject *Sender, char &Key)
{
	if(!((Key>='0'&&Key<='9')||Key=='-'||Key==VK_BACK||Key==VK_DELETE)) Key=0;
}
//---------------------------------------------------------------------------
