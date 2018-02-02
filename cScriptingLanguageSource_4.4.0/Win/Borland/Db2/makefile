# Copyright (c) 2001 IBK-Landquart-Switzerland. All rights reserved.
#
# Module      :  Makefile
# Application :  IBK Open Class Library
# Purpose     :  Build ZDb2B.dll
#
# Date        Description                                 Who
# --------------------------------------------------------------------------
# 2001.06.09  First implemention                          P.Koch, IBK
#
# OPEN SOURCE LICENSE
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published by
# the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to IBK at info@ibk-software.ch.

!include ..\location

TARGET     = ZDb2B
TARGETLIB  =
SRCDIR     = $(SRCROOT)\Db2
TARGETS    = db2clib.lib $(BINDIR)\$(TARGET).dll
TARGETDIRS = $(BINDIR)
LIB        = $(DB2)\lib;$(LIB)
LIBRARIES  = db2clib.lib
CLEANUPS   = db2clib.lib
CDEFS      =
OBJECTS    = ZDb2.obj
INCLUDE    = $(DB2)\include;$(INCLUDE)
INCLUDES   = $(SRCDIR)\ZDb2.hpp

!include ..\makelib

db2clib.lib : {$(LIB)}db2cli.lib
   @echo create db2 cli library for bcc
   @coff2omf -q -lib:ca $** $@
