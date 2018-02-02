/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZMysql.cpp
 * Application :  IBK Open Class Library
 * Purpose     :  Database interface for MySQL
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.06.11  First implementation                        P.Koch, IBK
 * 2001.07.01  Fixed a bug in dropResult() wich caused a   P.Koch, IBK
 *             GPF at 2nd parse.
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

#include <stdlib.h>
#include <string.h>
#include <ZTrace.hpp>
#include "ZMysql.hpp"

static char *typeName[] = {
    "<varTypeInt8>",
    "<varTypeInt16>",
    "<varTypeInt32>",
    "<varTypeFloat32>",
    "<varTypeFloat64>",
    "<varTypeChar>",
    "<varTypeRaw>"
};

#define throwSqlError(aMessage) \
{ \
   ZSqlError exc(aMessage); \
   ZTHROW(exc); \
} // throwSqlError

#define throwMysqlError(aHandle, aFunction) \
{ \
   ZSqlError exc(ZString(aFunction)+": "+mysql_error(aHandle)); \
   ZTHROW(exc); \
} // throwMysqlError

// select vaue mismatches

static const char selErr1[] = "selected value from column ";
static const char selErr2[] = ") doesn't fit into ";

#define throwSelValSizeError(index) \
   throwSqlError( \
      ZString(selErr1)+iSelCols[iSelNxtFld-1].iName+" ("+ \
      ZString(x)+selErr2+typeName[index])

////////////////////////////////// ZMysqlLink //////////////////////////////////

ZMysqlLink::ZMysqlLink(
   const char* aConnection,
   const char* aUsername,
   const char* aPassword,
   int aMaxCursor) :
   ZSqlLink(aMaxCursor)
{
   ZFUNCTRACE_DEVELOP("ZMysqlLink::ZMysqlLink(...)");
   ZTRACE_DEVELOP(MYSQL_SERVER_VERSION);

   iHandle = new MYSQL;
   ZTRACE_DEVELOP("mysql_init");
   mysql_init(iHandle);

   ZTRACE_DEVELOP("mysql_options");
   int ret =
      mysql_options(
         iHandle,
         MYSQL_OPT_COMPRESS,
         0);
   if (ret)
      throwMysqlError(iHandle, "mysql_options");

   // parse aConnection as:
   //   database[:host[:port[:socket]]]

   ZString database(aConnection), host, port, socket;
   long pos = database.indexOf(':');
   if (pos) {
      host = database.subString(pos+1);
      database = database.subString(1,pos-1);
      pos = host.indexOf(':');
      if (pos) {
         port = host.subString(pos+1);
         database = host.subString(1,pos-1);
         pos = port.indexOf(':');
         if (pos) {
            socket = port.subString(pos+1);
            port = port.subString(1,pos-1);
         } // if
      } // if
   } // if

   ZTRACE_DEVELOP("database = "+database);
   ZTRACE_DEVELOP("host     = "+host);
   ZTRACE_DEVELOP("port     = "+port);
   ZTRACE_DEVELOP("socket   = "+socket);

   const char* iHost     = host.size()     > 0 ? host.constBuffer() : 0;
   const char* iDatabase = database.size() > 0 ? database.constBuffer() : 0;
   const char* iSocket   = socket.size()   > 0 ? socket.constBuffer() : 0;
   if (*aUsername == 0) aUsername = 0;
   if (*aPassword == 0) aPassword = 0;

   ZTRACE_DEVELOP("mysql_real_connect");
   MYSQL* handle =
      mysql_real_connect(
         iHandle,                        // MYSQL handle
         iHost,                          // host name or tcp address
         aUsername,                      // user name
         aPassword,                      // password
         iDatabase,                      // db name
         port.asUnsigned(),              // port
         iSocket,                        // unix_socket
         0);                             // client flags

   ZTRACE_DEVELOP("mysql_real_connect handle = "+ZString::hex(handle)+" "+ZString::hex(iHandle));
   if (!handle)
      throwMysqlError(iHandle, "mysql_real_connect");
} // ZMysqlLink

ZMysqlLink::~ZMysqlLink()
{
   ZFUNCTRACE_DEVELOP("ZMysqlLink::~ZMysqlLink()");

   // release all cursors before logoff
   for (int i = 0; i < iCsrCount; i++) delete iCsrPool[i];
   iCsrCount = 0;

   // now we can retire...
   ZTRACE_DEVELOP("mysql_close");
   mysql_close(iHandle);
   delete iHandle;
} // ~ZMysqlLink

ZString ZMysqlLink::dataBaseName() const
{
   ZFUNCTRACE_DEVELOP("ZMysqlLink::dataBaseName()");
   return "MYSQL";
} // dataBaseName

ZSqlCursor* ZMysqlLink::allocCursor()
{
   ZFUNCTRACE_DEVELOP("ZMysqlLink::allocCursor()");
   ZMysqlCursor *csr = new ZMysqlCursor;
   csr->iHandle = iHandle;
   return csr;
} // allocCursor

ZSqlLink& ZMysqlLink::commit()
{
   ZFUNCTRACE_DEVELOP("ZMysqlLink::commit()");
   ZString sql("COMMIT");
   int ret = mysql_real_query(iHandle, sql.constBuffer(), sql.size());
   if (ret)
      throwMysqlError(iHandle, "mysql_real_query");
   return *this;
} // commit

ZSqlLink& ZMysqlLink::rollback()
{
   ZFUNCTRACE_DEVELOP("ZMysqlLink::rollback()");
   ZString sql("ROLLBACK");
   int ret = mysql_real_query(iHandle, sql.constBuffer(), sql.size());
   if (ret)
      throwMysqlError(iHandle, "mysql_real_query");
   return *this;
} // rollback

////////////////////////////////// ZMysqlCursor ////////////////////////////////

ZMysqlCursor::ZMysqlCursor() :
   iHandle(0),
   iResult(0),
   iRow(0),
   iLengths(0),
   iBndNxtFld(0),
   iSelNxtFld(9999),
   iExecuted(zFalse)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::ZMysqlCursor()");
} // ZMysqlCursor

ZMysqlCursor::~ZMysqlCursor()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::~ZMysqlCursor()");
   dropResult();
} // ~ZMysqlCursor

void ZMysqlCursor::dropResult()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::dropResult()");
   if (iResult) {
      ZTRACE_DEVELOP("mysql_free_result");
      mysql_free_result(iResult);
      iResult = 0;
   } // if
   iRow = 0;
   iLengths = 0;
   iSql = "";
   iBndCols.drop();
   iSelCols.drop();
   iBndNxtFld = 0;
   iSelNxtFld = 9999;
   iExecuted = zFalse;
   iSelRowsProcessed = 0;
} // dropResult

int ZMysqlCursor::newParseRequired()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::newParseRequired()");
   iExecuted = zFalse;
   return 0;
} // newParseRequired

ZSqlCursor& ZMysqlCursor::parse(
   const char* stmt,     // sql statement
   short bndarrsize,     // bind array size
   short selarrsize,     // select array size
   long  longsize)       // max.select size for long types
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::parse(...)");
   dropResult();
   iSql = stmt;
   ZTRACE_DEVELOP("Stmt = "+iSql);
   return *this;
} // parse

void ZMysqlCursor::query(const ZString& aSql)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::query(...)");
   ZTRACE_DEVELOP("Stmt = "+aSql);

   ZTRACE_DEVELOP("mysql_real_query");
   int ret = mysql_real_query(iHandle, aSql, aSql.size());
   if (ret)
      throwMysqlError(iHandle, "mysql_real_query");

   ZTRACE_DEVELOP("mysql_store_result");
   iResult = mysql_store_result(iHandle);
   if (!iResult) {
      ZTRACE_DEVELOP("mysql_errno");
      if (mysql_errno(iHandle))
         throwMysqlError(iHandle, "mysql_store_result");
   } // if
   iExecuted = zTrue;
   iSelRowsProcessed = 0;
} // query

ZSqlCursor& ZMysqlCursor::define(
   const char* aName,        // bind variable name
   VarType     aHtype,       // variable type
   long        aHsize)       // max. size of text-types
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::define(...)");
   ZTRACE_DEVELOP( "name="+ZString(aName)+
                 "  type="+ZString(aHtype)+
                 "  size="+ZString(aHsize) );
   iSelCols.addAsLast(new Column(aName,aHtype,aHsize));
   return *this;
} // define

ZSqlCursor& ZMysqlCursor::bind(
   const char* aName,          // bind variable name
   VarType     aHtype,         // variable type
   long        aHsize)         // max. size of text-types
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::bind(...)");
   ZTRACE_DEVELOP("name="+ZString(aName)+
                "  type="+ZString(aHtype)+
                "  size="+ZString(aHsize) );
   iBndCols.addAsLast(new Column(aName,aHtype,aHsize));
   return *this;
} // bind

void ZMysqlCursor::flushBinds()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::flushBinds()");
   if (iBndCols.count()) {
      if (iBndNxtFld >= iBndCols.count()) {
         // insert bound values into the sql statement
         ZString stmt;
         const char* s = iSql.constBuffer();
         for (int c = 0; c < iBndCols.count(); c++) {
            if (!*s)
               throwSqlError("more bind values than placeholders in statement");
            // find next placeholder
            while (*s && *s !='?' && *s != '\'') stmt += *s++;
            if (*s == '\'') {
               // skip literal
               while (*s && *s != '\'') stmt += *s++;
               if (*s == '\'') stmt += *s++;
            } else
            if (*s == '?') {
               // insert bind value
               s++;
               stmt += iBndCols[c].iBuffer;
            } // if
         } // for
         while (*s) stmt += *s++;

         // execute it
         query(stmt);

         iBndNxtFld = 0;
      } else
         if (iBndNxtFld)
            throwSqlError("not all bind vars supplied");
   } // if
} // flushBinds

ZMysqlCursor::Column& ZMysqlCursor::beginBind()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::beginBind()");
   if (!iBndCols.count())
      throwSqlError("No bind columns defined");

   if (iBndNxtFld >= iBndCols.count())
      flushBinds();

   return iBndCols[iBndNxtFld];
} // beginBind

void ZMysqlCursor::bindText(Column& aCol, const ZString& aVal)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::bindText(...)");
   long size = aVal.size();
   if (size > aCol.iHsize) size = aCol.iHsize;
   char* buf = new char[2 * size + 1];
//   ZTRACE_DEVELOP("mysql_real_escape_string");
//   size = mysql_real_escape_string(iHandle, buf, aVal.constBuffer(), size);
   ZTRACE_DEVELOP("mysql_escape_string");
   size = mysql_escape_string(buf, aVal.constBuffer(), size);
   aCol.iBuffer = "'"+ZString(buf,size)+"'";
   delete [] buf;
} // bindText

void ZMysqlCursor::bindValue(const ZSqlInt32& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::bindValue(const ZSqlInt32& val)");
   Column &col = beginBind();
   if (val.isNull())
      col.iBuffer = "NULL";
   else
      switch (col.iHtype) {
         case varTypeInt8:
         case varTypeInt16:
         case varTypeInt32:
         case varTypeFloat32:
         case varTypeFloat64:
            col.iBuffer = ZString(val);
            break;
         case varTypeChar:
         case varTypeRaw:
         default:
            bindText(col, ZString(val));
      } // switch
   iBndNxtFld++;
} // bindValue(ZSqlInt32)

void ZMysqlCursor::bindValue(const ZSqlInt16& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::bindValue(const ZSqlInt16& val)");
   bindValue(ZSqlInt32(val,val.isNull()));
} // bindValue(ZSqlInt16)

void ZMysqlCursor::bindValue(const ZSqlInt8& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::bindValue(const ZSqlInt8& val)");
   bindValue(ZSqlInt32(val,val.isNull()));
} // bindValue(ZSqlInt8)

void ZMysqlCursor::bindValue(const ZSqlFloat64& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::bindValue(const ZSqlFloat64& val)");
   Column &col = beginBind();
   if (val.isNull())
      col.iBuffer = "NULL";
   else
      switch (col.iHtype) {
         case varTypeInt8:
         case varTypeInt16:
         case varTypeInt32:
         case varTypeFloat32:
         case varTypeFloat64:
            col.iBuffer = ZString(val);
            break;
         case varTypeChar:
         case varTypeRaw:
         default:
            bindText(col, ZString(val));
      } // switch
   iBndNxtFld++;
} // bindValue(ZSqlFloat64)

void ZMysqlCursor::bindValue(const ZSqlFloat32& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::bindValue(const ZSqlFloat32& val)");
   bindValue(ZSqlFloat64(val,val.isNull()));
} // bindValue(ZSqlFloat32)

void ZMysqlCursor::bindValue(const ZSqlChar& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::bindValue(const ZSqlChar& val)");
   Column &col = beginBind();
   if (val.isNull())
      col.iBuffer = "NULL";
   else
      switch (col.iHtype) {
         case varTypeInt8:
         case varTypeInt16:
         case varTypeInt32:
         case varTypeFloat32:
         case varTypeFloat64:
            throwSqlError("bind value type mismatch in column "+ZString(iBndNxtFld+1));
         case varTypeChar:
         case varTypeRaw:
         default:
            bindText(col, val);
      } // switch
   iBndNxtFld++;
} // bindValue(ZString)

ZSqlCursor& ZMysqlCursor::execute()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::execute()");
   flushBinds();

   if (!iExecuted) query(iSql);

   if (iResult && !iSelCols.count()) {
      // auto-define select columns
      ZTRACE_DEVELOP("mysql_num_fields");
//      unsigned selColCnt = mysql_num_fields(iResult);
//      unsigned selColCnt = mysql_num_fields(iHandle);
      unsigned selColCnt = mysql_field_count(iHandle);
      ZTRACE_DEVELOP("mysql_num_fields = "+ZString(selColCnt));

      for (unsigned col = 0; col < selColCnt; col++) {

         ZTRACE_DEVELOP("mysql_fetch_field_direct");
         MYSQL_FIELD* fld = mysql_fetch_field_direct(iResult, col);

         VarType htype;
         switch (fld->type) {
            case FIELD_TYPE_TINY:
               htype = varTypeInt8;
               break;
            case FIELD_TYPE_SHORT:
            case FIELD_TYPE_YEAR:
               htype = varTypeInt16;
               break;
            case FIELD_TYPE_INT24:
            case FIELD_TYPE_ENUM:
            case FIELD_TYPE_LONG:
               htype = varTypeInt32;
               break;
            case FIELD_TYPE_FLOAT:
               htype = varTypeFloat32;
               break;
            case FIELD_TYPE_DECIMAL:
            case FIELD_TYPE_DOUBLE:
            case FIELD_TYPE_LONGLONG:
               htype = varTypeFloat64;
               break;
            case FIELD_TYPE_TIMESTAMP:
            case FIELD_TYPE_DATE:
            case FIELD_TYPE_TIME:
            case FIELD_TYPE_DATETIME:
            case FIELD_TYPE_NEWDATE:
            case FIELD_TYPE_VAR_STRING:
            case FIELD_TYPE_STRING:
               htype = varTypeChar;
               break;
            case FIELD_TYPE_NULL:
            case FIELD_TYPE_SET:
            case FIELD_TYPE_TINY_BLOB:
            case FIELD_TYPE_MEDIUM_BLOB:
            case FIELD_TYPE_LONG_BLOB:
            case FIELD_TYPE_BLOB:
            default:
               htype = varTypeChar;
         } // switch
         define(fld->name, htype, fld->length);
         Column &col = iSelCols[iSelCols.count()-1];
         col.descrInfo(fld->type, fld->max_length, fld->length, fld->decimals);
      } // for
   } // if

   return *this;
} // execute

int ZMysqlCursor::fetch()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::fetch()");

   execute();

   if (!iResult) {
      ZTRACE_DEVELOP("mysql_field_count");
      unsigned selColCnt = mysql_field_count(iHandle);
      if (!selColCnt)
         throwSqlError("cannot fetch after non-select statement");
      return 0;
   } // if

   if ( !iSelCols.count() )
      throwSqlError("no select vars available");

   ZTRACE_DEVELOP("mysql_fetch_row");
   iRow = mysql_fetch_row(iResult);

   if (iRow) {
      ZTRACE_DEVELOP("mysql_fetch_lengths");
      iLengths = mysql_fetch_lengths(iResult);
      iSelNxtFld = 0;
      iSelRowsProcessed++;
   } // if

   return iRow != 0;
} // fetch

ZMysqlCursor::Column& ZMysqlCursor::beginFetch()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::beginFetch()");
   if (iSelNxtFld >= iSelCols.count())
      throwSqlError("no more columns in this row");
   return iSelCols[iSelNxtFld++];
} // beginFetch

void ZMysqlCursor::fetchValue(ZSqlInt32& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::fetchValue(ZSqlInt32& val)");
   int c = iSelNxtFld;
   beginFetch();
   if (iRow[c] == 0)
      val.setNull();
   else
      val = ZString(iRow[c], iLengths[c]).asLong();
} // fetchValue(ZSqlInt32&)

void ZMysqlCursor::fetchValue(ZSqlInt16 &val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::fetchValue(ZSqlInt16& val)");
   ZSqlInt32 x;
   fetchValue(x);
   val = ZSqlInt16(x, x.isNull());
   if (x != val)
      throwSelValSizeError(1);
} // fetchValue(ZSqlInt16&)

void ZMysqlCursor::fetchValue(ZSqlInt8& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::fetchValue(ZSqlInt8& val)");
   ZSqlInt32 x;
   fetchValue(x);
   val = ZSqlInt8(x, x.isNull());
   if (x != val)
      throwSelValSizeError(0);
} // fetchValue(ZSqlInt8&)

void ZMysqlCursor::fetchValue(ZSqlFloat64& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::fetchValue(ZSqlFloat64& val)");
   int c = iSelNxtFld;
   beginFetch();
   if (iRow[c] == 0)
      val.setNull();
   else
      val = ZString(iRow[c], iLengths[c]).asDouble();
} // fetchValue(ZMysqlFloat64&)

void ZMysqlCursor::fetchValue(ZSqlFloat32 &val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::fetchValue(ZSqlFloat32& val)");
   ZSqlFloat64 x;
   fetchValue(x);
   val = ZSqlFloat32(x, x.isNull());
   if (x != val)
      throwSelValSizeError(3);
} // fetchValue(ZSqlFloat32&)

void ZMysqlCursor::fetchValue(ZSqlChar& val)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::fetchValue(ZSqlChar& val)");
   int c = iSelNxtFld;
   beginFetch();
   if (iRow[c] == 0)
      val.setNull();
   else
      val = ZString(iRow[c], iLengths[c]);
} // fetchValue(ZSqlChar&)

unsigned long ZMysqlCursor::rowsProcessed()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::rowsProcessed()");
   if (iSelRowsProcessed) return iSelRowsProcessed;
   ZTRACE_DEVELOP("mysql_affected_rows");
   return mysql_affected_rows(iHandle);
} // rowsProcessed

int ZMysqlCursor::selectColumnCount()
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::selectColumnCount()");
   return iSelCols.count();
} // selectColumnCount

char* ZMysqlCursor::selectColumnName(int index)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::selectColumnName(int index)");
   return iSelCols[index].iName;
} // selectColumnName

char* ZMysqlCursor::selectColumnDescription(int index)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::selectColumnDescription(int index)");
   return iSelCols[index].iDescr;
} // selectColumnDescription

ZSqlCursor::VarType ZMysqlCursor::selectColumnType(int index)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::selectColumnType(int index)");
   return iSelCols[index].iHtype;
} // selectColumnType

int ZMysqlCursor::selectColumnSize(int index)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::selectColumnSize(int index)");
   return iSelCols[index].iHsize;
} // selectColumnSize

int ZMysqlCursor::selectColumnScale(int index)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::selectColumnScale(int index)");
   return iSelCols[index].iHscale;
} // selectColumnScale

///////////////////////////// ZMysqlCursor::Column //////////////////////////////

ZMysqlCursor::Column::Column(
   const char* aName,          // column name
   VarType     aHtype,         // c++ type code
   long        aHsize)         // size for Char & Raw
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::Column::Column(...)");
   iName  = aName;
   iHsize = aHsize;
   iHscale = 0;
   switch (iHtype = aHtype) {
      case varTypeInt8:
         iEtype   = FIELD_TYPE_TINY;
         break;
      case varTypeInt16:
         iEtype   = FIELD_TYPE_SHORT;
         break;
      case varTypeInt32:
         iEtype   = FIELD_TYPE_LONG;
         break;
      case varTypeFloat32:
         iEtype   = FIELD_TYPE_FLOAT;
         break;
      case varTypeFloat64:
         iEtype   = FIELD_TYPE_DOUBLE;
         break;
      case varTypeChar:
         iHsize = aHsize < 1 ? 2000 : aHsize;
         if (iHsize <= 255)
            iEtype   = FIELD_TYPE_VAR_STRING;
         else
         if (iHsize <= 65535)
            iEtype   = FIELD_TYPE_BLOB;
         else
         if (iHsize <= 16777215)
            iEtype   = FIELD_TYPE_MEDIUM_BLOB;
         else
            iEtype   = FIELD_TYPE_LONG_BLOB;
         break;
      case varTypeRaw:
         iHsize = aHsize < 1 ? 2000 : aHsize;
         if (iHsize <= 255)
            iEtype   = FIELD_TYPE_TINY_BLOB;
         else
         if (iHsize <= 65535)
            iEtype   = FIELD_TYPE_BLOB;
         else
         if (iHsize <= 16777215)
            iEtype   = FIELD_TYPE_MEDIUM_BLOB;
         else
            iEtype   = FIELD_TYPE_LONG_BLOB;
         break;
      default:
         throwSqlError("bind/define: unknown VarType");
   } // switch
} // Column

ZMysqlCursor::Column::Column(const Column& aColumn) :
   iBuffer(aColumn.iBuffer),
   iName(aColumn.iName),
   iDescr(aColumn.iDescr),
   iHsize(aColumn.iHsize),
   iHscale(aColumn.iHscale),
   iHtype(aColumn.iHtype),
   iEtype(aColumn.iEtype)
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::Column::Column(const Column& aColumn)");
} // copy constructor

ZBoolean ZMysqlCursor::Column::operator==(const Column& aColumn) const
{
   return zTrue; // Columnlist::find() not used
} // operator==

void ZMysqlCursor::Column::descrInfo(
   enum enum_field_types aType,    // mysql internal type code
   long  aSize,                    // displayed size
   short aPrec,                    // precision for numerics
   short aScale )                  // scale for numerics
{
   ZFUNCTRACE_DEVELOP("ZMysqlCursor::Column::descrInfo(...)");
   iHscale = 0;
   switch (aType) {
      case FIELD_TYPE_DECIMAL:
         iDescr = "DECIMAL";
         if (aPrec) {
            iDescr += "("+ZString(aPrec);
            if (aScale)
               iDescr += ","+ZString(aScale);
            iDescr += ")";
         } // if
         iHsize = aPrec ? aPrec : 38;
         iHscale = aScale;
         break;
      case FIELD_TYPE_TINY:
         iDescr = "TINY";
         break;
      case FIELD_TYPE_SHORT:
         iDescr = "SHORT";
         break;
      case FIELD_TYPE_LONG:
         iDescr = "LONG";
         break;
      case FIELD_TYPE_FLOAT:
         iDescr = "FLOAT";
         break;
      case FIELD_TYPE_DOUBLE:
         iDescr = "DOUBLE";
         break;
      case FIELD_TYPE_NULL:
         iDescr = "NULL";
         break;
      case FIELD_TYPE_TIMESTAMP:
         iDescr = "TIMESTAMP";
         break;
      case FIELD_TYPE_LONGLONG:
         iDescr = "LONGLONG";
         break;
      case FIELD_TYPE_INT24:
         iDescr = "INT24";
         break;
      case FIELD_TYPE_DATE:
         iDescr = "DATE";
         break;
      case FIELD_TYPE_TIME:
         iDescr = "TIME";
         break;
      case FIELD_TYPE_DATETIME:
         iDescr = "DATETIME";
         break;
      case FIELD_TYPE_YEAR:
         iDescr = "YEAR";
         break;
      case FIELD_TYPE_NEWDATE:
         iDescr = "NEWDATE";
         break;
      case FIELD_TYPE_ENUM:
         iDescr = "ENUM";
         break;
      case FIELD_TYPE_SET:
         iDescr = "SET";
         break;
      case FIELD_TYPE_TINY_BLOB:
         iDescr = "TINYBLOB("+ZString(aSize)+")";
         break;
      case FIELD_TYPE_MEDIUM_BLOB:
         iDescr = "MEDIUMBLOB("+ZString(aSize)+")";
         break;
      case FIELD_TYPE_LONG_BLOB:
         iDescr = "LONGBLOB("+ZString(aSize)+")";
         break;
      case FIELD_TYPE_BLOB:
         iDescr = "BLOB("+ZString(aSize)+")";
         break;
      case FIELD_TYPE_VAR_STRING:
         iDescr = "VARCHAR("+ZString(aSize)+")";
         break;
      case FIELD_TYPE_STRING:
         iDescr = "CHAR("+ZString(aSize)+")";
         break;
      default:
         throwSqlError("unknown mysql internal datatype");
   } // switch
} // descrInfo

ZExport(ZSqlLink*) ZSqlConnectTo(
   const char* aConnection,
   const char* aUsername,
   const char* aPassword,
   int         aMaxCursor)
{
   ZFUNCTRACE_DEVELOP("ZSqlConnectTo(...)");
   return new ZMysqlLink(aConnection, aUsername, aPassword, aMaxCursor);
} // ZSqlConnectTo
