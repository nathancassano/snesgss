//---------------------------------------------------------------------------

#ifndef UnitMainH
#define UnitMainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Menus.hpp>
#include <Dialogs.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>
#include <ComCtrls.hpp>
#include <FileCtrl.hpp>

#include <vcl.h>
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

//---------------------------------------------------------------------------

#define MAX_INSTRUMENTS		99
#define MAX_SONGS			99
#define MAX_ROWS			10000

#define DEFAULT_PAGE_ROWS	16

struct instrumentStruct {
	short *source;
	int source_length;
	int source_rate;
	int source_volume;

	int env_ar;
	int env_dr;
	int env_sl;
	int env_sr;

	int length;

	int loop_start;
	int loop_end;
	bool loop_enable;
	bool loop_unroll;

	int wav_loop_start;
	int wav_loop_end;

	int resample_type;
	int downsample_factor;
	bool ramp_enable;

	int BRR_adr;
	int BRR_size;

	int eq_low;
	int eq_mid;
	int eq_high;
	
	AnsiString name;
};

struct noteFieldStruct {
	unsigned char note;
	unsigned char instrument;
	unsigned char effect;
	unsigned char value;
	unsigned char volume;
};

struct rowStruct {
	int speed;
	bool marker;
	AnsiString name;
	noteFieldStruct chn[8];
};

struct songStruct {
	rowStruct row[MAX_ROWS];

	int length;
	int loop_start;
	int measure;

	bool effect;

	int compiled_size;

	AnsiString name;
};

struct undoStruct {
	rowStruct row[MAX_ROWS];

	bool available;

	int colCur;
	int rowCur;
};

//---------------------------------------------------------------------------
class TFormMain : public TForm
{
	__published:	// IDE-managed Components
	TMainMenu *MainMenu;
	TMenuItem *MFile;
	TMenuItem *MExit;
	TMenuItem *N1;
	TMenuItem *MOpen;
	TMenuItem *MSave;
	TOpenDialog *OpenDialogModule;
	TSaveDialog *SaveDialogModule;
	TOpenDialog *OpenDialogWave;
	TPageControl *PageControlMode;
	TTabSheet *TabSheetSong;
	TTabSheet *TabSheetInstruments;
	TTabSheet *TabSheetInfo;
	TGroupBox *GroupBoxSampleInfo;
	TLabel *LabelWavInfo;
	TLabel *LabelBRRInfo;
	TGroupBox *GroupBoxSampleEnvelope;
	TLabel *LabelAR;
	TLabel *LabelDR;
	TLabel *LabelSL;
	TLabel *LabelSR;
	TTrackBar *TrackBarAR;
	TTrackBar *TrackBarDR;
	TTrackBar *TrackBarSL;
	TTrackBar *TrackBarSR;
	TGroupBox *GroupBoxSampleList;
	TEdit *EditInsName;
	TListBox *ListBoxIns;
	TGroupBox *GroupBoxSampleLength;
	TLabel *LabelSize;
	TSpeedButton *SpeedButtonLengthToMax;
	TEdit *EditLength;
	TGroupBox *GroupBoxDownsampling;
	TSpeedButton *SpeedButtonDownsample1;
	TSpeedButton *SpeedButtonDownsample2;
	TSpeedButton *SpeedButtonDownsample4;
	TGroupBox *GroupBoxResampling;
	TSpeedButton *SpeedButtonResampleNearest;
	TSpeedButton *SpeedButtonResampleLinear;
	TSpeedButton *SpeedButtonResampleCubic;
	TSpeedButton *SpeedButtonResampleSine;
	TSpeedButton *SpeedButtonResampleBand;
	TGroupBox *GroupBoxSampleMisc;
	TLabel *LabelVolume;
	TTrackBar *TrackBarVolume;
	TGroupBox *GroupBoxSampleLoop;
	TSpeedButton *SpeedButtonLoop;
	TLabel *LabelLoopStart;
	TSpeedButton *SpeedButtonLoopToBegin;
	TLabel *LabelLoopEnd;
	TSpeedButton *SpeedButtonLoopToEnd;
	TSpeedButton *SpeedButtonLoopWav;
	TEdit *EditLoopStart;
	TEdit *EditLoopEnd;
	TSpeedButton *SpeedButtonImportWav;
	TGroupBox *GroupBoxWaveEnvPreview;
	TPaintBox *PaintBoxADSR;
	TSpeedButton *SpeedButtonInsMoveUp;
	TSpeedButton *SpeedButtonInsMoveDown;
	TSpeedButton *SpeedButtonInsDelete;
	TSpeedButton *SpeedButtonInsLoad;
	TSpeedButton *SpeedButtonInsSave;
	TOpenDialog *OpenDialogInstrument;
	TSaveDialog *SaveDialogInstrument;
	TPaintBox *PaintBoxSong;
	TMenuItem *MSongPlayStart;
	TMenuItem *MSongPlayCur;
	TMenuItem *MSongStop;
	TMenuItem *MOctave;
	TMenuItem *MAutostep;
	TMenuItem *MSong;
	TMenuItem *N2;
	TMenuItem *MSongClear;
	TTabSheet *TabSheetSongList;
	TGroupBox *GroupBoxSongList;
	TEdit *EditSongName;
	TListBox *ListBoxSong;
	TSpeedButton *SpeedButtonSongUp;
	TSpeedButton *SpeedButtonSongDown;
	TCheckBox *CheckBoxEffect;
	TSpeedButton *SpeedButtonLoopRamp;
	TMenuItem *MExportSPC;
	TSaveDialog *SaveDialogExportSPC;
	TLabel *Label5;
	TLabel *Label6;
	TMenuItem *N3;
	TMenuItem *MExport;
	TMenuItem *MImportXM;
	TMenuItem *MImportXMSong;
	TMenuItem *MImportXMSoundEffects;
	TOpenDialog *OpenDialogImportXM;
	TMenuItem *MImportSPCBank;
	TMenuItem *MOctave1;
	TMenuItem *MOctave2;
	TMenuItem *MOctave3;
	TMenuItem *MOctave4;
	TMenuItem *MOctave5;
	TMenuItem *MOctave6;
	TMenuItem *MOctave7;
	TMenuItem *MOctave8;
	TOpenDialog *OpenDialogImportSPCBank;
	TMenuItem *MCleanupInstruments;
	TTimer *TimerScrollDelay;
	TSpeedButton *SpeedButtonSampleLengthInc;
	TSpeedButton *SpeedButtonSampleLengthDec;
	TSpeedButton *SpeedButtonLoopStartInc;
	TSpeedButton *SpeedButtonLoopStartDec;
	TSpeedButton *SpeedButtonLoopEndInc;
	TSpeedButton *SpeedButtonLoopEndDec;
	TMenuItem *MInstruments;
	TMenuItem *MSongCleanUp;
	TMenuItem *MExtractSourceWave;
	TSaveDialog *SaveDialogExportWav;
	TOpenDialog *OpenDialogImportMidi;
	TMenuItem *MImportMidi;
	TMenuItem *N4;
	TMenuItem *N5;
	TStaticText *StaticTextEQLow;
	TStaticText *StaticTextEQMid;
	TStaticText *StaticTextEQHigh;
	TSpeedButton *SpeedButtonEQReset;
	TMenuItem *N6;
	TMenuItem *MInstrumentReplace;
	TMenuItem *N7;
	TMenuItem *MTransposeDialog;
	TMenuItem *MExportAndSave;
	TGroupBox *GroupBoxMemoryUse;
	TPaintBox *PaintBoxMemoryUse;
	TGroupBox *GroupBoxAbout;
	TLabel *Label4;
	TLabel *Label1;
	TLabel *Label2;
	TLabel *Label3;
	TLabel *Label7;
	TMenuItem *MImportFamiTracker;
	TOpenDialog *OpenDialogImportFTM;
	TSpeedButton *SpeedButtonLoopUnroll;
	TMenuItem *MSongScaleVolume;
	TMenuItem *N8;
	TMenuItem *N9;
	TMenuItem *MOutputMonitor;
	TTimer *TimerOutputMonitor;
	TLabel *Label8;
	TMenuItem *MNew;
	TMenuItem *N10;
	TMenuItem *N11;
	TMenuItem *N12;
	TMenuItem *MInstrumentAutoNumber;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall MExitClick(TObject *Sender);
	void __fastcall EditInsNameChange(TObject *Sender);
	void __fastcall MOpenClick(TObject *Sender);
	void __fastcall MSaveClick(TObject *Sender);
	void __fastcall SpeedButtonImportWavClick(TObject *Sender);
	void __fastcall PaintBoxADSRPaint(TObject *Sender);
	void __fastcall TrackBarARChange(TObject *Sender);
	void __fastcall TrackBarDRChange(TObject *Sender);
	void __fastcall TrackBarSLChange(TObject *Sender);
	void __fastcall TrackBarSRChange(TObject *Sender);
	void __fastcall TrackBarVolumeChange(TObject *Sender);
	void __fastcall SpeedButtonLoopClick(TObject *Sender);
	void __fastcall SpeedButtonLoopToBeginClick(TObject *Sender);
	void __fastcall SpeedButtonLoopToEndClick(TObject *Sender);
	void __fastcall EditLengthChange(TObject *Sender);
	void __fastcall EditLoopStartChange(TObject *Sender);
	void __fastcall SpeedButtonLoopWavClick(TObject *Sender);
	void __fastcall SpeedButtonLengthToMaxClick(TObject *Sender);
	void __fastcall EditLoopEndChange(TObject *Sender);
	void __fastcall SpeedButtonResampleNearestClick(TObject *Sender);
	void __fastcall SpeedButtonInsDeleteClick(TObject *Sender);
	void __fastcall SpeedButtonInsMoveUpClick(TObject *Sender);
	void __fastcall SpeedButtonInsMoveDownClick(TObject *Sender);
	void __fastcall SpeedButtonInsLoadClick(TObject *Sender);
	void __fastcall SpeedButtonInsSaveClick(TObject *Sender);
	void __fastcall PaintBoxSongPaint(TObject *Sender);
	void __fastcall PaintBoxSongMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall MSongPlayStartClick(TObject *Sender);
	void __fastcall MSongPlayCurClick(TObject *Sender);
	void __fastcall MSongStopClick(TObject *Sender);
	void __fastcall MSongClearClick(TObject *Sender);
	void __fastcall MAutostepClick(TObject *Sender);
	void __fastcall EditSongNameChange(TObject *Sender);
	void __fastcall ListBoxSongClick(TObject *Sender);
	void __fastcall CheckBoxEffectClick(TObject *Sender);
	void __fastcall SpeedButtonSongUpClick(TObject *Sender);
	void __fastcall SpeedButtonSongDownClick(TObject *Sender);
	void __fastcall PaintBoxSongMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y);
	void __fastcall SpeedButtonLoopRampClick(TObject *Sender);
	void __fastcall SpeedButtonDownsample1Click(TObject *Sender);
	void __fastcall TabSheetInfoShow(TObject *Sender);
	void __fastcall PaintBoxMemoryUsePaint(TObject *Sender);
	void __fastcall MExportSPCClick(TObject *Sender);
	void __fastcall ListBoxSongDblClick(TObject *Sender);
	void __fastcall MExportClick(TObject *Sender);
	void __fastcall MImportXMSongClick(TObject *Sender);
	void __fastcall MImportXMSoundEffectsClick(TObject *Sender);
	void __fastcall MImportSPCBankClick(TObject *Sender);
	void __fastcall MOctave1Click(TObject *Sender);
	void __fastcall ListBoxInsDblClick(TObject *Sender);
	void __fastcall MCleanupInstrumentsClick(TObject *Sender);
	void __fastcall TimerScrollDelayTimer(TObject *Sender);
	void __fastcall EditLengthKeyPress(TObject *Sender, char &Key);
	void __fastcall SpeedButtonSampleLengthIncMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall SpeedButtonSampleLengthDecMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall SpeedButtonLoopStartIncMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall SpeedButtonLoopStartDecMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall SpeedButtonLoopEndIncMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall SpeedButtonLoopEndDecMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall ListBoxInsDrawItem(TWinControl *Control, int Index,
          TRect &Rect, TOwnerDrawState State);
	void __fastcall ListBoxSongDrawItem(TWinControl *Control, int Index,
          TRect &Rect, TOwnerDrawState State);
	void __fastcall MSongCleanUpClick(TObject *Sender);
	void __fastcall MExtractSourceWaveClick(TObject *Sender);
	void __fastcall MImportMidiClick(TObject *Sender);
	void __fastcall StaticTextEQLowMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y);
	void __fastcall StaticTextEQLowMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall SpeedButtonEQResetClick(TObject *Sender);
	void __fastcall MInstrumentReplaceClick(TObject *Sender);
	void __fastcall MTransposeDialogClick(TObject *Sender);
	void __fastcall ListBoxInsMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall TabSheetSongListEnter(TObject *Sender);
	void __fastcall MExportAndSaveClick(TObject *Sender);
	void __fastcall PaintBoxSongMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall PaintBoxSongDblClick(TObject *Sender);
	void __fastcall MImportFamiTrackerClick(TObject *Sender);
	void __fastcall SpeedButtonLoopUnrollClick(TObject *Sender);
	void __fastcall MSongScaleVolumeClick(TObject *Sender);
	void __fastcall MOutputMonitorClick(TObject *Sender);
	void __fastcall TimerOutputMonitorTimer(TObject *Sender);
	void __fastcall EditSongNameKeyPress(TObject *Sender, char &Key);
	void __fastcall MNewClick(TObject *Sender);
	void __fastcall EditInsNameKeyPress(TObject *Sender, char &Key);

private:	// User declarations
public:		// User declarations
	__fastcall TFormMain(TComponent* Owner);

	void __fastcall TFormMain::BRREncode(int ins);

	int __fastcall TFormMain::SamplesCompile(int adr,int one_sample);

	bool __fastcall TFormMain::SongIsChannelEmpty(songStruct *s,int chn);
	bool __fastcall TFormMain::SongIsRowEmpty(songStruct *s,int row,bool marker);
	bool __fastcall TFormMain::SongIsEmpty(songStruct *s);
	int __fastcall TFormMain::SongFindLastRow(songStruct *s);
	void __fastcall TFormMain::SongCleanUp(songStruct *s);

	int __fastcall TFormMain::ChannelCompile(songStruct *s,int chn,int start_row,int compile_adr,int &play_adr);
	int __fastcall TFormMain::ChannelCompress(int compile_adr,int &play_adr,int loop_adr,int src_size,int ref_max);
	void __fastcall TFormMain::ChannelCompressFlush(int compile_adr);
	int __fastcall TFormMain::SongCompile(songStruct *s,int row,int adr,bool mute);
	int __fastcall TFormMain::EffectsCompile(int start_adr);
	int __fastcall TFormMain::DelayCompile(int adr,int delay);

	bool __fastcall TFormMain::SPCCompile(songStruct *s,int start_row,bool mute,bool effects,int one_sample);

	void __fastcall TFormMain::SPCPlaySong(songStruct *s,int start_row,bool mute,int one_sample);
	void __fastcall TFormMain::SPCPlayRow(int start_row);
	void __fastcall TFormMain::SPCPlayNote(int note,int ins);
	void __fastcall TFormMain::SPCStop(void);


	void __fastcall TFormMain::ModuleInit(void);
	void __fastcall TFormMain::ModuleClear(void);
	bool __fastcall TFormMain::ModuleOpenFile(AnsiString filename);
	void __fastcall TFormMain::ModuleOpen(AnsiString filename);
	bool __fastcall TFormMain::ModuleSave(AnsiString filename);

	void __fastcall TFormMain::InsListUpdate(void);
	void __fastcall TFormMain::InsChange(int ins);
	void __fastcall TFormMain::InsUpdateControls(void);
	void __fastcall TFormMain::UpdateEQ(void);

	void __fastcall TFormMain::RenderADSR(void);

	void __fastcall TFormMain::InstrumentClear(int ins);

	void __fastcall TFormMain::InstrumentDataWrite(FILE *file,int id,int ins);
	void __fastcall TFormMain::InstrumentDataParse(int id,int ins);

	int __fastcall TFormMain::PatternGetTopRow2x(void);
	int __fastcall TFormMain::PatternScreenToActualRow(int srow);
	void __fastcall TFormMain::CenterView(void);
	void __fastcall TFormMain::RenderPattern(void);
	void __fastcall TFormMain::RenderPatternColor(TCanvas *c,int row,int col,TColor bgCol);

	void __fastcall TFormMain::AppMessage(tagMSG &Msg, bool &Handled);

	void __fastcall TFormMain::SongMoveCursor(int rowx,int colx,bool relative);
	void __fastcall TFormMain::SongMoveRowCursor(int off);
	void __fastcall TFormMain::SongMoveColCursor(int off);
	void __fastcall TFormMain::SongMoveChannelCursor(int off);

	int __fastcall TFormMain::IsNumberKey(int key);
	int __fastcall TFormMain::IsNoteKey(int key);
	bool __fastcall TFormMain::IsAllNoteKeysUp(void);

	void __fastcall TFormMain::SongDeleteShiftColumn(int col,int row);
	void __fastcall TFormMain::SongInsertShiftColumn(int col,int row);
	void __fastcall TFormMain::SongDeleteShift(int col,int cur);
	void __fastcall TFormMain::SongInsertShift(int col,int row);

	void __fastcall TFormMain::ToggleChannelMute(int chn,bool solo);

	void __fastcall TFormMain::UpdateInfo(bool header_only);
	void __fastcall TFormMain::SongDataClear(int song);
	void __fastcall TFormMain::SongClear(int song);

	void __fastcall TFormMain::SongMoveRowPrevMarker(void);
	void __fastcall TFormMain::SongMoveRowNextMarker(void);

	void __fastcall TFormMain::ResetUndo(void);
	void __fastcall TFormMain::SetUndo(void);
	void __fastcall TFormMain::Undo(void);

	void __fastcall TFormMain::SongListUpdate(void);
	void __fastcall TFormMain::SongUpdateControls(void);
	void __fastcall TFormMain::SongChange(int song);

	void __fastcall TFormMain::ResetSelection(void);
	void __fastcall TFormMain::StartSelection(int col,int row);
	void __fastcall TFormMain::UpdateSelection(void);
	void __fastcall TFormMain::SetChannelSelection(bool channel,bool section);

	void __fastcall TFormMain::ResetCopyBuffer(void);
	void __fastcall TFormMain::CopyCutToBuffer(bool copy,bool cut,bool shift);
	void __fastcall TFormMain::PasteFromBuffer(bool shift,bool mix);

	void __fastcall TFormMain::TransposeArea(int semitones,int song,int chn,int row,int width,int height,int ins);
	void __fastcall TFormMain::Transpose(int semitones,bool block,bool song,bool channel,int ins);

	void __fastcall TFormMain::ScaleVolumeArea(int percent,int song,int chn,int row,int width,int height,int ins);
	void __fastcall TFormMain::ScaleVolume(int percent,bool block,bool song,bool channel,int ins);

	void __fastcall TFormMain::ReplaceInstrumentArea(int song,int chn,int row,int width,int height,int from,int to);
	void __fastcall TFormMain::ReplaceInstrument(bool block,bool song,bool channel,int from,int to);

	void __fastcall TFormMain::ExpandSelection(void);
	void __fastcall TFormMain::ShrinkSelection(void);

	void __fastcall TFormMain::SwapInstrumentNumberAll(int ins1,int ins2);

	void __fastcall TFormMain::RenderMemoryUse(void);

	char* __fastcall TFormMain::MakeNameForAlias(AnsiString name);
	bool __fastcall TFormMain::ExportAll(AnsiString dir);

	void __fastcall TFormMain::UpdateAll(void);
	AnsiString __fastcall TFormMain::ImportXM(AnsiString filename,bool song);
	void __fastcall TFormMain::CompileAllSongs(void);
	int __fastcall TFormMain::InsCalculateBRRSize(int ins,bool loop_only);

	AnsiString __fastcall TFormMain::GetSectionName(int row);
	void __fastcall TFormMain::SetSectionName(int row,AnsiString name);

	int __fastcall TFormMain::SongCalculateDuration(int song);

	void __fastcall TFormMain::EnterNoteKey(int note);

	void __fastcall TFormMain::MInstrumentItemClick(TObject *Sender);

	int WaveOutSampleRate;
	int WaveOutBufferSize;
	int WaveOutBufferCount;

	int InsCur;

	HMIDIIN MidiHandle;
	unsigned char MidiKeyState[128];
};
//---------------------------------------------------------------------------
extern PACKAGE TFormMain *FormMain;
//---------------------------------------------------------------------------
#endif
