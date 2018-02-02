/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZcPrfLib.cpp
 * Application :  CSL profile library
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.31  First implementation                        P.Koch, IBK
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

#include <ZProfile.hpp>
#include <ZExcept.hpp>
#include <ZTrace.hpp>

#if ZC_GNU
  #include <strstream.h>
#else
  #include <strstrea.h>
#endif

#ifdef ZC_NATIVECSLLIB
  #include <ZCsl.hpp>
#else
  #include <ZCslWrap.hpp>
#endif

static ZPointerlist* iList = 0;
static long iUseCount(0);

////////////////////////////////// Utilities ////////////////////////////////

static ZProfile* getHandle(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("getHandle(ZCsl* aCsl)");
   ZProfile* h = (ZProfile*)aCsl->get("handle").asUnsignedLong();
   if (iList->find(h) < 0)
      ZTHROWEXC("Invalid profile handle!");
   return h;
} // getHandle

//////////////////////////////// CSL Functions //////////////////////////////

static ZString prfOpen(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfOpen(ZCsl* aCsl)");
   ZString path;
   if (aCsl->get("argCount").asInt() == 2)
      path = aCsl->get("path");
   ZProfile* p = new ZProfile(aCsl->get("root"), path);
   iList->addAsLast(p);
   return (unsigned long)p;
} // prfOpen

static ZString prfOpenUser(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfOpenUser(ZCsl* aCsl)");
   ZString path;
   if (aCsl->get("argCount").asInt() == 1)
      path = aCsl->get("path");
   ZProfile* p = new ZProfile(ZProfile::userProfile(path));
   iList->addAsLast(p);
   return (unsigned long)p;
} // prfOpenUser

static ZString prfOpenSystem(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfOpenSystem(ZCsl* aCsl)");
   ZString path;
   if (aCsl->get("argCount").asInt() == 1)
      path = aCsl->get("path");
   ZProfile* p = new ZProfile(ZProfile::systemProfile(path));
   iList->addAsLast(p);
   return (unsigned long)p;
} // prfOpenSystem

static ZString prfClose(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfClose(ZCsl* aCsl)");
   ZProfile* p = getHandle(aCsl);
   iList->removeFromPos(iList->find(p));
   delete p;
   return 1;
} // prfClose

static ZString prfPath(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfPath(ZCsl* aCsl)");
   ZProfile* p = getHandle(aCsl);
   if (aCsl->get("argCount").asInt() == 2)
      p->setPath(aCsl->get("path"));
   return p->path();
} // prfPath

static ZString prfGetKeys(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfGetKeys(ZCsl* aCsl)");
   ZProfile* p = getHandle(aCsl);
   ZStringlist list;
   p->getKeys(list);
   if (aCsl->get("argCount").asInt() == 2) {
      long maxVals = aCsl->varSizeof("keys");
      for (int i = 0; i < list.count() && i < maxVals; i++)
         aCsl->set("keys["+ZString(i)+"]", list[i]);
   } // if
   return list.count();
} // prfGetKeys

static ZString prfKeyExists(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfKeyExists(ZCsl* aCsl)");
   ZString key;
   if (aCsl->get("argCount").asInt() == 2)
      key = aCsl->get("key");
   return getHandle(aCsl)->keyExists(key);
} // prfKeyExists

static ZString prfDeleteKey(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfDeleteKey(ZCsl* aCsl)");
   getHandle(aCsl)->deleteKey(aCsl->get("key"));
   return 1;
} // prfDeleteKey

static ZString prfGetValues(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfGetValues(ZCsl* aCsl)");
   ZProfile* p = getHandle(aCsl);
   ZStringlist list;
   p->getValues(list);
   if (aCsl->get("argCount").asInt() == 2) {
      long maxVals = aCsl->varSizeof("values");
      for (int i = 0; i < list.count() && i < maxVals; i++)
         aCsl->set("values["+ZString(i)+"]", list[i]);
   } // if
   return list.count();
} // prfGetValues

static ZString prfValueType(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfValueType(ZCsl* aCsl)");
   ZString name;
   if (aCsl->get("argCount").asInt() == 2)
      name = aCsl->get("name");
   return getHandle(aCsl)->valueType(name);
} // prfValueType

static ZString prfValueExists(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfValueExists(ZCsl* aCsl)");
   ZString name;
   if (aCsl->get("argCount").asInt() == 2)
      name = aCsl->get("name");
   return getHandle(aCsl)->valueExists(name);
} // prfValueExists

static ZString prfGetValue(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfGetValue(ZCsl* aCsl)");
   ZString name;
   if (aCsl->get("argCount").asInt() == 2)
      name = aCsl->get("name");
   ZProfile* p = getHandle(aCsl);
   if (p->valueType(name) == ZProfile::Integer)
      return p->longValue(name);
   else
      return p->value(name);
} // prfGetValue

static ZString prfSetValue(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfSetValue(ZCsl* aCsl)");
   ZString name;
   int type(ZProfile::Auto);
   switch (aCsl->get("argCount").asInt()) {
      case 4: type = aCsl->get("type").asInt();
      case 3: name = aCsl->get("name");
      default:;
   } // switch
   getHandle(aCsl)->setValue(aCsl->get("data"), name, type);
   return 1;
} // prfSetValue

static ZString prfDeleteValue(ZCsl* aCsl)
{
   ZFUNCTRACE_DEVELOP("prfDeleteValue(ZCsl* aCsl)");
   ZString name;
   if (aCsl->get("argCount").asInt() == 2)
      name = aCsl->get("name");
   getHandle(aCsl)->deleteValue(name);
   return 1;
} // prfDeleteValue

//////////////////////////////////// Library Entries ////////////////////////

ZCslInitLib(aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslInitLib(aCsl)");

   if (iUseCount++ == 0 && !iList)
      iList = new ZPointerlist;

   ZString iFile("ZcPrfLib");
   ZString init(ZString(
      "const prfVersion = '4.04';\n"
      "const prfCompiler = '")+ZC_COMPILER+"';\n"
      "const prfLibtype = '"+ZC_CSLLIBTYPE+"';\n"
      "const prfBuilt = '"+ZString(__DATE__)+" "+__TIME__+"';\n"

      "const prfTypeString = "+ZString((int)ZProfile::String)+";\n"
      "const prfTypeBinary = "+ZString((int)ZProfile::Binary)+";\n"
      "const prfTypeInteger = "+ZString((int)ZProfile::Integer)+";\n"
      "const prfTypeOther = "+ZString((int)ZProfile::Other)+";\n"
      "const prfTypeAuto = "+ZString((int)ZProfile::Auto)+";\n"
   );
   istrstream str((char*)init);

   aCsl->loadScript(iFile, &str);
   (*aCsl)
      .addFunc(iFile,
         "prfOpen("
            "const root,"
            "[const path])",
          prfOpen)

      .addFunc(iFile,
         "prfOpenSystem("
            "[const path])",
          prfOpenSystem)

      .addFunc(iFile,
         "prfOpenUser("
            "[const path])",
          prfOpenUser)

      .addFunc(iFile,
         "prfClose("
            "const handle)",
          prfClose)

      .addFunc(iFile,
         "prfPath("
            "const handle,"
            "[const path])",
          prfPath)

      .addFunc(iFile,
         "prfGetKeys("
            "const handle,"
            "[var& keys[]])",
          prfGetKeys)

      .addFunc(iFile,
         "prfKeyExists("
            "const handle,"
            "[const key])",
          prfKeyExists)

      .addFunc(iFile,
         "prfDeleteKey("
            "const handle,"
            "const key)",
          prfDeleteKey)

      .addFunc(iFile,
         "prfGetValues("
            "const handle,"
            "[var& values[]])",
          prfGetValues)

      .addFunc(iFile,
         "prfValueType("
            "const handle,"
            "[const name])",
          prfValueType)

      .addFunc(iFile,
         "prfValueExists("
            "const handle,"
            "[const name])",
          prfValueExists)

      .addFunc(iFile,
         "prfGetValue("
            "const handle,"
            "[const name])",
          prfGetValue)

      .addFunc(iFile,
         "prfSetValue("
            "const handle,"
            "const data,"
            "[const name,"
             "const type])",
          prfSetValue)

      .addFunc(iFile,
         "prfDeleteValue("
            "const handle,"
            "[const name])",
          prfDeleteValue);
} // ZCslInitLib

ZCslCleanupLib(aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslCleanupLib(aCsl)");
   if (--iUseCount == 0 && iList) {
      delete iList;
      iList = 0;
   } // if
} // ZCslCleanupLib
