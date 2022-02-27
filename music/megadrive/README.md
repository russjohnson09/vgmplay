With just the SN76489 playing for Greeen Hill Zone
you can hear what the sn was mainly focused on
which was sound effects, noise, and precussion.



SN76489 just playing for the SF prologue you
can hear a lot more of the main melody especially
at the first half.

The second half it mostly produces noise.


YM2612


https://segaretro.org/SN76489#:~:text=PSG,-See%20Sega%20Master&text=The%20Sega%20SN76496%2C%20also%20known,and%20Mega%20Drive%20game%20consoles.


https://segaretro.org/Mega_Drive_official_documentation


SN 76489 AN

Sega PSG is a modified version based off of the SN 76489


https://www.youtube.com/watch?v=0N5vNKG4F_0



			case 0x04:
				RetStr = "SN76496";
				break;
			case 0x05:
				RetStr = "SN94624";
				break;
			case 0x06:
				RetStr = "SEGA PSG";
				break;


Used chips:     SEGA PSG, YM2612


TODO what is are the differences in emulation between the sega psg and 
case 0x01:
    RetStr = "SN76489";
    break;

GetAccurateChipName    






- fixed SN76489 PSG muting (muting didn't work if the frequency was set to 0 to play PCM)




vgmspec171.txt has contains the spec for vgm files.

vgz are also supported as a comporessed version of vgm.



/*#define MAX_SN76489     4*/

/*
    More testing is needed to find and confirm feedback patterns for
    SN76489 variants and compatible chips.
*/
enum feedback_patterns {
    FB_BBCMICRO =   0x8005, /* Texas Instruments TMS SN76489N (original) from BBC Micro computer */
    FB_SC3000   =   0x0006, /* Texas Instruments TMS SN76489AN (rev. A) from SC-3000H computer */
    FB_SEGAVDP  =   0x0009, /* SN76489 clone in Sega's VDP chips (315-5124, 315-5246, 315-5313, Game Gear) */
};


https://forums.sonicretro.org/index.php?threads/pcm-on-the-genesis.13533/


russ@DESKTOP-R0T9071 MINGW64 /d/documents/dev/vgmplay/VGMPlay (feat/remove-sn76489)
$ ./vgmplay.exe ../music/megadrive/01\ -\ \[Prologue\].vgm 
VGM Player
----------

File Name:      01 - [Prologue].vgm

Track Title:    [Prologue]
Game Name:      Shining Force
System:         Sega Mega Drive / Genesis
Composer:       Masahiko Yoshimura
Release:        1992
Version:        1.50      Gain: 1.00    Loop: Yes (00:35.25)
VGM by:         Duchemole & Danjuro
Notes:

Used chips:     GetChipClock
GetChipClock
SEGA PSG, GetChipClock
GetChipClock
GetChipClock
YM2612, GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock
GetChipClock


GetChipClock ...
GetChipClock
GetChipClock
Playing 99.50%  00:36.60 / 00:36.72 seconds
Playing finished.



atmega328p has a clock speed of 20 MHz

https://segaretro.org/Sega_Mega_Drive/Technical_specifications#CPU

CPU
Main CPU: Motorola 68000
Clock rate: 7.670454 MHz (NTSC), 7.600489 MHz (PAL)



SoundLogging


	case 0x02:
		if (LogToWave)
		{



	PlayTimeEnd = 0;
	QuitPlay = false;
	while(! QuitPlay)