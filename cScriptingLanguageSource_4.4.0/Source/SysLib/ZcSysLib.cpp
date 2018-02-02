/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZcSysLib.cpp
 * Application :  CSL system library
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.23  First implementation                        P.Koch, IBK
 * 2001.07.07  Renaming from css to csl                    P.Koch, IBK
 *             Added sysIsBSD, sysIsAIX, sysIsSolaris,
 *             sysIsUnix and sysIsUnixFamily
 * 2001.07.28  Enhanced trace facilities                   P.Koch, IBK
 * 2001.10.27  Add sysIsFreeBSD and sysIsNetBSD            P.Koch, IBK
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

#include <errno.h>
#include <stdlib.h>
#include <fstream.h>
#include <ZExcept.hpp>
#include <ZProcess.hpp>
#include <ZPlatfrm.hpp>
#include <ZTrace.hpp>

#if ZC_UNIXFAM
  #include <unistd.h>
#else
  #include <direct.h>
#endif

#ifdef ZC_NATIVECSLLIB
  #include <ZCsl.hpp>
#else
  #include <ZCslWrap.hpp>
#endif
#ifdef ZC_GNU
  #include <strstream.h>
#else
  #include <strstrea.h>
#endif

static ostream*   iLogStr   = 0;     // logfile stream
static int        iLogLevel = 0;     // log level
static int        iUseCount = 0;     // library use count

static ZString sysElapsed(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysElapsed(ZCsl* aCsl)");
   return ZDateTime()-aCsl->startDateTime();
} // sysElapsed

static ZString sysTrace(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysTrace(ZCsl* aCsl)");
   if (aCsl->get("argCount").asInt()) {
      int mode = aCsl->get("mode").asInt();
      if (mode < ZCsl::traceNone || mode > ZCsl::traceAll)
         mode = ZCsl::traceNone;
      aCsl->setTraceMode((ZCsl::TraceMode)mode);
   } // if
   return (int)aCsl->traceMode();
} // sysTrace

static ZString sysDateFormat(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysDateFormat(ZCsl* aCsl)");
   if (aCsl->get("argCount").asInt())
      ZDateTime::setFormat(aCsl->get("format").asInt());
   return ZDateTime::format();
} // sysDateFormat

static ZString sysDate(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysDate(ZCsl* aCsl)");
   ZDateTime t;
   return t.asDate();
} // sysDate

static ZString sysTime(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysTime(ZCsl* aCsl)");
   ZDateTime t;
   return t.asTime();
} // sysTime

static ZString sysDateTime(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysDateTime(ZCsl* aCsl)");
   ZDateTime t;
   ZTRACE_DEVELOP(t.asDate()+" "+t.asTime());
   return t.asDate()+" "+t.asTime();
} // sysDate

static ZString sysTimestamp(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysTimestamp(ZCsl* aCsl)");
   ZDateTime t;
   return t.asIsoDate()+"."+t.asIsoTime();
} // sysTimestamp

static ZString sysStartDate(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysStartDate(ZCsl* aCsl)");
   return aCsl->startDateTime().asDate();
} // sysStartDate

static ZString sysStartTime(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysStartTime(ZCsl* aCsl)");
   return aCsl->startDateTime().asTime();
} // sysStartTime

static ZString sysStartDateTime(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysStartDateTime(ZCsl* aCsl)");
   return aCsl->startDateTime().asDate()+" "+aCsl->startDateTime().asTime();
} // sysStartDateTime

static ZString sysStartTimestamp(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysStartTimestamp(ZCsl* aCsl)");
   return aCsl->startDateTime().asIsoDate()+"."+aCsl->startDateTime().asIsoTime();
} // sysStartTimestamp

static ZString sysSecondsSince(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysSecondsSince(ZCsl* aCsl)");
   int argc(aCsl->get("argCount").asInt());
   ZString ts1(aCsl->get("timestamp1"));
   ZString ts2;
   if (argc == 2)
      ts2 = aCsl->get("timestamp2");
   else {
      ZDateTime t;
      ts2 = t.asIsoDate()+"."+t.asIsoTime();
   } // if
   ZDateTime t1, t2;
   t1
      .setYear  (ts1.subString(1,4).asInt())
      .setMonth (ts1.subString(6,2).asInt())
      .setDay   (ts1.subString(9,2).asInt())
      .setHour  (ts1.subString(12,2).asInt())
      .setMinute(ts1.subString(15,2).asInt())
      .setSecond(ts1.subString(18,2).asInt());
   t2
      .setYear  (ts2.subString(1,4).asInt())
      .setMonth (ts2.subString(6,2).asInt())
      .setDay   (ts2.subString(9,2).asInt())
      .setHour  (ts2.subString(12,2).asInt())
      .setMinute(ts2.subString(15,2).asInt())
      .setSecond(ts2.subString(18,2).asInt());
   return t2-t1;
} // sysSecondsSince

static ZString sysSleep(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysSleep(ZCsl* aCsl)");
   ZCurrentThread::sleep(aCsl->get("millisecs").asLong());
   return 1;
} // sysSleep

static void closeLogFile()
{
   ZFUNCTRACE_DEVELOP("closeLogFile()");
   if (iLogStr) {
      ZDateTime now;
      *iLogStr
         << "Log End  : " << now.asDate() << " "  << now.asTime()
         << endl << endl;
      delete iLogStr;
      iLogStr = 0;
   } // if
} // closeLogFile

static ZString sysLog(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysLog(ZCsl* aCsl)");
   ZString msg;
   int raw(0), quiet(0);

   int argCount = aCsl->get("argCount").asInt();
   if (argCount >= 1) msg = aCsl->get("message");
   if (argCount >= 2) raw = aCsl->get("raw").asInt() ? 1 : 0;
   if (argCount >= 3) quiet = aCsl->get("quiet").asInt() ? 1 : 0;
   ZTRACE_DEVELOP("message = <"+msg+">");
   ZTRACE_DEVELOP("raw = <"+ZString(raw)+">");
   ZTRACE_DEVELOP("quiet = <"+ZString(quiet)+">");
   short l;
   int level(iLogLevel);
   if (level > 36) level = 36;
   if (!raw)
      for (l = 0; l < level; l++) cout << "  ";
   if (!quiet) cout << msg << endl;
   if (iLogStr) {
      if (!raw) {
         *iLogStr << ZDateTime().asTime() << " ";
         for (l = 0; l < level; l++) *iLogStr << "  ";
      } // if
      *iLogStr << msg << endl;
   } // if
   return 1;
} // sysLog

static ZString sysPrompt(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysPrompt(ZCsl* aCsl)");
   ZString msg;
   int raw(0);

   int argCount = aCsl->get("argCount").asInt();
   if (argCount >= 1) msg = aCsl->get("message");
   if (argCount >= 2) raw = aCsl->get("raw").asInt() ? 1 : 0;
   short l;
   int level(iLogLevel);
   if (level > 36) level = 36;
   if (!raw)
      for (l = 0; l < level; l++) cout << "  ";
   cout << msg;
   if (iLogStr) {
      if (!raw) {
         *iLogStr << ZDateTime().asTime() << " ";
         for (l = 0; l < level; l++) *iLogStr << "  ";
      } // if
      *iLogStr << msg;
   } // if
   msg = ZString::lineFrom(cin);
   if (iLogStr) *iLogStr << msg << endl;
   return msg;
} // sysPrompt

static ZString sysLogLevel(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysLogLevel(ZCsl* aCsl)");
   if (aCsl->get("argCount").asInt()) {
      iLogLevel += aCsl->get("level").asInt();
      if (iLogLevel < 0) iLogLevel = 0;
   } // if
   return iLogLevel;
} // sysLogLevel

static ZString sysLogFile(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysLogFile(ZCsl* aCsl)");
   closeLogFile();
   if (aCsl->get("argCount").asInt()) {
      ZString lname(aCsl->get("filename"));
      int add(0);
      long dotPos = lname.lastIndexOf('.');
      if (!dotPos)
         add = 1;
      else {
         long slashPos = lname.lastIndexOf(ZC_PATHSEPARATOR);
         if (slashPos > dotPos) add = 1;
      } // if
      if (add) lname += ".log";
      iLogStr = new ofstream(lname.constBuffer(),ios::out|ios::app);
      if (!*iLogStr) {
         iLogStr = 0;
         ZTHROWEXC("logfile "+lname+" cannot be opened");
      } // if
      ZDateTime now;
      *iLogStr << "Log Start: " << now.asDate() << " " << now.asTime() << endl;
   } // if
   return 1;
} // sysLogFile

static ZString sysLoadScript(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysLoadScript(ZCsl* aCsl)");
   aCsl->loadScript(aCsl->get("filename"));
   return 1;
} // sysLoadScript

static ZString sysLoadLibrary(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysLoadLibrary(ZCsl* aCsl)");
   aCsl->loadLibrary(aCsl->get("dllname"));
   return 1;
} // sysLoadLibrary

static ZString sysShow(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysShow(ZCsl* aCsl)");
   ZCsl::ShowMode mode((ZCsl::ShowMode)(aCsl->get("mode").asInt()));
   long depth(99999999);
   if (aCsl->get("argCount").asInt()>1) depth = aCsl->get("depth").asInt();
   aCsl->show(mode, depth);
   return 1;
} // sysShow

static ZString sysEnvVar(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysEnvVar(ZCsl* aCsl)");
   char* var = getenv(aCsl->get("varname").constBuffer());
   if (var)
      return var;
   else
      return ZString();
} // sysEnvVar

static ZString sysDirectory(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysDirectory(ZCsl* aCsl)");
   int argc(aCsl->get("argCount").asInt());
   if (argc) {
      // change current drive/directory
      ZString path(aCsl->get("path"));
#if ZC_WIN || ZC_OS2
      if (path[2]==':') {
         char a(path[1] > 'Z' ? 'a' : 'A');
         int drive();
         if (_chdrive(path[1]-a+1))
            ZTHROWEXC("sysDirectory: invalid drive ("+path+")");
      } // if
#endif
#ifdef ZC_GNU
      if (chdir(path.constBuffer()))
#else
      if (chdir(path))
#endif
         ZTHROWEXC("sysDirectory: invalid path ("+path+")");
   } // if
   // query current directory
   char buf[_MAX_PATH];
   if (!getcwd(buf, _MAX_PATH))
      ZTHROWEXC("sysDirectory: error getting current directory");
   return buf;
} // sysDirectory

static ZString sysCommand(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("sysCommand(ZCsl* aCsl)");
   int ret = system(aCsl->get("cmd").constBuffer());
   if (ret < 0) {
#if ZC_IBM
      switch (errno) {
         case ENOCMD: ZTHROWEXC("sysCommand: no command processor found");
         case ENOMEM: ZTHROWEXC("sysCommand: out of memory");
         default    : ZTHROWEXC("sysCommand: system error ("+ZString(_doserrno)+")");
      } // switch
#elif ZC_UNIXFAM
      ZTHROWEXC("sysCommand: system error");
#else
      ZTHROWEXC("sysCommand: system error ("+ZString(_doserrno)+")");
#endif
   } // if
   return ret;
} // sysCommand

ZCslInitLib(aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslInitLib(aCsl)");
   ZTRACE_DEVELOP("aCsl = "+ZString::hex(aCsl));
   iUseCount++;

   ZString iFile("ZcSysLib");
   ZString init(ZString(
      "const sysVersion = '4.04';\n"
      "const sysCompiler = '")+ZC_COMPILER+"';\n"
      "const sysLibtype = '"+ZC_CSLLIBTYPE+"';\n"
      "const sysBuilt = '"+ZString(__DATE__)+" "+__TIME__+"';\n"

      "const sysIsOS2 = "+ZString(ZPlatform::isOS2())+";\n"
      "const sysIsLinux = "+ZString(ZPlatform::isLinux())+";\n"
      "const sysIsBSD = "+ZString(ZPlatform::isBSD())+";\n"
      "const sysIsFreeBSD = "+ZString(ZPlatform::isFreeBSD())+";\n"
      "const sysIsNetBSD = "+ZString(ZPlatform::isNetBSD())+";\n"
      "const sysIsAIX = "+ZString(ZPlatform::isAIX())+";\n"
      "const sysIsSolaris = "+ZString(ZPlatform::isSolaris())+";\n"
      "const sysIsUnix = "+ZString(ZPlatform::isUnix())+";\n"
      "const sysIsUnixFamily = "+ZString(ZPlatform::isUnixFamily())+";\n"
      "const sysIsWindows = "+ZString(ZPlatform::isWindows())+";\n"
      "const sysIsWin95Family = "+ZString(ZPlatform::isWin95Family())+";\n"
      "const sysIsWinNTFamily = "+ZString(ZPlatform::isWinNTFamily())+";\n"
      "const sysIsWinServer = "+ZString(ZPlatform::isWinServer())+";\n"
      "const sysIsWin95 = "+ZString(ZPlatform::isWin95())+";\n"
      "const sysIsWin98 = "+ZString(ZPlatform::isWin98())+";\n"
      "const sysIsWinME = "+ZString(ZPlatform::isWinME())+";\n"
      "const sysIsWinNT3 = "+ZString(ZPlatform::isWinNT3())+";\n"
      "const sysIsWinNT4 = "+ZString(ZPlatform::isWinNT4())+";\n"
      "const sysIsWin2000 = "+ZString(ZPlatform::isWin2000())+";\n"
      "const sysIsWinXP = "+ZString(ZPlatform::isWinXP())+";\n"

      "const sysDateFormatISO = "+ZString((int)ZDateTime::iso)+";\n"
      "const sysDateFormatEuro = "+ZString((int)ZDateTime::euro)+";\n"
      "const sysDateFormatUS = "+ZString((int)ZDateTime::us)+";\n"

      "const sysTraceNone = "+ZString((int)ZCsl::traceNone)+";\n"
      "const sysTraceCode = "+ZString((int)ZCsl::traceCode)+";\n"
      "const sysTraceFuncs = "+ZString((int)ZCsl::traceFuncs)+";\n"
      "const sysTraceBlks = "+ZString((int)ZCsl::traceBlks)+";\n"
      "const sysTraceMsgs = "+ZString((int)ZCsl::traceMsgs)+";\n"
      "const sysTraceInfo = "+ZString((int)ZCsl::traceInfo)+";\n"
      "const sysTraceAll = "+ZString((int)ZCsl::traceAll)+";\n"

      "const sysShowFunctions = "+ZString((int)ZCsl::showFunctions)+";\n"
      "const sysShowCallStack = "+ZString((int)ZCsl::showCallStack)+";\n"
      "const sysShowFullStack = "+ZString((int)ZCsl::showFullStack)+";\n"
      "const sysShowGlobals = "+ZString((int)ZCsl::showGlobals)+";\n"
      "const sysShowLibraries = "+ZString((int)ZCsl::showLibraries)+";\n"
   );
   istrstream str((char*)init);
   aCsl->loadScript(iFile, &str);
   (*aCsl)
      .addFunc(iFile,
         "sysCommand(const cmd)",
          sysCommand)

      .addFunc(iFile,
         "sysDate()",
          sysDate)

      .addFunc(iFile,
         "sysDateFormat([const format])",
          sysDateFormat)

      .addFunc(iFile,
         "sysDateTime()",
          sysDateTime)

      .addFunc(iFile,
         "sysDirectory([const path])",
          sysDirectory)

      .addFunc(iFile,
         "sysElapsed()",
          sysElapsed)

      .addFunc(iFile,
         "sysEnvVar(const varname)",
          sysEnvVar)

      .addFunc(iFile,
         "sysLoadScript(const filename)",
          sysLoadScript)

      .addFunc(iFile,
         "sysLoadLibrary(const dllname)",
          sysLoadLibrary)

      .addFunc(iFile,
         "sysLog([const message, const raw, const quiet])",
          sysLog)

      .addFunc(iFile,
         "sysLogFile([const filename])",
          sysLogFile)

      .addFunc(iFile,
         "sysLogLevel([const level])",
          sysLogLevel)

      .addFunc(iFile,
         "sysPrompt([const message, const raw])",
          sysPrompt)

      .addFunc(iFile,
         "sysSleep(const millisecs)",
          sysSleep)

      .addFunc(iFile,
         "sysShow(const mode, [const depth])",
          sysShow)

      .addFunc(iFile,
         "sysSecondsSince(const timestamp1, [const timestamp2])",
          sysSecondsSince)

      .addFunc(iFile,
         "sysStartDate()",
          sysStartDate)

      .addFunc(iFile,
         "sysStartDateTime()",
          sysStartDateTime)

      .addFunc(iFile,
         "sysStartTime()",
          sysStartTime)

      .addFunc(iFile,
         "sysStartTimestamp()",
          sysStartTimestamp)

      .addFunc(iFile,
         "sysTime()",
          sysTime)

      .addFunc(iFile,
         "sysTimestamp()",
          sysTimestamp)

      .addFunc(iFile,
         "sysTrace([const mode])",
          sysTrace);
} // ZCslInitLib

ZCslCleanupLib(aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslCleanupLib(aCsl)");
   if (--iUseCount == 0)
      closeLogFile();
} // ZCslCleanupLib
