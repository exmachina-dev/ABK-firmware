@echo off

@echo Compiling...
mbed compile
@echo Done.

@echo Flashing...
python.exe tools\nxpprog.py --baud=230400 --cpu=lpc1768 --oscfreq=120000 COM8 BUILD\LPC1768\GCC_ARM\ABK-firmware.bin
@echo Done.


