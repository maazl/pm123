/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <stdio.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "playlist.h"
#include "pm123.h"
#include "utilfct.h"
#include "httpget.h"
#include "plugman.h"

#define PL_ADD_FILE      0
#define PL_ADD_DIRECTORY 1
#define PL_COMPLETED     2
#define PL_CLEAR         3
#define PL_TERMINATE     4

#define PR_ADD_FILE      0
#define PR_ADD_DIRECTORY 0
#define PR_COMPLETED     0
#define PR_CLEAR        14
#define PR_TERMINATE    15

typedef struct _PL_DATA {
  char* filename;
  char* title;
  int   options;
} PLDATA;

static  TID     tid       = 0;
static  HAB     hab       = NULLHANDLE;
static  HMQ     hmq       = NULLHANDLE;
static  HQUEUE  hqueue    = NULLHANDLE;
static  HWND    container = NULLHANDLE;
static  HEV     init_done = NULLHANDLE;
static  BOOL    is_busy   = FALSE;

static char current_playlist[_MAX_PATH];

BOOL pl_scroll_to_record( PLRECORD* );
BOOL pl_peek_queue( ULONG, ULONG );

/* Sets the title of the playlist window according to current
   playlist state. */
void
pl_display_status( void )
{
  char title[ sizeof( _MAX_FNAME ) + 128 ];

  strcpy( title, "PM123 Playlist"  );

  if( *current_playlist ) {
    strcat( title, " - " );
    sfnameext( title + strlen( title ), current_playlist );
  }

  if( amp_playmode == AMP_PLAYLIST ) {
    strcat( title, " - [USED]" );
  }

  if( is_busy ) {
    strcat( title, " - loading..." );
  }

  pl_set_title( title );
}

/* Frees the data contained in playlist record. */
static void
pl_free_record( PLRECORD* rec )
{
  free( rec->rc.pszIcon );
  free( rec->songname );
  free( rec->length );
  free( rec->time );
  free( rec->info );
  free( rec->full );
  free( rec->info_string );
  free( rec->comment );
}

/* Removes the specified playlist record. */
void
pl_remove_record( PLRECORD** array, USHORT count )
{
  PLRECORD* load_after = (PLRECORD*)-1;
  int i;

  if( amp_playmode == AMP_PLAYLIST ) {
    for( i = count; i >= 0; i-- ) {
      if( array[i] == currentf || array[i] == load_after ) {
        if( decoder_playing()) {
          amp_stop();
        }

        load_after = pl_prev_record( array[i] );
      }
    }
  }

  if( WinSendMsg( container, CM_REMOVERECORD, MPFROMP( array ),
                  MPFROM2SHORT( count, CMA_INVALIDATE )) != MRFROMLONG( -1 ))
  {
    if( amp_playmode == AMP_PLAYLIST ) {
      if( pl_size() == 0 ) {
        amp_reset();
      } else {
        if( load_after == NULL ) {
          amp_pl_load_record( pl_first_record());
        } else if( load_after != (PLRECORD*)-1 ) {
          amp_pl_load_record( load_after );
        }
      }
    }

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

   if( amp_playmode == AMP_PLAYLIST ) {
     amp_reset();
   }

   for( rec = pl_first_record(); rec; rec = pl_next_record( rec )) {
     pl_free_record( rec );
   }

   WinSendMsg( container, CM_REMOVERECORD,
               MPFROMP(NULL), MPFROM2SHORT( 0, CMA_FREE | CMA_INVALIDATE ));
}

/* Copies the playlist record to specified position. */
PLRECORD*
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
  copy->bitrate         = rec->bitrate;
  copy->channels        = rec->channels;
  copy->secs            = rec->secs;
  copy->freq            = rec->freq;
  copy->format          = rec->format;
  copy->track           = rec->track;
  copy->size            = rec->size;
  copy->rc.hptrIcon     = mp3;
  copy->full            = strdup( rec->full );
  copy->length          = strdup( rec->length );
  copy->comment         = strdup( rec->comment );
  copy->songname        = strdup( rec->songname );
  copy->info            = strdup( rec->info );
  copy->time            = strdup( rec->time );
  copy->info_string     = strdup( rec->info_string );
  copy->rc.pszIcon      = strdup( rec->rc.pszIcon );
  copy->played          = FALSE;
  copy->play            = FALSE;

  strcpy( copy->cd_drive, rec->cd_drive );
  strcpy( copy->decoder_module_name, rec->decoder_module_name );

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
PLRECORD*
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

/* Refreshes the specified playlist record. */
void
pl_refresh_record( PLRECORD* rec )
{
  WinSendMsg( container, CM_INVALIDATERECORD,
              MPFROMP( &rec ), MPFROM2SHORT( 1, 0 ));
}

/* Returns the pointer to the first playlist record. */
PLRECORD*
pl_first_record( void )
{
  PLRECORD* result = (PLRECORD*)WinSendMsg( container, CM_QUERYRECORD, NULL,
                                            MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER ));
  if( result == (PLRECORD*)-1 ) {
    result = NULL;
  }

  return result;
}

/* Returns the pointer to the last playlist record. */
PLRECORD*
pl_last_record( void )
{
  PLRECORD* result = (PLRECORD*)WinSendMsg( container, CM_QUERYRECORD, NULL,
                                            MPFROM2SHORT( CMA_LAST, CMA_ITEMORDER ));
  if( result == (PLRECORD*)-1 ) {
    result = NULL;
  }

  return result;
}

/* Returns the pointer to the next playlist record of specified. */
PLRECORD*
pl_next_record( PLRECORD* rec )
{
  PLRECORD* result = (PLRECORD*)WinSendMsg( container, CM_QUERYRECORD, MPFROMP(rec),
                                            MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER ));
  if( result == (PLRECORD*)-1 ) {
    result = NULL;
  }

  return result;
}

/* Returns the pointer to the previous playlist record of specified. */
PLRECORD*
pl_prev_record( PLRECORD* rec )
{
  PLRECORD* result = (PLRECORD*)WinSendMsg( container, CM_QUERYRECORD, MPFROMP(rec),
                                            MPFROM2SHORT( CMA_PREV, CMA_ITEMORDER ));
  if( result == (PLRECORD*)-1 ) {
    result = NULL;
  }

  return result;
}

/* Returns the pointer to the first selected playlist record. */
PLRECORD*
pl_first_selected( void )
{
  PLRECORD* result = (PLRECORD*)WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_SELECTED ));
  if( result == (PLRECORD*)-1 ) {
    result = NULL;
  }

  return result;
}

/* Returns the pointer to the next selected playlist record of specified. */
PLRECORD*
pl_next_selected( PLRECORD* rec )
{
  PLRECORD* result = (PLRECORD*)WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                                            MPFROMP( rec ), MPFROMSHORT( CRA_SELECTED ));
  if( result == (PLRECORD*)-1 ) {
    result = NULL;
  }

  return result;
}

/* Returns the pointer to the cursored playlist record. */
PLRECORD*
pl_cursored( void )
{
  PLRECORD* result = (PLRECORD*)WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_CURSORED ));
  if( result == (PLRECORD*)-1 ) {
    result = NULL;
  }

  return result;
}

/* Returns the pointer to the record of the specified file. */
PLRECORD*
pl_find_filename( const char* filename )
{
  PLRECORD* rec;

  for( rec = pl_first_record(); rec; rec = pl_next_record( rec )) {
    if( strcmp( rec->full, filename ) == 0 ) {
      break;
    }
  }

  return rec;
}

/* Returns the pointer to the random playlist record. */
PLRECORD*
pl_random_record( void )
{
  int pos = rand() % pl_size();
  PLRECORD* rec;

  for( rec = pl_first_record(); rec && pos; rec = pl_next_record( rec )) {
    --pos;
  }

  return rec;
}

/* Returns a ordinal number of the specified record. */
int
pl_index( PLRECORD* find )
{
  int pos = 1;
  PLRECORD* rec;

  for( rec = pl_first_record(); rec; rec = pl_next_record( rec ), ++pos ) {
    if( rec == find ) {
      return pos;
    }
  }

  return 0;
}

/* Returns the number of records in the playlist. */
int
pl_size( void )
{
  CNRINFO info;
  if( WinSendMsg( container, CM_QUERYCNRINFO,
                  MPFROMP(&info), MPFROMLONG(sizeof(info))) != 0 )
  {
    return info.cRecords;
  } else {
    return 0;
  }
}

/* Returns a summary play time of the remained part of the playlist. */
ULONG pl_playtime( BOOL only_non_played )
{
  ULONG time = 0;
  PLRECORD* rec = currentf;

  if( only_non_played || !rec ) {
    rec = pl_first_record();
  }

  while( rec ) {
    if( !rec->played || rec == currentf || !only_non_played ) {
      time += rec->secs;
    }
    rec = pl_next_record( rec );
  }

  return time;
}

/* Selects the specified record and deselects all others. */
void
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

static SHORT EXPENTRY
pl_compare_rand( PLRECORD* p1, PPLRECORD* p2, PVOID pStorage )
{
  return ( rand() % 2 ) - 1;
}

static SHORT EXPENTRY
pl_compare_size( PLRECORD* p1, PLRECORD* p2, PVOID pStorage )
{
  if( p1->size < p2->size ) return -1;
  if( p1->size > p2->size ) return  1;
  return 0;
}

static SHORT EXPENTRY
pl_compare_time( PLRECORD* p1, PLRECORD* p2, PVOID pStorage )
{
  if( p1->secs < p2->secs ) return -1;
  if( p1->secs > p2->secs ) return  1;
  return 0;
}

static SHORT EXPENTRY
pl_compare_name( PLRECORD* p1, PLRECORD* p2, PVOID pStorage )
{
  char s1[_MAX_PATH], s2[_MAX_PATH];

  sfname( s1, p1->full );
  sfname( s2, p2->full );

  return stricmp( s1, s2 );
}

static SHORT EXPENTRY
pl_compare_song( PLRECORD* p1, PLRECORD* p2, PVOID pStorage )
{
  return stricmp( p1->songname, p2->songname );
}

/* Sorts the playlist records. */
void
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

/* Assigns the specified file tag to the specified playlist record. */
void
pl_set_tag( PLRECORD* rec, const tune* tag, const char* title )
{
  free( rec->comment  );
  free( rec->songname );
  free( rec->info     );

  rec->comment = strdup( tag->comment );

  /* Songname */
  if( title ) {
    rec->songname = strdup( title );
  } else {
    rec->songname = malloc( strlen( tag->artist ) + 2 +
                            strlen( tag->title  ) + 1 );
    if( rec->songname ) {
      strcpy( rec->songname, tag->artist );
      if( *tag->title && *tag->artist ) {
        strcat( rec->songname, "- " );
      }
      strcat( rec->songname, tag->title );
    }
  }

  /* Information */
  rec->info = malloc( strlen( tag->album       ) + 1 +
                      strlen( tag->year        ) + 2 +
                      strlen( tag->genre       ) + 2 +
                      strlen( rec->info_string ) +
                      strlen( tag->comment     ) + 12 );
  if( rec->info ) {
    strcpy( rec->info, tag->album );

    if( *tag->album ) {
      strcat( rec->info, " "  );
    }
    if( *tag->year  ) {
      strcat( rec->info, tag->year );
      strcat( rec->info, ", " );
    }
    if( *tag->genre ) {
      strcat( rec->info, tag->genre );
      strcat( rec->info, ", " );
    }

    strcat( rec->info, rec->info_string );

    if( *tag->comment ) {
      strcat( rec->info, ", comment: " );
      strcat( rec->info, tag->comment  );
    }
  }
}

/* Creates the playlist record for specified file. */
PLRECORD*
pl_create_record( const char *filename, PLRECORD* pos, const char *title )
{
  DECODER_INFO info;
  struct stat  fi = {0};
  char         cd_drive[4] = "1:";
  int          cd_track    = -1;
  char         module_name[_MAX_FNAME];
  PLRECORD*    rec;
  tune         tag;
  int          rc;
  char         buffer[_MAX_PATH];
  RECORDINSERT insert;

  if( !filename ) {
    return NULL;
  }

  memset( &info, 0, sizeof( info ));

  if( is_file( filename ) && stat( filename, &fi ) != 0 ) {
    amp_error( container, "Unable load file:\n%s\n%s", filename, clib_strerror(errno));
    return NULL;
  }

  /* Decide wether this is a CD track, or a normal file. */
  if( is_track( filename ))
  {
    sscanf( filename, "cd:///%1c:\\Track %d", cd_drive, &cd_track );
    if( cd_drive[0] == '1' || cd_track == -1 ) {
      amp_error( container, "Invalid CD URL:\n%s", filename );
      return NULL;
    }
    rc = dec_trackinfo( cd_drive, cd_track, &info, module_name );
  } else {
    rc = dec_fileinfo((char*)filename, &info, module_name );
    cd_drive[0] = 0;
  }

  if( rc != 0 ) { /* error, invalid file */
    handle_dfi_error( rc, filename );
    return NULL;
  }

  /* Load ID3 tag */
  amp_gettag( filename, &info, &tag );

  /* Allocate a new record */
  rec = (PLRECORD*)WinSendMsg( container, CM_ALLOCRECORD,
                               MPFROMLONG( sizeof( PLRECORD ) - sizeof( RECORDCORE )),
                               MPFROMLONG( 1 ));

  rec->rc.cb           = sizeof(RECORDCORE);
  rec->rc.flRecordAttr = CRA_DROPONABLE;
  rec->bitrate         = info.bitrate;
  rec->channels        = info.mode;
  rec->secs            = info.songlength/1000;
  rec->freq            = info.format.samplerate;
  rec->format          = info.format;
  rec->track           = cd_track;
  rec->rc.hptrIcon     = mp3;
  rec->full            = strdup( filename );
  rec->size            = fi.st_size;
  rec->comment         = NULL;
  rec->songname        = NULL;
  rec->info            = NULL;

  strcpy( rec->cd_drive, cd_drive );
  strcpy( rec->decoder_module_name, module_name );

  sprintf( buffer, "%u kB", fi.st_size / 1024 );
  rec->length = strdup( buffer );
  sprintf( buffer, "%02u:%02u", rec->secs / 60, rec->secs % 60 );
  rec->time = strdup( buffer );

  /* Decoder info string needed for eventual playback */
  rec->info_string = strdup(info.tech_info);

  /* File name or host or cd track */
  if( is_http( filename ) &&
    ( strrchr( filename, '/' ) == filename + 6 || *( strrchr( filename, '/' ) + 1 ) == 0 ))
  {
    rec->rc.pszIcon = strdup( filename );
  } else if( is_track( filename )) {
    rec->rc.pszIcon = strdup( filename + 6 );
  } else {
    sfnameext( buffer, filename );
    rec->rc.pszIcon = strdup( buffer );
  }

  /* Set ID3 tag */
  pl_set_tag( rec, &tag, title );

  insert.cb                = sizeof(RECORDINSERT);
  insert.pRecordOrder      = (PRECORDCORE) pos;
  insert.pRecordParent     = NULL;
  insert.fInvalidateRecord = FALSE;
  insert.zOrder            = CMA_TOP;
  insert.cRecordsInsert    = 1;

  WinSendMsg( container, CM_INSERTRECORD,
              MPFROMP( rec ), MPFROMP( &insert ));

  pl_refresh_record( rec );
  amp_invalidate( UPD_FILEINFO | UPD_DELAYED );
  return rec;
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

  rec = pl_create_record( filename, (PLRECORD*)CMA_END, title );

  if( rec ) {
    if( options & PL_ADD_SELECT ) {
      pl_select( rec );
      pl_scroll_to_record( rec );
    }

    if( options & PL_ADD_LOAD   ) {
      amp_playmode = AMP_PLAYLIST;
      amp_pl_load_record( rec );
    }
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
  if( *findpath && findpath[strlen(findpath)-1] != '\\' ) {
    strcat( findpath, "\\" );
  }

  strcpy( findspec, findpath );
  strcat( findspec, "*" );

  findrc = findfirst( &hdir, findspec, FIND_ALL, &findbuff );

  while( findrc == 0 && !pl_peek_queue( PL_CLEAR, PL_TERMINATE ))
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
      char module_name[_MAX_FNAME];
      DECODER_INFO info;

      if( dec_fileinfo( fullname, &info, module_name ) == 0 ) {
        pl_create_record( fullname, (PLRECORD*)CMA_END, NULL );
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

/* Adds an specified request to a playlist queue. */
static BOOL
pl_write_queue( ULONG request, ULONG priority,
                const char* filename, const char* title, int options )
{
  PLDATA* data = (PLDATA*)malloc( sizeof( PLDATA ));
  APIRET  rc   = 0;

  if( !data ) {
    amp_player_error( "Not enough memory" );
    return FALSE;
  }

  data->filename = filename ? strdup( filename ) : NULL;
  data->title    = title    ? strdup( title )    : NULL;
  data->options  = options;

  rc = DosWriteQueue( hqueue, request,
                      sizeof( PLDATA ), data, priority );

  return rc == NO_ERROR;
}

/* Purges a playlist queue of all its requests. */
static void
pl_purge_queue( void )
{
  REQUESTDATA request;
  ULONG       dlen;
  PLDATA*     data;
  BYTE        priority;
  ULONG       entries;

  while( DosQueryQueue( hqueue, &entries ) == NO_ERROR && entries )
  {
    if( DosReadQueue( hqueue, &request, &dlen, (PPVOID)&data, 0,
                      DCWW_WAIT, &priority, NULLHANDLE ) != NO_ERROR ) {
      break;
    }

    free( data->filename );
    free( data->title );
    free( data );
  }
}

/* Examines a playlist queue request without removing it from the queue. */
static BOOL
pl_peek_queue( ULONG first, ULONG last )
{
  REQUESTDATA request;
  ULONG       dlen;
  PLDATA*     data;
  BYTE        priority;
  ULONG       entries = 0;
  ULONG       element = 0;

  if( DosQueryQueue( hqueue, &entries ) == NO_ERROR && entries &&
      DosPeekQueue ( hqueue, &request, &dlen, (PPVOID)&data, &element,
                     DCWW_WAIT, &priority, NULLHANDLE ) == NO_ERROR )
  {
    return request.ulData >= first && request.ulData <= last;
  }

  return FALSE;
}

/* Dispatches the playlist management requests. */
static void APIENTRY
pl_broker( HWND hwnd )
{
  PPIB ppib;
  char qname[_MAX_PATH];

  REQUESTDATA request;
  ULONG       dlen;
  PLDATA*     data;
  BYTE        priority;
  APIRET      rc;
  ULONG       entries;

  container = hwnd;
  hab       = WinInitialize( 0 );
  hmq       = WinCreateMsgQueue( hab, 0 );

  DosGetInfoBlocks( NULL, &ppib );
  sprintf( qname, "\\QUEUES\\PM123\\PL%06X.QUE", ppib->pib_ulpid );
  rc = DosCreateQueue ( &hqueue, QUE_FIFO | QUE_PRIORITY | QUE_NOCONVERT_ADDRESS, qname );

  DosPostEventSem( init_done );

  if( rc != NO_ERROR ) {
    amp_player_error( "Unable create queue %s", qname );
    return;
  }

  while( rc == NO_ERROR )
  {
    rc = DosReadQueue( hqueue, &request, &dlen, (PPVOID)&data, 0,
                                DCWW_WAIT, &priority, NULLHANDLE );

    switch( request.ulData ) {
      case PL_CLEAR:
        pl_remove_all();
        if( data->options & PL_CLR_NEW ) {
          *current_playlist = 0;
          pl_display_status();
        }
        break;

      case PL_ADD_FILE:
        pl_broker_add_file( data->filename, data->title, data->options );
        break;

      case PL_ADD_DIRECTORY:
        pl_broker_add_directory( data->filename, data->options );
        pl_completed();
        break;

      case PL_COMPLETED:
        if( cfg.autouse || amp_playmode == AMP_PLAYLIST ) {
          amp_pl_use();
        }
        break;
    }

    free( data->filename );
    free( data->title );
    free( data );

    rc = DosQueryQueue( hqueue, &entries );

    if( !entries && is_busy ) {
      is_busy = !is_busy;
      pl_display_status();
    }
  }

  DosCloseQueue( hqueue );
  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );

  hmq       = NULLHANDLE;
  hab       = NULLHANDLE;
  container = NULLHANDLE;
  hqueue    = NULLHANDLE;
}

/* Initializes the playlist service thread. */
BOOL
pl_init_thread( HWND container )
{
  if( DosCreateEventSem( NULL, &init_done, 0, FALSE ) != NO_ERROR ) {
    amp_error( container, "Unable create the event semaphore." );
    return FALSE;
  }

  if( DosCreateThread( &tid, pl_broker, container, CREATE_READY, 204800 ) != NO_ERROR ) {
    amp_error( container, "Unable create the playlist service thread." );
    return FALSE;
  }

  DosWaitEventSem ( init_done, SEM_INDEFINITE_WAIT );
  DosCloseEventSem( init_done );
  return TRUE;
}

/* Terminates the playlist service thread. */
BOOL
pl_term_thread( void )
{
  return pl_write_queue( PL_TERMINATE,
                         PR_TERMINATE, NULL, NULL, 0 );
}

/* Sends request about clearing of the playlist. */
BOOL
pl_clear( int options )
{
  pl_purge_queue();
  return pl_write_queue( PL_CLEAR,
                         PR_CLEAR, NULL, NULL, options );
}

/* Sends request about addition of the whole directory to the playlist. */
BOOL
pl_add_directory( const char* path, int options )
{
  return pl_write_queue( PL_ADD_DIRECTORY,
                         PR_ADD_DIRECTORY, path, NULL, options );
}

/* Sends request about addition of the file to the playlist. */
BOOL
pl_add_file( const char* filename, const char* title, int options )
{
  return pl_write_queue( PL_ADD_FILE,
                         PR_ADD_FILE, filename, title, options );
}

/* Notifies on completion of the playlist */
BOOL
pl_completed( void )
{
  return pl_write_queue( PL_COMPLETED,
                         PR_COMPLETED, NULL, NULL, 0 );
}

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

/* Marks the specified playlist record as currently played. */
void
pl_mark_as_play( PLRECORD* rec, BOOL play )
{
  if( !rec ) {
    return;
  }

  if( play ) {
    rec->rc.hptrIcon = mp3play;

    if( cfg.selectplayed ) {
      pl_select( rec );
    }

    pl_scroll_to_record( rec );
  } else {
    rec->rc.hptrIcon = mp3;
  }

  if( rec->play != play ) {
    rec->play = play;
    pl_refresh_record( rec );
  }
}

/* Marks the specified playlist record as already played. */
void
pl_mark_as_played( PLRECORD* rec, BOOL played )
{
  if( rec ) {
    rec->played = played;
  }
}

/* Removes "already played" marks from all playlist records. */
void
pl_clean_shuffle( void )
{
  PLRECORD* rec;
  for( rec = pl_first_record(); rec; rec = pl_next_record( rec )) {
    pl_mark_as_played( rec, FALSE );
  }
}

/* Returns true if the specified file is a playlist file. */
BOOL
is_playlist( const char *filename )
{
 char ext[_MAX_EXT];

 sext( ext, filename );
 return ( stricmp( ext, ".lst" ) == 0 ||
          stricmp( ext, ".mpl" ) == 0 ||
          stricmp( ext, ".pls" ) == 0 ||
          stricmp( ext, ".m3u" ) == 0 );
}

/* Loads the PM123 native playlist file. */
static BOOL
pl_load_playlist( const char *filename, int options )
{
  char basepath[_MAX_PATH];
  char fullname[_MAX_PATH];
  char file    [_MAX_PATH];

  FILE* playlist = fopen( filename, "r" );

  if( !playlist ) {
    amp_error( container, "Unable open playlist file:\n%s\n%s", filename, clib_strerror(errno));
    return FALSE;
  }

  sdrivedir( basepath, filename );

  if( options & PL_LOAD_CLEAR ) {
    pl_clear( 0 );
  }

  while( fgets( file, sizeof(file), playlist ))
  {
    blank_strip( file );

    if( *file != 0 && *file != '#' && *file != '>' && *file != '<' )
    {
      if( is_file( file ) && rel2abs( basepath, file, fullname, sizeof(fullname))) {
        strcpy( file, fullname );
      }
      pl_add_file( file, NULL, 0 );
    }
  }
  fclose( playlist );
  return TRUE;
}

/* Loads the M3U playlist file. */
static BOOL
pl_load_m3u_list( const char *filename, int options )
{
  char file[_MAX_PATH];
  int  playlist;

  HTTP_INFO http_info;

  if( !is_http( filename )) {
    return pl_load_playlist( filename, options );
  }

  playlist = http_open( filename, &http_info );

  if( !playlist ) {
    return FALSE;
  }

  if( options & PL_LOAD_CLEAR ) {
    pl_clear( 0 );
  }

  while( readline( file, sizeof(file), playlist ))
  {
    if( strchr( file, '\n')) {
       *strchr( file, '\n') = 0;
    } else if( strchr( file, '\r' )) {
       *strchr( file, '\r') = 0;
    }
    blank_strip(file);

    if( *file != 0 && *file != '#' && *file != '>' && *file != '<' ) {
      pl_add_file( file, NULL, 0 );
    }
  }

  http_close( playlist );
  return TRUE;
}

/* Loads the WarpAMP playlist file. */
static BOOL
pl_load_mpl_list( const char *filename, int options )
{
  char  basepath[_MAX_PATH];
  char  fullname[_MAX_PATH];
  char  file    [_MAX_PATH];
  char* eq_pos;

  FILE* playlist = fopen( filename, "r" );

  if( !playlist ) {
    amp_error( container, "Unable open playlist file:\n%s\n%s", filename, clib_strerror(errno));
    return FALSE;
  }

  sdrivedir( basepath, filename );

  if( options & PL_LOAD_CLEAR ) {
    pl_clear( 0 );
  }

  while( fgets( file, sizeof(file), playlist ))
  {
    blank_strip( file );

    if( *file != 0 && *file != '#' && *file != '[' && *file != '>' && *file != '<' )
    {
      eq_pos = strchr( file, '=' );

      if( eq_pos && strnicmp( file, "File", 4 ) == 0 )
      {
        strcpy( file, eq_pos + 1 );
        if( is_file( file ) && rel2abs( basepath, file, fullname, sizeof(fullname))) {
          strcpy( file, fullname );
        }
        pl_add_file( file, NULL, 0 );
      }
    }
  }
  fclose( playlist );
  return TRUE;
}

/* Loads the WinAMP playlist file. */
static BOOL
pl_load_pls_list( const char *filename, int options )
{
  char  basepath[_MAX_PATH];
  char  fullname[_MAX_PATH];
  char  file    [_MAX_PATH] = "";
  char  title   [_MAX_PATH] = "";
  char  line    [_MAX_PATH];
  int   last_idx = -1;
  BOOL  have_title = FALSE;
  char* eq_pos;

  FILE* playlist = fopen( filename, "r" );

  if( !playlist ) {
    amp_error( container, "Unable open playlist file:\n%s\n%s", filename, clib_strerror(errno));
    return FALSE;
  }

  sdrivedir( basepath, filename );

  if( options & PL_LOAD_CLEAR ) {
    pl_clear( 0 );
  }

  while( fgets( line, sizeof(line), playlist ))
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

          if( is_file( file ) && rel2abs( basepath, file, fullname, sizeof(fullname))) {
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

  fclose( playlist );
  return TRUE;
}

/* Loads the specified playlist file. */
BOOL pl_load( const char *filename, int options )
{
  BOOL rc = FALSE;
  char ext[_MAX_EXT];
  int  i;

  sext( ext, filename );

  if( stricmp( ext, ".lst" ) == 0 ) {
    rc = pl_load_playlist( filename, options );
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
  }

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
    amp_error( container, "Unable open playlist file:\n%s\n%s", filename, clib_strerror(errno));
    return FALSE;
  }

  if( !(options & PL_SAVE_M3U )) {
    fprintf( playlist, "#\n"
                       "# Playlist created with %s\n"
                       "# Do not modify!\n"
                       "# Lines starting with '>' are used by Playlist Manager.\n"
                       "#\n", VERSION);
  }

  sdrivedir( base, filename );
  for( rec = pl_first_record(); rec; rec = pl_next_record( rec ))
  {
    if( !(options & PL_SAVE_M3U )) {
      fprintf( playlist, "# %s, %s, %s\n", rec->length, rec->time, rec->info );
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
      fprintf( playlist, ">%u,%u,%u,%u,%lu\n", rec->bitrate,
                         rec->freq, rec->mode, rec->size, rec->secs );
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
  PLRECORD* rec;

  FILE* playlist = fopen( filename, "w" );

  if( !playlist ) {
    amp_error( container, "Unable create status file:\n%s\n%s", filename, clib_strerror(errno));
    return FALSE;
  }

  fprintf( playlist, "#\n"
                     "# Player state file created with %s\n"
                     "# Do not modify! This file is compatible with the playlist format,\n"
                     "# but information written in this file is different.\n"
                     "#\n", VERSION);

  for( rec = pl_first_record(); rec; rec = pl_next_record( rec ))
  {
    fprintf( playlist, ">%u,%u\n",
             rec->rc.flRecordAttr & CRA_CURSORED ? 1 : 0, rec == currentf );
    fprintf( playlist, "%s\n" , rec->full );
  }

  if( amp_playmode == AMP_SINGLE && *current_filename && is_file(current_filename)) {
    fprintf( playlist, "<%s\n", current_filename );
  }

  fprintf( playlist, "# End of playlist\n" );
  fclose ( playlist );
  return TRUE;
}

/* Loads the playlist and player status from specified file. */
BOOL
pl_load_bundle( const char *filename, int options )
{
  char  file[_MAX_PATH];
  BOOL  selected = FALSE;
  BOOL  loaded   = FALSE;
  FILE* playlist = fopen( filename, "r" );

  if( !playlist ) {
    amp_error( container, "Unable open status file:\n%s\n%s", filename, clib_strerror(errno));
    return FALSE;
  }

  pl_clear( 0 );

  while( fgets( file, sizeof(file), playlist ))
  {
    blank_strip( file );

    if( *file == '<' ) {
      struct stat fi;
      if( stat( file + 1, &fi ) == 0 ) {
        amp_load_singlefile( file + 1, AMP_LOAD_NOT_PLAY | AMP_LOAD_NOT_RECALL );
      }
    } else if( *file == '>' ) {
      sscanf( file, ">%u,%u\n", &selected, &loaded );
    } else if( *file != 0 && *file != '#' ) {
      pl_add_file( file, NULL,
                   PL_ADD_IF_EXIST | ( selected ? PL_ADD_SELECT : 0 )
                                   | ( loaded   ? PL_ADD_LOAD   : 0 ));
    }
  }
  fclose( playlist );
  return TRUE;
}

