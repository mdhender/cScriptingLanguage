# Copyright (c) 2001 IBK-Landquart-Switzerland. All rights reserved.
#
# Module      :  Makefile
# Application :  IBK Open Class Library
# Purpose     :  Build ZCsl.dll and ZCslI.lib
#
# Date        Description                                 Who
# --------------------------------------------------------------------------
# 2001.06.08  First implemention                          P.Koch, IBK
# 2001.07.07  Rename from CSS to CSL                      P.Koch, IBK
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

TARGET     = ZCslI
TARGETLIB  = ZCslI
SRCDIR     = $(SRCROOT)\Csl
TARGETS    = $(BINDIR)\$(TARGET).dll $(LIBDIR)\$(TARGETLIB).lib
TARGETDIRS = $(INCDIR) $(BINDIR) $(LIBDIR)
LIBRARIES  =
CLEANUPS   =
CDEFS      =

OBJECTS    = \
  Block.obj \
  Compile.obj \
  Function.obj \
  Input.obj \
  Instruct.obj \
  Interprt.obj \
  Library.obj \
  Parametr.obj \
  Utility.obj \
  Variable.obj \
  ZCsl.obj

INCLUDES   = \
  $(INCDIR)\ZCsl.h \
  $(INCDIR)\ZCsl.hpp \
  $(INCDIR)\ZCss.hpp \
  $(SRCDIR)\Internal.h

!include ..\makelib
