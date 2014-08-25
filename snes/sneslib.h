//last updated 24.08.14

#include <string.h>	//for memcpy

//#define MULTITAP_SUPPORT

/*types*/

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef NULL
#define NULL	0
#endif

/*SNES standart hardware registers*/

#define INIDISP(x)			*((unsigned char*)0x2100)=(x);
#define OBSEL(x)			*((unsigned char*)0x2101)=(x);
#define OAM_ADDR(x)			*((unsigned  int*)0x2102)=(x);
#define BGMODE(x)			*((unsigned char*)0x2105)=(x);
#define MOSAIC(x)			*((unsigned char*)0x2106)=(x);
#define BG1SC(x)			*((unsigned char*)0x2107)=(x);
#define BG2SC(x)			*((unsigned char*)0x2108)=(x);
#define BG3SC(x)			*((unsigned char*)0x2109)=(x);
#define BG4SC(x)			*((unsigned char*)0x210a)=(x);
#define BG12NBA(x)			*((unsigned char*)0x210b)=(x);
#define BG34NBA(x)			*((unsigned char*)0x210c)=(x);
#define BG1HOFFS(x)			*((unsigned char*)0x210d)=(x);
#define BG1VOFFS(x)			*((unsigned char*)0x210e)=(x);
#define BG2HOFFS(x)			*((unsigned char*)0x210f)=(x);
#define BG2VOFFS(x)			*((unsigned char*)0x2110)=(x);
#define BG3HOFFS(x)			*((unsigned char*)0x2111)=(x);
#define BG3VOFFS(x)			*((unsigned char*)0x2112)=(x);
#define BG4HOFFS(x)			*((unsigned char*)0x2113)=(x);
#define BG4VOFFS(x)			*((unsigned char*)0x2114)=(x);
#define VMAIN(x)			*((unsigned char*)0x2115)=(x);
#define VRAM_ADDR(x)		*((unsigned  int*)0x2116)=(x);
#define M7SEL(x)			*((unsigned char*)0x211a)=(x);
#define M7A(x)				{ *((unsigned char*)0x211b)=(x)&255; \
							  *((unsigned char*)0x211b)=(x)>>8; }
#define M7B(x)				{ *((unsigned char*)0x211c)=(x)&255; \
							  *((unsigned char*)0x211c)=(x)>>8; }
#define M7C(x)				{ *((unsigned char*)0x211d)=(x)&255; \
							  *((unsigned char*)0x211d)=(x)>>8; }
#define M7D(x)				{ *((unsigned char*)0x211e)=(x)&255; \
							  *((unsigned char*)0x211e)=(x)>>8; }
#define M7X(x)				{ *((unsigned char*)0x211f)=(x)&255; \
							  *((unsigned char*)0x211f)=(x)>>8; }
#define M7Y(x)				{ *((unsigned char*)0x2120)=(x)&255; \
							  *((unsigned char*)0x2120)=(x)>>8; }
#define CGADD(x)			*((unsigned char*)0x2121)=(x);
#define CGDATA(x)			*((unsigned char*)0x2122)=(x);
#define W12SEL(x)			*((unsigned char*)0x2123)=(x);
#define W34SEL(x)			*((unsigned char*)0x2124)=(x);
#define WOBJSEL(x)			*((unsigned char*)0x2125)=(x);
#define WH0(x)				*((unsigned char*)0x2126)=(x);
#define WH1(x)				*((unsigned char*)0x2127)=(x);
#define WH2(x)				*((unsigned char*)0x2128)=(x);
#define WH3(x)				*((unsigned char*)0x2129)=(x);
#define WBGLOG(x)			*((unsigned char*)0x212a)=(x);
#define WOBJLOG(x)			*((unsigned char*)0x212b)=(x);
#define TM(x)				*((unsigned char*)0x212c)=(x);
#define TS(x)				*((unsigned char*)0x212d)=(x);
#define TMW(x)				*((unsigned char*)0x212e)=(x);
#define TSW(x)				*((unsigned char*)0x212f)=(x);
#define CGWSEL(x)			*((unsigned char*)0x2130)=(x);
#define CGADSUB(x)			*((unsigned char*)0x2131)=(x);
#define COLDATA(x)			*((unsigned char*)0x2132)=(x);
#define SETINI(x)			*((unsigned char*)0x2133)=(x);
#define STAT78()			*((unsigned char*)0x213f)

#define WRAM_ADDR(x)		{ *((unsigned char*)0x2181)= (x)     &0xff; \
							  *((unsigned char*)0x2182)=((x)>>8 )&0xff; \
							  *((unsigned char*)0x2183)=((x)>>16)&0xff; }

#define NMITIMEEN(x)		*((unsigned char*)0x4200)=(x);
#define WRIO(x)				*((unsigned char*)0x4201)=(x);
#define WRDIVA(x)			*((unsigned  int*)0x4204)=(x);
#define WRDIVB(x)			*((unsigned char*)0x4206)=(x);
#define MEMSEL(x)			*((unsigned char*)0x420d)=(x);
#define HVBJOY()			*((unsigned char*)0x4212)
#define RDDIV()				*((unsigned  int*)0x4214)
#define RDMPY()				*((unsigned  int*)0x4216)
#define JOY_RD(x)			 ((unsigned  int*)0x4218)[(x)]
#define JOYSER0_WR(x)		*((unsigned char*)0x4016)=(x);
#define JOYSER_RD(x)		 ((unsigned char*)0x4016)[(x)]

#define DMA_TYPE(chn,x)		*((unsigned int*)(0x4300|((chn)<<4)))=(x);
#define DMA_ADDR(chn,x)		*((void**)       (0x4302|((chn)<<4)))=(void*)(x);
#define DMA_SIZE(chn,x)		*((unsigned int*)(0x4305|((chn)<<4)))=(x);

#define MDMAEN(x)			*((unsigned char*)0x420b)=(x);
#define HDMAEN(x)			*((unsigned char*)0x420c)=(x);

#define OBSEL_8_16			(0<<5)
#define OBSEL_8_32			(1<<5)
#define OBSEL_8_64			(2<<5)
#define OBSEL_16_32			(3<<5)
#define OBSEL_16_64			(4<<5)
#define OBSEL_32_64			(5<<5)

/*APU registers*/

#define APU0(x)		*((unsigned char*)0x2140)=(x);
#define APU1(x)		*((unsigned char*)0x2141)=(x);
#define APU2(x)		*((unsigned char*)0x2142)=(x);
#define APU3(x)		*((unsigned char*)0x2143)=(x);
#define APU01(x)	*((unsigned int*)0x2140)=(x);
#define APU23(x)	*((unsigned int*)0x2142)=(x);

#define APU0RD		*((unsigned char*)0x2140)
#define APU1RD		*((unsigned char*)0x2141)
#define APU2RD		*((unsigned char*)0x2142)
#define APU3RD		*((unsigned char*)0x2143)
#define APU01RD		*((unsigned int*)0x2140)
#define APU23RD		*((unsigned int*)0x2142)


/*DSP1 registers*/

#define DSP1_CMD(x)		*((unsigned char*)0x3f8000)=(x);
#define DSP1_PARAM(x)	*((unsigned int*) 0x3f8000)=(x);
#define DSP1_READ()		*((unsigned int*) 0x3f8000)
#define DSP1_WAIT()		while(!((*(unsigned int*)0x3fc000)&0x8000));

/*DSP1 commands*/

#define DSP1_MULTIPLY			0x00
#define DSP1_INVERSE			0x10
#define DSP1_TRIANGLE			0x04

#define DSP1_RADIUS				0x08
#define DSP1_RANGE				0x18
#define DSP1_DISTANCE			0x28

#define DSP1_ROTATE				0x0c
#define DSP1_POLAR				0x1c

#define DSP1_PARAMETER			0x02
#define DSP1_RASTER				0x0a//0x1a
#define DSP1_PROJECT			0x06
#define DSP1_TARGET				0x0e

#define DSP1_ATTITUDE			0x01//0x11,0x21
#define DSP1_OBJECTIVE			0x0d//0x1d,0x2d
#define DSP1_SUBJECTIVE			0x03//0x13,0x23
#define DSP1_SCALAR				0x0b//0x1b,0x2b

#define DSP1_GYRATE				0x14

/*aliases for pad buttons*/

#define PAD_R		0x0010
#define PAD_L		0x0020
#define PAD_X		0x0040
#define PAD_A		0x0080
#define PAD_RIGHT	0x0100
#define PAD_LEFT	0x0200
#define PAD_DOWN	0x0400
#define PAD_UP		0x0800
#define PAD_START	0x1000
#define PAD_SELECT	0x2000
#define PAD_Y		0x4000
#define PAD_B		0x8000

/*macro for sprite attributes*/

#define SPR_PAL(x)	(((x)&7)<<9)
#define SPR_PRI(x)	((x)<<12)
#define SPR_HFLIP	0x4000
#define SPR_VFLIP	0x8000
#define SPR_BANK1	0x0100

/*macro for background attributes*/

#define BG_PAL(x)	(((x)&7)<<10)
#define BG_PRI		0x2000
#define BG_HFLIP	0x4000
#define BG_VFLIP	0x8000

/*masks for R5G5B5 components*/

#define R_MASK		0x001f
#define G_MASK		0x03e0
#define B_MASK		0x7c00

#define RGB555(r,g,b)	((((b)<<10)&B_MASK)|(((g)<<5)&G_MASK)|((r)&R_MASK))

/*command codes for sound driver, nnnnmmmm where nnn is channel number and mmm is command code*/

#define SCMD_NONE				0x00	/*no command, means the APU is ready for a new command*/
#define SCMD_INITIALIZE			0x01	/*initialize DSP*/
#define SCMD_LOAD				0x02 	/*load new music data*/
#define SCMD_STEREO				0x03	/*change stereo sound mode, mono by default*/
#define SCMD_GLOBAL_VOLUME		0x04	/*set global volume*/
#define SCMD_CHANNEL_VOLUME		0x05	/*set channel volume*/
#define SCMD_MUSIC_PLAY 		0x06	/*start music*/
#define SCMD_MUSIC_STOP 		0x07	/*stop music*/
#define SCMD_MUSIC_PAUSE 		0x08	/*pause music*/
#define SCMD_SFX_PLAY			0x09	/*play sound effect*/
#define SCMD_STOP_ALL_SOUNDS	0x0a	/*stop all sounds*/
#define SCMD_STREAM_START		0x0b	/*start BRR streaming*/
#define SCMD_STREAM_STOP		0x0c	/*stop streaming*/
#define SCMD_STREAM_SEND		0x0d	/*send BRR block*/



/*variables for basic hardware abstract layer*/

static unsigned int  snes_pad_state  [8];
static unsigned int  snes_pad_prev   [8];
static unsigned int  snes_pad_trigger[8];
static unsigned int  snes_pad_source [8];
static unsigned int  snes_multitap_present[2];
static unsigned int  snes_pad_serial[2];
static unsigned int  snes_pad_count;

static unsigned int  snes_frame_cnt;
static unsigned int  snes_rand_seed1;
static unsigned int  snes_rand_seed2;
static unsigned int  snes_palette[256];
static unsigned char snes_oam[128*4+32];
static unsigned int  snes_ntsc;
static unsigned int  snes_skip_cnt;

/*sound driver variables*/

static unsigned int spc_music_load_adr;

static const unsigned char** spc_stream_block_list;
static const unsigned char*  spc_stream_block_src;

static unsigned int spc_stream_block;
static unsigned int spc_stream_block_ptr;
static unsigned int spc_stream_block_ptr_prev;
static unsigned int spc_stream_block_size;
static unsigned int spc_stream_block_repeat;
static unsigned int spc_stream_enable;
static unsigned int spc_stream_stop_flag;



/*hardware multiplication 8 by 8 bits*/

unsigned int hw_mul8(unsigned int a,unsigned int b);



/*16-bit random number generaton*/
/*implemented in sneslib.asm*/

void randomize(unsigned int n);

unsigned int rand(void);

/*
void randomize(unsigned int n)
{
	snes_rand_seed1=n>>8;
	snes_rand_seed2=n&255;
}

unsigned int rand(void)
{
	snes_rand_seed1+=(snes_rand_seed2>>1);
	snes_rand_seed2-=(15^snes_rand_seed1);

	return snes_rand_seed1;
}
*/


/*nmi handler*/

//implemented in sneslib.asm as a subroutine directly called from VBlank
/*
void nmi_handler(void)
{
	++snes_frame_cnt;
}
*/



/*enable or disable NMI*/

void nmi_enable(unsigned int enable)
{
	if(!enable)
	{
		NMITIMEEN(0);
	}
	else
	{
		NMITIMEEN(0x80|(snes_pad_count>2?0:1)); /*pad autoread is only enabled when there is no multitap connected*/
	}
}



/*wait for the next TV frame*/

void nmi_wait(void);


/*wait for the next virtual 50 Hz frame, skips every sixth frame on NTSC*/

void nmi_wait50(void)
{
	nmi_wait();

	if(snes_ntsc)
	{
		++snes_skip_cnt;

		if(snes_skip_cnt==5)
		{
			snes_skip_cnt=0;
			nmi_wait();
		}
	}
}



/*poll pad without using autoread, used for multitap support*/

#ifdef MULTITAP_SUPPORT

void pad_poll_oldser(unsigned int port)
{
	static unsigned int i,data,data1,data2;

	JOYSER0_WR(1);
	JOYSER0_WR(0);

	data1=0;
	data2=0;

	for(i=0;i<16;++i)
	{
		data=JOYSER_RD(port);

		data1=(data1<<1)|( data    &1);
		data2=(data2<<1)|((data>>1)&1);
	}

	snes_pad_serial[0]=data1;
	snes_pad_serial[1]=data2;
}

#endif



/*poll pad ports registers and remember their state*/
/*also polls buttons in trigger mode, bit is set only when a button*/
/*is pressed, and reset the next frame*/

void pad_read_ports(void)
{
	static unsigned int i,j,pad;

	/*wait while joypad is ready*/

	while(HVBJOY()&0x01);

#ifdef MULTITAP_SUPPORT

	if(snes_multitap_present[0]||snes_multitap_present[1])
	{
		/*poll all presented pads manually*/

		pad=0;

		for(i=0;i<2;++i)
		{
			WRIO(0xc0);

			pad_poll_oldser(i);

			if(!snes_multitap_present[i])
			{
				snes_pad_state[pad++]=snes_pad_serial[0];
			}
			else
			{
				snes_pad_state[pad++]=snes_pad_serial[0];
				snes_pad_state[pad++]=snes_pad_serial[1];

				WRIO(0x00);

				pad_poll_oldser(i);

				snes_pad_state[pad++]=snes_pad_serial[0];
				snes_pad_state[pad++]=snes_pad_serial[1];
			}
		}
	}
	else

#endif

	{
		/*use autoread to poll both pads*/

		snes_pad_state[0]=JOY_RD(0);
		snes_pad_state[1]=JOY_RD(1);
	}

	/*set trigger state*/

	for(i=0;i<snes_pad_count;++i)
	{
		j=snes_pad_state[i];

		snes_pad_trigger[i]=(j^snes_pad_prev[i])&j;
		snes_pad_prev   [i]=j;
	}
}



/*get previously remembered pad state*/

#define pad_poll(pad) (snes_pad_state[(pad)])

/*get previously remembered pad trigger state*/

#define pad_poll_trigger(pad) (snes_pad_trigger[(pad)])

/*check presence of multitap on a given port*/

#define pad_check_multitap(port) (snes_multitap_present[(port)])

/*check number of potentially connected pads*/

#define pad_number() (snes_pad_count)

/*check pad source, return 0xPC where P=port, C=controller*/

#define pad_source(pad) (snes_pad_source[(pad)])

/*set few entries of the RAM copy of the palette*/

#define set_palette(entry,len,src) { memcpy(&snes_palette[(entry)],(const unsigned int*)(src),(len)*sizeof(unsigned int)); }

/*set an entry of the RAM copy of the palette*/

#define set_color(entry,color) { snes_palette[(entry)]=(color); }



/*update the palette in the VRAM from the RAM copy*/

void update_palette(void);

/*copy data from RAM or ROM to the VRAM through DMA*/

void copy_to_vram(unsigned int adr,const unsigned char *src,unsigned int size);

/*copy data from VRAM to RAM or ROM through DMA*/

void copy_from_vram(unsigned int adr,const unsigned char *src,unsigned int size)
{
	volatile unsigned short dummy;

	VRAM_ADDR(adr);

	dummy=*(unsigned short*)0x2139;

	DMA_TYPE(7,0x3981);
	DMA_ADDR(7,(unsigned char*)src);
	DMA_SIZE(7,size);
	MDMAEN(1<<7);
}



/*fill VRAM with a value through DMA*/

void fill_vram(unsigned int adr,const unsigned char value,unsigned int size)
{
	VRAM_ADDR(adr);
	DMA_TYPE(7,0x1809);
	DMA_ADDR(7,&value);
	DMA_SIZE(7,size);
	MDMAEN(1<<7);
}



/*copy data from ROM to RAM or vice versa through DMA (RAM-RAM copy is not possible)*/

void copy_mem(unsigned char *dst,unsigned char *src,unsigned int size)
{
	WRAM_ADDR(((unsigned long)dst));
	DMA_TYPE(7,0x8000);
	DMA_ADDR(7,(unsigned char*)src);
	DMA_SIZE(7,size);
	MDMAEN(1<<7);
}



/*set brightness, also enables and disables rendering*/

void set_bright(unsigned int i)
{
	if(!i) i=0x80;
	INIDISP(i);

	nmi_enable(i);
}



/*set scroll registers for specified layer*/

void set_scroll(unsigned int layer,unsigned int x,unsigned int y);/*implemented in sneslib.asm*/
/*
void set_scroll(unsigned int layer,unsigned int x,unsigned int y)
{
	switch(layer)
	{
	case 0:
		BG1HOFFS(x&255);
		BG1HOFFS(x>>8);
		BG1VOFFS(y&255);
		BG1VOFFS(y>>8);
		break;

	case 1:
		BG2HOFFS(x&255);
		BG2HOFFS(x>>8);
		BG2VOFFS(y&255);
		BG2VOFFS(y>>8);
		break;

	case 2:
		BG3HOFFS(x&255);
		BG3HOFFS(x>>8);
		BG3VOFFS(y&255);
		BG3VOFFS(y>>8);
		break;

	case 3:
		BG4HOFFS(x&255);
		BG4HOFFS(x>>8);
		BG4VOFFS(y&255);
		BG4VOFFS(y>>8);
		break;
	}
}
*/


/*set pixelize effect, 0..15 min to max*/

void set_pixelize(unsigned int depth,unsigned int layers);

/*update sprites list through DMA, should be called in VBLANK as soon as possible*/

void oam_update(void);

/*set a sprite in the OAM buffer, this function allows to set complete X value, but works slower than oam_spr1*/

void oam_spr(unsigned int x,unsigned int y,unsigned int chr,unsigned int off);
/*
void oam_spr(unsigned int x,unsigned int y,unsigned int chr,unsigned int off)
{
	static unsigned int offx;

	snes_oam[off+0]=x;
	snes_oam[off+1]=y;
	snes_oam[off+2]=chr;
	snes_oam[off+3]=chr>>8;

	offx=512+(off>>4);

	switch(off&0x0c)
	{
	case 0x00:
		snes_oam[offx]=(snes_oam[offx]&0xfe)|((x>>8)&0x01);
		break;
	case 0x04:
		snes_oam[offx]=(snes_oam[offx]&0xfb)|((x>>6)&0x04);
		break;
	case 0x08:
		snes_oam[offx]=(snes_oam[offx]&0xef)|((x>>4)&0x10);
		break;
	case 0x0c:
		snes_oam[offx]=(snes_oam[offx]&0xbf)|((x>>2)&0x40);
		break;
	}
}
*/


/*set a sprite in the OAM buffer, this function only sets LSB of X, but works much faster than oam_spr*/

void oam_spr1(unsigned int x,unsigned int y,unsigned int chr,unsigned int off);


/*set size bits of a sprite in the OAM buffer*/

void oam_size(unsigned int off,unsigned int size);
/*
void oam_size(unsigned int off,unsigned int size)
{
	static unsigned int c,offx;

	offx=512+(off>>4);
	c=snes_oam[offx];

	switch((off>>2)&3)
	{
	case 0: c=(c&0xfd)|(size<<1); break;
	case 1: c=(c&0xf7)|(size<<3); break;
	case 2: c=(c&0xdf)|(size<<5); break;
	case 3: c=(c&0x7f)|(size<<7); break;
	}

	snes_oam[offx]=c;
}
*/


/*clear the OAM buffer*/

void oam_clear(void)
{
	static unsigned int i;

	for(i=0;i<512;i+=4)
	{
		oam_spr(0,240,0,i);
		oam_size(i,0);
	}
}



/*hides rest of sprite list starting from a given offset*/

void oam_hide_rest(unsigned int off);
/*
void oam_hide_rest(unsigned int off)
{
	while(off<512)
	{
		snes_oam[off+1]=224;
		off+=4;
	}
}
*/


/*wait for specified number of TV frames*/

void delay(unsigned int i)
{
	while(--i) nmi_wait();
}



/*initialize the hardware*/

void system_init(void)
{
	static unsigned int i,j,val1,val2,pad;

	nmi_enable(FALSE);

	MEMSEL(1);		/*FastROM enable*/
	SETINI(0);		/*no overscan, 224 pixels*/

	snes_frame_cnt=0;
	snes_ntsc=STAT78()&0x10?0:1;
	snes_skip_cnt=0;

	randomize(0x0105);

	for(i=0;i<4;++i) set_scroll(i,0,0);

	/*detect multitap*/

#ifdef MULTITAP_SUPPORT

	pad=0;

	for(i=0;i<2;++i)
	{
		val1=0;
		val2=0;

		JOYSER0_WR(0);
		JOYSER0_WR(1);

		for(j=0;j<8;++j) val1=(val1<<1)|((JOYSER_RD(i)>>1)&1);

		JOYSER0_WR(0);

		for(j=0;j<8;++j) val2=(val2<<1)|((JOYSER_RD(i)>>1)&1);

		snes_multitap_present[i]=(val1==0xff&&val2!=0xff)?TRUE:FALSE;

		for(j=0;j<(snes_multitap_present[i]?4:1);++j) snes_pad_source[pad++]=(i<<4)|j;

		snes_pad_serial[i]=0;
	}

	snes_pad_count=(snes_multitap_present[0]?4:1)+(snes_multitap_present[1]?4:1);

#else

	snes_pad_count=2;
	snes_pad_source[0]=0x00;
	snes_pad_source[1]=0x10;

#endif

	/*init pad polling vars*/

	for(i=0;i<snes_pad_count;++i)
	{
		snes_pad_state  [i]=0;
		snes_pad_prev   [i]=0;
		snes_pad_trigger[i]=0;
	}

	/*set bright, clear OAM and VRAM*/

	set_bright(0);

	oam_clear();
	oam_update();

	fill_vram(0,0,65535);
}




/*send a command to the SPC driver*/

void spc_command(unsigned int command,unsigned int param);

/*upload data into the sound memory using IPL loader*/

void spc_load_data(unsigned int adr,unsigned int size,const unsigned char *src);

/*change stereo sound mode*/

void spc_stereo(unsigned int stereo);

/*set global sound volume, target volume 0..127, fade speed 1..255 (slowest to fastest) */

void spc_global_volume(unsigned int volume,unsigned int speed);

/*set channel volume*/

void spc_channel_volume(unsigned int channels,unsigned int volume);

/*initialize sound, set variables, and upload driver code*/

void spc_init(void)
{
	const unsigned int header_size=2;
	static unsigned int i,size;

	size=spc700_code_1[0]+(spc700_code_1[1]<<8);

	spc_music_load_adr=spc700_code_1[14]+(spc700_code_1[15]<<8);

	spc_stream_enable=FALSE;

	nmi_enable(FALSE);

	if(size<32768-header_size)
	{
		spc_load_data(0x0200,size,spc700_code_1+header_size);
	}
	else
	{
		spc_load_data(0x0200,32768-header_size,spc700_code_1+header_size);
		spc_command(SCMD_LOAD,0);
		spc_load_data(0x0200+32768-header_size,size-(32768-header_size),spc700_code_2);
	}

	spc_command(SCMD_INITIALIZE,0);

	nmi_enable(TRUE);
	nmi_wait();
}

/*play sound effect*/

void sfx_play(unsigned int chn,unsigned int sfx,unsigned int vol,int pan);//implemented in sneslib.asm

/*play music*/

void music_play(const unsigned char *data)
{
	static unsigned int size;

	size=data[0]+(data[1]<<8);

	spc_command(SCMD_MUSIC_STOP,0);
	spc_command(SCMD_LOAD,0);
	spc_load_data(spc_music_load_adr,size,data+2);
	spc_command(SCMD_MUSIC_PLAY,0);
}

/*get channel count*/

unsigned int music_channel_count(const unsigned char *data)
{
	return data[2];
}

/*stop music*/

void music_stop(void);

/*pause or unpause music*/

void music_pause(unsigned int pause);

/*stop all sounds*/

void sound_stop_all(void);

/*internal BRR streaming function*/

void spc_stream_set_block(void)
{
	spc_stream_block_src=spc_stream_block_list[spc_stream_block];

	if(!spc_stream_block_src)
	{
		spc_stream_stop_flag=TRUE;
		return;
	}

	spc_stream_block_size=spc_stream_block_src[0]+(spc_stream_block_src[1]<<8);

	spc_stream_block_src+=2;

	spc_stream_block_ptr=0;
	spc_stream_block_ptr_prev=0;
	spc_stream_block_repeat=0;
}

/*start BRR streaming, it uses channels 6 and 7*/
/*these channels should not be used with any functions while the streamer is active*/

void spc_stream_start(const unsigned char const **list)
{
	spc_stream_block_list=list;
	spc_stream_block=0;
	spc_stream_stop_flag=FALSE;
	spc_stream_enable=TRUE;

	spc_stream_set_block();

	spc_command(SCMD_STREAM_START,0);
}

/*stop streaming*/

void spc_stream_stop(void)
{
	spc_stream_enable=FALSE;

	spc_command(SCMD_STREAM_STOP,0);
}

/*internal BRR streaming function*/

void spc_stream_advance_ptr(unsigned int i)
{
	spc_stream_block_ptr+=i;

	if(spc_stream_block_ptr>=spc_stream_block_size)
	{
		++spc_stream_block;

		spc_stream_set_block();
	}
}

/*send BRR stream data when needed, should be called every TV frame*/

void spc_stream_update(void);
/*
void spc_stream_update(void)
{
	static unsigned int n;
	static const unsigned char *src;

	while(1)
	{
		if(!spc_stream_enable) break;

		while(APU0RD);//wait while APU is ready for a new command

		APU0(SCMD_STREAM_SEND);

		while(!APU0RD);//wait while APU confirms a new command

		while(APU0RD);//wait while APU finishes the command

		if(!APU2RD) break;//check if APU needs more data


		if(!spc_stream_block_repeat)
		{
			n=spc_stream_block_src[spc_stream_block_ptr];

			if(n&1)
			{
				spc_stream_block_repeat=n>>1;
				spc_stream_advance_ptr(1);

				src=&spc_stream_block_src[spc_stream_block_ptr_prev];
			}
			else
			{
				src=&spc_stream_block_src[spc_stream_block_ptr];

				spc_stream_block_ptr_prev=spc_stream_block_ptr;
				spc_stream_advance_ptr(9);
			}
		}
		else
		{
			--spc_stream_block_repeat;

			src=&spc_stream_block_src[spc_stream_block_ptr_prev];
		}

		//send 9 bytes

		APU1(*src++);
		APU2(*src++);
		APU3(*src++);

		APU0(1);
		while(APU0RD!=2);

		APU1(*src++);
		APU2(*src++);
		APU3(*src++);

		APU0(2);
		while(APU0RD!=3);

		APU1(*src++);
		APU2(*src++);
		APU3(*src++);

		APU0(3);

		while(APU0RD);//wait while APU finishes the command

		if(spc_stream_stop_flag)
		{
			spc_stream_stop();
			break;
		}
	}
}
*/
