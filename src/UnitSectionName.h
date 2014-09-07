//---------------------------------------------------------------------------

#ifndef UnitSectionNameH
#define UnitSectionNameH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TFormName : public TForm
{
__published:	// IDE-managed Components
	TEdit *EditName;
	void __fastcall EditNameKeyPress(TObject *Sender, char &Key);
private:	// User declarations
public:		// User declarations
	__fastcall TFormName(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormName *FormName;
//---------------------------------------------------------------------------
#endif
