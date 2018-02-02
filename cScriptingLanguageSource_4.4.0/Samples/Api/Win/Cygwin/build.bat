@echo off

rem -- set up path
rem -- you will have to adapt this to your installation
set tmpdir=
set path=i:\cygnus\cygwin-b20\H-i586-cygwin32\bin;%path%

rem -- set up include and library shortcuts
set cslinc=-I..\..\..\..\Include
set csllib=..\..\..\..\Win\Lib\ZCslApiG.a

echo Building %cslname%.exe
set cslname=Embed
set cslsrc=..\..\Source\%cslname%.c
gcc -o %cslname%.exe -g -Wall %cslinc% %cslsrc% %csllib%

echo.

echo Building %cslname%.dll
set cslname=MyLib
set cslsrc=..\..\source\%cslname%.c ..\..\Source\DllInit.c

echo ...compiling
gcc -c %cslinc% -g -Wall %cslsrc%

echo ...linking
dllwrap --entry __cygwin_noncygwin_dll_entry@12 -o %cslname%.dll %cslname%.o DllInit.o %csllib% -A
rem dllwrap --add-stdcall-alias --target i386-cygwin -o %cslname%.dll %cslname%.o DllInit.o %csllib%

copy ..\..\Source\testlib.csl

set cslinc=
set csllib=
set cslname=
set cslsrc=
