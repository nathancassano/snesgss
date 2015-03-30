//---------------------------------------------------------------------------

#ifndef UnitSectionListH
#define UnitSectionListH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TFormSectionList : public TForm
{
__published:	// IDE-managed Components
	TListBox *ListBoxSections;
	void __fastcall ListBoxSectionsDblClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall ListBoxSectionsClick(TObject *Sender);
	void __fastcall ListBoxSectionsKeyPress(TObject *Sender, char &Key);
private:	// User declarations
public:		// User declarations
	__fastcall TFormSectionList(TComponent* Owner);

	int Selected;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormSectionList *FormSectionList;
//---------------------------------------------------------------------------
#endif
