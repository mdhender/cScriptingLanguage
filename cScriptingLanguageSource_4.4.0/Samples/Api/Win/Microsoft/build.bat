@echo off

set msvc=i:\msvc50

set path=..\..\..\..\win\bin;%msvc%\vc\bin;%msvc%\SharedIDE\bin;%path%

set include=..\..\..\..\include;%msvc%\vc\include;%include%

set lib=..\..\..\..\win\lib;%msvc%\vc\lib;%lib%

echo.
echo Building Embed.exe
echo -------------------
echo.
cl ..\..\Source\Embed.c

echo.
echo Building MyLib.dll (run 'csl testlib' to test it)
echo -------------------------------------------------
echo.
cl /GD /LD /MDd ..\..\Source\MyLib.c /link /DEF:MyLib.def

copy ..\..\Source\testlib.csl
