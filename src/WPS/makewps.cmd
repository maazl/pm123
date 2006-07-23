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

say "Creating PM123 Plug-in Developer's Guide..."
if SysCreateObject( "WPProgram", "Plug-in Developer's Guide", "<PM123PDK>",,
   "PARAMETERS="||dir||"\PDK\PM123_PDK.INF;PROGTYPE=PM;EXENAME=VIEW.EXE;OBJECTID=<PM123_PDK_GUIDE>;", "REPLACE") = 0 then
   call error

say "Creating FFTW Guide..."
if SysCreateObject( "WPShadow", "FFTW Guide", "<PM123PDK>",,
   "SHADOWID="||dir||"\PDK\FFT123.PDF;OBJECTID=<PM123_FFT_GUIDE>;", "REPLACE") = 0 then
   call error

say "Associating PM123 with WAV files..."
if SysCreateObject("WPProgram", "PM123[WAV]", "<PM123ASSOTIATIONS>",,
   "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123_WAV>;ICONFILE="||dir||"\icons\wav.ico;STARTUPDIR="||dir||";ASSOCFILTER=*.WAV;", "REPLACE" ) = 0 then
   call error

say "Associating PM123 with MP2 files..."
if SysCreateObject("WPProgram", "PM123[MP2]", "<PM123ASSOTIATIONS>",,
   "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123_MP2>;ICONFILE="||dir||"\icons\mp2.ico;STARTUPDIR="||dir||";ASSOCFILTER=*.MP2;", "REPLACE" ) = 0 then
   call error

say "Associating PM123 with MP3 files..."
if SysCreateObject("WPProgram", "PM123[MP3]", "<PM123ASSOTIATIONS>",,
   "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123_MP3>;ICONFILE="||dir||"\icons\mp3.ico;STARTUPDIR="||dir||";ASSOCFILTER=*.MP3;", "REPLACE" ) = 0 then
   call error

say "Associating PM123 with M3U files..."
if SysCreateObject("WPProgram", "PM123[M3U]", "<PM123ASSOTIATIONS>",,
   "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123_M3U>;ICONFILE="||dir||"\icons\m3u.ico;STARTUPDIR="||dir||";ASSOCFILTER=*.M3U;", "REPLACE" ) = 0 then
   call error

say "Associating PM123 with PLS files..."
if SysCreateObject("WPProgram", "PM123[PLS]", "<PM123ASSOTIATIONS>",,
   "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123_PLS>;ICONFILE="||dir||"\icons\pls.ico;STARTUPDIR="||dir||";ASSOCFILTER=*.PLS;", "REPLACE" ) = 0 then
   call error

say "Associating PM123 with MPL files..."
if SysCreateObject("WPProgram", "PM123[MPL]", "<PM123ASSOTIATIONS>",,
   "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123_MPL>;ICONFILE="||dir||"\icons\mpl.ico;STARTUPDIR="||dir||";ASSOCFILTER=*.MPL;", "REPLACE" ) = 0 then
   call error

say "Associating PM123 with LST files..."
if SysCreateObject("WPProgram", "PM123[LST]", "<PM123ASSOTIATIONS>",,
   "PROGTYPE=PM;EXENAME="||dir||"\PM123.EXE;OBJECTID=<PM123_LST>;ICONFILE="||dir||"\icons\lst.ico;STARTUPDIR="||dir||";ASSOCFILTER=*.LST;", "REPLACE" ) = 0 then
   call error

call SysDestroyObject "<PM123MANUAL>"
call SysDestroyObject "<PM123SKINSMANUAL>"
call SysDestroyObject "<PM123NETSCAPE>"
call SysDestroyObject "<PM123HISTORY>"

say "Object creation was successful!"
call end

error:
say "Object creation was NOT successful!"
call end

end:
exit 0
