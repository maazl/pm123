/* This REXX script installs the PM123 objects on your WPS desktop. */

Call RxFuncAdd 'SysLoadFuncs', 'REXXUTIL', 'SysLoadFuncs'
Call SysLoadFuncs

say "PM123 Desktop Installation Utility"
say ""
say "Are you sure you want to create a PM123 folder with a PM123"
say "program object associated with WAV, MP2, MP3, M3U, PLS and LST files?"
say ""
say "(y or n)? "

key = SysGetKey('NOECHO')
parse upper var key key
if key <> 'Y' then call end

dir = directory()

say "Creating PM123 folder..."
if SysCreateObject( "WPFolder", "PM123", "<WP_DESKTOP>",,
   "OBJECTID=<PM123FOLDER>;", "REPLACE" ) = 0 then
   call error

say "Creating PM123 associations folder..."
if SysCreateObject( "WPFolder", "Associations", "<PM123FOLDER>",,
   "OBJECTID=<PM123ASSOTIATIONS>;", "REPLACE" ) = 0 then
   call error

say "Creating PM123 PDK folder..."
if SysCreateObject( "WPFolder", "PDK", "<PM123FOLDER>",,
   "OBJECTID=<PM123PDK>;", "REPLACE" ) = 0 then
   call error

say "Creating PM123 program object..."
if SysCreateObject( "WPProgram", "PM123", "<PM123FOLDER>",,
   "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123>;ICONFILE="||dir||"\PM123.EXE;STARTUPDIR="||dir||";", "REPLACE") = 0 then
   call error

say "Creating PM123 online help..."
if SysCreateObject( "WPProgram", "Online Help", "<PM123FOLDER>",,
   "PARAMETERS="||dir||"\PM123.INF;PROGTYPE=PM;EXENAME=VIEW.EXE;OBJECTID=<PM123_HELP>;", "REPLACE") = 0 then
   call error

say "Creating PM123 history..."
if SysCreateObject( "WPProgram", "History of Changes", "<PM123FOLDER>",,
   "PARAMETERS="||dir||"\PM123.INF History of Changes;PROGTYPE=PM;EXENAME=VIEW.EXE;OBJECTID=<PM123_HISTORY>;", "REPLACE") = 0 then
   call error

say "Creating PM123 support & contacts..."
if SysCreateObject( "WPProgram", "Support & Contacts", "<PM123FOLDER>",,
   "PARAMETERS="||dir||"\PM123.INF Support & Contact;PROGTYPE=PM;EXENAME=VIEW.EXE;OBJECTID=<PM123_CONTACTS>;", "REPLACE") = 0 then
   call error

say "Creating PM123 Plug-in Developer's Guide..."
if SysCreateObject( "WPProgram", "Plug-in Developer's Guide", "<PM123PDK>",,
   "PARAMETERS="||dir||"\PDK\PM123_PDK.INF;PROGTYPE=PM;EXENAME=VIEW.EXE;OBJECTID=<PM123_PDK_GUIDE>;", "REPLACE") = 0 then
   call error

say "Creating FFTW Guide..."
if SysCreateObject( "WPShadow", "FFTW Guide", "<PM123PDK>",,
   "SHADOWID="||dir||"\PDK\FFT123.PDF;OBJECTID=<PM123_FFT_GUIDE>;", "REPLACE") = 0 then
   call error

extensions = "WAV AIF AU AVR CAF IFF MAT PAF PVF SD2 SDS SF VOC W64 XI " ||,
             "MP1 MP2 MP3 M3U PLS MPL LST M3U8 OGG"

do i = 1 to words( extensions )
   ext = word( extensions, i )
   say "Associating PM123 with *."ext" files..."

   if SysCreateObject("WPProgram", "PM123["ext"]", "<PM123ASSOTIATIONS>",,
      "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123_"ext">;ICONFILE="||dir||"\icons\"ext".ico;STARTUPDIR="||dir||";ASSOCFILTER=*."ext";", "REPLACE" ) = 0 then do
      call error
   end
end

call SysDestroyObject "<PM123MANUAL>"
call SysDestroyObject "<PM123SKINSMANUAL>"
call SysDestroyObject "<PM123NETSCAPE>"
call SysDestroyObject "<PM123HISTORY>"

say "Objects creation was successful!"
call end

error:
  say "Object creation was NOT successful!"
  call end

end:
  exit 0
