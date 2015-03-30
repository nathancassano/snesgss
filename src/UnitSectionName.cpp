//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitSectionName.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormName *FormName;
//---------------------------------------------------------------------------
__fastcall TFormName::TFormName(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormName::EditNameKeyPress(TObject *Sender, char &Key)
{
	if(Key==VK_RETURN||Key==VK_ESCAPE)
	{
		Key=0;
		Close();
	}
}
//---------------------------------------------------------------------------
