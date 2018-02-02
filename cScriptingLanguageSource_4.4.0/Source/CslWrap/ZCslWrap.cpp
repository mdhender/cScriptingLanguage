/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZCslWrap.cpp
 * Application :  IBK Open Class Library
 * Purpose     :  CSL wraper C++ API for kernel
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.06.01  First implementation                        P.Koch, IBK
 * 2001.07.07  Renaming from css to csl                    P.Koch, IBK
 * 2001.07.28  Enhanced trace facilities                   P.Koch, IBK
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

#define ZC_BUILDING_ZCSLWRAP

#include <ZCslWrap.hpp>
#include <ZExcept.hpp>

ZCsl* ZCsl::iWrapList = 0;

static ZString funcName(const ZString& aFuncHeader)
{
   ZFUNCTRACE_DEVELOP("funcName(const ZString& aFuncHeader)");
   const char *s = aFuncHeader;
   ZString name;
   while (*s == ' ') s++;
   while (*s && *s != '(' && *s != ' ') name += *s++;
   return name;
} // funcName

ZCslWrapFunction::ZCslWrapFunction(
   const ZString& aFileName,
   const ZString& aFuncName,
   ZString (*aFuncAddr)(ZCsl*),
   ZCslWrapFunction* aNext) :
   iFileName(aFileName),
   iFuncName(aFuncName),
   iFuncAddr(aFuncAddr),
   iNext(aNext)
{
   ZFUNCTRACE_DEVELOP("ZCslWrapFunction::ZCslWrapFunction(...)");
} // ZCslWrapFunction

ZCslWrapFunction::~ZCslWrapFunction()
{
   ZFUNCTRACE_DEVELOP("ZCslWrapFunction::~ZCslWrapFunction()");
   if (iNext) delete iNext;
} // ~ZCslWrapFunction

void ZCsl::checkErrors(long aErrCount) const
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::checkErrors(long aErrCount) const");
   if (iErrorCheck && aErrCount) {
      ZException exc;
      for (int i = 0; i < aErrCount; i++) {
         // First we get the size of the message only so a buffer
         // large enough for the string can be allocated:
         long size;
         checkErrors(ZCslGetError(iCslHandle, i, 0, &size));

         // Now a buffer is allocated and the message is retrieved:
         char* msg = new char[size];
         ZCslGetError(iCslHandle, i, msg, &size); // get message
         exc.addAsLast(msg);

         // the buffer is no longer required and gets discarded:
         delete [] msg;
      } // for
      ZTHROW(exc);
   } // if
} // checkErrors

ZExport0 ZCsl::ZCsl(int aFlags) :
   iCslHandle(0),
   iErrorCheck(zTrue),
   iCloseHandle(zTrue),
   iFuncList(0)
{
   ZFUNCTRACE_DEVELOP("ZExport0 ZCslWrap::ZCslWrap(int aFlags)");
   ZCslOpen(&iCslHandle, aFlags);

   // add self to list
   iNext = iWrapList;
   iWrapList = this;
} // ZCsl

ZExport0 ZCsl::ZCsl(ZCslHandle aHandle) :
   iCslHandle(aHandle),
   iErrorCheck(zTrue),
   iCloseHandle(zFalse),
   iFuncList(0)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::ZCslWrap(ZCslHandle aHandle)");
   // add self to list
   iNext = iWrapList;
   iWrapList = this;
} // ZCsl

ZExport0 ZCsl::~ZCsl()
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::~ZCslWrap()");
   if (iFuncList) delete iFuncList;
   if (iCloseHandle) ZCslClose(iCslHandle);

   // remove self from list
   ZCsl* wrap = iWrapList;
   ZCsl* prev = 0;
   while (wrap) {
      if (wrap == this) {
         if (wrap == iWrapList)
            iWrapList = iNext;
         else
            prev->iNext = iNext;
         break;
      } // if
      wrap = wrap->iNext;
   } // while
} // ~ZCsl

ZExport(ZCsl&) ZCsl::set(const ZString& aVarName, const ZString& aValue)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::set(const ZString& aVarName, const ZString& aValue)");
   checkErrors(ZCslSet(iCslHandle, aVarName, aValue, aValue.size()));
   return *this;
} // set

ZExport(ZString) ZCsl::get(const ZString& aVarName)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::get(const ZString& aVarName)");
   // get size of data
   long size;
   checkErrors(ZCslGet(iCslHandle, aVarName, 0, &size));

   // create string with this size-1 (terminating zero not counted in ZString's)
   ZString str(0, size-1);

   // now read data
   ZCslGet(iCslHandle, aVarName, str, &size);
   return str;
} // get

ZExport(long) ZCsl::varSizeof(const ZString& aVarName)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::varSizeof(const ZString& aVarName)");
   long size;
   checkErrors(ZCslVarSizeof(iCslHandle, aVarName, &size));
   return size;
} // varSizeOf

ZExport(ZCsl&) ZCsl::varResize(const ZString& aVarName)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::varResize(const ZString& aVarName)");
   checkErrors(ZCslVarResize(iCslHandle, aVarName));
   return *this;
} // varResize

ZExport(ZCsl&) ZCsl::loadScript(const ZString& aFileName, istream* aStr)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::loadScript(const ZString& aFileName, istream* aStr)");
   ZString mem;
   char buf[256];
   int cnt;
   for (;;) {
      aStr->read(buf, sizeof(buf));
      cnt = aStr->gcount();
      if (cnt > 0) mem += ZString(buf, cnt);
      if (cnt < (int)sizeof(buf)) break;
   } // for
   ZTRACE_DEVELOP("iCslHandle = "+ZString::hex((unsigned long)iCslHandle));
   ZTRACE_DEVELOP("aFileName = "+aFileName);
   ZTRACE_DEVELOP("mem = "+mem);
   checkErrors(ZCslLoadScriptMem(iCslHandle,aFileName,mem.constBuffer()));
   return *this;
} // loadScript

ZExport(ZCsl&) ZCsl::loadScript(const ZString& aFileName)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::loadScript(const ZString& aFileName)");
   checkErrors(ZCslLoadScriptFile(iCslHandle, aFileName));
   return *this;
} // loadScript

ZExport(ZCsl&) ZCsl::loadLibrary(const ZString& aDllName)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::loadLibrary(const ZString& aDllName)");
   checkErrors(ZCslLoadLibrary(iCslHandle, aDllName));
   return *this;
} // loadLibrary

ZExport(ZString) ZCsl::call(
   const ZString& aFileName,
   const ZString& aFuncName,
   long aArgCount,
   char** aArgs,
   long* aSize)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::call(...)");
   checkErrors(ZCslCallEx(iCslHandle, aFileName, aFuncName, aArgCount, aArgs, aSize));

   // ret size of result
   long size;
   checkErrors(ZCslGetResult(iCslHandle, 0, &size));

   // create string with this size-1 (terminating zero not counted in ZString's)
   ZString str(0, size-1);

   // now read data
   ZCslGetResult(iCslHandle, str, &size);
   return str;
} // call

ZExport(ZString) ZCsl::call(
   const ZString& aFileName,
   const ZString& aFuncName,
   int aArgCount, ...)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::call(...)");
   char** args = 0;

   if (aArgCount) {
      args = new char*[aArgCount];
      va_list argPtr;
      va_start(argPtr, aArgCount);
      for (int argc = 0; argc < aArgCount; argc++)
         args[argc] = va_arg(argPtr, char*);
      va_end(argPtr);
   } // if
   return call(aFileName, aFuncName, (long)aArgCount, args);
} // call

ZExport(ZString) ZCsl::callEx(
   const ZString& aFileName,
   const ZString& aFuncName,
   int aArgCount, ...)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::callEx(...)");
   char** args = 0;
   long* size = 0;

   if (aArgCount) {
      args = new char*[aArgCount];
      size = new long[aArgCount];
      va_list argPtr;
      va_start(argPtr, aArgCount);
      for (int argc = 0; argc < aArgCount; argc++) {
         args[argc] = va_arg(argPtr, char*);
         size[argc] = va_arg(argPtr, long);
      } // for
      va_end(argPtr);
   } // if
   return call(aFileName, aFuncName, (long)aArgCount, args, size);
} // callEx

ZCslWrapFunction* ZCsl::findFunction(const ZString& aFileName, const ZString& aFuncName)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::findFunction(...)");
   ZCslWrapFunction *func = iFuncList;
   while (func) {
      if (func->fileName() == aFileName &&
          func->funcName() == aFuncName)
         return func;
      func = func->next();
   } // while
   return 0;
} // findFunction

ZCsl* ZCsl::findWrapObject(ZCslHandle aHandle)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::findWrapObject(ZCslHandle aHandle)");
   ZCsl* wrap = iWrapList;
   while (wrap) {
      if (wrap->iCslHandle == aHandle) return wrap;
      wrap = wrap->iNext;
   } // while
   return 0;
} // findWrapObject

ZExport(ZCsl*) ZCsl::createWrapObject(ZCslHandle aHandle)
{
   ZFUNCTRACE_DEVELOP("ZCsl::createWrapObject(ZCslHandle aHandle)");
   ZTRACE_DEVELOP("aHandle = "+ZString::hex((unsigned long)aHandle));
   ZCsl* wrap = findWrapObject(aHandle);
   if (!wrap)
      wrap = new ZCsl(aHandle);
   ZTRACE_DEVELOP("wrap = "+ZString::hex((unsigned long)wrap));
   return wrap;
} // createWrapObject

ZExportAPI(void) zCslWrapFunction(ZCslHandle aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslFunction(zCslWrapFunction)");
   try {
      long size;

      // get filename
      ZCslGet(aCsl, "cslFileName", 0, &size);
      ZString fileName(0, size-1);
      ZCslGet(aCsl, "cslFileName", fileName, &size);

      // get function name
      ZCslGet(aCsl, "cslFunction", 0, &size);
      ZString funcName(0, size-1);
      ZCslGet(aCsl, "cslFunction", funcName, &size);

      // find wrapper object
      ZCsl* wrap = ZCsl::findWrapObject(aCsl);

      // find function
      ZCslWrapFunction* func = 0;
      if (wrap)
         func = wrap->findFunction(fileName, funcName);

      // check for success
      if (!func)
         ZTHROWEXC("Function "
                   +fileName+"::"+funcName
                   +" not found in C++ wrappers!");

      // execute the call
      ZString result;
      wrap->enableErrorCheck(zFalse);
      try {
         ZString (*function)(ZCsl*);
         function = (ZString (*)(ZCsl*))func->funcAddr();
         result = (*function)(wrap);
      } // try
      catch (const ZException& exc) {
         wrap->enableErrorCheck();
         throw;
      } // catch

      // save return value
      ZCslSetResult(aCsl, result.constBuffer(), result.size());
   } // try
   catch (const ZException& exc) {
      for (int i = 0; i < exc.count(); i++)
         ZCslSetError(aCsl, exc[i].constBuffer(), exc[i].size());
   } // catch
} // zCslWrapFunction

ZExport(ZCsl&) ZCsl::addFunc(
   const ZString& aFileName,
   const ZString& aFuncHeader,
   ZString (*aFuncAddr)(ZCsl*))
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::addFunc(...)");
   checkErrors(ZCslAddFunc(iCslHandle, aFileName, aFuncHeader, zCslWrapFunction));
   iFuncList =
      new ZCslWrapFunction(
             aFileName,
             funcName(aFuncHeader),
             aFuncAddr,
             iFuncList
          );
   return *this;
} // addFunc

ZExport(ZCsl&) ZCsl::addVar(
   const ZString& aVarName,
   const ZString& aInitValue,
   ZBoolean aIsConst)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::addVar(...)");
   checkErrors(ZCslAddVar(iCslHandle, aVarName, aInitValue, aIsConst));
   return *this;
} // addVar

ZExport(ZDateTime) ZCsl::startDateTime() const
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::startDateTime() const");
   long year, month, day, hour, minute, second;
   checkErrors(
      ZCslStartDateTime(
         iCslHandle, &year, &month, &day,
                     &hour, &minute, &second)
   );
   return ZDateTime(year, month, day, hour, minute, second);
} // startDateTime

ZExport(ZCsl::TraceMode) ZCsl::traceMode() const
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::traceMode() const");
   long mode;
   checkErrors(ZCslTraceMode(iCslHandle, &mode));
   return (TraceMode)mode;
} // traceMode

ZExport(ZCsl&) ZCsl::setTraceMode(TraceMode aMode)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::setTraceMode(TraceMode aMode)");
   checkErrors(ZCslSetTraceMode(iCslHandle, aMode));
   return *this;
} // setTraceMode

ZExport(ZCsl&) ZCsl::trace(const ZString& aMessage)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::trace(const ZString& aMessage)");
   checkErrors(ZCslTrace(iCslHandle, aMessage));
   return *this;
} // trace

ZExport(ZCsl&) ZCsl::show(ShowMode aMode, long aDepth)
{
   ZFUNCTRACE_DEVELOP("ZCslWrap::show(ShowMode aMode, long aDepth)");
   checkErrors(ZCslShow(iCslHandle, aMode, aDepth));
   return *this;
} // show
