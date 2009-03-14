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

#ifndef PM123_UTILS_H
#define PM123_UTILS_H

#include <config.h>
#define INCL_PM
#include <rel2abs.h>
#include <abs2rel.h>
#include <errorstr.h>
#include <minmax.h>
#include <strutils.h>
#include <os2.h>

#ifndef BKS_TABBEDDIALOG
  /* Tabbed dialog. */
  #define BKS_TABBEDDIALOG 0x00000800UL
#endif
#ifndef BKS_BUTTONAREA
  /* Reserve space for buttons. */
  #define BKS_BUTTONAREA   0x00000200UL
#endif

#define TOSTRING(x) #x

#ifndef MRFROMBOOL
  #define MRFROMBOOL(b) ((MRESULT)(BOOL)(b))
#endif
#ifndef BOOLFROMMR
  #define BOOLFROMMR(m) ((BOOL)(m))
#endif
#ifndef MPFROMBOOL
  #define MPFROMBOOL(b) ((MPARAM)(BOOL)(b))
#endif
#ifndef BOOLFROMMP
  #define BOOLFROMMP(m) ((BOOL)(m))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Returns TRUE if WarpSans is supported by operating system. */
BOOL check_warpsans( void );
/* Assigns the 9.WarpSans as default font for a specified window if it is supported by
   operating system. Otherwise assigns the 8.Helv as default font. */
void do_warpsans( HWND hwnd );

/* Queries a module handle and name. */
void getModule ( HMODULE* hmodule, char* name, int name_size );
/* Queries a program name. */
void getExeName( char* name, int name_size );

/* Removes leading and trailing spaces. */
char* blank_strip( char* string );
/* Replaces series of control characters by one space. */
char* control_strip( char* string );
/* Removes leading and trailing spaces and quotes. */
char* quote_strip( char* string );
/* Removes comments starting with "#". */
char* uncomment  ( char* string );

/* Places the current thread into a wait state until another thread
   in the current process has ended. Kills another thread if the
   time expires and return FALSE. */
BOOL  wait_thread( TID tid, ULONG msec );
/* Same as wait_thread, but keep the PM message queue alive. */
BOOL  wait_thread_pm( HAB hab, TID tid, ULONG msec );

/* Adds an item into a menu control. */
SHORT mn_add_item( HWND menu, SHORT id, const char* item, BOOL enable, BOOL check, PVOID handle );
/* Returns the identity of a menu item of a specified index. */
SHORT mn_item_id( HWND menu, SHORT i );
/* Deletes an item from the menu control. */
SHORT mn_remove_item( HWND menu, SHORT id );
/* Makes a menu item selectable. */
BOOL  mn_enable_item( HWND menu, SHORT id, BOOL enable );
/* Places a a check mark to the left of the menu item. */
BOOL  mn_check_item ( HWND menu, SHORT id, BOOL check  );
/* Change a menu item to MS_CONDITIONALCASCADE and sets the default ID */
BOOL  mn_make_conditionalcascade( HWND menu, SHORT submenuid, SHORT defaultid );
/* Returns the handle of the specified menu item. */
PVOID mn_get_handle( HWND menu, SHORT id );
/* Returns the handle of the specified submenu. */
HWND  mn_get_submenu( HWND menu, SHORT id );
/* Returns a count of the number of items in the menu control. */
SHORT mn_size( HWND menu );

/* Delete all the items in the list box. */
BOOL  lb_remove_all( HWND hwnd, SHORT id );
/* Deletes an item from the list box control. */
SHORT lb_remove_item( HWND hwnd, SHORT id, SHORT i );
/* Adds an item into a list box control. */
SHORT lb_add_item( HWND hwnd, SHORT id, const char* item );
/* Queries the indexed item of the list box control. */
SHORT lb_get_item( HWND hwnd, SHORT id, SHORT i, char* item, LONG size );
/* Queries a size the indexed item of the list box control. */
SHORT lb_get_item_size( HWND hwnd, SHORT id, SHORT i );
/* Sets the handle of the specified list box item. */
BOOL  lb_set_handle( HWND hwnd, SHORT id, SHORT i, PVOID handle );
/* Returns the handle of the indexed item of the list box control. */
PVOID lb_get_handle( HWND hwnd, SHORT id, SHORT i );
/* Sets the selection state of an item in a list box. */
BOOL  lb_select( HWND hwnd, SHORT id, SHORT i );
/* Returns the current cursored item. */
SHORT lb_cursored( HWND hwnd, SHORT id );
/* Returns the current selected item. */
SHORT lb_selected( HWND hwnd, SHORT id, SHORT starti );
/* Returns a count of the number of items in the list box control. */
SHORT lb_size( HWND hwnd, SHORT id );
/* Searches an item in a list box control. */
SHORT lb_search( HWND hwnd, SHORT id, SHORT starti, char *item );

/* Return the ID of the selected radiobutton in a group. */
SHORT rb_selected( HWND hwnd, SHORT id );

/* Sets the enable state of the entryfield in the dialog template to the enable flag. */
void  en_enable( HWND hwnd, SHORT id, BOOL enable );

/* append a tabbed dialog page. The index param gives
   the index (low word) and the total (high word) number of subpages (if any).
   Returns the new page ID or 0 on error. */
ULONG nb_append_tab( HWND book, HWND page, const char* major, char* minor, MPARAM index );
/* Adjusting the position and size of a notebook window. */
BOOL  nb_adjust( HWND hwnd, SWP* pswp );

/* Sets the upper and lower limit of a numeric spin button and the maximum text length */
BOOL  sb_setnumlimits( HWND hwnd, USHORT id, LONG low, LONG high, USHORT len); 

/* This function sets the visibility state of a dialog item. */
#define WinShowDlgItem( hwndDlg, idItem, fNewVisibility ) \
        WinShowWindow( WinWindowFromID( hwndDlg, idItem ), fNewVisibility )

#ifdef __cplusplus
}
#endif
#endif /* PM123_UTILS_H */
