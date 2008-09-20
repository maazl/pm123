/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
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

#define  INCL_WIN
#define  INCL_DOS
#define  INCL_GPI
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <errno.h>

#include <utilfct.h>
#include <snprintf.h>
#include <debuglog.h>

#include "pm123.h"
#include "pfreq.h"
#include "docking.h"
#include "playlist.h"
#include "assertions.h"
#include "iniman.h"

static HWND plman;
static HWND container;
static HWND pm_main_menu;
static HWND pm_list_menu;
static HWND pm_file_menu;

#define PM_TYPE_LIST   1
#define PM_TYPE_FILE   2

#define PM_MAX_BROKERS 4

typedef struct _PMRECORD
{
  RECORDCORE rc;

  int  type;
  PSZ  filename;
  int  bitrate;
  int  samplerate;
  int  mode;
  int  filesize;
  int  secs;
  int  childs;
  int  populated;
  BOOL removed;

} PMRECORD, *PPMRECORD;

typedef struct _PMDROPINFO {

  HWND   hwndItem;  /* Window handle of the source of the drag operation. */
  ULONG  ulItemID;  /* Information used by the source to identify the
                       object being dragged. */
} PMDROPINFO;

typedef struct _INFO
{
  PMRECORD* parent;
  PMRECORD* pos;

  int  type;
  int  bitrate;
  int  samplerate;
  int  mode;
  int  filesize;
  int  secs;

} INFO, *PINFO;


#define PM_BEGIN_POPULATE ( WM_USER + 1000 )
#define PM_END_POPULATE   ( WM_USER + 1001 )
#define PM_ADD_RECORD     ( WM_USER + 1002 )

#define PM_POPULATE   0
#define PM_TERMINATE  1

static  BOOL pm_m_remove_playlist( HWND hwnd, PMRECORD* rec );

static  TID    broker_tids[ PM_MAX_BROKERS ];
static  PQUEUE broker_queue = NULL;

/* WARNING!! All functions returning a pointer to the
   manager record, return a NULL if suitable record is not found. */

INLINE PMRECORD*
PM_RC( MRESULT rc )
{
  if((PMRECORD*)rc != (PMRECORD*)-1 ) {
    return (PMRECORD*)rc;
  } else {
    return NULL;
  }
}

/* Returns the pointer to the first child record of specified. */
static PMRECORD*
pm_m_first_child_record( PMRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;
  return PM_RC( WinSendMsg( container, CM_QUERYRECORD, MPFROMP( rec ),
                            MPFROM2SHORT( CMA_FIRSTCHILD, CMA_ITEMORDER )));
}

/* Returns the pointer to the first playlist manager record. */
static PMRECORD*
pm_m_first_record( void )
{
  ASSERT_IS_MAIN_THREAD;
  return PM_RC( WinSendMsg( container, CM_QUERYRECORD, NULL,
                            MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER )));
}

/* Returns the pointer to the next playlist manager record of specified. */
static PMRECORD*
pm_m_next_record( PMRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;
  return PM_RC( WinSendMsg( container, CM_QUERYRECORD, MPFROMP(rec),
                            MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER )));
}

/* Returns the pointer to the selected playlist manager record. */
static PMRECORD*
pm_m_selected( void )
{
  ASSERT_IS_MAIN_THREAD;
  return PM_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_SELECTED )));
}

/* Returns the pointer to the record of the specified playlist. */
static PMRECORD*
pm_m_playlist_record( const char* filename )
{
  PMRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  for( rec = pm_m_first_record(); rec; rec = pm_m_next_record( rec )) {
    if( stricmp( filename, rec->filename ) == 0 ) {
      break;
    }
  }
  return rec;
}

/* Frees the data contained in the playlist manager record. */
static void
pm_m_free_record( PMRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;

  free( rec->rc.pszTree );
  free( rec->filename   );
}

/* Removes the specified playlist manager record. */
static void
pm_m_remove_record( PMRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;

  if( rec->removed
      || WinSendMsg( container, CM_REMOVERECORD,
                     MPFROMP( &rec ), MPFROM2SHORT( 1, 0 )) != MRFROMLONG( -1 ))
  {
    if( !rec->populated ) {
      pm_m_free_record( rec );
      WinSendMsg( container, CM_FREERECORD, MPFROMP( &rec ), MPFROMSHORT( 1 ));
    } else {
      rec->removed = TRUE;
    }
  }
}

static void
pm_m_destroy_children_and_self( PMRECORD* rec )
{
  PMRECORD* child;
  ASSERT_IS_MAIN_THREAD;

  while(( child = pm_m_first_child_record( rec )) != NULL ) {
    pm_m_remove_record( child );
  }

  pm_m_remove_record( rec );
}

/* Returns the number of records in the playlist manager. */
static ULONG
pm_m_size( void )
{
  CNRINFO info;
  ASSERT_IS_MAIN_THREAD;

  if( WinSendMsg( container, CM_QUERYCNRINFO,
                  MPFROMP(&info), MPFROMLONG(sizeof(info))) != 0 )
  {
    return info.cRecords;
  } else {
    return 0;
  }
}

/* Sets playlist manager title. */
static void
pm_m_set_title( const char* title )
{
  static CNRINFO info = { 0 };
  ASSERT_IS_MAIN_THREAD;

  if( info.pszCnrTitle != NULL ) {
    free( info.pszCnrTitle );
  }

  if( title ) {
    info.pszCnrTitle = strdup( title );
    WinSendMsg( container, CM_SETCNRINFO, MPFROMP(&info), MPFROMLONG( CMA_CNRTITLE ));
  } else {
    info.pszCnrTitle = NULL;
  }
}

/* Prints formatted technical data to buffer. */
static char*
pm_m_sprint_info( char* result, PMRECORD* rec, int size )
{
  char digits[64];
  ASSERT_IS_MAIN_THREAD;

  strlcpy( result, rec->filename, size );

  if( rec->type == PM_TYPE_FILE ) {
    if( rec->secs != -1 ) {
      strlcat( result, ", ", size );
      strlcat( result, itoa( rec->secs / 60, digits, 10 ), size );
      strlcat( result, ":", size );
      strlcat( result, itoa( rec->secs % 60, digits, 10 ), size );
    }
    if( rec->filesize != -1 ) {
      strlcat( result, ", ", size );
      strlcat( result, itoa( rec->filesize / 1024, digits, 10 ), size );
      strlcat( result, " kB", size );
    }
    if( rec->bitrate != -1 ) {
      strlcat( result, ", ", size );
      strlcat( result, itoa( rec->bitrate, digits, 10 ), size );
      strlcat( result, " kb/s", size );
    }
    if( rec->samplerate != -1 ) {
      strlcat( result, ", ", size );
      strlcat( result, itoa( rec->samplerate / 1000, digits, 10 ), size );
      strlcat( result, "." , size );
      strlcat( result, itoa( rec->samplerate % 1000 / 100, digits, 10 ), size );
      strlcat( result, " kHz", size );
    }
    if( rec->mode != -1 ) {
      strlcat( result, ", ", size );
      strlcat( result, modes( rec->mode ), size );
    }
  } else {
    strlcat( result, ", total ", size );
    strlcat( result, itoa( rec->childs, digits, 10 ), size );
    strlcat( result, " files",  size );

    if( rec->filesize > 0 ) {
      strlcat( result, ", ", size );
      strlcat( result, itoa( rec->filesize / 1024, digits, 10 ), size );
      strlcat( result, " kB", size );
    }
    if( rec->secs > 0 ) {
      strlcat( result, ", ", size );
      strlcat( result, itoa( rec->secs / 86400, digits, 10 ), size );
      strlcat( result, "d ", size );
      strlcat( result, itoa( rec->secs % 86400 / 3600, digits, 10 ), size );
      strlcat( result, "h ", size );
      strlcat( result, itoa( rec->secs % 3600 / 60, digits, 10 ), size );
      strlcat( result, "m ", size );
      strlcat( result, itoa( rec->secs % 60, digits, 10 ), size );
      strlcat( result, "s", size );
    }
  }
  return result;
}

/* Begins population of the specified playlist. */
static PMRECORD*
pm_m_begin_populate( const char* filename )
{
  PMRECORD* rec;
  ASSERT_IS_MAIN_THREAD;

  for( rec = pm_m_first_record(); rec; rec = pm_m_next_record( rec )) {
    if( stricmp( filename, rec->filename ) == 0 ) {
      break;
    }
  }
  if( rec ) {
    ++rec->populated;
  }
  return rec;
}

/* Ends population of the specified playlist. */
static void
pm_m_end_populate( PMRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;

  if( rec ) {
    if( !--rec->populated && rec->removed ) {
      pm_m_destroy_children_and_self( rec );
    } else {
      if( rec == pm_m_selected()) {
        char title[1024];
        pm_m_set_title( pm_m_sprint_info( title, rec, sizeof( title )));
      }
    }
  }
}

/* Creates the manager record for a specified file. */
PMRECORD*
pm_m_create_record( const char* filename, const INFO* info )
{
  PMRECORD* rec;
  RECORDINSERT insert;

  ASSERT_IS_MAIN_THREAD;

  if( info->type == PM_TYPE_FILE && info->parent && info->parent->removed ) {
    return NULL;
  }

  rec = (PMRECORD*)WinSendMsg( container, CM_ALLOCRECORD,
                               MPFROMLONG( sizeof( PMRECORD ) - sizeof( RECORDCORE )),
                               MPFROMLONG( 1 ));
  if( rec )
  {
    char title[_MAX_PATH];

    if( info->type == PM_TYPE_LIST ) {
      sdname( title, filename, sizeof( title ));
    } else {
      sdnameext( title, filename, sizeof( title ));
    }

    rec->rc.cb            = sizeof( RECORDCORE );
    rec->rc.flRecordAttr  = 0;
    rec->rc.pszTree       = strdup( title );
    rec->type             = info->type;
    rec->filename         = strdup( filename );
    rec->bitrate          = info->bitrate;
    rec->samplerate       = info->samplerate;
    rec->mode             = info->mode;
    rec->filesize         = info->filesize;
    rec->secs             = info->secs;
    rec->childs           = 0;
    rec->populated        = 0;
    rec->removed          = FALSE;

    insert.cb                = sizeof(RECORDINSERT);
    insert.pRecordOrder      = (PRECORDCORE)info->pos;
    insert.pRecordParent     = (PRECORDCORE)info->parent;
    insert.fInvalidateRecord = TRUE;
    insert.zOrder            = CMA_TOP;
    insert.cRecordsInsert    = 1;

    if( WinSendMsg( container, CM_INSERTRECORD,
                    MPFROMP( rec ), MPFROMP( &insert )) != 0 )
    {
      if( info->parent ) {
        if( info->filesize != -1 ) {
          info->parent->filesize += info->filesize;
        }
        if( info->secs != -1 ) {
          info->parent->secs += info->secs;
        }
        ++info->parent->childs;
      }
    } else {
      pm_m_free_record( rec );
      WinSendMsg( container, CM_FREERECORD, MPFROMP( &rec ), MPFROMSHORT( 1 ));
      rec = NULL;
    }
  }
  return rec;
}

/* Moves the playlist manager record to the specified position. */
static PMRECORD*
pm_m_move_record( PMRECORD* rec, PMRECORD* pos )
{
  RECORDINSERT insert;
  PMRECORD*    copy;
  PMRECORD*    child;

  ASSERT_IS_MAIN_THREAD;

  if( rec->type != PM_TYPE_LIST ) {
    return NULL;
  }

  copy = (PMRECORD*)WinSendMsg( container, CM_ALLOCRECORD,
                                MPFROMLONG( sizeof( PMRECORD ) - sizeof( RECORDCORE )),
                                MPFROMLONG( 1 ));
  if( !copy ) {
    return NULL;
  }

  memset( copy, 0, sizeof( *copy ));

  copy->rc.cb      = sizeof( RECORDCORE );
  copy->rc.pszTree = strdup( rec->rc.pszTree );
  copy->type       = rec->type;
  copy->filename   = strdup( rec->filename );

  insert.cb                = sizeof(RECORDINSERT);
  insert.pRecordOrder      = (PRECORDCORE)pos;
  insert.pRecordParent     = NULL;
  insert.fInvalidateRecord = FALSE;
  insert.zOrder            = CMA_TOP;
  insert.cRecordsInsert    = 1;

  if( WinSendMsg( container, CM_INSERTRECORD,
                  MPFROMP( copy ), MPFROMP( &insert )) == 0 )
  {
    pm_m_free_record( copy );
    WinSendMsg( container, CM_FREERECORD, MPFROMP( &copy ), MPFROMSHORT( 1 ));
    return NULL;
  }

  for( child = pm_m_first_child_record( rec ); child; child = pm_m_next_record( child ))
  {
    INFO info;

    info.parent     = copy;
    info.pos        = (PMRECORD*)CMA_END;;
    info.type       = PM_TYPE_FILE;
    info.bitrate    = child->bitrate;
    info.samplerate = child->samplerate;
    info.mode       = child->mode;
    info.filesize   = child->filesize;
    info.secs       = child->secs;

    pm_m_create_record( child->filename, &info );
  }

  pm_m_destroy_children_and_self( rec );
  WinPostMsg( container, CM_INVALIDATERECORD,
              MPFROMP( NULL ), MPFROM2SHORT( 0, 0 ));

  return copy;
}

static void TFNENTRY
pm_broker( void* dummy )
{
  ULONG   request;
  PVOID   reqdata;
  HAB     hab = WinInitialize( 0 );
  HMQ     hmq = WinCreateMsgQueue( hab, 0 );

  while( qu_read( broker_queue, &request, &reqdata ))
  {
    char* filename = (char*)reqdata;


    switch( request ) {
      case PM_POPULATE:
        if( filename ) {
          DEBUGLOG(( "pfreq: begin populate %s\n", filename ));
          pl_load( filename, PL_LOAD_TO_PM | PL_LOAD_NOT_QUEUE );
        }
        break;

      case PM_TERMINATE:
        break;
    }

    free( filename );

    if( request == PM_TERMINATE ) {
      break;
    }
  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
  _endthread();
}

static void TFNENTRY
pm_m_manager_populate( void* dummy )
{
  HAB hab = WinInitialize( 0 );
  HMQ hmq = WinCreateMsgQueue( hab, 0 );

  char  filename[_MAX_PATH];
  char  playlist[_MAX_PATH];
  FILE* file;
  INFO  info = { NULL, (PMRECORD*)CMA_END, PM_TYPE_LIST };

  sprintf( filename, "%spm123.mgr", startpath );
  file = fopen( filename, "r" );

  if( file ) {
    while( fgets( playlist, sizeof(playlist), file ))
    {
      blank_strip( playlist );
      if( *playlist && *playlist != '#' && *playlist != '>' && *playlist != '<' ) {
        if( WinSendMsg( plman, PM_ADD_RECORD, MPFROMP( &info ), playlist )) {
          qu_write( broker_queue, PM_POPULATE, strdup( playlist ));
        }
      }
    }
    fclose( file );
  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
}

/* Saves playlist manager data to the file. */
static BOOL
pm_m_save( HWND owner )
{
  FILE*     hfile;
  PMRECORD* rec;

  char lstfile[_MAX_PATH ];
  char bakfile[_MAX_PATH ];

  ASSERT_IS_MAIN_THREAD;

  strcpy( lstfile, startpath   );
  strcat( lstfile, "pm123.mgr" );
  strcpy( bakfile, startpath   );
  strcat( bakfile, "pm123.mg~" );

  if( remove( bakfile ) != 0 && errno != ENOENT ) {
    amp_error( owner, "Unable delete backup file:\n%s\n%s",
               bakfile, strerror(errno));
    return FALSE;
  }

  if( rename( lstfile, bakfile ) != 0 && errno != ENOENT ) {
    amp_error( owner, "Unable backup playlist manager file:\n%s\n%s",
               lstfile, strerror(errno));
    return FALSE;
  }

  hfile = fopen( lstfile, "w" );
  if( hfile == NULL ) {
    amp_error( owner, "Unable create playlist manager file:\n%s\n%s",
               lstfile, strerror(errno));
    return FALSE;
  }

  for( rec = pm_m_first_record(); rec; rec = pm_m_next_record( rec )) {
    fprintf( hfile, "%s\n", rec->filename );
  }

  if( fclose( hfile ) == 0 ) {
    remove( bakfile );
    return TRUE;
  } else {
    return FALSE;
  }
}

static void
pm_m_load_record( PMRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;

  if( rec ) {
    if( rec->type == PM_TYPE_LIST ) {
      pl_load( rec->filename, PL_LOAD_CLEAR );
    } else if( rec->type == PM_TYPE_FILE ) {
      amp_load_singlefile( rec->filename, 0 );
    }
  }
}

/* Prepares the playlist manager container item to the drag operation. */
static void
pm_m_drag_init_item( HWND hwnd, PMRECORD* rec,
                     PDRAGINFO drag_infos, PDRAGIMAGE drag_image, int i )
{
  DRAGITEM ditem = { 0 };
  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];

  ASSERT_IS_MAIN_THREAD;

  sdrivedir( pathname, rec->filename, sizeof( pathname ));
  sfnameext( filename, rec->filename, sizeof( filename ));

  ditem.hwndItem          = hwnd;
  ditem.ulItemID          = (ULONG)rec;
  ditem.hstrType          = DrgAddStrHandle( DRT_BINDATA );
  ditem.hstrContainerName = DrgAddStrHandle( pathname );
  ditem.hstrSourceName    = DrgAddStrHandle( filename );
  ditem.hstrTargetName    = DrgAddStrHandle( filename );

  if( rec->type == PM_TYPE_LIST ) {
    ditem.fsSupportedOps  = DO_MOVEABLE | DO_COPYABLE | DO_LINKABLE;
    ditem.hstrRMF         = DrgAddStrHandle( "(DRM_123FILE,DRM_DISCARD)x(DRF_UNKNOWN)" );
  } else {
    ditem.fsSupportedOps  = DO_COPYABLE | DO_LINKABLE;
    ditem.hstrRMF         = DrgAddStrHandle( "(DRM_123FILE)x(DRF_UNKNOWN)" );
  }

  DrgSetDragitem( drag_infos, &ditem, sizeof( DRAGITEM ), i );

  drag_image->cb       = sizeof( DRAGIMAGE );
  drag_image->hImage   = mp3;
  drag_image->fl       = DRG_ICON | DRG_MINIBITMAP;
  drag_image->cxOffset = i < 5 ? 5 * i : 25;
  drag_image->cyOffset = i < 5 ? 5 * i : 25;

  WinSendDlgItemMsg( hwnd, CNR_PM, CM_SETRECORDEMPHASIS,
                     MPFROMP( rec ), MPFROM2SHORT( TRUE, CRA_SOURCE ));
}

/* Prepares the playlist manager container to the drag operation. */
static MRESULT
pm_m_drag_init( HWND hwnd, PCNRDRAGINIT pcdi )
{
  PMRECORD*  rec;
  PDRAGIMAGE drag_images   = NULL;
  PDRAGINFO  drag_infos    = NULL;

  ASSERT_IS_MAIN_THREAD;

  // If the record under the mouse is NULL, we must be over whitespace,
  // in which case we don't want to drag any records.

  if( !(PMRECORD*)pcdi->pRecord ) {
    return 0;
  }

  // Allocate an array of DRAGIMAGE structures. Each structure contains
  // info about an image that will be under the mouse pointer during the
  // drag. This image will represent a container record being dragged.

  drag_images = (PDRAGIMAGE)malloc( sizeof(DRAGIMAGE));

  if( !drag_images ) {
    return 0;
  }

  // Let PM allocate enough memory for a DRAGINFO structure as well as
  // a DRAGITEM structure for each record being dragged. It will allocate
  // shared memory so other processes can participate in the drag/drop.

  drag_infos = DrgAllocDraginfo( 1 );

  if( !drag_infos ) {
    return 0;
  }

  pm_m_drag_init_item( hwnd, (PMRECORD*)pcdi->pRecord,
                       drag_infos, drag_images, 0 );

  // If DrgDrag returns NULLHANDLE, that means the user hit Esc or F1
  // while the drag was going on so the target didn't have a chance to
  // delete the string handles. So it is up to the source window to do
  // it. Unfortunately there doesn't seem to be a way to determine
  // whether the NULLHANDLE means Esc was pressed as opposed to there
  // being an error in the drag operation. So we don't attempt to figure
  // that out. To us, a NULLHANDLE means Esc was pressed...

  if( !DrgDrag( hwnd, drag_infos, drag_images, 1, VK_ENDDRAG, NULL )) {
    DrgDeleteDraginfoStrHandles( drag_infos );
  }

  rec = (PMRECORD*)CMA_FIRST;
  while( rec ) {
    rec = (PMRECORD*)WinSendDlgItemMsg( hwnd, CNR_PM, CM_QUERYRECORDEMPHASIS,
                                        MPFROMP( rec ), MPFROMSHORT( CRA_SOURCE ));

    if( rec == (PMRECORD*)(-1)) {
      break;
    } else if( rec ) {
      WinSendDlgItemMsg( hwnd, CNR_PM, CM_SETRECORDEMPHASIS,
                         MPFROMP( rec ), MPFROM2SHORT( FALSE, CRA_SOURCE ));
      WinSendDlgItemMsg( hwnd, CNR_PM, CM_INVALIDATERECORD,
                         MPFROMP( &rec ), MPFROM2SHORT( 1, 0 ));
    }
  }

  free( drag_images );
  DrgFreeDraginfo( drag_infos );
  return 0;
}

/* Prepares the playlist manager container to the drop operation. */
static MRESULT
pm_m_drag_over( HWND hwnd, PCNRDRAGINFO pcdi )
{
  PDRAGINFO pdinfo  = pcdi->pDragInfo;
  PDRAGITEM pditem;
  PMRECORD* rec     = (PMRECORD*)pcdi->pRecord;
  int       i;
  USHORT    drag_op = DO_UNKNOWN;
  USHORT    drag    = DOR_NEVERDROP;

  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];
  char fullname[_MAX_PATH];

  ASSERT_IS_MAIN_THREAD;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROM2SHORT( DOR_NEVERDROP, DO_UNKNOWN );
  }

  if( rec && rec->type != PM_TYPE_LIST ) {
    DrgFreeDraginfo( pdinfo );
    return MPFROM2SHORT( DOR_NODROP, DO_UNKNOWN );
  }

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    DrgQueryStrName( pditem->hstrContainerName, sizeof( pathname ), pathname );
    DrgQueryStrName( pditem->hstrSourceName,    sizeof( filename ), filename );
    strlcpy( fullname, pathname, sizeof( fullname ));
    strlcat( fullname, filename, sizeof( fullname ));

    if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL )) {
      if( !is_playlist( filename )) {
        drag    = DOR_NEVERDROP;
        drag_op = DO_UNKNOWN;
        break;
      }
      else if( pdinfo->hwndSource == hwnd ) {
        if( pdinfo->usOperation == DO_DEFAULT || pdinfo->usOperation == DO_MOVE )
        {
          drag    = DOR_DROP;
          drag_op = DO_MOVE;
        } else {
          drag    = DOR_NEVERDROP;
          drag_op = DO_UNKNOWN;
          break;
        }
      }
      else if( pm_m_playlist_record( fullname ))
      {
        drag    = DOR_NEVERDROP;
        drag_op = DO_UNKNOWN;
        break;
      }
      else if( pdinfo->usOperation == DO_DEFAULT )
      {
        drag    = DOR_DROP;
        drag_op = DO_COPY;
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
      if( !DrgVerifyType( pditem, "UniformResourceLocator" ) &&
        ( !is_playlist( filename ) || pm_m_playlist_record( fullname )))
      {
        drag    = DOR_NEVERDROP;
        drag_op = DO_UNKNOWN;
        break;
      }
      else if( pdinfo->usOperation == DO_DEFAULT &&
               pditem->fsSupportedOps & DO_COPYABLE )
      {
        drag    = DOR_DROP;
        drag_op = DO_COPY;
      }
      else if(( pdinfo->usOperation == DO_LINK && pditem->fsSupportedOps & DO_LINKABLE ) ||
              ( pdinfo->usOperation == DO_COPY && pditem->fsSupportedOps & DO_COPYABLE ))
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

/* Discards playlist manager records dropped into shredder.
   Must be called from the main thread. */
static MRESULT
pm_m_drag_discard( HWND hwnd, PDRAGINFO pdinfo )
{
  PDRAGITEM  pditem;
  int        i;

  ASSERT_IS_MAIN_THREAD;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROMLONG( DRR_ABORT );
  }

  // We get as many DM_DISCARDOBJECT messages as there are
  // records dragged but the first one has enough info to
  // process all of them.

  for( i = 0; i < pdinfo->cditem; i++ ) {
    pditem = DrgQueryDragitemPtr( pdinfo, i );
    pm_m_destroy_children_and_self((PMRECORD*)pditem->ulItemID );
  }

  pm_m_save ( hwnd );
  WinPostMsg( container, CM_INVALIDATERECORD,
              MPFROMP( NULL ), MPFROM2SHORT( 0, 0 ));

  DrgFreeDraginfo( pdinfo );
  return MRFROMLONG( DRR_SOURCE );
}

/* Receives the dropped playlist manager records. */
static MRESULT
pm_m_drag_drop( HWND hwnd, PCNRDRAGINFO pcdi )
{
  PDRAGINFO pdinfo = pcdi->pDragInfo;
  PDRAGITEM pditem;

  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];
  char fullname[_MAX_PATH];
  int  i;

  PMRECORD* pos = pcdi->pRecord ? (PMRECORD*)pcdi->pRecord : (PMRECORD*)CMA_END;
  ASSERT_IS_MAIN_THREAD;

  if( !DrgAccessDraginfo( pdinfo )) {
    return 0;
  }

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );

    DrgQueryStrName( pditem->hstrSourceName,    sizeof( filename ), filename );
    DrgQueryStrName( pditem->hstrContainerName, sizeof( pathname ), pathname );
    strlcpy( fullname, pathname, sizeof( fullname ));
    strlcat( fullname, filename, sizeof( fullname ));

    if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL ))
    {
      PMRECORD* rec = (PMRECORD*)pditem->ulItemID;
      PMRECORD* ins;

      if( pdinfo->hwndSource == hwnd ) {
        if( pdinfo->usOperation == DO_MOVE ) {
          ins = pm_m_move_record( rec, pos );
          if( ins ) {
            pos = ins;
          }
        }
      } else {
        if( !pm_m_playlist_record( fullname ))
        {
          INFO info = { 0 };

          info.type = PM_TYPE_LIST;
          info.pos  = pos;

          ins = pm_m_create_record( fullname, &info );
          qu_write( broker_queue, PM_POPULATE, strdup( fullname ));

          if( ins ) {
            pos = ins;
          }
          if( pdinfo->usOperation == DO_MOVE ) {
            WinSendMsg( pdinfo->hwndSource, WM_123FILE_REMOVE, MPFROMP( rec ), 0 );
          }
        }
      }
    }
    else if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL ))
    {
      PMRECORD* ins;

      if( pditem->hstrContainerName && pditem->hstrSourceName ) {
        // Have full qualified file name.
        if( DrgVerifyType( pditem, "UniformResourceLocator" )) {
          amp_url_from_file( fullname, fullname, sizeof( fullname ));
        }
        if( is_playlist( fullname ) && !pm_m_playlist_record( fullname ))
        {
          INFO info = { 0 };

          info.type = PM_TYPE_LIST;
          info.pos  = pos;

          ins = pm_m_create_record( fullname, &info );
          qu_write( broker_queue, PM_POPULATE, strdup( fullname ));

          if( ins ) {
            pos = ins;
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
        PMDROPINFO* pdsource = (PMDROPINFO*)malloc( sizeof( PMDROPINFO ));
        char renderto[_MAX_PATH];

        if( !pdtrans || !pdsource ) {
          return 0;
        }

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
  }

  pm_m_save( hwnd );
  DrgDeleteDraginfoStrHandles( pdinfo );
  DrgFreeDraginfo( pdinfo );
  return 0;
}

/* Receives dropped and rendered files and urls. */
static MRESULT
pm_m_drag_render_done( HWND hwnd, PDRAGTRANSFER pdtrans, USHORT rc )
{
  char rendered[_MAX_PATH];
  char fullname[_MAX_PATH];

  PMDROPINFO* pdsource = (PMDROPINFO*)pdtrans->ulTargetInfo;
  ASSERT_IS_MAIN_THREAD;

  // If the rendering was successful, use the file, then delete it.
  if(( rc & DMFL_RENDEROK ) && pdsource &&
       DrgQueryStrName( pdtrans->hstrRenderToName, sizeof( rendered ), rendered ))
  {

    amp_url_from_file( fullname, rendered, sizeof( fullname ));
    DosDelete( rendered );

    if( is_playlist( fullname ) && !pm_m_playlist_record( fullname ))
    {
      INFO info = { 0 };

      info.type = PM_TYPE_LIST;
      info.pos  = (PMRECORD*)CMA_END;

      pm_m_create_record( fullname, &info );
      qu_write( broker_queue, PM_POPULATE, strdup( fullname ));
      pm_m_save( hwnd );
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


static void
pm_m_add_playlist( HWND hwnd )
{
  FILEDLG filedialog = { 0 };

  int   i       = 0;
  APSZ  types[] = {{ FDT_PLAYLIST }, { 0 }};
  char* file;

  ASSERT_IS_MAIN_THREAD;

  filedialog.cbSize         = sizeof( FILEDLG );
  filedialog.fl             = FDS_CENTER | FDS_OPEN_DIALOG | FDS_CUSTOM;
  filedialog.pszTitle       = "Add Playlist(s)";
  filedialog.hMod           = hmodule;
  filedialog.usDlgId        = DLG_FILE;
  filedialog.pfnDlgProc     = amp_file_dlg_proc;
  filedialog.papszITypeList = types;
  filedialog.pszIType       = FDT_PLAYLIST;

  strcpy( filedialog.szFullFile, cfg.listdir );
  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK ) {
    if( filedialog.ulFQFCount > 1 ) {
      file = (*filedialog.papszFQFilename)[i];
    } else {
      file = filedialog.szFullFile;
    }

    while( *file ) {
      sdrivedir( cfg.listdir, file, sizeof( cfg.listdir ));
      if( !pm_m_playlist_record( file ))
      {
       INFO info = { 0 };

       info.type = PM_TYPE_LIST;
       info.pos  = (PMRECORD*)CMA_END;

        pm_m_create_record( file, &info );
        qu_write( broker_queue, PM_POPULATE, strdup( file ));
      }

      if( ++i >= filedialog.ulFQFCount ) {
        break;
      } else {
        file = (*filedialog.papszFQFilename)[i];
      }
    }
    pm_m_save( hwnd );
  }

  WinFreeFileDlgList( filedialog.papszFQFilename );
}

static BOOL
pm_m_remove_playlist( HWND hwnd, PMRECORD* rec )
{
  ASSERT_IS_MAIN_THREAD;

  if( !rec || rec->type != PM_TYPE_LIST ) {
    return FALSE;
  }

  pm_m_destroy_children_and_self( rec );
  pm_m_save( hwnd );

  WinPostMsg( container, CM_INVALIDATERECORD,
              MPFROMP( NULL ), MPFROM2SHORT( 0, 0 ));

  if( !pm_m_size()) {
    pm_m_set_title( "No playlist selected. Right click for menu." );
  }
  return TRUE;
}

/* Shows the context menu of the playlist manager. */
static void
pm_m_show_context_menu( HWND parent, PMRECORD* rec )
{
  HWND   menu;
  POINTL pos;
  SWP    swp;

  ASSERT_IS_MAIN_THREAD;

  if( rec ) {
    if( rec->type == PM_TYPE_LIST ) {
      menu = pm_list_menu;
    } else {
      menu = pm_file_menu;
    }

    WinSendMsg( container, CM_SETRECORDEMPHASIS,
                MPFROMP( rec ), MPFROM2SHORT( TRUE, CRA_SELECTED | CRA_CURSORED ));
  } else {
    menu = pm_main_menu;
  }

  WinQueryPointerPos( HWND_DESKTOP, &pos );
  WinMapWindowPoints( HWND_DESKTOP, parent, &pos, 1 );

  if( WinWindowFromPoint( parent, &pos, TRUE ) == NULLHANDLE )
  {
    // The context menu is probably activated from the keyboard.
    WinQueryWindowPos( parent, &swp );
    pos.x = swp.cx/2;
    pos.y = swp.cy/2;
  }

  WinPopupMenu( parent, parent, menu, pos.x, pos.y, 0,
                PU_HCONSTRAIN   | PU_VCONSTRAIN | PU_MOUSEBUTTON1 |
                PU_MOUSEBUTTON2 | PU_KEYBOARD   );
}

/* Initializes the playlist manager presentation window. */
static void
pm_m_init_window( HWND hwnd )
{
  CNRINFO  info;
  HPOINTER hicon;
  HACCEL   accel;
  HAB      hab = WinQueryAnchorBlock( hwnd );

  ASSERT_IS_MAIN_THREAD;
  container = WinWindowFromID( hwnd, CNR_PLAYLIST );

  info.flWindowAttr = CV_TREE | CV_TEXT | CA_TREELINE | CA_CONTAINERTITLE | CA_TITLELEFT |
                      CA_TITLESEPARATOR | CA_TITLEREADONLY;
  info.hbmExpanded  = NULLHANDLE;
  info.hbmCollapsed = NULLHANDLE;
  info.cxTreeIndent = 8;
  info.pszCnrTitle  = "No playlist selected. Right click for menu.";

  WinSendMsg( container, CM_SETCNRINFO, MPFROMP( &info ),
              MPFROMLONG( CMA_FLWINDOWATTR | CMA_CXTREEINDENT | CMA_CNRTITLE ));

  accel = WinLoadAccelTable( hab, hmodule, ACL_PM );

  if( accel ) {
    WinSetAccelTable( hab, accel, hwnd );
  }

  hicon = WinLoadPointer( HWND_DESKTOP, hmodule, ICO_MAIN );
  WinSendMsg( hwnd, WM_SETICON, (MPARAM)hicon, 0 );
  do_warpsans( hwnd );

  if( !rest_window_pos( hwnd, 0 )) {
    pm_set_colors( DEF_FG_COLOR, DEF_BG_COLOR, DEF_HI_FG_COLOR, DEF_HI_BG_COLOR );
  }

  dk_add_window( hwnd, 0 );
}

/* Processes messages of the playlist manager presentation window. */
static MRESULT EXPENTRY
pm_m_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_CONTROL:
      switch( SHORT2FROMMP( mp1 )) {
        case CN_HELP:
          amp_show_help( IDH_PM );
          return 0;

        case CN_EMPHASIS:
        {
          PNOTIFYRECORDEMPHASIS emphasis = (PNOTIFYRECORDEMPHASIS)mp2;
          PMRECORD* rec;
          char title[1024];

          if( emphasis != NULL ) {
            rec = (PMRECORD*)emphasis->pRecord;
            pm_m_set_title( pm_m_sprint_info( title, rec, sizeof( title )));
          }
          return 0;
        }
        case CN_CONTEXTMENU:
          pm_m_show_context_menu( hwnd, (PMRECORD*)mp2 );
          return 0;

        case CN_ENTER:
        {
          NOTIFYRECORDENTER* notify = (NOTIFYRECORDENTER*)mp2;
          pm_m_load_record((PMRECORD*)notify->pRecord );
          return 0;
        }

        case CN_INITDRAG:
          return pm_m_drag_init( hwnd, (PCNRDRAGINIT)mp2 );
        case CN_DRAGOVER:
          return pm_m_drag_over( hwnd, (PCNRDRAGINFO)mp2 );
        case CN_DROP:
          return pm_m_drag_drop( hwnd, (PCNRDRAGINFO)mp2 );
      }
      break;

    case DM_DISCARDOBJECT:
      return pm_m_drag_discard( hwnd, (PDRAGINFO)mp1 );
    case DM_RENDERCOMPLETE:
      return pm_m_drag_render_done( hwnd, (PDRAGTRANSFER)mp1, SHORT1FROMMP( mp2 ));

    case WM_123FILE_LOAD:
      pm_m_load_record( mp1 );
      return 0;

    case WM_123FILE_REMOVE:
      pm_m_remove_playlist( hwnd, mp1 );
      return 0;

    case WM_COMMAND:
      switch( COMMANDMSG(&msg)->cmd ) {
        case IDM_PM_CALC:
        {
          int  total_files = 0;
          int  total_secs  = 0;
          int  total_size  = 0;
          char title[1024];

          PMRECORD* rec;

          for( rec = pm_m_first_record(); rec; rec = pm_m_next_record( rec )) {
            total_files += rec->childs;
            total_secs  += rec->secs;
            total_size  += rec->filesize;
          }

          snprintf( title, sizeof(title), "Total %u files, %u kB, %ud %uh %um %us",
                    total_files, total_size / 1024, total_secs / 86400, total_secs % 86400 / 3600,
                    total_secs % 3600 / 60,  total_secs % 60 );

          pm_m_set_title( title );
          break;
        }

        case IDM_PM_L_DELETE:
        {
          PMRECORD* rec = pm_m_selected();

          if( rec && rec->type == PM_TYPE_LIST ) {
            if( amp_query( plman, "Do you want remove selected playlist from the manager "
                                  "and delete this files from your disk?" ))
            {
              if( pm_m_remove_playlist( hwnd, rec )) {
                remove( rec->filename );
              }
            }
          }
          return 0;
        }

        case IDM_PM_L_REMOVE:
          pm_m_remove_playlist( hwnd, pm_m_selected());
          return 0;

        case IDM_PM_L_LOAD:
        {
          PMRECORD* rec = pm_m_selected();

          if( rec && rec->type == PM_TYPE_LIST ) {
            pl_load( rec->filename, PL_LOAD_CLEAR );
          }
          return 0;
        }

        case IDM_PM_F_LOAD:
        {
          PMRECORD* rec = pm_m_selected();

          if( rec && rec->type == PM_TYPE_FILE ) {
            amp_load_singlefile( rec->filename, 0 );
          }
          return 0;
        }

        case IDM_PM_ADD:
          pm_m_add_playlist( hwnd );
          return 0;

        case IDM_PM_MENU:
          WinSendMsg( hwnd, WM_CONTROL,
                      MPFROM2SHORT( CNR_PLAYLIST, CN_CONTEXTMENU ),
                      PVOIDFROMMP( NULL ));
          return 0;

        case IDM_PM_MENU_RECORD:
          WinSendMsg( hwnd, WM_CONTROL,
                      MPFROM2SHORT( CNR_PLAYLIST, CN_CONTEXTMENU ),
                      PVOIDFROMMP( pm_m_selected()));
          return 0;
      }
      return 0;

    case WM_INITDLG:
      pm_m_init_window( hwnd );
      return 0;

    case WM_SYSCOMMAND:
      if( SHORT1FROMMP(mp1) == SC_CLOSE ) {
        pm_show( FALSE );
        return 0;
      }
      break;

    case WM_WINDOWPOSCHANGED:
    {
      SWP* pswp = PVOIDFROMMP(mp1);

      if( pswp[0].fl & SWP_SHOW ) {
        cfg.show_plman = TRUE;
      }
      if( pswp[0].fl & SWP_HIDE ) {
        cfg.show_plman = FALSE;
      }
      break;
    }

    case PM_BEGIN_POPULATE:
      return pm_m_begin_populate( mp1 );
    case PM_END_POPULATE:
      pm_m_end_populate( mp1 );
      return 0;

    case PM_ADD_RECORD: {
      return pm_m_create_record( mp2, mp1 );
    }
 }

 return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Begins population of the specified playlist. */
HPOPULATE
pm_begin_populate( const char* filename ) {
  return WinSendMsg( plman, PM_BEGIN_POPULATE, MPFROMP( filename ), 0 );
}

/* Ends population of the specified playlist. */
void
pm_end_populate( HPOPULATE handle ) {
  WinSendMsg( plman, PM_END_POPULATE, handle, 0 );
}

/* Adds the specified file to the populated playlist. */
BOOL
pm_add_file( HPOPULATE handle, const char* filename,
             int bitrate, int samplerate, int mode, int filesize, int secs )
{
  INFO info;

  info.type       = PM_TYPE_FILE;
  info.parent     = handle;
  info.pos        = (PMRECORD*)CMA_END;
  info.bitrate    = bitrate;
  info.samplerate = samplerate;
  info.mode       = mode;
  info.filesize   = filesize;
  info.secs       = secs;

  if( WinSendMsg( plman, PM_ADD_RECORD, MPFROMP( &info ), MPFROMP( filename )) != NULL ) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Sets the visibility state of the playlist manager presentation window. */
void
pm_show( BOOL show )
{
  HSWITCH hswitch = WinQuerySwitchHandle( plman, 0 );
  SWCNTRL swcntrl;

  if( WinQuerySwitchEntry( hswitch, &swcntrl ) == 0 ) {
    swcntrl.uchVisibility = show ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry( hswitch, &swcntrl );
  }

  dk_set_state( plman, show ? 0 : DK_IS_GHOST );
  WinSetWindowPos( plman, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );
}

/* Changes the playlist manager colors. */
BOOL
pm_set_colors( ULONG fgcolor, ULONG bgcolor, ULONG hi_fgcolor, ULONG hi_bgcolor )
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

/* Creates the playlist manager presentation window.
   Must be called from the main thread. */
HWND
pm_create( void )
{
  int i;
  ASSERT_IS_MAIN_THREAD;

  plman = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
                      pm_m_dlg_proc, hmodule, DLG_PM, NULL );

  pm_list_menu = WinLoadMenu( HWND_OBJECT, hmodule, MNU_LIST    );
  pm_file_menu = WinLoadMenu( HWND_OBJECT, hmodule, MNU_FILE    );
  pm_main_menu = WinLoadMenu( HWND_OBJECT, hmodule, MNU_MANAGER );


  broker_queue = qu_create();

  if( !broker_queue ) {
    amp_player_error( "Unable create playlist manager service queue." );
  } else {
    for( i = 0; i < PM_MAX_BROKERS; i++ ) {
      if(( broker_tids[i] = _beginthread( pm_broker, NULL, 65535, NULL )) == -1 ) {
        amp_error( container, "Unable create the playlist manager service thread." );
      }
    }
  }

  _beginthread( pm_m_manager_populate, NULL, 65535, NULL );
  pm_show( cfg.show_plman );
  return plman;
}

/* Destroys the playlist manager presentation window.
   Must be called from the main thread. */
void
pm_destroy( void )
{
  HAB    hab   = WinQueryAnchorBlock( plman );
  HACCEL accel = WinQueryAccelTable ( hab, plman );
  int    i;

  PMRECORD* rec;

  ASSERT_IS_MAIN_THREAD;
  save_window_pos( plman, 0 );

  for( i = 0; i < PM_MAX_BROKERS; i++ ) {
    qu_push( broker_queue, PM_TERMINATE, NULL );
  }
  for( i = 0; i < PM_MAX_BROKERS; i++ ) {
    if( broker_tids[i] ) {
      wait_thread( broker_tids[i], 100 );
    }
  }

  WinSetAccelTable( hab, plman, NULLHANDLE );
  if( accel != NULLHANDLE ) {
    WinDestroyAccelTable( accel );
  }
  while(( rec = pm_m_first_record()) != NULL ) {
    pm_m_destroy_children_and_self( rec );
  }

  pm_m_set_title( NULL );

  WinDestroyWindow( pm_list_menu );
  WinDestroyWindow( pm_file_menu );
  WinDestroyWindow( pm_main_menu );
  WinDestroyWindow( plman );
}

