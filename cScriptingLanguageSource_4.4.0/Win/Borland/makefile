# Copyright (c) 2001 IBK-Landquart-Switzerland. All rights reserved.
#
# Module      :  Makefile
# Application :  IBK Open Class Library
# Purpose     :  Build all targets for Borland BCC
#
# Date        Description                                 Who
# --------------------------------------------------------------------------
# 2001.06.09  First implemention                          P.Koch, IBK
# 2001.07.05  Add odbc interface                          P.Koch, IBK
# 2001.07.07  Rename CSS to CSL                           P.Koch, IBK
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

.SUFFIXES:

TARGETS=\
  RegEx.bld \
  Base.bld \
  Csl.bld \
  CslApi.bld \
  CslWrap.bld \
  CslExe.bld \
  Odbc.bld \
  Oracle.bld \
  Db2.bld \
  Mysql.bld \
  ComLib.bld \
  DaxLib.bld \
  FileLb.bld \
  MathLb.bld \
  PrfLib.bld \
  RexLib.bld \
  StrLib.bld \
  SysLib.bld \
  WinLib.bld

all: $(TARGETS)

$(TARGETS):
  !@echo --------------------------------------------------------
  !@echo Build $*
  !@cd $*
  !@$(MAKE) -s -$(MAKEFLAGS)
  !@cd ..

install : all
  @echo --------------------------------------------------------
  @echo Installing csl kernel and libraries
  @copy /y bin\*.* ..\bin >nul

clean : $(TARGETS:.bld=.cln)
  @echo remove .\Bin
  @rd /S /Q Bin

$(TARGETS:.bld=.cln) :
  !@echo Clean up $*
  !@cd $*
  !@$(MAKE) -s clean
  !@cd ..
