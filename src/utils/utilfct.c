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
#include <os2.h>
#include "utilfct.h"

APIRET APIENTRY DosQueryModFromEIP( HMODULE *phMod, ULONG *pObjNum, ULONG BuffLen,
                                    PCHAR pBuff, ULONG *pOffset, ULONG Address );
static BOOL have_warpsans = -1;

/* Returns TRUE if WarpSans is supported by operating system. */
BOOL
check_warpsans( void )
{
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
  if( name && name_size > 0 )
  {
    ULONG ObjNum = 0, Offset = 0;

    DosQueryModFromEIP( hmodule, &ObjNum, name_size, name, &Offset, (ULONG)(&getModule));
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
    PTIB ptib;

    DosGetInfoBlocks( &ptib, &ppib );
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
   time expires. */
void
wait_thread( TID tid, ULONG msec )
{
  while( msec > 0 &&
         DosWaitThread( &tid, DCWW_NOWAIT ) == ERROR_THREAD_NOT_TERMINATED )
  {
    DosSleep( 100 );
    msec -= 100;
  }

  if( DosWaitThread( &tid, DCWW_NOWAIT ) == ERROR_THREAD_NOT_TERMINATED ) {
    DosKillThread( tid );
  }
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

