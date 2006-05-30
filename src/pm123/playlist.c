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
#include <string.h>
#include <stdio.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "playlist.h"
#include "pm123.h"
#include "utilfct.h"
#include "plugman.h"

static HWND menu_playlist;
static HWND menu_record;
static HWND playlist;

extern BOOL pl_init_thread( HWND container );
extern BOOL pl_term_thread( void );
static void pl_init_window( HWND hwnd );
static void pl_term_window( HWND hwnd );

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

  WinSetWindowPos( playlist, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );
}

/* Sets a title of the playlist window. */
BOOL
pl_set_title( const char* title )
{
  return WinSetWindowText( playlist, (PSZ)title );
}

/* Recalls one of the saved playlists. */
static void
pl_recall( int i )
{
  char filename[_MAX_PATH];
  strcpy( filename, cfg.list[i] );

  if( is_playlist( filename )) {
    pl_load( filename, PL_LOAD_CLEAR );
  }
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
      array = realloc( array, size * sizeof( PLRECORD* ));
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
    for( rec = pl_first_selected(); rec; rec = pl_next_selected( rec )) {
      if( amp_playmode == AMP_PLAYLIST && rec == currentf && decoder_playing()) {
          amp_stop();
      }
      if( is_file( rec->full ) && unlink( rec->full ) != 0 ) {
        amp_error( playlist, "Unable delete file:\n%s\n%s",
                              rec->full, clib_strerror(errno));
      }
    }

    pl_remove_selected();
  }
}

/* Shows the context menu of the playlist. */
static void
pl_show_context_menu( HWND parent, PLRECORD* rec )
{
  POINTL   pos;
  SWP      swp;
  char     file[_MAX_PATH];
  HWND     mh;
  MENUITEM mi;
  int      i;
  int      count;
  short    id;


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
    if( !is_file( rec->full )) {
      mn_enable_item( menu_record, IDM_PL_S_TAG,  FALSE );
      mn_enable_item( menu_record, IDM_PL_S_DTAG, FALSE );
    } else {
      mn_enable_item( menu_record, IDM_PL_S_TAG,  TRUE  );
      mn_enable_item( menu_record, IDM_PL_S_DTAG, TRUE  );
    }

    WinPopupMenu( parent, parent, menu_record, pos.x, pos.y, IDM_PL_S_PLAY,
                  PU_POSITIONONITEM | PU_HCONSTRAIN   | PU_VCONSTRAIN |
                  PU_MOUSEBUTTON1   | PU_MOUSEBUTTON2 | PU_KEYBOARD   );
    return;
  }

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

        if( is_http( cfg.list[i] )) {
          strcat( file, "[http] " );
        }

        sfnameext( file + strlen(file), cfg.list[i] );

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
pl_drag_init_item( HWND hwnd, PLRECORD* rec,
                   PDRAGINFO drag_infos, PDRAGIMAGE drag_image, int i )
{
  DRAGITEM ditem;
  char pathname[_MAX_PATH];
  char filename[_MAX_PATH];

  memset( &ditem, 0, sizeof( ditem ));

  sdrivedir( pathname, rec->full );
  sfnameext( filename, rec->full );

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
  USHORT    drag_op;
  USHORT    drag;

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

  array = malloc( pdinfo->cditem * sizeof( PLRECORD* ));

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
      if( is_dir( fullname )) {
        pl_add_directory( fullname, PL_DIR_RECURSIVE );
      } else if( is_playlist( fullname )) {
        pl_load( fullname, 0 );
      } else {
        pl_add_file( fullname, NULL, 0 );
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
        ins = pl_create_record( fullname, pos, NULL );

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

/* Processes messages of the playlist presentation window. */
static MRESULT EXPENTRY
pl_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_INITDLG:
      pl_init_window( hwnd );
      break;
    case WM_DESTROY:
      pl_term_window( hwnd );
      break;

    case WM_HELP:
      amp_show_help( IDH_PL );
      return 0;

    case WM_SYSCOMMAND:
      if( SHORT1FROMMP(mp1) == SC_CLOSE ) {
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

    case WM_ADJUSTWINDOWPOS:
    {
      PSWP pswp = (PSWP)mp1;
      if( cfg.dock_windows && pswp->fl & SWP_MOVE ) {
        amp_dock( hwnd, pswp, cfg.dock_margin );
      }
      break;
    }

    case DM_DISCARDOBJECT:
      return pl_drag_discard( hwnd, (PDRAGINFO)mp1 );

    case WM_PM123_REMOVE_RECORD:
      pl_remove_record((PLRECORD**)&mp1, 1 );
      return 0;

    case WM_COMMAND:
      if( COMMANDMSG(&msg)->cmd >  IDM_PL_LAST &&
          COMMANDMSG(&msg)->cmd <= IDM_PL_LAST + MAX_RECALL )
      {
        pl_recall( COMMANDMSG(&msg)->cmd - IDM_PL_LAST - 1 );
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
        case IDM_PL_CLEAR:
          pl_clear( PL_CLR_NEW  );
          return 0;
        case IDM_PL_USE:
          if( amp_playmode == AMP_PLAYLIST ) {
            amp_pl_release();
          } else {
            amp_pl_use();
          }
          return 0;
        case IDM_PL_URL:
          amp_add_url( hwnd, URL_ADD_TO_LIST );
          return 0;
        case IDM_PL_TRACK:
          amp_add_tracks( hwnd, TRK_ADD_TO_LIST );
          return 0;
        case IDM_PL_LOAD:
          amp_add_files( hwnd );
          return 0;
        case IDM_PL_LST_SAVE:
          amp_save_list_as( hwnd, SAV_LST_PLAYLIST );
          return 0;
        case IDM_PL_M3U_SAVE:
          amp_save_list_as( hwnd, SAV_M3U_PLAYLIST );
          return 0;
        case IDM_PL_LOADL:
          amp_load_list( hwnd );
          return 0;
        case IDM_PL_S_PLAY:
          amp_pl_play_record( pl_cursored());
          return 0;
        case IDM_PL_S_DEL:
          pl_remove_selected();
          return 0;
        case IDM_PL_S_KILL:
          pl_delete_selected();
          return 0;

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

        case IDM_PL_S_TAG:
        {
          PLRECORD* rec = pl_cursored();
          if( rec ) {
            amp_id3_edit( hwnd, rec->full );
          }
          return 0;
        }

        case IDM_PL_S_DTAG:
        {
          PLRECORD* rec = pl_cursored();
          if( rec ) {
            amp_id3_wipe( hwnd, rec->full );
          }
          return 0;
        }
      }
      break;

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
            amp_pl_play_record((PLRECORD*)notify->pRecord );
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

  FIELDINFOINSERT insert;
  CNRINFO cnrinfo;

  HWND container = WinWindowFromID( hwnd, CNR_PLAYLIST );
  HAB  hab = WinQueryAnchorBlock( hwnd );

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

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Size";
  field->offStruct  = FIELDOFFSET( PLRECORD, length );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Time";
  field->offStruct  = FIELDOFFSET( PLRECORD, time );

  field = field->pNextFieldInfo;

  field->flData     = CFA_SEPARATOR | CFA_HORZSEPARATOR | CFA_STRING;
  field->pTitleData = "Information";
  field->offStruct  = FIELDOFFSET( PLRECORD, info );

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

  pl_init_thread( container );

  /* Initializes the playlist presentation window. */
  accel = WinLoadAccelTable( hab, NULLHANDLE, ACL_PLAYLIST );

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

  WinSendMsg( menu_playlist, MM_QUERYITEM,
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
                              MPFROMLONG( IDM_PL_LOADL ), 0 );

  WinSendMsg( menu_playlist, MM_QUERYITEM,
              MPFROM2SHORT( IDM_PL_ADD, TRUE ), MPFROMP( &mi ));

  WinSetWindowULong( mi.hwndSubMenu, QWL_STYLE,
    WinQueryWindowULong( mi.hwndSubMenu, QWL_STYLE ) | MS_CONDITIONALCASCADE );

  WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID,
                              MPFROMLONG( IDM_PL_LOAD ), 0 );
}

/* Terminates the playlist presentation window. */
static void
pl_term_window( HWND hwnd )
{
  HAB hab = WinQueryAnchorBlock( hwnd );
  HACCEL accel = WinQueryAccelTable( hab, hwnd );

  pl_term_thread();
  save_window_pos( hwnd, 0 );

  WinSetAccelTable( hab, hwnd, NULLHANDLE );
  if( accel != NULLHANDLE ) {
    WinDestroyAccelTable( accel );
  }

  WinDestroyWindow( menu_record   );
  WinDestroyWindow( menu_playlist );
}

/* Creates the playlist presentation window. */
HWND
pl_create( void )
{
  playlist = WinLoadDlg( HWND_DESKTOP, HWND_DESKTOP,
                         pl_dlg_proc, NULLHANDLE, DLG_PLAYLIST, NULL );

  pl_show( cfg.show_playlist );
  return playlist;
}

