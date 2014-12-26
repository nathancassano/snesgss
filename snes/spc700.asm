arch snes.smp

define UPDATE_RATE_HZ	160		//quantization of music events



//memory layout
// $0000..$00ef direct page, all driver variables are there
// $00f0..$00ff DSP registers
// $0100..$01ff stack page
// $0200..$nnnn driver code
//        $xx00 sample directory
//        $nnnn adsr data list
//        $nnnn sound effects data
//        $nnnn music data
//		  $nnnn BRR streamer buffer



//I/O registers

define TEST 			$f0
define CTRL 			$f1
define ADDR 			$f2
define DATA 			$f3
define CPU0				$f4
define CPU1 			$f5
define CPU2 			$f6
define CPU3 			$f7
define TMP0 			$f8
define TMP1 			$f9
define T0TG 			$fa
define T1TG 			$fb
define T2TG 			$fc
define T0OT 			$fd
define T1OT 			$fe
define T2OT 			$ff


//DSP channel registers, x0..x9, x is channel number

define DSP_VOLL  		$00
define DSP_VOLR 		$01
define DSP_PL   		$02
define DSP_PH    		$03
define DSP_SRCN  		$04
define DSP_ADSR1 		$05
define DSP_ADSR2 		$06
define DSP_GAIN	 		$07
define DSP_ENVX	 		$08
define DSP_OUTX	 		$09


//DSP registers for global settings

define DSP_MVOLL 		$0c
define DSP_MVOLR 		$1c
define DSP_EVOLL 		$2c
define DSP_EVOLR 		$3c
define DSP_KON	 		$4c
define DSP_KOF	 		$5c
define DSP_FLG	 		$6c
define DSP_ENDX	 		$7c
define DSP_EFB	 		$0d
define DSP_PMON	 		$2d
define DSP_NON	 		$3d
define DSP_EON	 		$4d
define DSP_DIR	 		$5d
define DSP_ESA	 		$6d
define DSP_EDL	 		$7d
define DSP_C0	 		$0f
define DSP_C1	 		$1f
define DSP_C2	 		$2f
define DSP_C3	 		$3f
define DSP_C4	 		$4f
define DSP_C5	 		$5f
define DSP_C6	 		$6f
define DSP_C7			$7f


//vars
//max address for vars is $ef, because $f0..$ff is used for the I/O registers

//first two bytes of the direct page are used by IPL!

define D_STEREO			$02	//byte
define D_BUFPTR	 		$03	//byte
define D_ADSR_L			$04	//byte
define D_ADSR_H			$05	//byte
define D_CHx0 			$06	//byte
define D_CH0x 			$07	//byte
define D_KON	 		$08	//byte
define D_KOF	 		$09	//byte
define D_PAN			$0a	//byte
define D_VOL			$0b	//byte
define D_PTR_L			$0c //byte
define D_PTR_H			$0d //byte
define D_PITCH_UPD		$0e	//byte
define D_PITCH_L		$0f	//byte
define D_PITCH_H		$10	//byte
define D_TEMP			$11	//byte
define D_MUSIC_CHNS		$12 //byte
define D_PAUSE			$13	//byte
define D_EFFECTS_L		$14	//byte
define D_EFFECTS_H		$15	//byte
define D_GLOBAL_TGT		$16	//byte
define D_GLOBAL_OUT		$17	//byte
define D_GLOBAL_STEP	$18	//byte
define D_GLOBAL_DIV		$19	//byte

define S_ENABLE	 		$20	//byte
define S_BUFFER_OFF		$21	//byte
define S_BUFFER_PTR		$22	//word
define S_BUFFER_RD		$24	//byte
define S_BUFFER_WR		$25	//byte

define D_CHNVOL	 		$30	//8 bytes
define D_CHNPAN			$38	//8 bytes

define M_PTR_L			$48	//8 bytes
define M_PTR_H			$50	//8 bytes
define M_WAIT_L			$58	//8 bytes
define M_WAIT_H			$60	//8 bytes
define M_PITCH_L		$68	//8 bytes
define M_PITCH_H		$70	//8 bytes
define M_VOL			$78	//8 bytes
define M_DETUNE			$80	//8 bytes
define M_SLIDE			$88	//8 bytes
define M_PORTAMENTO		$90	//8 bytes
define M_PORTA_TO_L		$98	//8 bytes
define M_PORTA_TO_H		$a0	//8 bytes
define M_MODULATION		$a8	//8 bytes
define M_MOD_OFF		$b0	//8 bytes
define M_REF_LEN		$b8	//8 bytes
define M_REF_RET_L		$c0	//8 bytes
define M_REF_RET_H		$c8	//8 bytes
define M_INSTRUMENT		$d0	//8 bytes

//DSP registers write buffer located in the stack page, as it is not used for the most part
//first four bytes are reserved for shortest echo buffer

define D_BUFFER			$0104


define streamBufSize	28		//how many BRR blocks in a buffer, max is 28 (28*9=252)
define streamSampleRate	16000	//stream sample rate

define streamData		$ffc0-({streamBufSize}*9*9)	//fixed location for streaming data

define streamData1		{streamData}
define streamData2		{streamData1}+({streamBufSize}*9)
define streamData3		{streamData2}+({streamBufSize}*9)
define streamData4		{streamData3}+({streamBufSize}*9)
define streamData5		{streamData4}+({streamBufSize}*9)
define streamData6		{streamData5}+({streamBufSize}*9)
define streamData7		{streamData6}+({streamBufSize}*9)
define streamData8		{streamData7}+({streamBufSize}*9)
define streamSync		{streamData8}+({streamBufSize}*9)

define streamPitch		(4096*{streamSampleRate}/32000)



	org 0
	base $0200-2

	dw end-start			//size of the driver code

start:
	clp
	ldx #$ef				//stack is located in $100..$1ff
	txs
	bra mainLoopInit		//$0204, editor should set two zeroes here
	bra editorInit

sampleADSRPtr:
	dw sampleADSRAddr		//$0208
soundEffectsDataPtr:
	dw 0					//$020a
musicDataPtr:
	dw musicDataAddr		//$020c



editorInit:

	jsr initDSPAndVars

	ldx #1
	stx {D_STEREO}			//enable stereo in the editor, mono by default
	dex
	jsr	startMusic



mainLoopInit:

	lda #8000/{UPDATE_RATE_HZ}
	sta {T0TG}
	lda #$81				//enable timer 0 and IPL
	sta {CTRL}

	jsr setReady



//main loop waits for next frame checking the timer
//during the wait it checks and performs commands, also updates BRR streamer position (if enabled)

mainLoop:

	lda {CPU0}				//read command code, when it is zero (SCMD_NONE), no new command
	beq commandDone
	sta {CPU0}				//set busy flag for CPU by echoing a command code
	tay
	and #$0f				//get command offset in the jump table into X
	asl
	tax
	tya
	xcn						//get channel number into Y
	and #$0f
	tay
	jmp (cmdList,x)

//all commands jump back here

commandDone:

	jsr updateBRRStreamerPos

	lda {T0OT}
	beq mainLoop

	jsr updateGlobalVolume

	jsr updateMusicPlayer

	bra mainLoop



cmdInitialize:

	jsr setReady
	jsr initDSPAndVars

	bra commandDone



cmdStereo:

	lda {CPU2}
	sta {D_STEREO}
	jsr setReady

	bra commandDone



cmdGlobalVolume:

	lda {CPU2}	//volume
	ldx {CPU3}	//change speed

	jsr setReady

	cmp #127
	bcc .noClip
	lda #127

.noClip:

	asl
	sta {D_GLOBAL_TGT}
	stx {D_GLOBAL_STEP}

	bra commandDone



cmdChannelVolume:

	lda {CPU2}	//volume
	ldx {CPU3}	//channel mask

	jsr setReady

	cmp #127
	bcc .noClip
	lda #127

.noClip:

	asl
	tay
	txa

	ldx #0

.check:

	ror
	bcc .noVol

	sty {D_CHNVOL},x

.noVol:

	inx
	cpx #8
	bne .check

	jsr updateAllChannelsVolume

	bra commandDone



cmdMusicPlay:

	jsr setReady

	ldx #0					//music always uses channels starting from 0
	stx {D_PAUSE}			//reset pause flag
	jsr	startMusic

	jmp commandDone



cmdStopAllSounds:

	jsr setReady

	lda #8
	sta {D_MUSIC_CHNS}

	bra stopChannels



cmdMusicStop:

	jsr setReady

	lda {D_MUSIC_CHNS}
	bne stopChannels

	jmp commandDone



stopChannels:

	lda #0

.loop:

	pha
	jsr setChannel

	ldx {D_CH0x}
	lda #0
	sta {M_PTR_H},x

	jsr bufKeyOff

	pla
	inc
	dec {D_MUSIC_CHNS}

	bne .loop

	jsr bufKeyOffApply
	jsr bufKeyOnApply

	jmp commandDone



cmdMusicPause:

	lda {CPU2}			//pause state
	sta {D_PAUSE}

	jsr setReady

	jsr updateAllChannelsVolume

	jmp commandDone



cmdSfxPlay:

	cpy {D_MUSIC_CHNS}		//don't play effects on music channels
	bcs .play

	jsr setReady

	jmp	commandDone

.play:

	sty {D_TEMP}			//remember requested channel

	ldx {CPU1}				//volume
	stx {D_VOL}
	lda {CPU2}				//effect number
	ldx {CPU3}				//pan
	stx {D_PAN}

	jsr setReady

	ldy #0					//check if there is an effect with this number, just in case

	cmp ({D_EFFECTS_L}),y
	bcs .done

	asl
	tay
	iny

	lda ({D_EFFECTS_L}),y
	sta {D_PTR_L}
	iny
	lda ({D_EFFECTS_L}),y
	sta {D_PTR_H}

	ldx {D_TEMP}
	jsr startSoundEffect

.done:

	jmp commandDone



cmdLoad:

	jsr setReady

	jsr streamStop			//stop BRR streaming to prevent glitches after loading

	ldx #0
	stx {D_KON}
	dex
	stx {D_KOF}

	jsr bufKeyOffApply

	ldx #$ef
	txs

	jmp $ffc9



cmdStreamStart:

	jsr setReady

	lda #0
	sta {S_BUFFER_WR}
	sta {S_BUFFER_RD}

	jsr streamSetBufPtr
	jsr streamClearBuffers

	lda #$ff
	sta {S_ENABLE}

	ldx #BRRStreamInitData&255
	ldy #BRRStreamInitData/256
	jsr sendDSPSequence

	lda #0			//sync channel volume to zero
	sta {M_VOL}+6
	lda #127		//stream channel volume to max
	sta {M_VOL}+7

	jsr updateAllChannelsVolume

	jmp commandDone



cmdStreamStop:

	jsr setReady
	jsr streamStop

	jmp commandDone



cmdStreamSend:

	lda {S_ENABLE}
	beq .nosend

	lda {S_BUFFER_WR}
	cmp {S_BUFFER_RD}
	bne .send

.nosend:

	lda #0
	sta {CPU2}

	jsr setReady

	jmp commandDone

.send:

	lda #1
	sta {CPU2}

	jsr setReady

	ldx #1

.wait1:

	jsr updateBRRStreamerPos
	cpx {CPU0}
	bne .wait1


	ldy {S_BUFFER_OFF}

	lda ({S_BUFFER_PTR}),y	//keep the flags
	and #3
	ora {CPU1}
	sta ({S_BUFFER_PTR}),y
	iny
	lda {CPU2}
	sta ({S_BUFFER_PTR}),y
	iny
	lda {CPU3}
	sta ({S_BUFFER_PTR}),y
	iny

	inx
	stx {CPU0}

.wait2:

	jsr updateBRRStreamerPos
	cpx {CPU0}
	bne .wait2

	lda {CPU1}
	sta ({S_BUFFER_PTR}),y
	iny
	lda {CPU2}
	sta ({S_BUFFER_PTR}),y
	iny
	lda {CPU3}
	sta ({S_BUFFER_PTR}),y
	iny

	inx
	stx {CPU0}

.wait3:

	jsr updateBRRStreamerPos
	cpx {CPU0}
	bne .wait3

	lda {CPU1}
	sta ({S_BUFFER_PTR}),y
	iny
	lda {CPU2}
	sta ({S_BUFFER_PTR}),y
	iny
	lda {CPU3}
	sta ({S_BUFFER_PTR}),y
	iny

	jsr setReady
	lda #0
	sta {CPU2}

	sty {S_BUFFER_OFF}
	cpy #{streamBufSize}*9
	bne .skip

	lda {S_BUFFER_WR}
	inc
	and #7
	sta {S_BUFFER_WR}

	jsr streamSetBufPtr

.skip:

	jmp commandDone



streamStop:

	lda #0
	sta {S_ENABLE}

	lda #{DSP_KOF}	//stop channels 6 and 7
	sta {ADDR}

	lda #$c0
	sta {DATA}

	jsr updateAllChannelsVolume

	rts



//clear BRR streamer buffers, fill them with c0 00..00 with stop bit in the last block

streamClearBuffers:

	ldx #0
	lda #{streamBufSize}
	sta {D_TEMP}

.clear0:

	lda #$c0
	sta {streamData1},x
	sta {streamData2},x
	sta {streamData3},x
	sta {streamData4},x
	sta {streamData5},x
	sta {streamData6},x
	sta {streamData7},x
	sta {streamData8},x
	sta {streamSync},x
	inx
	lda #$00
	ldy #8

.clear1:

	sta {streamData1},x
	sta {streamData2},x
	sta {streamData3},x
	sta {streamData4},x
	sta {streamData5},x
	sta {streamData6},x
	sta {streamData7},x
	sta {streamData8},x
	sta {streamSync},x
	inx
	dey
	bne .clear1

	dec {D_TEMP}
	bne .clear0

	lda #$c3
	sta {streamData8}+{streamBufSize}*9-9
	sta {streamSync}+{streamBufSize}*9-9

	rts



streamSetBufPtr:

	asl
	tax
	lda streamBufferPtr+0,x
	sta {S_BUFFER_PTR}+0
	lda streamBufferPtr+1,x
	sta {S_BUFFER_PTR}+1

	lda #0
	sta {S_BUFFER_OFF}

	rts



//start music, using the channels starting from the specified one
//in: X=the starting channel, musicDataPtr=music data location

startMusic:

	lda musicDataPtr+0
	sta {D_PTR_L}
	lda musicDataPtr+1
	sta {D_PTR_H}

	ldy #0					//how many channels in the song
	lda ({D_PTR_L}),y

	sta {D_MUSIC_CHNS}

	bne .start				//just in case

	jmp commandDone

.start:

	sta {D_TEMP}
	iny

.loop:

	phx
	txa
	jsr setChannel

	phy
	jsr startChannel

	lda #128
	sta {D_CHNPAN},x		//reset pan for music channels

	ply
	plx

	inx						//the song requires too many channels, skip those that don't fit
	cpx #8
	bcs .done

	iny
	iny

	dec {D_TEMP}
	bne .loop

.done:

	rts



//start sound effects, almost the same as music
//in: X=the starting channel, D_PTR=effect data location,D_VOL,D_PAN

startSoundEffect:

	ldy #0
	lda ({D_PTR_L}),y		//how many channels in the song

	sta {D_TEMP}

	iny

.loop:

	phx
	txa
	jsr setChannel

	phy
	jsr startChannel
	
	lda {D_VOL}				//apply effect volume
	cmp #127
	bcc .noClip

.noClip:

	asl
	sta {D_CHNVOL},x

	lda {D_PAN}				//apply effect pan
	sta {D_CHNPAN},x

	ply
	plx

	inx
	cpx #8
	bcs .done

	iny
	iny

	dec {D_TEMP}
	bne .loop

.done:

	rts



//initialize channel
//in: {D_CH0x}=channel, {D_PTR_L},Y=offset to the channel data

startChannel:

	jsr bufKeyOff
	jsr bufKeyOffApply

	ldx {D_CH0x}

	lda #0

	sta {M_PITCH_L},x		//reset current pitch
	sta {M_PITCH_H},x

	sta {M_PORTA_TO_L},x	//reset portamento pitch
	sta {M_PORTA_TO_H},x

	sta {M_DETUNE},x		//reset all effects
	sta {M_SLIDE},x
	sta {M_PORTAMENTO},x
	sta {M_MODULATION},x
	sta {M_MOD_OFF},x
	sta {M_REF_LEN},x		//reset reference

	sta {M_WAIT_H},x
	lda #4					//short delay between initial keyoff and starting a channel, to prevent clicks
	sta {M_WAIT_L},x

	lda #127				//set default channel volume
	sta {M_VOL},x
	
	lda ({D_PTR_L}),y
	sta {M_PTR_L},x
	iny
	lda ({D_PTR_L}),y
	sta {M_PTR_H},x

	rts



//run one frame of music player

updateMusicPlayer:

	jsr bufClear

	lda #0

.loop:

	pha

	ldx {D_PAUSE}
	beq .noPause

	cmp {D_MUSIC_CHNS}		//the pause only skips channels with music
	bcc .skipChannel

.noPause:

	jsr setChannel			//select current channel

	lda #0					//reset pitch update flag, set by any pitch change event in the updateChannel
	sta {D_PITCH_UPD}

	jsr updateChannel		//perform channel update
	jsr updatePitch			//write pitch registers if the pitch was updated

.skipChannel:

	pla
	inc
	cmp #8
	bne .loop

	jsr bufKeyOffApply
	jsr bufApply
	jsr bufKeyOnApply

	rts



//read one byte of a music channel data, and advance the read pointer in temp variable
//readChannelByteZ sets Y offset to zero, readChannelByte skips it as Y often remains zero anyway
//it also handles reference calls and returns

readChannelByteZ:

	ldy #0

readChannelByte:

	lda ({D_PTR_L}),y

	inw {D_PTR_L}

	pha
	ldx {D_CH0x}

	lda {M_REF_LEN},x		//check reference length
	beq .done

	dec {M_REF_LEN},x		//decrement reference length
	bne .done

	lda {M_REF_RET_L},x		//restore original pointer
	sta {D_PTR_L}
	lda {M_REF_RET_H},x
	sta {D_PTR_H}

.done:

	pla
	rts



//perform update of one music data channel, and parse channel data when needed

updateChannel:

	ldx {D_CH0x}

	lda {M_PTR_H},x			//when the pointer MSB is zero, channel is inactive
	bne .processChannel
	rts

.processChannel:

	sta {D_PTR_H}
	lda {M_PTR_L},x
	sta {D_PTR_L}

	lda {M_MODULATION},x	//when modulation is active, pitch gets updated constantly
	sta {D_PITCH_UPD}
	beq .noModulation

	and #$0f				//advance the modulation pointer
	clc
	adc {M_MOD_OFF},x

	cmp #192
	bcc .noWrap

	sec
	sbc #192

.noWrap:

	sta {M_MOD_OFF},x

.noModulation:

	lda {M_PORTAMENTO},x
	bne .doPortamento		//portamento has priority over slides

	lda {M_SLIDE},x
	bne .doSlide
	jmp .noSlide

.doSlide:

	sta {D_PITCH_UPD}

	bmi .slideDown
	bra .slideUp



.doPortamento:

	lda {M_PITCH_H},x		//compare current pitch and needed pitch
	cmp {M_PORTA_TO_H},x
	bne .portaCompareDone
	lda {M_PITCH_L},x
	cmp {M_PORTA_TO_L},x

.portaCompareDone:

	beq .noSlide			//go noslide if no pitch change is needed
	bcs .portaDown



.portaUp:

	inc {D_PITCH_UPD}

	lda {M_PITCH_L},x
	clc
	adc {M_PORTAMENTO},x
	sta {M_PITCH_L},x
	lda {M_PITCH_H},x
	adc #0
	sta {M_PITCH_H},x

	cmp {M_PORTA_TO_H},x	//check if the pitch goes higher than needed
	bne .portaUpLimit
	lda {M_PITCH_L},x
	cmp {M_PORTA_TO_L},x

.portaUpLimit:

	bcs .portaPitchLimit
	bra .noSlide



.portaDown:

	inc {D_PITCH_UPD}

	lda {M_PITCH_L},x
	sec
	sbc {M_PORTAMENTO},x
	sta {M_PITCH_L},x
	lda {M_PITCH_H},x
	sbc #0
	sta {M_PITCH_H},x

	cmp {M_PORTA_TO_H},x	//check if the pitch goes lower than needed
	bne .portaDownLimit
	lda {M_PITCH_L},x
	cmp {M_PORTA_TO_L},x

.portaDownLimit:

	bcc .portaPitchLimit
	bra .noSlide



.portaPitchLimit:

	lda {M_PORTA_TO_L},x
	sta {M_PITCH_L},x
	lda {M_PORTA_TO_H},x
	sta {M_PITCH_H},x

	bra .noSlide



.slideUp:

	lda {M_PITCH_L},x
	clc
	adc {M_SLIDE},x
	sta {M_PITCH_L},x
	lda {M_PITCH_H},x
	adc #0
	sta {M_PITCH_H},x

	cmp #$40
	bcc .noSlide

	lda #$3f
	sta {M_PITCH_H},x
	lda #$30
	sta {M_PITCH_L},x

	bra .noSlide



.slideDown:

	lda {M_PITCH_L},x
	clc
	adc {M_SLIDE},x
	sta {M_PITCH_L},x
	lda {M_PITCH_H},x
	adc #255
	sta {M_PITCH_H},x

	bcs .noSlide

	lda #0
	sta {M_PITCH_H},x
	sta {M_PITCH_L},x



.noSlide:

	lda {M_WAIT_L},x
	bne .wait1
	lda {M_WAIT_H},x
	beq .readLoopStart

	dec {M_WAIT_H},x

.wait1:

	dec {M_WAIT_L},x

	lda {M_WAIT_L},x
	ora {M_WAIT_H},x

	beq .readLoopStart

	rts


.readLoopStart:

	jsr updateChannelVolume
	
.readLoop:

	jsr readChannelByteZ		//read a new byte, set Y to zero, X to D_CH0x

	cmp #149
	bcc .setShortDelay
	cmp #245
	beq .keyOff
	bcc .newNote

	cmp #246
	beq .setLongDelay
	cmp #247
	beq .setEffectVolume
	cmp #248
	beq .setEffectPan
	cmp #249
	beq .setEffectDetune
	cmp #250
	beq .setEffectSlide
	cmp #251
	beq .setEffectPortamento
	cmp #252
	beq .setEffectModulation
	cmp #253
	beq .setReference
	cmp #254
	bne .jumpLoop
	jmp .setInstrument

.jumpLoop:	//255

	jsr readChannelByte
	pha
	jsr readChannelByte

	sta {D_PTR_H}
	pla
	sta {D_PTR_L}

	bra .readLoop



.keyOff:

	jsr bufKeyOff

	bra .readLoop



.setShortDelay:

	sta {M_WAIT_L},x

	bra .storePtr



.newNote:

	sec
	sbc #150

	jsr setPitch

	lda {M_PORTAMENTO},x	//only set key on when portamento is not active
	bne .readLoop

	jsr bufKeyOn

	bra .readLoop



.setLongDelay:

	jsr readChannelByte
	sta {M_WAIT_L},x

	jsr readChannelByte
	sta {M_WAIT_H},x

	bra .storePtr



.setEffectVolume:

	jsr readChannelByte		//read volume value

	sta {M_VOL},x

	jsr updateChannelVolume	//apply volume and pan

	bra .readLoop



.setEffectPan:

	jsr readChannelByte		//read pan value

	sta {D_CHNPAN},x

	jsr updateChannelVolume	//apply volume and pan

	bra .readLoop



.setEffectDetune:

	jsr readChannelByte		//read detune value

	sta {M_DETUNE},x
	inc {D_PITCH_UPD}		//set pitch update flag

	jmp .readLoop



.setEffectSlide:

	jsr readChannelByte		//read slide value (8-bit signed, -99..99)

	sta {M_SLIDE},x

	jmp .readLoop



.setEffectPortamento:

	jsr readChannelByte		//read portamento value (8-bit unsigned, 0..99)

	sta {M_PORTAMENTO},x

	jmp .readLoop



.setEffectModulation:

	jsr readChannelByte		//read modulation value

	sta {M_MODULATION},x

	jmp .readLoop



.setReference:

	jsr readChannelByte		//read reference LSB
	pha
	jsr readChannelByte		//read reference MSB
	pha
	jsr readChannelByte		//read reference length in bytes

	sta {M_REF_LEN},x

	lda {D_PTR_L}			//remember previous pointer as the return address
	sta {M_REF_RET_L},x
	lda {D_PTR_H}
	sta {M_REF_RET_H},x

	pla						//set reference pointer
	sta {D_PTR_H}
	pla
	sta {D_PTR_L}

	jmp .readLoop



.setInstrument:

	jsr readChannelByte
	jsr setInstrument

	jmp .readLoop



.storePtr:

	ldx {D_CH0x}

	lda {D_PTR_L}			//store changed pointer
	sta {M_PTR_L},x
	lda {D_PTR_H}
	sta {M_PTR_H},x

	rts



//initialize DSP registers and driver variables

initDSPAndVars:

	ldx #0

	stx {D_KON}					//no keys pressed
	stx {D_KOF}					//no keys released
	stx {D_STEREO}				//mono by default
	stx {D_MUSIC_CHNS}			//how many channels current music uses
	stx {D_PAUSE}				//pause mode is inactive
	stx {S_ENABLE}				//disble streaming
	stx {D_GLOBAL_DIV}			//reset global volume change speed divider

	stx {D_GLOBAL_OUT}			//current global volume
	ldx #255
	stx {D_GLOBAL_TGT}			//target global volume
	stx {D_GLOBAL_STEP}			//global volume change speed
	
	ldx #DSPInitData&255
	ldy #DSPInitData/256
	jsr sendDSPSequence

	lda sampleADSRPtr+0			//set DP pointer to the ADSR list
	sta {D_ADSR_L}
	lda sampleADSRPtr+1
	sta {D_ADSR_H}

	lda soundEffectsDataPtr+0	//set DP pointer to sound effects data
	sta {D_EFFECTS_L}
	lda soundEffectsDataPtr+1
	sta {D_EFFECTS_H}

	ldx #0

.initChannels:

	lda #0
	sta {M_PTR_H},x

	lda #128
	sta {D_CHNPAN},x
	lda #255
	sta {D_CHNVOL},x
	
	sta {M_INSTRUMENT},x

	inx
	cpx #8
	bne .initChannels

	jsr streamClearBuffers

	rts



//updates play position for the BRR streamer

updateBRRStreamerPos:

	lda #{DSP_ENDX}				//check if channel 6 stopped playing
	sta {ADDR}
	lda {DATA}
	and #$40
	beq .skip
	sta {DATA}

	lda {S_BUFFER_RD}
	inc
	and #7
	sta {S_BUFFER_RD}

.skip:

	rts



//set the ready flag for the 65816 side, so it can send a new command

setReady:

	pha

	lda #%10010001				//reset communication I/O, enable timer 0 and IPL
	sta {CTRL}
	lda #0
	sta {CPU0}
	sta {CPU1}

	pla
	rts



//set current channel for all other setNNN routines
//in: A=channel 0..7

setChannel:

	sta {D_CH0x}
	asl
	asl
	asl
	asl
	sta {D_CHx0}

	rts



//set instrument (sample and ADSR) for specified channel
//in: {D_CHx0}=channel, A=instrument number 0..255

setInstrument:

	tay

	ldx {D_CH0x}
	cmp {M_INSTRUMENT},x
	beq .skip
	sta {M_INSTRUMENT},x

	lda {D_CHx0}
	ora #{DSP_SRCN}
	tax

	tya
	jsr bufRegWrite				//write instrument number, remains in A after jsr

	asl							//get offset for parameter tables
	tay

	lda {D_CHx0}				//write adsr parameters
	ora #{DSP_ADSR1}
	tax
	lda ({D_ADSR_L}),y			//first adsr byte
	jsr bufRegWrite

	lda {D_CHx0}
	ora #{DSP_ADSR2}
	tax
	iny
	lda ({D_ADSR_L}),y			//second adsr byte
	jsr bufRegWrite

.skip:

	rts



//set pitch variables accoding to note
//in: {D_CH0x}=channel, A=note

setPitch:

	asl
	tay

	ldx {D_CH0x}

	lda notePeriodTable+0,y
	sta {M_PORTA_TO_L},x

	lda notePeriodTable+1,y
	sta {M_PORTA_TO_H},x

	lda {M_PORTAMENTO},x
	beq .portaOff

	cmp #99						//P99 gets legato effect, changes pitch right away without sending keyon
	beq .portaOff

	rts

.portaOff:

	lda {M_PORTA_TO_L},x		//when portamento is inactive, copy the target pitch into current pitch immideately
	sta {M_PITCH_L},x
	lda {M_PORTA_TO_H},x
	sta {M_PITCH_H},x

	inc {D_PITCH_UPD}

	rts



//apply current pitch of the channel, also applies the detune value
//in: {D_CHx0}=channel if the pitch change flag is set

updatePitch:

	lda {D_PITCH_UPD}
	bne .update
	rts

.update:

	ldx {D_CH0x}

	lda {M_PITCH_L},x
	clc
	adc {M_DETUNE},x
	sta {D_PITCH_L}
	lda {M_PITCH_H},x
	adc #0
	sta {D_PITCH_H}

	lda {M_MODULATION},x
	beq .noModulation

	and #$f0
	tay

	lda {M_MOD_OFF},x
	tax
	lda vibratoTable,x
	bmi .modSubtract

.modAdd:

	mul
	tya

	clc
	adc {D_PITCH_L}
	sta {D_PITCH_L}
	lda {D_PITCH_H}
	adc #0
	sta {D_PITCH_H}

	bra .noModulation

.modSubtract:

	eor #255
	inc

	mul
	tya
	sta {D_TEMP}

	lda {D_PITCH_L}
	sec
	sbc {D_TEMP}
	sta {D_PITCH_L}
	lda {D_PITCH_H}
	sbc #0
	sta {D_PITCH_H}

.noModulation:

	lda {D_CHx0}
	ora #{DSP_PH}
	tax
	lda {D_PITCH_H}
	jsr bufRegWrite

	lda {D_CHx0}
	ora #{DSP_PL}
	tax
	lda {D_PITCH_L}
	jsr bufRegWrite

	rts



//set volume and pan for current channel, according to M_VOL,D_CHNVOL,D_CHNPAN, and music pause state
//in: {D_CHx0}=channel

updateChannelVolume:

	ldx {D_CH0x}

	cpx {D_MUSIC_CHNS}
	bcs .noMute

	lda {D_PAUSE}
	beq .noMute

	lda #0
	bra .calcVol

.noMute:

	lda {D_CHNVOL},x		//get virtual channel volume

.calcVol:

	ldy {M_VOL},x			//get music channel volume
	mul
	sty {D_VOL}				//resulting volume

	lda {D_STEREO}
	bne .stereo

.mono:

	lda {D_VOL}
	pha
	pha
	bra .setVol

.stereo:

	lda {D_CHNPAN},x		//calculate left volume
	eor #$ff
	tay
	lda {D_VOL}
	mul
	phy

	ldy {D_CHNPAN},x		//calculate right volume
	lda {D_VOL}
	mul
	phy

.setVol:

	lda {D_CHx0}			//write volume registers
	ora #{DSP_VOLR}
	tax
	pla
	jsr bufRegWrite

	lda {D_CHx0}
	ora #{DSP_VOLL}
	tax
	pla
	jsr bufRegWrite

	rts



//set volume and pan for all channels

updateAllChannelsVolume:

	lda #0

.updVol:

	pha
	jsr setChannel
	jsr updateChannelVolume
	pla
	inc
	cmp #8
	bne .updVol

	jsr bufApply

	rts



//update global volume, considering current fade speed

updateGlobalVolume:

	inc {D_GLOBAL_DIV}
	lda {D_GLOBAL_DIV}
	and #7
	beq .update
	lda {D_GLOBAL_STEP}
	cmp #255
	beq .update

	rts

.update:

	lda {D_GLOBAL_OUT}
	cmp {D_GLOBAL_TGT}
	beq .setVolume

	bcs .fadeOut
	

.fadeIn:

	adc {D_GLOBAL_STEP}
	bcc .fadeInLimit
	
	lda {D_GLOBAL_TGT}
	bra .setVolume
	
.fadeInLimit:

	cmp {D_GLOBAL_TGT}
	bcc .setVolume
	lda {D_GLOBAL_TGT}
	bra .setVolume


.fadeOut:

	sbc {D_GLOBAL_STEP}
	bcs .fadeOutLimit
	
	lda {D_GLOBAL_TGT}
	bra .setVolume
	
.fadeOutLimit:
	
	cmp {D_GLOBAL_TGT}
	bcs .setVolume
	lda {D_GLOBAL_TGT}
	

.setVolume:

	sta {D_GLOBAL_OUT}
	lsr

	ldx #{DSP_EVOLL}
	stx {ADDR}
	sta {DATA}

	ldx #{DSP_EVOLR}
	stx {ADDR}
	sta {DATA}

	rts
	
	
	
//clear register writes buffer, just set ptr to 0

bufClear:

	str {D_BUFPTR}=#0

	rts



//add register write in buffer
//in X=reg, A=value

bufRegWrite:

	pha
	txa
	ldx {D_BUFPTR}
	sta {D_BUFFER},x
	inx

	pla
	sta {D_BUFFER},x
	inx
	stx {D_BUFPTR}

	rts



//set keyon for specified channel in the temp variable
//in: {D_CH0x}=channel

bufKeyOn:

	ldx {D_CH0x}
	lda channelMask,x
	ora {D_KON}
	sta {D_KON}

	rts



//set keyoff for specified channel in the temp variable
//in: {D_CH0x}=channel

bufKeyOff:

	ldx {D_CH0x}
	lda channelMask,x
	ora {D_KOF}
	sta {D_KOF}

	rts



//send writes from buffer and clear it

bufApply:

	lda {D_BUFPTR}
	beq .done

	ldx #0

.loop:

	lda {D_BUFFER},x
	sta {ADDR}
	inx

	lda {D_BUFFER},x
	sta {DATA}
	inx
	cpx {D_BUFPTR}
	bne .loop
	str {D_BUFPTR}=#0

.done:

	rts



//send keyon from the temp variable

bufKeyOnApply:

	lda {D_KON}
	eor #$ff
	and {D_KOF}
	ldx #{DSP_KOF}
	stx {ADDR}
	sta {D_KOF}
	sta {DATA}

	lda #{DSP_KON}
	sta {ADDR}
	lda {D_KON}
	str {D_KON}=#0
	sta {DATA}

	rts



//send keyoff from the temp variable

bufKeyOffApply:

	lda #{DSP_KOF}
	sta {ADDR}
	lda {D_KOF}
	sta {DATA}

	rts



//send sequence of DSP register writes, used for init

sendDSPSequence:

	stx {D_PTR_L}
	sty {D_PTR_H}
	ldy #0

.loop:

	lda ({D_PTR_L}),y
	beq .done

	sta {ADDR}
	iny
	lda ({D_PTR_L}),y
	sta {DATA}
	iny

	bra .loop

.done:

	rts



cmdList:
	dw 0					//0 no command, means the APU is ready for a new command
	dw cmdInitialize		//1 initialize DSP
	dw cmdLoad				//2 load new music data
	dw cmdStereo			//3 change stereo sound mode, mono by default
	dw cmdGlobalVolume		//4 set global volume
	dw cmdChannelVolume		//5 set channel volume
	dw cmdMusicPlay			//6 start music
	dw cmdMusicStop			//7 stop music
	dw cmdMusicPause		//8 pause music
	dw cmdSfxPlay			//9 play sound effect
	dw cmdStopAllSounds		//10 stop all sounds
	dw cmdStreamStart		//11 start sound streaming
	dw cmdStreamStop		//12 stop sound streaming
	dw cmdStreamSend		//13 send a block of data if needed



notePeriodTable:
	dw $0042,$0046,$004a,$004f,$0054,$0059,$005e,$0064,$006a,$0070,$0077,$007e
	dw $0085,$008d,$0095,$009e,$00a8,$00b2,$00bc,$00c8,$00d4,$00e0,$00ee,$00fc
	dw $010b,$011b,$012b,$013d,$0150,$0164,$0179,$0190,$01a8,$01c1,$01dc,$01f8
	dw $0216,$0236,$0257,$027b,$02a1,$02c9,$02f3,$0320,$0350,$0382,$03b8,$03f0
	dw $042c,$046c,$04af,$04f6,$0542,$0592,$05e7,$0641,$06a0,$0705,$0770,$07e1
	dw $0859,$08d8,$095f,$09ed,$0a85,$0b25,$0bce,$0c82,$0d41,$0e0a,$0ee0,$0fc3
	dw $10b3,$11b1,$12be,$13db,$150a,$164a,$179d,$1905,$1a82,$1c15,$1dc1,$1f86
	dw $2166,$2362,$257d,$27b7,$2a14,$2c95,$2f3b,$320a,$3504,$382b,$3b82,$3f0c



vibratoTable:
	db $00,$04,$08,$0c,$10,$14,$18,$1c,$20,$24,$28,$2c,$30,$34,$38,$3b
	db $3f,$43,$46,$49,$4d,$50,$53,$56,$59,$5c,$5f,$62,$64,$67,$69,$6b
	db $6d,$70,$71,$73,$75,$76,$78,$79,$7a,$7b,$7c,$7d,$7d,$7e,$7e,$7e
	db $7f,$7e,$7e,$7e,$7d,$7d,$7c,$7b,$7a,$79,$78,$76,$75,$73,$71,$70
	db $6d,$6b,$69,$67,$64,$62,$5f,$5c,$59,$56,$53,$50,$4d,$49,$46,$43
	db $3f,$3b,$38,$34,$30,$2c,$28,$24,$20,$1c,$18,$14,$10,$0c,$08,$04
	db $00,$fc,$f8,$f4,$f0,$ec,$e8,$e4,$e0,$dc,$d8,$d4,$d0,$cc,$c8,$c5
	db $c1,$bd,$ba,$b7,$b3,$b0,$ad,$aa,$a7,$a4,$a1,$9e,$9c,$99,$97,$95
	db $93,$90,$8f,$8d,$8b,$8a,$88,$87,$86,$85,$84,$83,$83,$82,$82,$82
	db $81,$82,$82,$82,$83,$83,$84,$85,$86,$87,$88,$8a,$8b,$8d,$8f,$90
	db $93,$95,$97,$99,$9c,$9e,$a1,$a4,$a7,$aa,$ad,$b0,$b3,$b7,$ba,$bd
	db $c1,$c5,$c8,$cc,$d0,$d4,$d8,$dc,$e0,$e4,$e8,$ec,$f0,$f4,$f8,$fc



channelMask:
	db $01,$02,$04,$08,$10,$20,$40,$80



streamBufferPtr:
	dw {streamData1}
	dw {streamData2}
	dw {streamData3}
	dw {streamData4}
	dw {streamData5}
	dw {streamData6}
	dw {streamData7}
	dw {streamData8}



DSPInitData:
	db {DSP_FLG}  ,%01100000				//mute, disable echo
	db {DSP_MVOLL},0						//global volume to zero, echo volume used as global instead
	db {DSP_MVOLR},0
	db {DSP_PMON} ,0						//no pitch modulation
	db {DSP_NON}  ,0						//no noise
	db {DSP_ESA}  ,1						//echo buffer in the stack page
	db {DSP_EDL}  ,0						//minimal echo buffer length
	db {DSP_EFB}  ,0						//no echo feedback
	db {DSP_C0}	  ,127						//zero echo filter
	db {DSP_C1}	  ,0
	db {DSP_C2}	  ,0
	db {DSP_C3}	  ,0
	db {DSP_C4}	  ,0
	db {DSP_C5}	  ,0
	db {DSP_C6}	  ,0
	db {DSP_C7}	  ,0
	db {DSP_EON}  ,255						//echo enabled
	db {DSP_EVOLL},0						//echo (as global) volume to zero, it gets to the max after init
	db {DSP_EVOLR},0
	db {DSP_KOF}  ,255						//all keys off
	db {DSP_DIR}  ,sampleDirAddr/256		//sample dir location
	db {DSP_FLG}  ,%00000000				//no mute, enable echo
	db 0



BRRStreamInitData:
	db {DSP_ADSR1}+$70,$00					//disable ADSR on channel 7
	db {DSP_GAIN} +$70,$1f
	db {DSP_PL}   +$70,{streamPitch}&255	//set pitch on channels 6 and 7
	db {DSP_PH}   +$70,{streamPitch}/256
	db {DSP_PL}   +$60,{streamPitch}&255
	db {DSP_PH}   +$60,{streamPitch}/256
	db {DSP_SRCN} +$70,0					//stream always playing on channel 7
	db {DSP_SRCN} +$60,8					//sync stream always playing on channel 6
	db {DSP_KON}      ,$c0					//start channels 6 and 7
	db 0



	align 256		//sample dir list should be aligned to 256 bytes

sampleDirAddr:		//four bytes per sample, first 9 entries reserved for the BRR streamer

	dw {streamData1},{streamData1}
	dw {streamData2},{streamData2}
	dw {streamData3},{streamData3}
	dw {streamData4},{streamData4}
	dw {streamData5},{streamData5}
	dw {streamData6},{streamData6}
	dw {streamData7},{streamData7}
	dw {streamData8},{streamData8}
	dw {streamSync},{streamSync}
