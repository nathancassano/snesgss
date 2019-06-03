//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "UnitOutputMonitor.h"
#include "UnitMain.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormOutputMonitor *FormOutputMonitor;


void __fastcall TFormOutputMonitor::DrawLevel(int x,int y,int max,int pos,int peak)
{
	TCanvas *c;
	int px;

	c=Canvas;

	c->Pen->Color=clBlack;
	c->Brush->Color=clLime;
	c->Rectangle(x,y,x+pos,y+10);
	c->Brush->Color=clBlack;
	c->Rectangle(x+pos,y,x+max,y+10);

	px=x+peak;

	if(px<x+1) px=x+1;
	if(px>x+max-2) px=x+max-2;

	c->Pen->Color=clRed;
	c->PenPos=TPoint(px,y);
	c->LineTo(x+peak,y+10);
	c->PenPos=TPoint(px+1,y);
	c->LineTo(x+peak+1,y+10);
}

//---------------------------------------------------------------------------
__fastcall TFormOutputMonitor::TFormOutputMonitor(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TFormOutputMonitor::FormPaint(TObject *Sender)
{
	DrawLevel(10,10,256,OutL*256/32768,PeakL*256/32768);
	DrawLevel(10,25,256,OutR*256/32768,PeakR*256/32768);
}
//---------------------------------------------------------------------------

void __fastcall TFormOutputMonitor::FormCreate(TObject *Sender)
{
	OutL=0;
	OutR=0;
	PeakL=0;
	PeakR=0;

	DoubleBuffered=true;
}
//---------------------------------------------------------------------------

