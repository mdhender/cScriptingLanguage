@echo off

set bccdir=i:\bcc55
set path=..\..\..\..\Win\Bin;%bccdir%\bin;%path%
set include=..\..\..\..\Include;%bccdir%\include;%include%
set lib=..\..\..\..\Win\Lib;%bccdir%\lib;%lib%

echo.
echo Building Embed.exe
echo ------------------
echo.
bcc32 -I%include% -L%lib% ..\..\Source\Embed.cpp

echo.
echo Building MyLib.dll (run 'csl testlib' to test it)
echo -------------------------------------------------
echo.
bcc32 -w-par -tWCDE -I%include% -L%lib% ..\..\source\MyLib.cpp
copy ..\..\Source\testlib.csl
