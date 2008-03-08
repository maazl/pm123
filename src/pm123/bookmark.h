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

#ifndef PM123_BOOKMARK_H
#define PM123_BOOKMARK_H

#define DLG_BOOKMARKS        444
#define CNR_BOOKMARKS FID_CLIENT
#define ACL_BOOKMARKS       8002
#define ICO_BOOKMARK         300

#define DLG_BM_ADD           500
#define ST_BM_DESC           501
#define EF_BM_DESC           502

#define DLG_BM_RENAME        600

#define MNU_BM_RECORD        444
#define MNU_BM_LIST          445

#define IDM_BM_REMOVE        901
#define IDM_BM_RENAME        902
#define IDM_BM_LOAD          903
#define IDM_BM_ADDTOPL       904
#define IDM_BM_RMENU         905
#define IDM_BM_CLEAR         907
#define IDM_BM_ADD           908
#define IDM_BM_LMENU         909

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the bookmarks presentation window. Must be
   called from the main thread. */
HWND bm_create( void );
/* Destroys the bookmarks presentation window. Must be
   called from the main thread. */
void bm_destroy( void );
/* Sets the visibility state of the bookmarks presentation window. */
void bm_show( BOOL show );

/* Changes the bookmarks presentation window colors. */
BOOL bm_set_colors( ULONG, ULONG, ULONG, ULONG );

/* Adds a user selected bookmark of the specified file. */
void bm_add_bookmark( HWND owner, const char* filename, const DECODER_INFO* info, ULONG pos );
/* Adds bookmarks to submenu in the main pop-up menu. */
void bm_add_bookmarks_to_menu( HWND hmenu );
/* Loads bookmark selected via pop-up menu. */
void bm_use_bookmark_from_menu( USHORT id );

#ifdef __cplusplus
}
#endif
#endif /* PM123_BOOKMARK_H */
