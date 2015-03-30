//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitSectionList.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormSectionList *FormSectionList;
//---------------------------------------------------------------------------
__fastcall TFormSectionList::TFormSectionList(TComponent* Owner)
	: TForm(Owner)
{
	Selected=-1;
}
//---------------------------------------------------------------------------
void __fastcall TFormSectionList::ListBoxSectionsDblClick(TObject *Sender)
{
	Selected=ListBoxSections->ItemIndex;
	Close();
}
//---------------------------------------------------------------------------
void __fastcall TFormSectionList::FormShow(TObject *Sender)
{
	if(ListBoxSections->Items->Count&&ListBoxSections->ItemIndex<0) ListBoxSections->ItemIndex=0;
	if(ListBoxSections->ItemIndex>=ListBoxSections->Items->Count) ListBoxSections->ItemIndex=ListBoxSections->Items->Count-1;
	
	Selected=ListBoxSections->ItemIndex;
}
//---------------------------------------------------------------------------
void __fastcall TFormSectionList::ListBoxSectionsClick(TObject *Sender)
{
	Selected=ListBoxSections->ItemIndex;
}
//---------------------------------------------------------------------------
void __fastcall TFormSectionList::ListBoxSectionsKeyPress(TObject *Sender,
      char &Key)
{
	if(ListBoxSections->Items->Count) Selected=ListBoxSections->ItemIndex;

	if(Key==VK_RETURN&&Selected>=0)
	{
		Key=0;
		Close();
	}

	if(Key==VK_ESCAPE)
	{
		Key=0;
		Selected=-1;
		Close();
	}
}
//---------------------------------------------------------------------------
