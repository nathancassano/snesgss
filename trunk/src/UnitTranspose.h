//---------------------------------------------------------------------------

#ifndef UnitTransposeH
#define UnitTransposeH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFormTranspose : public TForm
{
__published:	// IDE-managed Components
	TGroupBox *GroupBoxWhere;
	TRadioButton *RadioButtonAllSongs;
	TRadioButton *RadioButtonCurrentSong;
	TRadioButton *RadioButtonCurrentChannel;
	TSpeedButton *SpeedButtonOK;
	TSpeedButton *SpeedButtonCancel;
	TGroupBox *GroupBox1;
	TRadioButton *RadioButtonAllInstruments;
	TRadioButton *RadioButtonCurrentInstrument;
	TRadioButton *RadioButtonBlock;
	TEdit *EditValue;
	TUpDown *UpDownValue;
	TLabel *LabelHint;
	void __fastcall SpeedButtonOKClick(TObject *Sender);
	void __fastcall SpeedButtonCancelClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall EditValueKeyPress(TObject *Sender, char &Key);
	void __fastcall FormShow(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFormTranspose(TComponent* Owner);

	bool Confirm;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormTranspose *FormTranspose;
//---------------------------------------------------------------------------
#endif
