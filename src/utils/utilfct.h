/*
 * Copyright 2007-2011 M.Mueller
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
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

#ifndef UTILFCT_H
#define UTILFCT_H

/** @brief Presentation parameter type for resizing dialog window controls.
 * The value must be of type PPResizeInfo.
 *
 * @details Resource file syntax:
 * @code PRESPARAMS PPU_RESIZEINFO, pos_resize, size_resize @endcode
 *
 * Each resize value represents two rational numbers, each of it two bytes in size.
 * The numerator is in the high byte, the denominator is in the low byte.
 * The high word is the x component while the low word is the y component.
 *
 * @example
 * @code PRESPARAMS PPU_RESIZEINFO, 0x00010001, 0x01010101 @endcode
 *   Scales the control 1:1 with the parent window. I.e. the left and bottom
 *   coordinates are left unchanged (factor 0/1), whereas the width and height
 *   scale linear with the window size (factor 1/1).
 * @code PRESPARAMS PPU_RESIZEINFO, 0x01010102, 0x00010102 @endcode
 *   Shifts the control with the right border of the parent window and the upper
 *   border with upper window border and resizes the control with 1/2 of the parent
 *   window change.
 * @code PRESPARAMS PPU_RESIZEINFO, 0x01030203, 0x01030103 @endcode
 *   Manages the resizing of a child window that represents the center top rectangle
 *   in a 3x3 grid.
 *
 */
#define PPU_RESIZEINFO      0x8100 // PP_USER + 0x100
/** @brief Convenient way to create \c PPU_RESIZEINFO.
 * @details The Macro takes 8 parameters:
 * - x-numerator, x-denominator,
 * - y-numerator, y-denominator,
 * - cx-numerator, cx-denominator,
 * - cy-numerator, cy-denominator
 * @example @code PRESPARAMS MAKE_PPU_RESIZE_INFO(1,1, 1,2, 0,1, 1,2) @endcode
 *   Shifts the control with the right border of the parent window and the upper
 *   border with upper window border and resizes the control with 1/2 of the parent
 *   window change.
 */
#define MAKE_PPU_RESIZE_INFO(xn,xd,yn,yd,wn,wd,hn,hd) PPU_RESIZEINFO, (xn)*16777216+(xd)*65536+(yn)*256+(yd), (wn)*16777216+(wd)*65536+(hn)*256+(hd)

/** @brief Presentation parameter type for resizing dialog window controls.
 * The value must be of type PPResizeConstr.
 *
 * @details Resource file syntax:
 * @code PRESPARAMS PPU_RESIZECONSTR min_size[, max_size] @endcode
 *
 * min_size are two words with the minimum size of the control. The x limit is
 * in the high word and the y limit in the low word. Any attempt to resize the frame window
 * that causes at least one child window to break this constraint will be adjusted
 * to meet the constraint.
 */
#define PPU_RESIZECONSTR    0x8101 // PP_USER + 0x101
/** @brief Convenient way to create \c PPU_RESIZEINFO.
 * @details The Macro takes 4 parameters:
 * - cx-minimum, cy-minimum,
 * - cx maximum, cy-maximum
 * If the maximum values are zero there is no max constraint.
 * @example @code PRESPARAMS MAKE_PPU_SIZE_CONSTR(10,4,0,0) @endcode
 */
#define MAKE_PPU_SIZE_CONSTR(xmin,ymin,xmax,ymax) PPU_RESIZECONSTR, (xmin)*65536+(ymin), (xmax)*65536+(ymax)

/** Create BTNCDATA structure in resource file.
 * @param checked TRUE = button is checked
 * @param highlight TRUE = button is highlighted
 */
#define CD_BTNCDATA(checked, highlight) 10, checked, highlight, 0,0
/** Create EFDATA structure in resource file.
 * @param length Maximum allowed length of the input
 */
#define CD_EFDATA(length) 8, length, 0, 0
/*** Create SPBCDATA structure in resource file.
 * @param length Maximum allowed length of the input
 * @param low lower limit
 * @param high upper limit
 */
#define CD_SPBCDATA(length, low, high) 24,0, length,0, (low)&0xffff,(low)>>16, (high)&0xffff,(high)>>16, 0,0, 0,0

#ifndef RC_INVOKED

#include <config.h>
#define INCL_PM
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

#ifndef DC_PREPAREITEM
  #define DC_PREPAREITEM   0x0040
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

/** Queries a module handle and name. */
void getModule ( HMODULE* hmodule, char* name, int name_size );
/** Queries a program name. */
void getExeName( char* name, int name_size );

/** Returns TRUE if WarpSans is supported by operating system. */
BOOL check_warpsans( void );
/** Assigns the 9.WarpSans as default font for a specified window if it is supported by
 * operating system. Otherwise assigns the 8.Helv as default font. */
void do_warpsans( HWND hwnd );

/** Places the current thread into a wait state until another thread
 * in the current process has ended. Kills another thread if the
 * time expires and return FALSE. */
BOOL wait_thread( TID tid, ULONG msec );
/** Same as wait_thread, but keep the PM message queue alive. */
BOOL wait_thread_pm( HAB hab, TID tid, ULONG msec );

/** Small rational number */
typedef struct
{ /** Denominator of rational number. */
  UCHAR Denominator;
  /** Numerator of rational number. */
  UCHAR Numerator;
} PPResizeFactor;
/** @brief Presentation parameters for resizing dialog window controls.
 * This Parameter must be of type PPU_RESIZEINFO.
 */
typedef struct
{ /** Resize factor for the bottom border. */
  PPResizeFactor y_resize;
  /** Resize factor for the left border. */
  PPResizeFactor x_resize;
  /** Resize factor for the control height. */
  PPResizeFactor cy_resize;
  /** Resize factor for control width. */
  PPResizeFactor cx_resize;
} PPResizeInfo;

typedef struct
{ /** The minimum height of the control. Any attempt to resize the frame window
   * that causes at least one child window to break this constraint will be adjusted to meet
   * the constraint. */
  USHORT cy_min;
  /** The minimum width of the control. Any attempt to resize the frame window
   * that causes at least one child window to break this constraint will be adjusted to meet
   * the constraint. */
  USHORT cx_min;
  /** The minimum height of the control. Any attempt to resize the frame window
    * that causes at least one child window to break this constraint will be adjusted to meet
    * the constraint. */
  USHORT cy_max;
  /** The minimum width of the control. Any attempt to resize the frame window
   * that causes at least one child window to break this constraint will be adjusted to meet
   * the constraint. */
  USHORT cx_max;
} PPResizeConstr;

/** Adjust the result of a resize operation according to PPU_RESIZEINFO of the children.
 * This function is intended to be used at WM_ADJUSTWINDOWPOS processing.
 * If you do not have size constraints or you have the size constraints
 * at the dialog level, then this call is not necessary. */
void  dlg_adjust_resize(HWND hwnd, SWP* pswp);
/** Resize the children according to PPU_RESIZEINFO.
 * This function is intended to be used at WM_WINDOWPOSCHANGED processing. */
void  dlg_do_resize(HWND hwnd, SWP* pswpnew, SWP* pswpold);

/* Add dialog control at runtime
 * Helpful for controls that fail in resource files like WC_CIRCULARSLIDER */
HWND  dlg_addcontrol( HWND hwnd, PSZ cls, PSZ text, ULONG style,
                      LONG x, LONG y, LONG cx, LONG cy, SHORT after,
                      USHORT id, PVOID ctldata, PVOID presparams );

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
ULONG nb_append_tab( HWND book, HWND page, const char* major, const char* minor, MPARAM index );
/* Adjusting the position and size of a notebook window.
 * This function is superseded by dlg_do_resize in conjunction with PPU_RESIZEINFO. */
BOOL  nb_adjust( HWND hwnd, SWP* pswp );

/* Sets the upper and lower limit of a numeric spin button and the maximum text length */
BOOL  sb_setnumlimits( HWND hwnd, USHORT id, LONG low, LONG high, USHORT len ); 

/* Initialize circular slider control */
BOOL  cs_init( HWND hwnd, USHORT id, LONG low, LONG high, LONG inc, LONG tick, LONG value );

/* This function sets the visibility state of a dialog item. */
#define WinShowDlgItem( hwndDlg, idItem, fNewVisibility ) \
        WinShowWindow( WinWindowFromID( hwndDlg, idItem ), fNewVisibility )

#ifdef __cplusplus
}
#endif

#endif
#endif /* PM123_UTILS_H */
