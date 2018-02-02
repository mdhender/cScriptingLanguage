/* Copyright (c) 2001-2002 IBK-Landquart-Switzerland. All rights reserved.
 *
 * Module      :  ZOracle.cpp
 * Application :  IBK Open Class Library
 * Purpose     :  Database interface for Oracle V7/8/8i
 *
 * Date        Description                                 Who
 * --------------------------------------------------------------------------
 * 2001.05.29  First implementation                        P.Koch, IBK
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
#include "ZOracle.hpp"

#ifdef ZC_OS2
   #define ORA_FUNCTION _System
#endif
#ifdef ZC_WIN
   #define ORA_FUNCTION __cdecl
#endif

extern "C" {

/*
 *  O R A C L E   O C I - f u n c t i o n s
 */
int ORA_FUNCTION orlon(          // log on
    ZOraLDA *,                      // logon descriptor area
    unsigned char *,                // host data area
    const char *,                   // username
    int,                            // size of username
    const char *,                   // password
    int,                            // size of password
    int                             // audit
);

int ORA_FUNCTION ologof(         // log off
    ZOraLDA *                       // logon descriptor area
);

int ORA_FUNCTION oerhms(         // get errormessage
    ZOraLDA *,                      // logon descriptor area
    short,                          // V7 error code
    char *,                         // text buffer
    int                             // size of buffer
);

int ORA_FUNCTION ocom(           // commit
    ZOraLDA *                       // logon descriptor area
);

int ORA_FUNCTION orol(           // rollback
    ZOraLDA *                       // logon descriptor area
);

int ORA_FUNCTION oopen(          // open cursor
    ZOraCDA *,                      // cursor descriptor area
    ZOraLDA *,                      // logon descriptor area
    const char *,                   // database name (V2 compatibility)
    short,                          // database name length
    short,                          // area size
    const char *,                   // logon string
    short                           // logon string length
);

int ORA_FUNCTION oclose(         // close cursor
    ZOraCDA *                       // cursor descriptor area
);

int ORA_FUNCTION ocan(           // cancel current operation
    ZOraCDA *                       // cursor descriptor area
);

int ORA_FUNCTION oparse(         // parse statement
    ZOraCDA *,                      // cursor descriptor area
    const char *,                   // sql statement
    long,                           // length of sql statement
    int,                            // deferred mode flag
    unsigned long                   // V6/V7 mode flag
);

#ifdef ZC_WIN
int ORA_FUNCTION opinit(         // initialize multithreaded mode
    unsigned long                   // mode: 0 = single, 1 = multithread
);
#endif

int ORA_FUNCTION odescr(         // describe select items
    ZOraCDA *,                      // cursor descriptor area
    int,                            // position #
    long *,                         // max.size of column
    short *,                        // datatype code
    const char *,                   // column name
    long *,                         // name size
    long *,                         // display size
    short *,                        // numeric precision
    short *,                        // numeric scale
    short *                         // NOT NULL column
);

int ORA_FUNCTION odefin(         // define output variable for select item
    ZOraCDA *,                      // cursor descriptor area
    int,                            // position #
    unsigned char *,                // buffer address
    int,                            // buffer size
    int,                            // external datatype
    int,                            // packed dec.scale (unused)
    short *,                        // NULL indicator var
    char *,                         // packed dec.fmt (unused)
    int,                            // length of above (unused)
    int,                            // packed dec.type (unused)
    unsigned short *,               // length of fetched data
    unsigned short *                // column return code
);

int ORA_FUNCTION obndrv(         // bind program var to sql placeholder
    ZOraCDA *,                      // cursor descriptor area
    const char *,                   // name of placeholder (":abc")
    int,                            // length of placeholder name
    unsigned char *,                // buffer address
    int,                            // buffer size
    int,                            // external datatype code
    int,                            // packed dec.scale (unused)
    short *,                        // NULL indicator var
    char *,                         // packed dec.fmt (unused)
    int,                            // length of above (unused)
    int                             // packed dec.type (unused)
);

int ORA_FUNCTION oexn(           // execute statement
    ZOraCDA *,                      // cursor descriptor area
    int,                            // # of rows
    int                             // rows offset
);

int ORA_FUNCTION ofen(           // fetch rows
    ZOraCDA *,                      // cursor descriptor area
    int                             // # of rows
);

} // extern "C"

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

// general sql errors

static const char invType[] = "internal error (invalid type-code supplied)";

#define throwSqlError(txt) \
{ \
    ZSqlError exc(txt); \
    ZTHROW(exc); \
} // throwSqlError

// select value mismatches

static const char selErr1[] = "selected value from column ";
static const char selErr2[] = ") doesn't fit into ";

#define throwSelValSizeError(index) \
{\
    ZSqlError exc(sql); \
    exc.addAsLast( \
       ZString(selErr1)+sel[selnxtfld-1].name+" ("+ \
       ZString(x)+selErr2+typeName[index]); \
    ZTHROW(exc);\
} // throwSelValSizeError

// bind value mismatches

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

// general oracle errors

static ZSqlError oraError(
   ZOraLDA     *lda,    // logon descriptor area
   ZOraCDA     *cda,    // cursor descriptor area
   const char  *sql,    // sql statement
   const char  *oci)    // oci function name
{
   ZFUNCTRACE_DEVELOP("oraError(...)");

   ZString func;

   // get function name
   if (cda) switch (cda->csrfc) {
      case  4: func="oexn";     break;
      case  8: func="odefin";   break;
      case 12: func="ofen";     break;
      case 14: func="oopen";    break;
      case 16: func="oclose";   break;
      case 28: func="obndrv";   break;
      case 52: func="ocan";     break;
      case 54: func="oparse";   break;
      case 60: func="odescr";   break;
      default: func="OCI#"+ZString((int)cda->csrfc);
   } else
      if (oci) func = oci;

   // get error code
   int err = cda ? cda->csrarc : lda->ldarc;

   // first message line
   ZSqlError exc("ORACLE error "+ZString(err)+" in "+func);

   // add sql statement if applicable
   if (sql) exc.addAsLast(sql);

   // get message in clear text
   char buf[1000], *b0, *b1;
   *buf = 0;
   ZTRACE_DEVELOP("oerhms");
   oerhms(lda, err, b1 = buf, sizeof(buf) );

   // add line by line
   while (*b1) {
      b0 = b1;
      while (*b1 && *b1!='\r' && *b1!='\n') b1++;
      while (*b1=='\r' || *b1=='\n') *b1++ = 0;
      exc.addAsLast(b0);
   } // while
   return exc;
} // oraError

#define throwOraError(lda,cda,sql,oci) \
   ZTHROW(oraError(lda,cda,sql,oci))

////////////////////////////////// ZOraLink //////////////////////////////////

ZOraLink::ZOraLink(
   const char* aConnection,
   const char* aUsername,
   const char* aPassword,
   int aMaxCursor) :
   ZSqlLink(aMaxCursor)
{
   ZFUNCTRACE_DEVELOP("ZOraLink::ZOraLink(...)");

#ifdef ZC_WIN
   ZTRACE_DEVELOP("opinit");
   if (opinit(1))
      throwOraError(&lda,NULL,NULL,"opinit");
#endif

   ZTRACE_DEVELOP("orlon");
   ZString nm(aUsername);
   if (*aConnection) nm = nm+"@"+aConnection;
   if ( orlon(&lda, hda, nm.constBuffer(), -1, aPassword, -1, -1) )
      throwOraError(&lda,NULL,NULL,"orlon");
} // ZOraLink

ZOraLink::~ZOraLink()
{
   ZFUNCTRACE_DEVELOP("ZOraLink::~ZOraLink()");

   // release all cursors before logoff
   for (int i = 0; i < iCsrCount; i++) delete iCsrPool[i];
   iCsrCount = 0;

   // now we can retire...
   ZTRACE_DEVELOP("ologof");
   if (ologof( &lda ))
      throwOraError(&lda,NULL,NULL,"ologof");
} // ~ZOraLink

ZString ZOraLink::dataBaseName() const
{
   ZFUNCTRACE_DEVELOP("ZOraLink::dataBaseName()");
   return "ORACLE";
} // dataBaseName

ZSqlCursor* ZOraLink::allocCursor()
{
   ZFUNCTRACE_DEVELOP("ZOraLink::allocCursor()");
   ZOraCursor *csr = new ZOraCursor;
   csr->lda = &lda;
   ZTRACE_DEVELOP("oopen");
   if ( oopen( &csr->cda, &lda, NULL, -1, -1, NULL, -1 ) )
      throwOraError(&lda,&csr->cda,NULL,NULL);
   return csr;
} // allocCursor

ZSqlLink& ZOraLink::commit()
{
   ZFUNCTRACE_DEVELOP("ZOraLink::commit()");

   ZTRACE_DEVELOP("ocom");
   if (ocom( &lda ))
      throwOraError(&lda,NULL,NULL,"ocom");
   return *this;
} // commit

ZSqlLink& ZOraLink::rollback()
{
   ZFUNCTRACE_DEVELOP("ZOraLink::rollback()");

   ZTRACE_DEVELOP("orol");
   if (orol( &lda ))
      throwOraError(&lda,NULL,NULL,"orol");
   return *this;
} // rollback

////////////////////////////////// ZOraCursor ////////////////////////////////

ZOraCursor::ZOraCursor()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::ZOraCursor()");
   memset( &cda, 0, sizeof(cda) );
} // ZOraCursor

ZOraCursor::~ZOraCursor()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::~ZOraCursor()");
   ZTRACE_DEVELOP("oclose");
   if ( oclose(&cda) )
      throwOraError(lda,&cda,NULL,NULL);
} // ~ZOraCursor

int ZOraCursor::newParseRequired()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::newParseRequired()");

   // if ORACLE error occured on this cursor, force new parse
   return  (cda.csrarc && cda.csrarc!=1403) ? 1 : 0;
} // newParesRequired

ZSqlCursor& ZOraCursor::parse(
   const char* stmt,     // sql statement
   short bndarrsize,     // bind array size
   short selarrsize,     // select array size
   long  longsize)       // max.select size for long types
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::parse(...)");

   if (sql.size()) {
      ZTRACE_DEVELOP("ocan");
      ocan( &cda );
      if (cda.csrarc && cda.csrarc != 1003)
         throwOraError(lda,&cda,NULL,NULL);
   } // if

   ZTRACE_DEVELOP("oparse");
   sql = stmt;
   if ( oparse(&cda,sql.constBuffer(),-1,1,2) )
      throwOraError(lda,&cda,sql.constBuffer(),NULL);
   selarr = selarrsize;
   bndarr = bndarrsize;
   selLongSize = longsize<1 ? 2000 : longsize;

   ZTRACE_DEVELOP( "Stmt = "+sql );

   // clear bind variable list for subsequent bind-calls
   bnd.drop();
   bndnxt = 0;
   bndnxtfld = 0;

   // clear select item list for subsequent define-calls
   sel.drop();
   selnxt = 0;
   selnxtfld = 9999;
   return *this;
} // parse

ZSqlCursor& ZOraCursor::define(
   const char* name,        // bind variable name
   VarType     htype,       // variable type
   long        hsize)       // max. size of text-types
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::define(...)");

   ZTRACE_DEVELOP( "name="+ZString(name)+
                 "  type="+ZString(htype)+
                 "  size="+ZString(hsize) );

   sel.addAsLast(new Column(name,htype,hsize,selarr));
   Column &col = sel[sel.count()-1];

   if ( (col.etype==extTypeLongVarchar || col.etype==extTypeLongVarraw)
        && col.hsize>65532 ) {
      *(long*)col.buffer = col.bufsiz;
      ZTRACE_DEVELOP("odefin");
      if ( odefin(&cda,sel.count(),col.buffer,-1,col.etype,-1,
                  col.ind,NULL,-1,-1,NULL,NULL) )
         throwOraError(lda,&cda,sql.constBuffer(),NULL);
   } else {
      ZTRACE_DEVELOP("odefin");
      if ( odefin(&cda,sel.count(),col.buffer,col.bufsiz,col.etype,-1,
                  col.ind,NULL,-1,-1,NULL,NULL) )
         throwOraError(lda,&cda,sql.constBuffer(),NULL);
   } // if
   return *this;
} // define

ZSqlCursor& ZOraCursor::bind(
   const char* name,          // bind variable name
   VarType     htype,         // variable type
   long        hsize)         // max. size of text-types
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::bind(...)");

   ZTRACE_DEVELOP("name="+ZString(name)+
                "  type="+ZString(htype)+
                "  size="+ZString(hsize) );

   bnd.addAsLast(new Column(name,htype,hsize,bndarr));
   Column &col = bnd[bnd.count()-1];

   if ( (col.etype==extTypeLongVarchar || col.etype==extTypeLongVarraw)
        && col.hsize>65532 ) {
      *(long*)col.buffer = col.bufsiz;
      ZTRACE_DEVELOP("obndrv");
      if ( obndrv(&cda,name,-1,col.buffer,-1,
                  col.etype,-1,col.ind,NULL,-1,-1) )
         throwOraError(lda,&cda,sql.constBuffer(),NULL);
   } else {
      ZTRACE_DEVELOP("obndrv");
      if ( obndrv(&cda,name,-1,col.buffer,col.bufsiz,
                  col.etype,-1,col.ind,NULL,-1,-1) )
         throwOraError(lda,&cda,sql.constBuffer(),NULL);
   } // if
   return *this;
} // bind

ZOraCursor::Column& ZOraCursor::beginBind()
{
    ZFUNCTRACE_DEVELOP("ZOraCursor::beginBind()");
    if (!bnd.count())
       throwSqlError("no bind columns defined");
    if (bndnxt>=bndarr) {
        ZTRACE_DEVELOP("oexn");
        if ( oexn(&cda,bndnxt,0) )
            throwOraError(lda,&cda,sql.constBuffer(),NULL);
        bndnxt = 0;
        bndnxtfld = 0;
    } // if
    return bnd[bndnxtfld];
} // beginBind

void ZOraCursor::bindValue(const ZSqlInt32& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::bindValue(const ZSqlInt32& val)");
   Column &col = beginBind();

   // verify compatibility with column
   switch (col.htype) {
      case varTypeInt8:
         ((char *)col.buffer)[bndnxt] = val;
         if (((char *)col.buffer)[bndnxt] != val) goto noMatch;
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
   col.ind[bndnxt] = val.isNull() ? -1 : 0;
   if (++bndnxtfld>=bnd.count()) {
      bndnxt++;
      bndnxtfld = 0;
   } // if
} // bindValue(ZSqlInt32)

void ZOraCursor::bindValue(const ZSqlInt16& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::bindValue(const ZSqlInt16& val)");
   bindValue(ZSqlInt32(val,val.isNull()));
} // bindValue(ZSqlInt16)

void ZOraCursor::bindValue(const ZSqlInt8& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::bindValue(const ZSqlInt8& val)");
   bindValue(ZSqlInt32(val,val.isNull()));
} // bindValue(ZSqlInt8)

void ZOraCursor::bindValue(const ZSqlFloat64& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::bindValue(const ZSqlFloat64& val)");
   Column &col = beginBind();

   // verify compatibility with column
   switch (col.htype) {
      case varTypeInt8:
         ((char *)col.buffer)[bndnxt] = val;
         if (((char *)col.buffer)[bndnxt] != val) goto noMatch;
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
   col.ind[bndnxt] = val.isNull() ? -1 : 0;
   if (++bndnxtfld>=bnd.count()) {
      bndnxt++;
      bndnxtfld = 0;
   } // if
} // bindValue(ZSqlFloat64)

void ZOraCursor::bindValue(const ZSqlFloat32& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::bindValue(const ZSqlFloat32& val)");
   bindValue(ZSqlFloat64(val,val.isNull()));
} // bindValue(ZSqlFloat32)

void ZOraCursor::bindValue(const ZSqlChar& val)
{
    ZFUNCTRACE_DEVELOP("ZOraCursor::bindValue(const ZSqlChar& val)");
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
            if (length>col.hsize)
                throwBindValMatchError((const ZString&)val);
            if (col.etype==extTypeLongVarchar || col.etype==extTypeLongVarraw) {
                *(long*)&col.buffer[bndnxt*col.bufsiz] = length;
                memcpy(&col.buffer[bndnxt*col.bufsiz+4],(const char *)val,length);
            } else {
                *(short*)&col.buffer[bndnxt*col.bufsiz] = length;
                memcpy(&col.buffer[bndnxt*col.bufsiz+2],(const char *)val,length);
            } // if
            break;
        default:
            throwSqlError(invType);
    } // switch
    col.ind[bndnxt] = val.isNull() ? -1 : 0;
    if (++bndnxtfld>=bnd.count()) {
        bndnxt++;
        bndnxtfld = 0;
    } // if
} // bindValue(ZString)

/*
 *  REMARK:
 *  oexfet was tested here for select statements, but repeated use was
 *  _not_ possible without reparsing (Pro*C 1.5 and DB 7.0.16.6.0).
 *  So we abandoned use of oexfet because saveing a parse is much more
 *  efficiant than the possible performance gain of oexfet.
 */
ZSqlCursor& ZOraCursor::execute()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::execute()");
   if (sql.word(1).upperCase()=="SELECT" && !sel.count() )
      // auto-define select columns
      for (short pos = 1;;pos++) {
         // retrieve column info to next column
         long    isize, dsize;
         short   type, prec, scale;
         char    cbuf[80];
         long    cbufl = sizeof(cbuf)-1;
         ZTRACE_DEVELOP("odescr");
         if ( odescr( &cda, pos, &isize, &type,
                      cbuf, &cbufl, &dsize, &prec, &scale, NULL ) ) {
            if (cda.csrarc==1007) break;
            throwOraError(lda,&cda,sql.constBuffer(),NULL);
         } // if

         VarType htype = varTypeChar; // c++ type code
         switch (type) {
            case 2: {    // NUMBER,   max.    22 bytes
               int pr = prec ? prec : 38;
               if (!scale && pr<3)   htype = varTypeInt8;    else
               if (!scale && pr<5)   htype = varTypeInt16;   else
               if (!scale && pr<10)  htype = varTypeInt32;   else
//             if (0<prec && pr<7)   htype = varTypeFloat32; else
                                     htype = varTypeFloat64;
               break;
            }
            case 11:    // ROWID,    max.       6 bytes
            case 23:    // RAW,      max.     255 bytes
            case 106:   // MSLABEL,  max.     255 bytes
                htype = varTypeRaw;
                break;
            case 1:     // VARCHAR2, max.    2000 bytes
            case 12:    // DATE,     max.       7 bytes
            case 96:    // CHAR,     max.     255 bytes
                htype = varTypeChar;
                break;
            case 8:     // LONG,     max. 2**31-1 bytes
                htype = varTypeChar;
                dsize = selLongSize;
                break;
            case 24:    // LONG RAW, max. 2**31-1 bytes
                htype = varTypeRaw;
                dsize  = selLongSize;
                break;
            default:;
         } // switch
         ZString data(cbuf,cbufl);
         define(data.constBuffer(),htype,dsize);
         Column &col = sel[sel.count()-1];
         col.descrInfo(type,dsize,prec,scale);
      } /* for */
   if ( bndnxtfld )
      throwSqlError("not all bind vars supplied");
   ZTRACE_DEVELOP("oexn");
   if ( oexn(&cda, bndnxt?bndnxt:1 ,0) )
      throwOraError(lda,&cda,sql.constBuffer(),NULL);
   selcnt = selnxt = selarr;
   bndnxt = 0;
   return *this;
} // execute

int ZOraCursor::fetch()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::fetch()");
   if ( !sel.count() )
      throwSqlError("no select vars available");
   if (++selnxt>=selarr) {                // do we have to fetch more rows ?
      unsigned long rows = cda.csrrpc;
      ZTRACE_DEVELOP("ofen");
      if (ofen(&cda,selarr))
         if (cda.csrarc!=1403)
            throwOraError(lda,&cda,sql.constBuffer(),NULL);
      selnxt = 0;
      selcnt = cda.csrrpc - rows;
   } // if
   if (selnxt < selcnt) selnxtfld = 0;
   return selnxt < selcnt ? 1 : 0;
} // fetch

ZOraCursor::Column& ZOraCursor::beginFetch()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::beginFetch()");
   if (selnxtfld >= sel.count())
      throwSqlError("no more colums in this row");
   return sel[selnxtfld++];
} // beginFetch

void ZOraCursor::fetchValue(ZSqlInt32& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::fetchValue(ZSqlInt32& val)");
   Column     &col = beginFetch();
   if (col.ind[selnxt]<0)
      val.setNull();
   else
   switch (col.htype) {
      case varTypeInt8:
         val = (char)col.buffer[selnxt];
         break;
      case varTypeInt16:
         val = ((short *)col.buffer)[selnxt];
         break;
      case varTypeInt32:
         val = ((long *)col.buffer)[selnxt];
         break;
      case varTypeFloat32:
         val = ((float *)col.buffer)[selnxt];
         break;
      case varTypeFloat64:
         val = ((double *)col.buffer)[selnxt];
         break;
      case varTypeChar:
      case varTypeRaw: {
         ZString buf;
         if (col.etype==extTypeLongVarchar || col.etype==extTypeLongVarraw)
            buf = ZString(&((char*)col.buffer)[selnxt*col.bufsiz+4],
                          *(long*)&col.buffer[selnxt*col.bufsiz] );
         else
            buf = ZString(&((char*)col.buffer)[selnxt*col.bufsiz+2],
                             *(short*)&col.buffer[selnxt*col.bufsiz] );
         val = atol(buf.constBuffer());
         break;
      }
      default:
         throwSqlError(invType);
   } // switch
} // fetchValue(ZSqlInt32&)

void ZOraCursor::fetchValue(ZSqlInt16 &val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::fetchValue(ZSqlInt16& val)");
   ZSqlInt32 x;
   fetchValue(x);
   val = ZSqlInt16(x, x.isNull());
   if (x != val) throwSelValSizeError(1);
} // fetchValue(ZSqlInt16&)

void ZOraCursor::fetchValue(ZSqlInt8& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::fetchValue(ZSqlInt8& val)");
   ZSqlInt32 x;
   fetchValue(x);
   val = ZSqlInt8(x, x.isNull());
   if (x != val) throwSelValSizeError(0);
} // fetchValue(ZSqlInt8&)

void ZOraCursor::fetchValue(ZSqlFloat64& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::fetchValue(ZSqlFloat64& val)");
   Column& col = beginFetch();
   if (col.ind[selnxt]<0)
      val.setNull();
   else
   switch (col.htype) {
      case varTypeInt8:
         val = (char)col.buffer[selnxt];
         break;
      case varTypeInt16:
         val = ((short *)col.buffer)[selnxt];
         break;
      case varTypeInt32:
         val = ((long *)col.buffer)[selnxt];
         break;
      case varTypeFloat32:
         val = ((float *)col.buffer)[selnxt];
         break;
      case varTypeFloat64:
         val = ((double *)col.buffer)[selnxt];
         break;
      case varTypeChar:
      case varTypeRaw: {
         ZString buf;
         if (col.etype==extTypeLongVarchar || col.etype==extTypeLongVarraw)
            buf = ZString(&((char*)col.buffer)[selnxt*col.bufsiz+4],
                          *(long*)&col.buffer[selnxt*col.bufsiz] );
         else
            buf = ZString(&((char*)col.buffer)[selnxt*col.bufsiz+2],
                          *(short*)&col.buffer[selnxt*col.bufsiz] );
         val = atof(buf.constBuffer());
         break;
      }
      default:
         throwSqlError(invType);
   } // switch
} // fetchValue(ZOraFloat64&)

void ZOraCursor::fetchValue(ZSqlFloat32 &val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::fetchValue(ZSqlFloat32& val)");
   ZSqlFloat64 x;
   fetchValue(x);
   val = ZSqlFloat32(x, x.isNull());
   if (x != val) throwSelValSizeError(3);
} // fetchValue(ZSqlFloat32&)

void ZOraCursor::fetchValue(ZSqlChar& val)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::fetchValue(ZSqlChar& val)");
   Column& col = beginFetch();
   if (col.ind[selnxt]<0)
      val.setNull();
   else
   switch (col.htype) {
      case varTypeInt8:
         val = ZString((int)((char *)col.buffer)[selnxt]);
         break;
      case varTypeInt16:
         val = ZString(((short *)col.buffer)[selnxt]);
         break;
      case varTypeInt32:
         val = ZString(((long *)col.buffer)[selnxt]);
         break;
      case varTypeFloat32:
         val = ZString(((float *)col.buffer)[selnxt]);
         break;
      case varTypeFloat64:
         val = ZString(((double *)col.buffer)[selnxt]);
         break;
      case varTypeChar:
      case varTypeRaw:
         if (col.etype==extTypeLongVarchar || col.etype==extTypeLongVarraw)
            val = ZString(&((char*)col.buffer)[selnxt*col.bufsiz+4],
                          *(long*)&col.buffer[selnxt*col.bufsiz] );
         else
            val = ZString(&((char*)col.buffer)[selnxt*col.bufsiz+2],
                          *(short*)&col.buffer[selnxt*col.bufsiz] );
         break;
      default:
         throwSqlError(invType);
   } // switch
} // fetchValue(ZSqlChar&)

unsigned long ZOraCursor::rowsProcessed()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::rowsProcessed()");
   return cda.csrrpc;
} // rowsProcessed

int ZOraCursor::selectColumnCount()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::selectColumnCount()");
   return sel.count();
} // selectColumnCount

char* ZOraCursor::selectColumnName(int index)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::selectColumnName(int index)");
   return sel[index].name;
} // selectColumnName

char* ZOraCursor::selectColumnDescription(int index)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::selectColumnDescription(int index)");
   return sel[index].descr;
} // selectColumnDescription

ZSqlCursor::VarType ZOraCursor::selectColumnType(int index)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::selectColumnType(int index)");
   return sel[index].htype;
} // selectColumnType

int ZOraCursor::selectColumnSize(int index)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::selectColumnSize(int index)");
   return sel[index].hsize;
} // selectColumnSize

int ZOraCursor::selectColumnScale(int index)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::selectColumnScale(int index)");
   return sel[index].hscale;
} // selectColumnScale

///////////////////////////// ZOraCursor::Column //////////////////////////////

ZOraCursor::Column::Column(
   const char* _name,          // column name
   VarType     _htype,         // c++ type code
   long        _hsize,         // size for Char & Raw
   short       _arrsize )       // arraysize
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::Column::Column(...)");
   name    = _name;
   hsize   = _hsize;
   arrsize = _arrsize;
   hscale = 0;
   switch (htype = _htype) {
      case varTypeInt8:
         bufsiz  = sizeof(signed char);
         etype   = extTypeInteger;
         break;
      case varTypeInt16:
         bufsiz  = sizeof(short);
         etype   = extTypeInteger;
         break;
      case varTypeInt32:
         bufsiz  = sizeof(long);
         etype   = extTypeInteger;
         break;
      case varTypeFloat32:
         bufsiz  = sizeof(float);
         etype   = extTypeFloat;
         break;
      case varTypeFloat64:
         bufsiz  = sizeof(double);
         etype   = extTypeFloat;
         break;
      case varTypeChar:
         if (_hsize < 1) _hsize = 2000;
         if (_hsize > 2000) {
            bufsiz  = _hsize+4;
            hsize   = _hsize;
            etype   = extTypeLongVarchar;
         } else {
            bufsiz  = _hsize+2;
            hsize   = _hsize;
            etype   = extTypeVarchar;
         } // if
         break;
      case varTypeRaw:
         if (_hsize < 1) _hsize = 2000;
         if (_hsize > 2000) {
            bufsiz  = _hsize+4;
            hsize   = _hsize;
            etype   = extTypeLongVarraw;
         } else {
            bufsiz  = _hsize+2;
            hsize   = _hsize;
            etype   = extTypeVarraw;
         } // if
         break;
      default:
         throwSqlError("bind/define: unknown VarType");
   } // switch
   if (double(bufsiz)*arrsize > maxLong)
      throwSqlError("bind/define: array buffer size exceeds INT_MAX");
   buffer = new unsigned char[bufsiz*arrsize];
   ind    = new short[arrsize];
} // Column

ZOraCursor::Column::Column(const Column& aColumn) :
   name(aColumn.name),
   descr(aColumn.descr),
   bufsiz(aColumn.bufsiz),
   hsize(aColumn.hsize),
   hscale(aColumn.hscale),
   arrsize(aColumn.arrsize),
   htype(aColumn.htype),
   etype(aColumn.etype)
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::Column::Column(const Column& aColumn)");
   buffer = new unsigned char[bufsiz*arrsize];
   ind    = new short[arrsize];
   memcpy(buffer, aColumn.buffer, bufsiz*arrsize);
   memcpy(ind, aColumn.ind, sizeof(short)*arrsize);
} // copy constructor

ZOraCursor::Column::~Column()
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::Column::~Column()");
   delete [] buffer;
   delete [] ind;
} // ~Column

ZBoolean ZOraCursor::Column::operator==(const Column& aColumn) const
{
   return zTrue; // Columnlist::find() not used
} // operator==

void ZOraCursor::Column::descrInfo(
   short type,    // ORACLE internal type code
   long  size,    // displayed size
   short prec,    // precision for numerics
   short scale )  // scale for numerics
{
   ZFUNCTRACE_DEVELOP("ZOraCursor::Column::descrInfo(...)");
   hscale = 0;
   switch (type) {
      case 1:
         descr = "VARCHAR2("+ZString(size)+")";
         break;
      case 2:
         if (scale<=-127) {
            descr = "FLOAT";
            if (prec<38)
               descr += "("+ZString(prec)+")";
         } else {
            descr = "NUMBER";
            if (prec) {
               descr += "("+ZString(prec);
               if (scale)
                  descr += ","+ZString(scale);
               descr += ")";
            } // if
         } /* endif */
         hsize = prec ? prec : 38;
         hscale = scale>=0 ? scale : scale>-127 ? 0 : -1;
         break;
      case 8:
         descr = "LONG";
         break;
      case 11:
         descr = "ROWID";
         break;
      case 12:
         descr = "DATE";
         break;
      case 23:
         descr = "RAW("+ZString(size)+")";
         break;
      case 24:
         descr = "LONG RAW";
         break;
      case 96:
         descr = "CHAR("+ZString(size)+")";
         break;
      case 106:
         descr = "MLSLABEL";
         break;
      default:
         throwSqlError("unknown ORACLE-internal datatype");
   } // switch
} // descrInfo

ZExport(ZSqlLink*) ZSqlConnectTo(
   const char* aConnection,
   const char* aUsername,
   const char* aPassword,
   int         aMaxCursor)
{
   ZFUNCTRACE_DEVELOP("ZSqlConnectTo(...)");
   return new ZOraLink(aConnection, aUsername, aPassword, aMaxCursor);
} // ZSqlConnectTo
