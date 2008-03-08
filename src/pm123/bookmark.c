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
#include <errno.h>
#include <utilfct.h>

#include "pm123.h"
#include "bookmark.h"
#include "docking.h"
#include "iniman.h"
#include "assertions.h"
#include "playlist.h"

/* Structure that contains information for records in
   the bookmarks container control. */

typedef struct _BMRECORD {

  RECORDCORE   rc;
  char*        desc;     /* Name of the bookmark.     */
  char*        filename; /* Full path and file name.  */
  char*        time;     /* Displayed position time.  */
  ULONG        play_pos; /* Position.                 */

} BMRECORD, *PBMRECORD;

typedef struct _BMFINFO {

  char         filename[_MAX_PATH];
  DECODER_INFO info;
  ULONG        play_pos;

} BMFINFO;

typedef struct _BMDROPINFO {

  HWND   hwndItem;  /* Window handle of the source of the drag operation. */
  ULONG  ulItemID;  /* Information used by the source to identify the
                       object being dragged. */
} BMDROPINFO;

static HWND     menu_record = NULLHANDLE;
static HWND     menu_list   = NULLHANDLE;
static HWND     bookmarks   = NULLHANDLE;
static HWND     container   = NULLHANDLE;
static HPOINTER icon_record = NULLHANDLE;

static void bm_m_init_window( HWND hwnd );
static BOOL bm_m_save( HWND hwnd );

#define BM_ADD_BOOKMARK   ( WM_USER + 1000 )
#define BM_ADD_MENU       ( WM_USER + 1001 )
#define BM_USE_MENU       ( WM_USER + 1002 )

/* WARNING!!! All functions returning a pointer to the
   bookmark record, return a NULL if suitable record is not found. */

INLINE BMRECORD*
BM_RC( MRESULT rc )
{
  if((BMRECORD*)rc != (BMRECORD*)-1 ) {
    return (BMRECORD*)rc;
  } else {
    return NULL;
  }
}

/* Returns the pointer to the first bookmark record. */
static BMRECORD*
bm_m_first_record( void ) {
  return BM_RC( WinSendMsg( container, CM_QUERYRECORD, NULL,
                            MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER )));
}

/* Returns the pointer to the next bookmark record of specified. */
static BMRECORD*
bm_m_next_record( BMRECORD* rec ) {
  return BM_RC( WinSendMsg( container, CM_QUERYRECORD, MPFROMP(rec),
                            MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER )));
}

/* Returns the pointer to the bookmark record with the
   specified description. */
static BMRECORD*
bm_m_find_record( const char* desc )
{
  BMRECORD* rec;

  for( rec = bm_m_first_record(); rec; rec = bm_m_next_record( rec )) {
    if( strcmp( rec->desc, desc ) == 0 ) {
      break;
    }
  }

  return rec;
}

/* Returns the pointer to the first selected bookmark record. */
static BMRECORD*
bm_m_first_selected( void ) {
  return BM_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_SELECTED )));
}

/* Returns the pointer to the next selected bookmark record of specified. */
static BMRECORD*
bm_m_next_selected( BMRECORD* rec ) {
  return BM_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( rec ), MPFROMSHORT( CRA_SELECTED )));
}

/* Returns the pointer to the cursored bookmark record. */
static BMRECORD*
bm_m_cursored( void ) {
  return BM_RC( WinSendMsg( container, CM_QUERYRECORDEMPHASIS,
                            MPFROMP( CMA_FIRST ), MPFROMSHORT( CRA_CURSORED )));
}

/* Selects the specified bookmark record and deselects all others. */
static void
bm_m_select( BMRECORD* rec )
{
  BMRECORD* uns;

  for( uns = bm_m_first_selected(); uns; uns = bm_m_next_selected( uns )) {
    WinSendMsg( container, CM_SETRECORDEMPHASIS,
                MPFROMP( uns ), MPFROM2SHORT( FALSE, CRA_SELECTED ));
  }

  WinSendMsg( container, CM_SETRECORDEMPHASIS,
              MPFROMP( rec ), MPFROM2SHORT( TRUE , CRA_SELECTED | CRA_CURSORED ));
}

/* Returns the number of bookmarks in the container. */
static int
bm_m_size( void )
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

/* Frees the data contained in bookmark record. */
static void
bm_m_free_record( BMRECORD* rec )
{
  free( rec->rc.pszIcon );
  free( rec->desc );
  free( rec->filename );
  free( rec->time );

  rec->rc.pszIcon = NULL;
  rec->desc       = NULL;
  rec->filename   = NULL;
  rec->time       = NULL;
}

/* Removes the specified bookmark record. */
static void
bm_m_remove_records( BMRECORD** array, USHORT count )
{
  int i;

  if( WinSendMsg( container, CM_REMOVERECORD, MPFROMP( array ),
                  MPFROM2SHORT( count, CMA_INVALIDATE )) != MRFROMLONG( -1 ))
  {
    for( i = 0; i < count; i++ ) {
      bm_m_free_record( array[i] );
    }

    WinSendMsg( container, CM_FREERECORD,
                MPFROMP( array ), MPFROMSHORT( count ));
  }
}

/* Removes all bookmark records. */
static void
bm_m_remove_all( void )
{
  BMRECORD* rec;

  for( rec = bm_m_first_record(); rec; rec = bm_m_next_record( rec )) {
    bm_m_free_record( rec );
  }

  WinSendMsg( container, CM_REMOVERECORD,
              MPFROMP( NULL ), MPFROM2SHORT( 0, CMA_FREE | CMA_INVALIDATE ));
}

/* Moves the bookmark record to specified position. */
static BMRECORD*
bm_m_move_record( BMRECORD* rec, BMRECORD* pos )
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

/* Refreshes the specified bookmark record. */
static void
bm_m_refresh_record( BMRECORD* rec )
{
  WinSendMsg( container, CM_INVALIDATERECORD,
              MPFROMP( &rec ), MPFROM2SHORT( 1, 0 ));
}

/* Creates the bookmarks record for specified file. */
static BMRECORD*
bm_m_create_record( BMRECORD*   pos,
                    const char* filename,
                    const char* desc,
                    ULONG       play_pos )
{
  BMRECORD*    rec;
  RECORDINSERT insert;
  char         time[32];
  int          major;
  int          minor;

  /* Allocate a new record */
  rec = (BMRECORD*)WinSendMsg( container, CM_ALLOCRECORD,
                               MPFROMLONG( sizeof( BMRECORD ) - sizeof( RECORDCORE )),
                               MPFROMLONG( 1 ));
  if( rec ) {
    sec2num( play_pos / 1000,  &major, &minor );
    sprintf( time, "%02d:%02d", major,  minor );

    rec->rc.cb           = sizeof( RECORDCORE );
    rec->rc.flRecordAttr = CRA_DROPONABLE;
    rec->rc.hptrIcon     = icon_record;
    rec->rc.pszIcon      = strdup( desc );
    rec->filename        = strdup( filename );
    rec->desc            = strdup( desc );
    rec->play_pos        = play_pos;
    rec->time            = strdup( time );

    insert.cb                = sizeof(RECORDINSERT);
    insert.pRecordOrder      = (PRECORDCORE)pos;
    insert.pRecordParent     = NULL;
    insert.fInvalidateRecord = FALSE;
    insert.zOrder            = CMA_TOP;
    insert.cRecordsInsert    = 1;

    WinSendMsg( container, CM_INSERTRECORD,
                MPFROMP( rec ), MPFROMP( &insert ));

    bm_m_refresh_record( rec );
  }
  return rec;
}

/* Updates the bookmarks record for specified file. */
static void
bm_m_update_record( BMRECORD*   rec,
                    const char* filename,
                    const char* desc,
                    ULONG       play_pos )
{
  BMRECORD old = *rec;
  char     time[32];
  int      major;
  int      minor;

  sec2num( play_pos / 1000,  &major, &minor );
  sprintf( time, "%02d:%02d", major,  minor );

  rec->rc.pszIcon = strdup( desc );
  rec->filename   = strdup( filename );
  rec->desc       = strdup( desc );
  rec->play_pos   = play_pos;
  rec->time       = strdup( time );

  bm_m_free_record   ( &old );
  bm_m_refresh_record(  rec );
}

/* Removes all selected bookmark records. */
static void
bm_m_remove_selected( void )
{
  BMRECORD** array = NULL;
  BMRECORD*  rec;
  USHORT     count = 0;
  USHORT     size  = 0;

  for( rec = bm_m_first_selected(); rec; rec = bm_m_next_selected( rec )) {
    if( count == size ) {
      size  = size + 20;
      array = realloc( array, size * sizeof( BMRECORD* ));
    }
    if( !array ) {
      return;
    }
    array[count++] = rec;
  }

  if( count ) {
    bm_m_remove_records( array, count );
    bm_m_select( bm_m_cursored());
    bm_m_save( bookmarks );
  }

  free( array );
}

/* Shows the context menu of the bookmark record. */
static void
bm_m_show_context_menu( HWND parent, BMRECORD* rec )
{
  POINTL pos;
  SWP    swp;

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
    WinPopupMenu( parent, parent, menu_record, pos.x, pos.y, IDM_BM_LOAD,
                  PU_POSITIONONITEM | PU_HCONSTRAIN   | PU_VCONSTRAIN |
                  PU_MOUSEBUTTON1   | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
  } else {
    WinPopupMenu( parent, parent, menu_list,   pos.x, pos.y, IDM_BM_ADD,
                  PU_POSITIONONITEM | PU_HCONSTRAIN   | PU_VCONSTRAIN |
                  PU_MOUSEBUTTON1   | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
  }
  return;
}

/* Prepares the bookmarks container item to the drag operation. */
static void
bm_m_drag_init_item( HWND hwnd, BMRECORD* rec,
                     PDRAGINFO drag_infos, PDRAGIMAGE drag_image, int i )
{
  DRAGITEM ditem;
  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];

  memset( &ditem, 0, sizeof( ditem ));

  sdrivedir( pathname, rec->filename, sizeof( pathname ));
  sfnameext( filename, rec->filename, sizeof( filename ));

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

  WinSendDlgItemMsg( hwnd, CNR_BOOKMARKS, CM_SETRECORDEMPHASIS,
                     MPFROMP( rec ), MPFROM2SHORT( TRUE, CRA_SOURCE ));
}

/* Prepares the bookmarks container to the drag operation. */
static MRESULT
bm_m_drag_init( HWND hwnd, PCNRDRAGINIT pcdi )
{
  BMRECORD*  rec;
  BOOL       drag_selected = FALSE;
  int        drag_count    = 0;
  PDRAGIMAGE drag_images   = NULL;
  PDRAGINFO  drag_infos    = NULL;

  // If the record under the mouse is NULL, we must be over whitespace,
  // in which case we don't want to drag any records.

  if( !(BMRECORD*)pcdi->pRecord ) {
    return 0;
  }

  // Count the selected records. Also determine whether or not we should
  // process the selected records. If the container record under the
  // mouse does not have this emphasis, we shouldn't.

  for( rec = bm_m_first_selected(); rec; rec = bm_m_next_selected( rec )) {
    if( rec == (BMRECORD*)pcdi->pRecord ) {
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
    for( rec = bm_m_first_selected(); rec; rec = bm_m_next_selected( rec ), i++ ) {
      bm_m_drag_init_item( hwnd, rec, drag_infos, drag_images+i, i );
    }
  } else {
    bm_m_drag_init_item( hwnd, (BMRECORD*)pcdi->pRecord,
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

  rec = (BMRECORD*)CMA_FIRST;
  while( rec ) {
    rec = (BMRECORD*)WinSendDlgItemMsg( hwnd, CNR_BOOKMARKS, CM_QUERYRECORDEMPHASIS,
                                        MPFROMP( rec ), MPFROMSHORT( CRA_SOURCE ));
    if( rec == (BMRECORD*)(-1)) {
      break;
    } else if( rec ) {
      WinSendDlgItemMsg( hwnd, CNR_BOOKMARKS, CM_SETRECORDEMPHASIS,
                         MPFROMP( rec ), MPFROM2SHORT( FALSE, CRA_SOURCE ));
      WinSendDlgItemMsg( hwnd, CNR_BOOKMARKS, CM_INVALIDATERECORD,
                         MPFROMP( &rec ), MPFROM2SHORT( 1, 0 ));
    }
  }

  free( drag_images );
  DrgFreeDraginfo( drag_infos );
  return 0;
}

/* Prepares the bookmarks container to the drop operation. */
static MRESULT
bm_m_drag_over( HWND hwnd, PCNRDRAGINFO pcdi )
{
  PDRAGINFO pdinfo = pcdi->pDragInfo;
  PDRAGITEM pditem;
  int       i;
  USHORT    drag_op = 0;
  USHORT    drag    = DOR_NEVERDROP;
  char      filename[_MAX_PATH];

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROM2SHORT( DOR_NEVERDROP, 0 );
  }

  for( i = 0; i < pdinfo->cditem; i++ )
  {
    pditem = DrgQueryDragitemPtr( pdinfo, i );
    DrgQueryStrName( pditem->hstrSourceName, sizeof( filename ), filename );

    if( DrgVerifyRMF( pditem, "DRM_123FILE", NULL )) {
      if( pdinfo->hwndSource  == hwnd ) {
        if( pdinfo->usOperation == DO_DEFAULT ||
            pdinfo->usOperation == DO_MOVE    )
        {
          drag    = DOR_DROP;
          drag_op = DO_MOVE;
        } else {
          drag    = DOR_NODROPOP;
          drag_op = DO_UNKNOWN;
          break;
        }
      } else if( is_playlist( filename )) {
        drag    = DOR_NEVERDROP;
        drag_op = DO_UNKNOWN;
        break;
      } else {
        if( pdinfo->usOperation == DO_DEFAULT && pditem->fsSupportedOps & DO_COPYABLE )
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
      }
    }
    else if( DrgVerifyRMF( pditem, "DRM_OS2FILE", NULL )) {
      if( is_playlist( filename )) {
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

/* Discards bookmarks records dropped into shredder. */
static MRESULT
bm_m_drag_discard( HWND hwnd, PDRAGINFO pdinfo )
{
  PDRAGITEM  pditem;
  BMRECORD** array = NULL;
  int        i;

  if( !DrgAccessDraginfo( pdinfo )) {
    return MRFROMLONG( DRR_ABORT );
  }

  // We get as many DM_DISCARDOBJECT messages as there are
  // records dragged but the first one has enough info to
  // process all of them.

  array = malloc( pdinfo->cditem * sizeof( BMRECORD* ));

  if( array ) {
    for( i = 0; i < pdinfo->cditem; i++ ) {
      pditem = DrgQueryDragitemPtr( pdinfo, i );
      array[i] = (BMRECORD*)pditem->ulItemID;
    }

    bm_m_remove_records( array, pdinfo->cditem );
    bm_m_save( bookmarks );
    free( array );
  }

  DrgFreeDraginfo( pdinfo );
  return MRFROMLONG( DRR_SOURCE );
}

/* Receives the dropped bookmarks records. */
static MRESULT
bm_m_drag_drop( HWND hwnd, PCNRDRAGINFO pcdi )
{
  PDRAGINFO pdinfo = pcdi->pDragInfo;
  PDRAGITEM pditem;

  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];
  char fullname[_MAX_PATH];
  int  i;
  char desc[1024];

  BMRECORD* pos = pcdi->pRecord ? (BMRECORD*)pcdi->pRecord : (BMRECORD*)CMA_END;
  BMRECORD* ins;

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
      BMRECORD* rec = (BMRECORD*)pditem->ulItemID;

      if( pdinfo->hwndSource  == hwnd &&
          pdinfo->usOperation == DO_MOVE )
      {
        ins = bm_m_move_record( rec, pos );

        if( ins ) {
          pos = ins;
        }
      } else {
        amp_title_from_filename( desc, filename, sizeof( desc ));
        ins = bm_m_create_record( pos, fullname, desc, 0 );

        if( ins ) {
          pos = ins;
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

        amp_title_from_filename( desc, filename, sizeof( desc ));
        ins = bm_m_create_record( pos, fullname, desc, 0 );

        if( ins ) {
          pos = ins;
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
        BMDROPINFO* pdsource = (BMDROPINFO*)malloc( sizeof( BMDROPINFO ));
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

  DrgDeleteDraginfoStrHandles( pdinfo );
  DrgFreeDraginfo( pdinfo );
  bm_m_save( bookmarks );
  return 0;
}

/* Receives dropped and rendered files and urls. */
static MRESULT
bm_m_drag_render_done( HWND hwnd, PDRAGTRANSFER pdtrans, USHORT rc )
{
  char rendered[_MAX_PATH];
  char fullname[_MAX_PATH];
  char desc[1024];

  BMDROPINFO* pdsource = (BMDROPINFO*)pdtrans->ulTargetInfo;
  ASSERT_IS_MAIN_THREAD;

  // If the rendering was successful, use the file, then delete it.
  if(( rc & DMFL_RENDEROK ) && pdsource &&
       DrgQueryStrName( pdtrans->hstrRenderToName, sizeof( rendered ), rendered ))
  {
    amp_url_from_file( fullname, rendered, sizeof( fullname ));
    amp_title_from_filename( desc, fullname, sizeof( desc ));

    DosDelete( rendered );
    bm_m_create_record((BMRECORD*)CMA_END, fullname, desc, 0 );

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

/* Load specified bookmark to player. */
static BOOL
bm_m_load_bookmark( BMRECORD* rec )
{
  BOOL rc;
  BOOL decoder_was_playing = decoder_playing();
  ASSERT_IS_MAIN_THREAD;

  if( decoder_was_playing ) {
    amp_stop();
  }

  rc = pl_load_file_record( rec->filename );

  if( !rc ) {
    rc = amp_load_singlefile( rec->filename, AMP_LOAD_NOT_PLAY | AMP_LOAD_NOT_RECALL );
  }

  if( amp_playmode == AMP_NOFILE ) {
    amp_playmode = AMP_SINGLE;
  }

  if( rc && ( cfg.playonload || rec->play_pos > 0 || decoder_was_playing )) {
    amp_play( rec->play_pos );
  }

  return TRUE;
}

/* Processes messages of the dialog of addition of bookmark. */
static MRESULT EXPENTRY
bm_m_add_bookmark_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_COMMAND:
      if( COMMANDMSG(&msg)->cmd == DID_OK )
      {
        char desc[1024];
        WinQueryDlgItemText( hwnd, EF_BM_DESC, sizeof(desc), desc );

        if( bm_m_find_record( desc )) {
          if( !amp_query( hwnd, "Bookmark %s already exists. Overwrite it?", desc )) {
            WinSetFocus( HWND_DESKTOP, WinWindowFromID( hwnd, EF_BM_DESC ));
            return 0;
          }
        }
      }
      break;
  }

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Adds a user selected bookmark. */
static void
bm_m_add_bookmark( HWND owner, BMFINFO* finfo )
{
  char      desc[1024]  = "";
  char      file[_MAX_FNAME];
  BMRECORD* rec;
  HWND      hdlg = WinLoadDlg( HWND_DESKTOP, owner, bm_m_add_bookmark_dlg_proc,
                               hmodule, DLG_BM_ADD, NULL );
  if( *finfo->info.artist ) {
    strlcat( desc, finfo->info.artist, sizeof( desc ));
    strlcat( desc, "-", sizeof( desc ));
  }
  if( *finfo->info.title  ) {
    strlcat( desc, finfo->info.title, sizeof( desc ));
  } else {
    sdname ( file, finfo->filename, sizeof( file ));
    strlcat( desc, file, sizeof( desc ));
  }

  WinSetDlgItemText( hdlg, EF_BM_DESC, desc );
  if( WinProcessDlg( hdlg ) == DID_OK )
  {
    WinQueryDlgItemText( hdlg, EF_BM_DESC, sizeof(desc), desc );
    rec = bm_m_find_record( desc );

    if( rec ) {
      bm_m_update_record( rec, finfo->filename, desc, finfo->play_pos );
    } else {
      bm_m_create_record((BMRECORD*)CMA_END, finfo->filename, desc, finfo->play_pos );
    }

    bm_m_save( owner );
  }

  WinDestroyWindow( hdlg );
}

/* Adds bookmarks to submenu in the main pop-up menu.
   Must be called from the main thread. */
static void
bm_m_add_bookmarks_to_menu( HWND hmenu )
{
  BMRECORD* rec;
  MENUITEM  mi;
  int       i;
  int       count;
  short     id;

  ASSERT_IS_MAIN_THREAD;
  count = LONGFROMMR( WinSendMsg( hmenu, MM_QUERYITEMCOUNT, 0, 0 ));

  // Remove all items from the load menu except of two first
  // intended for a choice of object of loading.
  for( i = 2; i < count; i++ )
  {
    id = LONGFROMMR( WinSendMsg( hmenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(2), 0 ));
    WinSendMsg( hmenu, MM_DELETEITEM, MPFROM2SHORT( id, FALSE ), 0 );
  }

  if( bm_m_size() > 0 )
  {
    // Add separator.
    mi.iPosition = MIT_END;
    mi.afStyle = MIS_SEPARATOR;
    mi.afAttribute = 0;
    mi.id = 0;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem = 0;

    WinSendMsg( hmenu, MM_INSERTITEM, MPFROMP( &mi ), NULL );


    // Fill the bookmarks list.
    for( rec = bm_m_first_record(), i = 0; rec;
         rec = bm_m_next_record( rec ), i++ )
    {
      mi.iPosition   = MIT_END;
      mi.afStyle     = MIS_TEXT;
      mi.afAttribute = 0;
      mi.id          = IDM_M_BOOKMARKS + i + 1;
      mi.hwndSubMenu = (HWND)NULLHANDLE;
      mi.hItem       = 0;

      WinSendMsg( hmenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP( rec->desc ));
    }
  }
}

/* Loads bookmark selected via pop-up menu. Must be
   called from the main thread. */
static BOOL
bm_m_use_bookmark_from_menu( USHORT cmd )
{
  BMRECORD* rec;
  int pos = cmd - IDM_M_BOOKMARKS - 1;

  ASSERT_IS_MAIN_THREAD;

  for( rec = bm_m_first_record(); rec && pos; rec = bm_m_next_record( rec )) {
    --pos;
  }

  if( rec ) {
    return bm_m_load_bookmark( rec );
  } else {
    return FALSE;
  }
}

/* Renames a user specified bookmark. */
static void
bm_m_rename_bookmark( HWND owner, BMRECORD* rec )
{
  char desc[1024]  = "";
  HWND hdlg = WinLoadDlg( HWND_DESKTOP, owner, WinDefDlgProc,
                          hmodule, DLG_BM_RENAME, NULL );

  WinSetDlgItemText( hdlg, EF_BM_DESC, rec->desc );
  if( WinProcessDlg( hdlg ) == DID_OK )
  {
    WinQueryDlgItemText( hdlg, EF_BM_DESC, sizeof(desc), desc );
    bm_m_update_record( rec, rec->filename, desc, rec->play_pos );
    bm_m_save( owner );
  }

  WinDestroyWindow( hdlg );
}

/* Loads bookmarks from the file. */
static BOOL
bm_m_load( HWND owner )
{
  FILE* hfile;
  char  line[_MAX_PATH];
  char  file[_MAX_PATH];
  char  list[_MAX_PATH];
  ULONG play_pos = 0;

  strcpy( list, startpath );
  strcat( list, "bookmark.lst" );

  hfile = fopen( list, "r" );
  if( hfile == NULL ) {
    return FALSE;
  }

  bm_m_remove_all();

  while( !feof( hfile ))
  {
    fgets( line, sizeof( line ), hfile );
    blank_strip( line );

    if( *line != 0 && *line != '#' && *line != '>' && *line != '<' )
    {
      strcpy( file, line );
    }
    else if( *line == '>' )
    {
      // '>' entry is after the filename
      sscanf( line, ">%lu\n", &play_pos );
    }
    else if( *line == '<' )
    {
      // '<' entry is the last
      bm_m_create_record((BMRECORD*)CMA_END, file, line + 1, play_pos );
      play_pos = 0;
    }
  }

  fclose( hfile );
  return TRUE;
}

/* Saves bookmarks to the file. */
static BOOL
bm_m_save( HWND owner )
{
  FILE*     hfile;
  BMRECORD* rec;

  char lstfile[_MAX_PATH ];
  char bakfile[_MAX_PATH ];

  ASSERT_IS_MAIN_THREAD;

  strcpy( lstfile, startpath      );
  strcat( lstfile, "bookmark.lst" );
  strcpy( bakfile, startpath      );
  strcat( bakfile, "bookmark.ls~" );

  if( remove( bakfile ) != 0 && errno != ENOENT ) {
    amp_error( owner, "Unable delete backup file:\n%s\n%s",
               bakfile, strerror(errno));
    return FALSE;
  }

  if( rename( lstfile, bakfile ) != 0 && errno != ENOENT ) {
    amp_error( owner, "Unable backup bookmark file:\n%s\n%s",
               lstfile, strerror(errno));
    return FALSE;
  }

  hfile = fopen( lstfile, "w" );
  if( hfile == NULL ) {
    amp_error( owner, "Unable create bookmark file:\n%s\n%s",
               lstfile, strerror(errno));
    return FALSE;
  }

  fprintf( hfile,
      "#\n"
      "# Bookmark list created with %s\n"
      "# Do not modify! This file is compatible with the playlist format,\n"
      "# but information written in this file is different.\n"
      "#\n", AMP_FULLNAME );

  for( rec = bm_m_first_record(); rec; rec = bm_m_next_record( rec ))
  {
    int rc = fprintf( hfile, "%s\n>%lu\n<%s\n", rec->filename, rec->play_pos, rec->desc );
    if( rc < 0 )
    {
      fclose( hfile );
      return FALSE;
    }
  }

  fprintf( hfile, "# End of bookmark list\n" );

  if( fclose( hfile ) == 0 ) {
    remove( bakfile );
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Processes messages of the bookmark presentation window. */
static MRESULT EXPENTRY
bm_m_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_INITDLG:
      bm_m_init_window( hwnd );
      dk_add_window ( hwnd, 0 );
      break;

    case WM_SYSCOMMAND:
      if( SHORT1FROMMP(mp1) == SC_CLOSE ) {
        bm_show( FALSE );
        return 0;
      }
      break;

    case WM_WINDOWPOSCHANGED:
    {
      SWP* pswp = PVOIDFROMMP(mp1);

      if( pswp[0].fl & SWP_SHOW ) {
        cfg.show_bmarks = TRUE;
      }
      if( pswp[0].fl & SWP_HIDE ) {
        cfg.show_bmarks = FALSE;
      }
      break;
    }

    case DM_DISCARDOBJECT:
      return bm_m_drag_discard( hwnd, (PDRAGINFO)mp1 );
    case DM_RENDERCOMPLETE:
      return bm_m_drag_render_done( hwnd, (PDRAGTRANSFER)mp1, SHORT1FROMMP( mp2 ));

    case WM_123FILE_REMOVE:
    {
      BMRECORD* rec = (BMRECORD*)mp1;
      bm_m_remove_records( &rec, 1 );
      return 0;
    }

    case WM_123FILE_LOAD:
      bm_m_load_bookmark((BMRECORD*)mp1 );
      return 0;

    case WM_COMMAND:
      switch( COMMANDMSG(&msg)->cmd ) {
        case IDM_BM_REMOVE:
          bm_m_remove_selected();
          return 0;
        case IDM_BM_RMENU:
          WinSendMsg( hwnd, WM_CONTROL,
                      MPFROM2SHORT( CNR_BOOKMARKS, CN_CONTEXTMENU ),
                      PVOIDFROMMP( bm_m_cursored()));
          return 0;
        case IDM_BM_LMENU:
          WinSendMsg( hwnd, WM_CONTROL,
                      MPFROM2SHORT( CNR_BOOKMARKS, CN_CONTEXTMENU ), 0 );
          return 0;
        case IDM_BM_LOAD:
          bm_m_load_bookmark( bm_m_cursored());
          return 0;
        case IDM_BM_ADDTOPL:
          pl_add_file( bm_m_cursored()->filename, NULL, 0 );
          return 0;
        case IDM_BM_RENAME:
          bm_m_rename_bookmark( hwnd, bm_m_cursored());
          return 0;
        case IDM_BM_CLEAR:
          bm_m_remove_all();
          bm_m_save( hwnd );
          return 0;
        case IDM_BM_ADD:
          amp_post_command( IDM_M_BM_ADD );
          return 0;
      }
      break;

    case WM_CONTROL:
      switch( SHORT2FROMMP( mp1 )) {
        case CN_CONTEXTMENU:
          bm_m_show_context_menu( hwnd, (BMRECORD*)mp2 );
          return 0;

        case CN_ENTER:
        {
          NOTIFYRECORDENTER* notify = (NOTIFYRECORDENTER*)mp2;
          if( notify->pRecord ) {
            bm_m_load_bookmark((BMRECORD*)notify->pRecord );
          }
          return 0;
        }

        case CN_HELP:
          amp_show_help( IDH_MAIN );
          return 0;

        case CN_INITDRAG:
          return bm_m_drag_init( hwnd, (PCNRDRAGINIT)mp2 );
        case CN_DRAGAFTER:
          return bm_m_drag_over( hwnd, (PCNRDRAGINFO)mp2 );
        case CN_DROP:
          return bm_m_drag_drop( hwnd, (PCNRDRAGINFO)mp2 );
      }
      break;

    case BM_ADD_BOOKMARK:
      bm_m_add_bookmark((HWND)mp1, mp2 );
      free( mp2 );
      break;

    case BM_ADD_MENU:
      bm_m_add_bookmarks_to_menu((HWND)mp1 );
      break;
    case BM_USE_MENU:
      bm_m_use_bookmark_from_menu( SHORT1FROMMP( mp1 ));
      break;
  }

  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Initializes the bookmark presentation window. */
static void
bm_m_init_window( HWND hwnd )
{
  FIELDINFO* first;
  FIELDINFO* field;
  HPOINTER   hicon;
  HACCEL     accel;

  FIELDINFOINSERT insert;
  CNRINFO cnrinfo;

  HAB   hab = WinQueryAnchorBlock( hwnd );
  container = WinWindowFromID( hwnd, CNR_BOOKMARKS );

  /* Initializes the bookmarks container. */
  first = (FIELDINFO*)WinSendMsg( container, CM_ALLOCDETAILFIELDINFO, MPFROMSHORT(6), 0 );
  field = first;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_BITMAPORICON;
  field->pTitleData = "";
  field->offStruct  = FIELDOFFSET( BMRECORD, rc.hptrIcon);

  field = field->pNextFieldInfo;

  field->flData     = CFA_STRING | CFA_HORZSEPARATOR;
  field->pTitleData = "Description";
  field->offStruct  = FIELDOFFSET( BMRECORD, rc.pszIcon );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Position";
  field->offStruct  = FIELDOFFSET( BMRECORD, time );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Filename";
  field->offStruct  = FIELDOFFSET( BMRECORD, filename );

  insert.cb = sizeof(FIELDINFOINSERT);
  insert.pFieldInfoOrder = (PFIELDINFO)CMA_FIRST;
  insert.fInvalidateFieldInfo = TRUE;
  insert.cFieldInfoInsert = 4;

  WinSendMsg( container, CM_INSERTDETAILFIELDINFO,
              MPFROMP( first ), MPFROMP( &insert ));

  cnrinfo.cb             = sizeof(cnrinfo);
  cnrinfo.pFieldInfoLast = first->pNextFieldInfo;
  cnrinfo.flWindowAttr   = CV_DETAIL | CV_MINI  | CA_DRAWICON |
                           CA_DETAILSVIEWTITLES | CA_ORDEREDTARGETEMPH;
  cnrinfo.xVertSplitbar  = cfg.sbar_bmarks;

  WinSendMsg( container, CM_SETCNRINFO, MPFROMP(&cnrinfo),
              MPFROMLONG( CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR | CMA_FLWINDOWATTR ));

  /* Initializes the bookmarks presentation window. */
  accel = WinLoadAccelTable( hab, hmodule, ACL_BOOKMARKS );

  if( accel ) {
    WinSetAccelTable( hab, accel, hwnd );
  }

  hicon = WinLoadPointer( HWND_DESKTOP, hmodule, ICO_MAIN );
  WinSendMsg( hwnd, WM_SETICON, (MPARAM)hicon, 0 );
  do_warpsans( hwnd );

  if( !rest_window_pos( hwnd, 0 )) {
    bm_set_colors( DEF_FG_COLOR, DEF_BG_COLOR, DEF_HI_FG_COLOR, DEF_HI_BG_COLOR );
  }

  menu_record = WinLoadMenu   ( HWND_OBJECT,  hmodule, MNU_BM_RECORD );
  menu_list   = WinLoadMenu   ( HWND_OBJECT,  hmodule, MNU_BM_LIST   );
  icon_record = WinLoadPointer( HWND_DESKTOP, hmodule, ICO_BOOKMARK  );

  bm_m_load( hwnd );
}

/* Destroys the bookmarks presentation window. Must be
   called from the main thread. */
void
bm_destroy( void )
{
  HAB     hab   = WinQueryAnchorBlock( bookmarks );
  HACCEL  accel = WinQueryAccelTable ( hab, bookmarks );
  CNRINFO info;

  ASSERT_IS_MAIN_THREAD;
  save_window_pos( bookmarks, 0 );

  if( WinSendMsg( container, CM_QUERYCNRINFO,
                  MPFROMP(&info), MPFROMLONG(sizeof(info))) != 0 )
  {
    cfg.sbar_bmarks = info.xVertSplitbar;
  }

  WinSetAccelTable( hab, bookmarks, NULLHANDLE );
  if( accel != NULLHANDLE ) {
    WinDestroyAccelTable( accel );
  }

  bm_m_remove_all();

  WinDestroyPointer( icon_record );
  WinDestroyWindow ( menu_record );
  WinDestroyWindow ( bookmarks   );
}

/* Changes the bookmarks presentation window colors. */
BOOL
bm_set_colors( ULONG fgcolor, ULONG bgcolor, ULONG hi_fgcolor, ULONG hi_bgcolor )
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

/* Creates the bookmarks presentation window. Must be
   called from the main thread. */
HWND
bm_create( void )
{
  ASSERT_IS_MAIN_THREAD;

  bookmarks = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
                          bm_m_dlg_proc, hmodule, DLG_BOOKMARKS, NULL );

  bm_show( cfg.show_bmarks );
  return bookmarks;
}

/* Sets the visibility state of the bookmarks presentation window. */
void
bm_show( BOOL show )
{
  HSWITCH hswitch = WinQuerySwitchHandle( bookmarks, 0 );
  SWCNTRL swcntrl;

  if( WinQuerySwitchEntry( hswitch, &swcntrl ) == 0 ) {
    swcntrl.uchVisibility = show ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry( hswitch, &swcntrl );
  }

  dk_set_state( bookmarks, show ? 0 : DK_IS_GHOST );
  WinSetWindowPos( bookmarks, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );
}

/* Adds a user selected bookmark of the specified file. */
void
bm_add_bookmark( HWND owner, const char* filename, const DECODER_INFO* info, ULONG pos )
{
  BMFINFO* finfo = malloc( sizeof( BMFINFO ));

  if( finfo ) {
    finfo->info = *info;
    finfo->play_pos = pos;
    strlcpy( finfo->filename, filename, sizeof( finfo->filename ));
    WinPostMsg( bookmarks, BM_ADD_BOOKMARK, MPFROMHWND( owner ), finfo );
  } else {
    amp_player_error( "Not enough memory." );
  }
}

/* Adds bookmarks to submenu in the main pop-up menu. */
void
bm_add_bookmarks_to_menu( HWND hmenu ) {
  WinSendMsg( bookmarks, BM_ADD_MENU, MPFROMHWND( hmenu ), 0 );
}

/* Loads bookmark selected via pop-up menu. */
void
bm_use_bookmark_from_menu( USHORT id ) {
  WinPostMsg( bookmarks, BM_USE_MENU, MPFROMSHORT( id ), 0 );
}

