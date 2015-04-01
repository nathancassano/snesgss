;.define DISABLE_SOUND

.define SPR_HFLIP			$4000
.define SPR_VFLIP			$8000

.define SPR_PAL0			(0<<9)
.define SPR_PAL1			(1<<9)
.define SPR_PAL2			(2<<9)
.define SPR_PAL3			(3<<9)
.define SPR_PAL4			(4<<9)
.define SPR_PAL5			(5<<9)
.define SPR_PAL6			(6<<9)
.define SPR_PAL7			(7<<9)

.define SPR_PRI0			$0000
.define SPR_PRI1			$1000
.define SPR_PRI2			$2000
.define SPR_PRI3			$3000

.define OAM_ADDR			$2102
.define MOSAIC				$2106
.define BGxHOFFS			$210d
.define BGxVOFFS			$210e
.define VRAM_ADDR			$2116
.define CGADD				$2121

.define APU0				$2140
.define APU1				$2141
.define APU01				$2140
.define APU2				$2142
.define APU3				$2143
.define APU23				$2142

.define NMITIMEEN			$4200
.define WRMPYA				$4202
.define WRMPYB				$4203
.define MDMA_EN				$420b
.define RDMPY				$4216

.define DMA7_TYPE			$4370
.define DMA7_SIZE			$4375
.define DMA7_ADDR			$4372
.define DMA7_BANK			$4374

.define SCMD_NONE				$00
.define SCMD_INITIALIZE			$01
.define SCMD_LOAD				$02
.define SCMD_STEREO				$03
.define SCMD_GLOBAL_VOLUME		$04
.define SCMD_CHANNEL_VOLUME		$05
.define SCMD_MUSIC_PLAY 		$06
.define SCMD_MUSIC_STOP 		$07
.define SCMD_MUSIC_PAUSE 		$08
.define SCMD_SFX_PLAY			$09
.define SCMD_STOP_ALL_SOUNDS	$0a
.define SCMD_STREAM_START		$0b
.define SCMD_STREAM_STOP		$0c
.define SCMD_STREAM_SEND		$0d



.16bit

.macro A8
	sep #$20
.endm

.macro A16
	rep #$20
.endm

.macro AXY8
	sep #$30
.endm

.macro AXY16
	rep #$30
.endm

.macro XY8
	sep #$10
.endm

.macro XY16
	rep #$10
.endm



.ramsection ".sneslib_zp" bank 0 slot 4

sneslib_ptr:		dsb 4
sneslib_temp:		dsb 2
sneslib_rand1:		dsb 2
sneslib_rand2:		dsb 2

gss_param:			dsb 2
gss_command:		dsb 2

brr_stream_src:		dsb 4
brr_stream_list:	dsb 4

.ends

; .ramsection ".sneslib_vars" bank $7e slot 2

; .ends



.section ".sneslib" superfree

;void nmi_handler(void);

nmi_handler:

	php
	A16
	
	inc.w __tccs_snes_frame_cnt
	
	plp
	rtl



;void nmi_wait(void);

nmi_wait:

	php
	AXY16

	lda.w __tccs_snes_frame_cnt
-
	cmp.w __tccs_snes_frame_cnt
	beq -
	
	plp
	rtl
	
	
	
;void update_palette(void);

update_palette:

	php
	AXY16
	
	lda.w #$2200
	sta.l DMA7_TYPE
	lda.w #$0200
	sta.l DMA7_SIZE
	lda.w #__tccs_snes_palette
	sta.l DMA7_ADDR
	A8
	lda.b #:__tccs_snes_palette
	sta.l DMA7_BANK
	
	lda.b #0
	sta.l CGADD
	
	lda.b #$80
	sta.l MDMA_EN

	plp
	rtl
	
	
	
;void copy_to_vram(unsigned int adr,const unsigned char *src,unsigned int size);

copy_to_vram:

	php
	AXY16
	
	lda 5,s				;adr
	sta.l VRAM_ADDR

	lda.w #$1801
	sta.l DMA7_TYPE
	lda 11,s			;size
	sta.l DMA7_SIZE
	lda 7,s				;src offset
	sta.l DMA7_ADDR
	A8
	lda 9,s				;src bank
	sta.l DMA7_BANK
	lda.b #$80
	sta.l MDMA_EN
	
	plp
	rtl
	
	
	
;void set_pixelize(unsigned int depth,unsigned int layers);

set_pixelize:

	php
	A8
	
	lda 5,s		;depth
	asl
	asl
	asl
	asl
	and.b #$f0
	sta.b sneslib_temp
	
	lda 7,s		;layers
	and.b #$0f
	ora.b sneslib_temp
	
	sta.l MOSAIC

	plp
	rtl
	
	
	
;void oam_update(void);

oam_update:

	php
	AXY16

	lda.w #$0000
	sta.l OAM_ADDR
	lda.w #$0400
	sta.l DMA7_TYPE
	lda.w #$0220
	sta.l DMA7_SIZE
	lda.w #__tccs_snes_oam
	sta.l DMA7_ADDR
	A8
	lda.b #:__tccs_snes_oam
	sta.l DMA7_BANK
	lda.b #$80
	sta.l MDMA_EN

	plp
	rtl



;void oam_spr(unsigned int x,unsigned int y,unsigned int chr,unsigned int off);

oam_spr:

	php
	AXY16

	lda 11,s					;off
	tax
	lda 5,s						;x
	xba
	A8
	ror a						;x msb into carry
	lda 7,s						;y
	xba
	A16
	sta.w __tccs_snes_oam+0,x
	lda 9,s		;chr
	sta.w __tccs_snes_oam+2,x

	lda.w #$0200				;put $02 into MSB
	A8
	lda.l _oam_spr_mask+2,x		;get offset in the extra OAM data
	tay

	bcs +

	lda.l _oam_spr_mask+1,x
	and.w __tccs_snes_oam,y
	sta.w __tccs_snes_oam,y
	plp
	rtl
+
	lda.l _oam_spr_mask,x
	ora.w __tccs_snes_oam,y
	sta.w __tccs_snes_oam,y
	plp
	rtl


_oam_spr_mask:
	.byte $01,$fe,$00,$00,$04,$fb,$00,$00,$10,$ef,$00,$00,$40,$bf,$00,$00
	.byte $01,$fe,$01,$00,$04,$fb,$01,$00,$10,$ef,$01,$00,$40,$bf,$01,$00
	.byte $01,$fe,$02,$00,$04,$fb,$02,$00,$10,$ef,$02,$00,$40,$bf,$02,$00
	.byte $01,$fe,$03,$00,$04,$fb,$03,$00,$10,$ef,$03,$00,$40,$bf,$03,$00
	.byte $01,$fe,$04,$00,$04,$fb,$04,$00,$10,$ef,$04,$00,$40,$bf,$04,$00
	.byte $01,$fe,$05,$00,$04,$fb,$05,$00,$10,$ef,$05,$00,$40,$bf,$05,$00
	.byte $01,$fe,$06,$00,$04,$fb,$06,$00,$10,$ef,$06,$00,$40,$bf,$06,$00
	.byte $01,$fe,$07,$00,$04,$fb,$07,$00,$10,$ef,$07,$00,$40,$bf,$07,$00
	.byte $01,$fe,$08,$00,$04,$fb,$08,$00,$10,$ef,$08,$00,$40,$bf,$08,$00
	.byte $01,$fe,$09,$00,$04,$fb,$09,$00,$10,$ef,$09,$00,$40,$bf,$09,$00
	.byte $01,$fe,$0a,$00,$04,$fb,$0a,$00,$10,$ef,$0a,$00,$40,$bf,$0a,$00
	.byte $01,$fe,$0b,$00,$04,$fb,$0b,$00,$10,$ef,$0b,$00,$40,$bf,$0b,$00
	.byte $01,$fe,$0c,$00,$04,$fb,$0c,$00,$10,$ef,$0c,$00,$40,$bf,$0c,$00
	.byte $01,$fe,$0d,$00,$04,$fb,$0d,$00,$10,$ef,$0d,$00,$40,$bf,$0d,$00
	.byte $01,$fe,$0e,$00,$04,$fb,$0e,$00,$10,$ef,$0e,$00,$40,$bf,$0e,$00
	.byte $01,$fe,$0f,$00,$04,$fb,$0f,$00,$10,$ef,$0f,$00,$40,$bf,$0f,$00
	.byte $01,$fe,$10,$00,$04,$fb,$10,$00,$10,$ef,$10,$00,$40,$bf,$10,$00
	.byte $01,$fe,$11,$00,$04,$fb,$11,$00,$10,$ef,$11,$00,$40,$bf,$11,$00
	.byte $01,$fe,$12,$00,$04,$fb,$12,$00,$10,$ef,$12,$00,$40,$bf,$12,$00
	.byte $01,$fe,$13,$00,$04,$fb,$13,$00,$10,$ef,$13,$00,$40,$bf,$13,$00
	.byte $01,$fe,$14,$00,$04,$fb,$14,$00,$10,$ef,$14,$00,$40,$bf,$14,$00
	.byte $01,$fe,$15,$00,$04,$fb,$15,$00,$10,$ef,$15,$00,$40,$bf,$15,$00
	.byte $01,$fe,$16,$00,$04,$fb,$16,$00,$10,$ef,$16,$00,$40,$bf,$16,$00
	.byte $01,$fe,$17,$00,$04,$fb,$17,$00,$10,$ef,$17,$00,$40,$bf,$17,$00
	.byte $01,$fe,$18,$00,$04,$fb,$18,$00,$10,$ef,$18,$00,$40,$bf,$18,$00
	.byte $01,$fe,$19,$00,$04,$fb,$19,$00,$10,$ef,$19,$00,$40,$bf,$19,$00
	.byte $01,$fe,$1a,$00,$04,$fb,$1a,$00,$10,$ef,$1a,$00,$40,$bf,$1a,$00
	.byte $01,$fe,$1b,$00,$04,$fb,$1b,$00,$10,$ef,$1b,$00,$40,$bf,$1b,$00
	.byte $01,$fe,$1c,$00,$04,$fb,$1c,$00,$10,$ef,$1c,$00,$40,$bf,$1c,$00
	.byte $01,$fe,$1d,$00,$04,$fb,$1d,$00,$10,$ef,$1d,$00,$40,$bf,$1d,$00
	.byte $01,$fe,$1e,$00,$04,$fb,$1e,$00,$10,$ef,$1e,$00,$40,$bf,$1e,$00
	.byte $01,$fe,$1f,$00,$04,$fb,$1f,$00,$10,$ef,$1f,$00,$40,$bf,$1f,$00



;void oam_spr1(unsigned int x,unsigned int y,unsigned int chr,unsigned int off);

oam_spr1:

	php
	AXY16
	
	lda 11,s		;off
	tax

	lda 9,s			;chr
	sta.w __tccs_snes_oam+2,x
	
	A8

	lda 7,s			;y
	xba
	lda 5,s			;x
	
	A16
	
	sta.w __tccs_snes_oam+0,x

	plp
	rtl
	
	
	
;void oam_size(unsigned int off,unsigned int size);

oam_size:

	php
	AXY16

	lda 5,s			;off
	and.w #$0c
	tax

	lda 5,s			;off
	lsr a
	lsr a
	lsr a
	lsr a
	tay

	A8
	lda 7,s			;size
	bne +

	lda.w __tccs_snes_oam+$0200,y
	and.l _oam_size_mask,x
	sta.w __tccs_snes_oam+$0200,y
	plp
	rtl

+
	lda.w __tccs_snes_oam+$0200,y
	and.l _oam_size_mask,x
	ora.l _oam_size_mask+16,x
	sta.w __tccs_snes_oam+$0200,y
	plp
	rtl


_oam_size_mask:
	.byte $fd,$00,$00,$00,$f7,$00,$00,$00,$df,$00,$00,$00,$7f,$00,$00,$00
	.byte $02,$00,$00,$00,$08,$00,$00,$00,$20,$00,$00,$00,$80,$00,$00,$00



;void oam_hide_rest(unsigned int off);

oam_hide_rest:

	php
	AXY16

	lda 5,s	;off
	cmp.w #512
	bcc +
	plp
	rtl

+
	lsr a
	lsr a
	sta.b tcc__r0
	asl a
	adc.b tcc__r0	;*3
	adc.w #_oam_hide_list
	sta.b tcc__r0
	lda.w #:_oam_hide_list
	sta.b tcc__r0h

	A8
	lda.b #$e0

	jml [tcc__r0]

_oam_hide_list:
.define n 1
	.repeat 128
	sta.w __tccs_snes_oam+n
.redefine n n+4
	.endr
.undefine n

	plp
	rtl



;unsigned int hw_mul8(unsigned int a,unsigned int b);

hw_mul8:

	php

	AXY16

	lda 5,s				;a
	A8
	sta.l WRMPYA
	A16
	lda 7,s				;b
	A8
	sta.l WRMPYB
	nop					;2
	nop					;2
	nop					;2
	A16					;3
	lda.l RDMPY
	sta.b tcc__r0
	
	plp
	rtl



;void randomize(unsigned int n);

randomize:
	php

	AXY16

	lda 5,s
	xba
	and.w #$00ff
	sta.b sneslib_rand1
	lda 5,s
	and.w #$00ff
	sta.b sneslib_rand2

	plp
	rtl



;unsigned int rand(void);

rand:
	php

	AXY16

	lda.b sneslib_rand2
	lsr a
	adc.b sneslib_rand1
	sta.b sneslib_rand1
	eor.w #$00ff
	sta.b tcc__r0
	lda.b sneslib_rand2
	sbc.b tcc__r0
	sta.b sneslib_rand2

	plp
	rtl



;void set_scroll(unsigned int layer,unsigned int x,unsigned int y);

set_scroll:
	php

	AXY16

	lda 5,s			;layer
	and.w #3
	asl a
	tax

	lda 7,s			;x
	A8
	sta.l BGxHOFFS,x
	xba
	sta.l BGxHOFFS,x
	A16

	lda 9,s			;y
	A8
	sta.l BGxVOFFS,x
	xba
	sta.l BGxVOFFS,x

	plp
	rtl

.ends




.section ".snesgss_code" superfree

;void spc_load_data(unsigned int adr,unsigned int size,const unsigned char *src)

spc_load_data:

	php
	AXY16

	.ifndef DISABLE_SOUND
	
	sei

	A8
	lda.b #$aa
-
	cmp.l APU0
	bne -

	A16
	lda 11,s				;srch
	sta.b sneslib_ptr+2
	lda 9,s					;srcl
	sta.b sneslib_ptr+0
	lda 7,s					;size
	tax
	lda 5,s					;adr
	sta.l APU23
	
	A8
	lda.b #$01
	sta.l APU1
	lda.b #$cc
	sta.l APU0
	
-
	cmp.l APU0
	bne -
	
	ldy.w #0
	
_load_loop:

	xba
	lda.b [sneslib_ptr],y
	xba
	tya
	
	A16
	sta.l APU01
	A8
	
-
	cmp.l APU0
	bne -
	
	iny
	dex
	bne _load_loop
	
	xba
    lda.b #$00
    xba
	clc
	adc.b #$02
	A16
	tax
	
	lda.w #$0200			;loaded code starting address
	sta.l APU23

	txa
	sta.l APU01
	A8
	
-
	cmp.l APU0
	bne -
	
	A16
-
	lda.l APU0			;wait until SPC700 clears all communication ports, confirming that code has started
	ora.l APU2
	bne -
	
	cli					;enable IRQ
	
	.endif

	plp
	rtl


	
spc_command_asm:

	.ifndef DISABLE_SOUND

	A8

-
	lda.l APU0
	bne -

	A16

	lda.b gss_param
	sta.l APU23
	lda.b gss_command
	A8
	xba
	sta.l APU1
	xba
	sta.l APU0

	cmp.b #SCMD_LOAD	;don't wait acknowledge
	beq +

-
	lda.l APU0
	beq -

+
	.endif

	rtl

	

;void spc_command(unsigned int command,unsigned int param);

spc_command:

	php
	AXY16
	
	lda 7,s				;param
	sta.b gss_param
	lda 5,s				;command
	sta.b gss_command
	
	jsl spc_command_asm

	plp
	rtl



;void spc_stereo(unsigned int stereo);

spc_stereo:

	php
	AXY16
	
	lda 5,s			;stereo
	sta.b gss_param
	
	lda.w #SCMD_STEREO
	sta.b gss_command
	
	jsl spc_command_asm

	plp
	rtl
	
	
	
;void spc_global_volume(unsigned int volume,unsigned int speed);

spc_global_volume:

	php
	AXY16
	
	lda 7,s			;speed
	xba
	and.w #$ff00
	sta.b gss_param

	lda 5,s			;volume
	and.w #$00ff
	ora.b gss_param
	sta.b gss_param
	
	lda.w #SCMD_GLOBAL_VOLUME
	sta.b gss_command
	
	jsl spc_command_asm

	plp
	rtl
	
	
	
;void spc_channel_volume(unsigned int channels,unsigned int volume);

spc_channel_volume:

	php
	AXY16
	
	lda 5,s			;channels
	xba
	and.w #$ff00
	sta.b gss_param
	
	lda 7,s			;volume
	and.w #$00ff
	ora.b gss_param
	sta.b gss_param
	
	lda.w #SCMD_CHANNEL_VOLUME
	sta.b gss_command
	
	jsl spc_command_asm

	plp
	rtl
	
	
	
;void music_stop(void);

music_stop:

	php
	AXY16
	
	lda.w #SCMD_MUSIC_STOP
	sta.b gss_command
	stz.b gss_param
	
	jsl spc_command_asm
	
	plp
	rtl
	

	
;void music_pause(unsigned int pause);

music_pause:

	php
	AXY16
	
	lda 5,s			;pause
	sta.b gss_param
	
	lda.w #SCMD_MUSIC_PAUSE
	sta.b gss_command
	
	jsl spc_command_asm
	
	plp
	rtl
	
	
	
;void sound_stop_all(void);

sound_stop_all:

	php
	AXY16
	
	lda.w #SCMD_STOP_ALL_SOUNDS
	sta.b gss_command
	stz.b gss_param
	
	jsl spc_command_asm
	
	plp
	rtl
	
	
	
;void sfx_play(unsigned int chn,unsigned int sfx,unsigned int vol,int pan);

sfx_play:

	php
	AXY16

	lda 11,s			;pan
	bpl +
	lda.w #0
+
	cmp.w #255
	bcc +
	lda.w #255
+

	xba
	and.w #$ff00
	sta.b gss_param
	
	lda 7,s				;sfx number
	and.w #$00ff
	ora.b gss_param
	sta.b gss_param

	lda 9,s				;volume
	xba
	and.w #$ff00
	sta.b gss_command

	lda 5,s				;chn
	asl a
	asl a
	asl a
	asl a
	and.w #$00f0
	ora.w #SCMD_SFX_PLAY
	ora.b gss_command
	sta.b gss_command

	jsl spc_command_asm

	plp
	rtl
	
.ends



.section ".snes_gss_brr_streamer_code" superfree

;void spc_stream_update(void);

spc_stream_update:

	php
	AXY16

_stream_update_loop:

	lda.w __tccs_spc_stream_enable
	bne _stream_update_loop1

_stream_update_done:

	plp
	rtl

_stream_update_loop1:

	lda.w __tccs_spc_stream_stop_flag
	beq _stream_update_play

_test2:

	stz.w __tccs_spc_stream_enable

	A8

_stream_wait7:

	lda.l APU0
	bne _stream_wait7

	lda.b #SCMD_STREAM_STOP
	sta.l APU0

_stream_wait8:

	lda.l APU0
	beq _stream_wait8

	bra _stream_update_done

_stream_update_play:

	A8

_stream_wait1:

	lda.l APU0
	bne _stream_wait1

	lda.b #SCMD_STREAM_SEND
	sta.l APU0

_stream_wait2:

	lda.l APU0
	beq _stream_wait2

_stream_wait3:

	lda.l APU0
	bne _stream_wait3

	lda.l APU2
	beq _stream_update_done

	A16

	lda.w __tccs_spc_stream_block_src+0
	sta.b brr_stream_src+0
	lda.w __tccs_spc_stream_block_src+2
	sta.b brr_stream_src+2

	lda.w __tccs_spc_stream_block_repeat
	beq _norepeat

	dec.w __tccs_spc_stream_block_repeat
	ldy.w __tccs_spc_stream_block_ptr_prev

	bra _stream_update_send

_norepeat:

	ldy.w __tccs_spc_stream_block_ptr

	A8
	lda.b [brr_stream_src],y
	iny
	A16
	and.w #$00ff
	lsr a
	bcc _stream_update_repeat

	sta.w __tccs_spc_stream_block_repeat

	sty.w __tccs_spc_stream_block_ptr
	cpy.w __tccs_spc_stream_block_size
	bcc _stream_update_nojump1

	inc.w __tccs_spc_stream_block
	jsr _stream_set_block

_stream_update_nojump1:

	ldy.w __tccs_spc_stream_block_ptr_prev
	bra _stream_update_send

_stream_update_repeat:

	lda.w __tccs_spc_stream_block_ptr
	sta.w __tccs_spc_stream_block_ptr_prev
	tay
	clc
	adc.w #9
	sta.w __tccs_spc_stream_block_ptr
	cmp.w __tccs_spc_stream_block_size
	bcc _stream_update_send

	inc.w __tccs_spc_stream_block

	phy
	jsr _stream_set_block
	ply

_stream_update_send:

	A8

	lda.b [brr_stream_src],y
	iny
	sta.l APU1
	lda.b [brr_stream_src],y
	iny
	sta.l APU2
	lda.b [brr_stream_src],y
	iny
	sta.l APU3

	lda #1
	sta.l APU0
	inc a

_stream_wait5:

	cmp.l APU0
	bne _stream_wait5

	lda.b [brr_stream_src],y
	iny
	sta.l APU1
	lda.b [brr_stream_src],y
	iny
	sta.l APU2
	lda.b [brr_stream_src],y
	iny
	sta.l APU3

	lda #2
	sta.l APU0
	inc a

_stream_wait6:

	cmp.l APU0
	bne _stream_wait6

	lda.b [brr_stream_src],y
	iny
	sta.l APU1
	lda.b [brr_stream_src],y
	iny
	sta.l APU2
	lda.b [brr_stream_src],y
	iny
	sta.l APU3

	lda #3
	sta.l APU0

	jmp _stream_update_loop


_stream_set_block:

	lda.w __tccs_spc_stream_block_list+0
	sta.b brr_stream_list+0
	lda.w __tccs_spc_stream_block_list+2
	sta.b brr_stream_list+2

	lda.w __tccs_spc_stream_block
	asl a
	asl a
	tay

	lda.b [brr_stream_list],y
	iny
	iny
	sta.w __tccs_spc_stream_block_src+0
	
	lda.b [brr_stream_list],y
	iny
	iny
	sta.w __tccs_spc_stream_block_src+2

	ora.w __tccs_spc_stream_block_src+0
	bne _noeof
	
	lda.w #1
	sta.w __tccs_spc_stream_stop_flag

_noeof:

	lda.w __tccs_spc_stream_block_src+0
	sta.b brr_stream_list+0
	lda.w __tccs_spc_stream_block_src+2
	sta.b brr_stream_list+2
	
	ldy #0
	lda.b [brr_stream_list],y
	
	sta.w __tccs_spc_stream_block_size

	stz.w __tccs_spc_stream_block_repeat
	
	lda.w #2
	sta.w __tccs_spc_stream_block_ptr
	sta.w __tccs_spc_stream_block_ptr_prev

	rts

.ends
