@echo off
set msvc50=i:\msvc50
set db2=i:\sqllib
set oracle=i:\orant\oci
set mysql=i:\mysql

set path=%msvc50%\vc\bin;%msvc50%\SharedIDE\bin;%mysql%\lib\opt;%path%
set include=%msvc50%\vc\include
set lib=%msvc50%\vc\lib
