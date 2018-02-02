# Copyright (c) 2001 IBK-Landquart-Switzerland. All rights reserved.
#
# Module      :  Makefile
# Application :  IBK Open Class Library
# Purpose     :  Build ZBaseI.dll and ZBaseI.lib
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

TARGET     = ZBaseI
TARGETLIB  = ZBaseI
SRCDIR     = $(SRCROOT)\Base
TARGETS    = $(BINDIR)\$(TARGET).dll $(LIBDIR)\$(TARGETLIB).lib
TARGETDIRS = $(INCDIR) $(BINDIR) $(LIBDIR)
LIBRARIES  = advapi32.lib
CLEANUPS   =
CDEFS      =

OBJECTS    = \
  ZAsync.obj \
  ZDate.obj \
  ZDynLink.obj\
  ZExcept.obj \
  ZFile.obj \
  ZPlatfrm.obj \
  ZProcess.obj \
  ZProfile.obj \
  ZRegular.obj \
  ZSql.obj \
  ZString.obj \
  ZPtrlist.obj \
  ZTrace.obj

INCLUDES   = \
  $(INCDIR)\ZAsync.hpp \
  $(INCDIR)\ZBase.hpp \
  $(INCDIR)\ZDate.hpp \
  $(INCDIR)\ZDynLink.hpp \
  $(INCDIR)\ZExcept.hpp \
  $(INCDIR)\ZFile.hpp \
  $(INCDIR)\ZPlatfrm.hpp \
  $(INCDIR)\ZProcess.hpp \
  $(INCDIR)\ZProfile.hpp \
  $(INCDIR)\ZRegular.hpp \
  $(INCDIR)\ZSql.hpp \
  $(INCDIR)\ZString.hpp \
  $(INCDIR)\ZPtrlist.hpp \
  $(INCDIR)\ZStrlist.hpp \
  $(INCDIR)\ZTrace.hpp \
  $(INCDIR)\ZBase.h

!include ..\makelib
