/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZCslApi.cpp
 * Application :  IBK Open Class Library
 * Purpose     :  CSL C API
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.25  First implementation                        P.Koch, IBK
 * 2001.07.07  Renaming from css to csl                    P.Koch, IBK
 * 2001.07.28  Enhanced trace facilities                   P.Koch, IBK
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

#define ZC_BUILDING_ZCSLAPI

#include <string.h>
#include <ZCslApi.h>
#undef ZCslInitLib
#undef ZCslCleanupLib
#undef ZC_CSLLIBTYPE
#include <ZCsl.hpp>
#ifdef ZC_GNU
  #include <strstream.h>
#else
  #include <strstrea.h>
#endif

class ZCslApiFunction {
   public:
      ZCsl* iCsl;
      ZString iFileName;
      ZString iFuncName;
      void (ZFuncptrAPI iFunc)(ZCslHandle aHandle);
      ZCslApiFunction* iNext;
}; // ZCslApiFunction

static ZException iErrs;
static ZCslApiFunction* iFunc = 0;

static ZString funcName(const ZString& aFuncHeader)
{
   const char *s = aFuncHeader;
   ZString name;
   while (*s == ' ') s++;
   while (*s && *s != '(' && *s != ' ') name += *s++;
   return name;
} // funcName

static ZString pseudoFunction(ZCsl* aCsl)
{
   ZString fileName(aCsl->get("cslFileName"));
   ZString funcName(aCsl->get("cslFunction"));
   ZCslApiFunction *func = iFunc;
   ZBoolean found(zFalse);
   while (!found && func) {
      if (func->iCsl == aCsl &&
          func->iFileName == fileName &&
          func->iFuncName == funcName)
         found = zTrue;
      else
         func = func->iNext;
   } // while
   if (!found)
      ZTHROWEXC("Function "
                +fileName+"::"+funcName
                +" not found C wrapper!");
   ZException& errs = aCsl->errs();
   errs.drop();
   aCsl->result() = "";
   (*func->iFunc)(aCsl);
   if (errs.count()) {
      ZException exc;
      for (int i = 0; i < errs.count(); i++)
         exc.addAsLast(errs[i]);
      ZTHROW(exc);
   } // if
   return aCsl->result();
} // pseudoFunction

static long handleCatch(ZCsl* aCsl, const ZException& exc)
{
   ZException* errs = aCsl ? &aCsl->errs() : &iErrs;
   errs->drop();
   for (int i = 0; i < exc.count(); i++)
      errs->addAsLast(exc[i]);
   return errs->count();
} // handleCatch

static void fillBuffer(const ZString& aText, char *aBuffer, long *aSize)
{
   if (aBuffer) {
      // return message
      if (aSize) {
         long n(*aSize-1);
         const char *s = aText;
         char *d = aBuffer;
         while (n && *s) {
            *d++ = *s++;
            n--;
         } // while
         *d = 0;
      } else
         strcpy(aBuffer, aText);
   } // if
   if (aSize) {
      // return length
      *aSize = aText.length()+1;
   } // if
} // fillBuffer

static void clearStatus(ZCsl* aCsl)
{
   iErrs.drop();
   aCsl->errs().drop();
   aCsl->result() = "";
} // clearStatus

ZExportAPI(long) ZCslOpen(               /* Create CSL context & alloc mem */
   ZCslHandle *aHandle,                     /* address of handle var */
   long aFlags)                             /* initialization flags */
{
   try {
      iErrs.drop();
      *aHandle = new ZCsl(aFlags);
   } // try
   catch (const ZException& exc) {
      *aHandle = 0;
      return handleCatch(0, exc);
   } // catch
   return 0;
} // ZCslOpen

ZExportAPI(long) ZCslClose(              /* Destroy CSL Context & release mem */
   ZCslHandle aHandle)                      /* CSL handle */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      ZCslApiFunction *func = iFunc;
      ZCslApiFunction *prev = 0;
      while (func) {
         if (func->iCsl == aCsl) {
            if (func == iFunc) {
               iFunc = func->iNext;
               delete func;
               func = iFunc;
            } else {
               prev->iNext = func->iNext;
               delete func;
               func = prev->iNext;
            } // if
         } else {
            prev = func;
            func = func->iNext;
         } // if
      } // while
      delete aCsl;
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslClose

ZExportAPI(long) ZCslSet(                /* set var or const value */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aVarName,                    /* variable name */
   const char *aValue,                      /* new value (NULL = "") */
   long aSize)                              /* value size, -1 = ASCIZ */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      if (aValue) {
         if (aSize < 0)
            aCsl->set(aVarName, aValue);
         else
            aCsl->set(aVarName, ZString(aValue,aSize));
      } else
         aCsl->set(aVarName);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslSet

ZExportAPI(long) ZCslGet(                /* get var or const value */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aVarName,                    /* variable name */
   char *aBuffer,                           /* buffer for value (NULL = query size) */
   long *aSize)                             /* buffer size */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      fillBuffer(aCsl->get(aVarName), aBuffer, aSize);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslGet

ZExportAPI(long) ZCslVarSizeof(          /* get size of variable */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aVarName,                    /* name of variable */
   long *aLength)                           /* buffer for length */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      *aLength = aCsl->varSizeof(aVarName);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslVarSizeof

ZExportAPI(long) ZCslVarResize(          /* resize variable */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aVarName)                    /* name and new layout of variable */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      aCsl->varResize(aVarName);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslVarResize

ZExportAPI(long) ZCslLoadScriptMem(      /* load script from memory */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aFileName,                   /* file/module name for script */
   const char *aStr)                        /* Csl source code */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
#ifdef ZC_MICROSOFT
      // msvc patch: no istrstream constructor from const char*
      ZString iStr(aStr);
      istrstream str(iStr);
#else
      istrstream str(aStr);
#endif
      aCsl->loadScript(aFileName, &str);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslLoadScriptMem

ZExportAPI(long) ZCslLoadScriptFile(     /* load script from file */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aFileName)                   /* file name to load script */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      aCsl->loadScript(aFileName);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslLoadScriptFile

ZExportAPI(long) ZCslLoadLibrary(        /* load a CSL dll */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aDllName)                    /* dll filename */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      aCsl->loadLibrary(aDllName);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslLoadLibrary

ZExportAPI(long) ZCslCall(               /* call function */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aFileName,                   /* file/module caller belongs to */
   const char *aFuncName,                   /* function name */
   long aArgCount,                          /* # of arguments following */
   char *aParam[])                          /* parameter list */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      aCsl->result() = aCsl->call(aFileName, aFuncName, aArgCount, aParam);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslCall

ZExportAPI(long) ZCslCallEx(             /* call function */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aFileName,                   /* file/module caller belongs to */
   const char *aFuncName,                   /* function name */
   long aArgCount,                          /* # of arguments following */
   char *aParam[],                          /* parameter list */
   long aSize[])                            /* parameter sizes */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      aCsl->result() = aCsl->call(aFileName, aFuncName, aArgCount, aParam, aSize);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslCallEx

ZExportAPI(long) ZCslGetResult(          /* get function result */
   ZCslHandle aHandle,                      /* CSL handle */
   char *aBuffer,                           /* buffer for value (NULL = query size) */
   long *aSize)                             /* buffer size */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      fillBuffer(aCsl->result(), aBuffer, aSize);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslGetResult

ZExportAPI(long) ZCslAddFunc(            /* add a function */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aFileName,                   /* file/module name for function */
   const char *aFuncHeader,                 /* function header */
   void (ZFuncptrAPI aFunc)(ZCslHandle aHandle))/* function address */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      ZCslApiFunction* func = new ZCslApiFunction;
      func->iCsl = aCsl;
      func->iFileName = aFileName;
      func->iFuncName = funcName(aFuncHeader);
      func->iFunc = aFunc;
      func->iNext = iFunc;
      iFunc = func;
      aCsl->addFunc(aFileName, aFuncHeader, pseudoFunction);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslAddFunc

ZExportAPI(long) ZCslAddVar(             /* add a local var/const */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aVarName,                    /* name and layout of var/const */
   const char *aInitVal,                    /* initial value */
   long aIsConst)                           /* 0=var, 1=const */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      if (aInitVal)
         aCsl->addVar(aVarName, aInitVal, aIsConst);
      else
         aCsl->addVar(aVarName, "", aIsConst);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslAddVar

ZExportAPI(long) ZCslSetResult(          /* set return result */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aBuffer,                     /* buffer for result */
   long aSize)                              /* buffer size, -1 = ASCIZ */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   if (aSize < 0)
      aCsl->result() = aBuffer;
   else
      aCsl->result() = ZString(aBuffer, aSize);
   return 0;
} // ZCslSetResult

ZExportAPI(long) ZCslSetError(           /* set/add error text */
   ZCslHandle aHandle,                      /* CSL handle */
   const char *aBuffer,                     /* buffer for result */
   long aSize)                              /* buffer size, -1 = ASCIZ */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   if (aSize < 0)
      aCsl->errs().addAsLast(aBuffer);
   else
      aCsl->errs().addAsLast(ZString(aBuffer, aSize));
   return 0;
} // ZCslSetError

ZExportAPI(long) ZCslStartDateTime(      /* get start date/time */
   ZCslHandle aHandle,                      /* CSL handle */
   long *aYear,                             /* year (NULL = not required) */
   long *aMonth,                            /* month 1...12 (NULL = not required) */
   long *aDay,                              /* day 1...31 (NULL = not required) */
   long *aHour,                             /* hour 0...23 (NULL = not required) */
   long *aMinute,                           /* minute 0...59 (NULL = not required) */
   long *aSecond)                           /* second 0...59 (NULL = not required) */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      ZDateTime dt(aCsl->startDateTime());
      if (aYear)   *aYear   = dt.year();
      if (aMonth)  *aMonth  = dt.month();
      if (aDay)    *aDay    = dt.day();
      if (aHour)   *aHour   = dt.hour();
      if (aMinute) *aMinute = dt.minute();
      if (aSecond) *aSecond = dt.second();
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslStartDate

ZExportAPI(long) ZCslTraceMode(          /* query trace mode */
   ZCslHandle aHandle,                      /* CSL handle */
   long *aMode)                             /* buffer for mode */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      *aMode = aCsl->traceMode();
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslTraceMode

ZExportAPI(long) ZCslSetTraceMode(       /* set trace mode */
   ZCslHandle aHandle,                      /* CSL handle */
   long aMode)                              /* new trace mode */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      if (aMode < 0 || aMode > ZCsl::traceAll) aMode = 0;
      aCsl->setTraceMode((ZCsl::TraceMode)aMode);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslSetTraceMode

ZExportAPI(long) ZCslTrace(              /* write trace message */
   ZCslHandle aHandle,                      /* CSL handle */
   const char* aMessage)                    /* trace message */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      aCsl->trace(aMessage);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslTrace

ZExportAPI(long) ZCslShow(               /* show internal informations */
   ZCslHandle aHandle,                      /* CSL handle */
   long aMode,                              /* show mode */
   long aDepth)                             /* how much, -1 = all */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      clearStatus(aCsl);
      if (aDepth < 0) aDepth = 99999999;
      aCsl->show((ZCsl::ShowMode)aMode, aDepth);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslShow

ZExportAPI(long) ZCslGetError(           /* Get error text */
   ZCslHandle aHandle,                      /* CSL handle */
   long aIndex,                             /* index of text (0 based) */
   char *aBuffer,                           /* buffer for error text (NULL = query size) */
   long *aSize)                             /* buffer size information */
{
   ZCsl* aCsl = (ZCsl*)aHandle;
   try {
      ZString msg;
      ZString err1("Invalid index in ZCslGetError");
      if (aCsl) {
         ZException& errs = aCsl->errs();
         if (aIndex >= 0 && aIndex < errs.count())
            msg = errs[aIndex];
         else
            ZTHROWEXC(err1);
      } else {
         if (aIndex >= 0 && aIndex < iErrs.count())
            msg = iErrs[aIndex];
         else
            ZTHROWEXC(err1);
      } // if
      fillBuffer(msg, aBuffer, aSize);
   } // try
   catch (const ZException& exc) {
      return handleCatch(aCsl, exc);
   } // catch
   return 0;
} // ZCslGetError
