/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZcDaxLib.cpp
 * Application :  CSL sql database access library
 * Purpose     :  Implementation
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.30  First implementation                        P.Koch, IBK
 * 2001.07.07  Renaming from css to csl                    P.Koch, IBK
 * 2002.03.22  Added daxCheckCursor                        P.Koch, IBK
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
#include <ZSql.hpp>

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

class Link : public ZBase
{
   public:
      Link(ZSqlLink* aLink, int aMaxCursor);
      Link(const Link& aLink);
      ~Link();

      ZBoolean operator==(const Link& aLink) const;

      int nextCursor();                  // get next free cursor
      ZBoolean checkCursor(int x);       // check if cursor valid

      ZSqlLink*      iLink;              // sql link
      int            iMaxCursor;         // max # of cursors
      ZSqlCursor**   iCsr;               // sql cursors
      ZBoolean*      iExec;              // execution flag
      ZBoolean*      iInUse;             // is in use
      ZString*       iBtyp;              // bind type
      long*          iBcol;              // current bind column
      long*          iRefCount;          // reference count
}; // Link

ZLISTCLASSDECLARE(Linklist, Link)
ZLISTCLASSDECLARE(Longlist, long)

static Linklist* iLinkList = 0;          // db link list
static long iUseCount(0);

//////////////////////////////// Link Implemention /////////////////////////

Link::Link(ZSqlLink* aLink, int aMaxCursor) :
   iLink(aLink),
   iMaxCursor(aMaxCursor),
   iCsr(0),
   iExec(0),
   iInUse(0),
   iBtyp(0),
   iBcol(0),
   iRefCount(0)
{
   ZFUNCTRACE_DEVELOP("Link::Link(ZSqlLink* aLink, int aMaxCursor)");
   ZTRACE_DEVELOP("iMaxCursor="+ZString(iMaxCursor));
   if (iMaxCursor) {
      iCsr = new ZSqlCursor*[aMaxCursor];
      iExec = new ZBoolean[aMaxCursor];
      iInUse = new ZBoolean[aMaxCursor];
      iBtyp = new ZString[aMaxCursor];
      iBcol = new long[aMaxCursor];
      for (int i = 0; i < iMaxCursor; i++)
         iInUse[i] = zFalse;
   } // if
   iRefCount = new long;
   *iRefCount = 1;
} // Link (new)

Link::Link(const Link& aLink) :
   iLink(aLink.iLink),
   iMaxCursor(aLink.iMaxCursor),
   iCsr(aLink.iCsr),
   iExec(aLink.iExec),
   iInUse(aLink.iInUse),
   iBtyp(aLink.iBtyp),
   iBcol(aLink.iBcol),
   iRefCount(aLink.iRefCount)
{
   ZFUNCTRACE_DEVELOP("Link::Link(const Link& aLink)");
   ZTRACE_DEVELOP("iMaxCursor="+ZString(iMaxCursor));
   (*iRefCount)++;
} // Link (copy)

Link::~Link()
{
   ZFUNCTRACE_DEVELOP("Link::~Link()");
   (*iRefCount)--;
   ZTRACE_DEVELOP("*iRefCount="+ZString(*iRefCount));
   if (*iRefCount == 0) {
      ZTRACE_DEVELOP("iLink="+ZString::hex(iLink));
      ZTRACE_DEVELOP("iMaxCursor="+ZString(iMaxCursor));
      if (iLink && iMaxCursor>0) {
         ZTRACE_DEVELOP("rolling back work");
         iLink->rollback();
         delete iLink;
      } // if
      if (iCsr) delete [] iCsr;
      if (iExec) delete [] iExec;
      if (iInUse) delete [] iInUse;
      if (iBtyp) delete [] iBtyp;
      if (iBcol) delete [] iBcol;
      if (iRefCount) delete iRefCount;
   } // if
} // ~Link

ZBoolean Link::operator==(const Link& aLink) const
{
   ZFUNCTRACE_DEVELOP("Link::operator==(const Link& aLink) const");
   return iLink == aLink.iLink;
} // operator==

int Link::nextCursor()
{
   ZFUNCTRACE_DEVELOP("Link::nextCursor()");
   int x;
   for (x = 0; x < iMaxCursor; x++)
      if (!iInUse[x]) {
         iExec[x] = zTrue;
         iInUse[x] = zTrue;
         break;
      } // if
   if (x >= iMaxCursor)
      ZTHROWEXC("All dax cursors in use (apply daxDispose)");
   return x;
} // nextCursor

ZBoolean Link::checkCursor(int x)
{
   ZFUNCTRACE_DEVELOP("Link::checkCursor()");
   if (x < 0 || x >= iMaxCursor) return zTrue;
   if (!iInUse[x]) return zTrue;
   if (iLink->getCursor(&iCsr[x])) {
      iExec[x] = zFalse;
      iInUse[x] = zFalse;
      return zTrue;
   } // if
   return zFalse;
} // checkCursor

///////////////////////////// CSL Function Implementions /////////////////////

static long findLink(ZCsl* csl, int* csr = 0)
{
   ZFUNCTRACE_DEVELOP("findLink(ZCsl* csl)");
   ZString link(csl->get(csr ? "cursor" : "link"));

   if (link.word(1) != "ZSqlLink*")
      ZTHROWEXC("invalid dax link");

   Link lnk((ZSqlLink*)(link.word(2).asUnsignedLong()), 0);
   long pos = iLinkList->find(lnk);
   if (pos < 0)
      ZTHROWEXC("invalid dax link");

   if (csr)
      *csr = link.word(3).asInt();
   return pos;
} // findLink

static Link& getLink(ZCsl* csl, int* csr = 0)
{
   ZFUNCTRACE_DEVELOP("getLink(ZCsl* csl)");
   return (*iLinkList)[findLink(csl, csr)];
} // getLink

static ZSqlLink* getSqlLink(ZCsl* csl, int* csr = 0)
{
   ZFUNCTRACE_DEVELOP("getLink(ZCsl* csl)");
   return getLink(csl, csr).iLink;
} // getSqlLink

static ZString daxDisconnect(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxDisconnect(ZCsl* csl)");
   iLinkList->removeFromPos(findLink(csl));
   return 1;
} // daxDisconnect

static ZString daxCommit(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxCommit(ZCsl* csl)");
   getSqlLink(csl)->commit();
   return 1;
} // daxCommit

static ZString daxRollback(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxRollback(ZCsl* csl)");
   getSqlLink(csl)->rollback();
   return 1;
} // daxRollback

static ZString daxSimple(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxSimple(ZCsl* csl)");
   static ZSqlCursor* csr;
   getSqlLink(csl)->getCursor(&csr);
   ZString sql(csl->get("sql"));
   (*csr)
      .parse(sql.constBuffer())
      .execute();
   ZString ret;
   if (csr->selectColumnCount())
      if (csr->fetch())
         *csr >> ret;
   return ret;
} // daxSimple

static ZString daxCheckCursor(ZCsl* csl)
{
   ZString cursor(csl->get("cursor"));
   if (cursor.wordCount() != 3 || cursor.word(1) != "ZSqlLink*")
      return ZString((int)zTrue);
   int c;
   Link& link = getLink(csl, &c);
   return link.checkCursor(c);
} // daxCheckCursor

static ZString daxParse(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxParse(ZCsl* csl)");
   Link& link = (*iLinkList)[findLink(csl)];
   int c(link.nextCursor());
   (*link.iLink)
      .invCursor(&link.iCsr[c])
      .getCursor(&link.iCsr[c]);
   int argc(csl->get("argCount").asInt());
   ZString sql(csl->get("sql"));
   ZBoolean select(sql.word(1).lowerCase()=="select");
   link.iBtyp[c] = "";
   Longlist bindsize;
   const char* s = sql.constBuffer();
   ZString stmt;
   ZBoolean ora(link.iLink->dataBaseName()=="ORACLE");
   while (*s) {
      while (*s && *s !='#' && *s != '\'') stmt += *s++;
      if (*s == '\'') {
         // skip literals
         stmt += *s++;
         while (*s && *s != '\'') stmt += *s++;
         if (*s == '\'') stmt += *s++;
      } else
         if (*s == '#') {
            s++;
            if (ora)
               stmt += ":p"+ZString(bindsize.count());
            else
               stmt += '?';
            ZBoolean minus(zFalse);
            if (*s == '-') {
               minus = zTrue;
               s++;
            } // if
            int size = 0;
            while ('0'<=*s && *s<='9')
               size = size * 10 + *s++ - '0';
            if (minus) size = -size;
            bindsize.addAsLast(size);
         } // if
   } // while
   long longmax(2000);
   if (argc > 2) longmax = csl->get("longmax").asLong();
   (*link.iCsr[c])
      .parse(
         stmt.constBuffer(),
         select ? 1 : bindsize.count() ? 25 : 1,
         select ? 25 : 1,
         longmax);
   for (int b = 0; b < bindsize.count(); b++) {
      ZString nm;
      if (ora) nm = ":p"+ZString(b); else nm = "?";
      if (bindsize[b] > 0) {
         link.iCsr[c]->bind(nm.constBuffer(), ZSqlCursor::varTypeChar, bindsize[b]);
         link.iBtyp[c] += 'C';
      } else
         if (bindsize[b] < 0) {
            link.iCsr[c]->bind(nm.constBuffer(), ZSqlCursor::varTypeRaw, -bindsize[b]);
            link.iBtyp[c] += 'C';
         } else {
            link.iCsr[c]->bind(nm.constBuffer(), ZSqlCursor::varTypeFloat64);
            link.iBtyp[c] += 'F';
         } // if
   } // for
   link.iBcol[c] = 0;
   return csl->get("link")+" "+ZString(c);
} // daxParse

static ZString daxFetch(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxFetch(ZCsl* csl)");

   ZBoolean nullind(zFalse);
   if (csl->get("argCount").asInt() > 2 && csl->get("nullind").asInt())
      nullind = zTrue;

   int c;
   Link& link = getLink(csl, &c);
   if (link.iExec[c]) {
      link.iCsr[c]->execute();
      link.iExec[c] = zFalse;
   } // if
   ZBoolean ok = link.iCsr[c]->fetch();
   if (ok) {
      int cols = csl->varSizeof("values");
      if (nullind) {
         if (cols & 1)
            ZTHROWEXC("nullindicated fetch requires even size of valuearray");
         for (int b = 0; b < cols; b += 2) {
            ZSqlChar data;
            *link.iCsr[c] >> data;
            csl->set("values["+ZString(b)+"]", data);
            csl->set("values["+ZString(b+1)+"]", data.isNull() ? "1" : "0");
         } // for
      } else
         for (int b = 0; b < cols; b++) {
            ZSqlChar data;
            *link.iCsr[c] >> data;
            csl->set("values["+ZString(b)+"]", data);
         } // for
   } // if
   return ZString(ok);
} // daxFetch

static ZString daxSupply(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxSupply(ZCsl* csl)");
   int c;
   Link& link = getLink(csl, &c);

   if (!link.iBtyp[c].size())
      ZTHROWEXC("no placeholders (#) in sql statement for values supplied");

   ZBoolean nullind(zFalse);
   if (csl->get("argCount").asInt() > 2 && csl->get("nullind").asInt())
      nullind = zTrue;

   int cols = csl->varSizeof("values");
   if (nullind) {
      if (cols & 1)
         ZTHROWEXC("nullindicated supply requires even size of valuearray");
      for (int b = 0; b < cols; b += 2) {
         if (link.iBtyp[c][link.iBcol[c]+1] == 'F') {
            ZSqlFloat64
               data(
                  csl->get("values["+ZString(b)+"]").asDouble(),
                  csl->get("values["+ZString(b+1)+"]").asInt()
                     ? zTrue
                     : zFalse
               );
            *link.iCsr[c] << data;
         } else {
            ZSqlChar
               data(
                  csl->get("values["+ZString(b)+"]"),
                  csl->get("values["+ZString(b+1)+"]").asInt()
                     ? zTrue
                     : zFalse
               );
            *link.iCsr[c] << data;
         } // if
         if (++link.iBcol[c] >= link.iBtyp[c].length())
            link.iBcol[c] = 0;
      } // for
   } else
      for (int b = 0; b < cols; b++) {
         if (link.iBtyp[c][link.iBcol[c]+1] == 'F')
            *link.iCsr[c] << csl->get("values["+ZString(b)+"]").asDouble();
         else
            *link.iCsr[c] << csl->get("values["+ZString(b)+"]");
         if (++link.iBcol[c] >= link.iBtyp[c].length())
            link.iBcol[c] = 0;
      } // for
   link.iExec[c] = zTrue;
   return 1;
} // daxSupply

static ZString daxDone(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxDone(ZCsl* csl)");
   int c;
   Link& link = getLink(csl, &c);
   if (link.iExec[c]) {
      link.iCsr[c]->execute();
      link.iExec[c] = zFalse;
   } // if
   return 1;
} // daxDone

static ZString daxDispose(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxDispose(ZCsl* csl)");
   int c;
   Link& link = getLink(csl, &c);
   if (link.iExec[c]) {
      link.iCsr[c]->execute();
      link.iExec[c] = zFalse;
   } // if
   link.iInUse[c] = zFalse;
   return 1;
} // daxDispose

static ZSqlCursor* execCursor(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxDispose(ZCsl* csl)");
   int c;
   Link& link = getLink(csl, &c);
   if (link.iExec[c]) {
      link.iCsr[c]->execute();
      link.iExec[c] = zFalse;
   } // if
   return link.iCsr[c];
} // execCursor

static ZString daxRowsProcessed(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxRowsProcessed(ZCsl* csl)");
   return execCursor(csl)->rowsProcessed();
} // daxRowsProcessed

static ZString daxSelectColumns(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxSelectColumns(ZCsl* csl)");
   return execCursor(csl)->selectColumnCount();
} // daxSelectColumns

static ZString daxSelectColumnName(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxSelectColumnName(ZCsl* csl)");
   return execCursor(csl)->selectColumnName(csl->get("index").asInt());
} // daxSelectColumnName

static ZString daxSelectColumnSize(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxSelectColumnSize(ZCsl* csl)");
   return execCursor(csl)->selectColumnSize(csl->get("index").asInt());
} // daxSelectColumnSize

static ZString daxSelectColumnType(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxSelectColumnType(ZCsl* csl)");
   return execCursor(csl)->selectColumnDescription(csl->get("index").asInt());
} // daxSelectColumnType

static ZString daxDatabase(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxDatabase(ZCsl* csl)");
   return getSqlLink(csl)->dataBaseName();
} // daxDatabase

static ZString daxConnect(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxConnect(ZCsl* csl)");
   int argc = csl->get("argCount").asInt();
   ZString db, conn, name, pass;
   int maxcursor(16);
   switch (argc) {
      case  5: maxcursor = csl->get("maxcursor").asInt();
               if (maxcursor < 1) maxcursor = 1;
      case  4: pass = csl->get("password");
      case  3: name = csl->get("username");
      case  2: conn = csl->get("connection");
      default: db   = csl->get("database");
   } // if
   db.upperCase();
   ZTRACE_DEVELOP("ZSqlLink::connect");
   ZSqlLink* link =
      ZSqlLink::connect(
         db.constBuffer(),
         conn.constBuffer(),
         name.constBuffer(),
         pass.constBuffer(),
         maxcursor);
   ZTRACE_DEVELOP("ZSqlLink::connect link = "+ZString::hex(link));
   iLinkList->addAsLast(new Link(link, maxcursor));
   return "ZSqlLink* "+ZString((unsigned long)link);
} // daxConnect

static ZString daxLiteral(ZCsl* csl)
{
   ZFUNCTRACE_DEVELOP("daxLiteral(ZCsl* csl)");
   return ZSqlLink::literal(csl->get("string").constBuffer());
} // daxLiteral

ZCslInitLib(aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslInitLib(aCsl)");

   if (iUseCount++ == 0 && !iLinkList)
      iLinkList = new Linklist;

   ZString iFile("ZcDaxLib");
   ZString init(ZString(
      "const daxVersion = '4.04';\n"
      "const daxCompiler = '")+ZC_COMPILER+"';\n"
      "const daxLibtype = '"+ZC_CSLLIBTYPE+"';\n"
      "const daxBuilt = '"+ZString(__DATE__)+" "+__TIME__+"';\n"
   );
   istrstream str((char*)init);
   aCsl->loadScript(iFile, &str);
   (*aCsl)
      .addFunc(iFile,
         "daxCheckCursor("
            "const cursor)",
          daxCheckCursor)

      .addFunc(iFile,
         "daxCommit("
            "const link)",
          daxCommit)

      .addFunc(iFile,
         "daxConnect("
            "const database, "
            "[const connection, "
             "const username, "
             "const password, "
             "const maxcursor])",
          daxConnect)

      .addFunc(iFile,
         "daxDatabase("
            "const link)",
          daxDatabase)

      .addFunc(iFile,
         "daxDisconnect("
            "const link)",
          daxDisconnect)

      .addFunc(iFile,
         "daxDispose("
            "const cursor)",
          daxDispose)

      .addFunc(iFile,
         "daxDone("
            "const cursor)",
          daxDone)

      .addFunc(iFile,
         "daxFetch("
            "const cursor, "
            "var& values[], "
            "[const nullind])",
          daxFetch)

      .addFunc(iFile,
         "daxLiteral("
            "const string)",
          daxLiteral)

      .addFunc(iFile,
         "daxSimple("
            "const link, "
            "const sql)",
          daxSimple)

      .addFunc(iFile,
         "daxParse("
            "const link, "
            "const sql, "
            "[const longmax])",
          daxParse)

      .addFunc(iFile,
         "daxRollback("
            "const link)",
          daxRollback)

      .addFunc(iFile,
         "daxRowsProcessed("
            "const cursor)",
          daxRowsProcessed)

      .addFunc(iFile,
         "daxSelectColumnName("
            "const cursor, "
            "const index)",
          daxSelectColumnName)

      .addFunc(iFile,
         "daxSelectColumnSize("
            "const cursor, "
            "const index)",
          daxSelectColumnSize)

      .addFunc(iFile,
         "daxSelectColumnType("
            "const cursor, "
            "const index)",
          daxSelectColumnType)

      .addFunc(iFile,
         "daxSelectColumns("
            "const cursor)",
          daxSelectColumns)

      .addFunc(iFile,
         "daxSupply("
            "const cursor, "
            "const& values[], "
            "[const nullind])",
          daxSupply);
} // ZCslInitLib

ZCslCleanupLib(aCsl)
{
   ZFUNCTRACE_DEVELOP("ZCslCleanupLib(aCsl)");
   if (--iUseCount == 0 && iLinkList) {
      delete iLinkList;
      iLinkList = 0;
   } // if
} // ZCslCleanupLib
