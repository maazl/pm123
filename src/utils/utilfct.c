/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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
#define  INCL_PM
#define  INCL_ERRORS

#include <stdio.h>
#include "utilfct.h"
#include <os2.h>
#include <ctype.h>

#include "debuglog.h"

APIRET APIENTRY DosQueryModFromEIP( HMODULE *phMod, ULONG *pObjNum, ULONG BuffLen,
                                    PCHAR pBuff, ULONG *pOffset, ULONG Address );
/* Returns TRUE if WarpSans is supported by operating system. */
BOOL
check_warpsans( void )
{
  static int have_warpsans = -1;

  if( have_warpsans == -1 )
  {
    LONG fontcounter = 0;
    HPS  hps;
    BOOL rc;

    hps = WinGetPS( HWND_DESKTOP );
    rc  = GpiQueryFonts( hps, QF_PUBLIC, "WarpSans", &fontcounter, 0, NULL );
    WinReleasePS( hps );

    have_warpsans = ( rc != 0 && rc != GPI_ALTERROR );
  }

  return have_warpsans;
}

/* Assigns the 9.WarpSans as default font for a specified window if it is supported by
   operating system. Otherwise assigns the 8.Helv as default font. */
void
do_warpsans( HWND hwnd )
{
  char *font = check_warpsans() ? "9.WarpSans" : "8.Helv";
  WinSetPresParam( hwnd, PP_FONTNAMESIZE, strlen( font ) + 1, font );
}

/* Queries a module handle and name. */
void
getModule( HMODULE* hmodule, char* name, int name_size )
{
  ULONG ObjNum, Offset;

  DosQueryModFromEIP( hmodule, &ObjNum, name_size, name, &Offset, (ULONG)(&getModule));
  if( name_size ) {
    DosQueryModuleName( *hmodule, name_size, name );
  }
}

/* Queries a program name. */
void
getExeName( char* name, int name_size )
{
  if( name && name_size > 0 )
  {
    PPIB ppib;

    DosGetInfoBlocks( NULL, &ppib );
    DosQueryModuleName( ppib->pib_hmte, name_size, name );
  }
}

/* Removes leading and trailing spaces. */
char*
blank_strip( char* string )
{
  int   i;
  char* pos = string;

  while( *pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') {
    pos++;
  }

  i = strlen(pos)+1;

  if( pos != string ) {
    memmove( string, pos, i );
  }

  i -= 2;

  while( string[i] == ' ' || string[i] == '\t' || string[i] == '\n' || string[i] == '\r' ) {
    string[i] = 0;
    i--;
  }

  return string;
}

/* Replaces series of control characters by one space. */
char*
control_strip( char* string )
{
  char* s = string;
  char* t = string;

  while( *s ) {
    if( iscntrl( *s )) {
      while( *s && iscntrl( *s )) {
        ++s;
      }
      if( *s ) {
        *t++ = ' ';
      }
    } else {
      *t++ = *s++;
    }
  }
  *t = 0;
  return string;
}

/* Removes leading and trailing spaces and quotes. */
char*
quote_strip( char* string )
{
  int   i, e;
  char* pos = string;

  while( *pos == ' ' || *pos == '\t' || *pos == '\n' ) {
    pos++;
  }

  i = strlen( pos ) - 1;

  for( i = strlen( pos ) - 1; i > 0; i-- ) {
    if( pos[i] != ' ' && pos[i] != '\t' && pos[i] != '\n' ) {
      break;
    }
  }

  if( *pos == '\"' && pos[i] == '\"' )
  {
    pos++;
    i -= 2;
  }

  for( e = 0; e < i + 1; e++ ) {
    string[e] = pos[e];
  }

  string[e] = 0;
  return string;
}

/* Removes comments starting with "#". */
char*
uncomment( char *string )
{
  int  source   = 0;
  BOOL inquotes = FALSE;

  while( string[source] ) {
     if( string[source] == '\"' ) {
       inquotes = !inquotes;
     } else if( string[source] == '#' && !inquotes ) {
       string[source] = 0;
       break;
     }
     source++;
  }
  return string;
}

/* Places the current thread into a wait state until another thread
   in the current process has ended. Kills another thread if the
   time expires and return FALSE. */
BOOL
wait_thread( TID tid, ULONG msec )
{
  while( DosWaitThread( &tid, DCWW_NOWAIT ) == ERROR_THREAD_NOT_TERMINATED )
  { if (msec < 100)
    { // Emergency exit
      DEBUGLOG(("wait_thread: Thread %u has not terminated within time\n", tid));
      DosKillThread( tid );
      return FALSE;
    }
    DosSleep( 100 );
    msec -= 100;
  }
  return TRUE;
}

BOOL
wait_thread_pm( HAB hab, TID tid, ULONG msec )
{
  while( DosWaitThread( &tid, DCWW_NOWAIT ) == ERROR_THREAD_NOT_TERMINATED )
  { QMSG qmsg;
    if (WinPeekMsg(hab, &qmsg, NULLHANDLE, 0, 0, PM_REMOVE))
      WinDispatchMsg( hab, &qmsg );
    else if (msec < 31) // Well, not that exact
    { // Emergency exit
      DEBUGLOG(("wait_thread: Thread %u has not terminated within time\n", tid));
      DosKillThread( tid );
      return FALSE;
    } else
    { DosSleep( 31 );
      msec -= 31;
    }
  }
  return TRUE;
}

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

/* Delete all the items in the list box. */
BOOL
lb_remove_all( HWND hwnd, SHORT id )
{
  return LONGFROMMR( WinSendDlgItemMsg( hwnd, id, LM_DELETEALL, 0, 0 ));
}

/* Deletes an item from the list box control. Returns the number of
   items in the list after the item is deleted. */
SHORT
lb_remove_item( HWND hwnd, SHORT id, SHORT i )
{
  return SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id,  LM_DELETEITEM,
                                          MPFROMSHORT( i ), 0 ));
}

/* Adds an item into a list box control. */
SHORT
lb_add_item( HWND hwnd, SHORT id, const char* item )
{
  return SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, LM_INSERTITEM,
                       MPFROMSHORT( LIT_END ), MPFROMP( item )));
}

/* Queries the indexed item of the list box control. */
SHORT
lb_get_item( HWND hwnd, SHORT id, SHORT i, char* item, LONG size )
{
  return SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, LM_QUERYITEMTEXT,
                                          MPFROM2SHORT( i, size ), MPFROMP( item )));
}

/* Queries a size the indexed item of the list box control. */
SHORT
lb_get_item_size( HWND hwnd, SHORT id, SHORT i )
{
  return SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, LM_QUERYITEMTEXTLENGTH,
                                          MPFROMSHORT( i ), 0 ));
}

/* Sets the handle of the specified list box item. */
BOOL
lb_set_handle( HWND hwnd, SHORT id, SHORT i, PVOID handle )
{
  return LONGFROMMR( WinSendDlgItemMsg( hwnd, id,  LM_SETITEMHANDLE,
                     MPFROMSHORT( i ), MPFROMLONG( handle )));
}

/* Returns the handle of the indexed item of the list box control. */
PVOID lb_get_handle( HWND hwnd, SHORT id, SHORT i )
{
  return (PVOID)( WinSendDlgItemMsg( hwnd, id,  LM_QUERYITEMHANDLE,
                  MPFROMSHORT( i ), 0 ));
}

/* Sets the selection state of an item in a list box. */
BOOL
lb_select( HWND hwnd, SHORT id, SHORT i )
{
  return LONGFROMMR( WinSendDlgItemMsg( hwnd, id,  LM_SELECTITEM,
                     MPFROMSHORT( i ), MPFROMSHORT( TRUE )));
}

/* Returns the current cursored item. */
SHORT
lb_cursored( HWND hwnd, SHORT id )
{
  return SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, LM_QUERYSELECTION,
                       MPFROMSHORT( LIT_CURSOR ), 0 ));
}

SHORT
lb_selected( HWND hwnd, SHORT id, SHORT starti )
{
  return SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, LM_QUERYSELECTION,
                       MPFROMSHORT( starti), 0 ));
}

/* Returns a count of the number of items in the list box control. */
SHORT
lb_size( HWND hwnd, SHORT id )
{
  return SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, LM_QUERYITEMCOUNT, 0, 0 ));
}

/* Searches an item in a list box control. */
SHORT
lb_search( HWND hwnd, SHORT id, SHORT starti, char *item )
{
  return SHORT1FROMMR( WinSendDlgItemMsg( hwnd, id, LM_SEARCHSTRING,
                       MPFROM2SHORT( 0, starti ), MPFROMP( item )));
}

/* Sets the enable state of the entryfield in the dialog template to
   the enable flag. */
void
en_enable( HWND hwnd, SHORT id, BOOL enable )
{
  ULONG bg_color = enable ? SYSCLR_ENTRYFIELD : SYSCLR_DIALOGBACKGROUND;
  HWND  hcontrol = WinWindowFromID( hwnd, id );

  if( hcontrol ) {
    WinSendMsg( hcontrol, EM_SETREADONLY, MPFROMSHORT( !enable ), 0 );
    WinSetPresParam( hcontrol, PP_BACKGROUNDCOLORINDEX, sizeof( bg_color ), &bg_color );
  }
}

/* Append a tabbed dialog page */
ULONG
nb_append_tab(HWND book, HWND page, const char* major, char* minor, MPARAM index)
{ USHORT style = BKA_AUTOPAGESIZE | (SHORT2FROMMP(index) > 1) * (BKA_MINOR|BKA_STATUSTEXTON) | (SHORT1FROMMP(index) <= 1) * BKA_MAJOR;
  BOOKPAGEINFO bi = { sizeof bi };
  ULONG id = LONGFROMMR(WinSendMsg(book, BKM_INSERTPAGE, 0, MPFROM2SHORT(style, BKA_LAST)));
  if (id == 0)
    return 0;
  bi.fl = BFA_PAGEFROMHWND;
  bi.hwndPage = page;
  // set major text if supplied
  if (major)
  { bi.fl |= BFA_MAJORTABTEXT;
    bi.cbMajorTab = strlen(major);
    bi.pszMajorTab = (PSZ)major;
  }
  // set minor text if supplied
  if (minor)
  { bi.fl |= BFA_MINORTABTEXT;
    bi.cbMinorTab = strlen(minor);
    bi.pszMinorTab = (PSZ)minor;
  }
  // set status text if multiple pages
  if (style & BKA_STATUSTEXTON)
  { char buf[32];
    bi.fl |= BFA_STATUSLINE;
    bi.cbStatusLine = sprintf(buf, "Page %u of %u", SHORT1FROMMP(index), SHORT2FROMMP(index));
    bi.pszStatusLine = buf;
  }
  if (!WinSendMsg(book, BKM_SETPAGEINFO, MPFROMLONG(id), MPFROMP(&bi)))
    return 0;
  return id;
}

/* Adjusting the position and size of a notebook window. */
void
nb_adjust( HWND hwnd )
{
  int    buttons_count = 0;
  HWND   notebook = NULLHANDLE;
  HENUM  henum;
  HWND   hnext;
  char   classname[128];
  RECTL  rect;
  POINTL pos[2];

  struct BUTTON {
    HWND hwnd;
    SWP  swp;

  } buttons[32];

  henum = WinBeginEnumWindows( hwnd );

  while(( hnext = WinGetNextWindow( henum )) != NULLHANDLE ) {
    if( WinQueryClassName( hnext, sizeof( classname ), classname ) > 0 ) {
      if( strcmp( classname, "#40" ) == 0 ) {
        notebook = hnext;
      } else if( strcmp( classname, "#3" ) == 0 ) {
        if( buttons_count < sizeof( buttons ) / sizeof( struct BUTTON )) {
          if( WinQueryWindowPos( hnext, &buttons[buttons_count].swp )) {
            if(!( buttons[buttons_count].swp.fl & SWP_HIDE )) {
              buttons[buttons_count].hwnd = hnext;
              buttons_count++;
            }
          }
        }
      }
    }
  }
  WinEndEnumWindows( henum );

  if( !WinQueryWindowRect( hwnd, &rect ) ||
      !WinCalcFrameRect  ( hwnd, &rect, TRUE ))
  {
    return;
  }

  // Resizes notebook window.
  if( notebook != NULLHANDLE )
  {
    pos[0].x = rect.xLeft;
    pos[0].y = rect.yBottom;
    pos[1].x = rect.xRight;
    pos[1].y = rect.yTop;

    if( buttons_count ) {
      WinMapDlgPoints( hwnd, pos, 2, FALSE );
      pos[0].y += 14;
      WinMapDlgPoints( hwnd, pos, 2, TRUE  );
    }

    WinSetWindowPos( notebook, 0, pos[0].x,  pos[0].y,
                                  pos[1].x - pos[0].x, pos[1].y - pos[0].y, SWP_MOVE | SWP_SIZE );
  }

  // Adjust buttons.
  if( buttons_count )
  {
    int total_width = buttons_count * 2;
    int start;
    int i;

    for( i = 0; i < buttons_count; i++ ) {
      total_width += buttons[i].swp.cx;
    }

    start = ( rect.xRight - rect.xLeft + 1 - total_width ) / 2;

    for( i = 0; i < buttons_count; i++ ) {
      WinSetWindowPos( buttons[i].hwnd, 0, start, buttons[i].swp.y, 0, 0, SWP_MOVE );
      WinInvalidateRect( buttons[i].hwnd, NULL, FALSE );
      start += buttons[i].swp.cx + 2;
    }
  }
}

