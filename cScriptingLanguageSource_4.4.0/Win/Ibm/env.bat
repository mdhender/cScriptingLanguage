@echo off
set vacpp35=i:\vacpp35
set db2=i:\sqllib
set oracle=i:\orant\oci
set mysql=i:\mysql

set path=%vacpp35%\bin;%mysql%\lib\opt;%path%
set include=%vacpp35%\include;%vacpp35%\sdk\include;
set lib=%vacpp35%\lib;%vacpp35%\sdk\lib
