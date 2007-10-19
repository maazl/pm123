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

#include "playlist.i.h"
#include "playable.h"
#include "bookmark.h"
#include "pm123.h"
#include "pm123.rc.h"

// Default instance of bookmark window, representing BOOKMARK.LST in the program folder.
static PlaylistBase* DefaultBM = NULL;

/****************************************************************************
*
*  public external C interface
*
****************************************************************************/

/* Creates the playlist manager presentation window. */
void bm_create( void )
{ DEBUGLOG(("bm_create()\n"));
  url path = url::normalizeURL(startpath);
  xstring bmurl = path + "BOOKMARK.LST";
  DEBUGLOG(("bm_create - %s\n", bmurl.cdata()));
  DefaultBM = PlaylistView::Get(bmurl, "Bookmarks");
  DefaultBM->SetVisible(cfg.show_bmarks);
}

/* Destroys the playlist manager presentation window. */
void bm_destroy( void )
{ DEBUGLOG(("bm_destroy()\n"));
  DefaultBM = NULL;
  DEBUGLOG(("bm_destroy() - end\n"));
}

/* Sets the visibility state of the playlist manager presentation window. */
void bm_show( BOOL show )
{ DEBUGLOG(("bm_show(%u)\n", show));
  if (DefaultBM)
    DefaultBM->SetVisible(show);
}

Playlist* bm_get( void )
{ return (Playlist*)DefaultBM->GetContent();
}


/* Adds a user selected bookmark. */
void bm_add_bookmark(HWND owner, Playable* item, double pos)
{ DEBUGLOG(("bm_add_bookmark(%x, %p{%s}, %.1f)\n", owner, item, item->GetDisplayName().cdata(), pos));
  // TODO: !!!!!! request information before
  const META_INFO& meta = *item->GetInfo().meta;
  xstring desc = "";
  if (*meta.artist)
    desc = xstring(meta.artist) + "-";
  if (*meta.title)
    desc = desc + meta.title;
   else
    desc = desc + item->GetURL().getShortName();

  HWND hdlg = WinLoadDlg(HWND_DESKTOP, owner, &WinDefDlgProc, NULLHANDLE, DLG_BM_ADD, NULL);
                              
  WinSetDlgItemText(hdlg, EF_BM_DESC, desc);

  if (WinProcessDlg(hdlg) == DID_OK)
  { xstring alias;
    char* cp = alias.raw_init(WinQueryDlgItemTextLength(hdlg, EF_BM_DESC));
    WinQueryDlgItemText(hdlg, EF_BM_DESC, alias.length()+1, cp);
    if (alias == desc)
      alias = NULL; // Don't set alias if not required.
    bm_get()->InsertItem(item->GetURL(), alias, pos); 
    // TODO !!!!!
    //bm_save( owner );
  }

  WinDestroyWindow(hdlg);
}


