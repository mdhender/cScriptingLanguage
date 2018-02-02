/*  Copyright (c) 2000-2001 IBK-Landquart-Switzerland. All rights reserved.
 *
 *  Module      :  MyLib.c
 *  Application :  CSL Sample Library in plain C
 *  Author      :  Peter Koch, IBK
 *
 *  NOTES:
 *
 *  (1) Within C/C++ implementions of CSL functions, as well as within
 *      initialize and cleanup, there is no need to handle errors since
 *      that will be done by the caller. Just return in case of an error.
 *
 *  (2) These functions will allways return a value of 0, so there is no
 *      need to check for errors.
 *
 *  Date        Description                                 Who
 *  --------------------------------------------------------------------------
 *  Feb 2000    First release                               P.Koch, IBK
 *  Jun 2001    Adaptions for V4                            P.Koch, IBK
 */

#include <stdlib.h>
#include <stdio.h>

#include <ZCslApi.h>

/*
 * c h e c k N u m b e r
 *
 * Check if string represents a number
 */
static int checkNumber(char *s)
{
   int any;
   any = 0;
   if (*s=='-' || *s=='+') s++;
   while ('0'<=*s && *s<='9') { s++; any = 1; }
   if (*s=='.') s++;
   while ('0'<=*s && *s<='9') s++;
   return any && *s == 0;
} /* checkNumber */

/*
 * a v e r a g e
 *
 * Sample CSL function calculating the average of up 5 numbers
 */
ZExportAPI(void) average(ZCslHandle aCsl)
{
   double sum;
   long bufsiz;
   int argCount, i;
   char buf[40], name[4];

   /* get actual # of arguments */
   bufsiz = sizeof(buf);
   ZCslGet(aCsl, "argCount", buf, &bufsiz);
   argCount = atoi(buf);

   /* calculate sum of all arguments */
   sum = 0.0;
   for (i = 0; i < argCount; i++) {
      /* create name of parameter */
      sprintf(name, "p%d", i+1);

      /* get argument */
      bufsiz = sizeof(buf);
      if ( ZCslGet(aCsl, name, buf, &bufsiz) ) return; /* (1) */

      /* check for number */
      if (!checkNumber(buf)) {
         sprintf(buf, "argument %d is no number!", i);
         ZCslSetError(aCsl, buf, -1); /* (2) */
      } /* if */

      sum += atof(buf);
   } /* for */

   /* return result */
   sprintf(buf, "%f", sum / argCount);
   ZCslSetResult(aCsl, buf, -1); /* (2) */
} /* average */

/*
 * i n i t i a l i z e
 *
 * initialize csl library at load time
 */
ZCslInitLib(csl)
{
   static char* module = "MyLib";    /* module name */
   long errs;

   /* define a global constant by loading a script */
   errs = ZCslLoadScriptMem(
      csl,                               /* csl handle */
      module,                            /* module name */
      "const myVersion = 1.0;\n"        /* script source */
   );
   if (errs) return; /* (1) */

   /* load a function (no errs check since returning hereafter anyway) */
   ZCslAddFunc(
      csl,                               /* handle */
      module,                            /* module name */
      "average(const p1, "               /* declaration */
              "const p2, "
             "[const p3, "
              "const p4, "
              "const p5])",
      average);                          /* function address */
} /* initialize */

/*
 * c l e a n u p
 *
 * clean up csl library before unloading
 */
ZCslCleanupLib(csl)
{
   /* nothing to clean up in our sample */
} /* cleanup */
