/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

#ifndef _UTILFCT_H
#define _UTILFCT_H

#if __cplusplus
extern "C" {
#endif

#include "inimacro.h"
#include "bufstream.h"
#include "find.h"
#include "rel2abs.h"
#include "abs2rel.h"
#include "wildcards.h"
#include "nerrno_str.h"
#include "charset.h"

#ifndef bool // Defined by STLport
  typedef int bool;
  #define true 1
  #define false 0
#endif

void getModule( HMODULE *thisModule, char *thisModuleName, int size );
void getExeName( char *buf, int size );

BOOL check_warpsans( void );
void do_warpsans( HWND hwnd );
/* put in WM_CHAR */
void doControlNavigation( HWND hwnd, MPARAM mp1, MPARAM mp2 );
PFNWP SuperclassWindow( HAB hab, PSZ szWinClass,PFNWP pNewWinProc );

#ifndef  BKS_TABBEDDIALOG
 #define BKS_TABBEDDIALOG 0x00000800  /* Tabbed dialog     */
#endif
#ifndef  BKS_BUTTONAREA
 #define BKS_BUTTONAREA   0x00000200  /* Reserve space for */
#endif

HWND createClientFrame( HAB hab, HWND *hwndClient, char *classname,
             ULONG *frameFlags, PFNWP winproc, char *title );
BOOL loadPosition( HWND hwnd, char *inifilename );
BOOL savePosition( HWND hwnd, char *inifilename );

/* calling thread needs a message queue */
void _System updateError(char *fmt, ...);
void initError(HWND mainhwnd, LONG x, LONG y, LONG cx, LONG cy);
void unInitError(void);

void _System updateStatus(char *fmt, ...);
void initStatus(HWND mainhwnd, LONG x, LONG y, LONG cx, LONG cy);
void unInitStatus(void);

/********************************/
/* notebook page info structure */
/********************************/

typedef struct _NBPAGE
{
    PFNWP    pfnwpDlg;              /* Window procedure address for the dialog */
    PSZ      szStatusLineText;      /* Text to go on status line */
    PSZ      szTabText;             /* Text to go on major tab */
    ULONG    idDlg;                 /* ID of the dialog box for this page */
    BOOL     skip;                  /* skip this page (for major pages with minor pages) */
    USHORT   usTabType;             /* BKA_MAJOR or BKA_MINOR */
    ULONG    ulPageId;              /* notebook page ID */
    HWND     hwnd;                  /* set when page frame is loaded */

} NBPAGE, *PNBPAGE;

HWND createNotebook(HWND hwnd, NBPAGE *nbpage, ULONG pageCount);
BOOL loadNotebookDlg(HWND hwndNotebook, NBPAGE *nbpage, ULONG pageCount);

/* very common PM stuff which I couldn't care less to remember by heart */

BOOL initPM ( HAB* hab, HMQ* hmq );
void runPM  ( HAB  hab );
void closePM( HAB  hab, HMQ  hmq );
HWND loadDlg( HWND parent, PFNWP winproc, ULONG id);

/* check buttons */

USHORT getCheck( HWND hwnd, LONG id );
USHORT setCheck( HWND hwnd, LONG id, USHORT state );

/* general window */

ULONG getText( HWND hwnd, LONG id, char *buffer, LONG size );
BOOL  setText( HWND hwnd, LONG id, char *buffer );
BOOL  enable ( HWND hwnd, LONG id );
BOOL  disable( HWND hwnd, LONG id );

/* listbox */

SHORT getItemCount   ( HWND hwnd, LONG id );
SHORT getItemText    ( HWND hwnd, LONG id, SHORT item, char *buffer, LONG size );
SHORT getItemTextSize( HWND hwnd, LONG id, SHORT item );
SHORT insertItemText ( HWND hwnd, LONG id, SHORT item, char *buffer  );
BOOL  setItemHandle  ( HWND hwnd, LONG id, SHORT item, void *pointer );
void* getItemHandle  ( HWND hwnd, LONG id, SHORT item );
SHORT deleteItem     ( HWND hwnd, LONG id, SHORT item );
SHORT deleteAllItems ( HWND hwnd, LONG id );
SHORT selectItem     ( HWND hwnd, LONG id, SHORT item );
SHORT deSelectItem   ( HWND hwnd, LONG id, SHORT item );
SHORT getSelectItem  ( HWND hwnd, LONG id, SHORT startitem );
SHORT searchItemText ( HWND hwnd, LONG id, SHORT startitem, char *string );

/* containers */

RECORDCORE* allocaRecords    ( HWND hwnd, LONG id, USHORT count, USHORT custom );
BOOL        freeRecords      ( HWND hwnd, LONG id, RECORDCORE *records, ULONG count );
ULONG       insertRecords    ( HWND hwnd, LONG id, RECORDCORE *records, RECORDINSERT *info );
RECORDCORE* searchRecords    ( HWND hwnd, LONG id, RECORDCORE *record, USHORT emphasis );
RECORDCORE* enumRecords      ( HWND hwnd, LONG id, RECORDCORE *record, USHORT cmd );
ULONG       removeRecords    ( HWND hwnd, LONG id, RECORDCORE *records[], ULONG count );
BOOL        getRecordPosition( HWND hwnd, LONG id, RECORDCORE *record, RECTL *pos, ULONG fsExtent );
FIELDINFO*  allocaFieldInfo  ( HWND hwnd, LONG id, USHORT count );
USHORT      insertFieldInfo  ( HWND hwnd, LONG id, FIELDINFO *records, FIELDINFOINSERT *info );
SHORT       removeFieldInfo  ( HWND hwnd, LONG id, FIELDINFO *fields[], USHORT count );
BOOL        setRecordSource  ( HWND hwnd, LONG id, RECORDCORE *record, BOOL on );
BOOL        selectRecord     ( HWND hwnd, LONG id, RECORDCORE *record, BOOL on );
BOOL        selectAllRecords ( HWND hwnd, LONG id, BOOL on );

BOOL removeTitleFromContainer( HWND hwnd, LONG id, char *title );

/* processing of popup menu for containers, fiuf... */

typedef struct
{
  HWND PUMHwnd, CTHwnd;
  RECORDCORE *sourceRecord; /* record at the origin of the popup */

} removeSourceEmphasisInfo;

/* check if the source record is selected, and if so, return TRUE */
BOOL isSourceSelected( HWND hwnd, RECORDCORE *record );
/* sets source emphasis for all selected records */
void setSelectedSourceEmphasis( HWND hwnd, BOOL on );
/* for WM_ENDMENU, use return info from processPopUp */
BOOL removeSourceEmphasis( HWND hwnd, removeSourceEmphasisInfo *info );

/* hwnd = dialog who receives notification
   id = container ID
   record = source record  -  0 for container
   recordIDM = record menu ID
   container IDM = container menu ID
   info = returned info for use with removeSourceEmphasis and others
   use in CN_CONTEXTMENU */
void processPopUp( HWND hwnd, LONG id, RECORDCORE *record,
     LONG recordIDM, LONG containerIDM, removeSourceEmphasisInfo *info );

#define WM_DOCKWINDOW   WM_USER + 600
#define WM_UNDOCKWINDOW WM_USER + 601

/*******************************************/
/* for docking support of frame windows.   */
/* used in WM_ADJUSTWINDOWPOS and such     */
/*******************************************/
BOOL dockWindow( PSWP pswptodock, PSWP pswpanchor, LONG margin );
BOOL open_dir_browser( HWND hwnd, POINTL pos, char *desc, char *path, int size );

#define isrelative(path) (path[1] != ':' && path[0] != '\\' && path[0] != '/')

char *blank_strip(char *string);

/* these functions will encode characters included in "set" using the percent
   sign '%' followed by the ascii value (2 characters in hex) of the
   character, and will be able to decode to the original string later */
void percent_encode( char *string, int size, char *set );
void percent_decode( char *string );

char *quote_strip( char *something );
char *uncomment( char *something );
char *translateChar( char *string, char to[], char from[] );
char *LFN2SFN( char *LFN, char *SFN );

#if __cplusplus
}
#endif
#endif /* _UTILFCT_H */
