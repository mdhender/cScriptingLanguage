                                                           C SCRIPTING LANGUAGE
                                                                         README
                                                      VERSION 4.4.0, 05/26/2002



                 (C) COPYRIGHT INFORMATIK-BUERO KOCH 1998-2002
                             ALL RIGHTS RESERVED.



      +-----------------------------------------------------------------+
      |  NOTE: This file is easier to read if you change your font to a |
      |        monospace style.                                         |
      +-----------------------------------------------------------------+



  CONTENTS
  --------

  1.0   ABSTRACT
  1.1   LICENSE
  1.2   CONTACT AND LINKS

  2.0   INSTALLATION
  2.1   WIN32
  2.2   OS/2
  2.3   UNIX FAMILY
  2.3.1 SOURCE DISTRIBUTIONS
  2.3.2 BINARY DISTRIBUTIONS

  3.0  RELEASE NOTES


  1.0  ABSTRACT
  -------------

  The C Scripting Language (CSL) is an embedable scripting language with C
  syntax. If you know C or C++ you will immediately feel familiar with CSL.

  CSL comes with a comprehensive set of libraries enabling to write powerful
  applications for any purpose:

  - System functions
  - Strings
  - File handling
  - Maths
  - Asynchronous communications
  - Regular expressions
  - Registry- and profile control
  - Window control
  - Database access (including interfaces for DB2, ORACLE, MySQL and ODBC)

  While CSL contains an executive for running scripts from the command line,
  there are also 2 sets of interfaces available for writing CSL libraries
  and embedding the CSL engine into your own applications:

  - C API for almost every popular C and C++ compiler
  - C++ class library for selected compilers


  1.1  LICENSE
  ------------

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License version 2 as published
  by the Free Software Foundation

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to IBK at info@ibk-software.ch.

  You will need a special license if you want to publish an application
  including parts of CSL with any other license model than GPL. Contact
  IBK at info@ibk-software.ch for further details.


  1.2 CONTACT AND LINKS
  ---------------------

  Author:

    Peter Koch

    IBK Software
    Center Au
    7302 Landquart
    Switzerland

    peter.koch@ibk-software.ch


  Home site:

    http://csl.sourceforge.net


  2.0  INSTALLATION
  -----------------

  If an old version of CSL is installed, it is desperately recommended to
  uninstall it first. Remove any files remaining in the installation directory
  after the uninstall process before reinstalling the new version.

  Although CSL should work properly also without removing the old version,
  there may be some outdated files remaining on your disk.


  2.1  WIN32
  ----------

  Unzip the downloaded archive csl-x.y.z.win.zip in a temporary directory
  and run SETUP.EXE. If you are using WinZip you may start setup.exe within
  WinZip.

  Follow the directions given by SETUP.EXE. After installation you may
  delete the temporary directory.


  2.2  OS/2
  ---------

  Unzip the downloaded archive csl-x.y.z.os2.zip in a temporary directory.

  NOTE: If you are using good old pkunzip2.exe you have to rename the archive
        to a 8.3 name and use the -d switch to retain directory structure as:

        ren csl-x.y.z.os2.zip csl.zip
        pkunzip2 -d csl

  Run INSTALL.EXE and follow the directions. After installation you may
  delete the temporary directory.


  2.3  UNIX FAMILY
  ----------------

  2.3.1 SOURCE DISTRIBUTIONS
  --------------------------

  Unpack the archive as (replace x.y.z by csl version):

      tar xzvf csl-x.y.z.tar.gz

  Change to the directory csl-x.y.z/Unix/Gnu and read the file INSTALL in that
  directory for further instructions.


  2.3.2 BINARY DISTRIBUTIONS
  --------------------------

  You must be root to install the binary distribution!

  o Unpack the downloaded archive csl-x.y.z-sys-arch.tar.gz.
  o Change to the directory csl-x.y.z-sys-arch.
  o Unpack the archive files.tar to the desired destination.
    (The standard destination is /usr/local)
  o Run ldconfig to update system cache with csl libraries

  Example:
      tar xzf csl-4.3.0-FreeBSD-i386.tar.gz
      cd csl-4.3.0-FreeBSD-i386
      tar -C /usr/local -xf files.tar
      ldconfig

  NOTES:

  o You may drop the install directory after installation, it is not required
    for using csl:

      cd ..
      rm -rf csl-4.3.0-FreeBSD-i386

  o CSL installs into these directories (assumed you installed to /usr/local):

      /usr/local/bin                 executable program
      /usr/local/lib                 libraries
      /usr/local/include             include files
      /usr/local/share/csl           scripts
      /usr/local/share/csl/samples   samples
      /usr/local/share/doc/csl/html  documentation

  o You might have to set the CSLPATH in case you want to use the library
    headers for dynamic load:

      export CSLPATH = /usr/local/share/csl


  3.0 RELEASE NOTES
  -----------------

  Version 4.4.0

    o Added daxCheckCursor() to dax library

    o Fixed bug in switch statements incrementing stack with
      every execution

    o Fixed bug (GPF) when stack was relocated between try and catch
      and exception was thrown thereafter


  Version 4.3.0

    o Extended winPostVKey to enable posting of combined keys as Alt-F or
      Shift-Alt-F7

    o Modifications in makefiles and source for FreeBSD

    o Binary dirstributions for Linux-i386 and FreeBSD-i386

    o Fixed a bug causing error 'function not implemented' in some cases
      (Thanks to Ghislain Picard)


  Version 4.2.1

    o Fixed bug when loading a predeclared function at runtime

    o Fixed bug not preventing to load a library twice


  Version 4.2.0

    o Enhanced trace facilities

    o Optimized function calls for speed

    o Fixed bug in a special combination of 'if' and 'return'

    o Windows setup program:

      o Set environment vars for current user instead of system.

      o Enclose parameter of associations in double quotes to handle
        paths containing spaces.


  Version 4.1.0

    o Renamed to C Scripting Language. CSL is backward compatible to CSS, but
      programs and libraries using the API and class interface will need
      recompilation. Use the *Csl* API and Class Interface for new projects.

    o System library constants added: sysIsBSD, sysIsAIX, sysIsSolaris,
      sysIsUnixFamily

    o Added ODBC interface.

    o Changes to ease porting to other unixish systems.


  Version 4.0.2

    o Fixed a bug in MySQL interface leading to GPF at second parse in some
      configurations.

    o Minor changes in the ZString class.

    o Corrections in DB2 interface to make it compile without warnings by
      GCC. (Makefiles for the Oracle and Db2 interfaces are however still not
      included in the Linux package.)

    o Added a complex script sample "Tiny SQL Shell" (tss,tss.bat,tss.cmd).
      Tss is a SQL shell you can use with any database supported by CSS.
      Start tss and enter "?" for instructions.


  Version 4.0.1

    o Added MySQL interface to the binary OS/2 distribution.

    o Some minor code changes for upcoming GCC 3.0. (GCC 3.0 is however
      still to buggy to compile CSS)

    o Minor documentation updates.


  Version 4.0

    o CSS is now free software and open source, e.g. the source code is
      available to everybody who accepts the general public license as stated
      above.

    o The database access library has now an interface for MySQL. This enables
      you to use a free database which is very popular especially for CGI
      scripting.

    o The functions sysProfile, sysUserProfile and sysSystemProfile have been
      replaced by a profile library allowing now complete control of the
      windows registry as well as of OS/2 profiles. The profile library is not
      available for Linux however.

    o New directives: #if / #else / #endif, #message and #error.

    o New operator 'exists' and corresponding API's to check var/const
      existence.

    o Several system and library constants have been added: MAXLONG,
      PATHSEPARATOR, sysIsOS2, sysIsLinux, sysIsWindows, sysIsWin95Family,
      sysIsWinNTFamily, sysIsWinServer, sysIsWin95, sysIsWin98, sysIsWinME,
      sysIsWinNT3, sysIsWinNT4, sysIsWin2000, sysIsWinXP, xxxCompiler, xxxBuilt
      and xxxLibtype (where "xxx" is replaced by "css" or the library prefix
      respectively).

    o A bunch of new library functions have been added: strLastIndexOf,
      sysDateTime, sysStartDateTime, sysDateFormat, sysTimestamp,
      sysStartTimestamp, sysSecondsSince, comInputChars.

    o Function enhancements in strSubString, strIndexOf, strChange, strStrip,
      strStripLeading, strStripTrailing, daxConnect, daxDisconnect. The
      functions are still backward compatible with existing scripts however.

    o All libraries have been renamed from K..... to Z...... during the rewrite
      of CSS in order to make it open source. You will have to change that in
      existing scripts. Also if you have used the API or class library with
      former versions, you will have to change all "KCss" in your C/C++ source
      files to "ZCss". Check the manual for further details.


  Version 3.05a

    o Make environment vars and file associations configurable in setup
      program.

    o Recompiled with VisualAge C++ V3.5.9


  Version 3.05

    o Improved setup program for windows: Setup.exe now installs file associ-
      ations and creates the necessary environment vars automatically. Hence in
      explorer windows you will now find the commands "Run", "Edit" and "Print"
      when clicking on a CSS file with the right mouse button.

    o New: Function winIsMinimized() in windows control library


  Version 3.04

    o New: Windows control library

    o New: Math library


  Version 3.03

    o New: Library for asynchronous serial communications

    o New: API's callEx / KCssCallEx

    o New: Octal and hex constants recognized.

    o New: Exponential notation like 1.234e-12 recognized.

    o Fix: Literals containing \0 were truncated.

    o Fix: strChar(0) returned empty string instead of '\0'.

    o Fix: Memory leak in fileOpen


  Version 3.02

    o New: Global var arrays may now be resized

    o New: API's addVar / KCssAddVar

    o New: Support for gcc/g++ (mingw32 and cygwin)


  Version 3.01

    o New: Embed CSS into .BAT or .CMD batch files.

    o Fix: Remove debugger code in Win32 release.


  Version 3.00

    o New: C-API using __stdcall/_System convention with samples for
           IBM VisualAge C++, Microsoft Visual C/C++ and Borland C/C++.

    o New: Library KcRexLib.dll for regular expressions.


  Version 2.00

    o New: switch/case/default.

    o Change: Documentation completely revised.


  Version 1.04

    o First public version
