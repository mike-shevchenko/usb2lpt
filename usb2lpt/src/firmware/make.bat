@echo off
if "%1"=="clean" goto %1
c:\programme\ASEM51\Asemw.exe /INC:c:\programme\ASEM51\mcu /COLUMNS USB2LPT.A51
if errorlevel 1 goto stop
c:\programme\ASEM51\Asemw.exe /INC:c:\programme\ASEM51\mcu /COLUMNS USB2LPT2.A51
if errorlevel 1 goto stop
c:\Programme\cypress\usb\Bin\hex2bix.exe -I -V 0x16C0 -P 0x06B3 USB2LPT.HEX
c:\Programme\cypress\usb\Bin\hex2bix.exe -I -V 0x16C0 -P 0x06B3 -F 0xC2 -C 0x01 USB2LPT2.HEX
goto exi

:clean
rem  Nur die IIC-Dateien bleiben stehen
del USB2LPT.HEX
del USB2LPT2.HEX
del USB2LPT.LST
del USB2LPT2.LST
goto exi

:stop
echo Assembler-Fehler!
pause
:exi
