@goto start
/*  Copyright (c) 2001 IBK-Landquart-Switzerland. All rights reserved.
 *
 *  Module      :  Unix2Dos.bat
 *  Application :  Convert textfiles from Unix to Dos format
 *  Author      :  Peter Koch, IBK
 *
 *  Date        Description                                 Who
 *  --------------------------------------------------------------------------
 *  2001.06.17  First release                               P.Koch, IBK
 */

#loadLibrary 'ZcSysLib'
#loadLibrary 'ZcStrLib'
#loadLibrary 'ZcFileLb'

convert(const file)
{
   var inp;
   try { inp = fileOpen(file, fileOpenRead); }
   catch (const err[]) {}
	
   sysLog(file);
   var out =
      fileOpen(
         file|'.tmp',
         fileOpenWrite+fileOpenBinary+fileOpenTrunc);
   var blankLines = 0;
   while (!fileEof(inp)) {
      var line = strStripTrailing(fileReadLine(inp));
      if (line == '\32') break; // ctrl-z
      if (line == '')
         blankLines++;
      else {
	 while (blankLines) {
            fileWrite(out, '\r\n');
            blankLines--;
         } // while
         fileWrite(out, line|'\r\n');
      } // if
   } // while
   fileClose(inp);
   fileClose(out);
   fileDelete(file);
   fileRename(file|'.tmp',file);
} // convert

main()
{
   var a = 2;
   var options;
   var filemask;
   var ok = true;
   while (a < sizeof(mainArgVals)) {
      var token = mainArgVals[a++];
      if (strUpper(token) == '/S')
         options |= token;
      else
	     if (filemask == '')
            filemask = token;
         else
            ok = false;
   } // if
   if (filemask=='' || ok==false) {
      var msg = {
         'Usage: ',
         '  Unix2Dos filemask [options]',
         '',
         '  Options:',
         '    /S   handle subdirectories'
      };
      throw msg;
   } // if

   var tmp = fileTempName();
   var rc = sysCommand('dir '|filemask|' '|options|' /B /A-D >'+tmp+' 2>nul');
   if (rc)
      throw 'system error ('+rc+') querying file list';
   var fh = fileOpen(tmp, fileOpenRead);
   while (!fileEof(fh)) {
      var file = fileReadLine(fh);
      if (file != '') convert(file);
   } // while
   fileClose(fh);
   fileDelete(tmp);
} // main
/*
:start
@csl %0 %1 %2 %3 %4
@rem */
