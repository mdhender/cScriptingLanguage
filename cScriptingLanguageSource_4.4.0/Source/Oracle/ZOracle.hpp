/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZOracle.hpp
 * Application :  IBK Open Class Library
 * Purpose     :  Database interface for Oracle V7/8/8i
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.29  First implementation                         P.Koch, IBK
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

#pragma pack(1)

typedef struct {
    short               csrrc;           // v2 return code
    short               csrft;           // function type
    unsigned long       csrrpc;          // rows processed count
    short               csrpeo;          // parse error offset
    unsigned char       csrfc;           // function code
    unsigned char       csrfil;          // filler
    short               csrarc;          // return code
    unsigned char       csrwrn;          // warning flags
    unsigned char       csrflg;          // error flags
    //**** Operating system dependent ****
    unsigned short      csrcn;           // cursor number
    struct {                             // rowid structure
        struct {
            unsigned long  tidtrba;      // rba of first blockof table
            unsigned short tidpid;       // partition id of table
            unsigned char  tidtbl;       // table id of table
        } ridtid;
        unsigned long   ridbrba;         // rba of datablock
        unsigned short  ridsqn;          // sequence number of row in block
    } csrrid;
    unsigned short      csrose;          // os dependent error code
    unsigned char       csrchk;          // check byte
    unsigned char       crsfill[30];     // private, reserved fill
} ZOraCDA;

typedef struct {
    short               ldarc2;          // v2 return code
    unsigned short      filler_1[5];     // filler
    short               ldarc;           // return code
    unsigned short      filler_2;        // filler
    //**** Operating system dependent ****
    unsigned char       filler_3[14];    // filler
    unsigned char       filler_4;        //
    unsigned char       osderr1;         // op.sys. dependent error code
    unsigned char       osderr2;         // op.sys. dependent error code
    unsigned char       chkbyte;         // check byte
    unsigned char       filler_5[30];    // oracle internal parameters
} ZOraLDA;

#pragma pack(4)

class ZOraCursor : public ZSqlCursor
{
   public:
      enum ExtType {
         extTypeInteger      = 3,
         extTypeFloat        = 4,
         extTypeVarchar      = 9,
         extTypeVarraw       = 15,
         extTypeLongVarchar  = 94,
         extTypeLongVarraw   = 95
      };

   private:
      friend class ZOraLink;

      class Column
      {
         public:
            Column(                      // constructor
               const char* aColName,        // column name
               VarType aColType,            // c++ type
               long aSize,                  // size for Char/Raw
               short aArrSize);             // arraysize

            Column(const Column& aColumn); // copy constructor

            ~Column();                   // destructor

            ZBoolean operator==(const Column& aColumn) const;

            void descrInfo(              // prep. description info
               short aOraType,              // ORACLE internal type code
               long aDispSize,              // displayed size
               short aPrecision,            // precision for numerics
               short aScale);               // scale for numerics

            unsigned char*  buffer;      // buffer for data (array)
            short*          ind;         // indicator var array
            ZString         name;        // column name
            ZString         descr;       // column description
            int             bufsiz;      // size of one buffer
            int             hsize;       // size/precision
            short           hscale;      // scale
            short           arrsize;     // array size
            VarType         htype;       // program var type
            ExtType         etype;       // ORACLE external datatype
      }; // Column

      // declare list of columns
      class Columnlist;
      friend class ZOraCursor::Columnlist;
      ZLISTCLASSDECLARE(Columnlist, Column)
      friend class ZOraCursor::Columnlist::List; // VACPP 3.5 requires this

      ZOraCursor();                      // constructor

      virtual ~ZOraCursor();             // destructor

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

      ZSqlCursor& parse(                 // parse statement
         const char* aSqlStmt,              // sql statement
         short aBindArrSize = 1,            // bind array size
         short aSelArrSize = 1,             // select array size
         long aMaxLong = 2000);             // max.select size for long types

      ZSqlCursor& bind(                  // bind vars
         const char* aName,                 // placeholder name
         VarType aVarType = varTypeInt32,   // c++ variable type
         long aMaxSize = 0);                // max.size for Char & Raw

      ZSqlCursor& define(                // define vars
         const char* aColName,              // select column name
         VarType aVarType = varTypeInt32,   // c++ variable type
         long aMaxSize = 0);                // max.size for String & Raw

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

      ZOraLDA     *lda;                  // logon descriptor area
      Columnlist  bnd;                   // bind column list
      Columnlist  sel;                   // select column list
      ZOraCDA     cda;                   // cursor descriptor area
      ZString     sql;                   // sql statement
      short       bndarr;                // bind array size
      short       bndnxt;                // next row index
      short       bndnxtfld;             // next column to fill
      short       selarr;                // select array size
      short       selnxt;                // next row to fetch
      short       selnxtfld;             // next column to fetch
      short       selcnt;                // valid rows in array
      long        selLongSize;           // size for undet.long columns
}; // ZOraCursor

class ZOraLink : public ZSqlLink
{
   public:
      ZOraLink(                          // logon
         const char *aConnection,           // connection/datastore
         const char *aUsername,             // username
         const char *aPassword,             // password
         int aMaxCursor                     // cursor pool size
      );
      ~ZOraLink();                       // logoff

   private:
      ZSqlCursor *allocCursor();         // allocate a cursor
      ZSqlLink& commit();                // commit work
      ZSqlLink& rollback();              // rollback work
      ZString dataBaseName() const;      // return database name "ORACLE"

      ZOraLDA         lda;               // logon descriptor area
      unsigned char   hda[256];          // host descriptor area
}; // ZOraLink

#pragma pack()
