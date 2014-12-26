//---------------------------------------------------------------------------

#ifndef UnitSubSongH
#define UnitSubSongH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFormSubSong : public TForm
{
__published:	// IDE-managed Components
	TLabel *Label1;
	TEdit *EditSubSong;
	TUpDown *UpDownSubSong;
	TButton *ButtonImport;
	void __fastcall SpeedButtonImportClick(TObject *Sender);
	void __fastcall ButtonImportClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFormSubSong(TComponent* Owner);

	bool Import;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormSubSong *FormSubSong;
//---------------------------------------------------------------------------
#endif
