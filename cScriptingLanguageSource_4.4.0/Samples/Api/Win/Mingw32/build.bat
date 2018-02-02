@echo off

rem -- set up path
rem -- you will have to adapt this to your installation
set tmpdir=
set path=i:\gcc-2.95.2\bin;%path%

rem -- set up include and library shortcuts
set cslinc=-I../../../../include
set csllib=../../../../win/lib/ZCslApiG.a

set cslname=Embed
echo Building %cslname%.exe
set cslsrc=..\..\source\%cslname%.c
gcc -o %cslname%.exe -g -Wall %cslinc% %cslsrc% %csllib%
strip %cslname%.exe

echo.

set cslname=MyLib
echo Building %cslname%.dll
set cslsrc=..\..\source\%cslname%.c

echo compiling
gcc -c %cslinc% -g -Wall %cslsrc% ..\..\source\DllInit.c

echo linking
dllwrap --add-stdcall-alias --target i386-mingw32 -o %cslname%.dll %cslname%.o DllInit.o %csllib%
strip %cslname%.dll

copy ..\..\Source\testlib.csl

set cslinc=
set csllib=
set cslname=
set cslsrc=
