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
