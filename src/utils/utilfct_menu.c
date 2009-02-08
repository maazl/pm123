/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com> *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2009 Marcel Mueller
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

#define  INCL_PM

#include <stdio.h>
#include "utilfct.h"
#include <os2.h>
#include <ctype.h>

#include "debuglog.h"

/* Adds an item into a menu control. */
SHORT
mn_add_item( HWND menu, SHORT id, const char* item, BOOL enable, BOOL check, PVOID handle )
{
  MENUITEM mi;

  mi.iPosition   = MIT_END;
  mi.afStyle     = MIS_TEXT;
  mi.afAttribute = 0;
  mi.hwndSubMenu = NULLHANDLE;
  mi.hItem       = (ULONG)handle;
  mi.id          = id;
  mi.afAttribute = 0;

  if( !enable ) {
    mi.afAttribute |= MIA_DISABLED;
  }
  if( check ) {
    mi.afAttribute |= MIA_CHECKED;
  }

  return SHORT1FROMMR( WinSendMsg( menu, MM_INSERTITEM, MPFROMP( &mi ), MPFROMP( item )));
}

/* Returns the identity of a menu item of a specified index. */
SHORT
mn_item_id( HWND menu, SHORT i )
{
  return SHORT1FROMMR( WinSendMsg( menu, MM_ITEMIDFROMPOSITION,
                                   MPFROMSHORT( i ), 0 ));
}

/* Deletes an item from the menu control. */
SHORT
mn_remove_item( HWND menu, SHORT id )
{
  return SHORT1FROMMR( WinSendMsg( menu, MM_REMOVEITEM,
                                   MPFROM2SHORT( id, TRUE ), 0 ));
}

/* Makes a menu item selectable. */
BOOL
mn_enable_item( HWND menu, SHORT id, BOOL enable )
{
  return LONGFROMMR( WinSendMsg( menu, MM_SETITEMATTR,
                                 MPFROM2SHORT( id,  TRUE ),
                                 MPFROM2SHORT( MIA_DISABLED, enable ? 0 : MIA_DISABLED )));
}

/* Places a a check mark to the left of the menu item. */
BOOL
mn_check_item( HWND menu, SHORT id, BOOL check )
{
  return LONGFROMMR( WinSendMsg( menu, MM_SETITEMATTR,
                                 MPFROM2SHORT( id,  TRUE ),
                                 MPFROM2SHORT( MIA_CHECKED, check ? MIA_CHECKED : 0 )));
}

/* Change a menu item to MS_CONDITIONALCASCADE and sets the default ID */
BOOL mn_make_conditionalcascade( HWND menu, SHORT submenuid, SHORT defaultid )
{
  MENUITEM mi;
  return WinSendMsg( menu, MM_QUERYITEM, MPFROM2SHORT( submenuid, TRUE ), MPFROMP( &mi ))
      && WinSetWindowBits( mi.hwndSubMenu, QWL_STYLE, MS_CONDITIONALCASCADE, MS_CONDITIONALCASCADE )
      && WinSendMsg( mi.hwndSubMenu, MM_SETDEFAULTITEMID, MPFROMLONG( defaultid ), 0 );
}

/* Returns the handle of the specified menu item. */
PVOID
mn_get_handle( HWND menu, SHORT id )
{
  MENUITEM mi;

  if( LONGFROMMR( WinSendMsg( menu, MM_QUERYITEM,
                              MPFROM2SHORT( id, TRUE ), MPFROMP( &mi )))) {
    return (PVOID)mi.hItem;
  } else {
    return NULL;
  }
}

/* Returns the handle of the specified submenu. */
HWND
mn_get_submenu( HWND menu, SHORT id )
{
  MENUITEM mi;

  if( LONGFROMMR( WinSendMsg( menu, MM_QUERYITEM,
                              MPFROM2SHORT( id, TRUE ), MPFROMP( &mi )))) {
    return mi.hwndSubMenu;
  } else {
    return NULLHANDLE;
  }
}


/* Returns a count of the number of items in the menu control. */
SHORT
mn_size( HWND menu ) {
  return SHORT1FROMMR( WinSendMsg( menu, MM_QUERYITEMCOUNT, 0, 0 ));
}
