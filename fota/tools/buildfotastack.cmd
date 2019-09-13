@echo off

set SDK="%~1.."
set INCDIR=%SDK%\include
set LIBDIR=%SDK%\lib\ble_core\%2
set SRCDIR=..\stack
set DSTDIR=.\stack
set TOOLDIR=..\tools

set LIBS=%LIBDIR%\libblelib.a %LIBDIR%\libkelib.a

set LD_FLAGS=-r -T %SRCDIR%\sections.ld --build-id=md5

mkdir 2>NUL %DSTDIR%

rem Build fota_stack object file including blelib and kelib
%TOOLDIR%\wrap.py <%SRCDIR%\fota_stack_wrapper.c >%DSTDIR%\wrap.opt
arm-none-eabi-ld %LD_FLAGS% @%DSTDIR%\wrap.opt %LIBS% -o %DSTDIR%\fota_stack.o

rem Extract dummy build ID from fota_stack.o and store it in fota_sym.o
arm-none-eabi-objcopy -j .note.gnu.build-id --rename-section .note.gnu.build-id=.rodata.fota.build-id %DSTDIR%\fota_stack.o %DSTDIR%\fota_sym.o

rem Remove dummy build ID from fota_stack.o
arm-none-eabi-objcopy -R .note.gnu.build-id %DSTDIR%\fota_stack.o

rem Check for unkown undefined symbols in fota_stack object file
arm-none-eabi-nm -u %DSTDIR%\fota_stack.o | %TOOLDIR%\check.py
