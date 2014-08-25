//---------------------------------------------------------------------------

#ifndef UnitReplaceH
#define UnitReplaceH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFormReplace : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBoxWhere;
	TRadioButton *RadioButtonAllSongs;
	TRadioButton *RadioButtonCurrentSong;
	TRadioButton *RadioButtonCurrentChannel;
	TRadioButton *RadioButtonBlock;
	TLabel *Label1;
	TEdit *EditFrom;
	TUpDown *UpDownFrom;
	TSpeedButton *SpeedButtonReplace;
	TSpeedButton *SpeedButtonCancel;
	TLabel *Label2;
	TEdit *EditTo;
	TUpDown *UpDownTo;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall SpeedButtonCancelClick(TObject *Sender);
	void __fastcall SpeedButtonReplaceClick(TObject *Sender);
	void __fastcall EditFromKeyPress(TObject *Sender, char &Key);
private:	// User declarations
public:		// User declarations
	__fastcall TFormReplace(TComponent* Owner);

	bool Replace;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormReplace *FormReplace;
//---------------------------------------------------------------------------
#endif
