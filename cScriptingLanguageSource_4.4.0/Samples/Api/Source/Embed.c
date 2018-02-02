/*  Copyright (c) 1998-2001 IBK-Landquart-Switzerland. All rights reserved.
 *
 *  Module      :  Embed.c
 *  Application :  Sample to embed CSL as macro language with C-API
 *  Author      :  Peter Koch, IBK
 *
 *  NOTES:
 *
 *  (1) Within C/C++ implementions of CSL functions there is no need to
 *      handle errors since that will be done by the caller. Just return
 *      in case of an error.
 *
 *  (2) These functions will allways return a value of 0, so there is no
 *      need to check for errors.
 *
 *  (3) In real applications you could first call ZCslGet to get the
 *      required size and then allocate the buffer dynamicly (like in
 *      checkErrs).

 *  Date        Description                                 Who
 *  --------------------------------------------------------------------------
 *  Feb 2000    First release                               P.Koch, IBK
 */
#include <stdlib.h>
#include <stdio.h>
#include <ZCslApi.h>

static char* module = "Embed";           /* module name */
static ZCslHandle csl;                   /* csl handle */
static long errs;                        /* csl api return code */

/*
 * c h e c k E r r s
 *
 * This function ckecks for errors. Error messages are in case displayed
 * and program is terminated to keep the sample as simple as possible.
 */
static void checkErrs(void)
{
   long size, errs2;
   int i;
   char *msg;

   if (errs) {
      fprintf(stderr, "ZCsl error found:\r\n");
      for (i = 0; i < errs; i++) {
         /* First we get the size of the message only so a buffer */
         /* large enough for the string can be allocated: */
         errs2 = ZCslGetError(csl, i, 0, &size); /* get size of message */
         if (errs2) { errs = errs2; checkErrs();  }

         /* Now a buffer is allocated and the message is retrieved */
         /* and displayed: */
         msg = (char*)malloc(size);
         errs2 = ZCslGetError(csl, i, msg, &size); /* get message */
         if (errs2) { errs = errs2; checkErrs();  }
         fprintf(stderr, "%s\r\n", msg);

         /* the buffer is no longer required and gets discarded: */
         free(msg);
      } /* for */
      exit(1);
   } /* if */
} /* checkErrs */

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
   int argCount, i;
   long bufsiz;
   double sum;
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
         sprintf(buf, "argument p%d is no number!", i+1);
         ZCslSetError(aCsl, buf, -1); /* (2) */
      } /* if */

      sum += atof(buf);
   } /* for */

   /* return result */
   sprintf(buf, "%f", sum / argCount);
   ZCslSetResult(aCsl, buf, -1); /* (2) */
} /* average */

int main()
{
   long bufsize;
   char buf[1024];
   static char *vals[] = { "3", "12", "17" };

   printf("opening csl\r\n");
   errs = ZCslOpen(&csl,0);
   checkErrs();

   printf("\r\nget csl version\r\n");
   bufsize = sizeof(buf);
   errs = ZCslGet(csl, "cslVersion", buf, &bufsize); /* (1) */
   checkErrs();
   printf("  csl version is %s\r\n", buf);

   printf("\r\nload C function 'average'\r\n");
   errs = ZCslAddFunc(
      csl,                               /* handle */
      module,                            /* module name */
      "average(const p1, "               /* declaration */
             "[const p2, "
              "const p3, "
              "const p4, "
              "const p5])",
      average);                          /* function address */
   checkErrs();

   printf("\r\ncall 'average' from C\r\n");
   errs = ZCslCall(
      csl,                               /* handle */
      module,                            /* module name */
      "average",                         /* function to call */
      sizeof(vals)/sizeof(char*),        /* # of arguments */
      vals);                             /* arguments */
   checkErrs();

   printf("\r\nget result\r\n");
   bufsize = sizeof(buf);
   errs = ZCslGetResult(csl, buf, &bufsize); /* (1) */
   checkErrs();
   printf("  result = %s\r\n", buf);

   printf("\r\ncompile a script from memory\r\n");
   errs = ZCslLoadScriptMem(
      csl,                               /* handle */
      module,                            /* module name */

      "#loadLibrary 'ZcSysLib'\n"        /* the script */
      ""
      "check()\n"
      "{\n"
      "   sysLog('the average of 3, 5, 12 and 7 is '\n"
      "          |average(3,5,12,7)\n"
      "   );\n"
      "}\n"
   );
   checkErrs();

   printf("\r\ncall 'check' within compiled script\r\n");
   errs = ZCslCall(csl, module, "check", 0, 0);
   checkErrs();

   printf("\r\nclosing csl\r\n");
   errs = ZCslClose(csl);
   checkErrs();
   return 0;
} /* main */
