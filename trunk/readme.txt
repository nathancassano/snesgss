SNES Game Sound System (or Solution)

v1.02 25.08.14 - minor fixes, SPC700 driver improvements
v1.01 18.08.14 - important bug fixes, more features, transpose dialog
v1.0  15.08.14 - first release version, with all planned features
      22.07.14 - development of the new system has started
        ~06.11 - the old system, including spctool, xm2data, and SPC700
		         code; the new system is based on its parts.


The sole purpose of this software is to make music and sound effects for 
SNES homebrew games. It is not meant to be a stand-alone music authoring 
software in any way. 



The editor works in semi-interactive mode. This means that the result of 
any changes (even mute controls) will only heard after restarting the 
song, you can't hear the changes immediately. It is supposed that during 
editing the song is mostly played using the Enter key (play from current 
position). 

The system architecture uses a single sample bank for every song and 
sound effect in a game. The sound driver along with sample bank is fully 
loaded at start up. Sound effects created as shorter songs (marked as 
sound effect), they also loaded along with the driver. Larger songs are 
loaded on a one-by-one basis. It is kind of MIDI concept, music files 
only contain notes and stuff, and sound bank contains the actual sounds 
that used by the music files. 

It is supposed that WAV samples are resampled and looped in an external 
audio editor, Wavosaur or similar. The samples should be 16 bit mono, 
sampled at 8000, 16000, or 32000 Hz (the latter is preferable), with 
melodic content preferably tuned to B (of any octave) +21 cent. In other 
words, change pitch of a C-tuned sample 79 cents down. The reason behind 
this is the BRR encoding that works with 16 sample blocks, with this 
tuning a simple waveform will be a multiple of 16, improving looping 
quality. Pitch table is calculated to map the B +21 sampled instrument 
to be actual C note on the C key, giving the standard tuning. To play
a sample close to its original sample rate, use B key.

The editor could be called as a command line application to perform 
project file to set of resulting files conversion. Use snesgss.exe 
filename.gsm -e [optional export path] 

The MIDI import supports both format 0 and 1. It expects that notes are 
placed in channels 1-8 (or 1-6 plus drum channel), and there is not more 
than 1 note in a channel at a time. It imports notes only, with 
instrument assigned by the channel number. If the drum channel is 
present, it gets imported into SNES channels 7-8, channel 7 is used for 
hats, and channel 8 for kick, snare, and toms. Instruments numbers 
assigned to different drums are 10 for kick, 11 for snare, 12 for toms, 
13 for hats. 



General

F1 				Song editor
F2 				Instrument editor

Song editor

Up,Down			Move the cursor one row up/down
PgUp,PgDown		Move the cursor one page up/down, the page size is set in the config file

Left,Right		Move the cursor between columns
Tab,Shift+Tab 	Move the cursor between the same columns of the channels

Numbers			Enter a number value (speed, instrument number, effect value)
Note keys		Enter a note in the song editor, or play a note in the instrument editor

Delete			Delete a note, a number, or selection without shifting other notes
Backspace		Delete a note field, shifting everything below and moving the cursor up
Insert			Insert a blank note field, shifting everything below down

Ctrl+G			Edit current row number, to navigate through the song faster
                Pressing Enter returns the cursor back to the position where it was originally

Ctrl+1..8		Toggle channel mute
Ctrl+0			Toggle solo mode for current channel
		
[ ]             Change auto step, 0 to 16

Numpad / * 		Change octave, 1 to 8
Numpad 1..8		Set octave

F5				Play song from beginning
F7				Play song from current row
F8				Stop playing
Enter			Hold down to play from current row

Space			Set navigation marker on current row
Home			Go to previous marker
End				Go to next marker

Ctrl+[ ]		Change measure, each song has its own measure

Ctrl+Home		Set loop position (the row that will be played after loop)
Ctrl+End		Set song length (the last actual row)

Ctrl+Z			Undo last change in the song text, one step

Shift+Cursor	Set block selection for block functions

Ctrl+L			Select current channel, all fields

Ctrl+X			Cut selection
Shift+X			Cur selection and shift the rows below up
Ctrl+C			Copy selection
Ctrl+V			Paste selection
Shift+V			Paste selection and shift existing rows down

Ctrl+F1			Transpose selection semitone down
Ctrl+F2			Transpose selection semitone up
Ctrl+F3			Transpose selection octave down
Ctrl+F4			Transpose selection octave up

Ctrl+E			Expand selection
Ctrl+S			Shrink selection

Numpad + -		Switch between sub songs


Clicking on the pattern header changes the channel mute setting.
Left click toggles the mute for a channel, right click toggles the solo mode for a channel.

Click left mouse button and move the mouse while holding the button to make a block selection.



Row fields

RRRR SS [CH1..CH8]

RRRR is the row number
SS   is speed, could be set per row, lower values mean faster speed
CH1..CH8 are note fields, see below



Note fields

C#3iiEvv

C  is note (# sharp)
ii is the instrument number, 1..99
E  is the effect name
vv is the effect value, only editable if the effect is specified



Effects

All effect values are decimal, in 0..99 range.

All effects applied to the channel continuously. To turn off an effect, use it with 00 as value

Vxx is Volume, 0 is lowest, 99 is highest
Txx is (de)Tune, sets constant pitch offset that is applied to all following notes
Uxx is Slide Up, 0 is none, 1 is slowest, 99 is fastest
Dxx is Slide Down
Pxx is Portamento, 0 is note, 1 is slowest, 99 is fastest; has priority over the Slide
P99 is legato mode, pitch changes instantly, like on a new note, but without keyon
Mxy is Modulation (pitch vibrato), x is speed (1 slowest, 9 fastest), y is depth (1 min, 9 max)



Note keys

1 or A is the rest note

Current octave

 S D   G H J   L :
Z X C V B N M < > ?

Next octave

 2 3  5 6 7  8 9
Q W E R T Y U I O P



Music data format

Header:

+0 1 how many channels in the song
+1 2 absolute address of the first channel
+3 2 absolute address of the second channel (if exists)
...

Channel:

Series of bytes interpreted as following:

  0..149       short delay
150..245       note in semitones (C-1..B-8)
     246       keyoff
     247,L,H   long delay, followed with two bytes of the delay duration, up to 65535
248..253       song effects:
     248,V     volume, byte of the volume is 0..127
     249,D     detune, byte of the detune
     250,S     slide up or down, byte of the slide is signed -99..99
     251,P     portamento, byte of the slide is 0..99
     252,M     vibrato, byte of the vibrato parameters (high nibble is depth 0..9, low nibble is speed 0..9)
     253,L,H,C reference, next three bytes are reference absolute address and reference length (0..255 bytes)
     254,I     instrument change, followed with byte of the instrument number 0..99
     255,L,H   loop, followed with two bytes of absolute address of the loop point
