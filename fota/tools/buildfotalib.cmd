@echo off

set SRCDIR=..\stack
set DSTDIR=.\stack
set TOOLDIR=..\tools

set LIB=libfota.a

rem Add exported FOTA stack symbols
arm-none-eabi-nm -gnfs "%~1" | %TOOLDIR%\extractsym.py > %DSTDIR%\addsym.opt
arm-none-eabi-objcopy @%DSTDIR%\addsym.opt %DSTDIR%\fota_sym.o

rem Add FOTA stack build ID
%TOOLDIR%\extractid.py "%~n1.bin"
arm-none-eabi-objcopy --update-section .rodata.fota.build-id="%~n1.id" %DSTDIR%\fota_sym.o

rem Build FOTA support libary
rm 2>NUL %LIB%
arm-none-eabi-ar -r %LIB% %DSTDIR%\fota_startup.o %DSTDIR%\fota_system.o  %DSTDIR%\fota_sym.o
