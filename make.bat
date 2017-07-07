@echo off

@echo Compiling...
mbed compile
@echo Done.

@echo Flashing to %1...
python.exe tools\nxpprog.py --baud=230400 --cpu=lpc1768 --oscfreq=120000 %1 BUILD\LPC1768\GCC_ARM\ABK-firmware.bin
@echo Done.


