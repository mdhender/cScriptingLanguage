/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZDb2.cpp
 * Application :  IBK Open Class Library
 * Purpose     :  Database interface for IBM DB2
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.30  First implemention                          P.Koch, IBK
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

#include <ZTrace.hpp>
#include "ZDb2.hpp"

///////////////////////////// Constants And Errors ///////////////////////////

static char *typeName[] = {
   "<varTypeInt8>",
   "<varTypeInt16>",
   "<varTypeInt32>",
   "<varTypeFloat32>",
   "<varTypeFloat64>",
   "<varTypeChar>",
   "<varTypeRaw>"
};

static const char invType[] = "internal error (invalid type-code supplied)";

#define throwSqlError(txt) \
{ \
   ZSqlError exc(txt); \
   ZTHROW(exc); \
} // throwSqlError

static const char selErr1[] = "selected value from column ";
static const char selErr2[] = ") doesn't fit into ";

#define throwSelValSizeError(index) \
{ \
   ZSqlError exc(sql); \
   exc.addAsLast( \
      ZString(selErr1)+sel[selnxtfld-1].name+" ("+ \
      ZString(x)+selErr2+typeName[index]); \
   ZTHROW(exc); \
} // throwSelValSizeError

static const char bndErr1[] = "bind value ";
static const char bndErr2[] = ") does not match to ";

#define throwBindValMatchError(val) \
{ \
   ZSqlError exc(sql); \
   exc.addAsLast( \
      ZString(bndErr1)+col.name+" ("+ \
      val+bndErr2+typeName[col.htype]); \
   ZTHROW(exc); \
} // throwBindValMatchError

static ZSqlError db2Error(
   SQLRETURN      retc,                  // return code from cli call
   const char*    cli,                   // cli function name
   SQLHENV        henv,                  // environment handle
   SQLHDBC        hdbc,                  // database connection handle
   SQLHSTMT       hstmt,                 // statement handle
   const char*    sql)                   // sql statement
{
   ZFUNCTRACE_DEVELOP("ThrowDb2Error(...)");

   ZSqlError exc(ZString("DB2 error in ")+cli+":");\
   if (sql) exc.addAsLast(sql);
   switch (retc) {
      case SQL_SUCCESS_WITH_INFO:
      case SQL_ERROR:
         break;
      case SQL_SUCCESS:
         exc.addAsLast("no error");
         goto done;
      case SQL_NO_DATA_FOUND:
         exc.addAsLast("no data found");
         goto done;
      case SQL_NEED_DATA:
         exc.addAsLast("required data not supplied");
         goto done;
      case SQL_INVALID_HANDLE:
         exc.addAsLast("invalid handle");
         goto done;
      default:
         exc.addAsLast("unsupported cli returncode ("+ZString(retc)+")");
         goto done;
   } // switch

   SQLCHAR        state[SQL_SQLSTATE_SIZE+1];
   SQLCHAR        buf[SQL_MAX_MESSAGE_LENGTH+1];
   SQLINTEGER     code;
   SQLSMALLINT    length;
   ZTRACE_DEVELOP("SQLError");
   while ( SQLError(henv, hdbc, hstmt, state,
                    &code, buf, sizeof(buf), &length) == SQL_SUCCESS ) {
      exc.addAsLast(ZString((char*)state)+": "+(char*)buf);
   } // while
   done:
   return exc;
} // db2Error

#define throwDb2Error(retc,cli,henv,hdbc,hstmt,sql) \
{ \
   errflag = 1; \
   ZTHROW(db2Error(retc,cli,henv,hdbc,hstmt,sql)) \
} // throwDb2Error

#define throwDb2Error1(retc,cli,henv,hdbc) \
   ZTHROW(db2Error(retc,cli,henv,hdbc,SQL_NULL_HSTMT,NULL))

#define throwDb2Error2(retc,cli,henv) \
   ZTHROW(db2Error(retc,cli,henv,SQL_NULL_HDBC,SQL_NULL_HSTMT,NULL))

static ZBoolean isError(SQLRETURN retc)
{
   ZFUNCTRACE_DEVELOP("isError(SQLRETURN retc)");
   return (   retc!=SQL_SUCCESS
           && retc!=SQL_SUCCESS_WITH_INFO
           && retc!=SQL_NO_DATA_FOUND);
} // isError

////////////////////////////////// ZDb2Link //////////////////////////////////

ZDb2Link::ZDb2Link(
   const char* aConnection,
   const char* aUsername,
   const char* aPassword,
   int   aMaxCursor) :
   ZSqlLink(aMaxCursor)
{
   ZFUNCTRACE_DEVELOP("ZDb2Link::ZDb2Link(...)");

   ZTRACE_DEVELOP("SQLAllocEnv");
   if (isError(SQLAllocEnv(&henv)))
      throwSqlError("SQLAllocEnv failed.");

   ZTRACE_DEVELOP("SQLAllocConnect");
   SQLRETURN ret = SQLAllocConnect(henv, &hdbc);
   if (isError(ret))
      throwDb2Error2(ret, "SQLAllocConnect", henv);

   ZTRACE_DEVELOP("SQLSetConnectOption-autocommitt off");
   ret = SQLSetConnectOption(hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
   if (isError(ret))
      throwDb2Error1(ret, "SQLSetConnectOption-autocommit off", henv, hdbc);

   ZTRACE_DEVELOP("SQLConnect");
   ret = SQLConnect(hdbc, (SQLCHAR*)aConnection, SQL_NTS,
                          (SQLCHAR*)aUsername, SQL_NTS,
                          (SQLCHAR*)aPassword, SQL_NTS);
   if (isError(ret))
      throwDb2Error1(ret, "SQLConnect", henv, hdbc);
} // ZDb2Link

ZDb2Link::~ZDb2Link()
{
   ZFUNCTRACE_DEVELOP("ZDb2Link::~ZDb2Link()");

   // release all cursors before logoff
   for (int i = 0; i < iCsrCount; i++) delete iCsrPool[i];
   iCsrCount = 0;

   // now we can retire...
   ZTRACE_DEVELOP("SQLDisconnect");
   SQLRETURN ret = SQLDisconnect(hdbc);
   if (isError(ret))
      throwDb2Error1(ret, "SQLDisconnect", henv, hdbc);

   ZTRACE_DEVELOP("SQLFreeConnect");
   ret = SQLFreeConnect(hdbc);
   if (isError(ret))
      throwDb2Error1(ret, "SQLFreeConnect", henv, hdbc);

   ZTRACE_DEVELOP("SQLFreeEnv");
   ret = SQLFreeEnv(henv);
   if (isError(ret))
      throwDb2Error2(ret, "SQLFreeEnv", henv);
} // ~ZDb2Link

ZString ZDb2Link::dataBaseName() const
{
   ZFUNCTRACE_DEVELOP("ZDb2Link::dataBaseName() const");
   return "DB2";
} // dataBaseName

ZSqlCursor* ZDb2Link::allocCursor()
{
   ZFUNCTRACE_DEVELOP("ZDb2Link::allocCursor()");

   ZDb2Cursor *csr = new ZDb2Cursor(henv, hdbc);

   ZTRACE_DEVELOP("SQLAllocStmt");
   SQLRETURN ret = SQLAllocStmt(hdbc, &csr->hstmt);
   if (isError(ret)) {
      delete csr;
      throwDb2Error1(ret, "SQLAllocStmt", henv, hdbc);
   } // if
   return csr;
} // allocCursor

ZSqlLink& ZDb2Link::commit()
{
   ZFUNCTRACE_DEVELOP("ZDb2Link::commit()");

   ZTRACE_DEVELOP("SQLTransact-COMMIT");
   SQLRETURN ret = SQLTransact(henv, hdbc, SQL_COMMIT);
   if (isError(ret))
      throwDb2Error1(ret, "SQLTransact-COMMIT", henv, hdbc);
   return *this;
} // commit

ZSqlLink& ZDb2Link::rollback()
{
   ZFUNCTRACE_DEVELOP("ZDb2Link::rollback()");

   ZTRACE_DEVELOP("SQLTransact-ROLLBACK");
   SQLRETURN ret = SQLTransact(henv, hdbc, SQL_ROLLBACK);
   if (isError(ret))
      throwDb2Error1(ret, "SQLTransact-ROLLBACK", henv, hdbc);
   return *this;
} // rollback

///////////////////////////////// ZDb2Cursor /////////////////////////////////

ZDb2Cursor::ZDb2Cursor(SQLHENV he, SQLHDBC hd) :
   henv(he),
   hdbc(hd)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::ZDb2Cursor(SQLHENV he, SQLHDBC hd)");
} // ZDb2Cursor

ZDb2Cursor::~ZDb2Cursor()
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::~ZDb2Cursor()");
   ZTRACE_DEVELOP("SQLFreeStmt-DROP");
   SQLRETURN ret = SQLFreeStmt(hstmt, SQL_DROP);
   if (isError(ret))
      throwDb2Error(ret, "SQLFreeStmt-DROP", henv, hdbc, hstmt, NULL);
} // ~ZDb2Cursor

int ZDb2Cursor::newParseRequired()
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::newParseRequired()");

   if (sql.size() || errflag) {
      ZTRACE_DEVELOP("SQLFreeStmt-CLOSE");
      SQLRETURN ret = SQLFreeStmt(hstmt, SQL_CLOSE);
      if (isError(ret))
         throwDb2Error(ret, "SQLFreeStmt-CLOSE", henv, hdbc, hstmt, NULL);
   } // if

   // if DB2 error occured on this cursor, force new parse
   return errflag;
} // newParseRequired

ZSqlCursor& ZDb2Cursor::parse(
   const char*     stmt,           // sql statement
   short int       bndarrsize,     // bind array size
   short int       selarrsize,     // select array size
   long int        longsize )      // max.select size for long types
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::parse(...)");

   errflag = 0;

   ZTRACE_DEVELOP("SQLPrepare");
   sql = stmt;
   SQLRETURN ret = SQLPrepare(hstmt, (unsigned char*)stmt, SQL_NTS);
   if (isError(ret))
      throwDb2Error(ret, "SQLPrepare", henv, hdbc, hstmt, sql.constBuffer());

   selarr = selarrsize;
   bndarr = bndarrsize;
   selLongSize = longsize<1 ? 2000 : longsize;
   ZTRACE_DEVELOP( "Stmt = "+sql );

   // clear bind variable list for subsequent bind-calls
   bnd.drop();
   bndnxt = bndcnt = 0;
   bndnxtfld = 0;

   // clear select item list for subsequent define-calls
   sel.drop();
   selnxt = 0;
   selnxtfld = 9999;
   return *this;
} // parse

ZSqlCursor& ZDb2Cursor::define(
    const char*     name,           // bind variable name
    VarType         htype,          // variable type
    long int        hsize )         // max. size of text-types
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::define(...)");
   ZTRACE_DEVELOP( "name="+ZString(name)+
                 "  type="+ZString(htype)+
                 "  size="+ZString(hsize) );

   sel.addAsLast(new Column(name,htype,hsize,selarr));
   Column &col = sel[sel.count()-1];

   ZTRACE_DEVELOP("SQLBindCol");
   SQLRETURN ret = SQLBindCol(hstmt, sel.count(), col.etype, col.buffer, col.bufsiz, col.ind );
   if (isError(ret))
      throwDb2Error(ret, "SQLBindCol", henv, hdbc, hstmt, sql.constBuffer());
   return *this;
} // define

ZSqlCursor& ZDb2Cursor::bind(
    const char*     name,           // bind variable name
    VarType         htype,          // variable type
    long int        hsize )         // max. size of text-types / scale of float types
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::bind(...)");
   ZTRACE_DEVELOP( "name="+ZString(name)+
                 "  type="+ZString(htype)+
                 "  size="+ZString(hsize) );

   bnd.addAsLast(new Column(name,htype,hsize,bndarr));
   Column &col = bnd[bnd.count()-1];

   ZTRACE_DEVELOP("SQLBindParameter - column="+ZString(bnd.count()));
   SQLRETURN ret = SQLBindParameter( hstmt,
                                     bnd.count(), SQL_PARAM_INPUT, col.etype, col.itype,
                                     col.hsize, col.hscale, col.buffer, col.bufsiz,
                                     col.ind );
   if (isError(ret))
      throwDb2Error(ret, "SQLBindParameter", henv, hdbc, hstmt, sql.constBuffer());
   return *this;
} // bind

ZDb2Cursor::Column& ZDb2Cursor::beginBind()
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::beginBind()");
   if (!bnd.count())
      throwSqlError("no bind columns defined");

   if (bndnxt>=bndarr) {
      if (bndnxt != bndcnt) {
         ZTRACE_DEVELOP("SQLParamOptions - param.cnt="+ZString(bndnxt));
         SQLRETURN ret = SQLParamOptions(hstmt, bndcnt = bndnxt, NULL);
         if (isError(ret))
            throwDb2Error(ret, "SQLParamOptions", henv, hdbc, hstmt, sql.constBuffer());
      } // if
      ZTRACE_DEVELOP("SQLExecute");
      SQLRETURN ret = SQLExecute(hstmt);
      if (isError(ret))
         throwDb2Error(ret, "SQLExecute", henv, hdbc, hstmt, sql.constBuffer());
      bndnxt = 0;
      bndnxtfld = 0;
   } // if
   ZTRACE_DEVELOP("bndnxt="+ZString(bndnxt)+"  bndnxtfld="+ZString(bndnxtfld));
   return bnd[bndnxtfld];
} // beginBind

void ZDb2Cursor::bindValue(const ZSqlInt32& val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::bindValue(const ZSqlInt32& val)");
   ZTRACE_DEVELOP("value="+ZString(val));
   Column &col = beginBind();

   // verify compatibility with column
   switch (col.htype) {
      case varTypeInt8:
         ((signed char *)col.buffer)[bndnxt] = val;
         if (((signed char *)col.buffer)[bndnxt] != val) goto noMatch;
         break;
      case varTypeInt16:
         ((short *)col.buffer)[bndnxt] = val;
         if (((short *)col.buffer)[bndnxt] != val) goto noMatch;
         break;
      case varTypeInt32:
         ((long *)col.buffer)[bndnxt] = val;
         break;
      case varTypeFloat32:
         ((float *)col.buffer)[bndnxt] = val;
         break;
      case varTypeFloat64:
         ((double *)col.buffer)[bndnxt] = val;
         break;
      case varTypeChar:
      case varTypeRaw:
         noMatch:
         throwBindValMatchError(ZString(val));
      default:
         throwSqlError(invType);
   } // switch
   col.ind[bndnxt] = val.isNull() ? SQL_NULL_DATA : col.bufsiz;
   if (++bndnxtfld>=bnd.count()) {
      bndnxt++;
      bndnxtfld = 0;
   } // if
} // bindValue(ZSqlInt32)

void ZDb2Cursor::bindValue(const ZSqlInt16& val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::bindValue(const ZSqlInt16& val)");
   bindValue(ZSqlInt32(val,val.isNull()));
} // bindValue(ZSqlInt16)

void ZDb2Cursor::bindValue(const ZSqlInt8& val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::bindValue(const ZSqlInt8& val)");
   bindValue(ZSqlInt32(val,val.isNull()));
} // bindValue(ZSqlInt8)

void ZDb2Cursor::bindValue(const ZSqlFloat64& val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::bindValue(const ZSqlFloat64& val)");
   Column &col = beginBind();

   // verify compatibility with column
   switch (col.htype) {
      case varTypeInt8:
         ((signed char *)col.buffer)[bndnxt] = val;
         if (((signed char *)col.buffer)[bndnxt] != val) goto noMatch;
         break;
      case varTypeInt16:
         ((short *)col.buffer)[bndnxt] = val;
         if (((short *)col.buffer)[bndnxt] != val) goto noMatch;
         break;
      case varTypeInt32:
         ((long *)col.buffer)[bndnxt] = val;
         if (((long *)col.buffer)[bndnxt] != val) goto noMatch;
         break;
      case varTypeFloat32:
         ((float *)col.buffer)[bndnxt] = val;
         if (((float *)col.buffer)[bndnxt] != val) goto noMatch;
         break;
      case varTypeFloat64:
         ((double *)col.buffer)[bndnxt] = val;
         break;
      case varTypeChar:
      case varTypeRaw:
         noMatch:
         throwBindValMatchError(ZString(val));
      default:
         throwSqlError(invType);
   } // switch
   col.ind[bndnxt] = val.isNull() ? SQL_NULL_DATA : col.bufsiz;
   if (++bndnxtfld>=bnd.count()) {
      bndnxt++;
      bndnxtfld = 0;
   } // if
} // bindValue(ZSqlFloat64)

void ZDb2Cursor::bindValue(const ZSqlFloat32& val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::bindValue(const ZSqlFloat32& val)");
   bindValue(ZSqlFloat64(val,val.isNull()));
} // bindValue(ZSqlFloat64)

void ZDb2Cursor::bindValue(const ZSqlChar& val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::bindValue(const ZSqlChar& val)");
   Column &col = beginBind();
   int length(((const ZString&)val).length());

   // verify compatibility with column
   switch (col.htype) {
      case varTypeInt8:
      case varTypeInt16:
      case varTypeInt32:
      case varTypeFloat32:
      case varTypeFloat64:
         throwBindValMatchError((const ZString&)val);
      case varTypeChar:
      case varTypeRaw:
         if (length>col.hsize) throwBindValMatchError((const ZString&)val);
         memcpy(&col.buffer[bndnxt*col.bufsiz],(const char *)val,length);
         break;
      default:
         throwSqlError(invType);
   } // switch
   col.ind[bndnxt] = val.isNull() ? SQL_NULL_DATA : length;
   if (++bndnxtfld>=bnd.count()) {
      bndnxt++;
      bndnxtfld = 0;
   } // if
} // bindValue(ZSqlChar)

ZSqlCursor& ZDb2Cursor::execute()
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::execute()");

   if (!sel.count()) {
      SQLSMALLINT numCols;
      ZTRACE_DEVELOP("SQLNumResultCols");
      SQLRETURN ret = SQLNumResultCols(hstmt, &numCols);
      if (isError(ret))
         throwDb2Error(ret, "SQLNumResultCols", henv, hdbc, hstmt, sql.constBuffer());
      if (numCols > 0) {
         // auto-define select columns
         for (SQLSMALLINT pos = 1; pos <= numCols; pos++) {
            // retrieve column info to next column
            SQLINTEGER  dsize;
            SQLCHAR     cbuf[80];
            SQLUINTEGER prec;
            SQLSMALLINT cbufl, type, scale;

            ZTRACE_DEVELOP("SQLDescribeCol");
            ret = SQLDescribeCol(hstmt, pos, cbuf, sizeof(cbuf), &cbufl, &type, &prec, &scale, NULL);
            if (isError(ret))
               throwDb2Error(ret, "SQLDescribeCol", henv, hdbc, hstmt, sql.constBuffer());

            ZTRACE_DEVELOP("SQLColAttributes-DISPLAY SIZE");
            ret = SQLColAttributes(hstmt, pos, SQL_COLUMN_DISPLAY_SIZE, NULL, 0, NULL, &dsize);
            if (isError(ret))
               throwDb2Error(ret, "SQLColAttributes", henv, hdbc, hstmt, sql.constBuffer());

            VarType htype(varTypeInt8);  // c++ type code
            switch (type) {
               case SQL_DECIMAL:
               case SQL_NUMERIC: {
                  int pr = prec ? prec : 38;
                  if (!scale && pr<3)   htype = varTypeInt8;    else
                  if (!scale && pr<5)   htype = varTypeInt16;   else
                  if (!scale && pr<10)  htype = varTypeInt32;   else
//                if (0<prec && pr<7)   htype = varTypeFloat32; else
                                        htype = varTypeFloat64;
                  break;
               }
               case SQL_BINARY:
               case SQL_VARBINARY:
               case SQL_LONGVARBINARY:
               case SQL_BLOB:
               case SQL_BLOB_LOCATOR:
               case SQL_GRAPHIC:
               case SQL_VARGRAPHIC:
               case SQL_LONGVARGRAPHIC:
                  htype = varTypeRaw;
                  if (dsize > selLongSize) dsize = selLongSize;
                  break;
               case SQL_CHAR:
               case SQL_VARCHAR:
               case SQL_LONGVARCHAR:
               case SQL_CLOB:
               case SQL_CLOB_LOCATOR:
               case SQL_DBCLOB:
               case SQL_DBCLOB_LOCATOR:
               case SQL_DATE:
               case SQL_TIME:
               case SQL_TIMESTAMP:
                  htype = varTypeChar;
                  if (dsize > selLongSize) dsize = selLongSize;
                  break;
               case SQL_SMALLINT:
                  htype = varTypeInt16;
                  break;
               case SQL_INTEGER:
                  htype = varTypeInt32;
                  break;
               case SQL_FLOAT:
                  htype = varTypeFloat32;
                  dsize = scale;
                  break;
               case SQL_REAL:
               case SQL_DOUBLE:
                  htype = varTypeFloat64;
                  dsize = scale;
                  break;
               default:;
            } // switch
            define(ZString(cbuf,cbufl).constBuffer(),htype,dsize);
            Column &col = sel[sel.count()-1];
            col.descrInfo(type,dsize,prec,scale);
         } // for
      } // if
   } // if
   if ( bndnxtfld )
      throwSqlError("not all bind vars supplied");

   if (!bndnxt) bndnxt = 1;
   if (bndnxt != bndcnt) {
      ZTRACE_DEVELOP("SQLParamOptions");
      SQLRETURN ret = SQLParamOptions(hstmt, bndcnt = bndnxt, NULL);
      if (isError(ret))
         throwDb2Error(ret, "SQLParamOptions", henv, hdbc, hstmt, sql.constBuffer());
   } /* if */
   ZTRACE_DEVELOP("SQLExecute");
   SQLRETURN ret = SQLExecute(hstmt);
   if (isError(ret))
      throwDb2Error(ret, "SQLExecute", henv, hdbc, hstmt, sql.constBuffer());
   selcnt = selnxt = selarr;
   selRowsProcessed = 0L;
   bndnxt = 0;
   return *this;
} // execute

int ZDb2Cursor::fetch()
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::fetch()");
   if ( !sel.count() )
      throwSqlError("no select vars available");

   if (++selnxt>=selarr) {
      // do we have to fetch more rows ?

      ZTRACE_DEVELOP("SQLSetStmtOption");
      SQLRETURN ret = SQLSetStmtOption(hstmt, SQL_ROWSET_SIZE, selarr);
      if (isError(ret))
         throwDb2Error(ret, "SQLSetStmtOption", henv, hdbc, hstmt, sql.constBuffer());

      ZTRACE_DEVELOP("SQLExtendedFetch");
      ret = SQLExtendedFetch(hstmt, SQL_FETCH_NEXT, 0, &selcnt, NULL);

      if (ret == SQL_NO_DATA_FOUND)
         selcnt = 0;
      else
         if (isError(ret))
            throwDb2Error(ret, "SQLExtendedFetch", henv, hdbc, hstmt, sql.constBuffer());

      selRowsProcessed += selcnt;
      selnxt = 0;
   } // if
   if (selnxt < selcnt) selnxtfld = 0;
   return selnxt < selcnt ? 1 : 0;
} // fetch

ZDb2Cursor::Column& ZDb2Cursor::beginFetch()
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::beginFetch()");
   if (selnxtfld >= sel.count())
      throwSqlError("no more colums in this row");
   return sel[selnxtfld++];
} // beginFetch

void ZDb2Cursor::fetchValue(ZSqlInt32 &val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::fetchValue(ZSqlInt32 &val)");
   Column     &col = beginFetch();
   if (col.ind[selnxt] == SQL_NULL_DATA)
      val.setNull();
   else
   switch (col.htype) {
      case varTypeInt8:
         val = (signed char)col.buffer[selnxt];
         break;
      case varTypeInt16:
         val = ((short int *)col.buffer)[selnxt];
         break;
      case varTypeInt32:
         val = ((long int *)col.buffer)[selnxt];
         break;
      case varTypeFloat32:
         val = ((float *)col.buffer)[selnxt];
         break;
      case varTypeFloat64:
         val = ((double *)col.buffer)[selnxt];
         break;
      case varTypeChar:
      case varTypeRaw: {
         ZString buf(&((char*)col.buffer)[selnxt*col.bufsiz], col.ind[selnxt]);
         val = atol(buf.constBuffer());
         break;
      }
      default:
         throwSqlError(invType);
   } // switch
} // fetchValue(ZSqlInt32&)

void ZDb2Cursor::fetchValue(ZSqlInt16 &val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::fetchValue(ZSqlInt16 &val)");
   ZSqlInt32   x;
   fetchValue(x);
   val = ZSqlInt16(x, x.isNull());
   if (x != val) throwSelValSizeError(1);
} // fetchValue(ZSqlInt16&)

void ZDb2Cursor::fetchValue(ZSqlInt8 &val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::fetchValue(ZSqlInt8 &val)");
   ZSqlInt32   x;
   fetchValue(x);
   val = ZSqlInt8(x, x.isNull());
   if (x != val) throwSelValSizeError(0);
} // fetchValue(ZSqlInt8&)

void ZDb2Cursor::fetchValue(ZSqlFloat64 &val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::fetchValue(ZSqlFloat64 &val)");
   Column     &col = beginFetch();
   if (col.ind[selnxt] == SQL_NULL_DATA)
      val.setNull();
   else
   switch (col.htype) {
      case varTypeInt8:
         val = (signed char)col.buffer[selnxt];
         break;
      case varTypeInt16:
         val = ((short int *)col.buffer)[selnxt];
         break;
      case varTypeInt32:
         val = ((long int *)col.buffer)[selnxt];
         break;
      case varTypeFloat32:
         val = ((float *)col.buffer)[selnxt];
         break;
      case varTypeFloat64:
         val = ((double *)col.buffer)[selnxt];
         break;
      case varTypeChar:
      case varTypeRaw: {
         ZString buf(&((char*)col.buffer)[selnxt*col.bufsiz], col.ind[selnxt] );
         val = atof(buf.constBuffer());
         break;
      }
      default:
         throwSqlError(invType);
   } // switch
} // fetchValue(ZSqlFloat64&)

void ZDb2Cursor::fetchValue(ZSqlFloat32 &val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::fetchValue(ZSqlFloat32 &val)");
   ZSqlFloat64   x;
   fetchValue(x);
   val = ZSqlFloat32(x, x.isNull());
   if (x != val) throwSelValSizeError(3);
} // fetchValue(ZSqlFloat32&)

void ZDb2Cursor::fetchValue(ZSqlChar &val)
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::fetchValue(ZSqlChar &val)");
   Column &col = beginFetch();
   if (col.ind[selnxt] == SQL_NULL_DATA)
      val.setNull();
   else
   switch (col.htype) {
      case varTypeInt8:
         val = ZString((int)((signed char *)col.buffer)[selnxt]);
         break;
      case varTypeInt16:
         val = ZString(((short int *)col.buffer)[selnxt]);
         break;
      case varTypeInt32:
         val = ZString(((long int *)col.buffer)[selnxt]);
         break;
      case varTypeFloat32:
         val = ZString(((float *)col.buffer)[selnxt]);
         break;
      case varTypeFloat64:
         val = ZString(((double *)col.buffer)[selnxt]);
         break;
      case varTypeChar:
      case varTypeRaw:
         val = ZString(&((char*)col.buffer)[selnxt*col.bufsiz], col.ind[selnxt] );
         break;
      default:
         throwSqlError(invType);
   } // switch
} // fetchValue(ZString&)

unsigned long int ZDb2Cursor::rowsProcessed()
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::rowsProcessed()");
   if (selRowsProcessed) return selRowsProcessed;
   ZTRACE_DEVELOP("SQLRowCount");
   SQLINTEGER  cnt;
   SQLRETURN ret = SQLRowCount(hstmt, &cnt);
   if (isError(ret))
      throwDb2Error(ret, "SQLRowCount", henv, hdbc, hstmt, sql.constBuffer());
   return cnt == -1L ? 0 : cnt;
} // rowsProcessed

int ZDb2Cursor::selectColumnCount()
{
    ZFUNCTRACE_DEVELOP("ZDb2Cursor::selectColumnCount()");
    return sel.count();
} // selectColumnCount

char* ZDb2Cursor::selectColumnName(int index)
{
    ZFUNCTRACE_DEVELOP("ZDb2Cursor::selectColumnName(int index)");
    return sel[index].name;
} // selectColumnName

char* ZDb2Cursor::selectColumnDescription(int index)
{
    ZFUNCTRACE_DEVELOP("ZDb2Cursor::selectColumnDescription(int index)");
    return sel[index].descr;
} // selectColumnDescription

ZSqlCursor::VarType ZDb2Cursor::selectColumnType(int index)
{
    ZFUNCTRACE_DEVELOP("ZDb2Cursor::selectColumnType(int index)");
    return sel[index].htype;
} // selectColumnType

int ZDb2Cursor::selectColumnSize(int index)
{
    ZFUNCTRACE_DEVELOP("ZDb2Cursor::selectColumnSize(int index)");
    return sel[index].hsize;
} // selectColumnSize

int ZDb2Cursor::selectColumnScale(int index)
{
    ZFUNCTRACE_DEVELOP("ZDb2Cursor::selectColumnScale(int index)");
    return sel[index].hscale;
} // selectColumnScale

////////////////////////////// ZDb2Cursor::Column ////////////////////////////

ZDb2Cursor::Column::Column(
    const char* _name,          // column name
    VarType     _htype,         // c++ type code
    long int    _hsize,         // size for Char & Raw
    short int   _arrsize )      // arraysize
{
    ZFUNCTRACE_DEVELOP("ZDb2Cursor::Column::Column(...)");
    name  = _name;
    hsize = _hsize;
    arrsize = _arrsize;
    hscale = 0;
    switch (htype = _htype) {
        case varTypeInt8:
            bufsiz  = sizeof(signed char);
            etype   = SQL_C_TINYINT;
            itype   = SQL_INTEGER;
            break;
        case varTypeInt16:
            bufsiz  = sizeof(short int);
            etype   = SQL_C_SHORT;
            itype   = SQL_INTEGER;
            break;
        case varTypeInt32:
            bufsiz  = sizeof(long int);
            etype   = SQL_C_LONG;
            itype   = SQL_INTEGER;
            break;
        case varTypeFloat32:
            bufsiz  = sizeof(float);
            etype   = SQL_C_FLOAT;
            itype   = SQL_FLOAT;
            break;
        case varTypeFloat64:
            bufsiz  = sizeof(double);
            etype   = SQL_C_DOUBLE;
            itype   = SQL_DOUBLE;
            break;
        case varTypeChar:
            if (_hsize < 1) _hsize = 2000;
            bufsiz  = _hsize+1;
            etype   = SQL_C_CHAR;
            if (_hsize > 32700)
               itype = SQL_CLOB;
            else
               if (_hsize > 4000)
                  itype = SQL_LONGVARCHAR;
               else
                  itype = SQL_VARCHAR;
            break;
        case varTypeRaw:
            if (_hsize < 1) _hsize = 2000;
            bufsiz  = _hsize+1;
            etype   = SQL_C_BINARY;
            if (_hsize > 32700)
               itype = SQL_BLOB;
            else
               if (_hsize > 4000)
                  itype = SQL_LONGVARBINARY;
               else
                  itype = SQL_VARBINARY;
            break;
        default:
            throwSqlError("bind/define: unknown VarType");
    } // switch
    if (double(bufsiz)*arrsize > maxLong)
        throwSqlError("bind/define: array buffer size exceeds INT_MAX");
    buffer = new unsigned char[bufsiz*arrsize];
    ind    = new SQLINTEGER[arrsize];
} // Column

ZDb2Cursor::Column::Column(const Column& aColumn) :
   name(aColumn.name),
   descr(aColumn.descr),
   bufsiz(aColumn.bufsiz),
   hsize(aColumn.hsize),
   hscale(aColumn.hscale),
   arrsize(aColumn.arrsize),
   htype(aColumn.htype),
   etype(aColumn.etype),
   itype(aColumn.itype)
{
   buffer = new unsigned char[bufsiz*arrsize];
   ind    = new SQLINTEGER[arrsize];
   memcpy(buffer, aColumn.buffer, bufsiz*arrsize);
   memcpy(ind, aColumn.ind, sizeof(SQLINTEGER)*arrsize);
} // copy constructor

ZDb2Cursor::Column::~Column()
{
    ZFUNCTRACE_DEVELOP("ZDb2Cursor::Column::~Column()");
    delete [] buffer;
    delete [] ind;
} // ~Column

ZBoolean ZDb2Cursor::Column::operator==(const Column& aColumn) const
{
   return zTrue; // Columnlist::find() not used
} // operator==

void ZDb2Cursor::Column::descrInfo(
    short int       type,           // DB2 internal type code
    long int        size,           // displayed size
    short int       prec,           // precision for numerics
    short int       scale )         // scale for numerics
{
   ZFUNCTRACE_DEVELOP("ZDb2Cursor::Column::descrInfo(...)");
   hscale = 0;
   switch (type) {
      case SQL_DECIMAL:
         descr = "DECIMAL";
         if (prec) {
            descr += "("+ZString(prec);
            if (scale)
               descr += ","+ZString(scale);
            descr += ")";
         } // if
         hsize = prec;
         hscale = scale;
         break;
      case SQL_NUMERIC:
         descr = "NUMERIC";
         if (prec) {
            descr += "("+ZString(prec);
            if (scale)
               descr += ","+ZString(scale);
            descr += ")";
         } // if
         hsize = prec;
         hscale = scale;
         break;
      case SQL_SMALLINT:
         descr = "SMALLINT";
         hsize = prec;
         hscale = scale;
         break;
      case SQL_INTEGER:
         descr = "INTEGER";
         hsize = prec;
         hscale = scale;
         break;
      case SQL_FLOAT:
         descr = "FLOAT";
         hsize = prec;
         hscale = scale;
         break;
      case SQL_REAL:
         descr = "REAL";
         hsize = prec;
         hscale = scale;
         break;
      case SQL_DOUBLE:
         descr = "DOUBLE";
         hsize = prec;
         hscale = scale;
         break;
      case SQL_BINARY:
         descr = "BINARY("+ZString(size)+")";
         break;
      case SQL_VARBINARY:
         descr = "VARBINARY("+ZString(size)+")";
         break;
      case SQL_LONGVARBINARY:
         descr = "LONGVARBINARY("+ZString(size)+")";
         break;
      case SQL_BLOB:
         descr = "BLOB("+ZString(size)+")";
         break;
      case SQL_BLOB_LOCATOR:
         descr = "BLOB_LOCATOR("+ZString(size)+")";
         break;
      case SQL_GRAPHIC:
         descr = "GRAPHIC("+ZString(size)+")";
         break;
      case SQL_VARGRAPHIC:
         descr = "VARGRAPHIC("+ZString(size)+")";
         break;
      case SQL_LONGVARGRAPHIC:
         descr = "LONGVARGRAPHIC("+ZString(size)+")";
         break;
      case SQL_CHAR:
         descr = "CHAR("+ZString(size)+")";
         break;
      case SQL_VARCHAR:
         descr = "VARCHAR("+ZString(size)+")";
         break;
      case SQL_LONGVARCHAR:
         descr = "LONGVARCHAR("+ZString(size)+")";
         break;
      case SQL_CLOB:
         descr = "CLOB("+ZString(size)+")";
         break;
      case SQL_CLOB_LOCATOR:
         descr = "CLOB_LOCATOR("+ZString(size)+")";
         break;
      case SQL_DBCLOB:
         descr = "DBCLOB("+ZString(size)+")";
         break;
      case SQL_DBCLOB_LOCATOR:
         descr = "DBCLOB_LOCATOR("+ZString(size)+")";
         break;
      case SQL_DATE:
         descr = "DATE";
         break;
      case SQL_TIME:
         descr = "TIME";
         break;
      case SQL_TIMESTAMP:
         descr = "TIMESTAMP";
         break;
      default:
         throwSqlError("unknown DB2 internal datatype");
   } // switch
} // descrInfo

ZExport(ZSqlLink*) ZSqlConnectTo(
   const char* aConnection,
   const char* aUsername,
   const char* aPassword,
   int aMaxCursor)
{
   ZFUNCTRACE_DEVELOP("ZSqlConnectTo(...)");
   return new ZDb2Link(aConnection, aUsername, aPassword, aMaxCursor);
} // ZSqlConnectTo
