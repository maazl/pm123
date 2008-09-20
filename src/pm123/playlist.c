/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
 * Copyright 2004-2007 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <utilfct.h>

#include "playlist.h"
#include "pm123.h"
#include "plugman.h"
#include "docking.h"
#include "iniman.h"
#include "pfreq.h"
#include "assertions.h"
#include "tags.h"

#define PL_ADD_FILE         0
#define PL_ADD_DIRECTORY    1
#define PL_COMPLETED        2
#define PL_SELECT_BY_INDEX  3
#define PL_RELOAD_BY_INDEX  4
#define PL_LOAD_PLAYLIST    5
#define PL_CLEAR            6
#define PL_TERMINATE        7

/* pl_m_sort controls */
#define PL_SORT_RAND        0x0001
#define PL_SORT_SIZE        0x0002
#define PL_SORT_TIME        0x0003
#define PL_SORT_FILE        0x0004
#define PL_SORT_SONG        0x0005
#define PL_SORT_TRACK       0x0006

/* Structure that contains information for records in
   the playlist container control. */

typedef struct _PLRECORD {

  RECORDCORE   rc;
  char*        songname;    /* Name of the song.              */
  char*        full;        /* Full path and file name.       */
  char*        size;        /* Size of the file.              */
  char*        time;        /* Play time of the file.         */
  char*        moreinfo;    /* Information about the song.    */
  int          played;      /* Is it already played file.     */
  BOOL         playing;     /* Is it currently playing file.  */
  BOOL         exist;       /* Is this file exist.            */
  DECODER_INFO info;        /* File info returned by decoder. */
  char         decoder[_MAX_MODULE_NAME];

} PLRECORD, *PPLRECORD;

typedef struct _PLDATA {

  char* filename;
  char* songname;
  int   index;
  int   options;

} PLDATA;

typedef struct _PLDROPINFO {

  int    index;     /* Index of inserted record. */
  HWND   hwndItem;  /* Window handle of the source of the drag operation. */
  ULONG  ulItemID;  /* Information used by the source to identify the
                       object being dragged. */
} PLDROPINFO;

#define INDEX_FIRST  0
#define INDEX_END   -1
#define INDEX_NONE  -2

static HWND   menu_playlist  = NULLHANDLE;
static HWND   menu_record    = NULLHANDLE;
static HWND   playlist       = NULLHANDLE;
static HWND   container      = NULLHANDLE;
static BOOL   is_busy        = FALSE;
static int    played         = 0;
static TID    broker_tid     = 0;
static PQUEUE broker_queue   = NULL;

static char current_playlist[_MAX_PATH];

#define PL_MSG_MARK_AS_PLAY     ( WM_USER + 1000 )
#define PL_MSG_PLAY_LEFT        ( WM_USER + 1001 )
#define PL_MSG_LOAD_FRST_RECORD ( WM_USER + 1002 )
#define PL_MSG_LOAD_NEXT_RECORD ( WM_USER + 1003 )
#define PL_MSG_LOAD_PREV_RECORD ( WM_USER + 1004 )
#define PL_MSG_LOAD_FILE_RECORD ( WM_USER + 1005 )
#define PL_MSG_REMOVE_RECORD    ( WM_USER + 1006 )
#define PL_MSG_REMOVE_ALL       ( WM_USER + 1007 )
#define PL_MSG_INSERT_RECORD    ( WM_USER + 1008 )
#define PL_MSG_LOADED_INDEX     ( WM_USER + 1009 )
#define PL_MSG_SELECT_INDEX     ( WM_USER + 1010 )
#define PL_MSG_LOADBY_INDEX     ( WM_USER + 1011 )
#define PL_MSG_INSERT_RECALL    ( WM_USER + 1012 )
#define PL_MSG_REFRESH_FILE     ( WM_USER + 1013 )
#define PL_MSG_REFRESH_STATUS   ( WM_USER + 1014 )
#define PL_MSG_CLEAN_SHUFFLE    ( WM_USER + 1015 )
#define PL_MSG_SAVE_PLAYLIST    ( WM_USER + 1016 )
#define PL_MSG_SAVE_BUNDLE      ( WM_USER + 1017 )
#define PL_MSG_REFRESH_SONGNAME ( WM_USER + 1018 )

static BOOL pl_load_any_list( const char* filename, int options );
static BOOL pl_m_save_list  ( const char* filename, int options );
static BOOL pl_m_save_bundle( const char* filename, int options );
static void pl_m_insert_to_recall_list( const char* filename );

/* The pointer to playlist record of the currently loaded file,
   the pointer is NULL if such record is not present. */
static PLRECORD* loaded_record;
/* The pointer to playlist record of the currently played file,
   the pointer is NULL if such record is not present. */
static PLRECORD* played_record;

/* WARNING!!! All functions returning a pointer to the
   playlist record, return a NULL if suitable record is not found. */

INLINE PLRECORD*
PL_RC( MRESULT rc )
{
  if((PLRECORD*)rc != (PLRECORD*)-1 ) {
    return (PLRECORD*)rc;
  } else {
    return NULL;
  }
}

/* Returns the pointer to the first playlist record. */
static PLRECORD*
pl_m_first_record( void )
{
  ASSERT_IS_MAIN_THREAD;
  return PL_RC( WinSendMsg( container, CM_QUERYRECORD, NULL,
                            MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER )));
}

/* Returns the pointer to the next playlist record of specified. */
static PLRECORD*
pl_m_next_record( PLRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;
  return PL_RC( WinSendMsg( container, CM_QUERYRECORD, MPFROMP(rec),
                            MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER )));
}

/* Returns the pointer to the previous playlist record of specified. */
static PLRECORD*
pl_m_prev_record( PLRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;
  return PL_RC( WinSendMsg( container, CM_QUERYRECORD, MPFROMP(rec),
                            MPFROM2SHORT( CMA_PREV, CMA_ITEMORDER )));
}

/* Returns the pointer to the first selected playlist record. */
static PLRECORD*
pl_m_first_selected( void )
{
  ASSERT_IS_MAIN_THREAD;
  return PL_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_SELECTED )));
}

/* Returns the pointer to the next selected playlist record of specified. */
static PLRECORD*
pl_m_next_selected( PLRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;
  return PL_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( rec ), MPFROMSHORT( CRA_SELECTED )));
}

/* Returns the pointer to the cursored playlist record. */
static PLRECORD*
pl_m_cursored( void )
{
  ASSERT_IS_MAIN_THREAD;
  return PL_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_CURSORED )));
}

/* Selects the specified record and deselects all others. */
static void
pl_m_select( PLRECORD* select )
{
  PLRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  if( select ) {
    for( rec = pl_m_first_selected(); rec; rec = pl_m_next_selected( rec )) {
      WinSendMsg( container, CM_SETRECORDEMPHASIS,
                  MPFROMP( rec ), MPFROM2SHORT( FALSE, CRA_SELECTED ));
    }
    WinSendMsg( container, CM_SETRECORDEMPHASIS,
                MPFROMP( select ), MPFROM2SHORT( TRUE , CRA_SELECTED | CRA_CURSORED ));
  }
}

/* Queries a random playlist record for playing. */
static PLRECORD*
pl_m_query_random_record( void )
{
  PLRECORD* rec;
  int pos = 0;

  ASSERT_IS_MAIN_THREAD;

  for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
    if( !rec->played && rec->exist ) {
      ++pos;
    }
  }

  if( pos )
  {
    pos = rand() % pos;
    for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
      if( !rec->played && rec->exist && !pos-- ) {
        break;
      }
    }
  }
  return rec;
}

/* Queries a first playlist record for playing. */
static PLRECORD*
pl_m_query_first_record( void )
{
  PLRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  if( cfg.shf ) {
    return pl_m_query_random_record();
  } else {
    for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
      if( rec->exist ) {
        break;
      }
    }
    return rec;
  }
}

/* Queries a next playlist record for playing. */
static PLRECORD*
pl_m_query_next_record( void )
{
  PLRECORD* found = NULL;
  ASSERT_IS_MAIN_THREAD;

  if( loaded_record ) {
    if( cfg.shf ) {
      if( loaded_record->played == played ) {
        found =  pl_m_query_random_record();
      } else {
        PLRECORD* rec = pl_m_first_record();

        while( rec ) {
          // Search for record which it was played before current record.
          if( rec->played > loaded_record->played ) {
            if( !found || rec->played < found->played ) {
              found = rec;
              // Make search little bit faster.
              if( found->played - loaded_record->played == 1 ) {
                break;
              }
            }
          }
          rec = pl_m_next_record( rec );
        }
        if( !found ) {
          found = pl_m_query_random_record();
        }
      }
    } else {
      found = pl_m_next_record( loaded_record );
      while( found && !found->exist ) {
        found = pl_m_next_record( found );
      }
    }
  }

  return found;
}

/* Queries a previous playlist record for playing. */
static PLRECORD*
pl_m_query_prev_record( void )
{
  PLRECORD* found = NULL;
  ASSERT_IS_MAIN_THREAD;

  if( loaded_record ) {
    if( cfg.shf ) {
      PLRECORD* rec = pl_m_first_record();
      while( rec ) {
        // Search for record which it was played before current record.
        if( rec->played && rec->played < loaded_record->played ) {
          if( !found || rec->played > found->played ) {
            found = rec;
            // Make search little bit faster.
            if( loaded_record->played - found->played == 1 ) {
              break;
            }
          }
        }
        rec = pl_m_next_record( rec );
      }
    } else {
      found = pl_m_prev_record( loaded_record );
      while( found && !found->exist ) {
        found = pl_m_prev_record( found );
      }
    }
  }

  return found;
}

/* Queries a playlist record of the specified file for playing. */
static PLRECORD*
pl_m_query_file_record( const char* filename )
{
  PLRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  if( loaded_record ) {
    if( strcmp( loaded_record->full, filename ) == 0 ) {
      return loaded_record;
    }
  }

  for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
    if( strcmp( rec->full, filename ) == 0 ) {
      break;
    }
  }

  return rec;
}

/* Invalidates the specified playlist record. */
static void
pl_m_invalidate_record( PLRECORD* rec, USHORT flags )
{
  WinSendMsg( container, CM_INVALIDATERECORD,
              MPFROMP( &rec ), MPFROM2SHORT( 1, flags ));
}

/* Fills record by the information provided by the decoder. */
static void
pl_m_fill_record( PLRECORD* rec, const DECODER_INFO* info )
{
  char buffer[64];

  free( rec->songname );
  free( rec->moreinfo );
  free( rec->size );
  free( rec->time );

  if( info ) {
    rec->info = *info;
  } else {
    memset( &rec->info, 0, sizeof( rec->info ));
  }

  sprintf( buffer, "%u kB", (unsigned int)rec->info.filesize / 1024 );
  rec->size = strdup( buffer );
  sprintf( buffer, "%02u:%02u", rec->info.songlength / 60000, rec->info.songlength / 1000 % 60 );
  rec->time = strdup( buffer );

  // Songname
  if( *rec->info.title )
  {
    rec->songname = malloc( strlen( rec->info.artist ) + 3 +
                            strlen( rec->info.title  ) + 1 );
    if( rec->songname ) {
      strcpy( rec->songname, rec->info.artist );
      if( *rec->info.title && *rec->info.artist ) {
        strcat( rec->songname, " - " );
      }
      strcat( rec->songname, rec->info.title );
    }
  // If the song name is undefined then use the file name instead.
  } else {
    char filename[_MAX_PATH];
    amp_title_from_filename( filename, rec->rc.pszIcon, sizeof( filename ));

    rec->songname = malloc( strlen( rec->info.artist ) + 3 +
                            strlen( filename         ) + 1 );
    if( rec->songname ) {
      strcpy( rec->songname, rec->info.artist );
      if( *rec->info.artist && *filename ) {
        strcat( rec->songname, " - " );
      }
      strcat( rec->songname, filename );
    }
  }

  // Information
  rec->moreinfo = malloc( strlen( rec->info.album       ) + 1 +
                          strlen( rec->info.year        ) + 2 +
                          strlen( rec->info.genre       ) + 2 +
                          strlen( rec->info.track       ) + 4 +
                          strlen( rec->info.tech_info   ) +
                          strlen( rec->info.comment     ) + 12 );
  if( rec->moreinfo ) {
    strcpy( rec->moreinfo, rec->info.album );

    if( *rec->info.album ) {
      if( *rec->info.year  ) {
        strcat( rec->moreinfo, " "   );
      } else {
        strcat( rec->moreinfo, ", "  );
      }
    }
    if( *rec->info.year  ) {
      strcat( rec->moreinfo, rec->info.year );
      strcat( rec->moreinfo, ", " );
    }
    if( *rec->info.genre ) {
      strcat( rec->moreinfo, rec->info.genre );
      strcat( rec->moreinfo, ", " );
    }
    if( *rec->info.track ) {
      strcat( rec->moreinfo, "T."  );
      strcat( rec->moreinfo, rec->info.track );
      strcat( rec->moreinfo, ", " );
    }

    strcat( rec->moreinfo, rec->info.tech_info );

    if( *rec->info.comment ) {
      strcat( rec->moreinfo, ", comment: " );
      strcat( rec->moreinfo, rec->info.comment );
    }
  }

  control_strip( rec->songname );
  control_strip( rec->moreinfo );
}

/* Returns an icon suitable to the specified record. */
static HPOINTER
pl_m_proper_icon( PLRECORD* rec )
{
  if( rec->playing ) {
    return mp3play;
  } else if( !rec->exist ) {
    return mp3gray;
  } else {
    return mp3;
  }
}

/* Prepares the specified playlist record to play. */
static BOOL
pl_m_load_record( PLRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;

  if( rec ) {
    if( !*rec->decoder )
    {
      DECODER_INFO info;

      rec->exist = ( dec_fileinfo( rec->full, &info, rec->decoder ) == PLUGIN_OK );
      rec->rc.hptrIcon = pl_m_proper_icon( rec );
      pl_m_fill_record( rec, &info );
      pl_m_invalidate_record( rec, CMA_TEXTCHANGED );
    }

    strlcpy( current_filename, rec->full,    sizeof( current_filename ));
    strlcpy( current_decoder,  rec->decoder, sizeof( current_decoder  ));

    current_info  = rec->info;
    loaded_record = rec;

    amp_invalidate( UPD_FILENAME | UPD_FILEINFO | UPD_TIMERS );
    return TRUE;
  }

  return FALSE;
}

static SHORT EXPENTRY
pl_m_compare_rand( PLRECORD* p1, PLRECORD* p2, PVOID pStorage ) {
  return ( rand() % 2 ) - 1;
}

static SHORT EXPENTRY
pl_m_compare_size( PLRECORD* p1, PLRECORD* p2, PVOID pStorage )
{
  if( p1->info.filesize < p2->info.filesize ) return -1;
  if( p1->info.filesize > p2->info.filesize ) return  1;
  return 0;
}

static SHORT EXPENTRY
pl_m_compare_time( PLRECORD* p1, PLRECORD* p2, PVOID pStorage )
{
  if( p1->info.songlength < p2->info.songlength ) return -1;
  if( p1->info.songlength > p2->info.songlength ) return  1;
  return 0;
}

static SHORT EXPENTRY
pl_m_compare_name( PLRECORD* p1, PLRECORD* p2, PVOID pStorage )
{
  char s1[_MAX_PATH], s2[_MAX_PATH];

  sfname( s1, p1->full, sizeof( s1 ));
  sfname( s2, p2->full, sizeof( s2 ));

  return stricmp( s1, s2 );
}

static SHORT EXPENTRY
pl_m_compare_song( PLRECORD* p1, PLRECORD* p2, PVOID pStorage ) {
  return stricmp( p1->songname, p2->songname );
}

static SHORT EXPENTRY
pl_m_compare_track( PLRECORD* p1, PLRECORD* p2, PVOID pStorage )
{
  SHORT rc;

  if(( rc = stricmp( p1->info.artist, p2->info.artist )) == 0 ) {
    if(( rc = stricmp( p1->info.album, p2->info.album )) == 0 ) {
      return stricmp( p1->info.track, p2->info.track );
    }
  }
  return rc;
}

/* Sorts the playlist records. */
static void
pl_m_sort( int control )
{
  ASSERT_IS_MAIN_THREAD;

  switch( control ) {
    case PL_SORT_RAND:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_m_compare_rand  ), 0 );
      break;
    case PL_SORT_SIZE:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_m_compare_size  ), 0 );
      break;
    case PL_SORT_TIME:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_m_compare_time  ), 0 );
      break;
    case PL_SORT_FILE:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_m_compare_name  ), 0 );
      break;
    case PL_SORT_SONG:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_m_compare_song  ), 0 );
      break;
    case PL_SORT_TRACK:
      WinSendMsg( container, CM_SORTRECORD, MPFROMP( pl_m_compare_track ), 0 );
      break;
  }
  amp_invalidate( UPD_WINDOW );
}

/* Scrolls the playlist so that the specified record became visible. */
static BOOL
pl_m_scroll_to_record( PLRECORD* rec )
{
  QUERYRECORDRECT prcItem;
  RECTL rclRecord;
  RECTL rclContainer;

  ASSERT_IS_MAIN_THREAD;

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

/* Marks the currently loaded playlist record as currently played or
   as currently stopped. */
static void
pl_m_mark_as_play( BOOL play )
{
  ASSERT_IS_MAIN_THREAD;

  if( play ) {
    if( played_record ) {
      if( played_record != loaded_record ) {
        pl_m_mark_as_play( FALSE );
      }
    }
    if( loaded_record ) {
      played_record = loaded_record;
      loaded_record->playing = play;
      loaded_record->rc.hptrIcon = pl_m_proper_icon( loaded_record );

      if( cfg.selectplayed ) {
        pl_m_select( loaded_record );
      }
      if( cfg.shf && !loaded_record->played ) {
        loaded_record->played = ++played;
      }

      pl_m_scroll_to_record ( loaded_record );
      pl_m_invalidate_record( loaded_record, CMA_NOREPOSITION );
    }
  } else {
    if( played_record ) {
      played_record->playing = play;
      played_record->rc.hptrIcon = pl_m_proper_icon( played_record );
      pl_m_invalidate_record( played_record, CMA_NOREPOSITION );
      played_record = NULL;
    }
  }
}

/* Removes "already played" marks from all playlist records. */
static void
pl_m_clean_shuffle( void )
{
  PLRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
    rec->played = 0;
  }
  played = 0;
}

/* Prepares the first playlist record to playing. */
BOOL
pl_load_first_record( void ) {
  return BOOLFROMMR( WinSendMsg( playlist, PL_MSG_LOAD_FRST_RECORD, 0, 0 ));
}

/* Prepares the next playlist record to playing. */
BOOL
pl_load_next_record( void ) {
  return BOOLFROMMR( WinSendMsg( playlist, PL_MSG_LOAD_NEXT_RECORD, 0, 0 ));
}

/* Prepares the previous playlist record to playing. */
BOOL
pl_load_prev_record( void ) {
  return BOOLFROMMR( WinSendMsg( playlist, PL_MSG_LOAD_PREV_RECORD, 0, 0 ));
}

/* Prepares the first playlist record to playing. */
BOOL
pl_load_file_record( const char* filename ) {
  return BOOLFROMMR( WinSendMsg( playlist, PL_MSG_LOAD_FILE_RECORD, MPFROMP( filename ), 0 ));
}

/* Marks the currently loaded playlist record as currently played. */
void
pl_mark_as_play( void ) {
  WinSendMsg( playlist, PL_MSG_MARK_AS_PLAY, MPFROMLONG( TRUE  ), 0 );
}

/* Marks the currently loaded playlist record as currently stopped. */
void
pl_mark_as_stop( void ) {
  WinSendMsg( playlist, PL_MSG_MARK_AS_PLAY, MPFROMLONG( FALSE ), 0 );
}

/* Removes "already played" marks from all playlist records. */
void
pl_clean_shuffle( void ) {
  WinSendMsg( playlist, PL_MSG_CLEAN_SHUFFLE, 0, 0 );
}

/* Returns a ordinal number of the specified record. */
static ULONG
pl_m_index_of_record( PLRECORD* find )
{
  PLRECORD* rec;
  ULONG     pos = 1;

  ASSERT_IS_MAIN_THREAD;

  if( find == (PLRECORD*)CMA_END ) {
    return INDEX_END;
  } else if( find == (PLRECORD*)CMA_FIRST ) {
    return INDEX_FIRST;
  } else {
    for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec ), ++pos ) {
      if( rec == find ) {
        return pos;
      }
    }
  }
  return INDEX_NONE;
}

/* Returns a record specified via ordinal number. */
static PLRECORD*
pl_m_record_by_index( ULONG pos )
{
  PLRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  if( pos > 0 ) {
    for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
      if( !--pos ) {
        break;
      }
    }
    return rec;
  } else {
    return NULL;
  }
}

/* Inserts one record into a container control. */
static BOOL
pl_m_insert_record( int index, PLRECORD* rec )
{
  RECORDINSERT ins;
  PLRECORD*    pos;

  ASSERT_IS_MAIN_THREAD;

  if( index > 0 ) {
    if(!( pos = pl_m_record_by_index( index ))) {
      pos = (PLRECORD*)CMA_END;
    }
  } else if( index == INDEX_FIRST ) {
    pos = (PLRECORD*)CMA_FIRST;
  } else {
    pos = (PLRECORD*)CMA_END;
  }

  ins.cb                = sizeof(RECORDINSERT);
  ins.pRecordOrder      = (PRECORDCORE)pos;
  ins.pRecordParent     = NULL;
  ins.fInvalidateRecord = FALSE;
  ins.zOrder            = CMA_TOP;
  ins.cRecordsInsert    = 1;

  if( WinSendMsg( container, CM_INSERTRECORD, MPFROMP( rec ), MPFROMP( &ins )) != 0 ) {
    pl_m_invalidate_record( rec, CMA_TEXTCHANGED );
    return TRUE;
  }

  return FALSE;
}

/* Frees the data contained in playlist record. */
static void
pl_m_free_record( PLRECORD* rec )
{
  free( rec->rc.pszIcon );
  free( rec->songname );
  free( rec->size );
  free( rec->time );
  free( rec->moreinfo );
  free( rec->full );
}

/* Creates the playlist record for specified file. */
static PLRECORD*
pl_m_create_record( const char* filename, int index,
                    const DECODER_INFO* info, const char* decoder )
{
  PLRECORD* rec;
  char name[_MAX_PATH ];

  if( !filename || !*filename ) {
    return NULL;
  }

  // Allocate a new record.
  rec = (PLRECORD*)WinSendMsg( container, CM_ALLOCRECORD,
                               MPFROMLONG( sizeof( PLRECORD ) - sizeof( RECORDCORE )),
                               MPFROMLONG( 1 ));
  if( rec ) {
    rec->rc.cb           = sizeof( RECORDCORE );
    rec->rc.flRecordAttr = CRA_DROPONABLE;
    rec->played          = 0;
    rec->exist           = info != NULL;
    rec->rc.hptrIcon     = pl_m_proper_icon( rec );
    rec->full            = strdup( filename );
    rec->size            = NULL;
    rec->time            = NULL;
    rec->songname        = NULL;
    rec->moreinfo        = NULL;

    if( is_url( filename )) {
      sdecode( name, sfnameext( name, filename, sizeof( name )), sizeof( name ));
    } else {
      sfnameext( name, filename, sizeof( name ));
    }

    rec->rc.pszIcon = strdup( name );
    pl_m_fill_record( rec, info );

    if( decoder ) {
      strlcpy( rec->decoder, decoder, sizeof( rec->decoder ));
    }

    if( WinSendMsg( playlist, PL_MSG_INSERT_RECORD, MPFROMLONG( index ), rec )) {
      amp_invalidate( UPD_FILEINFO | UPD_TIMERS | UPD_DELAYED );
      return rec;
    } else {
      pl_m_free_record( rec );
      WinSendMsg( container, CM_FREERECORD, MPFROMP( &rec ), MPFROMSHORT( 1 ));
    }
  }
  return NULL;
}

/* Copies the playlist record to specified position. */
static PLRECORD*
pl_m_copy_record( PLRECORD* rec, PLRECORD* pos )
{
  RECORDINSERT insert;
  PLRECORD*    copy;

  ASSERT_IS_MAIN_THREAD;
  copy = (PLRECORD*)WinSendMsg( container, CM_ALLOCRECORD,
                                MPFROMLONG( sizeof( PLRECORD ) - sizeof( RECORDCORE )),
                                MPFROMLONG( 1 ));
  if( !copy ) {
    return NULL;
  }

  copy->rc.cb           = sizeof(RECORDCORE);
  copy->rc.flRecordAttr = CRA_DROPONABLE;
  copy->full            = strdup( rec->full );
  copy->size            = strdup( rec->size );
  copy->songname        = strdup( rec->songname );
  copy->moreinfo        = strdup( rec->moreinfo );
  copy->time            = strdup( rec->time );
  copy->rc.pszIcon      = strdup( rec->rc.pszIcon );
  copy->info            = rec->info;
  copy->played          = 0;
  copy->playing         = FALSE;
  copy->exist           = rec->exist;
  copy->rc.hptrIcon     = pl_m_proper_icon( copy );

  strcpy( copy->decoder, rec->decoder );

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
pl_m_move_record( PLRECORD* rec, PLRECORD* pos )
{
  RECORDINSERT insert;
  ASSERT_IS_MAIN_THREAD;

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

/* Removes the specified playlist records. */
static BOOL
pl_m_remove_records( PLRECORD** array, USHORT count, int options )
{
  PLRECORD* load = loaded_record;
  PLRECORD* rec;

  int array_size = 0;
  int i;

  ASSERT_IS_MAIN_THREAD;

  if( options & PL_REMOVE_SELECTED )
  {
    array = NULL;
    count = 0;

    for( rec = pl_m_first_selected(); rec; rec = pl_m_next_selected( rec )) {
      if( count == array_size ) {
        array_size += 20;
        array = realloc( array, array_size * sizeof( PLRECORD* ));
      }
      if( !array ) {
        break;
      }
      array[count++] = rec;
    }
  } else if( options & PL_REMOVE_LOADED ) {
    if( loaded_record ) {
      if(( array = malloc( sizeof( PLRECORD* ))) != NULL ) {
        array[0] = loaded_record;
        count    = 1;
      }
    }
  }

  if( !count || !array ) {
    return FALSE;
  }

  for( i = count - 1; i >= 0; i-- ) {
    if( array[i] == played_record ) {
      played_record = NULL;
    }
    if( array[i] == load && amp_playmode == AMP_PLAYLIST ) {
      while(( load = pl_m_prev_record( load )) != NULL ) {
        if( load->exist ) {
          break;
        }
      }
    }
  }

  if( WinSendMsg( container, CM_REMOVERECORD, MPFROMP( array ),
                  MPFROM2SHORT( count, CMA_INVALIDATE )) == MRFROMLONG( -1 ))
  {
    return FALSE;
  }

  if( !load ) {
    for( load = pl_m_first_record(); load; load = pl_m_next_record( load )) {
      if( load->exist ) {
        break;
      }
    }
  }

  if( amp_playmode == AMP_PLAYLIST ) {
    if( loaded_record != load && decoder_playing()) {
      amp_stop();
    }
    if( load ) {
      pl_m_load_record( load );
    } else {
      amp_reset();
    }
  }

  for( i = 0; i < count; i++ ) {
    if( options & PL_DELETE_FILES ) {
      if( is_file( array[i]->full ) && unlink( array[i]->full ) != 0 ) {
        amp_error( playlist, "Unable delete file:\n%s\n%s",
                              array[i]->full, strerror( errno ));
      }
    }
    pl_m_free_record( array[i] );
  }

  WinSendMsg( container, CM_FREERECORD,
              MPFROMP( array ), MPFROMSHORT( count ));

  pl_m_select( pl_m_cursored());

  if( options & PL_REMOVE_SELECTED ||
      options & PL_REMOVE_LOADED    )
  {
    free( array );
  }
  if( amp_playmode == AMP_PLAYLIST ) {
    amp_invalidate( UPD_FILEINFO | UPD_TIMERS | UPD_DELAYED );
  }

  return TRUE;
}

/* Removes all playlist records. */
static void
pl_m_remove_all( void )
{
  PLRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  if( amp_playmode == AMP_PLAYLIST ) {
    amp_reset();
  }

  loaded_record = NULL;
  played_record = NULL;

  for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
    pl_m_free_record( rec );
  }

  WinSendMsg( container, CM_REMOVERECORD,
              MPFROMP( NULL ), MPFROM2SHORT( 0, CMA_FREE | CMA_INVALIDATE ));
}

/* Removes the specified playlist records. */
void
pl_remove_records( int options ) {
  WinSendMsg( playlist, PL_MSG_REMOVE_RECORD, MPFROMLONG( options ), 0 );
}

/* Refreshes the playlist records of the specified file. */
static void
pl_m_refresh_file( const char* filename, const DECODER_INFO* info )
{
  PLRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
    if( stricmp( rec->full, filename ) == 0 ) {
      rec->exist = TRUE;
      rec->rc.hptrIcon = pl_m_proper_icon( rec );
      pl_m_fill_record( rec, info );
      pl_m_invalidate_record( rec, CMA_TEXTCHANGED );
    }
  }
}

/* Sets the songname for the specified file. */
static void
pl_m_refresh_songname( const char* filename, const char* songname )
{
  PLRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec )) {
    if( stricmp( rec->full, filename ) == 0 ) {
      free( rec->songname );
      rec->songname = strdup( songname );
      pl_m_invalidate_record( rec, CMA_TEXTCHANGED );
    }
  }
}

/* Refreshes the playlist records of the specified file. */
void
pl_refresh_file( const char* filename, const DECODER_INFO* info ) {
  WinSendMsg( playlist, PL_MSG_REFRESH_FILE,
                        MPFROMP( filename ), MPFROMP( info ));
}

/* Sets the songname for the specified file. */
void
pl_refresh_songname( const char* filename, const char* songname ) {
  WinSendMsg( playlist, PL_MSG_REFRESH_SONGNAME,
                        MPFROMP( filename ), MPFROMP( songname ));
}

/* Returns a ordinal number of the currently loaded record. */
ULONG
pl_loaded_index( void ) {
  return LONGFROMMR( WinSendMsg( playlist, PL_MSG_LOADED_INDEX, 0, 0 ));
}

/* Sets the title of the playlist window according to current
   playlist state. */
static void
pl_m_refresh_status( const char* playlist_name )
{
  char title[ _MAX_FNAME + 128 ];
  char file [ _MAX_FNAME ];

  ASSERT_IS_MAIN_THREAD;
  strcpy( title, "PM123 Playlist"  );

  if( playlist_name ) {
    strlcpy( current_playlist, playlist_name, sizeof( current_playlist ));
  }
  if( *current_playlist ) {
    if( is_url( current_playlist )) {
      sdecode( file, sfnameext( file, current_playlist, sizeof( file )), sizeof( file ));
    } else {
      sfnameext( file, current_playlist, sizeof( file ));
    }
    strlcat( title, " - ", sizeof( title ));
    strlcat( title, file,  sizeof( title ));
  }

  if( amp_playmode == AMP_PLAYLIST ) {
    strlcat( title, " - [USED]", sizeof( title ));
  }
  if( is_busy ) {
    strlcat( title, " - loading...", sizeof( title ));
  }

  WinSetWindowText( playlist, (PSZ)title );
}

/* Sets the title of the playlist window according to current
   playlist state. */
void
pl_refresh_status( void ) {
  WinSendMsg( playlist, PL_MSG_REFRESH_STATUS, NULL, 0 );
}

static PLDATA*
pl_m_create_request_data( const char* filename,
                          const char* songname, int index, int options )
{
  PLDATA* data = (PLDATA*)malloc( sizeof( PLDATA ));

  if( data ) {
    data->filename = filename ? strdup( filename ) : NULL;
    data->songname = songname ? strdup( songname ) : NULL;
    data->index    = index;
    data->options  = options;
  }

  return data;
}

static void
pl_m_delete_request_data( PLDATA* data )
{
  if( data ) {
    free( data->filename );
    free( data->songname );
    free( data );
  }
}

/* Purges a specified queue of all its requests. */
static void
pl_m_purge_queue( PQUEUE queue )
{
  ULONG  request;
  PVOID  data;

  while( !qu_empty( queue )) {
    qu_read( queue, &request, &data );
    pl_m_delete_request_data( data );
  }
}

/* Examines a queue request without removing it from the queue. */
static BOOL
pl_m_peek_queue( PQUEUE queue, ULONG first, ULONG last )
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
pl_broker_add_file( const char* filename,
                    const char* title, int index, int options )
{
  struct       stat fi;
  DECODER_INFO info;
  char         decoder[_MAX_MODULE_NAME] = "";

  if( options & PL_ADD_IF_EXIST && stat( filename, &fi ) != 0 ) {
    return;
  }

  if( !is_busy ) {
    is_busy = TRUE;
    pl_refresh_status();
  }

  if( dec_fileinfo( filename, &info, decoder ) == PLUGIN_OK ) {
    if( !*info.title && title ) {
      strlcpy( info.title, title, sizeof( info.title ));
    }
    pl_m_create_record( filename, index, &info, decoder );
  } else {
    pl_m_create_record( filename, index, NULL, NULL );
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
    is_busy = TRUE;
    pl_refresh_status();
  }

  strcpy( findpath, path );
  if( *findpath && findpath[strlen(findpath)-1] != '\\' ) {
    strcat( findpath, "\\" );
  }

  strcpy( findspec, findpath );
  strcat( findspec, "*" );

  findrc = findfirst( &hdir, findspec, FIND_ALL, &findbuff );

  while( findrc == 0 && !pl_m_peek_queue( broker_queue, PL_CLEAR, PL_TERMINATE ))
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
      char decoder[_MAX_MODULE_NAME] = "";
      DECODER_INFO info;

      if( dec_fileinfo( fullname, &info, decoder ) == PLUGIN_OK ) {
        pl_m_create_record( fullname, INDEX_END, &info, decoder );
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
        WinSendMsg( playlist, PL_MSG_REMOVE_ALL, 0, 0 );
        WinSendMsg( playlist, PL_MSG_REFRESH_STATUS, NULL, 0 );
        break;

      case PL_ADD_FILE:
        pl_broker_add_file( data->filename, data->songname,
                            data->index, data->options );
        break;

      case PL_TERMINATE:
        break;

      case PL_ADD_DIRECTORY:
        pl_broker_add_directory( data->filename, data->options );
        pl_completed();
        break;

      case PL_LOAD_PLAYLIST:
        pl_load_any_list( data->filename, data->options );
        break;

      case PL_SELECT_BY_INDEX:
        WinSendMsg( playlist, PL_MSG_SELECT_INDEX, MPFROMLONG( data->index ), 0 );
        break;

      case PL_RELOAD_BY_INDEX:
        if( !decoder_playing()) {
          amp_playmode = AMP_PLAYLIST;
          WinSendMsg( playlist, PL_MSG_LOADBY_INDEX, MPFROMLONG( data->index ), 0 );
        }
        break;

      case PL_COMPLETED:
        if( cfg.autouse || amp_playmode == AMP_PLAYLIST ) {
          amp_pl_use( TRUE );
        }
        break;
    }

    pl_m_delete_request_data( data );

    if( qu_empty( broker_queue ) && is_busy ) {
      is_busy = FALSE;
      pl_refresh_status();
    }

    if( request == PL_TERMINATE ) {
      break;
    }
  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
  _endthread();
}

/* Returns the number of records in the playlist. */
ULONG
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
static ULONG
pl_m_playleft( void )
{
  ULONG     time = 0;
  PLRECORD* rec;

  ASSERT_IS_MAIN_THREAD;

  if( cfg.shf || !loaded_record ) {
    rec = pl_m_first_record();
  } else {
    rec = loaded_record;
  }

  while( rec ) {
    if( !rec->played || rec == loaded_record || !cfg.shf ) {
      time += rec->info.songlength / 1000;
    }
    rec = pl_m_next_record( rec );
  }
  return time;
}

/* Returns a summary play time of the remained part of the playlist. */
ULONG
pl_playleft( void ) {
  return LONGFROMMR( WinSendMsg( playlist, PL_MSG_PLAY_LEFT, 0, 0 ));
}

/* Deletes all selected files. */
static void
pl_m_delete_selected( void )
{
  ASSERT_IS_MAIN_THREAD;

  if( amp_query( playlist, "Do you want remove all selected files from the playlist "
                           "and delete this files from your disk?" ))
  {
    pl_m_remove_records( NULL, 0, PL_REMOVE_SELECTED | PL_DELETE_FILES );
  }
}

/* Shows the context menu of the playlist. */
static void
pl_m_show_context_menu( HWND parent, PLRECORD* rec )
{
  POINTL   pos;
  SWP      swp;
  char     file[_MAX_PATH];
  HWND     mh;
  MENUITEM mi;
  int      i;
  int      count;
  short    id;

  ASSERT_IS_MAIN_THREAD;

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
    WinPopupMenu( parent, parent, menu_record, pos.x, pos.y, IDM_RC_PLAY,
                  PU_POSITIONONITEM | PU_HCONSTRAIN   | PU_VCONSTRAIN |
                  PU_MOUSEBUTTON1   | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
    return;
  }

  WinSendMsg( menu_playlist, MM_QUERYITEM,
              MPFROM2SHORT( IDM_PL_OPEN_MENU, TRUE ), MPFROMP( &mi ));

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
        mi.id = (IDM_PL_LAST + 1) + i;
        mi.hwndSubMenu = (HWND)NULLHANDLE;
        mi.hItem = 0;

        WinSendMsg( mh, MM_INSERTITEM, MPFROMP( &mi ), MPFROMP( file ));
      }
    }
    WinSendMsg( menu_playlist, MM_SETITEMATTR,
                MPFROM2SHORT( IDM_PL_LAST, TRUE ), MPFROM2SHORT( MIA_DISABLED, 0 ));
  } else {
    WinSendMsg( menu_playlist, MM_SETITEMATTR,
                MPFROM2SHORT( IDM_PL_LAST, TRUE ), MPFROM2SHORT( MIA_DISABLED, MIA_DISABLED ));
  }

  if( amp_playmode == AMP_PLAYLIST ) {
    WinSendMsg( menu_playlist, MM_SETITEMTEXT,
                MPFROMSHORT( IDM_PL_USE ), MPFROMP( "Don't ~use this Playlist\tCtrl+U" ));
  } else {
    WinSendMsg( menu_playlist, MM_SETITEMTEXT,
                MPFROMSHORT( IDM_PL_USE ), MPFROMP( "~Use this Playlist\tCtrl+U" ));
  }

  WinPopupMenu( parent, parent, menu_playlist, pos.x, pos.y, IDM_PL_USE,
                PU_POSITIONONITEM | PU_HCONSTRAIN   | PU_VCONSTRAIN |
                PU_MOUSEBUTTON1   | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
}

/* Prepares the playlist container item to the drag operation. */
static void
pl_m_drag_init_item( HWND hwnd, PLRECORD* rec,
                     PDRAGINFO drag_infos, PDRAGIMAGE drag_image, int i )
{
  DRAGITEM ditem = { 0 };
  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];

  ASSERT_IS_MAIN_THREAD;

  sdrivedir( pathname, rec->full, sizeof( pathname ));
  sfnameext( filename, rec->full, sizeof( filename ));

  ditem.hwndItem          = hwnd;
  ditem.ulItemID          = (ULONG)rec;
  ditem.hstrType          = DrgAddStrHandle( DRT_BINDATA );
  ditem.hstrRMF           = DrgAddStrHandle( "(DRM_123FILE,DRM_DISCARD)x(DRF_UNKNOWN)" );
  ditem.hstrContainerName = DrgAddStrHandle( pathname );
  ditem.hstrSourceName    = DrgAddStrHandle( filename );
  ditem.hstrTargetName    = DrgAddStrHandle( filename );
  ditem.fsSupportedOps    = DO_MOVEABLE | DO_COPYABLE | DO_LINKABLE;

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
pl_m_drag_init( HWND hwnd, PCNRDRAGINIT pcdi )
{
  PLRECORD*  rec;
  BOOL       drag_selected = FALSE;
  int        drag_count    = 0;
  PDRAGIMAGE drag_images   = NULL;
  PDRAGINFO  drag_infos    = NULL;

  ASSERT_IS_MAIN_THREAD;

  // If the record under the mouse is NULL, we must be over whitespace,
  // in which case we don't want to drag any records.

  if( !(PLRECORD*)pcdi->pRecord ) {
    return 0;
  }

  // Count the selected records. Also determine whether or not we should
  // process the selected records. If the container record under the
  // mouse does not have this emphasis, we shouldn't.

  for( rec = pl_m_first_selected(); rec; rec = pl_m_next_selected( rec )) {
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
    for( rec = pl_m_first_selected(); rec; rec = pl_m_next_selected( rec ), i++ ) {
      pl_m_drag_init_item( hwnd, rec, drag_infos, drag_images+i, i );
    }
  } else {
    pl_m_drag_init_item( hwnd, (PLRECORD*)pcdi->pRecord,
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
pl_m_drag_over( HWND hwnd, PCNRDRAGINFO pcdi )
{
  PDRAGINFO pdinfo = pcdi->pDragInfo;
  PDRAGITEM pditem;
  int       i;
  USHORT    drag_op = DO_UNKNOWN;
  USHORT    drag    = DOR_NEVERDROP;

  ASSERT_IS_MAIN_THREAD;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROM2SHORT( DOR_NEVERDROP, 0 );
  }

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL )) {
      if( pdinfo->usOperation == DO_DEFAULT )
      {
        drag    = DOR_DROP;
        drag_op = pdinfo->hwndSource == hwnd ? DO_MOVE : DO_COPY;
      }
      else if(( pdinfo->usOperation == DO_COPY && pditem->fsSupportedOps & DO_COPYABLE ) ||
              ( pdinfo->usOperation == DO_MOVE && pditem->fsSupportedOps & DO_MOVEABLE ) ||
              ( pdinfo->usOperation == DO_LINK && pditem->fsSupportedOps & DO_LINKABLE ))
      {
        drag    = DOR_DROP;
        drag_op = pdinfo->usOperation;
      } else {
        drag    = DOR_NODROPOP;
        drag_op = DO_UNKNOWN;
        break;
      }
    } else if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL )) {
      if( pdinfo->usOperation == DO_DEFAULT &&
          pditem->fsSupportedOps & DO_COPYABLE )
      {
        drag    = DOR_DROP;
        drag_op = DO_COPY;
      }
      else if( pdinfo->usOperation == DO_LINK &&
               pditem->fsSupportedOps & DO_LINKABLE )
      {
        drag    = DOR_DROP;
        drag_op = pdinfo->usOperation;
      } else {
        drag    = DOR_NODROPOP;
        drag_op = DO_UNKNOWN;
        break;
      }
    } else {
      drag    = DOR_NEVERDROP;
      drag_op = DO_UNKNOWN;
      break;
    }
  }

  DrgFreeDraginfo( pdinfo );
  return MPFROM2SHORT( drag, drag_op );
}

/* Discards playlist records dropped into shredder.
   Must be called from the main thread. */
static MRESULT
pl_m_drag_discard( HWND hwnd, PDRAGINFO pdinfo )
{
  PDRAGITEM  pditem;
  PLRECORD** array = NULL;
  int        i;

  ASSERT_IS_MAIN_THREAD;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROMLONG( DRR_ABORT );
  }

  // We get as many DM_DISCARDOBJECT messages as there are
  // records dragged but the first one has enough info to
  // process all of them.

  array = malloc( pdinfo->cditem * sizeof( PLRECORD* ));

  if( array ) {
    for( i = 0; i < pdinfo->cditem; i++ ) {
      pditem = DrgQueryDragitemPtr( pdinfo, i );
      array[i] = (PLRECORD*)pditem->ulItemID;
    }

    pl_m_remove_records( array, pdinfo->cditem, 0 );
    free( array );
  }

  DrgFreeDraginfo( pdinfo );
  return MRFROMLONG( DRR_SOURCE );
}

/* Receives the dropped playlist records. */
static MRESULT
pl_m_drag_drop( HWND hwnd, PCNRDRAGINFO pcdi )
{
  PDRAGINFO pdinfo = pcdi->pDragInfo;
  PDRAGITEM pditem;

  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];
  char fullname[_MAX_PATH];
  int  i;

  PLRECORD* pos = pcdi->pRecord ? (PLRECORD*)pcdi->pRecord : (PLRECORD*)CMA_END;
  int index = pl_m_index_of_record( pos );

  ASSERT_IS_MAIN_THREAD;

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

    if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ))
    {
      PLRECORD* rec = (PLRECORD*)pditem->ulItemID;
      PLRECORD* ins;

      if( pdinfo->hwndSource == hwnd ) {
        if( pdinfo->usOperation == DO_MOVE ) {
          ins = pl_m_move_record( rec, pos );
        } else {
          ins = pl_m_copy_record( rec, pos );
        }
        if( ins ) {
          pos = ins;
        }
      } else if( is_playlist( fullname )) {
        pl_load( fullname, 0 );
      } else {
        qu_write( broker_queue, PL_ADD_FILE,
                  pl_m_create_request_data( fullname, NULL, index, 0 ));

        if( index >= 0 ) {
          ++index;
        }

        if( pdinfo->usOperation == DO_MOVE ) {
          WinSendMsg( pdinfo->hwndSource, WM_123FILE_REMOVE, MPFROMP( rec ), 0 );
        }
      }
    }
    else if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ))
    {
      if( pditem->hstrContainerName && pditem->hstrSourceName ) {
        // Have full qualified file name.
        if( DrgVerifyType( pditem, "UniformResourceLocator" )) {
          amp_url_from_file( fullname, fullname, sizeof( fullname ));
        }
        if( is_dir( fullname )) {
          pl_add_directory( fullname, PL_DIR_RECURSIVE );
        } else if( is_playlist( fullname )) {
          pl_load( fullname, 0 );
        } else {
          qu_write( broker_queue, PL_ADD_FILE,
                    pl_m_create_request_data( fullname, NULL, index, 0 ));

          if( index >= 0 ) {
            ++index;
          }
        }
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
        PLDROPINFO* pdsource = (PLDROPINFO*)malloc( sizeof( PLDROPINFO ));
        char renderto[_MAX_PATH];

        if( !pdtrans || !pdsource ) {
          return 0;
        }

        pdsource->index    = index;
        pdsource->hwndItem = pditem->hwndItem;
        pdsource->ulItemID = pditem->ulItemID;

        if( index >= 0 ) {
          ++index;
        }

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
  }

  DrgDeleteDraginfoStrHandles( pdinfo );
  DrgFreeDraginfo( pdinfo );
  return 0;
}

/* Receives dropped and rendered files and urls. */
static MRESULT
pl_m_drag_render_done( HWND hwnd, PDRAGTRANSFER pdtrans, USHORT rc )
{
  char rendered[_MAX_PATH];
  char fullname[_MAX_PATH];

  PLDROPINFO* pdsource = (PLDROPINFO*)pdtrans->ulTargetInfo;
  ASSERT_IS_MAIN_THREAD;

  // If the rendering was successful, use the file, then delete it.
  if(( rc & DMFL_RENDEROK ) && pdsource &&
       DrgQueryStrName( pdtrans->hstrRenderToName, sizeof( rendered ), rendered ))
  {
    amp_url_from_file( fullname, rendered, sizeof( fullname ));
    DosDelete( rendered );

    if( is_playlist( fullname )) {
      pl_load( fullname, 0 );
    } else {
      qu_write( broker_queue, PL_ADD_FILE,
                pl_m_create_request_data( fullname, NULL, pdsource->index, 0 ));
    }

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

/* Loads the specified playlist record into the player and
   plays it if this is specified in the player properties or
   the player is already playing. Must be called from the
   main thread. */
static BOOL
pl_m_play_record( PLRECORD* rec )
{
  BOOL decoder_was_playing = decoder_playing();
  ASSERT_IS_MAIN_THREAD;

  if( decoder_was_playing ) {
    amp_stop();
  }

  if( amp_playmode == AMP_NOFILE ) {
    amp_playmode = AMP_SINGLE;
  }

  if( pl_m_load_record( rec )) {
    if( cfg.playonload == 1 || decoder_was_playing ) {
      amp_play( 0 );
    }
  }

  return TRUE;
}

/* Adds user selected files or directory to the playlist. */
static void
pl_dlg_add_files( HWND owner )
{
  FILEDLG filedialog;

  int   i = 0;
  char* file;
  char  type_audio[2048];
  char  type_all  [2048];
  APSZ  types[4] = {{ 0 }};

  ASSERT_IS_MAIN_THREAD;

  types[0][0] = type_audio;
  types[1][0] = FDT_PLAYLIST;
  types[2][0] = type_all;

  memset( &filedialog, 0, sizeof( FILEDLG ));
  filedialog.cbSize     = sizeof( FILEDLG );
  filedialog.fl         = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM | FDS_MULTIPLESEL;
  filedialog.ulUser     = FDU_DIR_ENABLE | FDU_RECURSEBTN;
  filedialog.pszTitle   = "Add file(s) to playlist";
  filedialog.hMod       = hmodule;
  filedialog.usDlgId    = DLG_FILE;
  filedialog.pfnDlgProc = amp_file_dlg_proc;

  // WinFileDlg returns error if a length of the pszIType string is above
  // 255 characters. Therefore the small part from the full filter is used
  // as initial extended-attribute type filter. This part has enough to
  // find the full filter in the papszITypeList.
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_AUDIO;

  strcpy( type_audio, FDT_AUDIO );
  dec_fill_types( type_audio + strlen( type_audio ),
                  sizeof( type_audio ) - strlen( type_audio ) - 1 );
  strcat( type_audio, ")" );

  strcpy( type_all, FDT_AUDIO_ALL );
  dec_fill_types( type_all + strlen( type_all ),
                  sizeof( type_all ) - strlen( type_all ) - 1 );
  strcat( type_all, ")" );

  strcpy( filedialog.szFullFile, cfg.filedir );
  WinFileDlg( HWND_DESKTOP, owner, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    if( filedialog.ulFQFCount > 1 ) {
      file = (*filedialog.papszFQFilename)[i];
    } else {
      file = filedialog.szFullFile;
    }

    while( *file ) {
      if( is_dir( file )) {
        pl_add_directory( file, filedialog.ulUser & FDU_RECURSE_ON ? PL_DIR_RECURSIVE : 0 );
        strcpy( cfg.filedir, file );
        if( !is_root( file )) {
          strcat( cfg.filedir, "\\" );
        }
      } else if( is_playlist( file )) {
        pl_load( file, PL_LOAD_NOT_RECALL );
        sdrivedir( cfg.filedir, file, sizeof( cfg.filedir ));
      } else {
        pl_add_file( file, NULL, 0 );
        sdrivedir( cfg.filedir, file, sizeof( cfg.filedir ));
      }

      if( ++i >= filedialog.ulFQFCount ) {
        break;
      } else {
        file = (*filedialog.papszFQFilename)[i];
      }
    }
    pl_completed();
  }

  WinFreeFileDlgList( filedialog.papszFQFilename );
}

/* Loads a playlist selected by the user to the player. Must be
   called from the main thread. */
static void
pl_dlg_load_list( HWND owner )
{
  FILEDLG filedialog;
  APSZ types[] = {{ FDT_PLAYLIST }, { 0 }};

  ASSERT_IS_MAIN_THREAD;
  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Open playlist";
  filedialog.hMod           = hmodule;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_PLAYLIST;

  strcpy( filedialog.szFullFile, cfg.listdir );
  WinFileDlg( HWND_DESKTOP, owner, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
    if( is_playlist( filedialog.szFullFile )) {
      pl_load( filedialog.szFullFile, PL_LOAD_CLEAR );
    }
  }
}

/* Saves current playlist to the file specified by user.
   Must be called from the main thread. */
static void
pl_dlg_save_list( HWND owner )
{
  FILEDLG filedialog;

  APSZ  types[] = {{ FDT_PLAYLIST_LST }, { FDT_PLAYLIST_M3U }, { FDT_PLAYLIST_M3U8 }, { 0 }};
  char  filez[_MAX_PATH];
  char  ext  [_MAX_EXT ];
  int   options = 0;

  ASSERT_IS_MAIN_THREAD;
  memset( &filedialog, 0, sizeof( FILEDLG ));

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_CUSTOM | FDS_ENABLEFILELB;
  filedialog.pszTitle       = "Save playlist";
  filedialog.hMod           = hmodule;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.ulUser         = FDU_RELATIVBTN;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = *types[ truncate( cfg.save_type, 0, 2 )];

  strcpy( filedialog.szFullFile, cfg.listdir );
  WinFileDlg( HWND_DESKTOP, owner, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    sdrivedir( cfg.listdir, filedialog.szFullFile, sizeof( cfg.listdir ));
    cfg.save_type = truncate( filedialog.sEAType, 0, 2 );

    if( stricmp( *types[ cfg.save_type ], FDT_PLAYLIST_M3U ) == 0 ) {
      options |= PL_SAVE_M3U;
    } else if( stricmp( *types[ cfg.save_type ], FDT_PLAYLIST_M3U8 ) == 0 ) {
      options |= PL_SAVE_M3U | PL_SAVE_UTF8;
    } else {
      options |= PL_SAVE_LST;
    }
    if( filedialog.ulUser & FDU_RELATIV_ON ) {
      options |= PL_SAVE_RELATIVE;
    }

    strcpy( filez, filedialog.szFullFile );
    if( strcmp( sfext( ext, filez, sizeof( ext )), "" ) == 0 ) {
      if( options & PL_SAVE_M3U ) {
        if( options & PL_SAVE_UTF8 ) {
          strcat( filez, ".m3u8" );
        } else {
          strcat( filez, ".m3u"  );
        }
      } else {
        strcat( filez, ".lst" );
      }
    }
    if( amp_warn_if_overwrite( owner, filez )) {
      pl_save( filez, options );
    }
  }
}

/* Initializes the playlist presentation window. */
static void
pl_m_init_window( HWND hwnd )
{
  FIELDINFO* first;
  FIELDINFO* field;
  HPOINTER   hicon;
  HACCEL     accel;
  MENUITEM   mi;
  HAB        hab = WinQueryAnchorBlock( hwnd );

  FIELDINFOINSERT insert;
  CNRINFO cnrinfo;

  ASSERT_IS_MAIN_THREAD;
  container = WinWindowFromID( hwnd, CNR_PLAYLIST );

  /* Initializes the container of the playlist. */
  first = (FIELDINFO*)WinSendMsg( container, CM_ALLOCDETAILFIELDINFO, MPFROMSHORT(6), 0 );
  field = first;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_BITMAPORICON;
  field->pTitleData = "";
  field->offStruct  = FIELDOFFSET( PLRECORD, rc.hptrIcon);

  field = field->pNextFieldInfo;

  field->flData     = CFA_STRING | CFA_HORZSEPARATOR;
  field->pTitleData = "Filename";
  field->offStruct  = FIELDOFFSET( PLRECORD, rc.pszIcon );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Song name";
  field->offStruct  = FIELDOFFSET( PLRECORD, songname );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING | CFA_RIGHT;
  field->pTitleData = "Size";
  field->offStruct  = FIELDOFFSET( PLRECORD, size );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Time";
  field->offStruct  = FIELDOFFSET( PLRECORD, time );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Information";
  field->offStruct  = FIELDOFFSET( PLRECORD, moreinfo );

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
  cnrinfo.xVertSplitbar  = cfg.sbar_playlist;

  WinSendMsg( container, CM_SETCNRINFO, MPFROMP(&cnrinfo),
              MPFROMLONG( CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR | CMA_FLWINDOWATTR ));

  /* Initializes the playlist presentation window. */
  accel = WinLoadAccelTable( hab, hmodule, ACL_PLAYLIST );

  if( accel ) {
    WinSetAccelTable( hab, accel, hwnd );
  }

  hicon = WinLoadPointer( HWND_DESKTOP, hmodule, ICO_MAIN );
  WinSendMsg( hwnd, WM_SETICON, (MPARAM)hicon, 0 );
  do_warpsans( hwnd );

  if( !rest_window_pos( hwnd, 0 )) {
    pl_set_colors( DEF_FG_COLOR, DEF_BG_COLOR, DEF_HI_FG_COLOR, DEF_HI_BG_COLOR );
  }

  menu_playlist = WinLoadMenu( HWND_OBJECT, hmodule, MNU_PLAYLIST );
  menu_record   = WinLoadMenu( HWND_OBJECT, hmodule, MNU_RECORD   );

  WinSendMsg( menu_playlist, MM_QUERYITEM,
              MPFROM2SHORT( IDM_PL_OPEN_MENU, TRUE ), MPFROMP( &mi ));

  WinSetWindowULong( mi.hwndSubMenu, QWL_STYLE,
    WinQueryWindowULong( mi.hwndSubMenu, QWL_STYLE ) | MS_CONDITIONALCASCADE );

  WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID,
                              MPFROMLONG( IDM_PL_OPEN_LIST ), 0 );

  WinSendMsg( menu_playlist, MM_QUERYITEM,
              MPFROM2SHORT( IDM_PL_ADD_MENU, TRUE ), MPFROMP( &mi ));

  WinSetWindowULong( mi.hwndSubMenu, QWL_STYLE,
    WinQueryWindowULong( mi.hwndSubMenu, QWL_STYLE ) | MS_CONDITIONALCASCADE );

  WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID,
                              MPFROMLONG( IDM_PL_ADD_FILES ), 0 );

  broker_queue = qu_create();

  if( !broker_queue ) {
    amp_player_error( "Unable create playlist service queue." );
  } else {
    if(( broker_tid = _beginthread( pl_broker, NULL, 2048000, NULL )) == -1 ) {
      amp_error( container, "Unable create the playlist service thread." );
    }
  }
}

/* Processes messages of the playlist presentation window. */
static MRESULT EXPENTRY
pl_m_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_INITDLG:
      pl_m_init_window( hwnd );
      dk_add_window( hwnd, 0 );
      break;

    case WM_HELP:
      amp_show_help( IDH_PL );
      return 0;

    case WM_SYSCOMMAND:
      if( SHORT1FROMMP( mp1 ) == SC_CLOSE ) {
        pl_show( FALSE );
        return 0;
      }
      break;

    case WM_WINDOWPOSCHANGED:
    {
      SWP* pswp = PVOIDFROMMP(mp1);

      if( pswp[0].fl & SWP_SHOW ) {
        cfg.show_playlist = TRUE;
      }
      if( pswp[0].fl & SWP_HIDE ) {
        cfg.show_playlist = FALSE;
      }
      break;
    }

    case DM_DISCARDOBJECT:
      return pl_m_drag_discard( hwnd, (PDRAGINFO)mp1 );
    case DM_RENDERCOMPLETE:
      return pl_m_drag_render_done( hwnd, (PDRAGTRANSFER)mp1, SHORT1FROMMP( mp2 ));

    case WM_123FILE_REMOVE:
    {
      PLRECORD* rec = (PLRECORD*)mp1;
      pl_m_remove_records( &rec, 1, 0 );
      return 0;
    }

    case WM_123FILE_LOAD:
      pl_m_play_record((PLRECORD*)mp1 );
      return 0;

    case WM_COMMAND:
      if( COMMANDMSG(&msg)->cmd >  IDM_PL_LAST &&
          COMMANDMSG(&msg)->cmd <= IDM_PL_LAST + MAX_RECALL )
      {
        char filename[_MAX_PATH];
        strcpy( filename, cfg.list[ COMMANDMSG(&msg)->cmd - IDM_PL_LAST - 1 ]);

        if( is_playlist( filename )) {
          pl_load( filename, PL_LOAD_CLEAR );
        }
        return 0;
      }

      switch( COMMANDMSG(&msg)->cmd ) {
        case IDM_PL_SORT_RANDOM:
          pl_m_sort( PL_SORT_RAND  );
          return 0;
        case IDM_PL_SORT_SIZE:
          pl_m_sort( PL_SORT_SIZE  );
          return 0;
        case IDM_PL_SORT_TIME:
          pl_m_sort( PL_SORT_TIME  );
          return 0;
        case IDM_PL_SORT_FILE:
          pl_m_sort( PL_SORT_FILE  );
          return 0;
        case IDM_PL_SORT_SONG:
          pl_m_sort( PL_SORT_SONG  );
          return 0;
        case IDM_PL_SORT_TRACK:
          pl_m_sort( PL_SORT_TRACK );
          return 0;
        case IDM_PL_CLEAR:
          pl_clear();
          return 0;
        case IDM_PL_USE:
          amp_pl_use(!( amp_playmode == AMP_PLAYLIST ));
          return 0;
        case IDM_PL_ADD_URL:
          amp_load_url( hwnd, URL_ADD_TO_LIST );
          return 0;
        case IDM_PL_ADD_TRACKS:
          amp_load_track( hwnd, TRK_ADD_TO_LIST );
          return 0;
        case IDM_PL_ADD_FILES:
          pl_dlg_add_files( hwnd );
          return 0;
        case IDM_PL_SAVE_LIST:
          pl_dlg_save_list( hwnd );
          return 0;
        case IDM_PL_OPEN_LIST:
          pl_dlg_load_list( hwnd );
          return 0;
        case IDM_RC_PLAY:
          pl_m_play_record( pl_m_cursored());
          return 0;
        case IDM_RC_REMOVE:
          pl_m_remove_records( NULL, 0, PL_REMOVE_SELECTED );
          return 0;
        case IDM_RC_DELETE:
          pl_m_delete_selected();
          return 0;

        case IDM_PL_MENU_RECORD:
          WinSendMsg( hwnd, WM_CONTROL,
                      MPFROM2SHORT( CNR_PLAYLIST, CN_CONTEXTMENU ),
                      PVOIDFROMMP( NULL ));
          return 0;

        case IDM_PL_MENU:
          WinSendMsg( hwnd, WM_CONTROL,
                      MPFROM2SHORT( CNR_PLAYLIST, CN_CONTEXTMENU ),
                      PVOIDFROMMP( pl_m_cursored()));
          return 0;

        case IDM_RC_EDIT:
        {
          PLRECORD* rec = pl_m_cursored();
          PLRECORD* selected;

          DECODER_INFO info = current_info;
          char decoder [_MAX_MODULE_NAME];
          char filename[_MAX_PATH];
          int  rc;

          if( rec ) {
            strlcpy( filename, rec->full,    sizeof( filename ));
            strlcpy( decoder,  rec->decoder, sizeof( decoder  ));
            info = rec->info;

            if(( selected = pl_m_first_selected()) != NULL
                 && pl_m_next_selected( selected ) != NULL )
            {
              rc = tag_edit( hwnd, filename, decoder, &info, TAG_GROUP_OPERATIONS );
            } else {
              rc = tag_edit( hwnd, filename, decoder, &info, 0 );
            }
            if( rc == TAG_APPLY ) {
              tag_apply( filename, decoder, &info, TAG_APPLY_ALL );
            } else if( rc == TAG_APPLY_TO_GROUP ) {
              for( selected = pl_m_first_selected(); selected; selected = pl_m_next_selected( selected )) {
                tag_apply( selected->full, selected->decoder, &info, TAG_APPLY_CHOICED );
              }
            }
          }
          return 0;
        }
      }
      break;

    case WM_CONTROL:
      switch( SHORT2FROMMP( mp1 )) {
        case CN_CONTEXTMENU:
          pl_m_show_context_menu( hwnd, (PLRECORD*)mp2 );
          return 0;

        case CN_HELP:
          amp_show_help( IDH_PL );
          return 0;

        case CN_ENTER:
        {
          NOTIFYRECORDENTER* notify = (NOTIFYRECORDENTER*)mp2;
          if( notify->pRecord ) {
            pl_m_play_record((PLRECORD*)notify->pRecord );
          }
          return 0;
        }

        case CN_INITDRAG:
          return pl_m_drag_init( hwnd, (PCNRDRAGINIT)mp2 );
        case CN_DRAGAFTER:
          return pl_m_drag_over( hwnd, (PCNRDRAGINFO)mp2 );
        case CN_DROP:
          return pl_m_drag_drop( hwnd, (PCNRDRAGINFO)mp2 );
      }
      break;

    case PL_MSG_MARK_AS_PLAY:
      pl_m_mark_as_play( BOOLFROMMP( mp1 ));
      return 0;
    case PL_MSG_REMOVE_ALL:
      pl_m_remove_all();
      return 0;
    case PL_MSG_REMOVE_RECORD:
      pl_m_remove_records( NULL, 0, LONGFROMMP( mp1 ));
      return 0;
    case PL_MSG_REFRESH_FILE:
      pl_m_refresh_file( mp1, mp2 );
      return 0;
    case PL_MSG_REFRESH_SONGNAME:
      pl_m_refresh_songname( mp1, mp2 );
      return 0;
    case PL_MSG_REFRESH_STATUS:
      pl_m_refresh_status( mp1 );
      return 0;
    case PL_MSG_SELECT_INDEX:
      pl_m_select( pl_m_record_by_index( LONGFROMMP( mp1 )));
      return 0;
    case PL_MSG_LOADBY_INDEX:
      pl_m_load_record( pl_m_record_by_index( LONGFROMMP( mp1 )));
      return 0;
    case PL_MSG_CLEAN_SHUFFLE:
      pl_m_clean_shuffle();
      return 0;
    case PL_MSG_INSERT_RECALL:
      pl_m_insert_to_recall_list( mp1 );
      return 0;

    case PL_MSG_LOAD_FRST_RECORD:
      return MRFROMBOOL( pl_m_load_record( pl_m_query_first_record()));
    case PL_MSG_LOAD_NEXT_RECORD:
      return MRFROMBOOL( pl_m_load_record( pl_m_query_next_record()));
    case PL_MSG_LOAD_PREV_RECORD:
      return MRFROMBOOL( pl_m_load_record( pl_m_query_prev_record()));
    case PL_MSG_LOAD_FILE_RECORD:
      return MRFROMBOOL( pl_m_load_record( pl_m_query_file_record( mp1 )));
    case PL_MSG_PLAY_LEFT:
      return MRFROMLONG( pl_m_playleft());
    case PL_MSG_INSERT_RECORD:
      return MRFROMBOOL( pl_m_insert_record( LONGFROMMP( mp1 ), mp2 ));
    case PL_MSG_LOADED_INDEX:
      return MRFROMLONG( pl_m_index_of_record( loaded_record ));
    case PL_MSG_SAVE_BUNDLE:
      return MRFROMBOOL( pl_m_save_bundle( mp1, LONGFROMMP( mp2 )));
    case PL_MSG_SAVE_PLAYLIST:
      return MRFROMBOOL( pl_m_save_list( mp1, LONGFROMMP( mp2 )));
  }

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Sets the visibility state of the playlist presentation window. */
void
pl_show( BOOL show )
{
  HSWITCH hswitch = WinQuerySwitchHandle( playlist, 0 );
  SWCNTRL swcntrl;

  if( WinQuerySwitchEntry( hswitch, &swcntrl ) == 0 ) {
    swcntrl.uchVisibility = show ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry( hswitch, &swcntrl );
  }

  dk_set_state( playlist, show ? 0 : DK_IS_GHOST );
  WinSetWindowPos( playlist, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );
}

/* Returns the visibility state of the playlist presentation window. */
BOOL
pl_is_visible( void ) {
  return WinIsWindowVisible( playlist );
}

/* Changes the playlist colors. */
BOOL
pl_set_colors( ULONG fgcolor, ULONG bgcolor, ULONG hi_fgcolor, ULONG hi_bgcolor )
{
  RGB rgb;

  if( fgcolor != 0xFFFFFFFFUL ) {
    rgb.bRed   = ( fgcolor & 0x00FF0000UL ) >> 16;
    rgb.bGreen = ( fgcolor & 0x0000FF00UL ) >>  8;
    rgb.bBlue  = ( fgcolor & 0x000000FFUL );

    WinSetPresParam( container, PP_FOREGROUNDCOLOR, sizeof(rgb), &rgb );
  }

  if( bgcolor != 0xFFFFFFFFUL ) {
    rgb.bRed   = ( bgcolor & 0x00FF0000UL ) >> 16;
    rgb.bGreen = ( bgcolor & 0x0000FF00UL ) >>  8;
    rgb.bBlue  = ( bgcolor & 0x000000FFUL );

    WinSetPresParam( container, PP_BACKGROUNDCOLOR, sizeof(rgb), &rgb );
  }

  if( hi_fgcolor != 0xFFFFFFFFUL ) {
    rgb.bRed   = ( hi_fgcolor & 0x00FF0000UL ) >> 16;
    rgb.bGreen = ( hi_fgcolor & 0x0000FF00UL ) >>  8;
    rgb.bBlue  = ( hi_fgcolor & 0x000000FFUL );

    WinSetPresParam( container, PP_HILITEFOREGROUNDCOLOR, sizeof(rgb), &rgb );
  }

  if( hi_bgcolor != 0xFFFFFFFFUL ) {
    rgb.bRed   = ( hi_bgcolor & 0x00FF0000UL ) >> 16;
    rgb.bGreen = ( hi_bgcolor & 0x0000FF00UL ) >>  8;
    rgb.bBlue  = ( hi_bgcolor & 0x000000FFUL );

    WinSetPresParam( container, PP_HILITEBACKGROUNDCOLOR, sizeof(rgb), &rgb );
  }

  return TRUE;
}

/* Creates the playlist presentation window. Must be called
   from the main thread. */
HWND
pl_create( void )
{
  ASSERT_IS_MAIN_THREAD;
  playlist = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
                         pl_m_dlg_proc, hmodule, DLG_PLAYLIST, NULL );

  pl_show( cfg.show_playlist );
  return playlist;
}

/* Destroys the playlist presentation window. Must be called
   from the main thread. */
void
pl_destroy( void )
{
  HAB     hab   = WinQueryAnchorBlock( playlist );
  HACCEL  accel = WinQueryAccelTable ( hab, playlist );
  CNRINFO info;

  ASSERT_IS_MAIN_THREAD;

  pl_m_purge_queue( broker_queue );
  qu_push( broker_queue, PL_TERMINATE,
           pl_m_create_request_data( NULL, NULL, INDEX_NONE, 0 ));

  wait_thread( broker_tid, 2000 );
  qu_close( broker_queue );
  save_window_pos( playlist, 0 );

  if( WinSendMsg( container, CM_QUERYCNRINFO,
                  MPFROMP(&info), MPFROMLONG(sizeof(info))) != 0 )
  {
    cfg.sbar_playlist = info.xVertSplitbar;
  }

  WinSetAccelTable( hab, playlist, NULLHANDLE );
  if( accel != NULLHANDLE ) {
    WinDestroyAccelTable( accel );
  }

  pl_m_remove_all();
  WinDestroyWindow( menu_record   );
  WinDestroyWindow( menu_playlist );
  WinDestroyWindow( playlist      );
}

/* Sends request about clearing of the playlist. */
BOOL
pl_clear()
{
  pl_m_purge_queue( broker_queue );
  WinSendMsg( playlist, PL_MSG_REFRESH_STATUS, "", 0 );
  return qu_push( broker_queue, PL_CLEAR,
                  pl_m_create_request_data( NULL, NULL, INDEX_NONE, 0 ));
}

/* Sends request about addition of the whole directory to the playlist. */
BOOL
pl_add_directory( const char* path, int options )
{
  return qu_write( broker_queue, PL_ADD_DIRECTORY,
                   pl_m_create_request_data( path, NULL, INDEX_END, options ));
}

/* Sends request about addition of the file to the playlist. */
BOOL
pl_add_file( const char* filename, const char* songname, int options )
{
  return qu_write( broker_queue, PL_ADD_FILE,
                   pl_m_create_request_data( filename, songname, INDEX_END, options ));
}

/* Notifies on completion of the playlist */
BOOL
pl_completed( void )
{
  return qu_write( broker_queue, PL_COMPLETED,
                   pl_m_create_request_data( NULL, NULL, INDEX_NONE, 0 ));
}

/* Returns true if the specified file is a playlist file. */
BOOL
is_playlist( const char *filename )
{
 char ext[_MAX_EXT];
 sfext( ext, filename, sizeof( ext ));
 return ( stricmp( ext, ".lst"  ) == 0 ||
          stricmp( ext, ".mpl"  ) == 0 ||
          stricmp( ext, ".pls"  ) == 0 ||
          stricmp( ext, ".m3u"  ) == 0 ||
          stricmp( ext, ".m3u8" ) == 0 );
}

/* Loads the PM123 native playlist file. */
static BOOL
pl_load_lst_list( const char* filename, XFILE* playlist, int options )
{
  char basepath[_MAX_PATH];
  char fullname[_MAX_PATH] = "";
  char file    [_MAX_PATH];

  HPOPULATE hp;

  int  bitrate    = -1;
  int  samplerate = -1;
  int  mode       = -1;
  int  filesize   = -1;
  int  secs       = -1;

  sdrivedir( basepath, filename, sizeof( basepath ));

  if( options & PL_LOAD_TO_PM ) {
    if(!( hp = pm_begin_populate( filename ))) {
      return FALSE;
    }
  }

  while( xio_fgets( file, sizeof(file), playlist ))
  {
    blank_strip( file );

    if( *file == '>' ) {
      sscanf( file, ">%d,%d,%d,%d,%d\n", &bitrate, &samplerate, &mode, &filesize, &secs );
    } else if( *file != 0 && *file != '#' && *file != '<' ) {
      if( *fullname ) {
        if( options & PL_LOAD_TO_PM ) {
          if( !pm_add_file( hp, fullname, bitrate, samplerate, mode, filesize, secs )) {
            break;
          } else {
            bitrate    = -1;
            samplerate = -1;
            mode       = -1;
            filesize   = -1;
            secs       = -1;
          }
        } else {
          pl_add_file( fullname, NULL, 0 );
        }
      }
      rel2abs( basepath, file, fullname, sizeof( fullname ));
    }
  }

  if( xio_ferror( playlist )) {
    amp_player_error( "%s", xio_strerror( xio_errno()));
  }

  if( *fullname ) {
    if( options & PL_LOAD_TO_PM ) {
      pm_add_file( hp, fullname, bitrate, samplerate, mode, filesize, secs );
    } else {
      pl_add_file( fullname, NULL, 0 );
    }
  }

  if( options & PL_LOAD_TO_PM ) {
    pm_end_populate( hp );
  }
  return TRUE;
}

/* Loads the M3U playlist file. */
static BOOL
pl_load_m3u_list( const char* filename, XFILE* playlist, int options )
{
  char basepath[_MAX_PATH];
  char fullname[_MAX_PATH];
  char file    [4096];
  int  secs;

  HPOPULATE hp;
  sdrivedir( basepath, filename, sizeof( basepath ));

  if( options & PL_LOAD_TO_PM ) {
    if(!( hp = pm_begin_populate( filename ))) {
      return FALSE;
    }
  }

  if( xio_fgets( file, sizeof(file), playlist )) {
    if( strnicmp( file, "\xEF\xBB\xBF", 3 ) == 0 ) {
      options |= PL_LOAD_UTF8;
      memmove( file, file + 3, strlen( file ) + 1 - 3 );
    }
  }

  do
  {
    blank_strip( file );

    if( *file == '#' ) {
      if( strnicmp( file, "#EXTINF:", 8 ) == 0 ) {
        sscanf( file, "#EXTINF:%d,%*s", &secs );
      }
    } else if( *file != 0 ) {
      if( options & PL_LOAD_UTF8 ) {
        ch_convert( CH_UTF_8, file, CH_DEFAULT, file, sizeof( file ));
      }
      if( rel2abs( basepath, file, fullname, sizeof(fullname))) {
        if( options & PL_LOAD_TO_PM ) {
          if( !pm_add_file( hp, fullname, -1, -1, -1, -1, secs )) {
            break;
          }
          secs = -1;
        } else {
          pl_add_file( fullname, NULL, 0 );
        }
      }
    }
  } while( xio_fgets( file, sizeof(file), playlist ));

  if( xio_ferror( playlist )) {
    amp_player_error( "%s", xio_strerror( xio_errno()));
  }

  if( options & PL_LOAD_TO_PM ) {
    pm_end_populate( hp );
  }
  return TRUE;
}

/* Loads the WinAMP playlist file. */
static BOOL
pl_load_pls_list( const char* filename, XFILE* playlist, int options )
{
  char  basepath[_MAX_PATH];
  char  fullname[_MAX_PATH] = "";
  char  file    [_MAX_PATH] = "";
  char  title   [_MAX_PATH] = "";
  char  line    [_MAX_PATH];
  char* eq_pos;
  int   secs = -1;
  int   id   = -1;

  HPOPULATE hp;
  sdrivedir( basepath, filename, sizeof( basepath ));

  if( options & PL_LOAD_TO_PM ) {
    if(!( hp = pm_begin_populate( filename ))) {
      return FALSE;
    }
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
          if( *fullname ) {
            if( options & PL_LOAD_TO_PM ) {
              if( !pm_add_file( hp, fullname, -1, -1, -1, -1, secs )) {
                return FALSE;
              }
            } else {
              pl_add_file( fullname, *title ? title : NULL, 0 );
            }
           *title =  0;
            secs  = -1;
          }

          strlcpy( file, eq_pos + 1, sizeof(file));
          rel2abs( basepath, file, fullname, sizeof(fullname));
          id = atoi( &line[4] );
        }
        else if( strnicmp( line, "Title", 5 ) == 0 )
        {
          // We hope the title field always follows the file field.
          if( id == atoi( &line[5] )) {
            strlcpy( title, eq_pos + 1, sizeof(title));
          }
        }
        else if( strnicmp( line, "Length", 6 ) == 0 )
        {
          // We hope the length field always follows the file field.
          if( id == atoi( &line[6] )) {
            secs = atoi( eq_pos + 1 );
          }
        }
      }
    }
  }

  if( *fullname ) {
    if( options & PL_LOAD_TO_PM ) {
      pm_add_file( hp, fullname, -1, -1, -1, -1, secs );
    } else {
      pl_add_file( fullname, *title ? title : NULL, 0 );
    }
  }

  if( options & PL_LOAD_TO_PM ) {
    pm_end_populate( hp );
  }
  return TRUE;
}

/* Loads the WarpAMP playlist file. */
static BOOL
pl_load_mpl_list( const char* filename, XFILE* playlist, int options ) {
  return pl_load_pls_list( filename, playlist, options );
}

/* Appends the specified playlist to recall list. */
static void
pl_m_insert_to_recall_list( const char* filename )
{
  int i;
  ASSERT_IS_MAIN_THREAD;

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

/* Loads the specified playlist file. */
static BOOL
pl_load_any_list( const char* filename, int options )
{
  XFILE* file;
  BOOL   rc = FALSE;
  char   ext[_MAX_EXT];

  if( !( options & PL_LOAD_TO_PM )) {
    if( !is_busy ) {
      is_busy = TRUE;
      pl_refresh_status();
    }
  }

  if(!( file = xio_fopen( filename, "r" ))) {
    if(!( options & PL_LOAD_TO_PM )) {
      amp_error( container, "Unable open playlist file:\n%s\n%s",
                            filename, xio_strerror( xio_errno()));
    }
    return FALSE;
  }

  sfext( ext, filename, sizeof( ext ));

  if( stricmp( ext, ".lst" ) == 0 ) {
    rc = pl_load_lst_list( filename, file, options );
  } else if( stricmp( ext, ".m3u"  ) == 0 ) {
    rc = pl_load_m3u_list( filename, file, options );
  } else if( stricmp( ext, ".m3u8" ) == 0 ) {
    rc = pl_load_m3u_list( filename, file, options | PL_LOAD_UTF8 );
  } else if( stricmp( ext, ".mpl"  ) == 0 ) {
    rc = pl_load_mpl_list( filename, file, options );
  } else if( stricmp( ext, ".pls"  ) == 0 ) {
    rc = pl_load_pls_list( filename, file, options );
  } else {
    rc = pl_load_m3u_list( filename, file, options );
  }

  if( rc && !( options & PL_LOAD_TO_PM )) {
    if(!( options & PL_LOAD_CONTINUE )) {
      pl_completed();
      WinSendMsg( playlist, PL_MSG_REFRESH_STATUS, MPFROMP( filename ), 0 );
    }
    if(!( options & PL_LOAD_NOT_RECALL )) {
      WinSendMsg( playlist, PL_MSG_INSERT_RECALL,  MPFROMP( filename ), 0 );
    }
  }

  xio_fclose( file );
  return rc;
}

/* Sends request about loading the specified playlist file. */
BOOL
pl_load( const char* filename, int options )
{
  if( options & PL_LOAD_CLEAR ) {
    pl_clear();
  }

  if( options & PL_LOAD_NOT_QUEUE ) {
    return pl_load_any_list( filename, options );
  } else {
    return qu_write( broker_queue, PL_LOAD_PLAYLIST,
                     pl_m_create_request_data( filename, NULL, INDEX_NONE, options ));
  }
}

/* Saves playlist to the specified file. */
static BOOL
pl_m_save_list( const char* filename, int options )
{
  PLRECORD* rec;
  FILE*     file;
  char      base[_MAX_PATH];
  char      path[_MAX_PATH];
  char      line[4096];

  ASSERT_IS_MAIN_THREAD;

  if(!( file = fopen( filename, "w" ))) {
    amp_error( container, "Unable open playlist file:\n%s\n%s", filename, strerror( errno ));
    return FALSE;
  }

  if( options & PL_SAVE_M3U ) {
    if( options & PL_SAVE_UTF8 ) {
      fprintf( file, "\xEF\xBB\xBF#EXTM3U\n" );
    } else {
      fprintf( file, "#EXTM3U\n" );
    }
  } else {
    fprintf( file, "#\n"
                   "# Playlist created with %s\n"
                   "# Do not modify!\n"
                   "# Lines starting with '>' are used by Playlist Manager.\n"
                   "#\n", AMP_FULLNAME );
  }

  sdrivedir( base, filename, sizeof( base ));
  for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec ))
  {
    if( options & PL_SAVE_M3U ) {
      if( options & PL_SAVE_UTF8 ) {
        ch_convert( CH_DEFAULT, rec->songname, CH_UTF_8, line, sizeof( line ));
      } else {
        strlcpy( line, rec->songname, sizeof( line ));
      }
      fprintf( file, "#EXTINF:%d,%s\n", rec->info.songlength / 1000, line  );
    } else {
      fprintf( file, "# %s, %s, %s\n", rec->size, rec->time, rec->moreinfo );
    }

    if( options & PL_SAVE_RELATIVE
        && is_file( rec->full )
        && abs2rel( base, rec->full, path, sizeof(path)))
    {
      strlcpy( line, path, sizeof( line ));
    } else {
      strlcpy( line, rec->full, sizeof( line ));
    }

    if( options & PL_SAVE_UTF8 ) {
      ch_convert( CH_DEFAULT, line, CH_UTF_8, line, sizeof( line ));
    }

    fprintf( file, "%s\n", line );

    if( !(options & PL_SAVE_M3U )) {
      fprintf( file, ">%d,%d,%d,%d,%d\n",
               rec->info.bitrate, rec->info.format.samplerate,
               rec->info.mode, rec->info.filesize, rec->info.songlength / 1000 );
    }
  }

  if( !(options & PL_SAVE_M3U )) {
    fprintf( file, "# End of playlist\n" );
  }

  fclose( file );
  WinSendMsg( playlist, PL_MSG_REFRESH_STATUS, MPFROMP( filename ), 0 );
  return TRUE;
}

/* Saves playlist to the specified file. */
BOOL
pl_save( const char* filename, int options ) {
  return BOOLFROMMR( WinSendMsg( playlist, PL_MSG_SAVE_PLAYLIST,
                                 MPFROMP( filename ), MPFROMLONG( options )));
}

/* Saves the playlist and player status to the specified file. */
static BOOL
pl_m_save_bundle( const char* filename, int options )
{
  FILE*     file;
  PLRECORD* rec;

  ASSERT_IS_MAIN_THREAD;

  if(!( file = fopen( filename, "w" ))) {
    amp_error( container, "Unable create status file:\n%s\n%s", filename, strerror(errno));
    return FALSE;
  }

  fprintf( file, "#\n"
                 "# Player state file created with %s\n"
                 "# Do not modify! This file is compatible with the playlist format,\n"
                 "# but information written in this file is different.\n"
                 "#\n", AMP_FULLNAME );

  for( rec = pl_m_first_record(); rec; rec = pl_m_next_record( rec ))
  {
    fprintf( file, ">%u,%u\n",
             rec->rc.flRecordAttr & CRA_CURSORED ? 1 : 0,
             rec == loaded_record && amp_playmode == AMP_PLAYLIST );
    fprintf( file, "%s\n" , rec->full );
  }

  if( amp_playmode == AMP_SINGLE && *current_filename && is_file( current_filename )) {
    fprintf( file, "<%s\n", current_filename );
  }

  fprintf( file, "# End of playlist\n" );
  fclose ( file );
  return TRUE;
}

/* Saves the playlist and player status to the specified file. */
BOOL
pl_save_bundle( const char* filename, int options ) {
  return BOOLFROMMR( WinSendMsg( playlist, PL_MSG_SAVE_BUNDLE,
                                 MPFROMP( filename ), MPFROMLONG( options )));
}

/* Loads the playlist and player status from specified file. */
BOOL
pl_load_bundle( const char *filename, int options )
{
  char  file[_MAX_PATH];
  BOOL  select   = FALSE;
  BOOL  loaded   = FALSE;
  FILE* playlist = fopen( filename, "r" );
  ULONG ordinal  = 0;

  if( !playlist ) {
    amp_error( container, "Unable open status file:\n%s\n%s", filename, strerror( errno ));
    return FALSE;
  }

  pl_clear();

  while( fgets( file, sizeof(file), playlist ))
  {
    blank_strip( file );

    if( *file == '<' ) {
      struct stat fi;
      if( stat( file + 1, &fi ) == 0 ) {
        amp_load_singlefile( file + 1, AMP_LOAD_NOT_PLAY | AMP_LOAD_NOT_RECALL );
      }
    } else if( *file == '>' ) {
      sscanf( file, ">%lu,%lu\n", &select, &loaded );
    } else if( *file != 0 && *file != '#' ) {
      ordinal++;
      pl_add_file( file, NULL, 0 );

      if( select ) {
        select = FALSE;
        qu_write( broker_queue, PL_SELECT_BY_INDEX,
                  pl_m_create_request_data( NULL, NULL, ordinal, 0 ));
      }
      if( loaded ) {
        loaded = FALSE;
        qu_write( broker_queue, PL_RELOAD_BY_INDEX,
                  pl_m_create_request_data( NULL, NULL, ordinal, 0 ));
      }
    }
  }
  fclose( playlist );
  return TRUE;
}

