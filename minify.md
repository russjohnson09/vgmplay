Minify this project to only include support for sega genesis.

Specifically to play the original shining force soundtrack.





# Build And Test

1. open MSYS and run `make WINDOWS=1` in VGMPlay's folder.
2. ./VGMPlay/vgmplay.exe 01\ -\ \[Prologue\].vgm 

# Clean Rebuild
make clean
make WINDOWS=1


# Supported Consoles
snes might not be supported.

./vgmplay.exe ../music/snes/01\ Title.vgz

This is picking it up as a NES APU chip. It plays but poorly.

# Sega Genesis Specs



# VGMPlay

Chips_GeneralActions

This will detect the chip to use from the header.

	
switch(Mode)
{
case 0x00:	// Start Chips


if (VGMHead.lngHzYM2612)



GetChipClock

/*
                        FbMask  Noise Taps  Negate Stereo Dv Freq0		Fb	SR	Flags
    01 SN76489		 0x4000, 0x01, 0x02, TRUE,  FALSE, 8, TRUE		03	0F	07 (02|04|00|01) [unverified]
    02 SN76489A		0x10000, 0x04, 0x08, FALSE, FALSE, 8, TRUE		0C	11	05 (00|04|00|01)
    03 SN76494		0x10000, 0x04, 0x08, FALSE, FALSE, 1, TRUE		0C	11	0D (00|04|08|01)
    04 SN76496		0x10000, 0x04, 0x08, FALSE, FALSE, 8, TRUE		0C	11	05 (00|04|00|01) [same as SN76489A]
    05 SN94624		 0x4000, 0x01, 0x02, TRUE,  FALSE, 1, TRUE		03	0F	0F (02|04|08|01) [unverified, SN76489A without /8]
    06 GameGear PSG	 0x8000, 0x01, 0x08, TRUE,  TRUE,  8, FALSE		09	10	02 (02|00|00|00)
    06 SEGA VDP PSG	 0x8000, 0x01, 0x08, TRUE,  FALSE, 8, FALSE		09	10	06 (02|04|00|00)
    07 NCR8496		 0x8000, 0x02, 0x20, TRUE,  FALSE, 8, TRUE		22	10	07 (02|04|00|01)
    08 PSSJ-3		 0x8000, 0x02, 0x20, FALSE, FALSE, 8, TRUE		22	10	05 (00|04|00|01)
    01 U8106		 0x4000, 0x01, 0x02, TRUE,  FALSE, 8, TRUE		03	0F	07 (02|04|00|01) [unverified, same as SN76489]
    02 Y2404		0x10000, 0x04, 0x08, FALSE, FALSE; 8, TRUE		0C	11	05 (00|04|00|01) [unverified, same as SN76489A]
    -- T6W28		0x10000, 0x04, 0x08, ????,  FALSE, 8, ????		0C	11	?? (??|??|00|01) [It IS stereo, but not in GameGear way].
*/

PlayVGM


