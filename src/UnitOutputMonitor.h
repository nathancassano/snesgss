//---------------------------------------------------------------------------

#ifndef UnitOutputMonitorH
#define UnitOutputMonitorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFormOutputMonitor : public TForm
{
__published:	// IDE-managed Components
	TLabel *LabelInfo;
	TLabel *LabelNote;
	TLabel *LabelCents;
	void __fastcall FormPaint(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TFormOutputMonitor(TComponent* Owner);

	void __fastcall TFormOutputMonitor::DrawLevel(int x,int y,int max,int pos,int peak);

	int OutL;
	int OutR;

	int PeakL;
	int PeakR;
};
//---------------------------------------------------------------------------
extern PACKAGE TFormOutputMonitor *FormOutputMonitor;
//---------------------------------------------------------------------------
#endif
