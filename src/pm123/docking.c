/*
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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
#include <os2.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

#include "pm123.h"
#include "properties.h"
#include "docking.h"
#include <utilfct.h>

#define IS_MASTER( data )   ( data->state &   DK_IS_MASTER )
#define IS_GHOST( data )    ( data->state &   DK_IS_GHOST  )
#define IS_DOCKABLE( data ) ( data->state & ( DK_IS_MASTER | DK_IS_DOCKED ))

static DK_DATA head;

/** Returns a pointer to the data of specified window or NULL if
    the specified window is not a part of a this docking subsystem. */
static DK_DATA*
dk_get_window( HWND hwnd )
{
  int  i;

  for( i = 0; i < head.childs_count; i++ ) {
    if( head.childs[i]->hwnd == hwnd ) {
      return head.childs[i];
    }
  }
  return NULL;
}

/** Returns a state of the specified window. */
int
dk_get_state( HWND hwnd )
{
  DK_DATA* window = dk_get_window( hwnd );

  if( window ) {
    return window->state;
  } else {
    return 0;
  }
}

/** Sets a state of the specified window. */
void
dk_set_state( HWND hwnd, int state )
{
  DK_DATA* window = dk_get_window( hwnd );

  if( window ) {
    if( state & DK_IS_GHOST ) {
      window->state |=  DK_IS_GHOST;
    } else {
      window->state &= ~DK_IS_GHOST;
    }
  }
}

/** Returns TRUE if the window is docked to the specified owner. */
static BOOL
dk_is_docked( DK_DATA* target, DK_DATA* window )
{
  int  i;

  for( i = 0; i < target->childs_count; i++ ) {
    if( target->childs[i] == window ) {
      return TRUE;
    }
  }
  return FALSE;
}

/** Docks a window to the specified target window. */
static BOOL
dk_dock( DK_DATA* target, DK_DATA* window )
{
  if( dk_is_docked( target, window ) ||
      dk_is_docked( window, target ))
  {
    return TRUE;
  }

  if( target->childs_count < DK_MAX_DOCKED ||
      window->owners_count < DK_MAX_DOCKED )
  {
    target->childs[ target->childs_count++ ] = window;
    window->owners[ window->owners_count++ ] = target;
    window->state |= DK_IS_DOCKED;
    return TRUE;
  } else {
    return FALSE;
  }
}

/** Removes a pointer to the specified window's data from
    the specified list. */
static BOOL
dk_remove_data( DK_DATA** list, int* count, DK_DATA* data )
{
  int i;

  for( i = 0; i < *count; i++ ) {
    if( list[i] == data ) {
      if( i < --*count ) {
        memmove( &list[i], &list[i+1], ( *count - i ) * sizeof( DK_DATA* ));
      }
      return TRUE;
    }
  }

  return FALSE;
}

/** Undocks a window from the specified target window. */
static void
dk_undock( DK_DATA* target, DK_DATA* window )
{
  dk_remove_data( window->owners, &window->owners_count, target );
  dk_remove_data( target->childs, &target->childs_count, window );

  if( !window->owners_count ) {
    window->state &= ~DK_IS_DOCKED;
    while( window->childs_count ) {
      dk_undock( window, window->childs[0] );
    }
  }
}

/** Docks a window position to the specified anchor window. */
static BOOL
dk_adjust_window( DK_DATA* anchor, DK_DATA* window, PSWP swp )
{
  SWP  swp_anchor;
  SWP  swp_window;
  LONG margin = cfg.dock_margin;
  BOOL docked = FALSE;

  WinQueryWindowPos( anchor->hwnd, &swp_anchor );
  WinQueryWindowPos( window->hwnd, &swp_window );

  if( max( swp->x, swp_anchor.x ) < min( swp->x + swp->cx, swp_anchor.x + swp_anchor.cx )) {
    if( abs( swp->y + swp->cy - swp_anchor.y ) < margin ) {
      // Docking when window to dock is under anchor window. If window is
      // moving, adjust the window position. If window is sizing, adjust
      // the window size.
      if( swp_window.y != swp->y ) {
        swp->y  = swp_anchor.y - swp->cy;
      } else {
        swp->cy = swp_anchor.y - swp->y;
      }
      docked = TRUE;
    }
    else if( abs( swp->y - ( swp_anchor.y + swp_anchor.cy )) < margin ) {
      // Docking when window to dock is over anchor window. If window is
      // moving, adjust the window position. If window is sizing, adjusts
      // the window size.
      if( swp_window.cy != swp->cy ) {
        swp->cy += swp->y - swp_anchor.y - swp_anchor.cy;
      }
      swp->y = swp_anchor.y + swp_anchor.cy;
      docked = TRUE;
    }
    if( docked ) {
      // If window is docked to top or bottom of anchor window, docks its
      // left and right borders. If window is moving, adjust the window
      // position. If window is sizing, adjust the window size.
      if( abs( swp->x - swp_anchor.x ) < margin ) {
        if( swp_window.cx != swp->cx ) {
          swp->cx += swp->x - swp_anchor.x;
        }
        swp->x = swp_anchor.x;
      }
      if( abs(( swp->x + swp->cx ) - ( swp_anchor.x + swp_anchor.cx )) < margin ) {
        if( swp_window.x != swp->x ) {
          swp->x  = swp_anchor.x + swp_anchor.cx - swp->cx;
        } else {
          swp->cx = swp_anchor.x + swp_anchor.cx - swp->x;
        }
      }
    }
  } else if( max( swp->y, swp_anchor.y ) < min( swp->y + swp->cy, swp_anchor.y + swp_anchor.cy )) {
    if( abs( swp->x + swp->cx - swp_anchor.x ) < margin ) {
      // Docking when window to dock is left of anchor window. If window is
      // moving, adjust the window position. If window is sizing, adjust
      // the window size.
      if( swp_window.x != swp->x ) {
        swp->x  = swp_anchor.x - swp->cx;
      } else {
        swp->cx = swp_anchor.x - swp->x;
      }
      docked = TRUE;
    }
    else if( abs( swp->x - ( swp_anchor.x + swp_anchor.cx )) < margin ) {
      // Docking when window to dock is right of anchor window. If window is
      // moving, adjust the window position. If window is sizing, adjust
      // the window size.
      if( swp_window.cx != swp->cx ) {
        swp->cx += swp->x - swp_anchor.x - swp_anchor.cx;
      }
      swp->x = swp_anchor.x + swp_anchor.cx;
      docked = TRUE;
    }
    if( docked ) {
      // If window is docked to left or right of anchor window, docks its
      // top and bottom borders. If window is moving, adjust the window
      // position. If window is sizing, adjust the window size.
      if( abs( swp->y - swp_anchor.y ) < margin ) {
        if( swp_window.cy != swp->cy ) {
          swp->cy += swp->y - swp_anchor.y;
        }
        swp->y = swp_anchor.y;
      }
      if( abs(( swp->y + swp->cy ) - ( swp_anchor.y + swp_anchor.cy )) < margin ) {
        if( swp_window.y != swp->y ) {
          swp->y  = swp_anchor.y + swp_anchor.cy - swp->cy;
        } else {
          swp->cy = swp_anchor.y + swp_anchor.cy - swp->y;
        }
      }
    }
  }
  return docked;
}

/** Returns true if a position is aligned to the specified anchor window. */
static BOOL
dk_is_window_aligned( DK_DATA* anchor, PSWP swp )
{
  SWP swp_anchor;
  WinQueryWindowPos( anchor->hwnd, &swp_anchor );

  if( max( swp->x, swp_anchor.x ) < min( swp->x + swp->cx, swp_anchor.x + swp_anchor.cx )) {
    if( swp->y == swp_anchor.y - swp->cy || swp->y == swp_anchor.y + swp_anchor.cy ) {
      return TRUE;
    }
  } else if( max( swp->y, swp_anchor.y ) < min( swp->y + swp->cy, swp_anchor.y + swp_anchor.cy )) {
    if( swp->x == swp_anchor.x - swp->cx || swp->x == swp_anchor.x + swp_anchor.cx ) {
      return TRUE;
    }
  }
  return FALSE;
}

/** Try to dock the specified window to all possible anchors. */
static void
dk_try_to_dock_window( DK_DATA* window, PSWP swp )
{
  int      i;
  DK_DATA* child;

  if( IS_MASTER( window )) {
    for( i = 0; i < head.childs_count; i++ )
    {
      child = head.childs[i];

      if( !IS_MASTER( child )
          && child != window
          && WinIsWindowVisible( child->hwnd )
          && !dk_is_docked( window, child ))
      {
        if( dk_adjust_window( child, window, swp )) {
          // Docks the master window only if its movement it is finished.
          if( swp->fl & SWP_ACTIVATE ) {
            WinPostMsg( window->hwnd, WM_DOCKWINDOW,
                        MPFROMHWND( child->hwnd ), MPFROMLONG( TRUE  ));
          }
        } else {
          WinPostMsg( window->hwnd, WM_DOCKWINDOW,
                      MPFROMHWND( child->hwnd ), MPFROMLONG( FALSE ));
        }
      }
    }
  } else {
    for( i = 0; i < head.childs_count; i++ )
    {
      child = head.childs[i];

      if( IS_DOCKABLE( child )
          && child != window
          && WinIsWindowVisible( child->hwnd ))
      {
        if( dk_adjust_window( child, window, swp )) {
          WinPostMsg( child->hwnd, WM_DOCKWINDOW,
                      MPFROMHWND( window->hwnd ), MPFROMLONG( TRUE  ));
        } else {
          WinPostMsg( child->hwnd, WM_DOCKWINDOW,
                      MPFROMHWND( window->hwnd ), MPFROMLONG( FALSE ));

          if( dk_is_docked( window, child )) {
            WinPostMsg( window->hwnd, WM_DOCKWINDOW,
                        MPFROMHWND( child->hwnd ), MPFROMLONG( FALSE ));
          }
        }
      }
    }
  }
}

/* Pulls a specified window. */
static void
dk_pull_window( DK_DATA* leader, DK_DATA* window, POINTL pos, LONG fl )
{
  SWP swp = { 0, 0, 0, 0 };
  int i;

  if( window->state & DK_IS_PULLED ) {
    return;
  }

  window->state |= DK_IS_PULLED;

  if( fl & SWP_MOVE ) {
    WinQueryWindowPos( window->hwnd, &swp );
    swp.x += pos.x;
    swp.y += pos.y;
  }

  if(!( fl & SWP_SHOW) || !IS_GHOST( window )) {
    WinSetWindowPos( window->hwnd, leader->hwnd,
                     swp.x, swp.y, 0, 0, fl | SWP_NOADJUST );
  }

  for( i = 0; i < window->childs_count; i++ ) {
    dk_pull_window( window, window->childs[i], pos, fl );
  }
  if( fl & SWP_ZORDER ) {
    for( i = 0; i < window->owners_count; i++ ) {
      dk_pull_window( window, window->owners[i], pos, fl );
    }
  }
}

/* Moves a specified master window. */
static void
dk_move_window( DK_DATA* window, PSWP swp_new, LONG fl )
{
  SWP    swp_old;
  POINTL pos = { 0, 0 };
  int    i;

  window->state |= DK_IS_PULLED;

  if( fl & SWP_MOVE ) {
    WinQueryWindowPos( window->hwnd, &swp_old );

    pos.x = swp_new->x - swp_old.x;
    pos.y = swp_new->y - swp_old.y;
  }

  for( i = 0; i < window->childs_count; i++ ) {
    dk_pull_window( window, window->childs[i], pos, fl );
  }
  if( fl & SWP_ZORDER ) {
    for( i = 0; i < window->owners_count; i++ ) {
      dk_pull_window( window, window->owners[i], pos, fl );
    }
  }

  for( i = 0; i < head.childs_count; i++ ) {
    head.childs[i]->state &= ~DK_IS_PULLED;
  }
}

/* Processes messages of the dockable window. */
static MRESULT EXPENTRY
dk_win_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  DK_DATA* window = dk_get_window( hwnd );

  switch( msg ) {
    case WM_DESTROY:
      if( window ) {
        window->def_proc( hwnd, msg, mp1, mp2 );
        dk_remove_window( hwnd );
      }
      return 0;

    case WM_ACTIVATE:
      if( mp1 && cfg.dock_windows ) {
        dk_move_window( window, NULL, SWP_ZORDER );
      }
      break;

    case WM_DOCKWINDOW:
    {
      DK_DATA* dock = dk_get_window((HWND)mp1);

      if( dock ) {
        if( mp2 ) {
          dk_dock  ( window, dock );
        } else {
          dk_undock( window, dock );
        }
      }
      return 0;
    }

    case WM_ADJUSTWINDOWPOS:
    {
      PSWP swp = (PSWP)mp1;

      if( cfg.dock_windows ) {
        if( swp->fl & ( SWP_MOVE | SWP_SIZE )) {
          dk_try_to_dock_window( window, swp );
        }
        if( IS_MASTER( window )) {
          if( swp->fl & ( SWP_MOVE )) {
            // Moves all windows docked to this master.
            dk_move_window( window, swp, SWP_MOVE );
          }
          if( swp->fl & ( SWP_HIDE )) {
            // Hides all windows docked to this master.
            dk_move_window( window, swp, SWP_HIDE );
          }
          if( swp->fl & ( SWP_SHOW ) && !WinIsWindowVisible( hwnd )) {
            // Shows all windows docked to this master.
            dk_move_window( window, swp, SWP_SHOW );
          }
        } else {
          if( swp->fl & ( SWP_HIDE )) {
            // Cleanups all relationships with this window.
            dk_cleanup( hwnd );
          }
          if( swp->fl & ( SWP_SHOW )) {
            // Rebuilds all relationships with this window
            dk_arrange( hwnd );
          }
        }
      }
      break;
    }
  }

  if( window ) {
    return window->def_proc( hwnd, msg, mp1, mp2 );
  } else {
    return 0;
  }
}

/** Adds a specified window to the docking subsystem. */
BOOL
dk_add_window( HWND hwnd, int state )
{
  DK_DATA* node;

  if( head.childs_count >= DK_MAX_DOCKED ) {
    return FALSE;
  }

  node = (DK_DATA*)malloc( sizeof( DK_DATA ));

  if( !node ) {
    amp_player_error( "Not enough memory." );
    return FALSE;
  }

  memset( node, 0, sizeof( DK_DATA ));

  node->hwnd     = hwnd;
  node->state    = state & ( DK_IS_MASTER | DK_IS_GHOST );
  node->def_proc = WinSubclassWindow( hwnd, dk_win_proc );

  head.childs[ head.childs_count++ ] = node;
  return TRUE;
}

/** Removes a specified window from the docking subsystem. */
BOOL
dk_remove_window( HWND hwnd )
{
  DK_DATA* window = dk_get_window( hwnd );

  if( window ) {
    while( window->childs_count ) {
      dk_undock( window, window->childs[0] );
    }
    while( window->owners_count ) {
      dk_undock( window->owners[0], window );
    }

    dk_remove_data( head.childs, &head.childs_count, window );
    WinSubclassWindow( window->hwnd, window->def_proc );
    free( window );

    return TRUE;
  } else {
    return FALSE;
  }
}

/* Cleanups all relationships with the specified window. */
void
dk_cleanup( HWND hwnd )
{
  DK_DATA* window = dk_get_window( hwnd );

  if( window ) {
    while( window->childs_count ) {
      dk_undock( window, window->childs[0] );
    }
  }
}

/* Rebuilds all relationships with the specified window. */
void
dk_arrange( HWND hwnd )
{
  DK_DATA* window = dk_get_window( hwnd );
  HAB      hab    = WinQueryAnchorBlock( hwnd );
  int      i;
  SWP      swp;
  QMSG     msg;

  if( !window ) {
    return;
  }

  while( WinPeekMsg( hab, &msg, NULLHANDLE,
                     WM_DOCKWINDOW, WM_DOCKWINDOW, PM_REMOVE )) {}

  while( window->childs_count ) {
    dk_undock( window, window->childs[0] );
  }

  WinQueryWindowPos( hwnd, &swp );

  for( i = 0; i < head.childs_count; i++ ) {
    if( head.childs[i] != window && !dk_is_docked( window, head.childs[i] )
                                 && !dk_is_docked( head.childs[i], window ))
    {
      if( dk_is_window_aligned( head.childs[i], &swp )) {
        if( IS_MASTER( window )) {
          if( !IS_MASTER( head.childs[i] )) {
            dk_dock( window, head.childs[i] );
          }
        } else {
          if( IS_DOCKABLE( window )) {
            dk_dock( window, head.childs[i] );
          } else if( IS_DOCKABLE( head.childs[i] )) {
            dk_dock( head.childs[i], window );
          }
        }
      }
    }
  }

  for( i = 0; i < window->childs_count; i++ ) {
    dk_arrange( window->childs[i]->hwnd );
  }
}

/** Initializes of the docking subsystem. */
void
dk_init()
{}

/** Terminates  of the docking subsystem. */
void
dk_term()
{
  while( head.childs_count ) {
    dk_remove_window( head.childs[0]->hwnd );
  }
}
