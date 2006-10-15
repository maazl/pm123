/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define INCL_BASE
#include <string.h>
#include <stdio.h>

#include <utilfct.h>
#include <errorstr.h>

#include "pm123.h"
#include "playlist.h"
#include "properties.h"

#include <os2.h>

#include <debuglog.h>


/* Pipe name decided on startup. */
char pipename[_MAX_PATH] = "\\PIPE\\PM123";


/* Writes data to the specified pipe. */
static void
pipe_write( HPIPE pipe, const char *buf )
{
  ULONG action;

  DosWrite( pipe, (char*)buf, strlen( buf ) + 1, &action );
  DosResetBuffer( pipe );
}

/* Dispatches requests received from the pipe. */
static void
pipe_thread( void* params )
{
  char  buffer [2048];
  char  command[2048];
  char* zork;
  char* dork;
  ULONG bytesread;
  HPIPE hpipe = (HPIPE)params;
  APIRET rc;

  for(;;)
  {
    DosDisConnectNPipe( hpipe );
    rc = DosConnectNPipe( hpipe );
    if ( rc != NO_ERROR )
    {
      #ifdef DEBUG
      char buf[1024];
      DEBUGLOG(("pipe_thread: DosConnectNPipe failed. %d - %s\n", rc, os2_strerror( rc, buf, sizeof buf )));
      #endif
      return;
    }

    if( DosRead( hpipe, buffer, sizeof( buffer ), &bytesread ) == NO_ERROR )
    {
      buffer[bytesread] = 0;
      blank_strip( buffer );

      if( *buffer && *buffer != '*' )
      {
        if( is_dir( buffer )) {
          pl_clear( PL_CLR_NEW );
          pl_add_directory( buffer, PL_DIR_RECURSIVE );
        } else {
          amp_load_singlefile( buffer, 0 );
        }
      }
      else if( *buffer == '*' )
      {
        strcpy( command, buffer + 1 ); /* Strip the '*' character */
        blank_strip( command );

        zork = strtok( command, " " );
        dork = strtok( NULL,    ""  );

        if( zork )
        {
          if( stricmp( zork, "status" ) == 0 ) {
            if( dork ) {
              const MSG_PLAY_STRUCT* cur = amp_get_current_file();
              if( cur == NULL ) {
                pipe_write( hpipe, "" );
              } else if( stricmp( dork, "file" ) == 0 ) {
                pipe_write( hpipe, cur->url );
              } else if( stricmp( dork, "tag"  ) == 0 ) {
                char info[512];
                pipe_write( hpipe, amp_construct_tag_string( info, &cur->info.meta ));
              } else if( stricmp( dork, "info" ) == 0 ) {
                pipe_write( hpipe, cur->info.tech.info );
              }
            }
          }
          if( stricmp( zork, "size" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "regular" ) == 0 ||
                  stricmp( dork, "0"       ) == 0 ||
                  stricmp( dork, "normal"  ) == 0  )
              {
                WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_NORMAL ), 0 );
              }
              if( stricmp( dork, "small"   ) == 0 ||
                  stricmp( dork, "1"       ) == 0  )
              {
                WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_SMALL  ), 0 );
              }
              if( stricmp( dork, "tiny"    ) == 0 ||
                  stricmp( dork, "2"       ) == 0  )
              {
                WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_TINY   ), 0 );
              }
            }
          }
          if( stricmp( zork, "rdir" ) == 0 ) {
            if( dork ) {
              pl_add_directory( dork, PL_DIR_RECURSIVE );
            }
          }
          if( stricmp( zork, "dir"  ) == 0 ) {
            if( dork ) {
              pl_add_directory( dork, 0 );
            }
          }
          if( stricmp( zork, "font" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "1" ) == 0 ) {
                WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_FONT1 ), 0 );
              }
              if( stricmp( dork, "2" ) == 0 ) {
                WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_FONT2 ), 0 );
              }
            }
          }
          if( stricmp( zork, "add" ) == 0 )
          {
            char* file;

            if( dork ) {
              while( *dork ) {
                file = dork;
                while( *dork && *dork != ';' ) {
                  ++dork;
                }
                if( *dork == ';' ) {
                  *dork++ = 0;
                }
                pl_add_file( file, NULL, 0 );
              }
            }
          }
          if( stricmp( zork, "load" ) == 0 ) {
            if( dork ) {
              amp_load_singlefile( dork, 0 );
            }
          }
          if( stricmp( zork, "hide"  ) == 0 ) {
            WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_MINIMIZE ), 0 );
          }
          if( stricmp( zork, "float" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                if( cfg.floatontop ) {
                  WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_FLOAT ), 0 );
                }
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                if( !cfg.floatontop ) {
                  WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_FLOAT ), 0 );
                }
              }
            }
          }
          if( stricmp( zork, "use" ) == 0 ) {
            amp_pl_use();
          }
          if( stricmp( zork, "clear" ) == 0 ) {
            pl_clear( PL_CLR_NEW );
          }
          if( stricmp( zork, "next" ) == 0 ) {
            WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_NEXT ), 0 );
          }
          if( stricmp( zork, "previous" ) == 0 ) {
            WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_PREV ), 0 );
          }
          if( stricmp( zork, "remove" ) == 0 ) {
            if( current_record ) {
              PLRECORD* rec = current_record;
              pl_remove_record( &rec, 1 );
            }
          }
          if( stricmp( zork, "forward" ) == 0 ) {
            WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_FWD  ), 0 );
          }
          if( stricmp( zork, "rewind" ) == 0 ) {
            WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_REW  ), 0 );
          }
          if( stricmp( zork, "stop" ) == 0 ) {
            WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_STOP ), 0 );
          }
          if( stricmp( zork, "jump" ) == 0 ) {
            if( dork ) {
              int i = atoi( dork ) * 1000;
              amp_msg( MSG_JUMP, &i, 0 );
            }
          }
          if( stricmp( zork, "play" ) == 0 ) {
            if( dork ) {
              amp_load_singlefile( dork, AMP_LOAD_NOT_PLAY );
              WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
            } else if( !decoder_playing()) {
              WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
            }
          }
          if( stricmp( zork, "pause" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                if( is_paused()) {
                  WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
                }
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                if( !is_paused()) {
                  WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
                }
              }
            } else {
              WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_PAUSE ), 0 );
            }
          }
          if( stricmp( zork, "playonload" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.playonload = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.playonload = TRUE;
              }
            }
          }
          if( stricmp( zork, "autouse" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.autouse = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.autouse = TRUE;
              }
            }
          }
          if( stricmp( zork, "playonuse" ) == 0 ) {
            if( dork ) {
              if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
                cfg.playonuse = FALSE;
              }
              if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
                cfg.playonuse = TRUE;
              }
            }
          }
          if( stricmp( zork, "repeat"  ) == 0 ) {
            WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_REPEAT  ), 0 );
          }
          if( stricmp( zork, "shuffle" ) == 0 ) {
            WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( BMP_SHUFFLE ), 0 );
          }
          if( stricmp( zork, "volume" ) == 0 )
          {
            char buf[64];

            if( dork )
            {
              HPS hps = WinGetPS( amp_player_window() );

              if( *dork == '+' ) {
                cfg.defaultvol += atoi( dork + 1 );
              } else if( *dork == '-' ) {
                cfg.defaultvol -= atoi( dork + 1 );
              } else {
                cfg.defaultvol  = atoi( dork );
              }

              if( cfg.defaultvol > 100 ) {
                  cfg.defaultvol = 100;
              }
              if( cfg.defaultvol < 0   ) {
                  cfg.defaultvol = 0;
              }

              bmp_draw_volume( hps, cfg.defaultvol );
              WinReleasePS( hps );
              amp_volume_adjust();
            }
            pipe_write( hpipe, _itoa( cfg.defaultvol, buf, 10 ));
          }
        }
      }
    }
  }
}

/* Create main pipe with only one instance possible since these pipe
   is almost all the time free, it wouldn't make sense having multiple
   intances. */
BOOL
pipe_create( void )
{
  ULONG rc;
  HPIPE hpipe;
  int   i = 1;

  while(( rc = DosCreateNPipe( pipename, &hpipe,
                               NP_ACCESS_DUPLEX,
                               NP_WAIT | NP_TYPE_BYTE | NP_READMODE_BYTE | 1,
                               2048,
                               2048,
                               500 )) == ERROR_PIPE_BUSY )
  {
    sprintf( pipename,"\\PIPE\\PM123_%d", ++i );
  }

  if( rc != 0 ) {
    amp_player_error( "Could not create pipe %s, rc = %d.", pipename, rc );
    return FALSE;
  } else {
    _beginthread( pipe_thread, NULL, 1024*1024, (void*)hpipe );

    return TRUE;
  }
}



/* Opens specified pipe and writes data to it. */
BOOL
pipe_open_and_write( const char* pipename, const char* data, size_t size )
{
  // Well DosCallNPipe would have done the job too...
  HPIPE  hpipe;
  ULONG  action;
  APIRET rc;

  rc = DosOpen((PSZ)pipename, &hpipe, &action, 0, FILE_NORMAL,
                OPEN_ACTION_FAIL_IF_NEW  | OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE | OPEN_FLAGS_FAIL_ON_ERROR,
                NULL );

  if( rc == NO_ERROR )
  {
    DosWrite( hpipe, (PVOID)data, size, &action );
    DosDisConnectNPipe( hpipe );
    DosClose( hpipe );
    return TRUE;
  } else {
    return FALSE;
  }
}


