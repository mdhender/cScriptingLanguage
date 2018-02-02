/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZTrace.hpp
 * Application :  IBK Open Class Library
 * Purpose     :  Program flow tracing
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.27  First implementation                        P.Koch, IBK
 * 2002.05.26  Release 4.4.0                               P.Koch, IBK
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

#ifndef _ZTRACE_
#define _ZTRACE_

#include <ZString.hpp>

class ZTrace : public ZBase
{
   public:
      ZBaseLink0
         ZTrace(
            const ZString& aFile,
            long aLine,
            const ZString& aName);

      ZBaseLink0 ~ZTrace();

      static ZBaseLink(void)
         writeMsg(
            const ZString& aFile,
            long aLine,
            const ZString& aMessage);

   private:
      enum Destination {
         none,
         stdError,
         stdOutput,
         toFile
      };

      static void
         write(
            const ZString& aFile,
            long aLine,
            const ZString& aMessage);

      static ZBoolean iInitialized;
      static ZBoolean iPrintFile;
      static ZBoolean iPrintLine;
      static Destination iDestination;
      static long iFuncLevel;
      static ZString iOutFile;

      ZString iName;
      ZString iFile;
      long iLine;
}; // ZTrace

#define ZFUNCTRACE(function) \
   ZTrace ztrace_function(__FILE__,__LINE__,function)

#define ZMODTRACE(module) \
   ZTrace ztrace_module(__FILE__,__LINE__,module)

#define ZTRACE(message) \
   ZTrace::writeMsg(__FILE__,__LINE__,message)

#ifdef ZC_TRACE_ALL
   #define ZFUNCTRACE_ALL(function) ZFUNCTRACE(function)
   #define ZMODTRACE_ALL(module)    ZMODTRACE(module)
   #define ZTRACE_ALL(message)      ZTRACE(message)
   #ifndef ZC_TRACE_DEVELOP
     #define ZC_TRACE_DEVELOP
   #endif
#else
   #define ZFUNCTRACE_ALL(function)
   #define ZMODTRACE_ALL(modname)
   #define ZTRACE_ALL(message)
#endif

#ifdef ZC_TRACE_DEVELOP
   #define ZFUNCTRACE_DEVELOP(function) ZFUNCTRACE(function)
   #define ZMODTRACE_DEVELOP(module)    ZMODTRACE(module)
   #define ZTRACE_DEVELOP(message)      ZTRACE(message)
   #ifndef ZC_TRACE_RUNTIME
     #define ZC_TRACE_RUNTIME
   #endif
#else
   #define ZFUNCTRACE_DEVELOP(function)
   #define ZMODTRACE_DEVELOP(module)
   #define ZTRACE_DEVELOP(message)
#endif

#ifdef ZC_TRACE_RUNTIME
   #define ZFUNCTRACE_RUNTIME(function) ZFUNCTRACE(function)
   #define ZMODTRACE_RUNTIME(module)    ZMODTRACE(module)
   #define ZTRACE_RUNTIME(message)      ZTRACE(message)
#else
   #define ZFUNCTRACE_RUNTIME(function)
   #define ZMODTRACE_RUNTIME(module)
   #define ZTRACE_RUNTIME(message)
#endif

#endif // _ZTRACE_
