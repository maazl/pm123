/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_GPI
#define  INCL_ERRORS
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <sys\types.h>
#include <sys\stat.h>

#include <xio.h>
#include "plugman.h"
#include "playlist.i.h"
#include "playlist.h"
#include "pm123.h"
#include "pm123.rc.h"
#include <utilfct.h>
#include "docking.h"
#include "iniman.h"

#include <stddef.h>

#include <debuglog.h>


static BOOL is_playlist( const char *filename );
static void pl_display_status( void );


#define PL_ADD_FILE      0
#define PL_ADD_DIRECTORY 1
#define PL_COMPLETED     2
#define PL_CLEAR         3
#define PL_TERMINATE     4

typedef struct _PLDATA {
  char* filename;
  char* songname;
  int   options;
} PLDATA;

static HWND   menu_playlist = NULLHANDLE;
static HWND   menu_record   = NULLHANDLE;
static HWND   playlist      = NULLHANDLE;
static HWND   container     = NULLHANDLE;
static BOOL   is_busy       = FALSE;
static int    played        = 0;
static TID    broker_tid    = 0;
static PQUEUE broker_queue  = NULL;
static char   current_playlist[_MAX_PATH];
static DECODER_WIZZARD_FUNC assists[16];

/* The pointer to playlist record of the currently loaded file,
   the pointer is NULL if such record is not present. */
PLRECORD* current_record;

/* WARNING!! All functions returning a pointer to the
   playlist record, return a NULL if suitable record is not found. */

#define PL_RC( p ) ((PLRECORD*)p != (PLRECORD*)-1 ? (PLRECORD*)p : NULL )

/* Returns the pointer to the first playlist record. */
static PLRECORD*
pl_first_record( void ) {
  return PL_RC( WinSendMsg( container, CM_QUERYRECORD, NULL,
                            MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER )));
}

/* Returns the pointer to the next playlist record of specified. */
static PLRECORD*
pl_next_record( PLRECORD* rec ) {
  return PL_RC( WinSendMsg( container, CM_QUERYRECORD, MPFROMP(rec),
                            MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER )));
}

/* Returns the pointer to the previous playlist record of specified. */
static PLRECORD*
pl_prev_record( PLRECORD* rec ) {
  return PL_RC( WinSendMsg( container, CM_QUERYRECORD, MPFROMP(rec),
                            MPFROM2SHORT( CMA_PREV, CMA_ITEMORDER )));
}

/* Returns the pointer to the record of the specified file. */
static PLRECORD*
pl_file_record( const char* filename )
{
  PLRECORD* rec;

  for( rec = pl_first_record(); rec; rec = pl_next_record( rec )) {
    if( strcmp( rec->full, filename ) == 0 ) {
      break;
    }
  }

  return rec;
}

/* Returns the pointer to the first selected playlist record. */
static PLRECORD*
pl_first_selected( void ) {
  return PL_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_SELECTED )));
}

/* Returns the pointer to the next selected playlist record of specified. */
static PLRECORD*
pl_next_selected( PLRECORD* rec ) {
  return PL_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( rec ), MPFROMSHORT( CRA_SELECTED )));
}

/* Returns the pointer to the cursored playlist record. */
static PLRECORD*
pl_cursored( void ) {
  return PL_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_CURSORED )));
}

/* Queries a random playlist record for playing. */
/*PLRECORD*
pl_query_random_record( void )
{
  PLRECORD* rec = pl_first_record();
  int pos = 0;

  while( rec ) {
    if( !rec->played && rec->exist ) {
      ++pos;
    }
    rec = pl_next_record( rec );
  }

  if( pos )
  {
    pos = rand() % pos;
    rec = pl_first_record();

    while( rec ) {
      if( !rec->played && rec->exist && !pos-- ) {
        break;
      }
      rec = pl_next_record( rec );
    }
  }

  return rec;
}*/

/* Queries a first playlist record for playing. */
/*PLRECORD*
pl_query_first_record( void )
{
  if( cfg.shf ) {
    return pl_query_random_record();
  } else {
    PLRECORD* rec = pl_first_record();

    while( rec && !rec->exist ) {
      rec = pl_next_record( rec );
    }
    return rec;
  }
}*/

/* Queries a previous playlist record for playing. */
/*PLRECORD*
pl_query_prev_record( void )
{
  PLRECORD* found = NULL;

  if( current_record ) {
    if( cfg.shf ) {
      PLRECORD* rec = pl_first_record();
      while( rec ) {
        // Search for record which it was played before current record.
        if( rec->played && rec->played < current_record->played ) {
          if( !found || rec->played > found->played ) {
            found = rec;
            // Make search little bit faster.
            if( current_record->played - found->played == 1 ) {
              break;
            }
          }
        }
        rec = pl_next_record( rec );
      }
    } else {
      found = pl_prev_record( current_record );
      while( found && !found->exist ) {
        found = pl_prev_record( found );
      }
    }
  }
  return found;
}*/

/* Queries a next playlist record for playing. */
/*PLRECORD*
pl_query_next_record( void )
{
  PLRECORD* found = NULL;

  if( current_record ) {
    if( cfg.shf ) {
      if( current_record->played == played ) {
        found =  pl_query_random_record();
      } else {
        PLRECORD* rec = pl_first_record();

        while( rec ) {
          // Search for record which it was played before current record.
          if( rec->played > current_record->played ) {
            if( !found || rec->played < found->played ) {
              found = rec;
              // Make search little bit faster.
              if( found->played - current_record->played == 1 ) {
                break;
              }
            }
          }
          rec = pl_next_record( rec );
        }
        if( !found ) {
          found = pl_query_random_record();
        }
      }
    } else {
      found = pl_next_record( current_record );
      while( found && !found->exist ) {
        found = pl_next_record( found );
      }
    }
  }
  return found;
}*/

/* Queries a playlist record of the specified file for playing. */
/*PLRECORD*
pl_query_file_record( const char* filename ) {
  return pl_file_record( filename );
}*/

static SHORT EXPENTRY
pl_compare_rand( const PLRECORD* p1, const PLRECORD* p2, PVOID pStorage ) {
  return ( rand() % 2 ) - 1;
}

static SHORT EXPENTRY
pl_compare_size( const PLRECORD* p1, const PLRECORD* p2, PVOID pStorage )
{
  if( p1->info.tech->filesize < p2->info.tech->filesize ) return -1;
  if( p1->info.tech->filesize > p2->info.tech->filesize ) return  1;
  return 0;
}

static SHORT EXPENTRY
pl_compare_time( const PLRECORD* p1, const PLRECORD* p2, PVOID pStorage )
{
  if( p1->info.tech->songlength < p2->info.tech->songlength ) return -1;
  if( p1->info.tech->songlength > p2->info.tech->songlength ) return  1;
  return 0;
}

static SHORT EXPENTRY
pl_compare_name( const PLRECORD* p1, const PLRECORD* p2, PVOID pStorage )
{
  char s1[_MAX_PATH], s2[_MAX_PATH];

  sfname( s1, p1->full, sizeof( s1 ));
  sfname( s2, p2->full, sizeof( s2 ));

  return stricmp( s1, s2 );
}

static SHORT EXPENTRY
pl_compare_song( const PLRECORD* p1, const PLRECORD* p2, PVOID pStorage ) {
  return stricmp( p1->songname, p2->songname );
}

/* Sorts the playlist records. */
static void
pl_sort( int control )
{
  switch( control ) {
    case PL_SORT_RAND:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_compare_rand ), 0 );
      break;
    case PL_SORT_SIZE:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_compare_size ), 0 );
      break;
    case PL_SORT_TIME:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_compare_time ), 0 );
      break;
    case PL_SORT_NAME:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_compare_name ), 0 );
      break;
    case PL_SORT_SONG:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_compare_song ), 0 );
      break;
  }
  amp_invalidate( UPD_ALL );
}

/* Refreshes the specified playlist record. */
static void
pl_refresh_record( PLRECORD* rec, USHORT flags )
{
  WinSendMsg( container, CM_INVALIDATERECORD,
              MPFROMP( &rec ), MPFROM2SHORT( 1, flags ));
}

/* Fills record by the information provided by the decoder. */
static void
pl_fill_record( PLRECORD* rec, const DECODER_INFO2* info ) ////
{
  char buffer[64];
  DEBUGLOG(("pl_fill_record(%p, %p)\n", rec, info));

  free( rec->songname );
  free( rec->moreinfo );
  free( rec->size );
  free( rec->time );
  
  rec->info = *info;

  sprintf( buffer, "%u kB", (unsigned int)info->tech->filesize / 1024 );
  rec->size = strdup( buffer );
  sprintf( buffer, "%02u:%02u", info->tech->songlength / 60000, (int)(info->tech->songlength / 1000) % 60 );
  rec->time = strdup( buffer );

  // Songname
  rec->songname = (char*)malloc( strlen( info->meta->artist ) + 2 +
                          strlen( info->meta->title  ) + 1 );
  if( rec->songname ) {
    strcpy( rec->songname, info->meta->artist );
    if( *info->meta->title && *info->meta->artist ) {
      strcat( rec->songname, "- " );
    }
    strcat( rec->songname, info->meta->title );
  }

  // Information
  rec->moreinfo = (char*)malloc( strlen( info->meta->album   ) + 1 +
                          strlen( info->meta->year    ) + 2 +
                          strlen( info->meta->genre   ) + 2 +
//                        strlen( info->meta->track   ) + 3 +
                          3                            + 3 +
                          strlen( info->tech->info    ) +
                          strlen( info->meta->comment ) + 12 );
  if( rec->moreinfo ) {
    strcpy( rec->moreinfo, info->meta->album );

    if( *info->meta->album ) {
      strcat( rec->moreinfo, " "  );
    }
    if( *info->meta->year  ) {
      strcat( rec->moreinfo, info->meta->year );
      strcat( rec->moreinfo, ", " );
    }
    if( *info->meta->genre ) {
      strcat( rec->moreinfo, info->meta->genre );
      strcat( rec->moreinfo, ", " );
    }
/*  TODO: make track a string 
    if( *info->meta.track ) {
      strcat( rec->moreinfo, "#"  );
      strcat( rec->moreinfo, info->meta.track );
      strcat( rec->moreinfo, ", " );
    }
*/
    if( info->meta->track > 0 ) {
      sprintf( rec->moreinfo + strlen( rec->moreinfo ), "#%i, ", info->meta->track );
    }
    strcat( rec->moreinfo, info->tech->info );

    if( *info->meta->comment ) {
      strcat( rec->moreinfo, ", comment: " );
      strcat( rec->moreinfo, info->meta->comment );
    }
  }
}

/* Creates the playlist record for specified file. */
static PLRECORD*
pl_create_record( const char* filename, PLRECORD* pos,
                  const char* prep_title, const DECODER_INFO2* prep_info, const char* prep_decoder )
{
  DECODER_INFO2 info;
  int          rc = 0;
  PLRECORD*    rec;
  RECORDINSERT insert;
  char         decoder[_MAX_FNAME] = "";
  char         buffer [_MAX_PATH ];

  if( !filename ) {
    return NULL;
  }

  // We can have the already prepared information about file and its decoder.
  if( !prep_info || !prep_decoder ) {
    rc = dec_fileinfo((char*)filename, &info, decoder );
  } else {
    info = *prep_info;
    strlcpy( decoder, prep_decoder, sizeof( decoder ));
  }

  if( !*info.meta->title && prep_title ) {
    strlcpy( info.meta->title, prep_title, sizeof( info.meta->title ));
  }

  // Allocate a new record.
  rec = (PLRECORD*)WinSendMsg( container, CM_ALLOCRECORD,
                               MPFROMLONG( sizeof( PLRECORD ) - sizeof( RECORDCORE )),
                               MPFROMLONG( 1 ));

  rec->rc.cb           = sizeof( RECORDCORE );
  rec->rc.flRecordAttr = CRA_DROPONABLE;
  //rec->rc.hptrIcon     = ( rc == 0 ) ? mp3 : mp3gray;
  rec->full            = strdup( filename );
  rec->size            = NULL;
  rec->time            = NULL;
  rec->songname        = NULL;
  rec->moreinfo        = NULL;
  rec->played          = 0;
  rec->exist           = ( rc == 0 );

  strlcpy( rec->decoder, decoder, sizeof( rec->decoder ));
  sfnameext( buffer, filename, sizeof( buffer ));

  if( is_url( filename )) {
    sdecode( buffer, buffer, sizeof( buffer ));
  }

  rec->rc.pszIcon = strdup( buffer );
  pl_fill_record( rec, &info );

  insert.cb                = sizeof(RECORDINSERT);
  insert.pRecordOrder      = (PRECORDCORE) pos;
  insert.pRecordParent     = NULL;
  insert.fInvalidateRecord = FALSE;
  insert.zOrder            = CMA_TOP;
  insert.cRecordsInsert    = 1;

  WinSendMsg( container, CM_INSERTRECORD,
              MPFROMP( rec ), MPFROMP( &insert ));

  pl_refresh_record( rec, CMA_TEXTCHANGED );
  amp_invalidate( UPD_FILEINFO | UPD_DELAYED );
  return rec;
}

/* Copies the playlist record to specified position. */
static PLRECORD*
pl_copy_record( PLRECORD* rec, PLRECORD* pos )
{
  RECORDINSERT insert;
  PLRECORD*    copy;

  copy = (PLRECORD*)WinSendMsg( container, CM_ALLOCRECORD,
                                MPFROMLONG( sizeof( PLRECORD ) - sizeof( RECORDCORE )),
                                MPFROMLONG( 1 ));
  if( !copy ) {
    return NULL;
  }

  copy->rc.cb           = sizeof(RECORDCORE);
  copy->rc.flRecordAttr = CRA_DROPONABLE;
  //copy->rc.hptrIcon     = rec->exist ? mp3 : mp3gray;
  copy->full            = strdup( rec->full );
  copy->size            = strdup( rec->size );
  copy->songname        = strdup( rec->songname );
  copy->moreinfo        = strdup( rec->moreinfo );
  copy->time            = strdup( rec->time );
  copy->rc.pszIcon      = strdup( rec->rc.pszIcon );
  copy->info            = rec->info;
  copy->played          = 0;
  copy->exist           = rec->exist;
  strlcpy( copy->decoder, rec->decoder, sizeof copy->decoder );

  insert.cb                = sizeof(RECORDINSERT);
  insert.pRecordOrder      = (PRECORDCORE)pos;
  insert.pRecordParent     = NULL;
  insert.fInvalidateRecord = TRUE;
  insert.zOrder            = CMA_TOP;
  insert.cRecordsInsert    = 1;

  if( WinSendMsg( container, CM_INSERTRECORD,
                  MPFROMP( copy ), MPFROMP( &insert )) == 0 )
  {
    return NULL;
  } else {
    return copy;
  }
}

/* Moves the playlist record to specified position. */
static PLRECORD*
pl_move_record( PLRECORD* rec, PLRECORD* pos )
{
  RECORDINSERT insert;

  if( WinSendMsg( container, CM_REMOVERECORD, MPFROMP( &rec ),
                  MPFROM2SHORT( 1, CMA_INVALIDATE )) != MRFROMLONG( -1 ))
  {
    insert.cb                = sizeof(RECORDINSERT);
    insert.pRecordOrder      = (PRECORDCORE)pos;
    insert.pRecordParent     = NULL;
    insert.fInvalidateRecord = TRUE;
    insert.zOrder            = CMA_TOP;
    insert.cRecordsInsert    = 1;

    if( WinSendMsg( container, CM_INSERTRECORD,
                    MPFROMP( rec ), MPFROMP( &insert )) == 0 )
    {
      return NULL;
    }
  }

  return rec;
}

/* Frees the data contained in playlist record. */
static void
pl_free_record( PLRECORD* rec )
{
  free( rec->rc.pszIcon );
  free( rec->songname );
  free( rec->size );
  free( rec->time );
  free( rec->moreinfo );
  free( rec->full );
}

/* Removes the specified playlist record. */
static void
pl_remove_record( PLRECORD** array, USHORT count )
{
  PLRECORD* load_after = (PLRECORD*)-1;
  int i;

  if( WinSendMsg( container, CM_REMOVERECORD, MPFROMP( array ),
                  MPFROM2SHORT( count, CMA_INVALIDATE )) != MRFROMLONG( -1 ))
  {
    for( i = 0; i < count; i++ ) {
      pl_free_record( array[i] );
    }

    WinSendMsg( container, CM_FREERECORD,
                MPFROMP( array ), MPFROMSHORT( count ));
  }

  amp_invalidate( UPD_FILEINFO | UPD_DELAYED );
}

/* Removes all playlist records. */
static void
pl_remove_all( void )
{
   PLRECORD* rec;

   for( rec = pl_first_record(); rec; rec = pl_next_record( rec )) {
     pl_free_record( rec );
   }

   WinSendMsg( container, CM_REMOVERECORD,
               MPFROMP(NULL), MPFROM2SHORT( 0, CMA_FREE | CMA_INVALIDATE ));
}

/* Refreshes the playlist record of the specified file. */
/*void
pl_refresh_file( const char* filename )
{
  PLRECORD* rec = pl_first_record();
  DECODER_INFO2 info;

  while( rec ) {
    if( stricmp( rec->full, filename ) == 0 ) {
      if( dec_fileinfo((char*)filename, &info, rec->decoder ) == 0 ) {
        pl_fill_record( rec, &info );
        pl_refresh_record( rec, CMA_NOREPOSITION );
      }
    }
    rec = pl_next_record( rec );
  }
}*/

/* Scrolls the playlist so that the specified record became visible. */
static BOOL
pl_scroll_to_record( PLRECORD* rec )
{
  QUERYRECORDRECT prcItem;

  RECTL rclRecord, rclContainer;

  prcItem.cb       = sizeof(prcItem);
  prcItem.pRecord  = (PRECORDCORE)rec;
  prcItem.fsExtent = CMA_ICON | CMA_TEXT;

  if( !WinSendMsg( container, CM_QUERYRECORDRECT, &rclRecord, &prcItem )) {
    return FALSE;
  }

  if( !WinSendMsg( container, CM_QUERYVIEWPORTRECT, &rclContainer,
                              MPFROM2SHORT( CMA_WINDOW, FALSE )))
  {
    return FALSE;
  }

  if( rclRecord.yBottom < rclContainer.yBottom )  {
    WinPostMsg( container, CM_SCROLLWINDOW, (MPARAM)CMA_VERTICAL,
                (MPARAM)(rclContainer.yBottom - rclRecord.yBottom ));
  } else if( rclRecord.yTop > rclContainer.yTop ) {
    WinPostMsg( container, CM_SCROLLWINDOW, (MPARAM)CMA_VERTICAL,
                (MPARAM)(rclContainer.yTop - rclRecord.yTop ));
  }
  return TRUE;
}

/* Selects the specified record and deselects all others. */
static void
pl_select( PLRECORD* rec )
{
  PLRECORD* uns;

  for( uns = pl_first_selected(); uns; uns = pl_next_selected( uns )) {
    WinSendMsg( container, CM_SETRECORDEMPHASIS,
                MPFROMP( uns ), MPFROM2SHORT( FALSE, CRA_SELECTED ));
  }

  WinSendMsg( container, CM_SETRECORDEMPHASIS,
              MPFROMP( rec ), MPFROM2SHORT( TRUE , CRA_SELECTED | CRA_CURSORED ));
}

static PLDATA*
pl_create_request_data( const char* filename,
                        const char* songname, int options )
{
  PLDATA* data = (PLDATA*)malloc( sizeof( PLDATA ));

  if( data ) {
    data->filename = filename ? strdup( filename ) : NULL;
    data->songname = songname ? strdup( songname ) : NULL;
    data->options  = options;
  }

  return data;
}

static void
pl_delete_request_data( PLDATA* data )
{
  if( data ) {
    free( data->filename );
    free( data->songname );
    free( data );
  }
}

/* Purges a specified queue of all its requests. */
static void
pl_purge_queue( PQUEUE queue )
{
  ULONG  request;
  PVOID  data;

  while( !qu_empty( queue )) {
    qu_read( queue, &request, &data );
    pl_delete_request_data( (PLDATA*)data );
  }
}

/* Examines a queue request without removing it from the queue. */
static BOOL
pl_peek_queue( PQUEUE queue, ULONG first, ULONG last )
{
  ULONG request;
  PVOID data;

  if( qu_peek( queue, &request, &data )) {
    return request >= first && request <= last;
  } else {
    return FALSE;
  }
}

/* The playlist broker helper function. */
static void
pl_broker_add_file( const char* filename, const char* title, int options )
{
  struct    stat fi;
  PLRECORD* rec;

  if( options & PL_ADD_IF_EXIST && stat( filename, &fi ) != 0 ) {
    return;
  }

  if( !is_busy ) {
    is_busy = !is_busy;
    pl_display_status();
  }

  rec = pl_create_record( filename, (PLRECORD*)CMA_END, title, NULL, NULL );

  if( rec ) {
    if( options & PL_ADD_SELECT ) {
      pl_select( rec );
      pl_scroll_to_record( rec );
    }

    /* TODO: make no sense without modification
    if( options & PL_ADD_LOAD   ) {
      amp_playmode = AMP_PLAYLIST;
      amp_pl_load_record( rec );
    }*/
  }
}

/* The playlist broker helper function. */
static void
pl_broker_add_directory( const char* path, int options )
{
  char  findpath[_MAX_PATH];
  char  findspec[_MAX_PATH];
  char  fullname[_MAX_PATH];
  ULONG findrc;
  HDIR  hdir;

  FILEFINDBUF3 findbuff;

  if( !is_busy ) {
    is_busy = !is_busy;
    pl_display_status();
  }

  strcpy( findpath, path );
  if( *findpath && findpath[strlen(findpath)-1] != '/' ) {
    strcat( findpath, "/" );
  }

  strcpy( findspec, findpath );
  strcat( findspec, "*" );

  findrc = findfirst( &hdir, findspec, FIND_ALL, &findbuff );

  while( findrc == 0 && !pl_peek_queue( broker_queue, PL_CLEAR, PL_TERMINATE ))
  {
    strcpy( fullname, findpath );
    strcat( fullname, findbuff.achName );

    if( findbuff.attrFile & FILE_DIRECTORY ) {
      if( options & PL_DIR_RECURSIVE
          && strcmp( findbuff.achName, "."  ) != 0
          && strcmp( findbuff.achName, ".." ) != 0 )
      {
        pl_broker_add_directory( fullname, options );
      }
    } else {
      char module_name[_MAX_FNAME] = "";
      DECODER_INFO2 info;

      if( dec_fileinfo( fullname, &info, module_name ) == 0 ) {
        pl_create_record( fullname, (PLRECORD*)CMA_END, NULL, &info, module_name );
      }
    }

    findrc = findnext( hdir, &findbuff );
  }

  if( findrc != ERROR_NO_MORE_FILES && findrc != NO_ERROR ) {
    char msg[1024];
    amp_error( container, "Unable scan the directory:\n%s\n%s", path,
               os2_strerror( findrc, msg, sizeof( msg )));
  }

  findclose( hdir );
}

/* Dispatches the playlist management requests. */
static void TFNENTRY
pl_broker( void* dummy )
{
  ULONG   request;
  PVOID   reqdata;
  HAB     hab = WinInitialize( 0 );
  HMQ     hmq = WinCreateMsgQueue( hab, 0 );

  while( qu_read( broker_queue, &request, &reqdata ))
  {
    PLDATA* data = (PLDATA*)reqdata;

    switch( request ) {
      case PL_CLEAR:
        pl_remove_all();
        if( data->options & PL_CLR_NEW ) {
          *current_playlist = 0;
          pl_display_status();
        }
        break;

      case PL_ADD_FILE:
        pl_broker_add_file( data->filename, data->songname, data->options );
        break;

      case PL_TERMINATE:
        break;

      case PL_ADD_DIRECTORY:
        /* TODO: will be removed probably
        pl_broker_add_directory( data->filename, data->options );
        pl_completed();*/
        break;

      case PL_COMPLETED:
        /* TODO
        if( cfg.autouse || amp_playmode == AMP_PLAYLIST ) {
          amp_pl_use();
        }*/
        break;
    }

    pl_delete_request_data( data );

    if( qu_empty( broker_queue ) && is_busy ) {
      is_busy = !is_busy;
      pl_display_status();
    }

    if( request == PL_TERMINATE ) {
      break;
    }
  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
  _endthread();
}

/* Returns a ordinal number of the currently loaded file. */
/*ULONG pl_current_index( void )
{
  PLRECORD* rec;
  ULONG     pos = 1;

  for( rec = pl_first_record(); rec; rec = pl_next_record( rec ), ++pos ) {
    if( rec == current_record ) {
      return pos;
    }
  }
  return 0;
}*/

/* Returns the number of records in the playlist. */
/*ULONG pl_size( void )
{
  CNRINFO info;

  if( WinSendMsg( container, CM_QUERYCNRINFO,
                  MPFROMP(&info), MPFROMLONG(sizeof(info))) != 0 )
  {
    return info.cRecords;
  } else {
    return 0;
  }
}*/

/* Returns a summary play time of the remained part of the playlist. */
/*ULONG pl_playleft( void )
{
  ULONG     time = 0;
  PLRECORD* rec  = current_record;

  if( cfg.shf || !rec ) {
    rec = pl_first_record();
  }

  // TODO: wrap around if more than 49 days playlist length ?!? 
  while( rec ) {
    if( !rec->played || rec == current_record || !cfg.shf ) {
      time += rec->info.tech->songlength / 1000;
    }
    rec = pl_next_record( rec );
  }

  return time;
}*/

/* Marks the currently loaded playlist record as currently played. */
/*void
pl_mark_as_play()
{
  if( current_record ) {
    //current_record->rc.hptrIcon = mp3play;

    if( cfg.selectplayed ) {
      pl_select( current_record );
    }
    if( cfg.shf && !current_record->played ) {
      current_record->played = ++played;
    }

    pl_scroll_to_record( current_record );
    pl_refresh_record  ( current_record, CMA_NOREPOSITION );
  }
}*/

/* Marks the currently loaded playlist record as currently stopped. */
/*void
pl_mark_as_stop()
{
  if( current_record ) {
    //current_record->rc.hptrIcon = current_record->exist ? mp3 : mp3gray;
    pl_refresh_record( current_record, CMA_NOREPOSITION );
  }
}*/

/* Removes "already played" marks from all playlist records. */
/*void
pl_clean_shuffle( void )
{
  PLRECORD* rec;
  for( rec = pl_first_record(); rec; rec = pl_next_record( rec )) {
    rec->played = 0;
  }
  played = 0;
}*/

/* Sets the title of the playlist window according to current
   playlist state. */
void
pl_display_status( void )
{
  char title[ _MAX_FNAME + 128 ];
  char file [ _MAX_FNAME ];

  strcpy( title, "PM123 Playlist"  );

  if( *current_playlist ) {
    sfnameext( file, current_playlist, sizeof( file ));
    strlcat( title, " - ", sizeof( title ));
    strlcat( title, file, sizeof( title ));
  }

  /* TODO: makes no more sense
  if( amp_playmode == AMP_PLAYLIST ) {
    strlcat( title, " - [USED]", sizeof( title ));
  }*/

  if( is_busy ) {
    strlcat( title, " - loading...", sizeof( title ));
  }

  WinSetWindowText( playlist, (PSZ)title );
}

/* Removes all selected playlist records. */
static void
pl_remove_selected( void )
{
  PLRECORD** array = NULL;
  PLRECORD*  rec;
  USHORT     count = 0;
  USHORT     size  = 0;

  for( rec = pl_first_selected(); rec; rec = pl_next_selected( rec )) {
    if( count == size ) {
      size  = size + 20;
      array = (PLRECORD**)realloc( array, size * sizeof( PLRECORD* ));
    }
    if( !array ) {
      return;
    }
    array[count++] = rec;
  }

  if( count ) {
    pl_remove_record( array, count );
    pl_select( pl_cursored());
  }
  free( array );
}

/* Deletes all selected files. */
static void
pl_delete_selected( void )
{
  PLRECORD* rec;

  if( amp_query( playlist, "Do you want remove all selected files from the playlist "
                           "and delete this files from your disk?" ))
  {
    /* TODO: we can't delete records currently in use
    for( rec = pl_first_selected(); rec; rec = pl_next_selected( rec )) {
      if( amp_playmode == AMP_PLAYLIST && rec == current_record && decoder_playing()) {
          amp_stop();
      }
      if( is_file( rec->full ) && unlink( rec->full ) != 0 ) {
        amp_error( playlist, "Unable delete file:\n%s\n%s",
                              rec->full, strerror(errno));
      }
    }*/

    pl_remove_selected();
  }
}

/* Shows the context menu of the playlist. */
static void
pl_show_context_menu( HWND parent, const PLRECORD* rec )
{
  POINTL   pos;
  SWP      swp;
  char     file[_MAX_PATH];
  HWND     mh;
  MENUITEM mi;
  int      i;
  int      count;
  short    id;

  DEBUGLOG(("pl_show_context_menu(%p, %p)\n", parent, rec));

  WinQueryPointerPos( HWND_DESKTOP, &pos );
  WinMapWindowPoints( HWND_DESKTOP, parent, &pos, 1 );

  if( WinWindowFromPoint( parent, &pos, TRUE ) == NULLHANDLE )
  {
    // The context menu is probably activated from the keyboard.
    WinQueryWindowPos( parent, &swp );
    pos.x = swp.cx/2;
    pos.y = swp.cy/2;
  }

  if( rec ) {
    // If have record, show the context menu for this record.
    mn_enable_item( menu_record, IDM_PL_EDIT, rec->info.meta_write );

    WinPopupMenu( parent, parent, menu_record, pos.x, pos.y, IDM_PL_USE,
                  PU_POSITIONONITEM | PU_HCONSTRAIN   | PU_VCONSTRAIN |
                  PU_MOUSEBUTTON1   | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
    return;
  }

  // Add Menu
  WinSendMsg( menu_playlist, MM_QUERYITEM,
              MPFROM2SHORT( IDM_PL_APPEND, TRUE ), MPFROMP( &mi ));
  
  mh    = mi.hwndSubMenu;
  count = LONGFROMMR( WinSendMsg( mh, MM_QUERYITEMCOUNT, 0, 0 ));
  // Remove anything from IDM_PL_ADDOTHER
  id    = IDM_PL_APPOTHER; 
  while ( --count == SHORT1FROMMR( WinSendMsg( mh, MM_DELETEITEM, MPFROM2SHORT( id++, FALSE ), 0 )) );

  append_load_menu( mh, IDM_PL_APPOTHER, assists, sizeof assists / sizeof *assists );

  // Open Menu
  WinSendMsg( menu_playlist, MM_QUERYITEM,
              MPFROM2SHORT( IDM_PL_OPEN, TRUE ), MPFROMP( &mi ));

  mh    = mi.hwndSubMenu;
  count = LONGFROMMR( WinSendMsg( mh, MM_QUERYITEMCOUNT, 0, 0 ));

  // Remove all items from the open menu except of first
  // intended for a choice of object of loading.
  for( i = 1; i < count; i++ )
  {
    id = LONGFROMMR( WinSendMsg( mh, MM_ITEMIDFROMPOSITION, MPFROMSHORT(1), 0 ));
    WinSendMsg( mh, MM_DELETEITEM, MPFROM2SHORT( id, FALSE ), 0 );
  }


  if( *cfg.list[0] )
  {
    // Add separator.
    mi.iPosition = MIT_END;
    mi.afStyle = MIS_SEPARATOR;
    mi.afAttribute = 0;
    mi.id = 0;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem = 0;

    WinSendMsg( mh, MM_INSERTITEM, MPFROMP( &mi ), NULL );

    // Fill the recall list.
    for( i = 0; i < MAX_RECALL; i++ ) {
      if( *cfg.list[i] )
      {
        sprintf( file, "~%u ", i + 1 );

        if( is_url( cfg.list[i] ))
        {
          char buff[_MAX_PATH];

          scheme( buff, cfg.list[i], sizeof( buff ));

          if( strchr( buff, ':' ) != 0 ) {
             *strchr( buff, ':' )  = 0;
          }

          strlcat( file, "[" , sizeof( file ));
          strlcat( file, buff, sizeof( file ));
          strlcat( file, "] ", sizeof( file ));
          sfnameext( buff, cfg.list[i], sizeof( buff ));
          sdecode( buff, buff, sizeof( buff ));
          strlcat( file, buff, sizeof( buff ));
        } else {
          sfnameext( file + strlen( file ), cfg.list[i], sizeof( file ) - strlen( file ) );
        }

        mi.iPosition = MIT_END;
        mi.afStyle = MIS_TEXT;
        mi.afAttribute = 0;
        mi.id = (IDM_PL_OPENLAST + 1) + i;
        mi.hwndSubMenu = (HWND)NULLHANDLE;
        mi.hItem = 0;

        WinSendMsg( mh, MM_INSERTITEM, MPFROMP( &mi ), MPFROMP( file ));
      }
    }
    WinSendMsg( menu_playlist, MM_SETITEMATTR,
                MPFROM2SHORT( IDM_PL_OPENLAST, TRUE ), MPFROM2SHORT( MIA_DISABLED, 0 ));
  } else {
    WinSendMsg( menu_playlist, MM_SETITEMATTR,
                MPFROM2SHORT( IDM_PL_OPENLAST, TRUE ), MPFROM2SHORT( MIA_DISABLED, MIA_DISABLED ));
  }

  /* TODO: makes no more sense
  if( amp_playmode == AMP_PLAYLIST ) {
    WinSendMsg( menu_playlist, MM_SETITEMTEXT,
                MPFROMSHORT( IDM_PL_USE ), MPFROMP( "Don't ~use this Playlist\tCtrl+U" ));
  } else {
    WinSendMsg( menu_playlist, MM_SETITEMTEXT,
                MPFROMSHORT( IDM_PL_USE ), MPFROMP( "~Use this Playlist\tCtrl+U" ));
  }*/

  WinPopupMenu( parent, parent, menu_playlist, pos.x, pos.y, IDM_PL_USE,
                PU_POSITIONONITEM | PU_HCONSTRAIN   | PU_VCONSTRAIN |
                PU_MOUSEBUTTON1   | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
}

/* Prepares the playlist container item to the drag operation. */
static void
pl_drag_init_item( HWND hwnd, PLRECORD* rec,
                   PDRAGINFO drag_infos, PDRAGIMAGE drag_image, int i )
{
  DRAGITEM ditem;
  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];

  memset( &ditem, 0, sizeof( ditem ));

  sdrivedir( pathname, rec->full, sizeof( pathname ));
  sfnameext( filename, rec->full, sizeof( filename ));

  ditem.hwndItem          = hwnd;
  ditem.ulItemID          = (ULONG)rec;
  ditem.hstrType          = DrgAddStrHandle( DRT_BINDATA );
  ditem.hstrRMF           = DrgAddStrHandle( "(DRM_123FILE,DRM_DISCARD)x(DRF_UNKNOWN)" );
  ditem.hstrContainerName = DrgAddStrHandle( pathname );
  ditem.hstrSourceName    = DrgAddStrHandle( filename );
  ditem.hstrTargetName    = DrgAddStrHandle( filename );
  ditem.fsSupportedOps    = DO_MOVEABLE | DO_COPYABLE;

  DrgSetDragitem( drag_infos, &ditem, sizeof( DRAGITEM ), i );

  drag_image->cb       = sizeof( DRAGIMAGE );
  drag_image->hImage   = rec->rc.hptrIcon;
  drag_image->fl       = DRG_ICON | DRG_MINIBITMAP;
  drag_image->cxOffset = i < 5 ? 5 * i : 25;
  drag_image->cyOffset = i < 5 ? 5 * i : 25;

  WinSendDlgItemMsg( hwnd, CNR_PLAYLIST, CM_SETRECORDEMPHASIS,
                     MPFROMP( rec ), MPFROM2SHORT( TRUE, CRA_SOURCE ));
}

/* Prepares the playlist container to the drag operation. */
static MRESULT
pl_drag_init( HWND hwnd, PCNRDRAGINIT pcdi )
{
  PLRECORD*  rec;
  BOOL       drag_selected = FALSE;
  int        drag_count    = 0;
  PDRAGIMAGE drag_images   = NULL;
  PDRAGINFO  drag_infos    = NULL;

  // If the record under the mouse is NULL, we must be over whitespace,
  // in which case we don't want to drag any records.

  if( !(PLRECORD*)pcdi->pRecord ) {
    return 0;
  }

  // Count the selected records. Also determine whether or not we should
  // process the selected records. If the container record under the
  // mouse does not have this emphasis, we shouldn't.

  for( rec = pl_first_selected(); rec; rec = pl_next_selected( rec )) {
    if( rec == (PLRECORD*)pcdi->pRecord ) {
      drag_selected = TRUE;
    }
    ++drag_count;
  }

  if( !drag_selected ) {
    drag_count = 1;
  }

  // Allocate an array of DRAGIMAGE structures. Each structure contains
  // info about an image that will be under the mouse pointer during the
  // drag. This image will represent a container record being dragged.

  drag_images = (PDRAGIMAGE)malloc( sizeof(DRAGIMAGE)*drag_count );

  if( !drag_images ) {
    return 0;
  }

  // Let PM allocate enough memory for a DRAGINFO structure as well as
  // a DRAGITEM structure for each record being dragged. It will allocate
  // shared memory so other processes can participate in the drag/drop.

  drag_infos = DrgAllocDraginfo( drag_count );

  if( !drag_infos ) {
    return 0;
  }

  if( drag_selected ) {
    int i = 0;
    for( rec = pl_first_selected(); rec; rec = pl_next_selected( rec ), i++ ) {
      pl_drag_init_item( hwnd, rec, drag_infos, drag_images+i, i );
    }
  } else {
    pl_drag_init_item( hwnd, (PLRECORD*)pcdi->pRecord,
                             drag_infos, drag_images, 0 );
  }

  // If DrgDrag returns NULLHANDLE, that means the user hit Esc or F1
  // while the drag was going on so the target didn't have a chance to
  // delete the string handles. So it is up to the source window to do
  // it. Unfortunately there doesn't seem to be a way to determine
  // whether the NULLHANDLE means Esc was pressed as opposed to there
  // being an error in the drag operation. So we don't attempt to figure
  // that out. To us, a NULLHANDLE means Esc was pressed...

  if( !DrgDrag( hwnd, drag_infos, drag_images, drag_count, VK_ENDDRAG, NULL )) {
    DrgDeleteDraginfoStrHandles( drag_infos );
  }

  rec = (PLRECORD*)CMA_FIRST;
  while( rec ) {
    rec = (PLRECORD*)WinSendDlgItemMsg( hwnd, CNR_PLAYLIST, CM_QUERYRECORDEMPHASIS,
                                        MPFROMP( rec ), MPFROMSHORT( CRA_SOURCE ));

    if( rec == (PLRECORD*)(-1)) {
      break;
    } else if( rec ) {
      WinSendDlgItemMsg( hwnd, CNR_PLAYLIST, CM_SETRECORDEMPHASIS,
                         MPFROMP( rec ), MPFROM2SHORT( FALSE, CRA_SOURCE ));
      WinSendDlgItemMsg( hwnd, CNR_PLAYLIST, CM_INVALIDATERECORD,
                         MPFROMP( &rec ), MPFROM2SHORT( 1, 0 ));
    }
  }

  free( drag_images );
  DrgFreeDraginfo( drag_infos );
  return 0;
}

/* Prepares the playlist container to the drop operation. */
static MRESULT
pl_drag_over( HWND hwnd, PCNRDRAGINFO pcdi )
{
  PDRAGINFO pdinfo = pcdi->pDragInfo;
  PDRAGITEM pditem;
  int       i;
  USHORT    drag_op = 0;
  USHORT    drag    = DOR_NEVERDROP;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROM2SHORT( DOR_NEVERDROP, 0 );
  }

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ) &&
      ( pdinfo->usOperation == DO_DEFAULT || pdinfo->usOperation == DO_COPY ) &&
      ( pditem->fsSupportedOps & DO_COPYABLE )) {

      drag    = DOR_DROP;
      drag_op = DO_COPY;
      break;

    } else if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ) &&
             ( pdinfo->usOperation == DO_DEFAULT )) {

      drag    = DOR_DROP;
      drag_op = pdinfo->hwndSource == hwnd ? DO_MOVE : DO_COPY;

    } else if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ) &&
             ( pdinfo->usOperation == DO_COPY || pdinfo->usOperation == DO_MOVE )) {

      drag    = DOR_DROP;
      drag_op = pdinfo->usOperation;

    } else {

      drag    = DOR_NEVERDROP;
      drag_op = DO_UNKNOWN;
      break;
    }
  }

  DrgFreeDraginfo( pdinfo );
  return MPFROM2SHORT( drag, drag_op );
}

/* Discards playlist records dropped into shredder. */
static MRESULT
pl_drag_discard( HWND hwnd, PDRAGINFO pdinfo )
{
  PDRAGITEM  pditem;
  PLRECORD** array = NULL;
  int        i;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROMLONG( DRR_ABORT );
  }

  // We get as many DM_DISCARDOBJECT messages as there are
  // records dragged but the first one has enough info to
  // process all of them.

  array = (PLRECORD**)malloc( pdinfo->cditem * sizeof( PLRECORD* ));

  if( array ) {
    for( i = 0; i < pdinfo->cditem; i++ ) {
      pditem = DrgQueryDragitemPtr( pdinfo, i );
      array[i] = (PLRECORD*)pditem->ulItemID;
    }

    pl_remove_record( array, pdinfo->cditem );
    free( array );
  }

  DrgFreeDraginfo( pdinfo );
  return MRFROMLONG( DRR_SOURCE );
}

/* Receives the dropped playlist records. */
static MRESULT
pl_drag_drop( HWND hwnd, PCNRDRAGINFO pcdi )
{
  PDRAGINFO pdinfo = pcdi->pDragInfo;
  PDRAGITEM pditem;

  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];
  char fullname[_MAX_PATH];
  int  i;

  PLRECORD* pos = pcdi->pRecord ? (PLRECORD*)pcdi->pRecord : (PLRECORD*)CMA_END;
  PLRECORD* ins;

  if( !DrgAccessDraginfo( pdinfo )) {
    return 0;
  }

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    DrgQueryStrName( pditem->hstrSourceName,    sizeof( filename ), filename );
    DrgQueryStrName( pditem->hstrContainerName, sizeof( pathname ), pathname );
    strcpy( fullname, pathname );
    strcat( fullname, filename );

    if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ))
    {
      if( pditem->hstrContainerName && pditem->hstrSourceName ) {
        // TODO: must be implemented new
        // Have full qualified file name.
        if( DrgVerifyType( pditem, "UniformResourceLocator" )) {
          amp_url_from_file( fullname, fullname, sizeof( fullname ));
        }
        /*
        if( is_dir( fullname )) {
          pl_add_directory( fullname, PL_DIR_RECURSIVE );
        } else if( is_playlist( fullname )) {
          pl_load( fullname, 0 );
        } else {
          pl_add_file( fullname, NULL, 0 );
        }*/
        if( pditem->hwndItem ) {
          // Tell the source you're done.
          DrgSendTransferMsg( pditem->hwndItem, DM_ENDCONVERSATION, (MPARAM)pditem->ulItemID,
                                                                    (MPARAM)DMFL_TARGETSUCCESSFUL );
        }
      }
      else if( pditem->hwndItem &&
               DrgVerifyType( pditem, "UniformResourceLocator" ))
      {
        // The droped item must be rendered.
        PDRAGTRANSFER pdtrans  = DrgAllocDragtransfer(1);
        AMP_DROPINFO* pdsource = (AMP_DROPINFO*)malloc( sizeof( AMP_DROPINFO ));
        char renderto[_MAX_PATH];

        if( !pdtrans || !pdsource ) {
          return 0;
        }

        pdsource->cditem   = pdinfo->cditem;
        pdsource->hwndItem = pditem->hwndItem;
        pdsource->ulItemID = pditem->ulItemID;

        pdtrans->cb               = sizeof( DRAGTRANSFER );
        pdtrans->hwndClient       = hwnd;
        pdtrans->pditem           = pditem;
        pdtrans->hstrSelectedRMF  = DrgAddStrHandle( "<DRM_OS2FILE,DRF_TEXT>" );
        pdtrans->hstrRenderToName = 0;
        pdtrans->ulTargetInfo     = (ULONG)pdsource;
        pdtrans->fsReply          = 0;
        pdtrans->usOperation      = pdinfo->usOperation;

        // Send the message before setting a render-to name.
        if( pditem->fsControl & DC_PREPAREITEM ) {
          DrgSendTransferMsg( pditem->hwndItem, DM_RENDERPREPARE, (MPARAM)pdtrans, 0 );
        }

        strlcpy( renderto, startpath , sizeof( renderto ));
        strlcat( renderto, "pm123.dd", sizeof( renderto ));

        pdtrans->hstrRenderToName = DrgAddStrHandle( renderto );

        // Send the message after setting a render-to name.
        if(( pditem->fsControl & ( DC_PREPARE | DC_PREPAREITEM )) == DC_PREPARE ) {
          DrgSendTransferMsg( pditem->hwndItem, DM_RENDERPREPARE, (MPARAM)pdtrans, 0 );
        }

        // Ask the source to render the selected item.
        DrgSendTransferMsg( pditem->hwndItem, DM_RENDER, (MPARAM)pdtrans, 0 );
      }
    }
    else if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ))
    {
      PLRECORD* rec = (PLRECORD*)pditem->ulItemID;

      if( pdinfo->hwndSource == hwnd ) {
        if( pdinfo->usOperation == DO_MOVE ) {
          ins = pl_move_record( rec, pos );
        } else {
          ins = pl_copy_record( rec, pos );
        }
      } else {
        ins = pl_create_record( fullname, pos, NULL, NULL, NULL );

        if( ins && pdinfo->usOperation == DO_MOVE ) {
          WinSendMsg( pdinfo->hwndSource, WM_PM123_REMOVE_RECORD, MPFROMP( rec ), 0 );
        }
      }
      if( ins ) {
        pos = ins;
      }
    }
  }

  DrgDeleteDraginfoStrHandles( pdinfo );
  DrgFreeDraginfo( pdinfo );
  return 0;
}

/* Receives dropped and rendered files and urls. */
static MRESULT
pl_drag_render_done( HWND hwnd, PDRAGTRANSFER pdtrans, USHORT rc )
{
  char rendered[_MAX_PATH];
  char fullname[_MAX_PATH];

  AMP_DROPINFO* pdsource = (AMP_DROPINFO*)pdtrans->ulTargetInfo;

  // If the rendering was successful, use the file, then delete it.
  if(( rc & DMFL_RENDEROK ) && pdsource &&
       DrgQueryStrName( pdtrans->hstrRenderToName, sizeof( rendered ), rendered ))
  {
    amp_url_from_file( fullname, rendered, sizeof( fullname ));
    DosDelete( rendered );

    // must be implemented new
    /*
    if( is_dir( fullname )) {
      pl_add_directory( fullname, PL_DIR_RECURSIVE );
    } else if( is_playlist( fullname )) {
      pl_load( fullname, 0 );
    } else {
      pl_add_file( fullname, NULL, 0 );
    }*/

    // Tell the source you're done.
    DrgSendTransferMsg( pdsource->hwndItem, DM_ENDCONVERSATION,
                       (MPARAM)pdsource->ulItemID, (MPARAM)DMFL_TARGETSUCCESSFUL );
    free( pdsource );
  }

  DrgDeleteStrHandle ( pdtrans->hstrSelectedRMF );
  DrgDeleteStrHandle ( pdtrans->hstrRenderToName );
  DrgFreeDragtransfer( pdtrans );
  return 0;
}

/* Initializes the playlist presentation window. */
static void
pl_init_window( HWND hwnd )
{
  FIELDINFO* first;
  FIELDINFO* field;
  HPOINTER   hicon;
  LONG       color;
  HACCEL     accel;
  MENUITEM   mi;
  HAB        hab = WinQueryAnchorBlock( hwnd );

  FIELDINFOINSERT insert;
  CNRINFO cnrinfo;

  container = WinWindowFromID( hwnd, CNR_PLAYLIST );

  /* Initializes the container of the playlist. */
  first = (FIELDINFO*)WinSendMsg( container, CM_ALLOCDETAILFIELDINFO, MPFROMSHORT(6), 0 );
  field = first;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_BITMAPORICON;
  field->pTitleData = "";
  field->offStruct  = offsetof( PLRECORD, rc.hptrIcon);

  field = field->pNextFieldInfo;

  field->flData     = CFA_STRING | CFA_HORZSEPARATOR;
  field->pTitleData = "Filename";
  field->offStruct  = offsetof( PLRECORD, rc.pszIcon );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Song name";
  field->offStruct  = offsetof( PLRECORD, songname );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Size";
  field->offStruct  = offsetof( PLRECORD, size );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Time";
  field->offStruct  = offsetof( PLRECORD, time );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Information";
  field->offStruct  = offsetof( PLRECORD, moreinfo );

  insert.cb = sizeof(FIELDINFOINSERT);
  insert.pFieldInfoOrder = (PFIELDINFO)CMA_FIRST;
  insert.fInvalidateFieldInfo = TRUE;
  insert.cFieldInfoInsert = 6;

  WinSendMsg( container, CM_INSERTDETAILFIELDINFO,
              MPFROMP( first ), MPFROMP( &insert ));

  cnrinfo.cb             = sizeof(cnrinfo);
  cnrinfo.pFieldInfoLast = first->pNextFieldInfo;
  cnrinfo.flWindowAttr   = CV_DETAIL | CV_MINI  | CA_DRAWICON |
                           CA_DETAILSVIEWTITLES | CA_ORDEREDTARGETEMPH;
  cnrinfo.xVertSplitbar  = 180;

  WinSendMsg( container, CM_SETCNRINFO, MPFROMP(&cnrinfo),
              MPFROMLONG( CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR | CMA_FLWINDOWATTR ));

  /* Initializes the playlist presentation window. */
  accel = WinLoadAccelTable( hab, NULLHANDLE, ACL_PLAYLIST );

  // TODO: acceleration table entries for plug-in extensions
  if( accel ) {
    WinSetAccelTable( hab, accel, hwnd );
  }

  hicon = WinLoadPointer( HWND_DESKTOP, 0, ICO_MAIN );
  WinSendMsg( hwnd, WM_SETICON, (MPARAM)hicon, 0 );
  do_warpsans( hwnd );

  if( !rest_window_pos( hwnd, 0 ))
  {
    color = CLR_GREEN;
    WinSetPresParam( container, PP_FOREGROUNDCOLORINDEX,
                     sizeof(color), &color);
    color = CLR_BLACK;
    WinSetPresParam( container, PP_BACKGROUNDCOLORINDEX,
                     sizeof(color), &color);
  }

  menu_playlist = WinLoadMenu( HWND_OBJECT, 0, MNU_PLAYLIST );
  menu_record   = WinLoadMenu( HWND_OBJECT, 0, MNU_RECORD   );

  WinSendMsg( menu_playlist, MM_QUERYITEM, ////
              MPFROM2SHORT( IDM_PL_SAVE, TRUE ), MPFROMP( &mi ));

  WinSetWindowULong( mi.hwndSubMenu, QWL_STYLE,
    WinQueryWindowULong( mi.hwndSubMenu, QWL_STYLE ) | MS_CONDITIONALCASCADE );

  WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID,
                              MPFROMLONG( IDM_PL_LST_SAVE ), 0 );

  WinSendMsg( menu_playlist, MM_QUERYITEM,
              MPFROM2SHORT( IDM_PL_OPEN, TRUE ), MPFROMP( &mi ));

  WinSetWindowULong( mi.hwndSubMenu, QWL_STYLE,
    WinQueryWindowULong( mi.hwndSubMenu, QWL_STYLE ) | MS_CONDITIONALCASCADE );

  WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID,
                              MPFROMLONG( IDM_PL_OPENL ), 0 );

  WinSendMsg( menu_playlist, MM_QUERYITEM,
              MPFROM2SHORT( IDM_PL_APPEND, TRUE ), MPFROMP( &mi ));

  WinSetWindowULong( mi.hwndSubMenu, QWL_STYLE,
    WinQueryWindowULong( mi.hwndSubMenu, QWL_STYLE ) | MS_CONDITIONALCASCADE );

  WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID,
                              MPFROMLONG( IDM_PL_OPEN ), 0 );

  broker_queue = qu_create();

  if( !broker_queue ) {
    amp_player_error( "Unable create playlist service queue." );
  } else {
    if(( broker_tid = _beginthread( pl_broker, NULL, 204800, NULL )) == -1 ) {
      amp_error( container, "Unable create the playlist service thread." );
    }
  }
}

static void DLLENTRY
pl_add_callback(void* param, const char* url)
{ //char buf[_MAX_FNAME];
  //char* cp;
  DEBUGLOG(("pl_add_callback(, %s)\n", url));
  if (strncmp(url, "file:", 5) == 0)
  { url += 5; // skip "file:";
    if (url[2] == '/')
      url += 3; // skip "///" if not UNC.
  }
  // TODO: can't handle folders here
  //pl_add_file(url, NULL, 0);
}

/* Processes messages of the playlist presentation window. */
static MRESULT EXPENTRY
pl_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_INITDLG: ////
      pl_init_window( hwnd );
      dk_add_window ( hwnd, 0 );
      break;

    case WM_HELP: ////
      amp_show_help( IDH_PL );
      return 0;

    case WM_SYSCOMMAND: ////
      if( SHORT1FROMMP(mp1) == SC_CLOSE ) {
        pl_show( FALSE );
        return 0;
      }
      break;

    case WM_WINDOWPOSCHANGED: ////
    {
      SWP* pswp = (SWP*)PVOIDFROMMP(mp1);

      if( pswp[0].fl & SWP_SHOW ) {
        cfg.show_playlist = TRUE;
      }
      if( pswp[0].fl & SWP_HIDE ) {
        cfg.show_playlist = FALSE;
      }
      break;
    }

    case DM_DISCARDOBJECT:
      return pl_drag_discard( hwnd, (PDRAGINFO)mp1 );
    case DM_RENDERCOMPLETE:
      return pl_drag_render_done( hwnd, (PDRAGTRANSFER)mp1, SHORT1FROMMP( mp2 ));

    case WM_PM123_REMOVE_RECORD: //// X
    {
      PLRECORD* rec = (PLRECORD*)mp1;
      pl_remove_record( &rec, 1 );
      return 0;
    }

    case WM_COMMAND:
    { CMDMSG* cm = COMMANDMSG(&msg);
      if( cm->cmd >  IDM_PL_OPENLAST &&
          cm->cmd <= IDM_PL_OPENLAST + MAX_RECALL )
      {
        char filename[_MAX_PATH];
        strcpy( filename, cfg.list[ cm->cmd - IDM_PL_OPENLAST - 1 ]);

        if( is_playlist( filename )) {
          pl_load( filename, PL_LOAD_CLEAR );
        }
        return 0;
      }     
      if( cm->cmd >= IDM_PL_APPOTHER &&
          cm->cmd <  IDM_PL_APPOTHER + sizeof assists / sizeof *assists &&
          assists[cm->cmd-IDM_PL_APPOTHER] )
      {
        (*assists[cm->cmd-IDM_PL_APPOTHER])(hwnd, "Add%s to playlist", &pl_add_callback, NULL);
        return 0;
      } 

      switch( COMMANDMSG(&msg)->cmd ) {
        case IDM_PL_SORT_RANDOM:
          pl_sort( PL_SORT_RAND );
          return 0;
        case IDM_PL_SORT_SIZE:
          pl_sort( PL_SORT_SIZE );
          return 0;
        case IDM_PL_SORT_PLTIME:
          pl_sort( PL_SORT_TIME );
          return 0;
        case IDM_PL_SORT_NAME:
          pl_sort( PL_SORT_NAME );
          return 0;
        case IDM_PL_SORT_SONG:
          pl_sort( PL_SORT_SONG );
          return 0;
        case IDM_PL_CLEAR: ////
          pl_purge_queue( broker_queue );
          qu_push( broker_queue, PL_CLEAR, pl_create_request_data( NULL, NULL, PL_CLR_NEW ));
          return 0;
        /*case IDM_PL_USE:
          TODO: wee need a more sophisticated approach here
          if( amp_playmode == AMP_PLAYLIST ) {
            amp_pl_release();
          } else {
            amp_pl_use();
          }
          return 0;*/
        case IDM_PL_APPURL: ////
          // TODO: already implemented in pfreq_base...
          //amp_add_url( hwnd, URL_ADD_TO_LIST );
          return 0;
        case IDM_PL_APPFILE: ////
          // TODO: already implemented in pfreq_base...
          //amp_add_files( hwnd );
          return 0;
        case IDM_PL_LST_SAVE:
          amp_save_list_as( hwnd, SAV_LST_PLAYLIST );
          return 0;
        case IDM_PL_M3U_SAVE:
          amp_save_list_as( hwnd, SAV_M3U_PLAYLIST );
          return 0;
        case IDM_PL_OPENL:
          // TODO: probably no longer needed
          //amp_load_list( hwnd );
          return 0;
        case IDM_PL_USE: ////
          amp_load_playable(url::normalizeURL(pl_cursored()->full), AMP_LOAD_KEEP_PLAYLIST);
          return 0;
        case IDM_PL_REMOVE: ////
          pl_remove_selected();
          return 0;
/*        case IDM_PL_S_KILL:
          pl_delete_selected();
          return 0;*/

        case IDM_PL_MENUCONT:
          WinSendMsg( hwnd, WM_CONTROL,
                      MPFROM2SHORT( CNR_PLAYLIST, CN_CONTEXTMENU ),
                      PVOIDFROMMP( NULL ));
          return 0;

        case IDM_PL_MENU:
          WinSendMsg( hwnd, WM_CONTROL,
                      MPFROM2SHORT( CNR_PLAYLIST, CN_CONTEXTMENU ),
                      PVOIDFROMMP( pl_cursored()));
          return 0;

        case IDM_PL_EDIT: ////
        {
          PLRECORD* rec = pl_cursored();
          if( rec ) {
            amp_info_edit( hwnd, rec->full, rec->decoder );
          }
          return 0;
        }
      }
      break;
    }
    case WM_CONTROL:
      switch( SHORT2FROMMP( mp1 )) {
        case CN_CONTEXTMENU:
          pl_show_context_menu( hwnd, (PLRECORD*)mp2 );
          return 0;

        case CN_HELP:
          amp_show_help( IDH_PL );
          return 0;

        case CN_ENTER:
        {
          NOTIFYRECORDENTER* notify = (NOTIFYRECORDENTER*)mp2;
          if( notify->pRecord ) {
            // TODO: we should use this list first...
            amp_load_playable(url::normalizeURL(((PLRECORD*)notify->pRecord)->full), AMP_LOAD_KEEP_PLAYLIST|AMP_LOAD_NOT_PLAY );
            amp_play(0);
          }
          return 0;
        }

        case CN_INITDRAG:
          return pl_drag_init( hwnd, (PCNRDRAGINIT)mp2 );
        case CN_DRAGAFTER:
          return pl_drag_over( hwnd, (PCNRDRAGINFO)mp2 );
        case CN_DROP:
          return pl_drag_drop( hwnd, (PCNRDRAGINFO)mp2 );
      }
      break;
  }

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

static PlaylistView* DefaultPL = NULL;

/* Sets the visibility state of the playlist presentation window. */
void
pl_show( BOOL show )
{
  if (DefaultPL)
    DefaultPL->SetVisible(show);

  /*HSWITCH hswitch = WinQuerySwitchHandle( playlist, 0 );
  SWCNTRL swcntrl;

  if( WinQuerySwitchEntry( hswitch, &swcntrl ) == 0 ) {
    swcntrl.uchVisibility = show ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry( hswitch, &swcntrl );
  }

  dk_set_state( playlist, show ? 0 : DK_IS_GHOST );
  WinSetWindowPos( playlist, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );*/
}

BOOL pl_visible()
{ return DefaultPL && DefaultPL->GetVisible();
}

/* Creates the playlist presentation window. */
void
pl_create( void )
{
  //playlist = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
  //                       pl_dlg_proc, NULLHANDLE, DLG_PLAYLIST, NULL );
  //pl_show( cfg.show_playlist );
  
  PlaylistView::Init();
  url path = url::normalizeURL(startpath);
  const xstring& file = path + "PM123.LST";
  DEBUGLOG(("pl_create - %s\n", file.cdata()));
  DefaultPL = PlaylistView::Get(file, "Default Playlist");
  DefaultPL->SetVisible(cfg.show_playlist);
}

/* Destroys the playlist presentation window. */
void
pl_destroy( void )
{
  /*HAB    hab   = WinQueryAnchorBlock( playlist );
  HACCEL accel = WinQueryAccelTable ( hab, playlist );

  pl_purge_queue( broker_queue );
  qu_push( broker_queue, PL_TERMINATE,
           pl_create_request_data( NULL, NULL, 0 ));

  wait_thread( broker_tid, 2000 );
  qu_close( broker_queue );

  save_window_pos( playlist, 0 );

  WinSetAccelTable( hab, playlist, NULLHANDLE );
  if( accel != NULLHANDLE ) {
    WinDestroyAccelTable( accel );
  }

  pl_remove_all();

  WinDestroyWindow( menu_record   );
  WinDestroyWindow( menu_playlist );
  WinDestroyWindow( playlist      );*/
  
  DefaultPL = NULL;
  PlaylistView::UnInit();
  
}

/* Sends request about clearing of the playlist. */
static BOOL
pl_clear( int options )
{
  pl_purge_queue( broker_queue );
  return qu_push( broker_queue, PL_CLEAR,
                  pl_create_request_data( NULL, NULL, options ));
}

/* Sends request about addition of the whole directory to the playlist. */
/*BOOL
pl_add_directory( const char* path, int options )
{
  return qu_write( broker_queue, PL_ADD_DIRECTORY,
                   pl_create_request_data( path, NULL, options ));
}*/

/* Sends request about addition of the file to the playlist. */
/*BOOL
pl_add_file( const char* filename, const char* songname, int options )
{
  return qu_write( broker_queue, PL_ADD_FILE,
                   pl_create_request_data( filename, songname, options ));
}*/

/* Notifies on completion of the playlist */
/*BOOL
pl_completed( void )
{
  return qu_write( broker_queue, PL_COMPLETED,
                   pl_create_request_data( NULL, NULL, 0 ));
}*/

/* Returns true if the specified file is a playlist file. */
static BOOL
is_playlist( const char *filename )
{
 char ext[_MAX_EXT];
 sfext( ext, filename, sizeof( ext ));
 return ( stricmp( ext, ".lst" ) == 0 ||
          stricmp( ext, ".mpl" ) == 0 ||
          stricmp( ext, ".pls" ) == 0 ||
          stricmp( ext, ".m3u" ) == 0 );
}

/* Loads the PM123 native playlist file. */
/*static BOOL
pl_load_lst_list( const char *filename, int options )
{
  char   basepath[_MAX_PATH];
  char   fullname[_MAX_PATH];
  char   file    [_MAX_PATH];

  XFILE* playlist = xio_fopen( filename, "r" );

  if( !playlist ) {
    amp_error( container, "Unable open playlist file:\n%s\n%s",
                          filename, xio_strerror( xio_errno()));
    return FALSE;
  }

  sdrivedir( basepath, filename, sizeof( basepath ));

  if( options & PL_LOAD_CLEAR ) {
    pl_clear( 0 );
  }

  while( xio_fgets( file, sizeof(file), playlist ))
  {
    blank_strip( file );

    if( *file != 0 && *file != '#' && *file != '>' && *file != '<' ) {
      if( rel2abs( basepath, file, fullname, sizeof(fullname))) {
        strcpy( file, fullname );
      }
      pl_add_file( file, NULL, 0 );
    }
  }
  xio_fclose( playlist );
  return TRUE;
}*/

/* Loads the M3U playlist file. */
/*static BOOL
pl_load_m3u_list( const char *filename, int options ) {
  return pl_load_lst_list( filename, options );
}*/

/* Loads the WarpAMP playlist file. */
/*static BOOL
pl_load_mpl_list( const char *filename, int options )
{
  char   basepath[_MAX_PATH];
  char   fullname[_MAX_PATH];
  char   file    [_MAX_PATH];
  char*  eq_pos;
  XFILE* playlist = xio_fopen( filename, "r" );

  if( !playlist ) {
    amp_error( container, "Unable open playlist file:\n%s\n%s",
                          filename, xio_strerror( xio_errno()));
    return FALSE;
  }

  sdrivedir( basepath, filename, sizeof( basepath ));

  if( options & PL_LOAD_CLEAR ) {
    pl_clear( 0 );
  }

  while( xio_fgets( file, sizeof(file), playlist ))
  {
    blank_strip( file );

    if( *file != 0 && *file != '#' && *file != '[' && *file != '>' && *file != '<' )
    {
      eq_pos = strchr( file, '=' );

      if( eq_pos && strnicmp( file, "File", 4 ) == 0 )
      {
        strcpy( file, eq_pos + 1 );
        if( rel2abs( basepath, file, fullname, sizeof(fullname))) {
          strcpy( file, fullname );
        }
        pl_add_file( file, NULL, 0 );
      }
    }
  }
  xio_fclose( playlist );
  return TRUE;
}*/

/* Loads the WinAMP playlist file. */
/*static BOOL
pl_load_pls_list( const char *filename, int options )
{
  char   basepath[_MAX_PATH];
  char   fullname[_MAX_PATH];
  char   file    [_MAX_PATH] = "";
  char   title   [_MAX_PATH] = "";
  char   line    [_MAX_PATH];
  int    last_idx = -1;
  BOOL   have_title = FALSE;
  char*  eq_pos;
  XFILE* playlist = xio_fopen( filename, "r" );

  if( !playlist ) {
    amp_error( container, "Unable open playlist file:\n%s\n%s",
                          filename, xio_strerror( xio_errno()));
    return FALSE;
  }

  sdrivedir( basepath, filename, sizeof( basepath ));

  if( options & PL_LOAD_CLEAR ) {
    pl_clear( 0 );
  }

  while( xio_fgets( line, sizeof(line), playlist ))
  {
    blank_strip( line );

    if( *line != 0 && *line != '#' && *line != '[' && *line != '>' && *line != '<' )
    {
      eq_pos = strchr( line, '=' );

      if( eq_pos ) {
        if( strnicmp( line, "File", 4 ) == 0 )
        {
          if( *file ) {
            pl_add_file( file, have_title ? title : NULL, 0 );
            have_title = FALSE;
          }

          strcpy( file, eq_pos + 1 );
          if( rel2abs( basepath, file, fullname, sizeof(fullname))) {
            strcpy( file, fullname );
          }
          last_idx = atoi( &line[4] );
        }
        else if( strnicmp( line, "Title", 5 ) == 0 )
        {
          // We hope the title field always follows the file field
          if( last_idx == atoi( &line[5] )) {
            strcpy( title, eq_pos + 1 );
            have_title = TRUE;
          }
        }
      }
    }
  }

  if( *file ) {
    pl_add_file( file, have_title ? title : NULL, 0 );
  }

  xio_fclose( playlist );
  return TRUE;
}*/

/* Loads the specified playlist file. */
BOOL pl_load( const char *filename, int options )
{
  BOOL rc = FALSE;
  /* DISCONTINUED
  char ext[_MAX_EXT];
  int  i;

  sfext( ext, filename, sizeof( ext ));

  if( stricmp( ext, ".lst" ) == 0 ) {
    rc = pl_load_lst_list( filename, options );
  }
  if( stricmp( ext, ".m3u" ) == 0 ) {
    rc = pl_load_m3u_list( filename, options );
  }
  if( stricmp( ext, ".mpl" ) == 0 ) {
    rc = pl_load_mpl_list( filename, options );
  }
  if( stricmp( ext, ".pls" ) == 0 ) {
    rc = pl_load_pls_list( filename, options );
  }

  if( rc ) {
    pl_completed();

    strcpy( current_playlist, filename );
    pl_display_status();

    if( !(options & PL_LOAD_NOT_RECALL )) {
      for( i = 0; i < MAX_RECALL; i++ ) {
        if( stricmp( cfg.list[i], filename ) == 0 ) {
          while( ++i < MAX_RECALL ) {
            strcpy( cfg.list[i-1], cfg.list[i] );
          }
          break;
        }
      }

      for( i = MAX_RECALL - 2; i >= 0; i-- ) {
        strcpy( cfg.list[i + 1], cfg.list[i] );
      }

      strcpy( cfg.list[0], filename );
    }
  }*/

  return rc;
}

/* Saves playlist to the specified file. */
BOOL
pl_save( const char* filename, int options )
{
  PLRECORD* rec;
  FILE*     playlist = fopen( filename, "w" );
  char      base[_MAX_PATH];
  char      path[_MAX_PATH];

  if( !playlist ) {
    amp_error( container, "Unable open playlist file:\n%s\n%s", filename, strerror(errno));
    return FALSE;
  }

  if( !(options & PL_SAVE_M3U )) {
    fprintf( playlist, "#\n"
                       "# Playlist created with %s\n"
                       "# Do not modify!\n"
                       "# Lines starting with '>' are used by Playlist Manager.\n"
                       "#\n", AMP_FULLNAME );
  }

  sdrivedir( base, filename, sizeof( base ));
  for( rec = pl_first_record(); rec; rec = pl_next_record( rec ))
  {
    if( !(options & PL_SAVE_M3U )) {
      fprintf( playlist, "# %s, %s, %s\n", rec->size, rec->time, rec->moreinfo );
    }

    if( options & PL_SAVE_RELATIVE
        && is_file( rec->full )
        && abs2rel( base, rec->full, path, sizeof(path)))
    {
      fprintf( playlist, "%s\n", path );
    } else {
      fprintf( playlist, "%s\n", rec->full );
    }

    if( !(options & PL_SAVE_M3U )) {
      fprintf( playlist, ">%u,%u,%u,%u,%u\n",
               rec->info.tech->bitrate, rec->info.format->samplerate,
               rec->info.format->channels == 2 ? 0 : 3, rec->info.tech->filesize,
               rec->info.tech->songlength / 1000 );
    }
  }

  if( !(options & PL_SAVE_M3U )) {
    fprintf( playlist, "# End of playlist\n" );
  }

  fclose( playlist );
  strcpy( current_playlist, filename );

  pl_display_status();
  return TRUE;
}

/* Saves the playlist and player status to the specified file. */
BOOL
pl_save_bundle( const char* filename, int options )
{
  /*PLRECORD* rec;
  FILE* playlist = fopen( filename, "w" );

  if( !playlist ) {
    amp_error( container, "Unable create status file:\n%s\n%s", filename, strerror(errno));
    return FALSE;
  }

  fprintf( playlist, "#\n"
                     "# Player state file created with %s\n"
                     "# Do not modify! This file is compatible with the playlist format,\n"
                     "# but information written in this file is different.\n"
                     "#\n", AMP_FULLNAME );

  for( rec = pl_first_record(); rec; rec = pl_next_record( rec ))
  {
    fprintf( playlist, ">%u,%u\n",
             rec->rc.flRecordAttr & CRA_CURSORED ? 1 : 0, rec == current_record );
    fprintf( playlist, "%s\n" , rec->full );
  }

  // TODO: this is completely wrong here
  //current = amp_get_current_file();
  //if( amp_playmode == AMP_SINGLE && current != NULL && *current->url && is_file(current->url)) {
  //  fprintf( playlist, "<%s\n", current->url );
  //}

  fprintf( playlist, "# End of playlist\n" );
  fclose ( playlist );*/
  return TRUE;
}

/* Loads the playlist and player status from specified file. */
BOOL
pl_load_bundle( const char *filename, int options )
{
/*  char  file[_MAX_PATH];
  BOOL  selected = FALSE;
  BOOL  loaded   = FALSE;
  FILE* playlist = fopen( filename, "r" );

  if( !playlist ) {
    amp_error( container, "Unable open status file:\n%s\n%s", filename, strerror( errno ));
    return FALSE;
  }

  pl_clear( 0 );

  while( fgets( file, sizeof(file), playlist ))
  {
    blank_strip( file );

    if( *file == '<' ) {
      struct stat fi;
      if( stat( file + 1, &fi ) == 0 ) {
        amp_load_playable( url::normalizeURL(file + 1), AMP_LOAD_NOT_PLAY | AMP_LOAD_NOT_RECALL );
      }
    } else if( *file == '>' ) {
      sscanf( file, ">%lu,%lu\n", &selected, &loaded );
    } else if( *file != 0 && *file != '#' ) {
      // TODO: reimplement onother way
      //pl_add_file( file, NULL, ( selected ? PL_ADD_SELECT : 0 ) |
      //                         ( loaded   ? PL_ADD_LOAD   : 0 ));
    }
  }
  fclose( playlist );*/
  return TRUE;
}
