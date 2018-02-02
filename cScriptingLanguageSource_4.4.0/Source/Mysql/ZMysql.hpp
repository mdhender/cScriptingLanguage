/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZMysql.hpp
 * Application :  IBK Open Class Library
 * Purpose     :  Database interface for Mysql
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.06.11  First implementation                        P.Koch, IBK
 * 2001.06.29  Changes for OS/2                            P.Koch, IBK
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
#if ZC_WIN
   #include <winsock.h> // need SOCKET for mysql.h
#endif
#include <mysql.h>

#if ZC_WIN
  #if MYSQL_VERSION_ID < 32338
    #error "CSS requires MySQL V3.23.38 or higher!"
  #endif
#endif
#if ZC_OS2
  #if MYSQL_VERSION_ID < 32332
    #error "CSS requires MySQL V3.23.32 or higher!"
  #endif
#endif
#if ZC_UNIXFAM
  #if MYSQL_VERSION_ID < 32339
    #error "CSS requires MySQL V3.23.39 or higher!"
  #endif
#endif

class ZMysqlCursor : public ZSqlCursor
{
   private:
      friend class ZMysqlLink;

      class Column
      {
         public:
            Column(                      // constructor
               const char* aColName,        // column name
               VarType aColType,            // c++ type
               long aSize);                 // size for Char/Raw

            Column(const Column& aColumn); // copy constructor

            ZBoolean operator==(const Column& aColumn) const;

            void descrInfo(              // prep. description info
               enum enum_field_types aType, // mysql internal type code
               long  aSize,                 // displayed size
               short aPrec,                 // precision for numerics
               short aScale);               // scale for numerics

            ZString     iBuffer;         // buffer for data
            ZString     iName;           // column name
            ZString     iDescr;          // column description
            long        iHsize;          // size/precision
            short       iHscale;         // scale
            VarType     iHtype;          // var type
            enum enum_field_types iEtype; // mysql datatype
      }; // Column

      // declare list of columns
      class Columnlist;
      friend class ZMysqlCursor::Columnlist;
      ZLISTCLASSDECLARE(Columnlist, Column)
      friend class ZMysqlCursor::Columnlist::List; // VACPP 3.5 requires this

      ZMysqlCursor();                    // constructor

      virtual ~ZMysqlCursor();           // destructor

      void dropResult();                 // drop result set

      int newParseRequired();            // check for new parse need

      // bind operations for all sql datatypes
      void bindValue(const ZSqlInt8& aValue);
      void bindValue(const ZSqlInt16& aValue);
      void bindValue(const ZSqlInt32& aValue);
      void bindValue(const ZSqlFloat32& aValue);
      void bindValue(const ZSqlFloat64& aValue);
      void bindValue(const ZSqlChar& aValue);

      // fetch operations for all sql datatypes
      void fetchValue(ZSqlInt8& aValue);
      void fetchValue(ZSqlInt16& aValue);
      void fetchValue(ZSqlInt32& aValue);
      void fetchValue(ZSqlFloat32& aValue);
      void fetchValue(ZSqlFloat64& aValue);
      void fetchValue(ZSqlChar& aValue);

      ZSqlCursor& parse(                 // parse statement
         const char* aSqlStmt,              // sql statement
         short aBindArrSize = 1,            // bind array size
         short aSelArrSize = 1,             // select array size
         long aMaxLong = 2000);             // max.select size for long types

      void query(                        // execute query
         const ZString& aSql);              // sql statement

      ZSqlCursor& define(                // define vars
         const char* aColName,              // select column name
         VarType aVarType = varTypeInt32,   // c++ variable type
         long aMaxSize = 0);                // max.size for String & Raw

      ZSqlCursor& bind(                  // bind vars
         const char* aName,                 // placeholder name
         VarType aVarType = varTypeInt32,   // c++ variable type
         long aMaxSize = 0);                // max.size for Char & Raw

      void flushBinds();                 // flush bound values

      Column& beginBind();               // begin bind var insertion

      void bindText(                     // bind text
         Column& aCol,                      // column
         const ZString& aVal);              // value to set

      Column& beginFetch();              // begin fetch value operation

      int fetch();                       // fetch next row

      ZSqlCursor& execute();             // execute dml statement

      unsigned long rowsProcessed();     // get # of rows processed

      void getDescrInfo();               // get description information

      int selectColumnCount();           // get # of select columns

      char* selectColumnName(            // get name of select column
         int aIndex);                       // index of column

      VarType selectColumnType(          // get type of select column
         int aIndex);                       // index of column

      int selectColumnSize(              // get size of select column
         int aIndex);                       // index of column

      int selectColumnScale(             // get scale of select column
         int aIndex);                       // index of column

      char* selectColumnDescription(     // get description of select column
         int aIndex);                       // index of column

      MYSQL*         iHandle;            // mysql connection handle
      MYSQL_RES*     iResult;            // mysql result set
      MYSQL_ROW      iRow;               // mysql row data set
      unsigned long* iLengths;           // lengths of row data set
      Columnlist     iBndCols;           // bind column list
      Columnlist     iSelCols;           // select column list
      ZString        iSql;               // sql statement
      short          iBndNxtFld;         // next column to fill
      short          iSelNxtFld;         // next column to fetch
      ZBoolean       iExecuted;          // query was executed
      unsigned long  iSelRowsProcessed;  // # of select rows processed
}; // ZMysqlCursor

class ZMysqlLink : public ZSqlLink
{
   public:
      ZMysqlLink(                        // logon
         const char *aConnection,           // connection/datastore
         const char *aUsername,             // username
         const char *aPassword,             // password
         int aMaxCursor                     // cursor pool size
      );
      ~ZMysqlLink();                     // logoff

   private:
      ZSqlCursor *allocCursor();         // allocate a cursor
      ZSqlLink& commit();                // commit work
      ZSqlLink& rollback();              // rollback work
      ZString dataBaseName() const;      // return database name "MYSQL"

      MYSQL* iHandle;                    // mysql connection handle
}; // ZMysqlLink
