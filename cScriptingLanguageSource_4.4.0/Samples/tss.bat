@goto start
/*  Copyright (c) 1995-2001 IBK-Landquart-Switzerland. All rights reserved.
 *
 *  Module      :  tss.cmd
 *  Application :  Tiny SQL Shell
 *  Purpose     :  Complex script sample for csl.
 *                 TSS is a simple sql shell that may be useful especially
 *                 in a mixed database environment.
 *  Author      :  Peter Koch, IBK
 *
 *  Date        Description                                 Who
 *  --------------------------------------------------------------------------
 *  1995.04.12  First release                               P.Koch, IBK
 *  1995.06.15  V1.1: New commands:
 *                      HOST/$, CALL/@, REM/REMARK, LOG
 *                    Define script & parameters or
 *                    run autoexec.tss at start             P.Koch, IBK
 *  1997.05.18  V1.2: Added DB2 interface                   P.Koch, IBK
 *  1997.05.05  V1.3: Null value handling                   P.Koch, IBK
 *  2001.06.30  V2.0: Migrated from c++ to css as script    P.Koch, IBK
 *                    sample.
 *  2001.07.05  V2.1: Support ODBC interface                P.Koch, IBK
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
#loadLibrary 'ZcSysLib'
#loadLibrary 'ZcStrLib'
#loadLibrary 'ZcDaxLib'
#loadLibrary 'ZcFileLb'

const MAXNESTING = 15;              // max. nesting level

const dbName[4] = { "MYSQL", "ORACLE", "DB2", "ODBC" };

static var mainLink;               // main link
static var istr[MAXNESTING];       // input stream stack
static var ipara[MAXNESTING][10];  // parameters %0 ... %9
static var input;                  // user input line
static var lname;                  // logfile name
static var connected   = 0;        // connect status
static var numwidth    = 40;       // max.numeric width
static var charwidth   = 40;       // max.character width
static var pause       = 24;       // pause after x lines
static var database    = 0;        // database type
static var output      = 0;        // output format
static var echo        = 1;        // echo mode
static var ipos        = 0;        // input stream ptr

/*
 *  p a u s e c h k
 *
 *  increment linecounter and pause output
 */
static pausechk(var &pcnt)
{
   if (++pcnt==pause) {
      pcnt = 0;
      fileWrite(fileStdOut, 'pause...  (enter "x" to abort) ');
      var inp =
         strUpper(
            strSubString(
               strStripLeading(
                  fileReadLine(fileStdIn)
               )
               ,1,1
            )
         );
      if (inp=="X") {
         sysLog("operation aborted.");
         return 1;
      } // if
   } // if
   return 0;
} // pausechk

/*
 *  d e s c r i b e
 *
 *  describe columns of a table
 */
static describe()
{
   if (!connected) {
      sysLog("Not connected!");
      return;
   } // if
   var table = strWords(input, 2);
   if (table=="") {
      sysLog("Missing table name!");
      return;
   } // if

   var csr;
   if (dbName[database] == "ORACLE")
      csr = daxParse(mainLink, "SELECT * FROM "|table|" WHERE ROWNUM<1");
   else
      csr = daxParse(mainLink, "SELECT * FROM "|table);
   try {
      const cols = daxSelectColumns(csr);
      if (cols) {
         var pcnt = 0;
         for (var col = 0; col < cols; col++) {
            var txt = strExport(daxSelectColumnName(csr,col));
            if (strLength(txt) > 20) txt = strSubString(txt,1,20);
            var tp = strExport(daxSelectColumnType(csr,col));
            if (strLength(tp) > 57) tp = strSubString(txt,1,57);
            sysLog(strLeftJustify(txt,20,'.')|': '|tp);
            if (pausechk(pcnt)) break;
         } // for
      } else {
         sysLog("No columns in table "|table);
      } // if
   } // try
   catch (const err[]) {
      daxDispose(csr);
      throw err;
   } // catch
   daxDispose(csr);
} // describe

static var numeric(const csr, const index)
{
   var num = false;
   var tp = daxSelectColumnType(csr,index);
   switch (dbName[database]) {
      case "MYSQL":
         if (strSubString(tp,1,7)=="DECIMAL" ||
             tp=="TINY"                      ||
             tp=="SHORT"                     ||
             tp=="LONG"                      ||
             tp=="FLOAT"                     ||
             tp=="DOUBLE"                    ||
             tp=="LONGLONG"                  ||
             tp=="INT24"                     ||
             tp=="YEAR"                      ||
             tp=="ENUM")
            num = true;
         break;
      case "DB2":
      case "ODBC":
         if (strSubString(tp,1,7)=="DECIMAL" ||
             strSubString(tp,1,7)=="NUMERIC" ||
             tp=="SMALLINT"                  ||
             tp=="INTEGER"                   ||
             tp=="FLOAT"                     ||
             tp=="REAL"                      ||
             tp=="DOUBLE")
            num = true;
         break;
      case "ORACLE":
         if (strSubString(tp,1,6)=="NUMBER"  ||
             tp=="FLOAT")
            num = true;
         break;
      default:;
   } // switch
   return num;
} // numeric

/*
 *  v e r t i c a l
 *
 *  show select results in vertical direction
 */
static vertical(const csr, const cols)
{
   var pcnt = 0;
   var vals[cols*2];
   while (daxFetch(csr, vals, true)) {
      sysLog(strLeftJustify('',79,'-'));
      if (pausechk(pcnt)) break;
      for (var col = 0; col < cols; col++) {
         var buf;
         var txt = strExport(daxSelectColumnName(csr,col));
         if (strLength(txt) > 20) txt = strSubString(txt,1,20);
         buf |= strLeftJustify(txt,20,'.')|': ';
         var val = strExport(vals[col*2]);
         if (vals[col*2+1]) val = "NULL";
         while (strLength(val) > 57) {
            sysLog(buf|strSubString(val, 1, 57));
            if (pausechk(pcnt)) return;
            val = strSubString(val, 58);
            buf = strLeftJustify('',22);
         } // while
         sysLog(buf|val);
         if (pausechk(pcnt)) return;
      } // for
   } // while
} // vertical

/*
 *  h o r i z o n t a l
 *
 *  show select results in horizontal direction
 */
static horizontal(const csr, const cols)
{
   // show heading
   var buf, num[cols], width[cols];
   for (var col = 0; col < cols; col++) {
      var w = daxSelectColumnSize(csr,col);
      var n = num[col] = numeric(csr,col);
      if (n) {
         if (numwidth > 0 && w > numwidth) w = numwidth;
      } else {
         if (charwidth > 0 && w > charwidth) w = charwidth;
      } // if
      width[col] = w;
      var txt = strExport(daxSelectColumnName(csr,col));
      if (strLength(txt) > w) txt = strSubString(txt,1,w);
      if (n)
         buf |= strRightJustify(txt, w);
      else
         buf |= strLeftJustify(txt, w);
      if (col < cols-1) buf |= ' ';
   } // for
   sysLog(buf);

   // underline heading
   buf = '';
   for (col = 0; col < cols; col++) {
      if (width[col])
         buf |= strLeftJustify('', width[col], '-');
      if (col < cols-1) buf |= ' ';
   } // for
   sysLog(buf);

   // show fetched columns
   var pcnt = 2;
   var vals[cols*2];
   while (daxFetch(csr, vals, true)) {
      buf = '';
      for (var col = 0; col < cols; col++) {
         var w = width[col];
         var val = vals[col*2];
         if (vals[col*2+1]) val = "NULL";
         if (strLength(val) > w) val = strSubString(val,1,w);
         if (num[col])
            buf |= strRightJustify(val, w);
         else
            buf |= strLeftJustify(strExport(val), w);
         if (col < cols-1) buf |= ' ';
      } // for
      sysLog(buf);
      if (pausechk(pcnt)) break;
   } // while
} // horizontal

/*
 *  e x e c u t e
 *
 *  execute any sql statement
 */
static execute()
{
   if (!connected) {
      sysLog("Not connected!");
      return;
   } // if
   var csr = daxParse(mainLink, input);
   try {
      const cols = daxSelectColumns(csr);
      if (cols) {
         if (output)
            vertical(csr, cols);
         else
            horizontal(csr, cols);
         sysLog(daxRowsProcessed(csr)|" row(s) selected.");
      } else {
         sysLog(daxRowsProcessed(csr)|" row(s) processed.");
      } // if
   } // try
   catch (const err[]) {
      daxDispose(csr);
      throw err;
   } // catch
   daxDispose(csr);
} // execute

/*
 *  r o l l b a c k
 *
 *  rollback work
 */
static rollback()
{
   if (connected) {
      daxRollback(mainLink);
      sysLog("Rolled back.");
   } else
      sysLog("Not connected!");
} // rollback

/*
 *  c o m m i t
 *
 *  commit any changes
 */
static commit()
{
   if (connected) {
      daxCommit(mainLink);
      sysLog("Commited.");
   } else
      sysLog("Not connected!");
} // commit

/*
 *  d i s c o n n e c t
 *
 *  disconnect from database
 */
static disconnect()
{
   if (connected) {
      daxDisconnect(mainLink);
      sysLog("Disconnected.");
      connected = 0;
   } else
      sysLog("Not connected!");
} // disconnect

/*
 *  c o n n e c t
 *
 *  connect to database
 */
static connect()
{
   if (connected) disconnect();
   var db, user, pass, conn;
   user = strSplitConnectString(strWords(input,2), pass, conn, db);
   if (db == '')
      db = dbName[database];
   else {
      db = strUpper(db);
      var found = false;
      for (var i=0; i < sizeof(dbName); i++)
         if (db==dbName[i]) {
            database = i;
            found = true;
            break;
         } // if
      if (!found) {
         sysLog("Unknown database type: "|db);
         return;
      } // if
   } // if
   mainLink = daxConnect(dbName[database], conn, user, pass);
   sysLog("Connected to "|daxDatabase(mainLink));
   connected = 1;
} // connect

/*
 *  h o s t
 *
 *  execute host command
 */
static host()
{
   const cmd = strStrip(strWords(input, 2));
   if (cmd=="") {
      sysLog("Type 'EXIT' to return to TSS");
      fileFlush(fileStdOut);
#if sysIsWinNTFamily | sysIsOS2
      sysCommand("cmd");
#endif
#if sysIsWin95Family
      sysCommand("command");
#endif
#if sysIsLinux
      sysCommand("sh");
#endif
   } else {
      fileFlush(fileStdOut);
      sysCommand(cmd);
   } // if
} // host

/*
 *  s e t l o g
 *
 *  handle log command
 */
static setlog()
{
   var file = strStrip(strWords(input,2));
   var fileu = strUpper(file);
   if (fileu=="CLOSE" && lname=="") {
      sysLog("no logfile open");
      return;
   } // if
   if (lname!="") sysLogFile();
   if (fileu=="CLOSE") {
      lname = "";
      return;
   } // if
   lname = file;
   sysLogFile(file);
} // setlog

/*
 *  s h o w
 *
 *  show current settings
 */
static show()
{
   var topic = strUpper(strWords(input,2,1));
   if (topic=="") topic = "ALL";

   if (topic=="CHARWIDTH"|| topic=="ALL")
      sysLog(        "CHARWIDTH   = "|charwidth);

   if (topic=="DATABASE" || topic=="ALL")
      sysLog("DATABASE    = "|dbName[database]);

   if (topic=="ECHO" || topic=="ALL") {
      if (echo)
         sysLog("ECHO        = ON");
      else
         sysLog("ECHO        = OFF");
   }

   if (topic=="LOG" || topic=="ALL") {
      if (lname=="")
         sysLog("LOG         = CLOSED");
      else
         sysLog("LOG         = "|lname);
   }

   if (topic=="NUMWIDTH" || topic=="ALL")
      sysLog("NUMWIDTH    = "|numwidth);

   if (topic=="OUTPUT" || topic=="ALL") {
      if (output)
         sysLog("OUTPUT      = VERTICAL");
      else
         sysLog("OUTPUT      = HORIZONTAL");
   }

   if (topic=="PAUSE" || topic=="ALL")
      sysLog("PAUSE       = "|pause);

   if (topic=="PARAMS" || topic=="ALL") {
      for (var p = 0; p < 10; p++) {
         if (ipara[ipos][p]=="") break;
         sysLog("%"|p|"          = "|ipara[ipos][p]);
      } // for
   } // if
} // show

/*
 *  s e t
 *
 *  set globals
 */
static set()
{
   const topic = strUpper(strWords(input,2,1));
   const word3u = strUpper(strWords(input,3,1));
   if (topic=="CHARWIDTH") {
      if (strIsInteger(word3u))
         charwidth = word3u;
      else
         sysLog("Integer value expected!");
   } else
   if (topic=="DATABASE") {
      for (var i=0; i < sizeof(dbName); i++)
         if (word3u==dbName[i]) {
            database = i;
            return;
         } // if
      sysLog("Unknown database type: "+word3u);
   } else
   if (topic=="ECHO") {
      if (word3u=="OFF" || word3u=="")
         echo = 0;
      else
      if (word3u=="ON")
         echo = 1;
      else
         sysLog("Unknown echo mode: "+word3u);
   } else
   if (topic=="NUMWIDTH") {
      if (strIsInteger(word3u))
         numwidth = word3u;
      else
         sysLog("Integer value expected!");
   } else
   if (topic=="OUTPUT") {
      if (word3u=="VERTICAL" || word3u=="")
         output = 1;
      else
      if (word3u=="HORIZONTAL")
         output = 0;
      else
         sysLog("Unknown direction: "|word3u);
   } else
   if (topic=="PAUSE") {
      if (strIsInteger(word3u))
         charwidth = word3u;
      else
         sysLog("Integer value expected!");
      pause = word3u;
   } else
      sysLog("Unknown topic: "|topic);
} // set

/*
 *  h e l p
 *
 *  show help screen
 */
static help()
{
    const txt = {
"Program startup:",
"   tss [filename[.sql] parameter1 parameter2 ... parameter9]",
"       starts TSS and optionaly runs a scriptfile.",
"       Placeholders %1 ... %9 in the scriptfile are substituted by parameters",
"       1...9. Parameters containing blanks must be enclosed in quotes e.g.",
"       \"Carol Smith\". %0 will be substituted by the scriptfile name.",
"",
"       If TSS is started without any parameters, it looks for a file named",
"       \"autoexec.tss\" and runs it if found. You may use such a file to set",
"       your preferences and automaticly connect to a database for example.",
"",
"SQL instruction execution:",
"   [sql instruction];[remark]",
"       The sql instruction may be spawned over several lines. Execution starts",
"       when the semicolon ist found. Lines starting with a semicolon are trea-",
"       ted as remark lines.",
"",
"TSS commands:",
"   TSS commands may be terminated by a semicolon. The remainder of the line is",
"   treated as remark in this case.",
"",
"   CALL <filename[.sql]> [parameter1 parameter2 ... parameter9]",
"   @<filename[.sql]> [parameter1 parameter2 ... parameter9]",
"       Execute a script file. For parameter description look at \"Program",
"       startup\".  Passing placeholders of parameters which may contain",
"       blanks must also by done enclosed in quotes, e.g. (look at %2):",
"               @adduser \"John Ross\" Ewing \"%2\" %3",
"",
"   COMMIT",
"       Commit all changes made by DML-statements (INSERT,UPDATE,DELETE).",
"",
"   CONNECT connect-string",
"       Connect to database.",
"       \"connectstring\" has form [db-type:][userid][/password][@connection]",
"       Where \"connection\" is:",
"          DB2   : database alias",
"          MYSQL : database[:host[:port[:socket]]]",
"          ORACLE: sql*net name",
"          ODBC  : data source name",
"       Examples:",
"          mysql:@test (default user on local mysql db \"test\")",
"          oracle:scott/tiger@sales",
"          tom/shark (user tom with password shark at current db-type)",
"",
"   DISCONNECT",
"       Disconnect (logoff) from database.",
"",
"   DESCRIBE <tablename>",
"       Describe the columns of a table.",
"",
"   EXIT",
"       Terminate TSS immediatly.",
"",
"   HELP or ?",
"       Show this help information.",
"",
"   HOST [command]  or  $[command]",
"       Execute command by host shell (cmd). If no command is given, a host",
"       shell session is opened in interactive mode.  Enter EXIT to return",
"       to TSS.",
"",
"   LOG <filename[.log]|CLOSE>",
"       Start logging to specified logfile.  All output produced by TSS will be",
"       appended to the file until LOG CLOSE is entered.  To print the output",
"       immediatly use \"prn.\" as filename (don't forget the dot!).",
"",
"   REM  or  REMARK",
"       Remark line.  A remark line may also be started by a semicolon.",
"",
"   RETURN",
"       Return to previous calling level.  From top level return acts like",
"       EXIT.",
"",
"   ROLLBACK",
"       All DML changes since last COMMIT or ROLLBACK are rolled back.",
"",
"   SET CHARWIDTH <value>",
"       Set maximum displayed character column width.  A value of 0 denotes",
"       unlimited width.",
"",
"   SET DATABASE <name>",
"       Select default database type for next CONNECT, e.g. \"ORACLE\", \"DB2\"",
"       \"MYSQL\", \"ODBC\" etc.",
"",
"   SET ECHO <mode>",
"       Turn echo of TSS input \"ON\" or \"OFF\".  This is fully true for",
"       scriptfile execution only; in interactive mode this parameter effects",
"       logging sole.",
"",
"   SET NUMWIDTH <value>",
"       Set maximum displayed numeric column width. A value of 0 denotes",
"       unlimited width.",
"",
"   SET OUTPUT <mode>",
"       Define presentation mode of select query results:",
"       \"HORIZONTAL\" or \"VERTICAL\".",
"",
"   SET PAUSE <value>",
"       Pause after <value> lines of output.  A value of 0 turns off pausing.",
"",
"   SHOW <topic>",
"       Show setting of single or all topic(s):",
"       ALL CHARWIDTH DATABASE ECHO LOG NUMWIDTH OUTPUT PAUSE PARAMS"
   };

   var pcnt = 0;
   for (var ln = 0; ln < sizeof(txt); ln++) {
      sysLog(txt[ln]);
      if (pausechk(pcnt)) break;
   } // for
} // help

/*
 *  e x p a n d
 *
 *  expand parameters on input line
 */
static expand(var &line)
{
   for (var p = 0; p < 10; p++) {
      if (ipara[ipos][p]=="") break;
      strChange(line, "%"|p, ipara[ipos][p]);
   } // for
} // expand

/*
 *  g e t p a r a m
 *
 *  get next parameter from line
 */
static var getparam(var &line)
{
   var param;
   line = strStripLeading(line);
   const len = strLength(line);
   if (len > 0) {
      var pos = 1;
      const delim = strSubString(line,pos,1);
      if (delim == '"' || delim =="'") {
         pos++;
         while (pos <= len) {
            if (strSubString(line,pos,1) == delim)
               break;
            pos++;
         } // while
         param = strSubString(line,2,pos-2);
      } else {
         while (pos <= len) {
            if (strIndexOf(" \t", strSubString(line,pos,1)))
               break;
            pos++;
         } // while
         param = strSubString(line,1,pos-1);
       } /* if */
       if (pos <= len) pos++;
       line = strSubString(line,pos);
   } // if
   return param;
} // getparam

/*
 *  r e d i r e c t
 *
 *  redirect input to file
 */
static redirect(var msg)
{
   if (ipos>=MAXNESTING-1) {
      sysLog("nesting of input streams too deep!");
      return;
   } // if
   ipara[++ipos][0] = strWords(input, 2, 1);
   if (strLastIndexOf(ipara[ipos][0],".")<=strLastIndexOf(ipara[ipos][0],PATHSEPARATOR))
      ipara[ipos][0] |= ".sql";
   var line = strRemoveWords(input, 1, 2);
   for (var p = 1; p < 10; p++)
      ipara[ipos][p] = getparam(line);
   try {
      istr[ipos] = fileOpen(ipara[ipos][0], fileOpenRead);
   } // try
   catch (const exc[]) {
      if (msg)
         sysLog("input file "|ipara[ipos][0]|" could not be opened!");
      ipos--;
   } // catch
} // redirect

/*
 *  m a i n
 */
main()
{
   sysLog("Tini SQL Shell V2.1");
   sysLog("Copyright (C) 1995-2002 by IBK-Landquart-Switzerland");
   sysLog("Enter '?' or 'HELP' for instructions.");
   istr[ipos] = fileStdIn;
   ipara[ipos][0] = "standard input";
   if (sizeof(mainArgVals) > 2) {
      var p = 2;
      input = "@ "|+mainArgVals[p++];
      while (p < sizeof(mainArgVals))
        input |= ' "'|mainArgVals[p++]|'"';
      sysLog(input);
      redirect(true);
   } else {
      input = "@ autoexec.tss";
      redirect(false);
   } // if
   for (;;) {
      var semiColon = false;
      var word1;
      if (fileEof(istr[ipos])) {
         sysLog("eof on "|ipara[ipos][0]);
         if (ipos) {
            fileClose(istr[ipos--]);
            continue;
         } else
            break;
      } // if
      if (!ipos) {
         fileWrite(fileStdOut, "TSS> ");
         fileFlush(fileStdOut);
      } // if
      input = fileReadLine(istr[ipos]);
      expand(input);
      if (ipos && echo)
         sysLog("TSS> "|input);
      if (semiColon = strLastIndexOf(input, ';'))
         input = strSubString(input,1,semiColon-1);
      input = strStrip(input);
      switch (strSubString(input,1,1)) {
         case '?':
         case '$':
         case '@':
            input = strSubString(input,1,1)|' '|strSubString(input,2);
         default:;
      } // switch
      word1 = strUpper(strWords(input,1,1));
      if (word1=="RETURN") {
         if (ipos) {
            fileClose(istr[ipos--]);
            continue;
         } else
            break;
      } // if
      if (word1=="EXIT") {
         break;
      } else  {
         try {
            if (word1=="REM" || word1=="REMARK") ; else
            if (word1=="HELP" || word1=="?"    ) help(); else
            if (word1=="SHOW"                  ) show(); else
            if (word1=="SET"                   ) set(); else
            if (word1=="LOG"                   ) setlog(); else
            if (word1=="HOST" || word1=="$"    ) host(); else
            if (word1=="CONNECT"               ) connect(); else
            if (word1=="DISCONNECT"            ) disconnect(); else
            if (word1=="COMMIT"                ) commit(); else
            if (word1=="ROLLBACK"              ) rollback(); else
            if (word1=="CALL" || word1=="@"    ) redirect(true); else
            if (word1=="DESCRIBE"              ) describe(); else
            if (word1!=""                      ) {
               while (!semiColon) {
                  if (!ipos) {
                     fileWrite(fileStdOut, "   > ");
                     fileFlush(fileStdOut);
                  } // if
                  var nxt = fileReadLine(istr[ipos]);
                  expand(nxt);
                  if (fileEof(istr[ipos])) break;
                  if (ipos && echo) sysLog("   > "|nxt);
                  if (semiColon = strLastIndexOf(nxt,';'))
                     nxt = strSubString(nxt,1,semiColon-1);
                  input |= " "|strStrip(nxt);
               } // while
               execute();
            } // if
         } // try
         catch (const err[]) {
           for (var i = 0; i < sizeof(err); i++) sysLog(err[i]);
         } // catch
      } // if
   } // for
   if (connected) disconnect();
   while (ipos) fileClose(istr[ipos--]);
   return 0;
} // main
/*
:start
@echo off
csl %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
rem */
