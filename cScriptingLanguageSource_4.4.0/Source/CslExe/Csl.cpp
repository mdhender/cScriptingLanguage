/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  Csl.cpp
 * Application :  CSL executive
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.25  First implementation                        P.Koch, IBK
 * 2001.07.07  Renaming from css to csl                    P.Koch, IBK
 * 2002.05.26  Release 4.04                                P.Koch, IBK
 *
 * OPEN SOURCE LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to IBK at info@ibk-software.ch.
 */

#include <stdlib.h>
#include <stdio.h>
#include <ZCsl.hpp>
#include <ZTrace.hpp>
#include <ZExcept.hpp>
#ifdef ZC_GNU
  #include <strstream.h>
#else
  #include <strstrea.h>
#endif

int main(int argc, char *argv[])
{
   ZFUNCTRACE_DEVELOP("main(int argc, char *argv[])");
   ZString module("csl.exe");
   int ret(1);

   ZCsl* csl = 0;
   try {
      ZMODTRACE_DEVELOP("try block");

      // check minimal parameters
      if (argc < 2)
         ZTHROWEXC("syntax: csl scriptfile [parameters]");

      // build init string
      ZTRACE_DEVELOP("build init");
      ZString init("const mainArgVals["+ZString(argc)+"] = {\n");
      for (int a = 0; a < argc; a++) {
         init += "  '"+ZString(argv[a]).change("\\","\\\\")+"'";
         if (a < argc-1) init += ",";
         init += '\n';
      } // for
      init += "};\n";
      ZTRACE_DEVELOP(init);
      istrstream str((char*)init);

      // create csl instance
      ZTRACE_DEVELOP("create csl instance");
      csl = new ZCsl();

      // load initialization
      ZTRACE_DEVELOP("load init script");
      csl->loadScript(module, &str);

      // load users script
      ZTRACE_DEVELOP("load user script");
      csl->loadScript(argv[1]);

      // execute main
      ZTRACE_DEVELOP("call main");
      ret = csl->call(module, "main").asInt();

      // close instance
      ZTRACE_DEVELOP("delete csl instance");
      delete csl;
   } // try
   catch (const ZException& exc) {
      ZMODTRACE_DEVELOP("catch block");
      // display on stderr
      for (int i = 0; i < exc.count(); i++)
         cerr << exc[i] << endl;
      if (csl) {
         // log to logfile in case
         ZTRACE_DEVELOP("try to log exception");
         try {
            for (int i = 0; i < exc.count(); i++)
               csl->call(
                  module, "sysLog", 3,
                  exc[i].constBuffer(),  // message
                  "0",                   // raw mode
                  "1");                  // quiet (to logfile only)
         } // try
         catch (const ZException& exc) {
            ZTRACE_DEVELOP("log exception failed");
         } // catch
         // close instance
         ZTRACE_DEVELOP("delete csl instance");
         delete csl;
      } // if
   } // catch
   catch (...) {
      cerr << "Caught unhandled exception in Csl.exe" << endl;
   } // catch
   return ret;
} // main
