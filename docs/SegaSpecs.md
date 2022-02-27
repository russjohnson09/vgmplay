

# YM2612 
is the main sound chip and was paired with the SN76489 to produce
the Sega Mega Drive Sound.



https://en.wikipedia.org/wiki/Yamaha_YM2612


https://en.wikipedia.org/wiki/Texas_Instruments_SN76489



# SN76489

Digital Complex Sound Generator

https://www.youtube.com/watch?v=gkgWgLmYFnE


Does more than I thought in terms of the actual music.

The main melody can still be heard without the YM2612.


Used chips:     SEGA PSG, YM2612



# Stereo Sound
https://www.reddit.com/r/SEGAGENESIS/comments/dayifd/what_games_specifically_benefit_from_stereo_sound/

Sonic The Hedgehog rings alternate left and right. The music of green hill zone you can definitely hear
the stereo sound as well.

I've heard that it is psuedo-stereo that needs to alternate between left and right. For me, I don't
think I can hear the difference between that and true stereo.




	CAA->SmpP = 0x00;
	CAA->SmpLast = 0x00;
	CAA->SmpNext = 0x00;
	CAA->LSmpl.Left = 0x00;
	CAA->LSmpl.Right = 0x00;


The chip will store the audio it found by interpreting the vgm file.

The ResampleChipStream will get the chip stream and put it into a wav buffer.


FillBuffer

InterpretFile

InterpretVGM // fill chip stream

InterpretVGM makes a call to ChipMapper->chip_reg_write.

This is the write to chip command. So just the low level command to set
a pin to HIGH or LOW that will be interpretted by the software emulation.

ChipMapper chip_reg_write ->
2612intf.c ym2612_w ->
fm2612.c ym2612_write





ResampleChipStream // get chip stream to buffer.




// When initializing the chip.
CAA->StreamUpdate = &ym2612_stream_update;

Used during the ResampleChipStream step.






