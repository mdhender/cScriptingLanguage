Building CSS with Microsoft Visual C++ V5.0
===========================================

Prerequisites
------------

 - Install Visual C++ V5.0

 - If you want to build the Oracle interface, have Oracle installed (at least
   the Oracle OCI package).

   If you do NOT want to build the Oracle interface, edit makefile in this
   directory and comment out the appropriate line as:

   ---before---
     Oracle.bld \
   ---after----
   # Oracle.bld \
   ------------

 - If you want to build the DB2 interface, have DB2 installed (at least the
   developer tools).

   If you do NOT want to build the DB2 interface, edit makefile in this
   directory and comment out the appropriate line as:

   ---before---
     Db2.bld \
   ---after----
   # Db2.bld \
   ------------

 - If you want to build the MySQL interface, have MySQL installed.

   If you do NOT want to build the MySQL interface, edit makefile in this
   directory and comment out the appropriate line as:

   ---before---
     Mysql.bld \
   ---after----
   # Mysql.bld \
   ------------

 - Make sure environment vars PATH, INCLUDE and LIB are set up correct to
   point to the appropriate directories of MSVC, Oracle, DB2 and MySQL.
   You may edit env.bat in this directory for this.


Building CSS
------------

Open a command line in this directory, execute env.bat to set up your
environment, and then enter the make command as:

  nmake
  nmake /a
    Builds all targets.
    Use /a to force rebuild even if targets are up-to-date.

  nmake install
  nmake install /a
    Builds all targets and copies to css kernel and libraries to ..\bin
    Use /a to force rebuild even if targets are up-to-date.

  nmake clean
    Cleans up subdirectories by delete all intermediate files.

                                                         2000.06.09 Peter Koch
