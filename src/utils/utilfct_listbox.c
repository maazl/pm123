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

#include "utilfct.h"
#include <os2.h>

#include "debuglog.h"


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
