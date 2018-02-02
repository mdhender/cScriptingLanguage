/*  Copyright (c) 1998-2001 IBK-Landquart-Switzerland. All rights reserved.
 *
 *  Module      :  Embed.cpp
 *  Application :  Sample to embed CSL as macro language by class library
 *  Author      :  Peter Koch, IBK
 *
 *  Date        Description                                 Who
 *  --------------------------------------------------------------------------
 *  Feb 2000    First release                               P.Koch, IBK
 *  Jun 2001    Adaptions for V4                            P.Koch, IBK
 */
#include <stdlib.h>
#include <stdio.h>
#include <ZExcept.hpp>

#if ZC_WIN
  #include <ZCslWrap.hpp>
#else
  #include <ZCsl.hpp>
#endif

#if ZC_GNU
#include <strstream.h>
#else
#include <strstrea.h>
#endif

static ZCsl* csl = 0;                    // csl object ptr
static ZBoolean cslOk(zFalse);           // ok flag
static ZString module("Embed");          // module name

/*
 * c h e c k N u m b e r
 *
 * Check if string represents a number
 */
static ZBoolean checkNumber(const ZString& str)
{
   const char *s = str;
   if (*s=='-' || *s=='+') s++;
   ZBoolean any(zFalse);
   while ('0'<=*s && *s<='9') { s++; any = zTrue; }
   if (*s=='.') s++;
   while ('0'<=*s && *s<='9') s++;
   return any && *s == 0;
} // checkNumber

/*
 * a v e r a g e
 *
 * Sample CSL function calculating the average of numbers
 */
static ZString average(ZCsl* aCsl)
{
   // get actual # of arguments
   int argCount = aCsl->get("argCount").asInt();

   // calculate sum of all arguments
   double sum(0.0);
   for (int i = 0; i < argCount; i++) {
      // get argument
      ZString val = aCsl->get("p"+ZString(i+1));

      // check for number
      if (!checkNumber(val))
         ZTHROWEXC("argument "+ZString(i+1)+" is no number!");

      sum += val.asDouble();
   } // for

   // return result
   return ZString(sum / argCount);
} // average

int main()
{
   int ret(0);
   try {
      cout << "creating csl object" << endl;
      csl = new ZCsl();
      cslOk = zTrue;

      cout << endl << "get csl version" << endl;
      cout << "  csl version is " << csl->get("cslVersion") << endl;

      cout << endl << "load c++ function 'average'" << endl;
      csl->addFunc(
         module,
         "average(const p1, [const p2, const p3, const p4, const p5])",
         average);

      cout << endl << "call 'average' from c++" << endl;
      ZString res = csl->call(module, "average", 3, "3", "12", "17");
      cout << "  result = " << res << endl;

      cout << endl << "compile a script from memory" << endl;
      istrstream str(
         "#loadLibrary 'ZcSysLib'\n"
         ""
         "check()\n"
         "{\n"
         "   sysLog('the average of 3,5,12,7 is '\n"
         "          |average(3,5,12,7)\n"
         "   );\n"
         "}\n"
      );
      csl->loadScript(module, &str);

      cout << endl << "call 'check' within compiled script" << endl;
      csl->call(module, "check");

      cout << endl << "deleting csl object" << endl;
      delete csl;
   } // try
   catch (const ZException& err) {
      for (int i = 0; i < err.count(); i++)
         cerr << err[i] << endl;
      if (cslOk) delete csl;
      ret = 1;
   } // catch
   return ret;
} // main
