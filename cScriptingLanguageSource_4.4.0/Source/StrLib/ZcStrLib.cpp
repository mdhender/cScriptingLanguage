/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZcStrLib.cpp
 * Application :  CSL string library
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.27  First implementation                        P.Koch, IBK
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

#include <stdio.h>
#include <stdlib.h>
#include <ZTrace.hpp>
#include <ZFile.hpp>

#ifdef ZC_NATIVECSLLIB
  #include <ZCsl.hpp>
#else
  #include <ZCslWrap.hpp>
#endif

#if ZC_GNU
  #include <strstream.h>
#else
  #include <strstrea.h>
#endif

static ZString strFormatNumber(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strFormatNumber(ZCsl* csl)");
   int argc = csl->get("argCount").asInt();
   int frac(0);
   if (argc == 3) frac = csl->get("frac").asInt();
   char buf[256];
   sprintf(buf, "%*.*f",
      csl->get("width").asInt(),
      frac,
      csl->get("val").asDouble()
   );
   return buf;
} // strFormatNumber

static ZString strCenter(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strCenter(ZCsl* csl)");
   char padd(' ');
   int argc = csl->get("argCount").asInt();
   if (argc == 3) {
      ZString p(csl->get("padd"));
      if (p.length() > 0) padd = p[1];
   } // if
   return
      csl->get("string")
         .center(
            csl->get("width").asLong(),
            padd);
} // strCenter

static ZString strChange(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strChange(ZCsl* csl)");
   long start(1);
   long count(ZBase::maxLong);
   ZBoolean ignoreCase(zFalse);
   switch(csl->get("argCount").asInt()) {
      case 6: ignoreCase = csl->get("ignoreCase").asInt()!=0;
      case 5: count = csl->get("count").asLong();
      case 4: start = csl->get("start").asLong();
      default:;
   } // switch
   return
      csl->get("string")
         .change(
            csl->get("oldpatt"),
            csl->get("newpatt"),
            start,
            count,
            ignoreCase);
} // strChange

static ZString strLeftJustify(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strLeftJustify(ZCsl* csl)");
   char padd(' ');
   int argc = csl->get("argCount").asInt();
   if (argc == 3) {
      ZString p(csl->get("padd"));
      if (p.length() > 0) padd = p[1];
   } // if
   return
      csl->get("string")
         .leftJustify(
            csl->get("width").asLong(),
            padd);
} // strLeftJustify

static ZString strRightJustify(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strRightJustify(ZCsl* csl)");
   char padd(' ');
   int argc = csl->get("argCount").asInt();
   if (argc == 3) {
      ZString p(csl->get("padd"));
      if (p.length() > 0) padd = p[1];
   } // if
   return
      csl->get("string")
         .rightJustify(
            csl->get("width").asLong(),
            padd);
} // strRightJustify

static ZString strUpper(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strUpper(ZCsl* csl)");
   return csl->get("string").upperCase();
} // strUpper

static ZString strLength(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strLength(ZCsl* csl)");
   return ZString(csl->get("string").length());
} // strLength

static ZString strLower(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strLower(ZCsl* csl)");
   return csl->get("string").lowerCase();
} // strLower

static ZString strStrip(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strStrip(ZCsl* csl)");
   char ch(' ');
   if (csl->get("argCount").asInt()==2) {
      ZString chr(csl->get("char"));
      if (chr.size() > 0) ch = chr[1];
   } // if
   ZTRACE_DEVELOP("char = <"+ZString(ch)+">");
   return csl->get("string").strip(ch);
} // strStrip

static ZString strStripTrailing(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strStripTrailing(ZCsl* csl)");
   char ch(' ');
   if (csl->get("argCount").asInt()==2) {
      ZString chr(csl->get("char"));
      if (chr.size() > 0) ch = chr[1];
   } // if
   ZTRACE_DEVELOP("char = <"+ZString(ch)+">");
   return csl->get("string").stripTrailing(ch);
} // strStripTrailing

static ZString strStripLeading(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strStripLeading(ZCsl* csl)");
   char ch(' ');
   if (csl->get("argCount").asInt()==2) {
      ZString chr(csl->get("char"));
      if (chr.size() > 0) ch = chr[1];
   } // if
   ZTRACE_DEVELOP("char = <"+ZString(ch)+">");
   return csl->get("string").stripLeading(ch);
} // strStripLeading

static ZString strSpread(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strSpread(ZCsl* csl)");
   ZString src(csl->get("string"));
   ZString dst;
   for (int i = 1; i <= src.length(); i++) {
      dst += src[i];
      dst += ' ';
   } // for
   return dst;
} // strSpread

static ZString strSubString(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strSubString(ZCsl* csl)");
   long count(-1);
   char padd(' ');
   switch (csl->get("argCount").asInt()) {
      case 4: {
         ZString p(csl->get("padd"));
         if (p.length() > 0) padd = p[1];
      }
      case 3:
         count = csl->get("count").asLong();
      default:;
   } // switch
   return
      csl->get("string").subString(
         csl->get("start").asLong(),
         count,
         padd);
} // strSubString

static ZString strSplitPath(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strSplitPath(ZCsl* csl)");
#if ZC_WIN || ZC_OS2
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];
 #if ZC_GNU
   _splitpath(csl->get("path").constBuffer(), drive, dir, fname, ext);
 #else
   _splitpath(csl->get("path"), drive, dir, fname, ext);
 #endif
#endif
#if ZC_UNIXFAM
   ZString dir, fname, ext, drive;
   fname = ZFile::stripPath(csl->get("path"), dir);
   fname = ZFile::stripExtension(fname, ext);
#endif
   switch (csl->get("argCount").asInt()) {
      case 4: csl->set("drive", drive);
      case 3: csl->set("dir", dir);
      case 2: csl->set("ext", ext);
      default:;
   } // switch
   return fname;
} // strSplitPath

static ZString strSplitConnectString(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strSplitConnectString(ZCsl* csl)");
   ZString connstr(csl->get("connstr"));
   char* s = connstr;
   ZString db, name, pass, conn;
   ZString delim(":/@");
   char lastdel(0);
   while (*s) {
      ZString wrk;
      while (*s && !delim.indexOf(*s)) wrk += *s++;
      if (*s==':')
         db = wrk.upperCase();
      else
         switch (lastdel) {
            case '/': pass = wrk; break;
            case '@': conn = wrk; break;
            default : name = wrk;
         } // switch
      if (!*s) break;
      switch (lastdel = *s++) {
         case ':': delim = "/@"; break;
         case '/': delim = "@"; break;
         default : delim = "";
      } // switch
   } // while
   switch (csl->get("argCount").asInt()) {
      case 4: csl->set("database",   db);
      case 3: csl->set("connection", conn);
      case 2: csl->set("password",   pass);
      default:;
   } // switch
   return name;
} // strSplitConnectString

static ZString strStripExtension(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strStripExtension(ZCsl* csl)");
   ZString base(csl->get("filename"));
   int pos(base.lastIndexOf('.'));
   if (pos > base.lastIndexOf(ZC_PATHSEPARATOR))
      base = base.subString(1,pos-1);
   return base;
} // strStripExtension

static ZString strWords(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strWords(ZCsl* csl)");
   long count(ZBase::maxLong);
   int argc = csl->get("argCount").asInt();
   if (argc == 3) count = csl->get("count").asLong();
   return
      csl->get("string")
         .words(
            csl->get("start").asLong(),
            count);
} // strWords

static ZString strWordCount(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strWordCount(ZCsl* csl)");
   return ZString(csl->get("string").wordCount());
} // strWordCount

static ZString strRemoveWords(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strRemoveWords(ZCsl* csl)");
   long count(ZBase::maxLong);
   int argc = csl->get("argCount").asInt();
   if (argc == 3) count = csl->get("count").asLong();
   return
      csl->get("string").removeWords(
         csl->get("start").asLong(),
         count);
} // strRemoveWords

static ZString strIsNumber(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strIsNumber(ZCsl* csl)");
   ZString string(csl->get("string"));
   char* s = string;
   if (*s=='-' || *s=='+') s++;
   ZBoolean any(zFalse);
   while ('0'<=*s && *s<='9') { s++; any = zTrue; }
   if (!*s || !any) return any;
   if (*s == '.') {
      s++;
      while ('0'<=*s && *s<='9') s++;
      if (!*s) return any;
   } // if
   any = zFalse;
   if (*s != 'e' && *s != 'E') return any;
   s++;
   if (*s == '-' || *s == '+') s++;
   while ('0'<=*s && *s<='9') { s++; any = zTrue; }
   return (int)(any && *s == 0);
} // strIsNumber

static ZString strIsInteger(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strIsInteger(ZCsl* csl)");
   ZString string(csl->get("string"));
   char* s = string;
   if (*s=='-' || *s=='+') s++;
   ZBoolean any(zFalse);
   double val(0.0);
   while ('0'<=*s && *s<='9') {
      val = val * 10.0 + *s++ - '0';
      any = zTrue;
   } // while
   return (int)(any && *s == 0 && val == int(val));
} // strIsInteger

static ZString strIsPrintable(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strIsPrintable(ZCsl* csl)");
   return csl->get("string").isPrintable();
} // strIsPrintable

static ZString strExport(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strExport(ZCsl* csl)");
   return ZString::exp(csl->get("string"));
} // strExport

static ZString strImport(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strImport(ZCsl* csl)");
   return ZString::imp(csl->get("string"));
} // strImport

static ZString strAscii(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strAscii(ZCsl* csl)");
   return int(csl->get("character")[1]);
} // strAscii

static ZString strChar(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strChar(ZCsl* csl)");
   unsigned char chr;
   chr = csl->get("asciival").asInt();
   return chr;
} // strChar

static ZString strIndexOf(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strIndexOf(ZCsl* csl)");
   long start(1);
   ZBoolean ignoreCase(zFalse);
   switch (csl->get("argCount").asInt()) {
      case 4: ignoreCase = csl->get("ignoreCase").asInt()!=0;
      case 3: start = csl->get("start").asLong();
      default:;
   } // switch
   return
      csl->get("string")
         .indexOf(
            csl->get("pattern"),
            start,
            ignoreCase);
} // strIndexOf

static ZString strLastIndexOf(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strLastIndexOf(ZCsl* csl)");
   long start(ZBase::maxLong);
   ZBoolean ignoreCase(zFalse);
   switch (csl->get("argCount").asInt()) {
      case 4: ignoreCase = csl->get("ignoreCase").asInt()!=0;
      case 3: start = csl->get("start").asLong();
      default:;
   } // switch
   return
      csl->get("string")
         .lastIndexOf(
            csl->get("pattern"),
            start,
            ignoreCase);
} // strLastIndexOf

static ZString buildAscii(ZString val, char type, int size, int frac)
{
   ZFUNCTRACE_DEVELOP("buildAscii(ZString val, char type, int size, int frac)");
   if (type=='t' || type=='T') {
      if (type=='t' || val.length()>=size)
         return val.subString(1, size).leftJustify(size);
      return val.rightJustify(size);
   } // if
   double x(val.asDouble());
   ZBoolean minus(x < 0.0);
   if (minus) x = -x;
   char buf[256];
   switch (type) {
      case 'n':
         if (minus)
            sprintf(buf, "-%0*.*f", size-1, frac, x);
         else
            sprintf(buf, "%0*.*f", size, frac, x);
         break;
      case 'N':
         sprintf(buf, "%0*.*f%c", size-1, frac, x, minus ? '-' : '+');
         break;
      case 'x':
         while (frac--) x *= 10.0;
         if (minus)
           sprintf(buf, "-%0*.0f", size-1, x);
         else
           sprintf(buf, "%0*.0f", size, x);
         break;
      case 'X':
         while (frac--) x *= 10.0;
         sprintf(buf, "%0*.0f%c", size-1, x, minus ? '-' : '+');
         break;
      default:
         *buf = 0;
   } // switch
   return buf;
} // buildAscii

static ZString buildCsv(ZString val, char type, int size, int frac, char hyph)
{
   ZFUNCTRACE_DEVELOP("buildCsv(ZString val, char type, int size, int frac, char hyph)");
   if (type=='t' || type=='T') {
      ZString str;
      if (hyph) str += hyph;
      char *v = val;
      while (*v) {
         if (*v == hyph) str += hyph;
         str += *v++;
      } // while
      if (hyph) str += hyph;
      return str;
   } // if

   double x(val.asDouble());
   ZBoolean minus(x < 0.0);
   if (minus) x = -x;
   char buf[256];
   switch (type) {
      case 'n':
         if (minus)
            sprintf(buf, "-%1.*f", frac, x);
         else
            sprintf(buf, "%1.*f", frac, x);
         break;
      case 'N':
         sprintf(buf, "%1.*f%c", frac, x, minus ? '-' : '+');
         break;
      case 'x':
         while (frac--) x *= 10.0;
         if (minus)
           sprintf(buf, "-%1.0f", x);
         else
           sprintf(buf, "%1.0f", x);
         break;
      case 'X':
         while (frac--) x *= 10.0;
         sprintf(buf, "%1.0f%c", x, minus ? '-' : '+');
         break;
      default:
         *buf = 0;
   } // switch
   return buf;
} // buildCsv

static ZString strBuildRecord(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strBuildRecord(ZCsl* csl)");
   ZString rec;
   ZBoolean nullind(zFalse);
   int argc(csl->get("argCount").asInt());
   if (argc >= 4) nullind = csl->get("nullind").asInt()!=0;
   char mode((csl->get("mode"))[1]);
   int fields(csl->varSizeof("fmts"));
   int v(0);
   for (int f = 0; f < fields; f++) {
      ZString fmt(csl->get("fmts["+ZString(f)+"]"));
      char type(fmt.word(1)[1]);
      int size = fmt.word(2).asInt();
      int frac = fmt.word(3).asInt();
      ZString val;
      if (type=='f' || type=='F') {
         val = fmt.words(3);
         type = (type == 'f') ? 't' : 'T';
      } else {
         val = csl->get("vals["+ZString(v++)+"]");
         if (nullind) {
            if (csl->get("vals["+ZString(v++)+"]").asInt()!=0)
               val = "";
         } // if
      } // if
      switch (mode) {
         case 'a':
         case 'A':
            rec += buildAscii(val, type, size, frac);
            break;
         case 'c':
            if (f) rec += ",";
            rec += buildCsv(val, type, size, frac, '\'');
            break;
         case 'C':
            if (f) rec += ",";
            rec += buildCsv(val, type, size, frac, '"');
            break;
         case 's':
            if (f) rec += ";";
            rec += buildCsv(val, type, size, frac, '\'');
            break;
         case 'S':
            if (f) rec += ";";
            rec += buildCsv(val, type, size, frac, '"');
            break;
         case 't':
            if (f) rec += "\t";
            rec += buildCsv(val, type, size, frac, '\'');
            break;
         case 'T':
            if (f) rec += "\t";
            rec += buildCsv(val, type, size, frac, '"');
            break;
         case 'u':
         case 'U':
            if (f) rec += "\t";
            rec += buildCsv(val, type, size, frac, 0);
            break;
         default:;
      } // switch
   } // for
   return rec;
} // strBuildRecord

static ZString strParseRecord(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("strParseRecord(ZCsl* csl)");
   ZString rec(csl->get("rec"));
   char *r = rec;
   ZBoolean nullind(zFalse);
   int argc(csl->get("argCount").asInt());
   if (argc >= 5) nullind = csl->get("nullind").asInt()!=0;
   char mode((csl->get("mode"))[1]);
   ZTRACE_DEVELOP("mode = "+ZString(mode));
   int fields(csl->varSizeof("fmts"));
   int v(0);
   for (int f = 0; f < fields; f++) {
      ZString fmt(csl->get("fmts["+ZString(f)+"]"));
      char type((fmt.word(1))[1]);
      ZString val;
      switch (mode) {
         case 'a':
         case 'A': {
            int size = fmt.word(2).asInt();
            while (size--) if (*r) val += *r++;
            val.stripTrailing();
            break;
         }
         default: {
            char sep;
            switch (mode) {
               case 'c': case 'C': sep = ','; break;
               case 's': case 'S': sep = ';'; break;
               default: sep = '\t';
            } // switch
            if (mode!='u' && mode!='U') while (*r == ' ') r++;
            if ((*r == '"' || *r == '\'') && mode!='u' && mode!='U') {
               char hyph = *r++;
               next:
               while (*r && *r != hyph) val += *r++;
               if (*r == hyph)
                  if (*++r == hyph) {
                     val += *r++;
                     goto next;
                  } // if
               while (*r && *r != sep) r++;
            } else
               while (*r && *r != sep) val += *r++;
            if (*r == sep) r++;
         } // default
      } // switch
      ZTRACE_DEVELOP("value = <"+val+">");
      switch (type) {
         case 't': case 'T': {
            ZTRACE_DEVELOP("type = t,T");
            csl->set("vals["+ZString(v++)+"]",val);
            if (nullind)
               if (val == "")
                  csl->set("vals["+ZString(v++)+"]", 1);
               else
                  csl->set("vals["+ZString(v++)+"]", 0);
            break;
         }
         case 'n': {
            ZTRACE_DEVELOP("type = n");
            val.strip();
            csl->set("vals["+ZString(v++)+"]",ZString(val.asDouble()));
            if (nullind)
               if (val == "")
                  csl->set("vals["+ZString(v++)+"]", 1);
               else
                  csl->set("vals["+ZString(v++)+"]", 0);
            break;
         }
         case 'N': {
            ZTRACE_DEVELOP("type = N");
            val.strip();
            double x(0.0);
            if (val.length()) {
               x = val.subString(1).asDouble();
               if (val[val.length()-1] == '-') x = -x;
            } // if
            csl->set("vals["+ZString(v++)+"]",ZString(x));
            if (nullind)
               if (val == "")
                  csl->set("vals["+ZString(v++)+"]", 1);
               else
                  csl->set("vals["+ZString(v++)+"]", 0);
            break;
         }
         case 'x': {
            ZTRACE_DEVELOP("type = x");
            val.strip();
            double x = val.asDouble();
            int frac = fmt.word(3).asInt();
            while (frac--) x /= 10.0;
            csl->set("vals["+ZString(v++)+"]",ZString(x));
            if (nullind)
               if (val == "")
                  csl->set("vals["+ZString(v++)+"]", 1);
               else
                  csl->set("vals["+ZString(v++)+"]", 0);
            break;
         }
         case 'X': {
            ZTRACE_DEVELOP("type = X");
            val.strip();
            double x(0.0);
            if (val.length()) {
               x = val.subString(1).asDouble();
               int frac = fmt.word(3).asInt();
               while (frac--) x /= 10.0;
               if (val[val.length()-1] == '-') x = -x;
            } // if
            csl->set("vals["+ZString(v++)+"]",ZString(x));
            if (nullind)
               if (val == "")
                  csl->set("vals["+ZString(v++)+"]", 1);
               else
                  csl->set("vals["+ZString(v++)+"]", 0);
            break;
         }
         default:;
      } // if
   } // for
   return 1;
} // strParseRecord

ZCslInitLib(aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslInitLib(aCsl)");
   ZString iFile("ZcStrLib");
   ZString init(ZString(
      "const strVersion = '4.04';\n"
      "const strCompiler = '")+ZC_COMPILER+"';\n"
      "const strLibtype = '"+ZC_CSLLIBTYPE+"';\n"
      "const strBuilt = '"+ZString(__DATE__)+" "+__TIME__+"';\n"
   );
   istrstream str((char*)init);
   aCsl->loadScript(iFile, &str);
   (*aCsl)
      .addFunc(iFile,
         "strAscii(const character)",
          strAscii)

      .addFunc(iFile,
         "strBuildRecord("
            "const mode, "
            "const &fmts[], "
            "const &vals[], "
            "[const nullind])",
          strBuildRecord)

      .addFunc(iFile,
         "strCenter("
            "const string, "
            "const width, "
            "[const padd])",
          strCenter)

      .addFunc(iFile,
         "strChange("
            "const string, "
            "const oldpatt, "
            "const newpatt, "
            "[const start, "
             "const count, "
             "const ignoreCase])",
          strChange)

      .addFunc(iFile,
         "strChar("
            "const asciival)",
          strChar)

      .addFunc(iFile,
         "strExport("
            "const string)",
          strExport)

      .addFunc(iFile,
         "strFormatNumber("
            "const val, "
            "const width, "
            "[const frac])",
          strFormatNumber)

      .addFunc(iFile,
         "strImport("
            "const string)",
          strImport)

      .addFunc(iFile,
         "strIndexOf("
            "const string, "
            "const pattern, "
            "[const start, "
             "const ignoreCase])",
          strIndexOf)

      .addFunc(iFile,
         "strIsInteger("
            "const string)",
          strIsInteger)

      .addFunc(iFile,
         "strIsNumber("
            "const string)",
          strIsNumber)

      .addFunc(iFile,
         "strIsPrintable("
            "const string)",
          strIsPrintable)

      .addFunc(iFile,
         "strLastIndexOf("
            "const string, "
            "const pattern, "
            "[const start, "
             "const ignoreCase])",
          strLastIndexOf)

      .addFunc(iFile,
         "strLeftJustify("
            "const string, "
            "const width, "
            "[const padd])",
          strLeftJustify)

      .addFunc(iFile,
         "strLength("
            "const string)",
          strLength)

      .addFunc(iFile,
         "strLower("
            "const string)",
          strLower)

      .addFunc(iFile,
         "strParseRecord("
            "const rec, "
            "const mode, "
            "const &fmts[], "
            "var &vals[], "
            "[const nullind])",
          strParseRecord)

      .addFunc(iFile,
         "strRemoveWords("
            "const string, "
            "const start, "
            "[const count])",
          strRemoveWords)

      .addFunc(iFile,
         "strRightJustify("
            "const string, "
            "const width, "
            "[const padd])",
          strRightJustify)

      .addFunc(iFile,
         "strSplitConnectString("
            "const connstr, "
            "[var& password, "
             "var& connection, "
             "var& database])",
          strSplitConnectString)

      .addFunc(iFile,
         "strSplitPath("
            "const path, "
            "[var& ext, "
             "var& dir, "
             "var& drive])",
          strSplitPath)

      .addFunc(iFile,
         "strSpread("
            "const string)",
          strSpread)

      .addFunc(iFile,
         "strStrip("
            "const string, "
            "[const char])",
          strStrip)

      .addFunc(iFile,
         "strStripExtension("
            "const filename)",
          strStripExtension)

      .addFunc(iFile,
         "strStripLeading("
            "const string, "
            "[const char])",
          strStripLeading)

      .addFunc(iFile,
         "strStripTrailing("
            "const string, "
            "[const char])",
          strStripTrailing)

      .addFunc(iFile,
         "strSubString("
            "const string, "
            "const start, "
            "[const count, "
             "const padd])",
          strSubString)

      .addFunc(iFile,
         "strUpper("
            "const string)",
          strUpper)

      .addFunc(iFile,
         "strWordCount("
            "const string)",
          strWordCount)

      .addFunc(iFile,
         "strWords("
            "const string, "
            "const start, "
            "[const count])",
          strWords);
} // ZCslInitLib

ZCslCleanupLib(aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslCleanupLib(aCsl)");
} // ZCslCleanupLib
