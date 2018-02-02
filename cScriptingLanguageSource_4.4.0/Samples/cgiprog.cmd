@goto exec
#loadLibrary 'ZcSysLib'
#loadLibrary 'ZcStrLib'
#loadLibrary 'ZcFileLb'

static const maxArgs   = 20;             // max. # of arguments to handle

static const ARG_NAME  = 0;              // arg name index
static const ARG_VALUE = 1;              // arg name index

static var method;                       // request method - GET or POST
static var input;                        // cgi input string
static var inLen;                        // input string length
static var inPos = 1;                    // parse input position
static var inChar = ' ';                 // parse input character
static var argCnt = 0;                   // argument count
static var arg[maxArgs][2];              // arguments

/*
 * n e x t C h a r
 *
 * fetch next char from cgi input string
 */
static var nextChar()
{
   if (inPos <= inLen)
      inChar = strSubString(input, inPos++, 1);
   else
      inChar = '';
   return inChar;
} // nextChar

/*
 * p a r s e R e q u e s t
 *
 * parse the cgi request
 */
static parseRequest()
{
   // get input data
   method = strUpper(sysEnvVar('REQUEST_METHOD'));
   if (method == '')
      throw 'Environment variable REQUEST_METHOD ist not defined!';
   if (method == 'GET') {
      input = strChange(sysEnvVar('QUERY_STRING'), '&', ' ');
      inLen = strLength(input);
   } else
      if (method == 'POST') {
         inLen = sysEnvVar('CONTENT_LENGTH');
         if (inLen == '')
            throw 'Environment variable CONTENT_LENGTH ist not defined!';
         input = strChange(fileRead(fileStdIn, inLen), '&', ' ');
      } else
         throw 'Invalid request method: '|method;

   // parse the parameters
   while (inChar != '') {
      while (inChar == ' ') nextChar(); // skip blanks

      // read the name until '='
      var name;
      while (inChar!='' && inChar!=' ' && inChar!='=') {
         // convert excaped hex chars
         if (inChar == '%')
            inChar = strImport('\\x'|nextChar()|nextChar());
         else
            if (inChar == '+')
               inChar = ' ';
         name |= inChar;
         nextChar();
      } // while

      // now read the value
      var value;
      if (inChar == '=') {
         nextChar();
         while (inChar!='' && inChar!=' ') {
            // convert excaped hex chars
            if (inChar == '%')
               inChar = strImport('\\x'|nextChar()|nextChar());
            else
               if (inChar == '+')
                  inChar = ' ';
            value |= inChar;
            nextChar();
         } // while
      } // while

      // save this parameter
      if (argCnt == maxArgs) throw 'Too many input arguments';
      arg[argCnt][ARG_NAME] = strUpper(name);
      arg[argCnt++][ARG_VALUE] = value;
   } // while
} // parseRequest

/*
 * f i n d A r g
 *
 * find an argument by name
 */
static var findArg(const name)
{
   var srch = strUpper(name);
   var found = false;
   var value;
   for (var i = 0; i < argCnt; i++)
      if (srch == arg[i][ARG_NAME]) {
         value = arg[i][ARG_VALUE];
         found = true;
         break;
      } // if
   if (!found) throw 'Argument "' | name | '" was not passed!';
   return value;
} // findArg

/*
 * e m i t
 *
 * emit a line
 */
static emit([var text])
{
   // NOTE:
   //   I used fileWriteLine here to document what's going on.
   //   Anyway sysLog() would do basicly the same as emit,
   //   with the additional option of saving to a logfile.
   if (argCount == 1)
      fileWriteLine(fileStdOut, text);
   else
      fileWriteLine(fileStdOut);
} // emit

main()
{
   // prepare CGI output
   emit('Content-Type: text/html');
   emit();

   // start of HTML output
   emit('<html>');
   emit(  '<head>');
   emit(    '<title>Sample CGI Output</title>');
   emit(  '</head>');
   emit(  '<body>');

   try {
      parseRequest(); // parse the CGI request
      emit('Request mode: ' | method | '<br><br>');
      emit('Unparsed input:<br>' | input | '<br><br>');


      emit('Parsed arguments:<br><br>');
      emit('<table border=1>');
      for (var i = 0; i < argCnt; i++) {
         emit('<tr>');
         emit(  '<td>' | arg[i][ARG_NAME] | '</td>');
         emit(  '<td>' | arg[i][ARG_VALUE] | '</td>');
         emit('</tr>');
      } // for
      emit('</table><br>');


      emit('Querying some args:<br><br>');
      const argnames = {
         'user',
         'email',
         'file'
      };
      emit('<table border=1>');
      for (i = 0; i < sizeof(argnames); i++) {
         var val;
         try {
            val = findArg(argnames[i]);
         } // try
         catch (var exc[]) {
            val = 'argument not passed!';
         } // catch
         emit('<tr>');
         emit(  '<td>' | argnames[i] | '</td>');
         emit(  '<td>' | val | '</td>');
         emit('</tr>');
      } // for
      emit('</table><br>');


      emit('Some other CGI values:<br><br>');
      const vars = {
         'REMOTE_HOST',
         'REMOTE_ADDR',
         'SCRIPT_NAME',
         'SERVER_NAME',
         'SERVER_SOFTWARE',
         'SERVER_PORT'
      };
      emit('<table border=1>');
      for (i = 0; i < sizeof(vars); i++) {
         emit(  '<tr>');
         emit(    '<td>' | vars[i] | '</td>');
         emit(    '<td>' | sysEnvVar(vars[i]) | '</td>');
         emit(  '</tr>');
      } // for
      emit('</table><br>');

   } // try
   catch (var exc[]) {
      emit(exc[0] | '<br>');
   } // catch

   // end of html output
   emit(  '</body>');
   emit('</html>');
} // main

/*
:exec
@echo off
rem set path=c:\csl\win;%path%
csl %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
rem */
