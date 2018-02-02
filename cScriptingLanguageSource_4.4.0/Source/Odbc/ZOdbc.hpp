/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZOdbc.hpp
 * Application :  IBK Open Class Library
 * Purpose     :  Database interface using ODBC
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.07.05  First implementation                        P.Koch, IBK
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
#include <windows.h>
#include <sqlext.h>

class ZOdbcCursor : public ZSqlCursor
{
   friend class ZOdbcLink;

   private:
      class Column {
         public:
            Column(                      // constructor
               const char* aColName,        // column name
               VarType aVarType,            // c++ type
               long aSize,                  // size for Char/Raw
               short aArrSize);             // arraysize

            Column(const Column& aColumn); // copy constructor

            ~Column();                   // destructor

            ZBoolean operator==(const Column& aColumn) const;

            void descrInfo(              // prep. description info
               short aOdbcType,              // ODBC internal type code
               long aDispSize,              // displayed size
               short aPrecision,            // precision for numerics
               short aScale);               // scale for numerics

            unsigned char   *buffer;     // buffer for data (array)
            SQLINTEGER      *ind;        // indicator var array
            ZString         name;        // column name
            ZString         descr;       // column description
            SQLINTEGER      bufsiz;      // size of one buffer
            SQLINTEGER      hsize;       // size/precision
            SQLSMALLINT     hscale;      // scale
            short           arrsize;     // array size
            VarType         htype;       // program var type
            SQLSMALLINT     etype;       // ODBC external datatype
            SQLSMALLINT     itype;       // ODBC internal datatype
      }; // Column

      // declare list of columns
      class Columnlist;
      friend class ZOdbcCursor::Columnlist;
      ZLISTCLASSDECLARE(Columnlist, Column)
      friend class ZOdbcCursor::Columnlist::List; // VACPP 3.5 requires this

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

      Column& beginBind();               // begin bind var insertion

      Column& beginFetch();              // begin fetch value operation

      ZOdbcCursor(SQLHENV, SQLHDBC);      // constructor

      virtual ~ZOdbcCursor();             // destructor

      ZSqlCursor& parse(                 // parse statement
         const char* aSqlStmt,              // sql statement
         short aBindArrSize= 1,             // bind array size
         short aSelArrSize= 1,              // select array size
         long aMaxSize = 2000 );            // max.select size for long types

      ZSqlCursor& bind(                  // bind vars
         const char* aName,                 // placeholder name
         VarType aVarType = varTypeInt32,   // c++ variable type
         long aMaxSize = 0);                // max.size for Char & Raw

      ZSqlCursor& define(                // define vars
         const char* aColName,              // select column name
         VarType aVarType = varTypeInt32,   // c++ variable type
         long aMaxSize=0);                  // max.size for String & Raw

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

      SQLHENV     henv;                  // environment handle
      SQLHDBC     hdbc;                  // database connection handle
      SQLHSTMT    hstmt;                 // statement handle
      Columnlist  bnd;                   // bind column list
      Columnlist  sel;                   // select column list
      ZString     sql;                   // sql statement
      short   errflag;                   // error occured
      SQLUINTEGER bndarr;                // bind array size
      SQLUINTEGER bndnxt;                // next row index
      SQLUINTEGER bndcnt;                // currently defined bind array count
      short   bndnxtfld;                 // next column to fill
      SQLUINTEGER selarr;                // select array size
      SQLUINTEGER selnxt;                // next row to fetch
      SQLUINTEGER selcnt;                // valid rows in array
      short   selnxtfld;                 // next column to fetch
      long    selLongSize;               // size for undet.long columns
      unsigned long selRowsProcessed;    // # of select rows processed
}; // ZOdbcCursor

/*
 *  C l a s s   :   K D B 2 L i n k
 *
 *  ODBC V2.x Link
 */
class ZOdbcLink : public ZSqlLink
{
   public:
      ZOdbcLink(                          // logon
         const char* aConnection,           // connection
         const char* aUsername,             // user name
         const char* aPassword,             // password
         int aMaxCursor                     // cursor pool size
      );
      ~ZOdbcLink();                       // logoff

   private:
      ZSqlCursor *allocCursor();         // allocate a cursor
      ZSqlLink& commit();                // commit work
      ZSqlLink& rollback();              // rollback work
      ZString dataBaseName() const;      // return database name "ODBC"

      SQLHENV  henv;                     // environment handle
      SQLHDBC  hdbc;                     // database connection handle
      ZBoolean txn;                      // supports transactions
      ZBoolean conn;                     // connected
}; // ZOdbcLink
