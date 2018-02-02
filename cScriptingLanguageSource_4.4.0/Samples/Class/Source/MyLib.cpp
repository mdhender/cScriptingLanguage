/*  Copyright (c) 1998-2001 IBK-Landquart-Switzerland. All rights reserved.
 *
 *  Module      :  MyLib.cpp
 *  Application :  CSL Sample Library
 *  Author      :  Peter Koch, IBK
 *
 *  Date        Description                                 Who
 *  --------------------------------------------------------------------------
 *  Jan 1998    First release                               P.Koch, IBK
 *  Feb 2000    Revised                                     P.Koch, IBK
 *  Jun 2001    Adaptions for V4                            P.Koch, IBK
 */

#include <ZBase.h>  // load ZC_.... defines

#if ZC_GNU
  #include <strstream.h>
#else
  #include <strstrea.h>
#endif

#if ZC_WIN
  #include <ZCslWrap.hpp>
#else
  #include <ZCsl.hpp>
#endif

static ZString myStrStrip(ZCsl* csl)
{
   return csl->get("string").strip();
} // myStrStrip

static ZString mySubString(ZCsl* csl)
{
   int argc = csl->get("argCount").asInt();
   switch (argc) {
      case 2:
         return csl->get("string").subString(
                   csl->get("start").asInt()
                );
      case 3:
         return csl->get("string").subString(
                   csl->get("start").asInt(),
                   csl->get("count").asInt()
                );
      default:
         return csl->get("string").subString(
                   csl->get("start").asInt(),
                   csl->get("count").asInt(),
                   csl->get("padchar")[1]
                );
   } // switch
} // mySubString

ZCslInitLib(csl)
{
   ZString iFile("MyLib");
   istrstream init("const myVersion = 0.1;\n");
   csl->loadScript(iFile, &init);
   (*csl)
      .addFunc(
          iFile,
         "myStrStrip(const string)",
          myStrStrip)
      .addFunc(
          iFile,
         "mySubString(const string, const start, [const count, const padchar])",
          mySubString);
} // ZCslInitLib

ZCslCleanupLib(csl)
{
   // nothing to cleanup in this sample
} // ZCslCleanupLib
