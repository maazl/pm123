#define  INCL_DOS
#define  INCL_ERRORS
#define  INCL_WIN
#include <os2.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <utilfct.h>
#include <debuglog.h>

typedef int (DLLENTRYP dll_main)( int argc, char* argv[] );

static void
error_box( const char* format, ... )
{
  HAB  hab = WinInitialize( 0 );
  HMQ  hmq = WinCreateMsgQueue( hab, 0 );

  char padded_title[60];
  char message[4096];

  va_list args;

  va_start( args, format );
  vsprintf( message, format, args );
  sprintf ( padded_title, "%-59s", "PM123 Error" );

  WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, (PSZ)message,
                 padded_title, 0, MB_ERROR | MB_OK | MB_MOVEABLE );

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
}

int main( int argc, char* argv[] )
{
  char     exename[_MAX_PATH];
  char     libpath[_MAX_PATH];
  int      libsize;

  char     error[1024] = "";
  HMODULE  hmodule;
  APIRET   rc;
  dll_main dll_entry;

  getExeName( exename, sizeof( exename ));
  sdrivedir ( libpath, exename, sizeof( libpath ));

  libsize = strlen( libpath );
  if( libsize && libpath[ libsize - 1 ] == '\\' && !is_root( libpath )) {
    libpath[ libsize - 1 ] = 0;
  }

  rc = DosSetExtLIBPATH( libpath, BEGIN_LIBPATH );
  DEBUGLOG(( "pm123: DosSetExtLIBPATH for %s, rc=%d\n", libpath, rc ));

  rc = DosLoadModule( error, sizeof( error ), "pm123.dll", &hmodule );
  if( rc != NO_ERROR ) {
    error_box( "Could not load PM123.DLL\n%s",
               os2_strerror( rc, error, sizeof( error )));
    return 1;
  }

  rc = DosQueryProcAddr( hmodule, 0L, "dll_main", &dll_entry );
  if( rc != NO_ERROR ) {
    error_box( "Could not load \"dll_main\" from PM123.DLL\n%s",
               os2_strerror( rc, error, sizeof( error )));
    return 1;
  }

  return dll_entry( argc, argv );
}
