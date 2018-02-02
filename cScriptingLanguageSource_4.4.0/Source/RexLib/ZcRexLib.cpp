/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZcRexLib.cpp
 * Application :  CSL regular expression library
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.30  First implementation                        P.Koch, IBK
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

#include <ZRegular.hpp>
#include <ZExcept.hpp>

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

static ZRegularExpression* getHandle(ZCsl* aCsl)
{
   ZRegularExpression* rex = (ZRegularExpression*)aCsl->get("handle").asUnsigned();
   if (iList->find(rex) < 0)
      ZTHROWEXC("Invalid regular expression handle!");
   return rex;
} // getHandle

static ZString rexClose(ZCsl* csl)
{
   ZRegularExpression* rex = getHandle(csl);
   iList->removeFromPos(iList->find(rex));
   delete rex;
   return 1;
} // rexClose

static ZString rexMatch(ZCsl* csl)
{
   // get and check nmatch
   int nmatch = csl->get("nmatch").asInt();

   // get flags
   int flags(0);
   if (csl->get("argCount").asInt() > 4) flags = csl->get("flags").asInt();

   long start[ZRegularExpression::maxMatches];
   long length[ZRegularExpression::maxMatches];

   // do the matching
   int cnt = getHandle(csl)->match(csl->get("string"), nmatch, start, length, flags);

   // get the results
   for (int x = 0; x < cnt; x++) {
      csl->set("match["+ZString(x)+"][0]", start[x]);
      csl->set("match["+ZString(x)+"][1]", length[x]);
   } // for
   return cnt;
} // rexMatch

static ZString rexOpen(ZCsl* csl)
{
   int flags(0);
   if (csl->get("argCount").asInt() > 1) flags = csl->get("flags").asInt();
   ZRegularExpression* rex = new ZRegularExpression(csl->get("pattern"), flags);
   iList->addAsLast(rex);
   return (unsigned long)rex;
} // rexOpen

ZCslInitLib(aCsl)
{
   if (iUseCount++ == 0 && !iList)
      iList = new ZPointerlist;

   ZString iFile("ZcRexLib");
   ZString init(ZString(
      "const rexVersion = '4.04';\n"
      "const rexCompiler = '")+ZC_COMPILER+"';\n"
      "const rexLibtype = '"+ZC_CSLLIBTYPE+"';\n"
      "const rexBuilt = '"+ZString(__DATE__)+" "+__TIME__+"';\n"

      "const rexOpenExtended   = "+ZString((int)ZRegularExpression::openExtended)+";\n"
      "const rexOpenIgnorecase = "+ZString((int)ZRegularExpression::openIgnorecase)+";\n"
      "const rexOpenNewline    = "+ZString((int)ZRegularExpression::openNewline)+";\n"
      "const rexOpenNosubreps  = "+ZString((int)ZRegularExpression::openNosubreps)+";\n"

      "const rexMatchNotbol    = "+ZString((int)ZRegularExpression::matchNotBol)+";\n"
      "const rexMatchNoteol    = "+ZString((int)ZRegularExpression::matchNotEol)+";\n"
   );
   istrstream str((char*)init);
   aCsl->loadScript(iFile, &str);
   (*aCsl)
      .addFunc(iFile,
         "rexClose("                     // close regular expression handle
            "const handle)",                // rex handle
         rexClose)

      .addFunc(iFile,
         "rexMatch("                     // compile regular expression
            "const handle, "                // rex handle
            "const string, "                // string to match
            "const nmatch, "                // # of matches to find
            "var &match[][], "              // index & length of each match
           "[const flags])",                // match flags
         rexMatch)

      .addFunc(iFile,
         "rexOpen("                      // compile regular expression
            "const pattern, "               // pattern to compile
           "[const flags])",                // open flags
         rexOpen);
} // ZCslInitLib

ZCslCleanupLib(aCsl)
{
   if (--iUseCount == 0 && iList) {
      delete iList;
      iList = 0;
   } // if
} // ZCslCleanupLib
