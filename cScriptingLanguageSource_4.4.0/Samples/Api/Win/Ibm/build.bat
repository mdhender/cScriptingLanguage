@echo off

set vacpp=i:\vacpp35
set path=..\..\..\..\win\bin;%vacpp%\bin;%path%
set include=..\..\..\..\include;%vacpp%\include;%include%
set lib=..\..\..\..\win\lib;%vacpp%\lib;%lib%

echo.
echo Building Embed.exe
echo ------------------
echo.
icc /Gm /Gd ..\..\Source\Embed.c

echo.
echo Building MyLib.dll (run 'csl testlib' to test it)
echo -------------------------------------------------
echo.
icc /Gm /Gd /Ge- /B"/dll" ..\..\Source\MyLib.c

copy ..\..\Source\testlib.csl
