//---------------------------------------------------------------------------
#pragma hdrstop

#include "UnitMain.h"
#include "UnitTranspose.h"
#include "UnitReplace.h"
#include "UnitSectionName.h"
#include "UnitSectionList.h"
#include "UnitSubSong.h"
#include "UnitOutputMonitor.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFormMain *FormMain;



#define VERSION_STR				"SNES GSS v1.42"

#define CONFIG_NAME				"snesgss.cfg"
#define PROJECT_SIGNATURE 		"[SNESGSS Module]"
#define INSTRUMENT_SIGNATURE 	"[SNESGSS Instrument]"

#define EX_VOL_SIGNATURE		"[EX VOL]"

#define UPDATE_RATE_HZ			160
#define DEFAULT_SPEED			(UPDATE_RATE_HZ/(120*(1.0/60.0)*4)) //default speed for 120 BPM

#define ENABLE_SONG_COMPRESSION



#include "spc700.h"

#include "gme/gme.h"

Music_Emu* emu=NULL;

bool MusicActive;

void handle_gme_error(gme_err_t error)
{
	if(error)
	{
		Application->MessageBox(error,"GME Error",MB_OK);
		exit(1);
	}
}

#include "tuner.h"
#include "waveout.h"
#include "config.h"
#include "3band_eq.h"



instrumentStruct insList[MAX_INSTRUMENTS];



unsigned char *BRRTemp;
int BRRTempAllocSize;
int BRRTempSize;
int BRRTempLoop;

unsigned char SPCTemp[66048];
unsigned char SPCTempPlay[66048];
unsigned char SPCMem[65536];

unsigned char SPCChnMem[65536];
unsigned char SPCChnPack[65536];

unsigned char CompressSeqBuf[256];

int CompressSrcPtr;
int CompressSeqPtr;
int CompressOutPtr;

songStruct songList[MAX_SONGS];

songStruct tempSong;

undoStruct UndoBuffer;


int SongCur;
int RowCur;
int RowView;
int ColCur;
int ColCurPrev;
int OctaveCur;

int TextFontHeight;
int TextFontWidth;

int PageStep;
int AutoStep;

bool ResetNumber;

bool ChannelMute[8];

int SelectionColS;
int SelectionRowS;
int SelectionColE;
int SelectionRowE;
int SelectionWidth;
int SelectionHeight;

unsigned char CopyBuffer[8*5+1][MAX_ROWS];
int CopyBufferColumnType[8*5+1];
int CopyBufferWidth;
int CopyBufferHeight;

bool ShiftKeyIsDown;

int SPCInstrumentsSize;
int SPCMusicSize;
int SPCEffectsSize;
int SPCMemTopAddr;
int SPCMusicLargestSize;
int InstrumentRenumberList[MAX_INSTRUMENTS];
int InstrumentsCount;

bool UpdateSampleData;

int PrevMouseY;

bool SongDoubleClick;



#define COLUMN_TYPE_SPEED   		0
#define COLUMN_TYPE_INSTRUMENT		1
#define COLUMN_TYPE_NOTE			2
#define COLUMN_TYPE_EFFECT			3
#define COLUMN_TYPE_VALUE	   		4



const AnsiString NoteNames[12]={"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"};

const unsigned char noteKeyCodes[]={
	'1',255,
	'A',255,

	'Z',0,
	'S',1,
	'X',2,
	'D',3,
	'C',4,
	'V',5,
	'G',6,
	'B',7,
	'H',8,
	'N',9,
	'J',10,
	'M',11,
	VK_OEM_COMMA,12,// <,
	'L',13,
	VK_OEM_PERIOD,14,// >.
	VK_OEM_1,15,// ::
	VK_OEM_2,16,// ?/

	'Q',12,
	'2',13,
	'W',14,
	'3',15,
	'E',16,
	'R',17,
	'5',18,
	'T',19,
	'6',20,
	'Y',21,
	'7',22,
	'U',23,

	'I',24,
	'9',25,
	'O',26,
	'0',27,
	'P',28
};

const int PatternColumnOffsets[1+4+1+2+1+11*8]={
	-1, 0, 0, 0, 0,
	-1, 1, 1,
	-1, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6,
	-1, 7, 7, 7, 8, 8, 9, 9,10,11,11,
	-1,12,12,12,13,13,14,14,15,16,16,
	-1,17,17,17,18,18,19,19,20,21,21,
	-1,22,22,22,23,23,24,24,25,26,26,
	-1,27,27,27,28,28,29,29,30,31,31,
	-1,32,32,32,33,33,34,34,35,36,36,
	-1,37,37,37,38,38,39,39,40,41,41,
	-1
};

AnsiString ModuleFileName;



float get_volume_scale(int vol)
{
	if(vol<128) return 1.0f+(float)(vol-128)/128.0f; else return 1.0f+(float)(vol-128)/64.0f;
}



#include "brr/brr.cpp"
#include "brr/brr_encoder.cpp"


void __fastcall TFormMain::BRREncode(int ins)
{
	const char resample_list[]="nlcsb";
	int i,smp,ptr,off,blocks,src_length,new_length,sum;
	int new_loop_start,padding,loop_size,new_loop_size;
	int src_loop_start,src_loop_end;
	short *source,*sample,*temp;
	float volume,s,delta,fade,ramp;
	bool loop_enable,loop_flag,end_flag;
	char resample_type;
	float scale;
	EQSTATE eq;
	double in_sample,out_sample,band_low,band_high,rate;

	if(BRRTemp)
	{
		free(BRRTemp);

		BRRTemp=NULL;
		BRRTempSize=0;
		BRRTempLoop=-1;
	}

	//get the source sample, downsample it if needed

	src_length=insList[ins].length;

	if(!insList[ins].source||src_length<16) return;//sample is shorter than one BRR block

	source=(short*)malloc(src_length*sizeof(short));

	memcpy(source,insList[ins].source,src_length*sizeof(short));

	//apply EQ if it is not reset, before any downsampling, as it needed to compensate downsampling effects as well

	if(insList[ins].eq_low!=0||insList[ins].eq_mid!=0||insList[ins].eq_high!=0)
	{
		rate=insList[ins].source_rate;

		band_low=rate/50.0;//880 for 44100
		band_high=rate/8.82;//5000 for 44100

		init_3band_state(&eq,band_low,band_high,rate);

		eq.lg = (double)(64+insList[ins].eq_low)/64.0f;
		eq.mg = (double)(64+insList[ins].eq_mid)/64.0f;
		eq.hg = (double)(64+insList[ins].eq_high)/64.0f;

		for(i=0;i<src_length;++i)
		{
			in_sample=(double)source[i]/32768.0;

			out_sample=do_3band(&eq,in_sample);

			out_sample*=32768.0;

			if(out_sample<-32768) out_sample=-32768;
			if(out_sample> 32767) out_sample= 32767;

			source[i]=(short int)out_sample;
		}
	}

	//get scale factor for downsampling

	resample_type=resample_list[insList[ins].resample_type];

	src_loop_start=insList[ins].loop_start;
	src_loop_end  =insList[ins].loop_end+1;//loop_end is the last sample of the loop, to calculate length it needs to be next to the last

	switch(insList[ins].downsample_factor)
	{
	case 1:  scale=.5f;  break;
	case 2:  scale=.25f; break;
	default: scale=1.0f;
	}

	if(scale!=1.0f)
	{
		new_length=(float(src_length)*scale);

		source=resample(source,src_length,new_length,resample_type);

		src_length    =new_length;
		src_loop_start=(float(src_loop_start)*scale);
		src_loop_end  =(float(src_loop_end  )*scale);
	}

	//align the sample as required

	loop_enable=insList[ins].loop_enable;

	if(!loop_enable)//no loop, just pad the source with zeroes to 16-byte boundary
	{
		new_length=(src_length+15)/16*16;

		sample=(short*)malloc(new_length*sizeof(short));

		ptr=0;

		for(i=0;i<src_length;++i) sample[ptr++]=source[i];

		for(i=src_length;i<new_length;++i) sample[ptr++]=0;//pad with zeroes

		BRRTempLoop=-1;
	}
	else
	{
		if(!insList[ins].loop_unroll)//resample the loop part, takes less memory, but lower quality of the loop
		{
			new_loop_start=(src_loop_start+15)/16*16;//align the loop start point to 16 samples

			padding=new_loop_start-src_loop_start;//calculate padding, how many zeroes to insert at beginning

			loop_size=src_loop_end-src_loop_start;//original loop length

			new_loop_size=loop_size/16*16;//calculate new loop size, aligned to 16 samples

			if((loop_size&15)>=8) new_loop_size+=16;//align to closest point, to minimize detune

			new_length=new_loop_start+new_loop_size;//calculate new source length

			sample=(short*)malloc(new_length*sizeof(short));

			ptr=0;

			for(i=0;i<padding;++i) sample[ptr++]=0;//add the padding bytes

			for(i=0;i<src_loop_start;++i) sample[ptr++]=source[i];//copy the part before loop

			if(new_loop_size==loop_size)//just copy the loop part
			{
				for(i=0;i<new_loop_size;++i) sample[ptr++]=source[src_loop_start+i];
			}
			else
			{
				temp=(short*)malloc(loop_size*sizeof(short));//temp copy of the looped part, as resample function frees up the source

				memcpy(temp,&source[src_loop_start],loop_size*sizeof(short));

				temp=resample(temp,loop_size,new_loop_size,resample_type);

				for(i=0;i<new_loop_size;++i) sample[ptr++]=temp[i];

				free(temp);
			}

			BRRTempLoop=new_loop_start/16;//loop point in blocks
		}
		else//unroll the loop, best quality in trade for higher memory use
		{
			new_loop_start=(src_loop_start+15)/16*16;//align the loop start point to 16 samples

			padding=new_loop_start-src_loop_start;//calculate padding, how many zeroes to insert at beginning

			loop_size=src_loop_end-src_loop_start;//original loop length

			new_length=new_loop_start;

			sample=(short*)malloc(new_length*sizeof(short));

			ptr=0;

			for(i=0;i<padding;++i) sample[ptr++]=0;//add the padding bytes

			for(i=0;i<src_loop_start;++i) sample[ptr++]=source[i];//copy the part before loop

			while(1)
			{
				if(new_length<ptr+loop_size)
				{
					new_length=ptr+loop_size;

					sample=(short*)realloc(sample,new_length*sizeof(short));
				}

				for(i=0;i<loop_size;++i) sample[ptr++]=source[src_loop_start+i];

				new_length=ptr;

				if(!(new_length&15)||new_length>=65536) break;
			}

			BRRTempLoop=new_loop_start/16;//loop point in blocks
		}
	}

	free(source);

	//apply volume

	volume=get_volume_scale(insList[ins].source_volume);

	for(i=0;i<new_length;++i)
	{
		smp=(int)(((float)sample[i])*volume);

		if(smp<-32768) smp=-32768;
		if(smp> 32767) smp= 32767;

		sample[i]=smp;
	}

	//smooth out the loop transition

	if(loop_enable&&insList[ins].ramp_enable)
	{
		ptr=new_length-16;

		fade=1.0f;
		ramp=0.0f;
		delta=((float)sample[new_loop_start])/16.0f;

		for(i=0;i<16;++i)
		{
			s=(float)sample[ptr];

			s=s*fade+ramp;
			fade-=1.0f/16.0f;
			ramp+=delta;

			sample[ptr++]=(short)s;
		}
	}

	//convert to brr

	BRRTempAllocSize=16384;

	BRRTemp=(unsigned char*)malloc(BRRTempAllocSize);

	ptr=0;
	off=0;

	blocks=new_length/16;

	sum=0;

	//add initial block if there is any non-zero value in the first sample block
	//it is not clear if it is really needed

	for(i=0;i<16;++i) sum+=sample[i];

	if(sum)
	{
		memset(BRRTemp+ptr,0,9);

		if(loop_enable) BRRTemp[0]=0x02;//loop flag is always set for a looped sample

		ptr+=9;
	}

	//this ia a magic line
	ADPCMBlockMash(sample+(blocks-1)*16,false,true);
	//prevents clicks at the loop point and sound glitches somehow
	//tested few times it really affects the result
	//it seems that it caused by the squared error calculation in the function

	for(i=0;i<blocks;++i)
	{
		loop_flag=loop_enable&&((i==BRRTempLoop)?true:false);//loop flag is only set for the loop position

		end_flag =(i==blocks-1)?true:false;//end flag is set for the last block

		memset(BRR,0,9);

		ADPCMBlockMash(sample+off,loop_flag,end_flag);

		if(loop_enable) BRR[0]|=0x02;//loop flag is always set for a looped sample

		if(end_flag)  BRR[0]|=0x01;//end flag

		memcpy(BRRTemp+ptr,BRR,9);

		off+=16;
		ptr+=9;

		if(ptr>=BRRTempAllocSize-9)
		{
			BRRTempAllocSize+=16384;
			BRRTemp=(unsigned char*)realloc(BRRTemp,BRRTempAllocSize);
		}
	}

	free(sample);

	BRRTempSize=ptr;//actual encoded sample size in bytes

	if(sum&&BRRTempLoop>=0) ++BRRTempLoop;
}



int rd_word_lh(unsigned char *data)
{
	return data[0]+(data[1]<<8);
}



int rd_dword_lh(unsigned char *data)
{
	return data[0]+(data[1]<<8)+(data[2]<<16)+(data[3]<<24);
}



void wr_word_lh(unsigned char *data,int value)
{
	data[0]=value&255;
	data[1]=(value>>8)&255;
}



void wr_dword_lh(unsigned char *data,int value)
{
	data[0]=value&255;
	data[1]=(value>>8)&255;
	data[2]=(value>>16)&255;
	data[3]=(value>>24)&255;
}



int rd_word_hl(unsigned char *data)
{
	return (data[0]<<8)+data[1];
}



int rd_dword_hl(unsigned char *data)
{
	return (data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
}



int rd_var_length(unsigned char *data,int &ptr)
{
	int n,value;

	value=data[ptr++];

	if(value&0x80)
	{
		value&=0x7f;

		while(1)
		{
			n=data[ptr++];

			value=(value<<7)+(n&0x7f);

			if(!(n&0x80)) break;
		}
	}

	return value;
}



void __fastcall TFormMain::ResetSelection(void)
{
	SelectionWidth=0;
	SelectionHeight=0;
}



void __fastcall TFormMain::StartSelection(int col,int row)
{
	if(col<1) return;

	SelectionColS=col;
	SelectionRowS=row;
	SelectionColE=col;
	SelectionRowE=row;

	UpdateSelection();
}



void __fastcall TFormMain::UpdateSelection(void)
{
	SelectionWidth =abs(SelectionColS-SelectionColE)+1;
	SelectionHeight=abs(SelectionRowS-SelectionRowE)+1;
}



void __fastcall TFormMain::SetChannelSelection(bool channel,bool section)
{
	int col,chn,from,to;

	if(section)
	{
		from=RowCur;
		to  =RowCur+1;

		while(from>0)
		{
			if(songList[SongCur].row[from].marker) break;

			--from;
		}

		while(to<MAX_ROWS)
		{
			if(songList[SongCur].row[to].marker) break;

			++to;
		}
	}
	else
	{
		from=0;
		to=MAX_ROWS;
	}

	if(channel)
	{
		if(ColCur<1) return;

		if(ColCur==1)
		{
			SelectionColS=1;
			SelectionColE=1;
		}
		else
		{
			chn=(ColCur-2)/5;
			col=2+chn*5;

			SelectionColS=col;
			SelectionColE=col+4;
		}
	}
	else
	{
		SelectionColS=1;
		SelectionColE=1+8*5;
	}

	SelectionRowS=from;
	SelectionRowE=to-1;

	UpdateSelection();
	RenderPattern();
}



void __fastcall TFormMain::TransposeArea(int semitones,int song,int chn,int row,int width,int height,int ins)
{
	noteFieldStruct *n;
	int songc,chnc,rowc,note,ins_cur;

	for(songc=0;songc<MAX_SONGS;++songc)
	{
		if(song>=0&&songc!=song) continue;

		for(chnc=chn;chnc<chn+width;++chnc)
		{
			ins_cur=-1;

			for(rowc=row;rowc<row+height;++rowc)
			{
				n=&songList[songc].row[rowc].chn[chnc];

				if(n->instrument) ins_cur=n->instrument;

				if(n->note>1)
				{
					if(!ins||(ins_cur==ins))
					{
						note=n->note+semitones;

						if(note<2) note=2;
						if(note>2+95) note=2+95;

						n->note=note;
					}
				}
			}
		}
	}

	UpdateAll();
}



void __fastcall TFormMain::Transpose(int semitones,bool block,bool song,bool channel,int ins)
{
	int row,col,chn,wdt;

	if(!semitones) return;

	if(song||channel||block) SetUndo();

	if(block)
	{
		if(SelectionRowS<SelectionRowE) row=SelectionRowS; else row=SelectionRowE;
		if(SelectionColS<SelectionColE) col=SelectionColS; else col=SelectionColE;

		chn=(col-2)/5;
		wdt=(col+SelectionWidth-2+4)/5-chn;

		TransposeArea(semitones,SongCur,chn,row,wdt,SelectionHeight,ins);
	}
	else
	{
		if(channel) song=true;

		TransposeArea(semitones,song?SongCur:-1,channel?(ColCur-2)/5:0,0,channel?1:8,MAX_ROWS,ins);
	}
}



void __fastcall TFormMain::ScaleVolumeArea(int percent,int song,int chn,int row,int width,int height,int ins)
{
	noteFieldStruct *n;
	int songc,chnc,rowc,ins_cur,vol;

	for(songc=0;songc<MAX_SONGS;++songc)
	{
		if(song>=0&&songc!=song) continue;

		for(chnc=chn;chnc<chn+width;++chnc)
		{
			ins_cur=-1;

			for(rowc=row;rowc<row+height;++rowc)
			{
				n=&songList[songc].row[rowc].chn[chnc];

				if(n->instrument) ins_cur=n->instrument;

				if(n->volume!=255)
				{
					if(!ins||(ins_cur==ins))
					{
						vol=n->volume;

						vol=vol*percent/100;

						if(vol<0)  vol=0;
						if(vol>99) vol=99;

						n->volume=vol;
					}
				}
			}
		}
	}

	UpdateAll();
}



void __fastcall TFormMain::ScaleVolume(int percent,bool block,bool song,bool channel,int ins)
{
	int row,col,chn,wdt;

	if(!percent) return;

	if(song||channel||block) SetUndo();

	if(block)
	{
		if(SelectionRowS<SelectionRowE) row=SelectionRowS; else row=SelectionRowE;
		if(SelectionColS<SelectionColE) col=SelectionColS; else col=SelectionColE;

		chn=(col-2)/5;
		wdt=(col+SelectionWidth-2+4)/5-chn;

		ScaleVolumeArea(percent,SongCur,chn,row,wdt,SelectionHeight,ins);
	}
	else
	{
		if(channel) song=true;

		ScaleVolumeArea(percent,song?SongCur:-1,channel?(ColCur-2)/5:0,0,channel?1:8,MAX_ROWS,ins);
	}
}



void __fastcall TFormMain::ReplaceInstrumentArea(int song,int chn,int row,int width,int height,int from,int to)
{
	noteFieldStruct *n;
	int songc,chnc,rowc;

	for(songc=0;songc<MAX_SONGS;++songc)
	{
		if(song>=0&&songc!=song) continue;

		for(chnc=chn;chnc<chn+width;++chnc)
		{
			for(rowc=row;rowc<row+height;++rowc)
			{
				n=&songList[songc].row[rowc].chn[chnc];

				if(n->instrument==from) n->instrument=to;
			}
		}
	}

	UpdateAll();
}



void __fastcall TFormMain::ReplaceInstrument(bool block,bool song,bool channel,int from,int to)
{
	int row,col,chn,wdt;

	if(song||channel||block) SetUndo();

	if(block)
	{
		if(SelectionRowS<SelectionRowE) row=SelectionRowS; else row=SelectionRowE;
		if(SelectionColS<SelectionColE) col=SelectionColS; else col=SelectionColE;

		chn=(col-2)/5;
		wdt=(col+SelectionWidth-2+4)/5-chn;

		ReplaceInstrumentArea(SongCur,chn,row,wdt,SelectionHeight,from,to);
	}
	else
	{
		if(channel) song=true;

		ReplaceInstrumentArea(song?SongCur:-1,channel?(ColCur-2)/5:0,0,channel?1:8,MAX_ROWS,from,to);
	}
}



void __fastcall TFormMain::ExpandSelection(void)
{
	int col,colc,row,rowc,rowcc;

	if(SelectionHeight<2) return;

	SetUndo();

	if(SelectionRowS<SelectionRowE) RowCur=SelectionRowS+1; else RowCur=SelectionRowE+1;
	if(SelectionColS<SelectionColE) col=SelectionColS; else col=SelectionColE;

	row=RowCur;

	for(rowc=0;rowc<SelectionHeight;++rowc)
	{
		for(colc=0;colc<SelectionWidth;++colc) SongInsertShiftColumn(col+colc,RowCur);

		RowCur+=2;
	}

	if(col<2||((SelectionColS-2)/5)!=((SelectionColE-2)/5))//shift markers and labels
	{
		for(rowc=0;rowc<SelectionHeight;++rowc)
		{
			if(row<MAX_ROWS)
			{
				for(rowcc=MAX_ROWS-1;rowcc>row;--rowcc)
				{
					songList[SongCur].row[rowcc].marker=songList[SongCur].row[rowcc-1].marker;
					songList[SongCur].row[rowcc].name  =songList[SongCur].row[rowcc-1].name;
				}

				songList[SongCur].row[row].marker=false;
				songList[SongCur].row[row].name="";
			}

			row+=2;
		}
	}

	--RowCur;

	CenterView();
	ResetSelection();
	RenderPattern();
}



void __fastcall TFormMain::ShrinkSelection(void)
{
	int col,colc,row,rowc,rowcc;

	if(SelectionHeight<2) return;

	SetUndo();

	if(SelectionRowS<SelectionRowE) RowCur=SelectionRowS+2; else RowCur=SelectionRowE+2;
	if(SelectionColS<SelectionColE) col=SelectionColS; else col=SelectionColE;

	row=RowCur;

	for(rowc=0;rowc<SelectionHeight/2;++rowc)
	{
		for(colc=0;colc<SelectionWidth;++colc) SongDeleteShiftColumn(col+colc,RowCur);

		++RowCur;
	}

	if(col<2||((SelectionColS-2)/5)!=((SelectionColE-2)/5))//shift markers and labels
	{
		for(rowc=0;rowc<SelectionHeight/2;++rowc)
		{
			if(row>0)
			{
				for(rowcc=row;rowcc<MAX_ROWS;++rowcc)
				{
					songList[SongCur].row[rowcc-1].marker=songList[SongCur].row[rowcc].marker;
					songList[SongCur].row[rowcc-1].name  =songList[SongCur].row[rowcc].name;
				}

				songList[SongCur].row[MAX_ROWS-1].marker=false;
				songList[SongCur].row[MAX_ROWS-1].name="";
			}

			++row;
		}
	}

	RowCur-=2;

	RenderPattern();
}



void __fastcall TFormMain::ResetUndo(void)
{
	UndoBuffer.available=false;
}



void __fastcall TFormMain::SetUndo(void)
{
	memcpy(UndoBuffer.row,songList[SongCur].row,sizeof(rowStruct)*MAX_ROWS);

	UndoBuffer.rowCur=RowCur;
	UndoBuffer.colCur=ColCur;
	UndoBuffer.available=true;

	UpdateInfo(true);
}



void __fastcall TFormMain::Undo(void)
{
	static rowStruct temp[MAX_ROWS];
	int r,c;

	if(!UndoBuffer.available) return;

	memcpy(temp,songList[SongCur].row,sizeof(rowStruct)*MAX_ROWS);
	memcpy(songList[SongCur].row,UndoBuffer.row,sizeof(rowStruct)*MAX_ROWS);
	memcpy(UndoBuffer.row,temp,sizeof(rowStruct)*MAX_ROWS);

	r=RowCur;
	c=ColCur;

	RowCur=UndoBuffer.rowCur;
	ColCur=UndoBuffer.colCur;

	UndoBuffer.rowCur=r;
	UndoBuffer.colCur=c;

	CenterView();
	RenderPattern();
}



void __fastcall TFormMain::ResetCopyBuffer(void)
{
	memset(CopyBuffer,0,sizeof(CopyBuffer));

	CopyBufferWidth=0;
	CopyBufferHeight=0;
}



void __fastcall TFormMain::CopyCutToBuffer(bool copy,bool cut,bool shift)
{
	noteFieldStruct *n,*m;
	int col,row,colc,rowc,rows,rowd,chn,chncol;
	bool oneshot;

	if(copy)
	{
		CopyBufferWidth =SelectionWidth;
		CopyBufferHeight=SelectionHeight;
	}

	if(SelectionColS<SelectionColE) col=SelectionColS; else col=SelectionColE;

	if(col<1) return;

	oneshot=true;

	for(colc=0;colc<SelectionWidth;++colc)
	{
		if(SelectionRowS<SelectionRowE) row=SelectionRowS; else row=SelectionRowE;

		if(col==1)
		{
			if(copy) CopyBufferColumnType[colc]=COLUMN_TYPE_SPEED;
		}
		else
		{
			chn=(col-2)/5;
			chncol=(col-2)%5;

			if(copy)
			{
				switch(chncol)
				{
				case 0: CopyBufferColumnType[colc]=COLUMN_TYPE_NOTE;       break;
				case 1: CopyBufferColumnType[colc]=COLUMN_TYPE_INSTRUMENT; break;
				case 2: CopyBufferColumnType[colc]=COLUMN_TYPE_VALUE;      break;
				case 3: CopyBufferColumnType[colc]=COLUMN_TYPE_EFFECT;     break;
				case 4: CopyBufferColumnType[colc]=COLUMN_TYPE_VALUE;      break;
				}
			}
		}

		for(rowc=0;rowc<SelectionHeight;++rowc)
		{
			if(col==1)
			{
				if(copy) CopyBuffer[colc][rowc]=songList[SongCur].row[row].speed;

				if(cut) songList[SongCur].row[row].speed=0;
			}
			else
			{
				n=&songList[SongCur].row[row].chn[chn];

				switch(chncol)
				{
				case 0: if(copy) CopyBuffer[colc][rowc]=n->note;       if(cut) n->note      =0;   break;
				case 1: if(copy) CopyBuffer[colc][rowc]=n->instrument; if(cut) n->instrument=0;   break;
				case 2: if(copy) CopyBuffer[colc][rowc]=n->volume;     if(cut) n->volume    =255; break;
				case 3: if(copy) CopyBuffer[colc][rowc]=n->effect;     if(cut) n->effect    =0;   break;
				case 4: if(copy) CopyBuffer[colc][rowc]=n->value;      if(cut) n->value     =255; break;
				}
			}

			++row;
		}

		if(cut&&shift)
		{
			if(SelectionRowS<SelectionRowE) rowd=SelectionRowS; else rowd=SelectionRowE;

			rows=rowd+SelectionHeight;

			if(oneshot)
			{
				if(songList[SongCur].loop_start>=rows) songList[SongCur].loop_start-=SelectionHeight;

				if(songList[SongCur].length<MAX_ROWS) songList[SongCur].length-=SelectionHeight;
			}

			while(rowd<MAX_ROWS)
			{
				if(oneshot)
				{
					if(rows<MAX_ROWS)
					{
						songList[SongCur].row[rowd].marker=songList[SongCur].row[rows].marker;
						songList[SongCur].row[rowd].name  =songList[SongCur].row[rows].name;
					}
					else
					{
						songList[SongCur].row[rowd].marker=false;
						songList[SongCur].row[rowd].name  ="";
					}
				}

				if(col==1)
				{
					if(rows<MAX_ROWS)
					{
						songList[SongCur].row[rowd].speed=songList[SongCur].row[rows].speed;
					}
					else
					{
						songList[SongCur].row[rowd].speed=0;
					}
				}
				else
				{
					n=&songList[SongCur].row[rowd].chn[chn];

					if(rows<MAX_ROWS)
					{
						m=&songList[SongCur].row[rows].chn[chn];

						switch(chncol)
						{
						case 0: n->note      =m->note;       break;
						case 1: n->instrument=m->instrument; break;
						case 2: n->value     =m->volume;     break;
						case 3: n->effect    =m->effect;     break;
						case 4: n->value     =m->value;      break;
						}
					}
					else
					{
						switch(chncol)
						{
						case 0: n->note      =0;   break;
						case 1: n->instrument=0;   break;
						case 2: n->volume    =255; break;
						case 3: n->effect    =0;   break;
						case 4: n->value     =255; break;
						}
					}
				}

				++rows;
				++rowd;
			}
		}

		++col;

		oneshot=false;
	}

	ResetSelection();
}



void __fastcall TFormMain::PasteFromBuffer(bool shift,bool mix)
{
	noteFieldStruct *n,*m;
	int col,row,colc,rowc,rows,rowd,chn,chncol,len;

	if(!CopyBufferWidth||!CopyBufferHeight||!ColCur) return;

	col=ColCur;

	for(colc=0;colc<CopyBufferWidth;++colc)
	{
		row=RowCur;

		if(col>1)
		{
			chn=(col-2)/5;
			chncol=(col-2)%5;
		}

		if(shift)
		{
			if(SelectionRowS<SelectionRowE) rows=SelectionRowS; else rows=SelectionRowE;

			rowd=rows+CopyBufferHeight;

			len=MAX_ROWS-rowd-1;

			rows+=len;
			rowd+=len;

			while(len>=0)
			{
				if(col==1)
				{
					songList[SongCur].row[rowd].speed=songList[SongCur].row[rows].speed;
				}
				else
				{
					n=&songList[SongCur].row[rowd].chn[chn];
					m=&songList[SongCur].row[rows].chn[chn];

					switch(chncol)
					{
					case 0: n->note      =m->note;       break;
					case 1: n->instrument=m->instrument; break;
					case 2: n->volume    =m->volume;     break;
					case 3: n->effect    =m->effect;     break;
					case 4: n->value     =m->value;      break;
					}
				}

				--rows;
				--rowd;
				--len;
			}
		}

		if(!mix)
		{
			for(rowc=0;rowc<CopyBufferHeight;++rowc)
			{
				if(col==1)
				{
					if(CopyBufferColumnType[colc]==COLUMN_TYPE_SPEED) songList[SongCur].row[row].speed=CopyBuffer[colc][rowc];
				}
				else
				{
					n=&songList[SongCur].row[row].chn[chn];

					switch(chncol)
					{
					case 0: if(CopyBufferColumnType[colc]==COLUMN_TYPE_NOTE)       n->note      =CopyBuffer[colc][rowc]; break;
					case 1: if(CopyBufferColumnType[colc]==COLUMN_TYPE_INSTRUMENT) n->instrument=CopyBuffer[colc][rowc]; break;
					case 2: if(CopyBufferColumnType[colc]==COLUMN_TYPE_VALUE)      n->volume    =CopyBuffer[colc][rowc]; break;
					case 3: if(CopyBufferColumnType[colc]==COLUMN_TYPE_EFFECT)     n->effect    =CopyBuffer[colc][rowc]; break;
					case 4: if(CopyBufferColumnType[colc]==COLUMN_TYPE_VALUE)      n->value     =CopyBuffer[colc][rowc]; break;
					}
				}

				++row;

				if(row>=MAX_ROWS) break;
			}
		}
		else
		{
			for(rowc=0;rowc<CopyBufferHeight;++rowc)
			{
				if(col==1)
				{
					if(CopyBufferColumnType[colc]==COLUMN_TYPE_SPEED)
					{
						if(!songList[SongCur].row[row].speed) songList[SongCur].row[row].speed=CopyBuffer[colc][rowc];
					}
				}
				else
				{
					n=&songList[SongCur].row[row].chn[chn];

					switch(chncol)
					{
					case 0: if(CopyBufferColumnType[colc]==COLUMN_TYPE_NOTE)       if(!n->note       ) n->note      =CopyBuffer[colc][rowc]; break;
					case 1: if(CopyBufferColumnType[colc]==COLUMN_TYPE_INSTRUMENT) if(!n->instrument ) n->instrument=CopyBuffer[colc][rowc]; break;
					case 2: if(CopyBufferColumnType[colc]==COLUMN_TYPE_VALUE)      if( n->volume==255) n->volume    =CopyBuffer[colc][rowc]; break;
					case 3: if(CopyBufferColumnType[colc]==COLUMN_TYPE_EFFECT)     if(!n->effect     ) n->effect    =CopyBuffer[colc][rowc]; break;
					case 4: if(CopyBufferColumnType[colc]==COLUMN_TYPE_VALUE)      if( n->value==255 ) n->value     =CopyBuffer[colc][rowc]; break;
					}
				}

				++row;

				if(row>=MAX_ROWS) break;
			}
		}

		++col;

		if(col>=42) break;
	}
}



void __fastcall TFormMain::ModuleInit(void)
{
	memset(insList,0,sizeof(insList));
	memset(songList,0,sizeof(songList));
}



void __fastcall TFormMain::InstrumentClear(int ins)
{
	if(insList[ins].source) free(insList[ins].source);

	insList[ins].source=NULL;
	insList[ins].source_length=0;
	insList[ins].source_rate=0;
	insList[ins].source_volume=128;

	insList[ins].env_ar=15;
	insList[ins].env_dr=7;
	insList[ins].env_sl=7;
	insList[ins].env_sr=0;

	insList[ins].length=0;

	insList[ins].loop_start=0;
	insList[ins].loop_end=0;
	insList[ins].loop_enable=false;

	insList[ins].wav_loop_start=-1;
	insList[ins].wav_loop_end=-1;

	insList[ins].eq_low=0;
	insList[ins].eq_mid=0;
	insList[ins].eq_high=0;

	insList[ins].resample_type=4;//band
	insList[ins].downsample_factor=0;

	insList[ins].name="none";
}



void __fastcall TFormMain::ModuleClear(void)
{
	int i;

	for(i=0;i<MAX_SONGS;++i) SongDataClear(i);

	SongClear(0);

	for(i=0;i<MAX_INSTRUMENTS;++i) InstrumentClear(i);

	InsChange(0);
	SongChange(0);

	OctaveCur=4;

	for(i=0;i<8;++i) ChannelMute[i]=false;

	UpdateInfo(false);
}



void __fastcall TFormMain::SongDataClear(int song)
{
	int row,chn;

	memset(&songList[song],0,sizeof(songStruct));

	for(row=0;row<MAX_ROWS;++row)
	{
		for(chn=0;chn<8;++chn)
		{
			songList[song].row[row].chn[chn].value =255;
			songList[song].row[row].chn[chn].volume=255;
		}

		songList[song].row[row].name="";
	}

	songList[song].measure=4;
	songList[song].length=MAX_ROWS-1;
	songList[song].name="untitled";
}



void __fastcall TFormMain::SongClear(int song)
{
	SongDataClear(song);

	RowCur=0;
	ColCur=2;
	ColCurPrev=-1;

	ResetNumber=true;

	RenderPattern();

	ResetUndo();
	ResetSelection();
	StartSelection(ColCur,RowCur);

	SelectionWidth=0;
	SelectionHeight=0;

	CenterView();
	UpdateInfo(false);
}



int __fastcall TFormMain::SongCalculateDuration(int song)
{
	int row,n,length,speed,duration;

	length=SongFindLastRow(&songList[song]);

	duration=0;
	speed=DEFAULT_SPEED;

	for(row=0;row<length;++row)
	{
		if(songList[song].row[row].speed) speed=songList[song].row[row].speed;

		duration+=speed;
	}

	if(!duration) return 0;

	if(duration<UPDATE_RATE_HZ) return 1;

	return duration/UPDATE_RATE_HZ;
}



char *gss_text;
int gss_ptr;
int gss_size;



void gss_init(char *text,int size)
{
	gss_text=text;
	gss_size=size;
	gss_ptr=0;
}



int gss_find_tag(AnsiString tag)
{
	char* text;
	int len,size;

	text=gss_text;
	size=gss_size;

	len=strlen(tag.c_str());

	while(size)
	{
		if(!memcmp(&text[gss_ptr],tag.c_str(),len)) return gss_ptr+len;

		++gss_ptr;

		if(gss_ptr>=gss_size) gss_ptr=0;

		--size;
	}

	return -1;
}



AnsiString gss_load_str(AnsiString tag)
{
	static char str[1024];
	char c,*text;
	int ptr,len,size;

	text=gss_text;
	size=gss_size;

	len=strlen(tag.c_str());

	while(size)
	{
		if(!memcmp(&text[gss_ptr],tag.c_str(),len))
		{
			text=&text[gss_ptr+len];

			ptr=0;

			while(size)
			{
				c=*text++;

				if(c<32) break;

				str[ptr++]=c;

				--size;
			}

			str[ptr]=0;

			return str;
		}

		++gss_ptr;

		if(gss_ptr>=gss_size) gss_ptr=0;

		--size;
	}

	return "";
}



int gss_load_int(AnsiString tag)
{
	char c,*text;
	int len,n,size,sign;

	text=gss_text;
	size=gss_size;

	len=strlen(tag.c_str());

	while(size)
	{
		if(!memcmp(&text[gss_ptr],tag.c_str(),len))
		{
			text=&text[gss_ptr+len];

			n=0;
			sign=1;

			while(size)
			{
				c=*text++;

				if(c=='-')
				{
					sign=-1;
					--size;
					continue;
				}

				if(c<'0'||c>'9') break;

				n=n*10+(c-'0');

				--size;
			}

			return n*sign;
		}

		++gss_ptr;

		if(gss_ptr>=gss_size) gss_ptr=0;

		--size;
	}

	return 0;
}



int gss_hex_to_byte(char n)
{
	if(n>='0'&&n<='9') return n-'0';
	if(n>='a'&&n<='f') return n-'a'+10;
	if(n>='A'&&n<='F') return n-'A'+10;

	return -1;
}



short* gss_load_short_data(AnsiString tag)
{
	unsigned short *data,n;
	char *text,c;
	int len,alloc_size,ptr,size;

	text=gss_text;
	size=gss_size;

	alloc_size=65536;

	data=(unsigned short*)malloc(alloc_size);

	ptr=0;

	len=strlen(tag.c_str());

	while(size)
	{
		if(!memcmp(&text[gss_ptr],tag.c_str(),len))
		{
			text=&text[gss_ptr+len];

			while(size)
			{
				if(*text<=32) break;

				n =gss_hex_to_byte(*text++)<<12;
				n|=gss_hex_to_byte(*text++)<<8;
				n|=gss_hex_to_byte(*text++)<<4;
				n|=gss_hex_to_byte(*text++);

				size-=4;

				data[ptr++]=n;

				if(ptr*2>=alloc_size-4)
				{
					alloc_size+=65536;

					data=(unsigned short*)realloc(data,alloc_size);
				}
			}

			data=(unsigned short*)realloc(data,ptr*2);

			return (short*)data;
		}

		++gss_ptr;

		if(gss_ptr>=gss_size) gss_ptr=0;

		--size;
	}

	free(data);

	return NULL;
}



int gss_parse_num_len(char* text,int len,int def)
{
	int c,num;

	num=0;

	while(len)
	{
		c=*text++;

		if(c=='.') return def;

		if(!(c>='0'&&c<='9')) return -1;

		num=num*10+(c-'0');

		--len;
	}

	return num;
}



void __fastcall TFormMain::InstrumentDataParse(int id,int ins)
{
	AnsiString insname;

	insname="Instrument"+IntToStr(id);

	insList[ins].name=gss_load_str(insname+"Name=");

	insList[ins].env_ar=gss_load_int(insname+"EnvAR=");
	insList[ins].env_dr=gss_load_int(insname+"EnvDR=");
	insList[ins].env_sl=gss_load_int(insname+"EnvSL=");
	insList[ins].env_sr=gss_load_int(insname+"EnvSR=");

	insList[ins].length=gss_load_int(insname+"Length=");

	insList[ins].loop_start =gss_load_int(insname+"LoopStart=");
	insList[ins].loop_end   =gss_load_int(insname+"LoopEnd=");
	insList[ins].loop_enable=gss_load_int(insname+"LoopEnable=")?true:false;

	insList[ins].source_length=gss_load_int(insname+"SourceLength=");
	insList[ins].source_rate  =gss_load_int(insname+"SourceRate=");
	insList[ins].source_volume=gss_load_int(insname+"SourceVolume=");

	insList[ins].wav_loop_start=gss_load_int(insname+"WavLoopStart=");
	insList[ins].wav_loop_end  =gss_load_int(insname+"WavLoopEnd=");

	insList[ins].eq_low           =gss_load_int(insname+"EQLow=");
	insList[ins].eq_mid           =gss_load_int(insname+"EQMid=");
	insList[ins].eq_high          =gss_load_int(insname+"EQHigh=");
	insList[ins].resample_type    =gss_load_int(insname+"ResampleType=");
	insList[ins].downsample_factor=gss_load_int(insname+"DownsampleFactor=");
	insList[ins].ramp_enable      =gss_load_int(insname+"RampEnable=")?true:false;
	insList[ins].loop_unroll      =gss_load_int(insname+"LoopUnroll=")?true:false;

	insList[ins].source=gss_load_short_data(insname+"SourceData=");
}



bool __fastcall TFormMain::ModuleOpenFile(AnsiString filename)
{
	FILE *file;
	char *text;
	noteFieldStruct *n;
	int ins,song,row,chn,ptr,size,note;
	AnsiString songname,name;
	bool ex_vol;

	file=fopen(filename.c_str(),"rb");

	if(!file) return false;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	text=(char*)malloc(size+1);

	fread(text,size,1,file);
	fclose(file);

	gss_init(text,size);

	//check signature

	if(gss_find_tag(PROJECT_SIGNATURE)<0)
	{
		free(text);
		return false;
	}

	if(gss_find_tag(EX_VOL_SIGNATURE)<0) ex_vol=false; else ex_vol=true;

	ModuleClear();

	//parse instruments

	for(ins=0;ins<MAX_INSTRUMENTS;++ins) InstrumentDataParse(ins,ins);

	//parse song settings

	for(song=0;song<MAX_SONGS;++song)
	{
		songname="Song"+IntToStr(song);

		songList[song].name      =gss_load_str(songname+"Name=");
		songList[song].length    =gss_load_int(songname+"Length=");
		songList[song].loop_start=gss_load_int(songname+"LoopStart=");
		songList[song].measure   =gss_load_int(songname+"Measure=");
		songList[song].effect    =gss_load_int(songname+"Effect=")?true:false;
	}

	//parse song text

	for(song=0;song<MAX_SONGS;++song)
	{
		ptr=gss_find_tag("[Song"+IntToStr(song)+"]");

		if(ptr<0) continue;

		while(ptr<size) if(text[ptr++]=='\n') break;

		while(1)
		{
			row=gss_parse_num_len(text+ptr,4,0);

			if(row<0) break;

			songList[song].row[row].marker=(text[ptr+4]!=' '?true:false);

			songList[song].row[row].speed=gss_parse_num_len(text+ptr+5,2,0);

			ptr+=7;

			for(chn=0;chn<8;++chn)
			{
				n=&songList[song].row[row].chn[chn];

				if(text[ptr]=='.')
				{
					note=0;
				}
				else
				{
					if(text[ptr]=='-')
					{
						n->note=1;
					}
					else
					{
						switch(text[ptr])
						{
						case 'C': note=0; break;
						case 'D': note=2; break;
						case 'E': note=4; break;
						case 'F': note=5; break;
						case 'G': note=7; break;
						case 'A': note=9; break;
						case 'B': note=11; break;
						default:  note=-1;
						}

						if(text[ptr+1]=='#') ++note;

						n->note=note+(text[ptr+2]-'0')*12+2;
					}
				}

				ptr+=3;

				n->instrument=gss_parse_num_len(text+ptr,2,0);

				ptr+=2;

				if(ex_vol)
				{
					n->volume=gss_parse_num_len(text+ptr,2,255);

					ptr+=2;
				}

				if(text[ptr]=='.') n->effect=0; else n->effect=text[ptr];

				ptr+=1;

				n->value=gss_parse_num_len(text+ptr,2,255);

				if(!ex_vol)
				{
					if(n->effect=='V')//convert old volume command into volume column value
					{
						n->volume=n->value;
						n->effect=0;
						n->value=255;
					}

					if(n->effect=='M') n->effect='V';//modulation has been renamed into vibrato
				}

				ptr+=2;
			}

			name="";

			while(ptr<size)
			{
				if(text[ptr]<' ') break;

				name+=text[ptr];

				++ptr;
			}

			songList[song].row[row].name=name;

			while(ptr<size)
			{
				if(text[ptr]>=' ') break;

				++ptr;
			}
		}
	}

	free(text);

	UpdateSampleData=true;

	ModuleFileName=ExtractFileName(filename);

	return true;
}



void __fastcall TFormMain::UpdateAll(void)
{
	InsListUpdate();
	InsUpdateControls();
	SongUpdateControls();
	SongListUpdate();
	RenderPattern();
	UpdateInfo(false);

	if(PageControlMode->ActivePage==TabSheetInfo) TabSheetInfoShow(this);
}



void __fastcall TFormMain::ModuleOpen(AnsiString filename)
{
	if(ModuleOpenFile(filename))
	{
		SPCCompile(&songList[0],0,false,false,-1);
		CompileAllSongs();
		UpdateAll();

		OpenDialogModule->FileName=filename;
		SaveDialogModule->FileName=filename;
	}
	else
	{
		Application->MessageBox("Can't open module","Error",MB_OK);
	}
}



void __fastcall TFormMain::InstrumentDataWrite(FILE *file,int id,int ins)
{
	int i;

	fprintf(file,"Instrument%iName=%s\n",id,insList[ins].name.c_str());

	fprintf(file,"Instrument%iEnvAR=%i\n",id,insList[ins].env_ar);
	fprintf(file,"Instrument%iEnvDR=%i\n",id,insList[ins].env_dr);
	fprintf(file,"Instrument%iEnvSL=%i\n",id,insList[ins].env_sl);
	fprintf(file,"Instrument%iEnvSR=%i\n",id,insList[ins].env_sr);

	fprintf(file,"Instrument%iLength=%i\n",id,insList[ins].length);

	fprintf(file,"Instrument%iLoopStart=%i\n" ,id,insList[ins].loop_start);
	fprintf(file,"Instrument%iLoopEnd=%i\n"   ,id,insList[ins].loop_end);
	fprintf(file,"Instrument%iLoopEnable=%i\n",id,insList[ins].loop_enable?1:0);

	fprintf(file,"Instrument%iSourceLength=%i\n",id,insList[ins].source_length);
	fprintf(file,"Instrument%iSourceRate=%i\n"  ,id,insList[ins].source_rate);
	fprintf(file,"Instrument%iSourceVolume=%i\n",id,insList[ins].source_volume);

	fprintf(file,"Instrument%iWavLoopStart=%i\n",id,insList[ins].wav_loop_start);
	fprintf(file,"Instrument%iWavLoopEnd=%i\n"  ,id,insList[ins].wav_loop_end);

	fprintf(file,"Instrument%iEQLow=%i\n"           ,id,insList[ins].eq_low);
	fprintf(file,"Instrument%iEQMid=%i\n"           ,id,insList[ins].eq_mid);
	fprintf(file,"Instrument%iEQHigh=%i\n"          ,id,insList[ins].eq_high);
	fprintf(file,"Instrument%iResampleType=%i\n"    ,id,insList[ins].resample_type);
	fprintf(file,"Instrument%iDownsampleFactor=%i\n",id,insList[ins].downsample_factor);
	fprintf(file,"Instrument%iRampEnable=%i\n"      ,id,insList[ins].ramp_enable?1:0);
	fprintf(file,"Instrument%iLoopUnroll=%i\n"      ,id,insList[ins].loop_unroll?1:0);

	fprintf(file,"Instrument%iSourceData=",id);

	if(insList[ins].source)
	{
		for(i=0;i<insList[ins].source_length;++i) fprintf(file,"%4.4x",(unsigned short)insList[ins].source[i]);
	}


	fprintf(file,"\n\n");
}



bool __fastcall TFormMain::SongIsRowEmpty(songStruct *s,int row,bool marker)
{
	noteFieldStruct *n;
	int sum,chn;

	sum=s->row[row].speed;

	if(marker&&s->row[row].marker) ++sum;

	for(chn=0;chn<8;++chn)
	{
		n=&s->row[row].chn[chn];

		sum+=n->note;
		sum+=n->instrument;
		sum+=(n->volume==255?0:1);
		sum+=n->effect;
		sum+=(n->value==255?0:1);
	}

	return sum?false:true;
}



bool __fastcall TFormMain::SongIsEmpty(songStruct *s)
{
	int row;

	for(row=0;row<s->length;++row) if(!SongIsRowEmpty(s,row,true)) return false;

	return true;
}



bool __fastcall TFormMain::ModuleSave(AnsiString filename)
{
	FILE *file;
	noteFieldStruct *n;
	int ins,song,row,chn;

	file=fopen(filename.c_str(),"wt");

	if(!file) return false;

	fprintf(file,"%s\n\n",PROJECT_SIGNATURE);
	fprintf(file,"%s\n\n",EX_VOL_SIGNATURE);

	for(ins=0;ins<MAX_INSTRUMENTS;++ins) InstrumentDataWrite(file,ins,ins);

	for(song=0;song<MAX_SONGS;++song)
	{
		fprintf(file,"Song%iName=%s\n"     ,song,songList[song].name.c_str());
		fprintf(file,"Song%iLength=%i\n"   ,song,songList[song].length);
		fprintf(file,"Song%iLoopStart=%i\n",song,songList[song].loop_start);
		fprintf(file,"Song%iMeasure=%i\n\n",song,songList[song].measure);
		fprintf(file,"Song%iEffect=%i\n\n" ,song,songList[song].effect?1:0);
	}

	for(song=0;song<MAX_SONGS;++song)
	{
		fprintf(file,"[Song%i]\n",song);

		for(row=0;row<MAX_ROWS;++row)
		{
			if(SongIsRowEmpty(&songList[song],row,true)) continue;

			fprintf(file,"%4.4i%c",row,songList[song].row[row].marker?'*':' ');

			if(songList[song].row[row].speed) fprintf(file,"%2.2i",songList[song].row[row].speed); else fprintf(file,"..");

			for(chn=0;chn<8;++chn)
			{
				n=&songList[song].row[row].chn[chn];

				switch(n->note)
				{
				case 0: fprintf(file,"..."); break;
				case 1: fprintf(file,"---"); break;
				default: fprintf(file,"%s%i",NoteNames[(n->note-2)%12].c_str(),(n->note-2)/12);
				}

				if(n->instrument) fprintf(file,"%2.2i",n->instrument); else fprintf(file,"..");

				if(n->volume==255) fprintf(file,".."); else fprintf(file,"%2.2i",n->volume);

				fprintf(file,"%c",n->effect?n->effect:'.');

				if(n->value==255) fprintf(file,".."); else fprintf(file,"%2.2i",n->value);
			}

			fprintf(file,"%s\n",songList[song].row[row].name.c_str());
		}

		fprintf(file,"\n");
	}

	fclose(file);

	return true;
}



AnsiString IntToStrLen(int n,int len)
{
	AnsiString str;

	str="";

	while(len)
	{
		str=IntToStr(n%10)+str;
		n/=10;
		--len;
	}

	return str;
}



AnsiString IntToStrLenDot(int n,int len)
{
	AnsiString str;

	if(!n)
	{
		str="";

		while(len)
		{
			str+=".";
			--len;
		}
	}
	else
	{
		str=IntToStrLen(n,len);
	}

	return str;
}



void __fastcall TFormMain::SongListUpdate(void)
{
	AnsiString str;
	int i;

	for(i=0;i<MAX_SONGS;++i)
	{
		str=IntToStrLen(i+1,2)+": "+songList[i].name;

		if(songList[i].compiled_size>1) str+=" ("+IntToStr(songList[i].compiled_size)+")";

		if(songList[i].effect) str+=" *";

		ListBoxSong->Items->Strings[i]=str;
	}

	if(ListBoxSong->ItemIndex<0) ListBoxSong->ItemIndex=SongCur;
}




void __fastcall TFormMain::MInstrumentItemClick(TObject *Sender)
{
	InsCur=((TMenuItem*)Sender)->Tag;
	ListBoxIns->ItemIndex=InsCur;
	InsChange(InsCur);
	InsListUpdate();
}



void __fastcall TFormMain::InsListUpdate(void)
{
	TMenuItem *NewItem;
	AnsiString item_name,ins_name;
	int i,j,ins,ins_count;

	ins_count=0;

	for(ins=0;ins<MAX_INSTRUMENTS;++ins)
	{
		//create instrument and menu item name

		ins_name=IntToStrLen(ins+1,2)+": "+insList[ins].name;
		item_name="MInstrumentItem"+IntToStr(ins);

		//find and delete main menu entries with this name

		for(j=0;j<MInstruments->Count;++j)
		{
			if(MInstruments->Items[j]->Name==item_name)
			{
				delete MInstruments->Items[j];
				break;
			}
		}

		//add menu item

		if(insList[ins].source)
		{
			NewItem = new TMenuItem(this); // The owner will cleanup these menu items.
			NewItem->Caption = ins_name;
			NewItem->Name = item_name;
			NewItem->Tag=ins;
			NewItem->OnClick=MInstrumentItemClick;

			MInstruments->Add(NewItem);

			++ins_count;
		}

		//update list box entry

		ListBoxIns->Items->Strings[ins]=ins_name;
	}

	//try to find the 'no instruments' menu item and delete it

	for(j=0;j<MInstruments->Count;++j)
	{
		if(MInstruments->Items[j]->Name=="MInstrumentNone")
		{
			delete MInstruments->Items[j];
			break;
		}
	}

	//add the item back if there is no instruments indeed

	if(!ins_count)
	{
		NewItem = new TMenuItem(this); // The owner will cleanup these menu items.
		NewItem->Caption = "no instruments";
		NewItem->Name = "MInstrumentNone";
		NewItem->Enabled=false;

		MInstruments->Add(NewItem);
	}

	if(ListBoxIns->ItemIndex<0) ListBoxIns->ItemIndex=InsCur;

	//scan through instrument items and make currently selected checked

	for(j=0;j<MInstruments->Count;++j)
	{
		if(MInstruments->Items[j]->Name.SubString(0,15)=="MInstrumentItem")
		{
			MInstruments->Items[j]->Checked=(MInstruments->Items[j]->Tag==InsCur?true:false);
		}
	}
}



void __fastcall TFormMain::SongUpdateControls(void)
{
	EditSongName->Text=songList[SongCur].name;

	CheckBoxEffect->Checked=songList[SongCur].effect;
}



void __fastcall TFormMain::UpdateEQ(void)
{
	StaticTextEQLow ->Caption=IntToStr(insList[InsCur].eq_low);
	StaticTextEQMid ->Caption=IntToStr(insList[InsCur].eq_mid);
	StaticTextEQHigh->Caption=IntToStr(insList[InsCur].eq_high);
}



int __fastcall TFormMain::InsCalculateBRRSize(int ins,bool loop_only)
{
	int i,brr_len,brr_loop_len,div,loop_size;
	int loop_cnt,new_loop_start;
	int loop_start,loop_end;

	if(!insList[ins].source) return 0;

	switch(insList[ins].downsample_factor)
	{
	case 1:  div=2; break;
	case 2:  div=4; break;
	default: div=1;
	}

	if(!insList[ins].loop_enable)
	{
		brr_len=(insList[ins].length/div+15)/16*9;

		brr_loop_len=0;
	}
	else
	{
		loop_start= insList[ins].loop_start /div;
		loop_end  =(insList[ins].loop_end+1)/div;

		loop_size=loop_end-loop_start;

		if(!insList[ins].loop_unroll)
		{
			brr_loop_len=loop_size/16*9;

			brr_len=(loop_start+15)/16*9+brr_loop_len;

			if((loop_size&15)>=8) brr_len+=9;
		}
		else
		{
			new_loop_start=(loop_start+15)/16*16;

			loop_cnt=new_loop_start;

			while(1)
			{
				loop_cnt+=loop_size;

				if(!(loop_cnt&15)||loop_cnt>=65536) break;
			}

			brr_loop_len=(loop_cnt-new_loop_start)/16*9;

			brr_len=loop_cnt/16*9;
		}
	}

	for(i=0;i<16;++i)
	{
		if(insList[ins].source[i]!=0)
		{
			brr_len+=9;
			break;
		}
	}

	return loop_only?brr_loop_len:brr_len;
}



void __fastcall TFormMain::InsUpdateControls(void)
{
	AnsiString strWav,strBrr;
	int div,len;

	EditInsName->Text=insList[InsCur].name;

	strWav="Source: ";
	strBrr="BRR: ";

	if(!insList[InsCur].source)
	{
		strWav+="---";
		strBrr+="---";
	}
	else
	{
		switch(insList[InsCur].downsample_factor)
		{
		case 1:  div=2; break;
		case 2:  div=4; break;
		default: div=1;
		}

		strWav+=IntToStr(insList[InsCur].source_rate)+" Hz, "+IntToStr(insList[InsCur].source_length)+" samples";

		strBrr+=IntToStr(insList[InsCur].source_rate/div)+" Hz, ";

		len=InsCalculateBRRSize(InsCur,false);

		strBrr+=IntToStr(len)+" bytes";

		if(insList[InsCur].loop_enable) strBrr+=" ("+IntToStr(InsCalculateBRRSize(InsCur,true))+" looped)";

		strBrr+=" ~"+IntToStr((int)(100.0f/65536.0f*float(len)))+"%";
	}

	if(insList[InsCur].wav_loop_start>=0&&(insList[InsCur].wav_loop_start!=0||insList[InsCur].wav_loop_end!=0))
	{
		strWav+=" (loop "+IntToStr(insList[InsCur].wav_loop_start)+" to "+insList[InsCur].wav_loop_end+")";
	}
	else
	{
		strWav+=" (no loop)";
	}

	LabelWavInfo->Caption=strWav;
	LabelBRRInfo->Caption=strBrr;

	LabelAR->Caption="AR: "+IntToStr(insList[InsCur].env_ar);
	LabelDR->Caption="DR: "+IntToStr(insList[InsCur].env_dr);
	LabelSL->Caption="SL: "+IntToStr(insList[InsCur].env_sl);
	LabelSR->Caption="SR: "+IntToStr(insList[InsCur].env_sr);

	LabelVolume->Caption="Vol: "+IntToStr(insList[InsCur].source_volume);

	TrackBarAR->Position=insList[InsCur].env_ar;
	TrackBarDR->Position=insList[InsCur].env_dr;
	TrackBarSL->Position=insList[InsCur].env_sl;
	TrackBarSR->Position=insList[InsCur].env_sr;

	TrackBarVolume->Position=insList[InsCur].source_volume;

	SpeedButtonLoop      ->Down=insList[InsCur].loop_enable;
	SpeedButtonLoopRamp  ->Down=insList[InsCur].ramp_enable;
	SpeedButtonLoopUnroll->Down=insList[InsCur].loop_unroll;

	EditLength->Text=IntToStr(insList[InsCur].length);

	EditLoopStart->Text=IntToStr(insList[InsCur].loop_start);
	EditLoopEnd  ->Text=IntToStr(insList[InsCur].loop_end);

	switch(insList[InsCur].resample_type)
	{
	case 0: SpeedButtonResampleNearest->Down=true; break;
	case 1: SpeedButtonResampleLinear ->Down=true; break;
	case 2: SpeedButtonResampleCubic  ->Down=true; break;
	case 3: SpeedButtonResampleSine   ->Down=true; break;
	case 4: SpeedButtonResampleBand   ->Down=true; break;
	}

	switch(insList[InsCur].downsample_factor)
	{
	case 0: SpeedButtonDownsample1->Down=true; break;
	case 1: SpeedButtonDownsample2->Down=true; break;
	case 2: SpeedButtonDownsample4->Down=true; break;
	}

	RenderADSR();

	UpdateEQ();
}



void __fastcall TFormMain::UpdateInfo(bool header_only)
{
	int duration;
	AnsiString dstr;

	duration=SongCalculateDuration(SongCur);

	dstr=IntToStr(duration/60)+"m"+Format("%2.2ds",ARRAYOFCONST((duration%60)));

	Caption=AnsiString(VERSION_STR)+" ["+ModuleFileName+"] [Song "+IntToStr(SongCur+1)+": "+songList[SongCur].name+"] ["+dstr+"]";

	if(header_only) return;

	MOctave->Caption="Octave:"+IntToStr(OctaveCur);
	MAutostep->Caption="Autostep:"+IntToStr(AutoStep);

	MOctave1->Checked=(OctaveCur==1)?true:false;
	MOctave2->Checked=(OctaveCur==2)?true:false;
	MOctave3->Checked=(OctaveCur==3)?true:false;
	MOctave4->Checked=(OctaveCur==4)?true:false;
	MOctave5->Checked=(OctaveCur==5)?true:false;
	MOctave6->Checked=(OctaveCur==6)?true:false;
	MOctave7->Checked=(OctaveCur==7)?true:false;
	MOctave8->Checked=(OctaveCur==8)?true:false;
}



void __fastcall TFormMain::InsChange(int ins)
{
	ListBoxIns->ItemIndex=ins;

	InsCur=ins;

	InsUpdateControls();
}



void __fastcall TFormMain::SongChange(int song)
{
	ListBoxSong->ItemIndex=song;

	SongCur=song;

	SongUpdateControls();
	SongListUpdate();
	ResetUndo();

	RenderPattern();

	if(PageControlMode->ActivePage==TabSheetInfo) TabSheetInfoShow(this);

	UpdateInfo(true);
}



void __fastcall TFormMain::SwapInstrumentNumberAll(int ins1,int ins2)
{
	noteFieldStruct *n;
	int song,row,chn;

	for(song=0;song<MAX_SONGS;++song)
	{
		for(row=0;row<MAX_ROWS;++row)
		{
			for(chn=0;chn<8;++chn)
			{
				n=&songList[song].row[row].chn[chn];

				if(n->instrument==ins1) n->instrument=ins2; else if(n->instrument==ins2) n->instrument=ins1;
			}
		}
	}

	RenderPattern();
}



void __fastcall TFormMain::RenderADSR(void)
{
	const float attackTime[16]={4100,2600,1500,1000,640,380,260,160,96,64,40,24,16,10,6,0};
	const float decayTime[8]={1200,740,440,290,180,110,74,37};
	const float sustainLevel[8]={1.0f/8.0f,2.0f/8.0f,3.0f/8.0f,4.0f/8.0f,5.0f/8.0f,6.0f/8.0f,7.0f/8.0f,1.0f};
	const float sustainTime[32]={-1,38000,28000,24000,19000,14000,12000,9400,7100,5900,4700,3500,2900,2400,1800,1500,1200,880,740,590,440,370,290,220,180,150,110,92,74,55,37,18};
	const float pps=400;

	TCanvas *c;
	unsigned char *data;
	float x,y,w,h,ax,dx,dy,sx,level,off,step,vol;
	bool loops,loope;
	int i,cnt;

	c=PaintBoxADSR->Canvas;

	w=(float)PaintBoxADSR->Width;
	h=(float)PaintBoxADSR->Height;

	ax=pps*(attackTime[insList[InsCur].env_ar]/1000.0f);
	dx=ax+pps*(decayTime[insList[InsCur].env_dr]/1000.0f);
	dy=(h-1)-(h-2)*sustainLevel[insList[InsCur].env_sl];
	sx=dx+pps*(sustainTime[insList[InsCur].env_sr]/1000.0f);

	loops=false;
	loope=false;

	c->Lock();
	c->Pen->Width=1;
	c->Brush->Color=clBlack;
	c->FillRect(TRect(0,0,(int)w,(int)h));

	w-=4;
	h-=4;

	vol=get_volume_scale(insList[InsCur].source_volume);

	x=1;
	cnt=0;

	while(x<w-1)//draw .25s and 1s marks
	{
		c->Pen->Color=!(cnt&3)?clSilver:TColor(0x303030);
		c->PenPos=TPoint((int)x+2,2);
		c->LineTo((int)x+2,(int)h);

		x+=pps/4.0f;
		++cnt;
	}

	if(insList[InsCur].source)
	{
		off=0;
		step=((float)insList[InsCur].source_rate)/pps;

		for(x=1;x<w-1;x+=1.0f)
		{
			if(!insList[InsCur].loop_enable&&off>=insList[InsCur].length)
			{
				level=0;
			}
			else
			{
				level=(h-2)/32768.0f*(float)abs(insList[InsCur].source[(int)off])*vol;
			}

			y=h/2-level/2;

			off+=step;

			c->Pen->Color=clGreen;
			c->PenPos=TPoint((int)x+2,(int)y+2);
			c->LineTo((int)x+2,(int)y+level+2);

			if(off>=insList[InsCur].loop_start)//draw loop start line
			{
				if(!loops&&insList[InsCur].loop_enable)
				{
					loops=true;

					c->Pen->Color=clRed;
					c->PenPos=TPoint((int)x+2,2);
					c->LineTo((int)x+2,(int)h);
				}
			}

			if(off>=insList[InsCur].length||off>=insList[InsCur].source_length||(insList[InsCur].loop_enable&&off>=insList[InsCur].loop_end))//draw loop end line, wrap up the waveform
			{
				if(insList[InsCur].loop_enable) off=insList[InsCur].loop_start; else off=insList[InsCur].length;

				if(!loope&&insList[InsCur].loop_enable)
				{
					loope=true;

					c->Pen->Color=clBlue;
					c->PenPos=TPoint((int)x+2,2);
					c->LineTo((int)x+2,(int)h);
				}
			}
		}
	}

	c->Pen->Color=clLime;
	c->Pen->Width=2;

	c->PenPos=TPoint(1+2,(int)h-1);
	c->LineTo((int)ax+2,2);
	c->LineTo((int)dx+2,(int)dy+2);

	if(sustainTime[insList[InsCur].env_sr]>=0)
	{
		c->LineTo((int)sx+2,(int)h-1);
	}
	else
	{
		c->LineTo((int)w-1,(int)dy+2);
	}

	c->Unlock();
}



int __fastcall TFormMain::SamplesCompile(int adr,int one_sample)
{
	int i,ins,size,dir_ptr,adsr_ptr,sample_adr,sample_loop,ins_count,prev,match;

	InstrumentsCount=0;

	if(one_sample<0)
	{
		for(ins=0;ins<MAX_INSTRUMENTS;++ins)
		{
			if(insList[ins].source&&insList[ins].length>=16)
			{
				InstrumentRenumberList[ins]=InstrumentsCount;//new instrument number

				++InstrumentsCount;
			}
			else
			{
				InstrumentRenumberList[ins]=-1;//no sample in this slot
			}
		}
	}
	else
	{
		for(ins=0;ins<MAX_INSTRUMENTS;++ins) InstrumentRenumberList[ins]=-1;

		InstrumentRenumberList[one_sample]=0;//new instrument number

		InstrumentsCount=1;
	}

	dir_ptr=adr;
	adr+=4*InstrumentsCount;//dir size

	adsr_ptr=adr;
	adr+=2*InstrumentsCount;//adsr list size

	size=adr-dir_ptr;

	for(ins=0;ins<MAX_INSTRUMENTS;++ins)
	{
		if(InstrumentRenumberList[ins]<0) continue;

		BRREncode(ins);

		sample_adr=adr;

		match=-1;

		if(one_sample<0)
		{
			for(prev=0;prev<ins;++prev)
			{
				if(insList[prev].BRR_size==BRRTempSize)
				{
					if(!memcmp(&SPCMem[insList[prev].BRR_adr],BRRTemp,BRRTempSize))
					{
						match=prev;
						break;
					}
				}
			}
		}

		if(match<0)//new sample
		{
			insList[ins].BRR_adr=sample_adr;
			insList[ins].BRR_size=BRRTempSize;

			for(i=0;i<BRRTempSize;++i)
			{
				SPCMem[adr]=BRRTemp[i];

				++adr;

				if(adr>=sizeof(SPCMem)) return -1;
			}

			if(BRRTempLoop>0) sample_loop=sample_adr+BRRTempLoop*9; else sample_loop=sample_adr;

			SPCMem[dir_ptr+0]=sample_adr&255;
			SPCMem[dir_ptr+1]=sample_adr>>8;
			SPCMem[dir_ptr+2]=sample_loop&255;
			SPCMem[dir_ptr+3]=sample_loop>>8;

			size+=BRRTempSize;
		}
		else//duplicated sample, just copy the parameters
		{
			if(BRRTempLoop>0) sample_loop=insList[match].BRR_adr+BRRTempLoop*9; else sample_loop=insList[match].BRR_adr;

			SPCMem[dir_ptr+0]=insList[match].BRR_adr&255;
			SPCMem[dir_ptr+1]=insList[match].BRR_adr>>8;
			SPCMem[dir_ptr+2]=sample_loop&255;
			SPCMem[dir_ptr+3]=sample_loop>>8;
		}

		dir_ptr+=4;

		SPCMem[adsr_ptr+0]=0x80|insList[ins].env_ar|(insList[ins].env_dr<<4);
		SPCMem[adsr_ptr+1]=insList[ins].env_sr|(insList[ins].env_sl<<5);

		adsr_ptr+=2;
	}

	return size;
}




bool __fastcall TFormMain::SongIsChannelEmpty(songStruct *s,int chn)
{
	int row;

	for(row=0;row<s->length;++row) if(s->row[row].chn[chn].note) return false;

	return true;
}



int __fastcall TFormMain::SongFindLastRow(songStruct *s)
{
	int row;

	for(row=MAX_ROWS-1;row>=0;--row)
	{
		if(!SongIsRowEmpty(s,row,false)) return row;
	}

	return 0;
}



void __fastcall TFormMain::SongCleanUp(songStruct *s)
{
	noteFieldStruct *n;
	int row,chn,song_length,prev_ins,prev_vol;

	song_length=SongFindLastRow(s);

	for(chn=0;chn<8;++chn)
	{
		prev_ins=-1;
		prev_vol=-1;

		for(row=0;row<song_length;++row)
		{
			n=&s->row[row].chn[chn];

			if(n->instrument)
			{
				if(n->instrument!=prev_ins) prev_ins=n->instrument; else n->instrument=0;
			}

			if(n->volume!=prev_vol)
			{
				if(n->volume!=255) prev_vol=n->volume;
			}
			else
			{
				n->volume=255;
			}

			if(row==s->loop_start&&s->loop_start>0)//set instrument and volume at the loop point, just in case
			{
				if(!n->instrument&&prev_ins>0) n->instrument=prev_ins;

				if(prev_vol>=0) n->volume=prev_vol;
			}
		}
	}
}



int __fastcall TFormMain::DelayCompile(int adr,int delay)
{
	const int max_short_wait=148;//max duration of the one-byte delay

	while(delay)
	{
		if(delay<max_short_wait*2)//can be encoded as one or two one-byte delays
		{
			if(delay>=max_short_wait)
			{
				SPCChnMem[adr++]=max_short_wait;

				delay-=max_short_wait;
			}
			else
			{
				SPCChnMem[adr++]=delay;

				delay=0;
			}
		}
		else//long delay, encoded with 3 bytes
		{
			if(delay>=65535)
			{
				SPCChnMem[adr++]=246;
				SPCChnMem[adr++]=255;
				SPCChnMem[adr++]=255;

				delay-=65535;
			}
			else
			{
				SPCChnMem[adr++]=246;
				SPCChnMem[adr++]=delay&255;
				SPCChnMem[adr++]=delay>>8;

				delay=0;
			}
		}
	}

	return adr;
}



int __fastcall TFormMain::ChannelCompile(songStruct *s,int chn,int start_row,int compile_adr,int &play_adr)
{
	const int keyoff_gap_duration=2;//number of frames between keyoff and keyon, to prevent clicks
	noteFieldStruct *n;
	int row,adr,ins,speed,speed_acc,loop_adr,ins_change,effect,value,loop_start,length,size,ins_last;
	int new_play_adr,ref_len,best_ref_len,new_size,min_size,effect_volume;
	bool change,insert_gap,key_is_on,porta_active,new_note_keyoff;

	play_adr=0;
	loop_adr=0;
	adr     =0;

	speed=20;
	speed_acc=0;
	ins_change=0;
	ins_last=1;
	insert_gap=false;
	effect=0;
	effect_volume=255;
	value=0;
	key_is_on=false;
	porta_active=false;

	length    =s->length;
	loop_start=s->loop_start;

	if(!loop_start&&length==MAX_ROWS-1)//if loop markers are in the default positions, loop the song on the next to the last non-empty row, making it seem to be not looped
	{
		length=SongFindLastRow(s)+2;
		loop_start=length-1;
	}

	for(row=0;row<length;++row)
	{
		change=false;

		if(row==loop_start||row==start_row)//break the ongoing delays at the start and loop points
		{
			adr=DelayCompile(adr,speed_acc);

			speed_acc=0;
		}

		if(row==start_row)
		{
			play_adr=adr;

			if(row)//force instrument change at starting row in the middle of the song
			{
				ins_change=ins_last;
				change=true;
			}
		}

		if(row==loop_start) loop_adr=adr;

		if(s->row[row].speed) speed=s->row[row].speed;

		n=&s->row[row].chn[chn];

		if(n->instrument)
		{
			ins_change=n->instrument;

			ins_last=ins_change;
		}

		if(n->effect&&n->value!=255)
		{
			effect=n->effect;
			value =n->value;

			change=true;
		}

		if(n->volume!=255)
		{
			effect_volume=n->volume;

			change=true;
		}

		if(n->note) change=true;

		if(change)
		{
			if(effect=='P') porta_active=(value?true:false);//detect portamento early to prevent inserting the keyoff gap

			if(n->note>1&&key_is_on&&!porta_active&&speed_acc>=keyoff_gap_duration)//if it is a note (not keyoff) and there is enough empty space, insert keyoff with a gap before keyon
			{
				speed_acc-=keyoff_gap_duration;

				insert_gap=true;
			}
			else
			{
				insert_gap=false;
			}

			adr=DelayCompile(adr,speed_acc);

			speed_acc=0;

			if(n->note==1)
			{
				SPCChnMem[adr++]=245;//keyoff

				key_is_on=false;
			}

			if(effect=='T')//detune
			{
				SPCChnMem[adr++]=249;
				SPCChnMem[adr++]=value;

				effect=0;
			}

			if(effect=='D')//slide down
			{
				SPCChnMem[adr++]=250;
				SPCChnMem[adr++]=-value;

				effect=0;
			}

			if(effect=='U')//slide up
			{
				SPCChnMem[adr++]=250;
				SPCChnMem[adr++]=value;

				effect=0;
			}

			if(effect=='P')//portamento
			{
				SPCChnMem[adr++]=251;
				SPCChnMem[adr++]=value;

				effect=0;

				//porta_active=(value?true:false);
			}

			if(effect=='V')//vibrato
			{
				SPCChnMem[adr++]=252;
				SPCChnMem[adr++]=(value/10%10)|((value%10)<<4);//rearrange decimal digits into hex nibbles, in reversed order

				effect=0;
			}

			if(effect=='S')//pan
			{
				SPCChnMem[adr++]=248;
				SPCChnMem[adr++]=(value*256/100);//recalculate 0..50..99 to 0..128..255

				effect=0;
			}

			new_note_keyoff=(key_is_on&&insert_gap&&!porta_active)?true:false;

			if((n->note<2&&!new_note_keyoff)&&effect_volume!=255)//volume, only insert it here when there is no new note with preceding keyoff, otherwise insert it after keyoff
			{
				SPCChnMem[adr++]=247;
				SPCChnMem[adr++]=((float)effect_volume*1.29f);//rescale 0..99 to 0..127

				effect_volume=255;
			}

			if(n->note>1)
			{
				if(new_note_keyoff)
				{
					SPCChnMem[adr++]=245;//keyoff

					adr=DelayCompile(adr,keyoff_gap_duration);

					key_is_on=false;
				}

				if(ins_change)
				{
					SPCChnMem[adr++]=254;

					ins=InstrumentRenumberList[ins_change-1];

					if(ins<0) ins=8; else ins+=9;//first 9 entries of the sample dir reserved for the BRR streamer, sample 8 is the sync same, it is always empty

					SPCChnMem[adr++]=ins;

					ins_change=0;
				}

				if(effect_volume!=255)//volume, inserted after keyoff to prevent clicks
				{
					SPCChnMem[adr++]=247;
					SPCChnMem[adr++]=((float)effect_volume*1.29f);//rescale 0..99 to 0..127

					effect_volume=255;
				}

				SPCChnMem[adr++]=(n->note-2)+150;

				key_is_on=true;
			}
		}

		speed_acc+=speed;
	}

	n=&s->row[loop_start].chn[chn];

	if(n->note>1) speed_acc-=keyoff_gap_duration;//if there is a new note at the loop point, prevent click by inserting a keyoff with gap just at the end of the channel

	adr=DelayCompile(adr,speed_acc);

	if(n->note>1)
	{
		SPCChnMem[adr++]=245;//keyoff

		adr=DelayCompile(adr,keyoff_gap_duration);
	}

	play_adr+=compile_adr;
	loop_adr+=compile_adr;

	SPCChnMem[adr++]=255;
	SPCChnMem[adr++]=loop_adr&255;
	SPCChnMem[adr++]=loop_adr>>8;

	size=adr;



#ifndef ENABLE_SONG_COMPRESSION

	memcpy(&SPCMem[compile_adr],SPCChnMem,size);

#else

	min_size=65536;
	best_ref_len=64;

	for(ref_len=5;ref_len<=64;++ref_len)
	{
		new_play_adr=play_adr;

		new_size=ChannelCompress(compile_adr,new_play_adr,loop_adr,size,ref_len);

		if(new_size<min_size)
		{
			min_size=new_size;
			best_ref_len=ref_len;
		}
	}

	size=ChannelCompress(compile_adr,play_adr,loop_adr,size,best_ref_len);

	memcpy(&SPCMem[compile_adr],SPCChnPack,size);

#endif

	return size;
}



void __fastcall TFormMain::ChannelCompressFlush(int compile_adr)
{
	int i,ref_len,ref_off;

	ref_len=CompressSeqPtr;
	CompressSeqPtr=0;

	if(!ref_len) return;

	ref_off=-1;

	for(i=0;i<CompressOutPtr-ref_len;++i)
	{
		if(!memcmp(&SPCChnPack[i],CompressSeqBuf,ref_len))
		{
			ref_off=compile_adr+i;
			break;
		}
	}

	if(ref_off<0)
	{
		memcpy(&SPCChnPack[CompressOutPtr],CompressSeqBuf,ref_len);

		CompressOutPtr+=ref_len;
	}
	else
	{
		SPCChnPack[CompressOutPtr++]=253;
		SPCChnPack[CompressOutPtr++]=ref_off&255;
		SPCChnPack[CompressOutPtr++]=ref_off>>8;
		SPCChnPack[CompressOutPtr++]=ref_len;
	}
}



int __fastcall TFormMain::ChannelCompress(int compile_adr,int &play_adr,int loop_adr,int src_size,int ref_max)
{
	int i,tag,len,src_ptr,new_loop_adr,new_play_adr;

	memset(SPCChnPack,0,sizeof(SPCChnPack));

	CompressSeqPtr=0;
	CompressOutPtr=0;

	src_ptr=0;

	new_loop_adr=-1;
	new_play_adr=-1;

	while(src_ptr<src_size)
	{
		if(new_loop_adr<0)
		{
			if(src_ptr==loop_adr-compile_adr)
			{
				ChannelCompressFlush(compile_adr);

				new_loop_adr=compile_adr+CompressOutPtr;
			}
		}

		if(new_play_adr<0)
		{
			if(src_ptr==play_adr-compile_adr)
			{
				ChannelCompressFlush(compile_adr);

				new_play_adr=compile_adr+CompressOutPtr;
			}
		}

		tag=SPCChnMem[src_ptr];

		switch(tag)
		{
		case 247:
		case 248:
		case 249:
		case 250:
		case 251:
		case 252:
		case 254: len=2; break;
		case 253: len=4; break;
		case 246:
		case 255: len=3; break;
		default:  len=1;
		}

		for(i=0;i<len;++i) CompressSeqBuf[CompressSeqPtr++]=SPCChnMem[src_ptr++];

		if(CompressSeqPtr>=ref_max) ChannelCompressFlush(compile_adr);
	}

	ChannelCompressFlush(compile_adr);

	if(new_play_adr<0) new_play_adr=compile_adr;
	if(new_loop_adr<0) new_loop_adr=compile_adr;

	SPCChnPack[CompressOutPtr-2]=new_loop_adr&255;
	SPCChnPack[CompressOutPtr-1]=new_loop_adr>>8;

	play_adr=new_play_adr;

	return CompressOutPtr;//compressed data size
}



int __fastcall TFormMain::SongCompile(songStruct *s_original,int start_row,int start_adr,bool mute)
{
	static songStruct s;
	noteFieldStruct *n,*m;
	int row,chn,adr,channels_all,channels_off,chn_size,play_adr,section_start,section_end,repeat_row;
	bool active[8],find_ins,find_vol,repeat;

	s_original->compiled_size=0;

	if(start_row>=s_original->length) return 0;//don't compile and play song if starting position is outside the song

	memcpy(&s,s_original,sizeof(songStruct));

	SongCleanUp(&s);//cleanup instrument numbers and volume effect

	//expand repeating sections

	for(chn=0;chn<8;++chn)
	{
		section_start=0;
		section_end=0;

		repeat=false;
		repeat_row=0;

		for(row=0;row<MAX_ROWS;++row)
		{
			if(s.row[row].marker)
			{
				section_start=section_end;
				section_end=row;
				repeat=false;
			}

			n=&s.row[row].chn[chn];

			if(n->effect=='R')
			{
				repeat=true;
				repeat_row=section_start;
			}

			if(repeat)
			{
				m=&s.row[repeat_row].chn[chn];

				n->note=m->note;
				n->instrument=m->instrument;

				if(m->effect!='R') n->effect=m->effect; else n->effect=0;

				n->value=m->value;

				++repeat_row;

				if(repeat_row>=section_end) repeat_row=section_start;
			}
		}
	}

	//modify current row if the song needs to be played from the middle

	if(start_row>0)
	{
		for(chn=0;chn<8;++chn)
		{
			n=&s.row[start_row].chn[chn];

			find_ins=true;
			find_vol=true;

			for(row=start_row-1;row>=0;--row)
			{
				if(!find_ins&&!find_vol) break;

				m=&s.row[row].chn[chn];

				if(find_ins&&m->instrument)
				{
					if(!n->instrument) n->instrument=m->instrument;

					find_ins=false;
				}

				if(find_vol&&m->volume!=255)
				{
					if(n->volume==255) n->volume=m->volume;

					find_vol=false;
				}
			}
		}
	}

	//count active channels

	channels_all=0;

	for(chn=0;chn<8;++chn)
	{
		if(SongIsChannelEmpty(&s,chn)||(ChannelMute[chn]&&mute))
		{
			active[chn]=false;
		}
		else
		{
			active[chn]=true;

			++channels_all;
		}
	}

	//set default instrument number

	for(chn=0;chn<8;++chn)
	{
		for(row=0;row<MAX_ROWS;++row)
		{
			n=&s.row[row].chn[chn];

			if(n->note>1)
			{
				if(!n->instrument) n->instrument=1;

				break;
			}
		}
	}

	//compile channels

	adr=start_adr+1+channels_all*2;

	channels_off=start_adr+1;

	for(chn=0;chn<8;++chn)
	{
		if(!active[chn]) continue;

		chn_size=ChannelCompile(&s,chn,start_row,adr,play_adr);//play_adr gets changed according to the start_row

		SPCMem[channels_off+0]=play_adr&255;
		SPCMem[channels_off+1]=play_adr>>8;

		channels_off+=2;

		adr+=chn_size;
	}

	SPCMem[start_adr]=channels_all;

	s_original->compiled_size=adr-start_adr;

	return s_original->compiled_size;
}



int __fastcall TFormMain::EffectsCompile(int start_adr)
{
	int adr,song,size,effects_all,effects_off;

	effects_all=0;

	for(song=0;song<MAX_SONGS;++song) if(songList[song].effect) ++effects_all;

	adr=start_adr+1+effects_all*2;

	SPCMem[start_adr]=effects_all;

	effects_off=start_adr+1;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(!songList[song].effect) continue;

		if(SongIsEmpty(&songList[song])) continue;

		SPCMem[effects_off+0]=adr&255;
		SPCMem[effects_off+1]=adr>>8;

		effects_off+=2;

		size=SongCompile(&songList[song],0,adr,false);

		if(size<0) return -1;

		adr+=size;
	}

	return adr-start_adr;
}



bool __fastcall TFormMain::SPCCompile(songStruct *s,int start_row,bool mute,bool effects,int one_sample)
{
	const int header_size=2;
	int ptr,code_adr,sample_adr,adsr_adr,music_adr,effects_adr;

	SPCMusicSize=0;
	SPCEffectsSize=0;

	//compile SPC700 RAM snapshot

	code_adr=0x200;//start address for the driver in the SPC700 RAM

	memcpy(&SPCMem[code_adr],spc700_data+header_size,spc700_size-header_size);//SPC700 driver code, header_size is for the extra header used by sneslib

	sample_adr=code_adr+spc700_size-header_size;

	if(UpdateSampleData||one_sample>=0)//only recompile samples when needed
	{
		memset(&SPCMem[sample_adr],0,65536-sample_adr);

		if(one_sample<0) UpdateSampleData=false; else UpdateSampleData=true;

		SPCInstrumentsSize=0;

		SPCInstrumentsSize=SamplesCompile(sample_adr,one_sample);//puts the dir, adsr list, and sample data directly into the RAM snapshot

		if(SPCInstrumentsSize<0)
		{
			Application->MessageBox("SPC700 memory overflow, too much sample data","Error",MB_OK);
			return false;
		}
	}

	adsr_adr=sample_adr+4*InstrumentsCount;

	effects_adr=sample_adr+SPCInstrumentsSize;

	if(effects)
	{
		SPCEffectsSize=EffectsCompile(effects_adr);

		if(SPCEffectsSize<0)
		{
			Application->MessageBox("SPC700 memory overflow, too much sound effects data","Error",MB_OK);
			return false;
		}
	}

	music_adr=effects_adr+SPCEffectsSize;

	SPCMusicSize=SongCompile(s,start_row,music_adr,mute);

	if(SPCMusicSize<0)
	{
		Application->MessageBox("SPC700 memory overflow, too much music data","Error",MB_OK);
		return false;
	}

	if(!effects)//if the effects aren't compiled, it is not for export
	{
		SPCMem[0x204]=0;//skip the bra mainLoopInit, to run preinitialized version of the driver
		SPCMem[0x205]=0;
	}

	SPCMem[0x208]=(adsr_adr-9*2)&255;
	SPCMem[0x209]=(adsr_adr-9*2)>>8;

	SPCMem[0x20a]=effects_adr&255;
	SPCMem[0x20b]=effects_adr>>8;

	SPCMem[0x20c]=music_adr&255;
	SPCMem[0x20d]=music_adr>>8;

	//compile SPC file

	memset(SPCTemp,0,sizeof(SPCTemp));

	memcpy(SPCTemp,"SNES-SPC700 Sound File Data v0.30",32);//header

	SPCTemp[0x21]=26;
	SPCTemp[0x22]=26;
	SPCTemp[0x23]=27;//no ID tag
	SPCTemp[0x24]=30;//version minor

	SPCTemp[0x25]=code_adr&255;//PC=driver start address
	SPCTemp[0x26]=code_adr>>8;
	SPCTemp[0x27]=0;//A
	SPCTemp[0x28]=0;//X
	SPCTemp[0x29]=0;//Y
	SPCTemp[0x2a]=0;//PSW
	SPCTemp[0x2b]=0;//SP (LSB)

	memcpy(&SPCTemp[0x100],SPCMem,sizeof(SPCMem));

	//10100h   128 DSP Registers
	//10180h    64 unused
	//101C0h    64 Extra RAM (Memory region used when the IPL ROM region is setto read-only)

	SPCMemTopAddr=music_adr;

	return true;
}



void __fastcall TFormMain::SPCPlaySong(songStruct *s,int start_row,bool mute,int one_sample)
{
	bool compile;

	if(start_row>=s->length) return;

	compile=SPCCompile(s,start_row,mute,false,one_sample);

	if(!compile) return;

	SPCStop();

	if(emu) gme_delete(emu);

	memcpy(SPCTempPlay,SPCTemp,sizeof(SPCTemp));

	handle_gme_error(gme_open_data(SPCTempPlay,sizeof(SPCTempPlay),&emu,WaveOutSampleRate));

	gme_ignore_silence(emu,1);

	handle_gme_error(gme_start_track(emu,0));

	MusicActive=true;
}



void __fastcall TFormMain::SPCPlayRow(int start_row)
{
	noteFieldStruct *n,*m;
	int chn,row,ins,vol;
	bool find_ins,find_vol;

	memset(&tempSong,0,sizeof(songStruct));

	tempSong.length=2;
	tempSong.loop_start=1;

	tempSong.row[0].speed=99;

	for(chn=0;chn<8;++chn)
	{
		if(ChannelMute[chn]) continue;

		m=&songList[SongCur].row[start_row].chn[chn];
		n=&tempSong.row[0].chn[chn];

		n->note=m->note;

		ins=1;
		vol=99;

		find_ins=true;
		find_vol=true;

		for(row=start_row;row>=0;--row)
		{
			if(!find_ins&&!find_vol) break;

			m=&songList[SongCur].row[row].chn[chn];

			if(find_ins&&m->instrument)
			{
				ins=m->instrument;
				find_ins=false;
			}

			if(find_vol&&m->volume!=255)
			{
				vol=m->volume;
				find_vol=false;
			}
		}

		if(n->note>1) n->instrument=ins;

		n->volume=vol;

		n=&tempSong.row[1].chn[chn];

		n->value=255;
		n->volume=255;
	}

	SPCPlaySong(&tempSong,0,true,-1);
}




void __fastcall TFormMain::SPCPlayNote(int note,int ins)
{
	noteFieldStruct *n;
	int chn,row;

	memset(&tempSong,0,sizeof(songStruct));

	tempSong.length=2;
	tempSong.loop_start=1;

	tempSong.row[0].speed=99;

	n=&tempSong.row[0].chn[0];

	n->note=note;
	n->instrument=ins+1;
	n->volume=99;

	chn=(ColCur-2)/5;

	for(row=RowCur;row>=0;--row)
	{
		if(songList[SongCur].row[row].chn[chn].volume!=255)
		{
			n->volume=songList[SongCur].row[row].chn[chn].volume;
			break;
		}
	}

	n=&tempSong.row[1].chn[0];

	n->value=255;
	n->volume=255;

	SPCPlaySong(&tempSong,0,false,ins);
}



void __fastcall TFormMain::SPCStop(void)
{
	MusicActive=false;

	while(1) if(!WSTREAMER.render) break;
}



void __fastcall TFormMain::RenderPatternColor(TCanvas *c,int row,int col,TColor bgCol)
{
	int color,r,g,b;
	int selx,sely;

	if(SelectionColS<SelectionColE) selx=SelectionColS; else selx=SelectionColE;
	if(SelectionRowS<SelectionRowE) sely=SelectionRowS; else sely=SelectionRowE;

	if(RowCur!=row||ColCur!=col)
	{
		if(row<songList[SongCur].length) c->Font->Color=clBlack; else c->Font->Color=clGray;

		c->Brush->Color=bgCol;
	}
	else
	{
		c->Font->Color=clWhite;
		c->Brush->Color=clBlack;
	}

	if(SelectionWidth&&SelectionHeight&&c->Brush->Color!=clBlack)
	{
		if(row>=sely&&row<sely+SelectionHeight)
		{
			if(col>=selx&&col<selx+SelectionWidth)
			{
				color=(int)c->Brush->Color;

				r= color     &0xff;
				g=(color>>8 )&0xff;
				b=(color>>16)&0xff;

				r-=32; if(r<0) r=0;
				g-=16; if(g<0) r=0;
				b-=16; if(b<0) r=0;

				color=r|(g<<8)|(b<<16);

				c->Brush->Color=(TColor)color;
			}
		}
	}
}



void __fastcall TFormMain::CenterView(void)
{
	if(RowCur<0) RowCur=0;
	if(RowCur>=MAX_ROWS) RowCur=MAX_ROWS-1;

	RowView=RowCur;
}



int __fastcall TFormMain::PatternGetTopRow2x(void)
{
	int row,hgt,phgt,rcnt;

	hgt=TextFontHeight;

	if(!hgt) hgt=1;//to avoid error if the font is not loaded for some reason

	phgt=PaintBoxSong->Height/hgt;

	rcnt=phgt/2;
	row=RowView*2;

	while(rcnt>0)
	{
		row-=2;

		if(row<0) break;

		if(songList[SongCur].row[row/2].marker||row/2==songList[SongCur].length) --rcnt;

		--rcnt;
	}

	if(rcnt<0) ++row;

	if(songList[SongCur].row[RowCur].marker) row+=2;

	if(row<0) row=0;

	if(row/2+phgt+1>=MAX_ROWS) row=(MAX_ROWS-phgt+1)*2;

	return row;
}



int __fastcall TFormMain::PatternScreenToActualRow(int srow)
{
	int row;

	row=PatternGetTopRow2x();

	while(srow>0)
	{
		row+=2;

		if(songList[SongCur].row[row/2].marker||row/2==songList[SongCur].length) --srow;

		--srow;
	}

	return row/2;
}



void __fastcall TFormMain::RenderPattern(void)
{
	TCanvas *c;
	songStruct *s;
	noteFieldStruct *n;
	TColor bgCol;
	int x,y,row,chn,rowhl,row2x;
	char fx[2];
	AnsiString rowspace1,rowspace2;

	if(!PaintBoxSong->Visible) return;

	if(!TextFontHeight||!TextFontWidth) return;

	s=&songList[SongCur];

	c=PaintBoxSong->Canvas;

	row2x=PatternGetTopRow2x();

	rowhl=0;

	for(row=0;row<row2x/2;++row)
	{
		if(s->row[row].marker) rowhl=0;

		++rowhl;
	}

	x=0;
	y=0;

	c->Font->Color=clBlack;
	c->Brush->Color=clWhite;

	c->TextOut(x,y," RowN Sp ");
	x+=9*TextFontWidth;

	for(chn=0;chn<8;++chn)
	{
		c->Font->Color=!ChannelMute[chn]?clBlack:TColor(0xe0e0e0);

		c->TextOut(x,y," CHANNEL"+IntToStr(chn+1)+" ");
		x+=10*TextFontWidth;

		c->Font->Color=clBlack;

		c->TextOut(x,y," ");
		x+=TextFontWidth*1;
	}

	y+=TextFontHeight;

	while(y<PaintBoxSong->Height)
	{
		x=0;

		row=row2x/2;

		if(row>=MAX_ROWS) break;

		if(!(row2x&1))
		{
			if(row&&(s->row[row].marker||(s->length<MAX_ROWS-1&&row==s->length)))
			{
				c->Brush->Color=Color;

				while(x<PaintBoxSong->Width)
				{
					c->TextOut(x,y," ");
					x+=TextFontWidth;
				}

				y+=TextFontHeight;
			}
		}
		else
		{
			if(s->row[row].marker) rowhl=0;

			if(!(rowhl%s->measure)) bgCol=TColor(0xe8e8e8); else bgCol=clWhite;

			if(row==RowCur) bgCol=TColor(0xffc0c0);

			if(row==s->loop_start) rowspace1="L"; else rowspace1=" ";
			if(row==s->length)     rowspace2="E"; else rowspace2=" ";

			RenderPatternColor(c,row,-1,bgCol);
			c->TextOut(x,y,rowspace1);
			x+=TextFontWidth*1;

			RenderPatternColor(c,row,0,bgCol);
			c->TextOut(x,y,IntToStrLen(row,4));//row number
			x+=TextFontWidth*4;

			RenderPatternColor(c,row,-1,bgCol);
			c->TextOut(x,y,rowspace2);
			x+=TextFontWidth*1;

			RenderPatternColor(c,row,1,bgCol);
			c->TextOut(x,y,IntToStrLenDot(s->row[row].speed,2));//speed
			x+=TextFontWidth*2;

			RenderPatternColor(c,row,-1,bgCol);
			c->TextOut(x,y," ");
			x+=TextFontWidth*1;

			for(chn=0;chn<8;++chn)
			{
				n=&s->row[row].chn[chn];

				RenderPatternColor(c,row,chn*5+2,bgCol);

				switch(n->note)//note and octave
				{
				case 0:  c->TextOut(x,y,"..."); break;
				case 1:  c->TextOut(x,y,"---"); break;
				default: c->TextOut(x,y,NoteNames[(n->note-2)%12]+IntToStrLen((n->note-2)/12,1));
				}

				x+=TextFontWidth*3;

				RenderPatternColor(c,row,chn*5+3,bgCol);
				c->TextOut(x,y,IntToStrLenDot(n->instrument,2));//instrument
				x+=TextFontWidth*2;

				RenderPatternColor(c,row,chn*5+4,bgCol);
				if(n->volume!=255) c->TextOut(x,y,IntToStrLen(n->volume,2)); else c->TextOut(x,y,"..");//volume
				x+=TextFontWidth*2;

				RenderPatternColor(c,row,chn*5+5,bgCol);
				sprintf(fx,"%c",n->effect?n->effect:'.');
				c->TextOut(x,y,AnsiString(fx));//effect
				x+=TextFontWidth*1;

				RenderPatternColor(c,row,chn*5+6,bgCol);
				if(n->value!=255) c->TextOut(x,y,IntToStrLen(n->value,2)); else c->TextOut(x,y,"..");//effect value
				x+=TextFontWidth*2;

				RenderPatternColor(c,row,-1,bgCol);
				c->TextOut(x,y," ");
				x+=TextFontWidth*1;
			}

			c->Brush->Color=Color;

			if(!row||s->row[row].marker)
			{
				c->TextOut(x,y,s->row[row].name);
				x+=s->row[row].name.Length()*TextFontWidth;
			}

			while(x<PaintBoxSong->Width)
			{
				c->TextOut(x,y," ");
				x+=TextFontWidth;
			}

			y+=TextFontHeight;

			++rowhl;
		}

		++row2x;
	}

	while(y<PaintBoxSong->Height)
	{
		c->Brush->Color=Color;

		x=0;

		while(x<PaintBoxSong->Width)
		{
			c->TextOut(x,y," ");
			x+=TextFontWidth;
		}

		y+=TextFontHeight;
	}
}



void __fastcall TFormMain::SongMoveCursor(int rowx,int colx,bool relative)
{
	if(relative)
	{
		RowCur+=rowx;
		ColCur+=colx;
	}
	else
	{
		RowCur=rowx;
		ColCur=colx;
	}

	ResetNumber=true;

	if(ShiftKeyIsDown)
	{
		SelectionColE=ColCur;
		SelectionRowE=RowCur;

		UpdateSelection();
	}
	else
	{
		ResetSelection();
		StartSelection(ColCur,RowCur);
	}

	CenterView();
}



void __fastcall TFormMain::SongMoveRowCursor(int off)
{
	int prev;

	prev=RowCur;

	SongMoveCursor(off,0,true);

	if(RowCur<0) RowCur=0;
	if(RowCur>MAX_ROWS-1) RowCur=MAX_ROWS-1;

	if(RowCur!=prev)
	{
		CenterView();
		RenderPattern();
	}
}



void __fastcall TFormMain::SongMoveRowPrevMarker(void)
{
	int row;

	row=RowCur;

	while(row)
	{
		--row;

		if(songList[SongCur].row[row].marker) break;
	}

	SongMoveCursor(row,ColCur,false);

	RenderPattern();
}



void __fastcall TFormMain::SongMoveRowNextMarker(void)
{
	int row;

	row=RowCur;

	while(row<MAX_ROWS-1)
	{
		++row;

		if(songList[SongCur].row[row].marker) break;
		if(row==songList[SongCur].length) break;
	}

	SongMoveCursor(row,ColCur,false);

	RenderPattern();
}



void __fastcall TFormMain::SongMoveColCursor(int off)
{
	SongMoveCursor(0,off,true);

	if(ColCur<1 ) ColCur=41;
	if(ColCur>41) ColCur=1;

	RenderPattern();
}



void __fastcall TFormMain::SongMoveChannelCursor(int off)
{
	if(ColCur<2)
	{
		if(off<0) SongMoveCursor(RowCur,2+5*8-5,false); else SongMoveCursor(RowCur,2,false);
	}
	else
	{
		if(off<0)
		{
			SongMoveCursor(0,-5,true);

			if(ColCur<2) ColCur+=5*8;
		}
		else
		{
			SongMoveCursor(0,5,true);

			if(ColCur>=2+5*8) ColCur-=5*8;
		}
	}

	RenderPattern();
}



void __fastcall TFormMain::SongDeleteShiftColumn(int col,int row)
{
	noteFieldStruct *n,*m;
	int rowc,chn;

	if(row<1||!col) return;

	if(col==1)
	{
		for(rowc=row;rowc<MAX_ROWS;++rowc)
		{
			songList[SongCur].row[rowc-1].speed=songList[SongCur].row[rowc].speed;
		}

		songList[SongCur].row[MAX_ROWS-1].speed=0;
	}
	else
	{
		chn=(col-2)/5;

		for(rowc=row;rowc<MAX_ROWS;++rowc)
		{
			n=&songList[SongCur].row[rowc-1].chn[chn];
			m=&songList[SongCur].row[rowc].chn[chn];

			switch((col-2)%5)
			{
			case 0: n->note      =m->note;       break;
			case 1: n->instrument=m->instrument; break;
			case 2: n->volume    =m->volume;     break;
			case 3: n->effect    =m->effect;     break;
			case 4: n->value     =m->value;      break;
			}
		}

		n=&songList[SongCur].row[MAX_ROWS-1].chn[chn];

		switch((col-2)%5)
		{
		case 0: n->note      =0;   break;
		case 1: n->instrument=0;   break;
		case 2: n->volume    =255; break;
		case 3: n->effect    =0;   break;
		case 4: n->value     =255; break;
		}
	}
}



void __fastcall TFormMain::SongInsertShiftColumn(int col,int row)
{
	noteFieldStruct *n,*m;
	int rowc,chn;

	if(!col||row>=MAX_ROWS) return;

	if(col==1)
	{
		for(rowc=MAX_ROWS-1;rowc>row;--rowc)
		{
			songList[SongCur].row[rowc].speed=songList[SongCur].row[rowc-1].speed;
		}

		songList[SongCur].row[row].speed=0;
	}
	else
	{
		chn=(col-2)/5;

		for(rowc=MAX_ROWS-1;rowc>row;--rowc)
		{
			n=&songList[SongCur].row[rowc].chn[chn];
			m=&songList[SongCur].row[rowc-1].chn[chn];

			switch((col-2)%5)
			{
			case 0: n->note      =m->note;       break;
			case 1: n->instrument=m->instrument; break;
			case 2: n->volume    =m->volume;     break;
			case 3: n->effect    =m->effect;     break;
			case 4: n->value     =m->value;      break;
			}
		}

		n=&songList[SongCur].row[row].chn[chn];

		switch((col-2)%5)
		{
		case 0: n->note      =0;   break;
		case 1: n->instrument=0;   break;
		case 2: n->volume    =255; break;
		case 3: n->effect    =0;   break;
		case 4: n->value     =255; break;
		}
	}
}



void __fastcall TFormMain::SongDeleteShift(int col,int row)
{
	int i;

	if(!col) return;

	SetUndo();

	if(col<2)
	{
		SongDeleteShiftColumn(col,row);
	}
	else
	{
		col=2+(col-2)/5*5;

		for(i=0;i<5;++i) SongDeleteShiftColumn(col++,row);
	}

	SongMoveRowCursor(-1);
}



void __fastcall TFormMain::SongInsertShift(int col,int row)
{
	int i;

	if(!col) return;

	SetUndo();

	if(col==1)
	{
		SongInsertShiftColumn(col,row);
	}
	else
	{
		col=((col-2)/5)*5+2;

		for(i=0;i<5;++i) SongInsertShiftColumn(col+i,row);
	}

	RenderPattern();
}



void __fastcall TFormMain::ToggleChannelMute(int chn,bool solo)
{
	int i,cnt;

	if(!solo)
	{
		ChannelMute[chn]^=true;
	}
	else
	{
		cnt=0;

		for(i=0;i<8;++i) if(ChannelMute[i]) ++cnt;

		if(cnt<7||ChannelMute[chn])
		{
			for(i=0;i<8;++i) ChannelMute[i]=(i==chn)?false:true;
		}
		else
		{
			for(i=0;i<8;++i) ChannelMute[i]=false;
		}
	}

	RenderPattern();
}



int __fastcall TFormMain::IsNumberKey(int key)
{
	if(key>='0'&&key<='9') return key-'0'; else return -1;
}



int __fastcall TFormMain::IsNoteKey(int key)
{
	int i,note;

	note=-1;

	for(i=0;i<sizeof(noteKeyCodes);i+=2)
	{
		if(noteKeyCodes[i]==key)
		{
			note=noteKeyCodes[i+1];
			break;
		}
	}

	if(note>=0)
	{
		if(note<255)
		{
			note=note+OctaveCur*12+2;

			if(note>=2+12*8) note=2+12*8-1;
		}
		else
		{
			note=1;
		}
	}

	return note;
}



bool __fastcall TFormMain::IsAllNoteKeysUp(void)
{
	int i;

	for(i=0;i<sizeof(noteKeyCodes);i+=2)
	{
		if(GetKeyState(noteKeyCodes[i])&0x8000) return false;
	}

	return true;
}



AnsiString __fastcall TFormMain::GetSectionName(int row)
{
	while(row)
	{
		if(songList[SongCur].row[row].marker) break;

		--row;
	}

	return songList[SongCur].row[row].name;
}



void __fastcall TFormMain::SetSectionName(int row,AnsiString name)
{
	while(row)
	{
		if(songList[SongCur].row[row].marker) break;

		--row;
	}

	songList[SongCur].row[row].name=name;

	RenderPattern();
}



void __fastcall TFormMain::EnterNoteKey(int note)
{
	int chn;

	chn=(ColCur-2)/5;

	SetUndo();

	songList[SongCur].row[RowCur].chn[chn].note=note;

	if(MInstrumentAutoNumber->Checked)
	{
		if(note>1) songList[SongCur].row[RowCur].chn[chn].instrument=InsCur+1;
	}

	SPCPlayRow(RowCur);
	SongMoveRowCursor(AutoStep);
	RenderPattern();
}



void __fastcall TFormMain::AppMessage(tagMSG &Msg, bool &Handled)
{
	noteFieldStruct *n;
	int i,Key,num,row,cnt,note;
	bool Shift,Ctrl,First;

	if(!Active&&!FormOutputMonitor->Active) return;

	if(Msg.message==MIM_DATA)
	{
		Key=(Msg.lParam>>8)&0x7f;

		note=Key+2-12;

		if(note<2) note=2;
		if(note>2+8*12-1) note=2+8*12-1;

		if((Msg.lParam&0xf0)==0x90)
		{
			if(((Msg.lParam>>16)&0xff)!=0)//keyon
			{
				MidiKeyState[Key]=TRUE;

				if(FormMain->PageControlMode->ActivePage==FormMain->TabSheetSong) FormMain->EnterNoteKey(note);

				if(PageControlMode->ActivePage==TabSheetInstruments) SPCPlayNote(note,InsCur);
			}
			else//keyoff
			{
				MidiKeyState[Key]=FALSE;

				num=0;

				for(i=0;i<128;++i) if(MidiKeyState[i]) ++num;

				if(!num) SPCStop();//stop sound only if there is no MIDI keys pressed
			}
		}

		Handled=true;
		return;
	}

	if(Msg.message==WM_KEYUP)
	{
		Key=Msg.wParam;

		if(Key==VK_RETURN)
		{
			SPCStop();
			//Handled=true;
			//return;
		}

		if(IsNoteKey(Key)>=0) if(IsAllNoteKeysUp()) SPCStop();

		if(Msg.wParam==VK_SHIFT) ShiftKeyIsDown=false;
	}

	if(Msg.message==WM_KEYDOWN)
	{
		Key=Msg.wParam;

		First=(Msg.lParam&(1<<30))?false:true;

		Shift=GetKeyState(VK_SHIFT  )&0x8000?true:false;
		Ctrl =GetKeyState(VK_CONTROL)&0x8000?true:false;

		ShiftKeyIsDown=Shift;

		//these keys works in any mode

		if(!Ctrl&&!Shift)
		{
			switch(Key)
			{
			case VK_F1:
				PageControlMode->ActivePage=TabSheetSong;
				Handled=true;
				return;

			case VK_F2:
				PageControlMode->ActivePage=TabSheetSongList;
				Handled=true;
				return;

			case VK_F3:
				PageControlMode->ActivePage=TabSheetInstruments;
				Handled=true;
				return;

			case VK_F4:
				PageControlMode->ActivePage=TabSheetInfo;
				Handled=true;
				return;

			case VK_F5:
				MSongPlayStartClick(this);
				Handled=true;
				return;

			case VK_F7:
				MSongPlayCurClick(this);
				Handled=true;
				return;

			case VK_F8:
				MSongStopClick(this);
				Handled=true;
				return;

			case VK_ADD:
				if(SongCur<MAX_SONGS-1)
				{
					++SongCur;
					SongChange(SongCur);
				}
				Handled=true;
				return;

			case VK_SUBTRACT:
				if(SongCur>0)
				{
					--SongCur;
					SongChange(SongCur);
				}
				Handled=true;
				return;

			case VK_DIVIDE:
				--OctaveCur;
				if(OctaveCur<1) OctaveCur=8;
				UpdateInfo(false);
				Handled=true;
				return;

			case VK_MULTIPLY:
				++OctaveCur;
				if(OctaveCur>8) OctaveCur=1;
				UpdateInfo(false);
				Handled=true;
				return;

			case VK_NUMPAD1:
			case VK_NUMPAD2:
			case VK_NUMPAD3:
			case VK_NUMPAD4:
			case VK_NUMPAD5:
			case VK_NUMPAD6:
			case VK_NUMPAD7:
			case VK_NUMPAD8:
				OctaveCur=Key-VK_NUMPAD1+1;
				UpdateInfo(false);
				Handled=true;
				return;
			}
		}

		if(ActiveControl) if(ActiveControl->ClassNameIs("TEdit")) return;

		//these keys only work in the instrument editor

		if(PageControlMode->ActivePage==TabSheetInstruments)
		{
			num=IsNoteKey(Key);

			if(num>=0&&First)//no autorepeat
			{
				SPCPlayNote(num,InsCur);
				Handled=true;
				return;
			}
		}

		//these keys only work in the pattern editor

		if(PageControlMode->ActivePage==TabSheetSong)
		{
			if(Key==VK_RETURN&&ColCur&&First)
			{
				SPCPlaySong(&songList[SongCur],RowCur,true,-1);
				Handled=true;
				return;
			}

			if(Shift)//check key combinations with the Shift key
			{
				switch(Key)
				{
				case 'X':
					SetUndo();
					CopyCutToBuffer(true,true,true);
					RenderPattern();
					Handled=true;
					return;

				case 'V':
					SetUndo();
					PasteFromBuffer(true,false);
					RenderPattern();
					Handled=true;
					return;
				}
			}

			if(Ctrl)//check key combinations with the Ctrl key first
			{
				switch(Key)
				{
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
					ToggleChannelMute(Key-'1',false);
					Handled=true;
					return;

				case '0':
					ToggleChannelMute((ColCur-2)/5,true);
					Handled=true;
					return;

				case 'F':
					FormSectionList->ListBoxSections->Clear();

					for(row=0;row<MAX_ROWS;++row)
					{
						if(songList[SongCur].row[row].name!=""||songList[SongCur].row[row].marker)
						{
							FormSectionList->ListBoxSections->Items->Add(Format("%4.4d",ARRAYOFCONST((row)))+": "+songList[SongCur].row[row].name);
						}

						if(row==RowCur) FormSectionList->ListBoxSections->ItemIndex=FormSectionList->ListBoxSections->Items->Count-1;
					}

					FormSectionList->ShowModal();

					if(FormSectionList->Selected>=0)
					{
						cnt=0;

						for(row=0;row<MAX_ROWS;++row)
						{
							if(songList[SongCur].row[row].name!=""||songList[SongCur].row[row].marker)
							{
								if(cnt==FormSectionList->Selected)
								{
									RowCur=row;
									break;
								}

								++cnt;
							}
						}

						SongMoveCursor(RowCur,0,false);
						RenderPattern();
					}

					Handled=true;
					return;

				case 'G':
					ColCurPrev=ColCur;
					SongMoveCursor(RowCur,0,false);
					RenderPattern();
					Handled=true;
					return;

				case VK_OEM_4:
					if(songList[SongCur].measure>2)
					{
						--songList[SongCur].measure;
						RenderPattern();
					}
					Handled=true;
					return;

				case VK_OEM_6:
					if(songList[SongCur].measure<16)
					{
						++songList[SongCur].measure;
						RenderPattern();
					}
					Handled=true;
					return;

				case VK_HOME:
					songList[SongCur].loop_start=RowCur;
					RenderPattern();
					Handled=true;
					return;

				case VK_END:
					songList[SongCur].length=RowCur;
					RenderPattern();
					Handled=true;
					return;

				case 'Z':
					Undo();
					Handled=true;
					return;

				case 'C':
					CopyCutToBuffer(true,false,false);
					RenderPattern();
					Handled=true;
					return;

				case 'X':
					SetUndo();
					CopyCutToBuffer(true,true,false);
					RenderPattern();
					Handled=true;
					return;

				case 'V':
					SetUndo();
					PasteFromBuffer(false,false);
					RenderPattern();
					Handled=true;
					return;

				case 'B':
					SetUndo();
					PasteFromBuffer(false,true);
					RenderPattern();
					Handled=true;
					return;

				case VK_F1:
					Transpose(-1,true,false,false,0);
					Handled=true;
					return;

				case VK_F2:
					Transpose(1,true,false,false,0);
					Handled=true;
					return;

				case VK_F3:
					Transpose(-12,true,false,false,0);
					Handled=true;
					return;

				case VK_F4:
					Transpose(12,true,false,false,0);
					Handled=true;
					return;

				case 'E':
					SetUndo();
					ExpandSelection();
					Handled=true;
					return;

				case 'S':
					SetUndo();
					ShrinkSelection();
					Handled=true;
					return;

				case 'A':
					SetChannelSelection(false,SelectionHeight<=1||SelectionHeight==MAX_ROWS?true:false);
					Handled=true;
					return;

				case 'L':
					SetChannelSelection(true,SelectionHeight<=1||SelectionHeight==MAX_ROWS?true:false);
					Handled=true;
					return;
				}
			}
			else//then keys without Ctrl
			{
				if(Key==VK_DELETE&&(SelectionWidth>1||SelectionHeight>1))
				{
					CopyCutToBuffer(false,true,false);
					RenderPattern();
					Handled=true;
					return;
				}

				if(ColCur>=2)
				{
					n=&songList[SongCur].row[RowCur].chn[(ColCur-2)/5];

					switch((ColCur-2)%5)
					{
					case 0://check note key
						{
							if(!Shift&&!Ctrl)
							{
								if(Key==VK_DELETE)
								{
									SetUndo();

									n->note=0;
									n->instrument=0;//also delete instrument number

									SongMoveRowCursor(AutoStep);
									RenderPattern();
									Handled=true;
									return;
								}

								num=IsNoteKey(Key);

								if(num>=0&&First)//no autorepeat for the note keys
								{
									EnterNoteKey(num);

									Handled=true;
									return;
								}
							}
						}
						break;

					case 1://two digit instrument number
						{
							if(Key==VK_DELETE)
							{
								SetUndo();
								n->instrument=0;
								SongMoveRowCursor(AutoStep);
								RenderPattern();
								Handled=true;
								return;
							}

							num=IsNumberKey(Key);

							if(num>=0)
							{
								if(ResetNumber)
								{
									SetUndo();
									n->instrument=0;
									ResetNumber=false;
								}

								n->instrument=(n->instrument*10+num)%100;
								RenderPattern();
								InsChange(n->instrument-1);
								Handled=true;
								return;
							}
						}
						break;

					case 2://two digit volume
						{
							if(Key==VK_DELETE)
							{
								SetUndo();
								n->volume=255;
								SongMoveRowCursor(AutoStep);
								RenderPattern();
								Handled=true;
								return;
							}

							num=IsNumberKey(Key);

							if(num>=0)
							{
								if(ResetNumber)
								{
									SetUndo();
									n->volume=num*10;
									ResetNumber=false;
								}
								else
								{
									n->volume+=num%10;
									ResetNumber=true;
								}

								RenderPattern();
								Handled=true;
								return;
							}
						}
						break;

					case 3://enter effect letter
						{
							if(Key==VK_DELETE)
							{
								SetUndo();

								n->effect=0;
								n->value=255;

								SongMoveRowCursor(AutoStep);
								RenderPattern();
								Handled=true;
								return;
							}

							if(Key>='A'&&Key<='Z')
							{
								SetUndo();

								n->effect=Key;

								if(n->value==255) n->value=0;

								//SongMoveCursor(0,1,true);
								RenderPattern();
								Handled=true;
								return;
							}
						}
						break;

					case 4://two digit effect value number
						{
							if(Key==VK_DELETE)
							{
								SetUndo();
								n->value=255;
								SongMoveRowCursor(AutoStep);
								RenderPattern();
								Handled=true;
								return;
							}

							num=IsNumberKey(Key);

							if(num>=0)
							{
								if(ResetNumber)
								{
									SetUndo();
									n->value=num*10;
									ResetNumber=false;
								}
								else
								{
									n->value+=num%10;
									ResetNumber=true;
								}

								RenderPattern();
								Handled=true;
								return;
							}
						}
						break;
					}
				}
				else
				{
					if(ColCur==0)//row number
					{
						num=IsNumberKey(Key);

						if(num>=0)
						{
							if(ResetNumber)
							{
								RowCur=0;
								ResetNumber=false;
							}

							RowCur=(RowCur*10+num)%MAX_ROWS;
							CenterView();
							RenderPattern();
							Handled=true;
							return;
						}

						if(Key==VK_RETURN&&ColCurPrev>=0)
						{
							SongMoveCursor(RowCur,ColCurPrev,false);
							ColCurPrev=-1;
							RenderPattern();
							Handled=true;
							return;
						}
					}
					else//speed
					{
						if(Key==VK_DELETE)
						{
							SetUndo();
							songList[SongCur].row[RowCur].speed=0;
							SongMoveRowCursor(AutoStep);
							RenderPattern();
							Handled=true;
							return;
						}

						num=IsNumberKey(Key);

						if(num>=0)
						{
							if(ResetNumber)
							{
								SetUndo();
								songList[SongCur].row[RowCur].speed=num*10;
								ResetNumber=false;
							}
							else
							{
								songList[SongCur].row[RowCur].speed+=num%10;
								ResetNumber=true;
							}

							RenderPattern();
							Handled=true;
							return;
						}
					}
				}

				switch(Key)
				{
				case VK_LEFT:
					SongMoveColCursor(-1);
					Handled=true;
					return;

				case VK_RIGHT:
					SongMoveColCursor(1);
					Handled=true;
					return;

				case VK_UP:
					SongMoveRowCursor(-1);
					Handled=true;
					return;

				case VK_DOWN:
					SongMoveRowCursor(1);
					Handled=true;
					return;

				case VK_PRIOR:
					SongMoveRowCursor(-PageStep);
					Handled=true;
					return;

				case VK_NEXT:
					SongMoveRowCursor(PageStep);
					Handled=true;
					return;

				case VK_TAB:
					SongMoveChannelCursor(Shift?-1:1);
					Handled=true;
					return;

				case VK_BACK:
					SongDeleteShift(ColCur,RowCur);
					Handled=true;
					return;

				case VK_INSERT:
					SongInsertShift(ColCur,RowCur);
					Handled=true;
					return;

				case VK_OEM_4:
					--AutoStep;
					if(AutoStep<0) AutoStep=16;
					UpdateInfo(false);
					Handled=true;
					return;

				case VK_OEM_6:
					++AutoStep;
					if(AutoStep>16) AutoStep=0;
					UpdateInfo(false);
					Handled=true;
					return;

				case VK_SPACE:
					songList[SongCur].row[RowCur].marker^=true;
					RenderPattern();
					Handled=true;
					return;

				case VK_HOME:
					SongMoveRowPrevMarker();
					Handled=true;
					return;

				case VK_END:
					SongMoveRowNextMarker();
					Handled=true;
					return;

				case VK_OEM_3:
					FormName->EditName->Text=GetSectionName(RowCur);
					FormName->ShowModal();
					SetSectionName(RowCur,FormName->EditName->Text);
					Handled=true;
					break;
				}
			}
		}
	}

	if(Msg.message==WM_MOUSEWHEEL)
	{
		if(PageControlMode->ActivePage==TabSheetSong)
		{
			SongMoveRowCursor(-GET_WHEEL_DELTA_WPARAM(Msg.wParam)/120);
			Handled=true;
			return;
		}
	}
}



void __fastcall TFormMain::RenderMemoryUse(void)
{
	TCanvas *c;
	float x,w,width,ppb,len;
	int i,y,height,free,stream,ins;

	stream=28*9*9+64;//64 is IPL size
	free=65536-512-(spc700_size-2)-SPCInstrumentsSize-SPCMusicSize-SPCEffectsSize-stream;

	c=PaintBoxMemoryUse->Canvas;

	width=(float)PaintBoxMemoryUse->Width;
	height=40;

	ppb=width/65536.0f;
	x=0;

	//direct page and stack

	w=0x200*ppb;

	c->Brush->Style=bsSolid;
	c->Brush->Color=clSkyBlue;

	c->Rectangle(x,0,x+w,height);

	x+=w-1;

	//driver code

	w=(spc700_size-2)*ppb;

	c->Brush->Color=clBlue;

	c->Rectangle(x,0,x+w,height);

	x+=w-1;

	//adsr, dir, and samples data

	w=SPCInstrumentsSize*ppb;

	c->Brush->Color=clYellow;

	c->Rectangle(x,0,x+w,height);

	len=0;

	for(ins=0;ins<MAX_INSTRUMENTS;++ins)
	{
		if(!insList[ins].source) continue;

		c->PenPos=TPoint(x+len*ppb,0);
		c->LineTo(x+len*ppb,height);

		len+=InsCalculateBRRSize(ins,false);
	}

	x+=w-1;

	//sound effects

	w=SPCEffectsSize*ppb;

	c->Brush->Color=clLime;

	c->Rectangle(x,0,x+w,height);

	x+=w-1;

	//music data

	w=SPCMusicLargestSize*ppb;

	c->Brush->Color=TColor(0x8080ff);

	c->Rectangle(x,0,x+w,height);

	w=SPCMusicSize*ppb;

	c->Brush->Color=TColor(0x0000ff);

	c->Rectangle(x,0,x+w,height);

	w=SPCMusicLargestSize*ppb;

	x+=w-1;

	//free memory

	c->Brush->Color=Color;

	w=stream*ppb;

	c->Rectangle(x,0,width-w,height);

	x=width-w-1;

	//BRR streaming buffer

	c->Brush->Style=bsBDiagonal;
	c->Brush->Color=clBlack;
	c->Rectangle(x,0,width,height);
	c->Brush->Style=bsSolid;

	//scale

	x=0;
	w=width/64;

	for(i=0;i<65;++i)
	{
		if(i&7)
		{
			c->PenPos=TPoint(x,height);
			c->LineTo(x,height+5);
		}
		else
		{
			c->PenPos=TPoint(x,height);
			c->LineTo(x,height+10);
			c->PenPos=TPoint(x+1,height);
			c->LineTo(x+1,height+10);
		}

		x+=w;

		if(x>width-2) x=width-2;
	}

	//legend

	y=height+20;

	c->Brush->Color=clSkyBlue;
	c->Rectangle(0,y,15,y+15);
	c->Brush->Color=Color;
	c->TextOut(20,y-1,"Direct Page and Stack: 512 bytes");

	y+=20;

	c->Brush->Color=clBlue;
	c->Rectangle(0,y,15,y+15);
	c->Brush->Color=Color;
	c->TextOut(20,y-1,"Driver code: "+IntToStr(spc700_size-2)+" bytes");

	y+=20;

	c->Brush->Color=clYellow;
	c->Rectangle(0,y,15,y+15);
	c->Brush->Color=Color;
	c->TextOut(20,y-1,"Instruments: "+IntToStr(SPCInstrumentsSize)+" bytes (Dir:"+IntToStr(4*InstrumentsCount)+", ADSR:"+IntToStr(2*InstrumentsCount)+", Samples:"+IntToStr(SPCInstrumentsSize-6*InstrumentsCount)+")");

	y+=20;

	c->Brush->Color=clLime;
	c->Rectangle(0,y,15,y+15);
	c->Brush->Color=Color;
	c->TextOut(20,y-1,"Sound effects: "+IntToStr(SPCEffectsSize)+" bytes        ");

	y+=20;

	c->Brush->Color=clRed;
	c->Rectangle(0,y,15,y+15);
	c->Brush->Color=Color;
	c->TextOut(20,y-1,"Music data: "+IntToStr(SPCMusicSize)+" bytes (current), "+IntToStr(SPCMusicLargestSize)+" bytes (largest)           ");

	y+=20;

	c->Brush->Style=bsBDiagonal;
	c->Brush->Color=clBlack;
	c->Rectangle(0,y,15,y+15);
	c->Brush->Style=bsSolid;
	c->Brush->Color=Color;
	c->TextOut(20,y-1,"BRR streaming buffer: "+IntToStr(stream)+" bytes");

	y+=20;

	c->Rectangle(0,y,15,y+15);
	c->TextOut(20,y-1,"Free memory: "+IntToStr(free)+" bytes        ");
}



char* __fastcall TFormMain::MakeNameForAlias(AnsiString name)
{
	static char alias[1024];
	int i,c;

	strcpy(alias,name.c_str());

	for(i=0;i<(int)strlen(alias);++i)
	{
		c=alias[i];

		if(c>='a'&&c<='z') c-=32;
		if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9'))) c='_';

		alias[i]=c;
	}

	return alias;
}



bool __fastcall TFormMain::ExportAll(AnsiString dir)
{
	unsigned char header[2];
	FILE *file;
	int song,size,spc700_size,song_id,sounds_all,songs_all;

	SPCStop();

	//compile driver code, samples and sound effects into single binary

	memset(&tempSong,0,sizeof(tempSong));

	SPCCompile(&tempSong,0,false,true,-1);

	file=fopen((dir+"spc700.bin").c_str(),"wb");

	if(file)
	{
		spc700_size=SPCMemTopAddr-0x200;

		header[0]=spc700_size&255;//header
		header[1]=spc700_size>>8;

		fwrite(header,sizeof(header),1,file);
		fwrite(&SPCMem[0x200],spc700_size,1,file);

		fclose(file);
	}

	//compile each song into a separate binary

	song_id=1;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(songList[song].effect) continue;

		if(SongIsEmpty(&songList[song])) continue;

		size=SongCompile(&songList[song],0,SPCMemTopAddr,false);

		if(size<0) return false;

		file=fopen((dir+"music_"+IntToStr(song_id)+".bin").c_str(),"wb");

		if(file)
		{
			header[0]=size&255;//header
			header[1]=size>>8;

			fwrite(header,sizeof(header),1,file);
			fwrite(&SPCMem[SPCMemTopAddr],size,1,file);

			fclose(file);
		}

		++song_id;
	}

	//generate resource inclusion file

	file=fopen((dir+"sounds.asm").c_str(),"wt");

	if(!file) return false;

	fprintf(file,";this file generated with SNES GSS tool\n\n");

	song_id=0;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(SongIsEmpty(&songList[song])) continue;

		if(!songList[song].effect) continue;

		fprintf(file,".define SFX_%s\t%i\n",MakeNameForAlias(songList[song].name),song_id);

		++song_id;
	}

	fprintf(file,"\n");

	song_id=0;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(SongIsEmpty(&songList[song])) continue;

		if(songList[song].effect) continue;

		fprintf(file,".define MUS_%s\t%i\n",MakeNameForAlias(songList[song].name),song_id);

		++song_id;
	}

	fprintf(file,"\n");

	fprintf(file,".section \".roDataSoundCode1\" superfree\n");

	fprintf(file,"spc700_code_1:\t.incbin \"spc700.bin\" skip 0 read %i\n",spc700_size<32768?spc700_size:32768);

	if(spc700_size<=32768) fprintf(file,"spc700_code_2:\n");

	fprintf(file,".ends\n\n");

	if(spc700_size>32768)
	{
		fprintf(file,".section \".roDataSoundCode2\" superfree\n");

		fprintf(file,"spc700_code_2:\t.incbin \"spc700.bin\" skip 32768\n");

		fprintf(file,".ends\n\n");
	}

	song_id=1;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(songList[song].effect) continue;

		if(SongIsEmpty(&songList[song])) continue;

		fprintf(file,".section \".roDataMusic%i\" superfree\n",song_id);

		fprintf(file,"music_%i_data:\t.incbin \"music_%i.bin\"\n",song_id,song_id);

		fprintf(file,".ends\n\n");

		++song_id;
	}

	fclose(file);

	//generate C header with externs and meta data for music and sound effects

	sounds_all=0;
	songs_all=0;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(SongIsEmpty(&songList[song])) continue;

		if(songList[song].effect) ++sounds_all; else ++songs_all;
	}

	file=fopen((dir+"sounds.h").c_str(),"wt");

	if(!file) return false;

	fprintf(file,"//this file generated with SNES GSS tool\n\n");

	fprintf(file,"#define SOUND_EFFECTS_ALL\t%i\n\n",sounds_all);
	fprintf(file,"#define MUSIC_ALL\t%i\n\n",songs_all);

	if(sounds_all)
	{
		fprintf(file,"//sound effect aliases\n\n",sounds_all);
		fprintf(file,"enum {\n");

		song_id=0;

		for(song=0;song<MAX_SONGS;++song)
		{
			if(SongIsEmpty(&songList[song])) continue;

			if(!songList[song].effect) continue;

			fprintf(file,"\tSFX_%s=%i",MakeNameForAlias(songList[song].name),song_id);

			if(song_id<sounds_all-1) fprintf(file,",\n");

			++song_id;
		}

		fprintf(file,"\n};\n\n");

		fprintf(file,"//sound effect names\n\n",sounds_all);
		fprintf(file,"const char* const soundEffectsNames[SOUND_EFFECTS_ALL]={\n");

		song_id=0;

		for(song=0;song<MAX_SONGS;++song)
		{
			if(SongIsEmpty(&songList[song])) continue;

			if(!songList[song].effect) continue;

			fprintf(file,"\t\"%s\"",songList[song].name.UpperCase());

			if(song_id<sounds_all-1) fprintf(file,",\t//%i\n",song_id);

			++song_id;
		}

		fprintf(file,"\t//%i\n};\n\n",song_id-1);
	}

	if(songs_all)
	{
		fprintf(file,"//music effect aliases\n\n",sounds_all);
		fprintf(file,"enum {\n");

		song_id=0;

		for(song=0;song<MAX_SONGS;++song)
		{
			if(SongIsEmpty(&songList[song])) continue;

			if(songList[song].effect) continue;

			fprintf(file,"\tMUS_%s=%i",MakeNameForAlias(songList[song].name),song_id);

			if(song_id<songs_all-1) fprintf(file,",\n");

			++song_id;
		}

		fprintf(file,"\n};\n\n");

		fprintf(file,"//music names\n\n",sounds_all);
		fprintf(file,"const char* const musicNames[MUSIC_ALL]={\n");

		song_id=0;

		for(song=0;song<MAX_SONGS;++song)
		{
			if(SongIsEmpty(&songList[song])) continue;

			if(songList[song].effect) continue;

			fprintf(file,"\t\"%s\"",songList[song].name.UpperCase());

			if(song_id<songs_all-1) fprintf(file,",\t//%i\n",song_id);

			++song_id;
		}

		fprintf(file,"\t//%i\n};\n\n",song_id-1);
	}

	fprintf(file,"extern const unsigned char spc700_code_1[];\n");
	fprintf(file,"extern const unsigned char spc700_code_2[];\n");

	song_id=1;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(SongIsEmpty(&songList[song])) continue;

		if(songList[song].effect) continue;

		fprintf(file,"extern const unsigned char music_%i_data[];\n",song_id);

		++song_id;
	}

	fprintf(file,"\n");

	fprintf(file,"const unsigned char* const musicData[MUSIC_ALL]={\n");

	song_id=1;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(SongIsEmpty(&songList[song])) continue;

		if(songList[song].effect) continue;

		fprintf(file,"\tmusic_%i_data",song_id);

		if(song_id<songs_all) fprintf(file,",\n");

		++song_id;
	}

	fprintf(file,"\n};\n");

	fclose(file);

	return true;
}



AnsiString __fastcall TFormMain::ImportXM(AnsiString filename,bool song)
{
	FILE *file;
	int size,order_len,order_loop,channels,pp,patlen;
	int tag,note,ins,vol,fx,param,speed,speed_prev,speed_int;
	int row,chn,ptn,header_size,dst_song,dst_row,ord,pnam_off;
	unsigned char *xm;

	file=fopen(filename.c_str(),"rb");

	if(!file) return "Can't open file";

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	xm=(unsigned char*)malloc(size);
	fread(xm,size,1,file);
	fclose(file);

	if(memcmp(xm,"Extended Module: ",17)) return "Not a module";

	header_size=rd_dword_lh(&xm[60]);
	order_len  =rd_word_lh(&xm[60+4]);
	order_loop =rd_word_lh(&xm[60+6]);
	channels   =rd_word_lh(&xm[60+8]);
	speed      =rd_word_lh(&xm[60+16]);

	if(!order_len) return "Module should have at least one order position";

	dst_song=SongCur;

	if(song)
	{
		SongClear(dst_song);

		xm[37]=0;//if module name uses all 20 bytes

		songList[dst_song].name.sprintf("%s",&xm[17]);
		songList[dst_song].name=songList[dst_song].name.Trim();

		dst_row=0;
		speed_prev=-1;
	}
	else
	{
		pnam_off=-1;
		pp=0;

		while(pp<size-4)
		{
			if(!memcmp(&xm[pp],"PNAM",4))
			{
				pnam_off=pp+8;
				break;
			}

			++pp;
		}
	}

	for(ord=0;ord<order_len;++ord)
	{
		if(!song)
		{
			SongClear(dst_song);

			dst_row=0;
			speed_prev=-1;

			if(pnam_off>=0)
			{
				songList[dst_song].name.sprintf("%s",&xm[pnam_off+ord*32]);
				songList[dst_song].name=songList[dst_song].name.Trim();
			}

			songList[dst_song].effect=true;
		}
		else
		{
			if(ord==order_loop) songList[dst_song].loop_start=dst_row;
		}

		songList[dst_song].row[dst_row].marker=true;

		pp=60+header_size;

		for(ptn=0;ptn<xm[60+20+ord];++ptn) pp+=(rd_dword_lh(&xm[pp])+rd_word_lh(&xm[pp+7]));

		patlen=rd_word_lh(&xm[pp+5]);
		pp+=rd_dword_lh(&xm[pp]);

		for(row=0;row<patlen;++row)
		{
			for(chn=0;chn<channels;++chn)
			{
				if(xm[pp]&0x80)
				{
					tag=xm[pp++];

					if(tag&0x01) note =xm[pp++]; else note =0;
					if(tag&0x02) ins  =xm[pp++]; else ins  =0;
					if(tag&0x04) vol  =xm[pp++]; else vol  =0;
					if(tag&0x08) fx   =xm[pp++]; else fx   =0;
					if(tag&0x10) param=xm[pp++]; else param=0;
				}
				else
				{
					note =xm[pp++];
					ins  =xm[pp++];
					vol  =xm[pp++];
					fx   =xm[pp++];
					param=xm[pp++];
				}

				if(fx==0x0f&&param<0x20) speed=param;

				if(speed!=speed_prev)
				{
					speed_prev=speed;

					speed_int=(int)(float(speed)*1.6f);

					if(speed_int<1 ) speed_int=1;
					if(speed_int>99) speed_int=99;

					songList[dst_song].row[dst_row].speed=speed_int;
				}

				if(chn<8)
				{
					if(note==97)
					{
						songList[dst_song].row[dst_row].chn[chn].note=1;
					}
					else
					{
						if(note>0&&note<97)
						{
							note=note-1+24+2;

							if(note<2) note=2;
							if(note>=2+12*8) note=2+12*8-1;

							songList[dst_song].row[dst_row].chn[chn].note=note;
						}
					}

					if(ins) songList[dst_song].row[dst_row].chn[chn].instrument=ins;

					if(vol)
					{
						vol=(int)((float)vol*1.547f);

						if(vol<1)  vol=1;
						if(vol>99) vol=99;

						songList[dst_song].row[dst_row].chn[chn].volume=vol;
					}

				}
			}

			++dst_row;

			if(dst_row>=MAX_ROWS) break;
		}

		if(song)
		{
			if(dst_row>=MAX_ROWS) break;
		}
		else
		{
			songList[dst_song].row[dst_row].marker=true;

			++dst_song;

			if(dst_song>MAX_SONGS) break;
		}
	}

	if(song)
	{
		songList[dst_song].row[dst_row].marker=true;
		songList[dst_song].length=dst_row;
	}

	fclose(file);

	free(xm);

	return "";
}



void __fastcall TFormMain::CompileAllSongs(void)
{
	int song;

	SPCMusicLargestSize=0;

	for(song=0;song<MAX_SONGS;++song)
	{
		if(SongIsEmpty(&songList[song])) continue;

		SongCompile(&songList[song],0,SPCMemTopAddr,false);

		if(songList[song].compiled_size>SPCMusicLargestSize) SPCMusicLargestSize=songList[song].compiled_size;
	}
}



//---------------------------------------------------------------------------
__fastcall TFormMain::TFormMain(TComponent* Owner)
: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFormMain::FormCreate(TObject *Sender)
{
	AnsiString dir;
	int i,midi_id,midi_all;

	for(i=0;i<this->ComponentCount;i++)
	{
		if(this->Components[i]->ClassNameIs("TGroupBox")||
				this->Components[i]->ClassNameIs("TEdit")||
				this->Components[i]->ClassNameIs("TUpDown")||
				this->Components[i]->ClassNameIs("TListBox")||
				this->Components[i]->ClassNameIs("TTrackBar")||
				this->Components[i]->ClassNameIs("TTabSheet"))
		{
			((TWinControl*)Components[i])->DoubleBuffered=true;
		}
	}

	BRRTemp=NULL;
	BRRTempSize=0;
	BRRTempAllocSize=0;

	memset(SPCMem,0,sizeof(SPCMem));
	memset(SPCTemp,0,sizeof(SPCTemp));

	SPCInstrumentsSize=0;
	SPCMusicSize=0;
	SPCEffectsSize=0;
	InstrumentsCount=0;
	SPCMusicLargestSize=0;

	ModuleFileName=SaveDialogModule->FileName+"."+SaveDialogModule->DefaultExt;

	UpdateSampleData=true;

	PrevMouseY=0;

	ModuleInit();
	ModuleClear();

	dir=ParamStr(0).SubString(0,ParamStr(0).LastDelimiter("\\/"));

	config_open((dir+CONFIG_NAME).c_str());

	WaveOutSampleRate =config_read_int("WaveOutSampleRate",44100);
	WaveOutBufferSize =config_read_int("WaveOutBufferSize",2048);
	WaveOutBufferCount=config_read_int("WaveOutBufferCount",8);

	midi_id=config_read_int("MidiInputDeviceID",0);

	PageStep=config_read_int("SongPageStep",DEFAULT_PAGE_ROWS);
	AutoStep=config_read_int("SongAutoStep",1);

	PaintBoxSong->Font->Name=config_read_string("SongFontName","Courier New");
	PaintBoxSong->Font->Size=config_read_int("SongFontSize",10);

	config_close();

	MusicActive=false;
	ShiftKeyIsDown=false;
	SongDoubleClick=false;

	InsListUpdate();
	SongListUpdate();
	SongUpdateControls();

	ResetCopyBuffer();

	Application->OnMessage=AppMessage;

	if(ParamStr(1)!="")
	{
		ModuleOpen(ParamStr(1));

		if(ParamStr(2).LowerCase()=="-e")
		{
			if(ParamStr(3)!="")
			{
				dir=ParamStr(3);

				if(dir.SubString(dir.Length()-1,1)!="\\"&&dir.SubString(dir.Length()-1,1)!="/") dir+="\\";
			}
			else
			{
				dir=ExtractFilePath(ParamStr(1));
			}

			ExportAll(dir);

			if(BRRTemp) free(BRRTemp);

			for(i=0;i<MAX_INSTRUMENTS;++i) if(insList[i].source) free(insList[i].source);

			exit(0);
		}
	}

	UpdateInfo(false);

	tuner_init();
	waveout_init(Handle,WaveOutSampleRate,WaveOutBufferSize,WaveOutBufferCount);

	MidiHandle=NULL;

	midi_all=midiInGetNumDevs();

	if(midi_all>0&&midi_id<midi_all)
	{
		if(midiInOpen(&MidiHandle,midi_id,(DWORD)Handle,NULL,CALLBACK_WINDOW)==MMSYSERR_NOERROR)
		{
			memset(MidiKeyState,0,sizeof(MidiKeyState));

			midiInStart(MidiHandle);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormClose(TObject *Sender, TCloseAction &Action)
{
	int i;

	waveout_shut();
	tuner_shut();

	SPCStop();

	if(MidiHandle)
	{
		midiInStop(MidiHandle);
		midiInReset(MidiHandle);
		midiInClose(MidiHandle);
	}

	if(BRRTemp) free(BRRTemp);

	for(i=0;i<MAX_INSTRUMENTS;++i) if(insList[i].source) free(insList[i].source);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExitClick(TObject *Sender)
{
	Close();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::EditInsNameChange(TObject *Sender)
{
	insList[InsCur].name=EditInsName->Text;

	InsListUpdate();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxInsMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	InsChange(ListBoxIns->ItemIndex);
	InsListUpdate();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOpenClick(TObject *Sender)
{
	if(!OpenDialogModule->Execute()) return;

	ModuleOpen(OpenDialogModule->FileName);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSaveClick(TObject *Sender)
{
	if(!SaveDialogModule->Execute()) return;

	ModuleSave(SaveDialogModule->FileName);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonImportWavClick(TObject *Sender)
{
	FILE *file;
	unsigned char *wave;
	int i,j,n,ptr,size,length,rate,channels,bits,samples;

	if(!OpenDialogWave->Execute()) return;

	file=fopen(OpenDialogWave->FileName.c_str(),"rb");

	if(!file) return;

	fseek(file,0,SEEK_END);

	size=ftell(file);

	fseek(file,0,SEEK_SET);

	wave=(unsigned char*)malloc(size);

	fread(wave,size,1,file);
	fclose(file);

	if(memcmp(wave,"RIFF",4))
	{
		Application->MessageBox("RIFF chunk not found","Error",MB_OK);
		free(wave);
		return;
	}

	if(memcmp(wave+8,"WAVEfmt ",8))
	{
		Application->MessageBox("WAVEfmt chunk not found","Error",MB_OK);
		free(wave);
		return;
	}

	if(rd_word_lh(wave+20)>1)
	{
		Application->MessageBox("Compressed files are not supported","Error",MB_OK);
		free(wave);
		return;
	}

	channels=rd_word_lh(wave+22);
	rate    =rd_dword_lh(wave+24);
	bits    =rd_word_lh(wave+34);

	ptr=36;

	while(ptr<size)
	{
		if(!memcmp(wave+ptr,"data",4)) break;

		++ptr;
	}

	if(ptr>=size)
	{
		Application->MessageBox("data chunk not found","Error",MB_OK);
		free(wave);
		return;
	}

	length=rd_dword_lh(wave+ptr+4);

	ptr+=8;

	samples=length/(bits/8)/channels;

	if(insList[InsCur].source) free(insList[InsCur].source);

	if(samples<16)
	{
		Application->MessageBox("Sample is too short","Error",MB_OK);
		free(wave);
		return;
	}

	insList[InsCur].source=(short*)malloc(samples*sizeof(short));
	insList[InsCur].source_length=samples;
	insList[InsCur].source_rate=rate;

	switch(bits)
	{
	case 8:
		{
			for(i=0;i<samples;++i)
			{
				n=0;

				for(j=0;j<channels;++j) n+=wave[ptr++];

				n/=channels;

				insList[InsCur].source[i]=(n-128)*256;
			}
		}
		break;

	case 16:
		{
			for(i=0;i<samples;++i)
			{
				n=0;

				for(j=0;j<channels;++j)
				{
					n+=rd_word_lh(wave+ptr);
					ptr+=2;
				}

				n/=channels;

				insList[InsCur].source[i]=n;
			}
		}
		break;

	case 24:
		{
			for(i=0;i<samples;++i)
			{
				n=0;

				for(j=0;j<channels;++j)
				{
					n+=(wave[ptr]+(wave[ptr+1]<<8)+(wave[ptr+2]<<16));
					ptr+=3;
				}

				n/=channels;

				insList[InsCur].source[i]=n/256;
			}
		}
		break;

	case 32:
		{
			for(i=0;i<samples;++i)
			{
				n=0;

				for(j=0;j<channels;++j)
				{
					n+=rd_dword_lh(wave+ptr);
					ptr+=4;
				}

				n/=channels;

				insList[InsCur].source[i]=n/65536;
			}
		}
		break;
	}

	insList[InsCur].wav_loop_start=-1;
	insList[InsCur].wav_loop_end  =-1;

	while(ptr<size)
	{
		if(!memcmp(wave+ptr,"smpl",4))
		{
			if(rd_dword_lh(wave+ptr+0x24)>0)
			{
				insList[InsCur].wav_loop_start=rd_dword_lh(wave+ptr+0x2c+0x08);
				insList[InsCur].wav_loop_end  =rd_dword_lh(wave+ptr+0x2c+0x0c);
			}

			break;
		}

		++ptr;
	}

	insList[InsCur].length=insList[InsCur].source_length;

	if(insList[InsCur].wav_loop_start>=0)
	{
		insList[InsCur].loop_start=insList[InsCur].wav_loop_start;
		insList[InsCur].loop_end  =insList[InsCur].wav_loop_end;

		if(insList[InsCur].loop_start>=insList[InsCur].length) insList[InsCur].loop_start=insList[InsCur].length-1;
		if(insList[InsCur].loop_end  >=insList[InsCur].length) insList[InsCur].loop_end  =insList[InsCur].length-1;
	}
	else
	{
		insList[InsCur].loop_start=0;
		insList[InsCur].loop_end  =insList[InsCur].length-1;
	}

	free(wave);

	insList[InsCur].name=ChangeFileExt(ExtractFileName(OpenDialogWave->FileName),"");

	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxADSRPaint(TObject *Sender)
{
	RenderADSR();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TrackBarARChange(TObject *Sender)
{
	insList[InsCur].env_ar=TrackBarAR->Position;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TrackBarDRChange(TObject *Sender)
{
	insList[InsCur].env_dr=TrackBarDR->Position;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TrackBarSLChange(TObject *Sender)
{
	insList[InsCur].env_sl=TrackBarSL->Position;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TrackBarSRChange(TObject *Sender)
{
	insList[InsCur].env_sr=TrackBarSR->Position;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TrackBarVolumeChange(TObject *Sender)
{
	insList[InsCur].source_volume=TrackBarVolume->Position;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopClick(TObject *Sender)
{
	insList[InsCur].loop_enable=SpeedButtonLoop->Down;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopToBeginClick(TObject *Sender)
{
	insList[InsCur].loop_start=0;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopToEndClick(TObject *Sender)
{
	insList[InsCur].loop_end=insList[InsCur].length-1;

	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::EditLengthChange(TObject *Sender)
{
	int len;

	if(TryStrToInt(EditLength->Text,len))
	{
		if(len>insList[InsCur].source_length) len=insList[InsCur].source_length;

		insList[InsCur].length=len;

		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::EditLoopStartChange(TObject *Sender)
{
	int start;

	if(TryStrToInt(EditLoopStart->Text,start))
	{
		insList[InsCur].loop_start=start;

		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopWavClick(TObject *Sender)
{
	if(insList[InsCur].wav_loop_start>=0)
	{
		insList[InsCur].loop_start=insList[InsCur].wav_loop_start;
		insList[InsCur].loop_end  =insList[InsCur].wav_loop_end;
	}

	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLengthToMaxClick(TObject *Sender)
{
	insList[InsCur].length=insList[InsCur].source_length;

	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::EditLoopEndChange(TObject *Sender)
{
	int end;

	if(TryStrToInt(EditLoopEnd->Text,end))
	{
		insList[InsCur].loop_end=end;

		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------


void __fastcall TFormMain::SpeedButtonResampleNearestClick(TObject *Sender)
{
	insList[InsCur].resample_type=((TSpeedButton*)Sender)->Tag;

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonInsDeleteClick(TObject *Sender)
{
	if(Application->MessageBox("Clear all settings of current instrument, are you sure?","Confirm",MB_YESNO)!=IDYES) return;

	InstrumentClear(InsCur);
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonInsMoveUpClick(TObject *Sender)
{
	static instrumentStruct temp;

	if(InsCur>0)
	{
		memcpy(&temp             ,&insList[InsCur-1],sizeof(instrumentStruct));
		memcpy(&insList[InsCur-1],&insList[InsCur  ],sizeof(instrumentStruct));
		memcpy(&insList[InsCur  ],&temp             ,sizeof(instrumentStruct));

		SwapInstrumentNumberAll(InsCur,InsCur+1);

		InsListUpdate();
		InsChange(InsCur-1);

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonInsMoveDownClick(TObject *Sender)
{
	static instrumentStruct temp;

	if(InsCur<MAX_INSTRUMENTS-1)
	{
		memcpy(&temp             ,&insList[InsCur+1],sizeof(instrumentStruct));
		memcpy(&insList[InsCur+1],&insList[InsCur  ],sizeof(instrumentStruct));
		memcpy(&insList[InsCur  ],&temp             ,sizeof(instrumentStruct));

		SwapInstrumentNumberAll(InsCur+1,InsCur+2);

		InsListUpdate();
		InsChange(InsCur+1);

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonInsLoadClick(TObject *Sender)
{
	FILE *file;
	char *text;
	int i,size;

	if(!OpenDialogInstrument->Execute()) return;

	for(i=0;i<OpenDialogInstrument->Files->Count;++i)
	{
		if(InsCur+i>=MAX_INSTRUMENTS) break;

		file=fopen(OpenDialogInstrument->Files->Strings[i].c_str(),"rb");

		if(!file) continue;

		fseek(file,0,SEEK_END);
		size=ftell(file);
		fseek(file,0,SEEK_SET);

		text=(char*)malloc(size);

		fread(text,size,1,file);
		fclose(file);

		gss_init(text,size);

		//check signature

		if(gss_find_tag(INSTRUMENT_SIGNATURE)<0)
		{
			free(text);
			continue;
		}

		InstrumentDataParse(0,InsCur+i);

		free(text);
	}

	InsListUpdate();
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonInsSaveClick(TObject *Sender)
{
	FILE *file;

	SaveDialogInstrument->FileName=insList[InsCur].name;

	if(!SaveDialogInstrument->Execute()) return;

	file=fopen(SaveDialogInstrument->FileName.c_str(),"wt");

	if(!file) return;

	fprintf(file,"%s\n\n",INSTRUMENT_SIGNATURE);

	InstrumentDataWrite(file,0,InsCur);

	fclose(file);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxSongPaint(TObject *Sender)
{
	TextFontWidth =PaintBoxSong->Canvas->TextWidth("1");
	TextFontHeight=PaintBoxSong->Canvas->TextHeight("1");

	RenderPattern();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxSongMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	int col,row,chn;

	if(!TextFontWidth||!TextFontHeight) return;

	col=X/TextFontWidth;
	row=Y/TextFontHeight;

	if(col>=0&&col<(1+4+1+2+1+11*8))
	{
		if(!row)//pattern header
		{
			chn=(col-9)/11;

			if(col>=9)
			{
				if(Shift.Contains(ssLeft))  ToggleChannelMute(chn,false);

				if(Shift.Contains(ssRight)) ToggleChannelMute(chn,true);
			}
		}
		else//song text
		{
			col=PatternColumnOffsets[col];
			row=PatternScreenToActualRow(row-1);

			if(col>=0&&row>=0&&row<MAX_ROWS)
			{
				if(col>=1) StartSelection(col,row);

				SongMoveCursor(row,col,false);
				ColCurPrev=-1;

				RenderPattern();
			}
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxSongMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int col,row,chn;
	bool update;

	if(!TextFontWidth||!TextFontHeight) return;

	update=false;

	if(Shift.Contains(ssLeft))
	{
		col=X/TextFontWidth;
		row=Y/TextFontHeight;

		if(!TimerScrollDelay->Enabled)
		{
			if(row<3)
			{
				if(RowView)
				{
					--RowView;
					update=true;
					TimerScrollDelay->Enabled=true;
				}
			}

			if(row>=(PaintBoxSong->Height/TextFontHeight)-2)
			{
				if(RowView<MAX_ROWS-1)
				{
					++RowView;
					update=true;
					TimerScrollDelay->Enabled=true;
				}
			}
		}

		if(col>=0&&col<(1+4+1+2+1+11*8)&&row>0)
		{
			col=PatternColumnOffsets[col];
			row=PatternScreenToActualRow(row-1);

			if(col>=1&&row>=0&&row<MAX_ROWS)
			{
				SelectionColE=col;
				SelectionRowE=row;

				update=true;
			}
		}
	}

	if(update)
	{
		UpdateSelection();
		RenderPattern();
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSongPlayStartClick(TObject *Sender)
{
	SPCPlaySong(&songList[SongCur],0,true,-1);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSongPlayCurClick(TObject *Sender)
{
	SPCPlaySong(&songList[SongCur],RowCur,true,-1);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSongStopClick(TObject *Sender)
{
	SPCStop();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSongClearClick(TObject *Sender)
{
	if(Application->MessageBox("Clear current song, are you sure?","Confirm",MB_YESNO)==IDYES) SongClear(SongCur);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MAutostepClick(TObject *Sender)
{
	++AutoStep;

	if(AutoStep>16) AutoStep=0;

	UpdateInfo(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::EditSongNameChange(TObject *Sender)
{
	songList[SongCur].name=EditSongName->Text;

	SongListUpdate();
	UpdateInfo(true);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxSongClick(TObject *Sender)
{
	SPCStop();
	SongChange(ListBoxSong->ItemIndex);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::CheckBoxEffectClick(TObject *Sender)
{
	songList[SongCur].effect=CheckBoxEffect->Checked;

	SongListUpdate();

	if(TabSheetSongList->Visible) ListBoxSong->SetFocus();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSongUpClick(TObject *Sender)
{
	static songStruct temp;

	if(SongCur>0)
	{
		memcpy(&temp               ,&songList[SongCur-1],sizeof(songStruct));
		memcpy(&songList[SongCur-1],&songList[SongCur  ],sizeof(songStruct));
		memcpy(&songList[SongCur  ],&temp               ,sizeof(songStruct));

		SongChange(SongCur-1);
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSongDownClick(TObject *Sender)
{
	static songStruct temp;

	if(SongCur<MAX_SONGS-1)
	{
		memcpy(&temp               ,&songList[SongCur+1],sizeof(songStruct));
		memcpy(&songList[SongCur+1],&songList[SongCur  ],sizeof(songStruct));
		memcpy(&songList[SongCur  ],&temp               ,sizeof(songStruct));

		SongChange(SongCur+1);
	}
}
//---------------------------------------------------------------------------
void __fastcall TFormMain::SpeedButtonLoopRampClick(TObject *Sender)
{
	insList[InsCur].ramp_enable=SpeedButtonLoopRamp->Down;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonDownsample1Click(TObject *Sender)
{
	insList[InsCur].downsample_factor=((TSpeedButton*)Sender)->Tag;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TabSheetInfoShow(TObject *Sender)
{
	CompileAllSongs();

	SPCCompile(&songList[SongCur],0,false,true,-1);

	RenderMemoryUse();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxMemoryUsePaint(TObject *Sender)
{
	RenderMemoryUse();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportSPCClick(TObject *Sender)
{
	FILE *file;

	if(!SaveDialogExportSPC->Execute()) return;

	SPCCompile(&songList[SongCur],0,false,false,-1);

	file=fopen(SaveDialogExportSPC->FileName.c_str(),"wb");

	if(!file) return;

	fwrite(SPCTemp,sizeof(SPCTemp),1,file);
	fclose(file);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxSongDblClick(TObject *Sender)
{
	SPCPlaySong(&songList[SongCur],0,false,-1);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportClick(TObject *Sender)
{
	AnsiString dir;

	if(!SelectDirectory(dir,TSelectDirOpts(),0)) return;

	if(!ExportAll(dir+"\\")) Application->MessageBox("Some export error","Error",MB_OK);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOctave1Click(TObject *Sender)
{
	OctaveCur=((TMenuItem*)Sender)->Tag;

	UpdateInfo(false);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxInsDblClick(TObject *Sender)
{
	SPCPlayNote(2+12*5,InsCur);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportSPCBankClick(TObject *Sender)
{
	FILE *file;
	static unsigned char smph[22];
	int ins,volume;

	if(!OpenDialogImportSPCBank->Execute()) return;

	file=fopen(OpenDialogImportSPCBank->FileName.c_str(),"rb");

	if(!file) return;

	for(ins=0;ins<MAX_INSTRUMENTS;++ins) InstrumentClear(ins);

	for(ins=0;ins<64;++ins)
	{
		fread(smph,sizeof(smph),1,file);

		insList[ins].source_length=rd_dword_lh(&smph[0]);
		insList[ins].length       =rd_dword_lh(&smph[4]);
		insList[ins].loop_start   =rd_dword_lh(&smph[8]);
		insList[ins].loop_end     =insList[ins].length-1;
		insList[ins].source_rate  =rd_word_lh(&smph[12]);
		insList[ins].env_ar       =smph[14];
		insList[ins].env_dr       =smph[15];
		insList[ins].env_sl       =smph[16];
		insList[ins].env_sr       =smph[17];
		insList[ins].loop_enable  =smph[18]?true:false;
		insList[ins].ramp_enable  =(smph[18]&2)?true:false;

		volume=smph[19];

		if(volume>128) volume=128+(volume-128)/2;

		insList[ins].source_volume=volume;

		if(insList[ins].source_length)
		{
			insList[ins].source=(short*)malloc(insList[ins].source_length*sizeof(short));

			fread(insList[ins].source,insList[ins].source_length*sizeof(short),1,file);

			insList[ins].name="imported "+IntToStr(ins+1);
		}
	}

	fclose(file);

	UpdateAll();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportXMSongClick(TObject *Sender)
{
	AnsiString err;

	if(!OpenDialogImportXM->Execute()) return;

	err=ImportXM(OpenDialogImportXM->FileName,true);

	if(err!="") Application->MessageBox(err.c_str(),"Error",MB_OK);

	CompileAllSongs();
	UpdateAll();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportXMSoundEffectsClick(TObject *Sender)
{
	AnsiString err;

	if(!OpenDialogImportXM->Execute()) return;

	err=ImportXM(OpenDialogImportXM->FileName,false);

	if(err!="") Application->MessageBox(err.c_str(),"Error",MB_OK);

	CompileAllSongs();
	UpdateAll();
}
//---------------------------------------------------------------------------
void __fastcall TFormMain::MCleanupInstrumentsClick(TObject *Sender)
{
	bool use[MAX_INSTRUMENTS];
	int ins,row,chn,song,cnt;

	if(Application->MessageBox("Remove all unused instruments from the project?","Confirm",MB_YESNO)!=IDYES) return;

	for(ins=0;ins<MAX_INSTRUMENTS;++ins) use[ins]=false;

	for(song=0;song<MAX_SONGS;++song)
	{
		for(row=0;row<songList[song].length;++row)
		{
			for(chn=0;chn<8;++chn)
			{
				use[songList[song].row[row].chn[chn].instrument]=true;
			}
		}
	}

	cnt=0;

	for(ins=0;ins<MAX_INSTRUMENTS;++ins)
	{
		if(!use[ins])
		{
			if(insList[ins-1].source)
			{
				InstrumentClear(ins-1);

				++cnt;
			}
		}
	}

	UpdateAll();

	UpdateSampleData=true;

	Application->MessageBox((IntToStr(cnt)+" instrument(s) removed").c_str(),"Done",MB_OK);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TimerScrollDelayTimer(TObject *Sender)
{
	TimerScrollDelay->Enabled=false;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::EditLengthKeyPress(TObject *Sender, char &Key)
{
	if(!((Key>='0'&&Key<='9')||Key==VK_DELETE||Key==VK_BACK)) Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSampleLengthIncMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(insList[InsCur].length<insList[InsCur].source_length)
	{
		++insList[InsCur].length;
		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonSampleLengthDecMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(insList[InsCur].length)
	{
		--insList[InsCur].length;
		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopStartIncMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(insList[InsCur].loop_start<insList[InsCur].source_length-1)
	{
		++insList[InsCur].loop_start;
		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopStartDecMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(insList[InsCur].loop_start)
	{
		--insList[InsCur].loop_start;
		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopEndIncMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(insList[InsCur].loop_end<insList[InsCur].source_length-1)
	{
		++insList[InsCur].loop_end;
		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopEndDecMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(insList[InsCur].loop_end)
	{
		--insList[InsCur].loop_end;
		InsUpdateControls();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxInsDrawItem(TWinControl *Control, int Index,
TRect &Rect, TOwnerDrawState State)
{
	TListBox *lb=(TListBox*)Control;
	TCanvas *c=lb->Canvas;

	if(State.Contains(odSelected))
	{
		c->Brush->Color=clHighlight;
		c->Font ->Color=clHighlightText;
	}
	else
	{
		c->Brush->Color=clWhite;

		if(insList[Index].source) c->Font->Color=clBlack; else c->Font->Color=clSilver;
	}

	c->FillRect(Rect);
	c->TextRect(Rect,Rect.Left,Rect.Top,lb->Items->Strings[Index]);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::ListBoxSongDrawItem(TWinControl *Control, int Index,
TRect &Rect, TOwnerDrawState State)
{
	TListBox *lb=(TListBox*)Control;
	TCanvas *c=lb->Canvas;

	if (State.Contains(odSelected))
	{
		c->Brush->Color=clHighlight;
		c->Font ->Color=clHighlightText;
	}
	else
	{
		c->Brush->Color=clWhite;

		if(!SongIsEmpty(&songList[Index])) c->Font->Color=clBlack; else c->Font->Color=clSilver;
	}

	c->FillRect(Rect);
	c->TextRect(Rect,Rect.Left,Rect.Top,lb->Items->Strings[Index]);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSongCleanUpClick(TObject *Sender)
{
	SetUndo();
	SongCleanUp(&songList[SongCur]);
	UpdateAll();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExtractSourceWaveClick(TObject *Sender)
{
	FILE *file;
	unsigned char wave[44];
	int size;

	SaveDialogExportWav->FileName=insList[InsCur].name;

	if(!SaveDialogExportWav->Execute()) return;

	file=fopen(SaveDialogExportWav->FileName.c_str(),"wb");

	if(!file) return;

	size=44+insList[InsCur].source_length*2;

	memcpy(wave,"RIFF",4);//riff signature
	wr_dword_lh(&wave[4],size-8);//filesize-8
	memcpy(&wave[8],"WAVEfmt ",8);//format id
	wr_dword_lh(&wave[16],16);//header size
	wr_word_lh(&wave[20],1);//PCM
	wr_word_lh(&wave[22],1);//mono
	wr_dword_lh(&wave[24],insList[InsCur].source_rate);//samplerate
	wr_dword_lh(&wave[28],insList[InsCur].source_rate);//byterate (samplerate*channels*bytespersample)
	wr_dword_lh(&wave[32],2);//channels*bytespersample
	wr_dword_lh(&wave[34],16);//bits per sample
	memcpy(&wave[36],"data",4);//data id
	wr_dword_lh(&wave[40],insList[InsCur].source_length*2);//data size in bytes

	fwrite(wave,sizeof(wave),1,file);
	fwrite(insList[InsCur].source,insList[InsCur].source_length*2,1,file);

	fclose(file);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::StaticTextEQLowMouseMove(TObject *Sender,
TShiftState Shift, int X, int Y)
{
	int delta;

	if(Shift.Contains(ssLeft))
	{
		delta=PrevMouseY-Y;

		if(Sender==(TObject*)StaticTextEQLow ) insList[InsCur].eq_low +=delta;
		if(Sender==(TObject*)StaticTextEQMid ) insList[InsCur].eq_mid +=delta;
		if(Sender==(TObject*)StaticTextEQHigh) insList[InsCur].eq_high+=delta;

		if(insList[InsCur].eq_low <-64) insList[InsCur].eq_low =-64;
		if(insList[InsCur].eq_low > 64) insList[InsCur].eq_low = 64;
		if(insList[InsCur].eq_mid <-64) insList[InsCur].eq_mid =-64;
		if(insList[InsCur].eq_mid > 64) insList[InsCur].eq_mid = 64;
		if(insList[InsCur].eq_high<-64) insList[InsCur].eq_high=-64;
		if(insList[InsCur].eq_high> 64) insList[InsCur].eq_high= 64;

		PrevMouseY=Y;

		UpdateEQ();

		UpdateSampleData=true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::StaticTextEQLowMouseDown(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(Shift.Contains(ssLeft)) PrevMouseY=Y;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonEQResetClick(TObject *Sender)
{
	insList[InsCur].eq_low=0;
	insList[InsCur].eq_mid=0;
	insList[InsCur].eq_high=0;

	UpdateEQ();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportMidiClick(TObject *Sender)
{
	songStruct *s;
	FILE *file;
	unsigned char *midi;
	int ptr,len,size,track_size,track_end,track,tracks,division,time,delta,event_type,meta_event_type,channel,param1,param2,row,note,status,prev_status;
	double bpm,prev_bpm,speed;
	bool first[8];

	if(!OpenDialogImportMidi->Execute()) return;

	file=fopen(OpenDialogImportMidi->FileName.c_str(),"rb");

	if(!file) return;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	midi=(unsigned char*)malloc(size);
	fread(midi,size,1,file);
	fclose(file);

	if(memcmp(midi,"MThd",4)||rd_dword_hl(midi+4)!=6)
	{
		Application->MessageBox("Not a MIDI file","Error",MB_OK);
		free(midi);
		return;
	}

	SongClear(SongCur);

	s=&songList[SongCur];

	s->row[0].speed=DEFAULT_SPEED;

	tracks  =rd_word_hl(midi+10);
	division=rd_word_hl(midi+12);

	ptr=14;

	for(track=0;track<tracks;++track)
	{
		if(memcmp(midi+ptr,"MTrk",4))
		{
			Application->MessageBox("MIDI track data not found","Error",MB_OK);
			free(midi);
			return;
		}

		track_size=rd_dword_hl(midi+ptr+4);

		ptr+=8;//skip track header

		track_end=ptr+track_size;

		time=0;
		bpm=120;
		prev_bpm=bpm;
		prev_status=-1;

		for(channel=0;channel<8;++channel) first[channel]=true;

		while(ptr<track_end)
		{
			delta=rd_var_length(midi,ptr);//advances ptr

			time+=delta;

			row=time*4/division;//not clear why 4, a quarter note, perhaps?

			status=midi[ptr];

			if(status<0x80&&prev_status>=0)//running status check
			{
				event_type=prev_status>>4;
				channel   =prev_status&15;
			}
			else
			{
				event_type=status>>4;
				channel   =status&15;

				++ptr;

				prev_status=status;
			}

			switch(event_type)
			{
			case 0x08://note off
				{
					ptr+=2;

					if(channel<8)
					{
						if(!s->row[row].chn[channel].note) s->row[row].chn[channel].note=1;
					}
				}
				break;

			case 0x09://note on
				{
					param1=midi[ptr++];//note
					param2=midi[ptr++];//velocity

					if(channel<8)
					{
						if(!param2)//zero velocity often used for keyoff
						{
							if(channel<8)
							{
								if(!s->row[row].chn[channel].note) s->row[row].chn[channel].note=1;
							}
						}
						else
						{
							note=2+param1;

							if(note>=2&&note<2+96)
							{
								s->row[row].chn[channel].note=note;

								if(first[channel])
								{
									s->row[row].chn[channel].instrument=channel+1;
									first[channel]=false;
								}
							}
						}
					}

					if(channel==9&&param2)//keyon on the drum channel
					{
						switch(param1)
						{
						case 35://bass drums
						case 36:
							s->row[row].chn[7].note=62;
							s->row[row].chn[7].instrument=10;
							break;

						case 38://snares
						case 39:
						case 40:
							s->row[row].chn[7].note=62;
							s->row[row].chn[7].instrument=11;
							break;

						case 41://toms
						case 43:
						case 45:
						case 47:
						case 48:
						case 50:
							if(!s->row[row].chn[7].note)//lower priority than kick and snare
							{
								s->row[row].chn[7].note=62;
								s->row[row].chn[7].instrument=12;
							}
							break;

						default://everything else, mostly hats
							s->row[row].chn[6].note=62;
							s->row[row].chn[6].instrument=13;
						}
					}
				}
				break;

			case 0x0a://note aftertouch
			case 0x0b://controller
			case 0x0e://pitch bend
				ptr+=2;
				break;

			case 0x0f://meta event
				meta_event_type=midi[ptr++];

				if(meta_event_type==0x51)
				{
					bpm=60000000.0/double((midi[ptr+1]<<16)+(midi[ptr+2]<<8)+midi[ptr+3]);
				}

				len=rd_var_length(midi,ptr);
				ptr+=len;
				break;

			default:
				++ptr;
				break;
			}

			if(prev_bpm!=bpm)
			{
				speed=UPDATE_RATE_HZ/(bpm*(1.0/60.0)*4);//bpm*(1/60) is bpm to hz, 4 is a quarter note?

				if(speed<1 ) speed=1;
				if(speed>99) speed=99;

				s->row[row].speed=(int)speed;

				prev_bpm=bpm;
			}
		}
	}

	free(midi);

	CompileAllSongs();
	UpdateAll();
}

//---------------------------------------------------------------------------

void __fastcall TFormMain::MTransposeDialogClick(TObject *Sender)
{
	int ins;

	FormTranspose->Caption="Transpose";
	FormTranspose->UpDownValue->Min=-96;
	FormTranspose->UpDownValue->Max= 96;
	FormTranspose->UpDownValue->Position=0;
	FormTranspose->LabelHint->Caption="Semitones:";
	FormTranspose->ShowModal();

	if(!FormTranspose->Confirm) return;

	if(FormTranspose->RadioButtonAllInstruments->Checked) ins=0; else ins=InsCur+1;

	Transpose(FormTranspose->UpDownValue->Position,FormTranspose->RadioButtonBlock->Checked,FormTranspose->RadioButtonCurrentSong->Checked,FormTranspose->RadioButtonCurrentChannel->Checked,ins);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MInstrumentReplaceClick(TObject *Sender)
{
	int from,to;

	FormReplace->UpDownTo->Position=InsCur+1;

	FormReplace->ShowModal();

	if(!FormReplace->Replace) return;

	from=FormReplace->UpDownFrom->Position;
	to=FormReplace->UpDownTo->Position;

	ReplaceInstrument(FormReplace->RadioButtonBlock->Checked,FormReplace->RadioButtonCurrentSong->Checked,FormReplace->RadioButtonCurrentChannel->Checked,from,to);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TabSheetSongListEnter(TObject *Sender)
{
	CompileAllSongs();//get actual sizes of all songs to display in the list
	SongListUpdate();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MExportAndSaveClick(TObject *Sender)
{
	AnsiString dir;

	if(!SaveDialogModule->Execute()) return;

	ModuleSave(SaveDialogModule->FileName);

	dir=ExtractFilePath(SaveDialogModule->FileName);

	if(!ExportAll(dir)) Application->MessageBox("Some export error","Error",MB_OK);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxSongMouseUp(TObject *Sender,
TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if(SongDoubleClick)
	{
		if(Y/TextFontHeight>=1)
		{
			SetChannelSelection(true,Shift.Contains(ssShift)?false:true);
		}

		SongDoubleClick=false;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::PaintBoxSongDblClick(TObject *Sender)
{
	SongDoubleClick=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MImportFamiTrackerClick(TObject *Sender)
{
	static unsigned char order[256][5];
	const char export_id[]="# FamiTracker text export";
	FILE *file;
	char *text;
	int c,chn,pos,ptr,ptn,val,sub,note,vol,srow,drow,size,order_len,pattern_len,loop_pos;
	bool cut,sfx;
	AnsiString temp;

	struct rowStruct {
		unsigned char chn_note[5];
		unsigned char chn_vol[5];
		unsigned char speed;
	};

	struct patternStruct {
		rowStruct row[256];
		int length;
	};

	struct songStruct {
		patternStruct pattern[256];
	};

	songStruct *song;
	rowStruct *row;

	if(!OpenDialogImportFTM->Execute()) return;

	file=fopen(OpenDialogImportFTM->FileName.c_str(),"rt");

	FormSubSong->Import=false;
	FormSubSong->ShowModal();

	if(!file) return;

	fseek(file,0,SEEK_END);
	size=ftell(file);
	fseek(file,0,SEEK_SET);

	text=(char*)malloc(size);

	fread(text,size,1,file);
	fclose(file);

	if(memcmp(text,export_id,strlen(export_id)))
	{
		Application->MessageBox("Not a FamiTracker text export file","Error",MB_OK);
		free(text);
	}

	gss_init(text,size);

	ptr=0;

	for(sub=0;sub<FormSubSong->UpDownSubSong->Position;++sub)
	{
		while(ptr<size)
		{
			if(!memcmp(&text[ptr],"TRACK",5))
			{
				ptr+=5;
				break;
			}

			++ptr;
		}
	}

	if(ptr<0)
	{
		Application->MessageBox("Can't find TRACK section","Error",MB_OK);
		free(text);
	}

	pattern_len=0;

	while(ptr<size)
	{
		if(text[ptr]>='0'&&text[ptr]<='9')
		{
			while(1)
			{
				c=text[ptr++];

				if(!(c>='0'&&c<='9')) break;

				pattern_len=pattern_len*10+(c-'0');
			}

			break;
		}

		++ptr;
	}

	while(ptr<size) if(memcmp(&text[ptr],"ORDER",5)) ++ptr; else break;

	if(ptr<0)
	{
		Application->MessageBox("Can't find ORDER section","Error",MB_OK);
		free(text);
	}

	order_len=0;

	while(1)
	{
		while(ptr<size) if(text[ptr]!=':'&&text[ptr]>=' ') ++ptr; else break;

		if(text[ptr]!=':') break;

		ptr+=2;

		for(chn=0;chn<5;++chn)
		{
			order[order_len][chn]=(gss_hex_to_byte(text[ptr+0])<<4)|gss_hex_to_byte(text[ptr+1]);

			if(chn<4) ptr+=3; else ptr+=2;
		}

		while(ptr<size) if(text[ptr]<' ') ++ptr; else break;

		++order_len;

		if(text[ptr]!='O') break;
	}

	song=(songStruct*)malloc(sizeof(songStruct));

	memset(song,0,sizeof(songStruct));

	loop_pos=0;

	while(ptr<size)
	{
		if(!memcmp(&text[ptr],"TRACK",5)) break;

		if(memcmp(&text[ptr],"PATTERN",7))
		{
			++ptr;
			continue;
		}

		ptn=(gss_hex_to_byte(text[ptr+8])<<4)+gss_hex_to_byte(text[ptr+9]);

		ptr+=10;

		song->pattern[ptn].length=0;

		cut=false;

		for(srow=0;srow<pattern_len;++srow)
		{
			while(ptr<size) if(text[ptr]<' ') ++ptr; else break;

			if(memcmp(&text[ptr],"ROW",3)) break;

			row=&song->pattern[ptn].row[srow];

			for(chn=0;chn<5;++chn)
			{
				while(ptr<size) if(text[ptr]!=':') ++ptr; else break;

				note=0;

				switch(text[ptr+2])
				{
				case '-': note=1; break;
				case 'C': note=2; break;
				case 'D': note=4; break;
				case 'E': note=6; break;
				case 'F': note=7; break;
				case 'G': note=9; break;
				case 'A': note=11; break;
				case 'B': note=13; break;
				}

				if(note>1)
				{
					if(text[ptr+3]=='#') ++note;

					note=note+(text[ptr+4]-'0')*12;
				}

				vol=0;
				
				if(text[ptr+9]>='0'&&text[ptr+9]<='9') vol=text[ptr+9]-'0';
				if(text[ptr+9]>='A'&&text[ptr+9]<='F') vol=text[ptr+9]-'A'+10;
				
				val=(gss_hex_to_byte(text[ptr+12])<<4)+gss_hex_to_byte(text[ptr+13]);

				switch(text[ptr+11])
				{
				case 'B': cut=true; loop_pos=val; break;
				case 'C': cut=true; break;
				case 'D': if(!val) cut=true; break;
				case 'F': row->speed=160*val/60; break;
				}

				row->chn_note[chn]=note;
				row->chn_vol [chn]=vol;

				ptr+=15;
			}

			++song->pattern[ptn].length;

			if(cut) break;
		}
	}

	free(text);

	//convert

	temp=songList[SongCur].name;
	sfx=songList[SongCur].effect;

	SongClear(SongCur);

	songList[SongCur].name=temp;
	songList[SongCur].effect=sfx;

	drow=0;

	for(pos=0;pos<order_len;++pos)
	{
		if(pos==loop_pos) songList[SongCur].loop_start=drow;

		songList[SongCur].row[drow].marker=true;

		for(srow=0;srow<song->pattern[order[pos][0]].length;++srow)
		{
			for(chn=0;chn<5;++chn)
			{
				row=&song->pattern[order[pos][chn]].row[srow];

				if(row->speed) songList[SongCur].row[drow].speed=row->speed;

				songList[SongCur].row[drow].chn[chn].note=row->chn_note[chn];

				if(row->chn_vol[chn]) songList[SongCur].row[drow].chn[chn].volume=row->chn_vol[chn]*99/15;
			}

			++drow;
		}
	}

	songList[SongCur].row[drow].marker=true;
	songList[SongCur].length=drow;

	free(song);

	CompileAllSongs();
	UpdateAll();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::SpeedButtonLoopUnrollClick(TObject *Sender)
{
	insList[InsCur].loop_unroll=SpeedButtonLoopUnroll->Down;
	InsUpdateControls();

	UpdateSampleData=true;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MSongScaleVolumeClick(TObject *Sender)
{
	int ins;

	FormTranspose->Caption="Scale volume";
	FormTranspose->UpDownValue->Min=0;
	FormTranspose->UpDownValue->Max=1000;
	FormTranspose->UpDownValue->Position=100;
	FormTranspose->LabelHint->Caption="Percents:";
	FormTranspose->ShowModal();

	if(!FormTranspose->Confirm) return;

	if(FormTranspose->RadioButtonAllInstruments->Checked) ins=0; else ins=InsCur+1;

	ScaleVolume(FormTranspose->UpDownValue->Position,FormTranspose->RadioButtonBlock->Checked,FormTranspose->RadioButtonCurrentSong->Checked,FormTranspose->RadioButtonCurrentChannel->Checked,ins);
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MOutputMonitorClick(TObject *Sender)
{
	if(FormOutputMonitor->Visible) FormOutputMonitor->Hide(); else FormOutputMonitor->Show();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::TimerOutputMonitorTimer(TObject *Sender)
{
	AnsiString str;
	const char *note;
	int cents;

	tuner_set_active(FormOutputMonitor->Visible);

	note =tuner_get_note_name();
	cents=tuner_get_detune();

	if(note)
	{
		FormOutputMonitor->LabelNote ->Caption=AnsiString(note);

		if(cents>0) str="+"; else str="";

		FormOutputMonitor->LabelCents->Caption=str+IntToStr(cents)+" cents";
	}
	else
	{
		FormOutputMonitor->LabelNote ->Caption="---";
		FormOutputMonitor->LabelCents->Caption="";
	}

	FormOutputMonitor->OutL=WSTREAMER.peakL;
	FormOutputMonitor->OutR=WSTREAMER.peakR;

	if(FormOutputMonitor->OutL>FormOutputMonitor->PeakL)
	{
		FormOutputMonitor->PeakL=FormOutputMonitor->OutL;
	}
	else
	{
		FormOutputMonitor->PeakL-=512;

		if(FormOutputMonitor->PeakL<0) FormOutputMonitor->PeakL=0;
	}

	if(FormOutputMonitor->OutR>FormOutputMonitor->PeakR)
	{
		FormOutputMonitor->PeakR=FormOutputMonitor->OutR;
	}
	else
	{
		FormOutputMonitor->PeakR-=512;

		if(FormOutputMonitor->PeakR<0) FormOutputMonitor->PeakR=0;
	}

	FormOutputMonitor->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::EditSongNameKeyPress(TObject *Sender, char &Key)
{
	if(Key==VK_RETURN)
	{
		if(TabSheetSongList->Visible) ListBoxSong->SetFocus();

		Key=0;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::MNewClick(TObject *Sender)
{
	if(Application->MessageBox("All current data wiil be lost. Are you sure you want to start a new project?","Confirm",MB_YESNO)!=ID_YES) return;

	ModuleInit();
	ModuleClear();

	SPCCompile(&songList[0],0,false,false,-1);
	CompileAllSongs();
	UpdateAll();
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::EditInsNameKeyPress(TObject *Sender, char &Key)
{
	if(Key==VK_RETURN)
	{
		if(TabSheetInstruments->Visible) ListBoxIns->SetFocus();

		Key=0;
	}
}
//---------------------------------------------------------------------------

