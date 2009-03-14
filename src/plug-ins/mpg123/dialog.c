/*
 * Copyright 2009 Marcel Mueller
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
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
#define  INCL_DOS
#define  INCL_ERRORS

#include "dialog.h"
#include "mpg123.h"

#include <charset.h>
#include <utilfct.h>
#include <fileutil.h>
#include <decoder_plug.h>
#include <debuglog.h>
#include <snprintf.h>
#include <eautils.h>
#include <debuglog.h>
#include <id3v1.h>
#include <id3v2.h>
#include <os2.h>

#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <ctype.h>

#include <genre.h>


static CH_ENTRY id3v2_ch_list[4]; 

/* Initialize the above list. Must be called once. */
void dialog_init()
{ static char name4[100];
  id3v2_ch_list[ID3V2_ENCODING_ISO_8859_1] = *ch_find(1004);
  id3v2_ch_list[ID3V2_ENCODING_UTF16_BOM]  =
  id3v2_ch_list[ID3V2_ENCODING_UTF16]      = *ch_find(1200);
  id3v2_ch_list[ID3V2_ENCODING_UTF8]       = *ch_find(1208);
  strlcpy(name4, id3v2_ch_list[ID3V2_ENCODING_UTF16_BOM].name, sizeof name4);
  strlcat(name4, " with BOM", sizeof name4);
  id3v2_ch_list[ID3V2_ENCODING_UTF16_BOM].name = name4;
}


/****************************************************************************
 *
 *  Configuration dialog
 *
 ***************************************************************************/

/* populate a listbox with character set items */
static void dlg_populate_charset_listbox( HWND lb, const CH_ENTRY* list, size_t count, ULONG selected )
{ while (count--)
  { SHORT idx = SHORT1FROMMR( WinSendMsg( lb, LM_INSERTITEM, MPFROMSHORT( LIT_END ), MPFROMP( list->name )));
    PMASSERT( idx != LIT_ERROR && idx != LIT_MEMERROR );
    PMRASSERT( WinSendMsg( lb, LM_SETITEMHANDLE, MPFROMSHORT( idx ), MPFROMP( list )));   
    if ( list->codepage == selected )
    { PMRASSERT( WinSendMsg( lb, LM_SELECTITEM, MPFROMSHORT( idx ), MPFROMSHORT( TRUE )));
      selected = (ULONG)-1L; // select only the first matching item 
    }   
    ++list;
  }
}

static CH_ENTRY* dlg_query_charset_listbox( HWND lb )
{ SHORT idx = SHORT1FROMMR( WinSendMsg( lb, LM_QUERYSELECTION, MPFROMSHORT( LIT_FIRST ), 0 ));
  CH_ENTRY* ret;
  if (idx == LIT_NONE)
    return NULL;
  ret = (CH_ENTRY*)PVOIDFROMMR( WinSendMsg( lb, LM_QUERYITEMHANDLE, MPFROMSHORT( idx ), 0 ));
  PMASSERT(ret);
  return ret;
}

static SHORT dlg_set_charset_listbox( HWND lb, ULONG encoding )
{ SHORT idx = SHORT1FROMMR( WinSendMsg( lb, LM_QUERYITEMCOUNT, 0, 0 ));
  while (idx--)
  { CH_ENTRY* ch = (CH_ENTRY*)PVOIDFROMMR( WinSendMsg( lb, LM_QUERYITEMHANDLE, MPFROMSHORT( idx ), 0 ));
    if ( ch->codepage == encoding )
    { PMRASSERT( WinSendMsg( lb, LM_SELECTITEM, MPFROMSHORT( idx ), MPFROMSHORT( TRUE )));
      return idx;
    } 
  }
  return -1;
}

/* Processes messages of the configuration dialog. */
static MRESULT EXPENTRY
cfg_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      switch (tag_read_type)
      {case TAG_READ_ID3V2_AND_ID3V1:
        WinCheckButton( hwnd, RB_2R_PREFER, TRUE );
        goto both;
       case TAG_READ_ID3V1_AND_ID3V2:
        WinCheckButton( hwnd, RB_1R_PREFER, TRUE );
       both:
        WinEnableControl( hwnd, RB_1R_PREFER, TRUE ); 
        WinEnableControl( hwnd, RB_2R_PREFER, TRUE ); 
        WinCheckButton( hwnd, CB_1R_READ,   TRUE );
        WinCheckButton( hwnd, CB_2R_READ,   TRUE );
        break;
       case TAG_READ_ID3V2_ONLY:
        WinCheckButton( hwnd, CB_2R_READ,   TRUE );
        goto one;
       case TAG_READ_ID3V1_ONLY:
        WinCheckButton( hwnd, CB_1R_READ,   TRUE );
       case TAG_READ_NONE:
       one:
        WinCheckButton( hwnd, RB_2R_PREFER, TRUE );
        break;
      }

      WinCheckButton( hwnd, CB_1R_AUTOENCODING, tag_read_id3v1_autoch );
      WinCheckButton( hwnd, RB_1W_UNCHANGED + tag_save_id3v1_type, TRUE );
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_1_ENCODING ), ch_list, ch_list_size - ch_list_dbcs, tag_id3v1_charset );

      WinCheckButton( hwnd, RB_2W_UNCHANGED + tag_save_id3v2_type, TRUE );
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_2R_ENCODING ), ch_list, ch_list_size - ch_list_dbcs, tag_read_id3v2_charset );
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_2W_ENCODING ), id3v2_ch_list, sizeof id3v2_ch_list / sizeof *id3v2_ch_list, 0 );
      PMRASSERT( lb_select( hwnd, CO_2W_ENCODING, tag_save_id3v2_encoding ));

      do_warpsans( hwnd );
      break;
    }

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case CB_1R_READ:
       case CB_2R_READ:
        switch (SHORT2FROMMP(mp1))
        { BOOL both; 
         case BN_CLICKED:
          both = WinQueryButtonCheckstate( hwnd, CB_1R_READ ) && WinQueryButtonCheckstate( hwnd, CB_2R_READ );
          PMRASSERT( WinEnableControl( hwnd, RB_1R_PREFER, both ));
          PMRASSERT( WinEnableControl( hwnd, RB_2R_PREFER, both ));
          return 0;
        }
        break;
       case RB_1R_PREFER:
        switch (SHORT2FROMMP(mp1))
        {case BN_CLICKED:
         case BN_DBLCLICKED:
          WinCheckButton( hwnd, RB_2R_PREFER, FALSE );
          WinCheckButton( hwnd, RB_1R_PREFER, TRUE );
          return 0;
        }
       case RB_2R_PREFER:
        switch (SHORT2FROMMP(mp1))
        {case BN_CLICKED:
         case BN_DBLCLICKED:
          WinCheckButton( hwnd, RB_1R_PREFER, FALSE );
          WinCheckButton( hwnd, RB_2R_PREFER, TRUE );
          return 0;
        }
      }
      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ))
      { case DID_OK:
        { CH_ENTRY* chp;
          SHORT val;
          switch( WinQueryButtonCheckstate( hwnd, CB_1R_READ ) + 2*WinQueryButtonCheckstate( hwnd, CB_2R_READ ))
          {case 0:
            tag_read_type = TAG_READ_NONE;
            break;
           case 1:
            tag_read_type = TAG_READ_ID3V1_ONLY;
            break;
           case 2:
            tag_read_type = TAG_READ_ID3V2_ONLY;
            break;
           default:
            tag_read_type = WinQueryButtonCheckstate( hwnd, RB_1R_PREFER ) ? TAG_READ_ID3V1_AND_ID3V2 : TAG_READ_ID3V2_AND_ID3V1;
            break;
          }
          chp = dlg_query_charset_listbox( WinWindowFromID( hwnd, CO_1_ENCODING ));
          if (chp) // Retain old invalid values
            tag_id3v1_charset       = chp->codepage;
          tag_read_id3v1_autoch   = WinQueryButtonCheckstate( hwnd, CB_1R_AUTOENCODING );
          tag_save_id3v1_type     = (save_id3v1_type)(rb_selected( hwnd, RB_1W_UNCHANGED ) - RB_1W_UNCHANGED);
          tag_save_id3v2_type     = (save_id3v2_type)(rb_selected( hwnd, RB_2W_UNCHANGED ) - RB_2W_UNCHANGED);
          chp = dlg_query_charset_listbox( WinWindowFromID( hwnd, CO_2R_ENCODING ));
          if (chp) // Retain old invalid values
            tag_read_id3v2_charset  = chp->codepage;
          val = lb_selected( hwnd, CO_2W_ENCODING, LIT_FIRST );
          if (val != LIT_NONE) // Retain old invalid values
            tag_save_id3v2_encoding = val;

          save_ini();
        }
        break;
      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Configure plug-in. */
int DLLENTRY
plugin_configure( HWND hwnd, HMODULE module ) {
  WinDlgBox( HWND_DESKTOP, hwnd, cfg_dlg_proc, module, DLG_CONFIGURE, NULL );
  return 0;
}

/****************************************************************************
 *
 *  ID3 tag editor
 *
 ***************************************************************************/

typedef enum
{ WR_UPDATE,
  WR_UNCHANGED,
  WR_CLEAN,
  WR_DELETE
} write_mode;

/* Working structure of the window procedures below */
typedef struct
{ const ID3V1_TAG* tagv1_old;
  const ID3V2_TAG* tagv2_old; // NULL = not exists
  ULONG            encoding_tagv1_old;
  ULONG            encoding_tagv2_old;
  ID3V1_TAG        tagv1;
  ID3V2_TAG*       tagv2;
  ULONG            encoding_tagv1;
  ULONG            encoding_tagv2;
  write_mode       write_tagv1; // update, unchanged, delete
  write_mode       write_tagv2; // update, unchanged, clean, delete
  BOOL             autowrite_tagv1;
  BOOL             autowrite_tagv2;
  BOOL             page_ready;
  unsigned         modified; // modification flags of id3all_page dialog.
} ID3_EDIT_TAGINFO;

enum
{ // Load control content from id3_edit_taginfo structure
  UM_LOAD = WM_USER +1, // mp1 = USHORT 0: current, 1: original, 2: clear, 3: copy
  // Store control content to id3_edit_taginfo structure
  UM_STORE,             // -> NULL: OK, != NULL: error message
  // View error message
  UM_ERROR,             // mp1 = error, mp2 = caption
  // Recalculate write mode automatically
  UM_AUTOMODE
}; 

/* Insert the predefined genre items into the genre listbox or combobox. */
static void id3v1_populate_genres( HWND hwnd )
{ SHORT lb = WinWindowFromID( hwnd, CO_GENRE );
  int i;
  SHORT idx = SHORT1FROMMR(WinSendMsg( lb, LM_INSERTITEM, MPFROMSHORT( LIT_END ), MPFROMP( "" ))); // none
  PMASSERT( idx != LIT_ERROR && idx != LIT_MEMERROR );
  for( i = 0; i <= GENRE_LARGEST; i++)
  { idx = SHORT1FROMMR(WinSendMsg( lb, LM_INSERTITEM, MPFROMSHORT( LIT_SORTASCENDING ), MPFROMP( genres[i] )));
    PMASSERT( idx != LIT_ERROR && idx != LIT_MEMERROR );
    PMRASSERT(WinSendMsg( lb, LM_SETITEMHANDLE, MPFROMSHORT( idx ), MPFROMLONG( i )));
  }
}

static void dlg_set_text_if_empty( HWND hwnd, SHORT id, const char* text )
{ HWND ctrl = WinWindowFromID(hwnd, id);
  if ( text && *text && WinQueryWindowTextLength( ctrl ) == 0 )
    PMRASSERT( WinSetWindowText( ctrl, (PSZ)text ));
}

/* Load the data from a V1.x tag into the dialog page. */
static void id3v1_load( HWND hwnd, const ID3V1_TAG* data, ULONG codepage )
{ 
  if (data)
  { char buf[32];
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_TITLE,  id3v1_get_string(data, ID3V1_TITLE,   buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_ARTIST, id3v1_get_string(data, ID3V1_ARTIST,  buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_ALBUM,  id3v1_get_string(data, ID3V1_ALBUM,   buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_TRACK,  id3v1_get_string(data, ID3V1_TRACK,   buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_DATE,   id3v1_get_string(data, ID3V1_YEAR,    buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, CO_GENRE,  id3v1_get_string(data, ID3V1_GENRE,   buf, sizeof buf, codepage));
    *buf = 0;
    dlg_set_text_if_empty(hwnd, EN_COMMENT,id3v1_get_string(data, ID3V1_COMMENT, buf, sizeof buf, codepage));
  }
}

/* Store the data into a V1.x tag. */
static void id3v1_store( HWND hwnd, ID3V1_TAG* data, unsigned what, ULONG codepage )
{ 
  char buf[32];
  if (what & (1 << (EN_TITLE   - EN_TITLE)))
  { WinQueryDlgItemText(hwnd, EN_TITLE,   sizeof buf, buf);
    id3v1_set_string(data, ID3V1_TITLE,   buf, codepage);
  }
  if (what & (1 << (EN_ARTIST  - EN_TITLE)))
  { WinQueryDlgItemText(hwnd, EN_ARTIST,  sizeof buf, buf);
    id3v1_set_string(data, ID3V1_ARTIST,  buf, codepage);
  }
  if (what & (1 << (EN_ALBUM   - EN_TITLE)))
  { WinQueryDlgItemText(hwnd, EN_ALBUM,   sizeof buf, buf);
    id3v1_set_string(data, ID3V1_ALBUM,   buf, codepage);
  }
  if (what & (1 << (EN_TRACK   - EN_TITLE)))
  { WinQueryDlgItemText(hwnd, EN_TRACK,   sizeof buf, buf);
    id3v1_set_string(data, ID3V1_TRACK,   buf, codepage);
  }
  if (what & (1 << (EN_DATE    - EN_TITLE)))
  { WinQueryDlgItemText(hwnd, EN_DATE,    sizeof buf, buf);
    id3v1_set_string(data, ID3V1_YEAR,    buf, codepage);
  }
  if (what & (1 << (CO_GENRE   - EN_TITLE)))
  { WinQueryDlgItemText(hwnd, CO_GENRE,   sizeof buf, buf);
    id3v1_set_string(data, ID3V1_GENRE,   buf, codepage);
  }
  if (what & (1 << (EN_COMMENT - EN_TITLE)))
  { WinQueryDlgItemText(hwnd, EN_COMMENT, sizeof buf, buf);
    id3v1_set_string(data, ID3V1_COMMENT, buf, codepage);
  }
}

static void apply_id3v2_string( HWND hwnd, SHORT id, ID3V2_FRAME* frame, size_t applen, ULONG codepage)
{ HWND ctrl;
  ULONG len;
  char buf[256];

  if (!frame)
    return;
  ctrl = WinWindowFromID(hwnd, id);
  if (ctrl == NULLHANDLE) // ignore non-existing controls
    return;
  len = WinQueryWindowTextLength(ctrl);
  if (len != 0 && len != applen)
    return;
  id3v2_get_string_ex(frame, buf, sizeof buf, codepage);
  if (len)
  { char buf2[32];
    PMRASSERT(WinQueryWindowText(ctrl, sizeof buf2, buf2));
    if (strncmp(buf, buf2, applen))
      return;
  }
  PMRASSERT(WinSetWindowText(ctrl, buf));
}

/* Load the data from a V2.x tag into the dialog page. */
static void id3v2_load( HWND hwnd, const ID3V2_TAG* tag, ULONG codepage )
{
  ID3V2_FRAME* frame;
  char buf[256];
  int  i;

  if (!tag)
    return;

  apply_id3v2_string(hwnd, EN_TITLE,  id3v2_get_frame(tag, ID3V2_TIT2, 1), 30, codepage);
  apply_id3v2_string(hwnd, EN_ARTIST, id3v2_get_frame(tag, ID3V2_TPE1, 1), 30, codepage);
  apply_id3v2_string(hwnd, EN_ALBUM,  id3v2_get_frame(tag, ID3V2_TALB, 1), 30, codepage);
  // Comments
  for( i = 1; (frame = id3v2_get_frame(tag, ID3V2_COMM, i)) != NULL ; i++)
  { char buffer[32];
    id3v2_get_description(frame, buf, sizeof buffer);
    // Skip iTunes specific comment tags.
    if( strnicmp( buffer, "iTun", 4 ) != 0 )
    { apply_id3v2_string(hwnd, EN_COMMENT, frame, WinQueryDlgItemTextLength(hwnd, EN_TRACK) ? 28 : 30, codepage);
      break;
    }
  }
  apply_id3v2_string(hwnd, EN_TRACK,    id3v2_get_frame(tag, ID3V2_TRCK, 1), 0, codepage);
  apply_id3v2_string(hwnd, EN_DATE,     id3v2_get_frame(tag, ID3V2_TDRC, 1), 4, codepage);
  apply_id3v2_string(hwnd, CO_GENRE,    id3v2_get_frame(tag, ID3V2_TCON, 1), 0, codepage);
  apply_id3v2_string(hwnd, EN_COPYRIGHT,id3v2_get_frame(tag, ID3V2_TCOP, 1), 0, codepage);
}

static void update_id3v2_string( HWND hwnd, SHORT id, ID3V2_TAG* tag, uint32_t type, uint8_t encoding )
{ char buf[256];
  HWND ctrl = WinWindowFromID(hwnd, id);
  ID3V2_FRAME* frame = id3v2_get_frame(tag, type, 1);
  char* descr;
  // skip iTunes comments
  if (type == ID3V2_COMM)
  { int i = 1;
    while ( frame
      && (descr = id3v2_get_description(frame, buf, sizeof buf)) != NULL
      && strnicmp( buf, "iTun", 4 ) != 0 )
      frame = id3v2_get_frame(tag, type, ++i);
  }
  if (WinQueryWindowText(ctrl, sizeof buf, buf))
  { // non-empty
    if (!frame)
      frame = id3v2_add_frame(tag, type);
    id3v2_set_string(frame, buf, encoding);
  } else
  { // empty
    if (frame)
      id3v2_delete_frame(frame);
  }
}

/* Store the data from a V2.x tag. */
static void id3v2_store( HWND hwnd, ID3V2_TAG* tag, unsigned what, uint8_t encoding )
{ if (what & (1 << (EN_TITLE     - EN_TITLE)))
    update_id3v2_string( hwnd, EN_TITLE,    tag, ID3V2_TIT2, encoding);
  if (what & (1 << (EN_ARTIST    - EN_TITLE)))
    update_id3v2_string( hwnd, EN_ARTIST,   tag, ID3V2_TPE1, encoding);
  if (what & (1 << (EN_ALBUM     - EN_TITLE)))
    update_id3v2_string( hwnd, EN_ALBUM,    tag, ID3V2_TALB, encoding);
  if (what & (1 << (EN_COMMENT   - EN_TITLE)))
    update_id3v2_string( hwnd, EN_COMMENT,  tag, ID3V2_COMM, encoding);
  if (what & (1 << (EN_TRACK     - EN_TITLE)))
    update_id3v2_string( hwnd, EN_TRACK,    tag, ID3V2_TRCK, encoding);
  if (what & (1 << (EN_DATE      - EN_TITLE)))
    update_id3v2_string( hwnd, EN_DATE,     tag, ID3V2_TDRC, encoding);
  if (what & (1 << (CO_GENRE     - EN_TITLE)))
    update_id3v2_string( hwnd, CO_GENRE,    tag, ID3V2_TCON, encoding);
  if (what & (1 << (EN_COPYRIGHT - EN_TITLE)))
    update_id3v2_string( hwnd, EN_COPYRIGHT,tag, ID3V2_TCOP, encoding);
}

static write_mode automode_id3v1(HWND hwnd)
{ switch (tag_save_id3v1_type)
  {case TAG_SAVE_ID3V1_UNCHANGED:
    return WR_UNCHANGED;
    
   case TAG_SAVE_ID3V2_DELETE:
    return WR_DELETE;

   case TAG_SAVE_ID3V1_NOID3V2:
    { HWND ctrl = WinWindowFromID(hwnd, RB_UPDATE2);
      if (ctrl != NULLHANDLE)
      { if (WinSendMsg(ctrl, BM_QUERYCHECK, 0, 0)) 
          return WR_CLEAN;
      } else
      { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
        if (data->write_tagv2 == WR_UPDATE)
          return WR_CLEAN;
      }
    }    
   default: //case TAG_SAVE_ID3V1_WRITE:
    return WinQueryDlgItemTextLength(hwnd, EN_TITLE)
        || WinQueryDlgItemTextLength(hwnd, EN_ARTIST)
        || WinQueryDlgItemTextLength(hwnd, EN_ALBUM)
        || WinQueryDlgItemTextLength(hwnd, EN_COMMENT)
        || WinQueryDlgItemTextLength(hwnd, EN_TRACK)
        || WinQueryDlgItemTextLength(hwnd, EN_DATE)
        || WinQueryDlgItemTextLength(hwnd, CO_GENRE)
      ? WR_UPDATE : WR_CLEAN;
  }
}

static write_mode automode_id3v2(HWND hwnd)
{ char buf[32];
  ULONG len, len2;
  switch (tag_save_id3v2_type)
  {case TAG_SAVE_ID3V2_UNCHANGED:
    return WR_UNCHANGED;

   case TAG_SAVE_ID3V2_DELETE:
    return WR_DELETE;

   default: //case TAG_SAVE_ID3V2_WRITE:
    return WinQueryDlgItemTextLength(hwnd, EN_TITLE)
        || WinQueryDlgItemTextLength(hwnd, EN_ARTIST)
        || WinQueryDlgItemTextLength(hwnd, EN_ALBUM)
        || WinQueryDlgItemTextLength(hwnd, EN_COMMENT)
        || WinQueryDlgItemTextLength(hwnd, EN_TRACK)
        || WinQueryDlgItemTextLength(hwnd, EN_DATE)
        || WinQueryDlgItemTextLength(hwnd, CO_GENRE)
        || WinQueryDlgItemTextLength(hwnd, EN_COPYRIGHT)
      ? WR_UPDATE : WR_CLEAN;

   case TAG_SAVE_ID3V2_ONDEMAND:
    if ( WinQueryDlgItemTextLength(hwnd, EN_TITLE ) > 30
      || WinQueryDlgItemTextLength(hwnd, EN_ARTIST) > 30
      || WinQueryDlgItemTextLength(hwnd, EN_ALBUM ) > 30
      || (WinQueryDlgItemText(hwnd, EN_TRACK, sizeof buf, buf) > 0 && strchr(buf, '/'))
      || (len = WinQueryDlgItemTextLength(hwnd, EN_COMMENT)) > 30 || (*buf && len > 28) )
      return WR_UPDATE;
    goto stdchk;

   case TAG_SAVE_ID3V2_ONDEMANDSPC:
    if ( WinQueryDlgItemText(hwnd, EN_TITLE,  sizeof buf, buf) > 30 || !ascii_check(buf)
      || WinQueryDlgItemText(hwnd, EN_ARTIST, sizeof buf, buf) > 30 || !ascii_check(buf)
      || WinQueryDlgItemText(hwnd, EN_ALBUM,  sizeof buf, buf) > 30 || !ascii_check(buf)
      || ((len2 = WinQueryDlgItemText(hwnd, EN_TRACK, sizeof buf, buf)) > 0 && strchr(buf, '/'))
      || (len = WinQueryDlgItemText(hwnd, EN_COMMENT, sizeof buf, buf)) > 30 || (len2 && len > 28) || !ascii_check(buf) )
      return WR_UPDATE;
   stdchk:
    return WinQueryDlgItemTextLength(hwnd, EN_DATE) > 4
        || (WinQueryDlgItemText(hwnd, CO_GENRE,  sizeof buf, buf) && lb_selected(hwnd, CO_GENRE, LIT_FIRST) == -1)
        || WinQueryDlgItemTextLength(hwnd, EN_COPYRIGHT)
      ? WR_UPDATE : WR_CLEAN;
  }
}

/* Strip leadion and trailing spaces from the window text */
static void dlg_strip_text( HWND ctrl )
{ char   buf[256];
  size_t len = WinQueryWindowText( ctrl, sizeof buf, buf );
  if (len)
  { char*  cp = buf+len;
    while (cp != buf && isblank(*--cp))
      *cp = 0;
    cp = buf;
    while (cp[0] && isblank(cp[0]))
      ++cp;
    if (buf[len-1] == 0 || cp != buf)
      PMRASSERT(WinSetWindowText(ctrl, cp));
  }
}

/* Strip leading and trailing spaces from all ID3 input fields */
static void id3_strip_all( HWND hwnd )
{ dlg_strip_text(WinWindowFromID(hwnd, EN_TITLE));
  dlg_strip_text(WinWindowFromID(hwnd, EN_ARTIST));
  dlg_strip_text(WinWindowFromID(hwnd, EN_ALBUM));
  dlg_strip_text(WinWindowFromID(hwnd, EN_TRACK));
  dlg_strip_text(WinWindowFromID(hwnd, EN_DATE));
  dlg_strip_text(WinWindowFromID(hwnd, CO_GENRE));
  dlg_strip_text(WinWindowFromID(hwnd, EN_COMMENT));
}

static const char* id3_validatetrack( HWND hwnd, unsigned* track, unsigned* total )
{ HWND ctrl = WinWindowFromID( hwnd, EN_TRACK );
  char   buf[8];
  size_t len = WinQueryWindowText( ctrl, sizeof buf, buf );
  if (len)
  { size_t n;
    if (total)
    { *total = 0;
      if (!sscanf(buf, "%u%n/%u%n", track, &n, total, &n) || n != len)
        return "The track number is not numeric or not in the format '1/2'";
    } else
    { if (!sscanf(buf, "%u%n", track, &n) || n != len)
        return "The track number is not numeric.";
    }
  }
  return NULL;
}

/* Validate ID3 date input.
 * Accepts "", "yyyy", "yyyy-[m]m", "yyyy-[m]m-[d]d", "[d]d.[m]m.yyyy".
 */
static const char* id3_validatedate( HWND hwnd )
{ static const char* formats[] = { "%04u", "%04u-%02u", "%04u-%02u-%02u" };
  HWND   ctrl = WinWindowFromID( hwnd, EN_DATE );
  char   buf[16];
  size_t len = WinQueryWindowText( ctrl, sizeof buf, buf );
  unsigned y, m, d;
  size_t n = (size_t)-1;
  int cnt;
  if (len == 0)
    return NULL;

  // syntax
  cnt = sscanf(buf, "%u%n-%u%n-%u%n", &y, &n, &m, &n, &d, &n);
  if (n != len)
  { // second try with d.m.y
    cnt = sscanf(buf, "%u.%u.%u%n", &d, &m, &y, &n);
    if (n != len)
      return MRFROMP("The date syntax is not valid.\n"
                     "Accepted formats are yyyy, yyyy-mm, yyyy-mm-dd and dd.mm.yyyy.");
  }
  // date values
  switch (cnt)
  {case 3:
    if ( d < 1 || d > 31
      || ((((9*m) >> 3) ^ d) & 1)
      || (m == 2 && (d & 3) > !(y & 3)) )
      return MRFROMP("The date does not exist.\n"
                     "Accepted formats are yyyy, yyyy-mm, yyyy-mm-dd and dd.mm.yyyy.");
   case 2:
    if (m < 1 || m > 12)
      return MRFROMP("The month is out of range.\n"
                     "Accepted formats are yyyy, yyyy-mm, yyyy-mm-dd and dd.mm.yyyy.");
   case 1:
    if (y < 100 || y > 9999)
      return MRFROMP("The release year is invalid. Remeber to use four digits.\n"
                     "Accepted formats are yyyy, yyyy-mm, yyyy-mm-dd and dd.mm.yyyy.");
  }
  // write date
  sprintf(buf, formats[cnt-1], y, m, d);
  PMRASSERT(WinSetWindowText(ctrl, buf));

  return NULL;
}

static MRESULT id3_page_dlg_base_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_COMMAND:
      switch ( COMMANDMSG(&msg)->cmd )
      { case PB_UNDO:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 1 ), 0 ));
          break;

        case PB_CLEAR:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 2 ), 0 ));
          break;

        case PB_COPY:
          PMRASSERT( WinPostMsg( hwnd, UM_LOAD, MPFROMSHORT( 3 ), 0 ));
          break;
      }
      return 0;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the dialog of edition of ID3 tag. */
static MRESULT EXPENTRY
id3all_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ 
  DEBUGLOG2(("mpg123:id3all_page_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  {
    case WM_INITDLG:
      id3v1_populate_genres( hwnd );
      break;

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case EN_TITLE:
       case EN_ARTIST:
       case EN_ALBUM:
       case EN_TRACK:
       case EN_DATE:
       case CO_GENRE:
       case EN_COMMENT:
       case EN_COPYRIGHT:
        if (SHORT2FROMMP(mp1) == EN_CHANGE)
        { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
          if (data->page_ready)
          { QMSG qmsg;
            data->modified |= 1 << (SHORT1FROMMP(mp1) - EN_TITLE);
            if (!WinPeekMsg(WinQueryAnchorBlock(hwnd), &qmsg, hwnd, UM_AUTOMODE, UM_AUTOMODE, PM_NOREMOVE))
              PMRASSERT(WinPostMsg(hwnd, UM_AUTOMODE, 0, 0));
            WinEnableControl(hwnd, RB_UPDATE, TRUE);
            WinEnableControl(hwnd, RB_UPDATE2, TRUE);
          }
        }
        break;

       case RB_UPDATE:
       case RB_UNCHANGED:
       case RB_DELETE:
        switch (SHORT2FROMMP(mp1))
        {case BN_CLICKED:
         case BN_DBLCLICKED:
          WinCheckButton(hwnd, CB_AUTOWRITE, FALSE);
        }
        break;

       case RB_UPDATE2:
       case RB_UNCHANGED2:
       case RB_CLEAN2:
       case RB_DELETE2:
        switch (SHORT2FROMMP(mp1))
        { case BN_CLICKED:
          case BN_DBLCLICKED:
          { QMSG qmsg;
            WinCheckButton(hwnd, CB_AUTOWRITE2, FALSE);
            if (!WinPeekMsg(WinQueryAnchorBlock(hwnd), &qmsg, hwnd, UM_AUTOMODE, UM_AUTOMODE, PM_NOREMOVE))
              PMRASSERT(WinPostMsg(hwnd, UM_AUTOMODE, 0, 0));
          }
        }
        break;
       
       case CB_AUTOWRITE:
       case CB_AUTOWRITE2:
        switch (SHORT2FROMMP(mp1))
        {case BN_CLICKED:
          if (WinSendMsg(HWNDFROMMP(mp2), BM_QUERYCHECK, 0, 0))
          { QMSG qmsg;
            if (!WinPeekMsg(WinQueryAnchorBlock(hwnd), &qmsg, hwnd, UM_AUTOMODE, UM_AUTOMODE, PM_NOREMOVE))
              PMRASSERT(WinPostMsg(hwnd, UM_AUTOMODE, 0, 0)); 
          }
        }
        break;
      };
      return 0;
      
    case WM_COMMAND:
      switch ( COMMANDMSG(&msg)->cmd )
      { case PB_COPY:
        { // Synchronize tags
          ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
          QMSG qmsg;
          data->modified = ~0;
          WinEnableControl(hwnd, RB_UPDATE, TRUE);
          WinEnableControl(hwnd, RB_UPDATE2, TRUE);
          WinCheckButton(hwnd, CB_AUTOWRITE, TRUE);
          WinCheckButton(hwnd, CB_AUTOWRITE2, TRUE);
          if (!WinPeekMsg(WinQueryAnchorBlock(hwnd), &qmsg, hwnd, UM_AUTOMODE, UM_AUTOMODE, PM_NOREMOVE))
            PMRASSERT(WinPostMsg(hwnd, UM_AUTOMODE, 0, 0)); 
          return 0;
        }
      }
      break;

    case UM_LOAD:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const ID3V1_TAG* tagv1;
      const ID3V2_TAG* tagv2;
      ULONG encv1;
      ULONG encv2;
      data->page_ready = FALSE;
      // clear old values to have a clean start
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TITLE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ARTIST,  ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ALBUM,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TRACK,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_DATE,    ""));
      PMRASSERT(WinSetDlgItemText(hwnd, CO_GENRE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_COMMENT, ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_COPYRIGHT, ""));
      switch (SHORT1FROMMP(mp1)) // mode
      {case 0: // load current
        WinCheckButton(hwnd, RB_UPDATE+data->write_tagv1, TRUE);
        WinCheckButton(hwnd, RB_UPDATE2+data->write_tagv2, TRUE);
        WinEnableControl(hwnd, RB_UPDATE, !id3v1_is_clean_tag(&data->tagv1));
        WinEnableControl(hwnd, RB_UPDATE2, data->tagv2->id3_frames_count != 0);
        WinCheckButton(hwnd, CB_AUTOWRITE, data->autowrite_tagv1);
        WinCheckButton(hwnd, CB_AUTOWRITE2, data->autowrite_tagv2);
        tagv1 = &data->tagv1;
        encv1 = data->encoding_tagv1;
        tagv2 = data->tagv2;
        encv2 = data->encoding_tagv2;
        data->modified = 0;
        break;
       case 1: // load original
        tagv1 = data->tagv1_old;
        encv1 = data->encoding_tagv1_old;
        tagv2 = data->tagv2_old;
        encv2 = data->encoding_tagv2_old;
        data->modified = 0;
        break;
       case 2: // reset
        tagv1 = NULL;
        encv1 = tag_id3v1_charset;
        tagv2 = NULL;
        encv2 = tag_read_id3v2_charset;
        data->modified = ~0;
        WinCheckButton(hwnd, RB_DELETE, TRUE);
        WinCheckButton(hwnd, RB_CLEAN2, TRUE);
        WinEnableControl(hwnd, RB_UPDATE, FALSE);
        WinEnableControl(hwnd, RB_UPDATE2, FALSE);
      }
      switch (tag_read_type)
      {case TAG_READ_ID3V2_AND_ID3V1:
        id3v2_load( hwnd, tagv2, encv2 );
       case TAG_READ_ID3V1_ONLY:
        id3v1_load( hwnd, tagv1, encv1 );
        break;
       case TAG_READ_ID3V1_AND_ID3V2:
        id3v1_load( hwnd, tagv1, encv1 );
       case TAG_READ_ID3V2_ONLY:
        id3v2_load( hwnd, tagv2, encv2 );
       case TAG_READ_NONE:
        break;
      }
      data->page_ready = TRUE;
      PMRASSERT(WinPostMsg( hwnd, UM_AUTOMODE, 0, 0));
      return 0;
    }
    case UM_STORE:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const char* msg;
      unsigned track, total;
      SHORT s;
      data->page_ready = FALSE;
      id3_strip_all( hwnd );
      // validate
      msg = id3_validatetrack( hwnd, &track, &total );
      if (msg)
        return MRFROMP(msg);
      msg = id3_validatedate( hwnd );
      if (msg)
        return MRFROMP(msg);
      // store
      id3v1_store( hwnd, &data->tagv1, data->modified, data->encoding_tagv1 );
      id3v2_store( hwnd, data->tagv2,  data->modified, tag_save_id3v2_encoding );
      // store control infos
      s = SHORT1FROMMR(WinSendDlgItemMsg(hwnd, RB_UPDATE, BM_QUERYCHECKINDEX, 0, 0));
      PMASSERT(s != -1);
      data->write_tagv1 = (write_mode)s;
      s = SHORT1FROMMR(WinSendDlgItemMsg(hwnd, RB_UPDATE2, BM_QUERYCHECKINDEX, 0, 0));
      PMASSERT(s != -1);
      data->write_tagv2 = (write_mode)s;
      data->autowrite_tagv1 = WinQueryButtonCheckstate(hwnd, CB_AUTOWRITE);
      data->autowrite_tagv2 = WinQueryButtonCheckstate(hwnd, CB_AUTOWRITE2);
      return 0; // OK
    }
    case UM_AUTOMODE:
      if (WinQueryButtonCheckstate(hwnd, CB_AUTOWRITE2))
        WinCheckButton(hwnd, RB_UPDATE2 + automode_id3v2(hwnd), TRUE);
      if (WinQueryButtonCheckstate(hwnd, CB_AUTOWRITE))
        WinCheckButton(hwnd, RB_UPDATE + automode_id3v1(hwnd), TRUE);
      return 0;
  }
  return id3_page_dlg_base_proc( hwnd, msg, mp1, mp2 );
}

static MRESULT EXPENTRY
id3v1_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ DEBUGLOG2(("mpg123:id3v1_page_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  { case WM_INITDLG:
      id3v1_populate_genres( hwnd );
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_ENCODING ),  ch_list, ch_list_size - ch_list_dbcs, 0 );
      break;

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case EN_TITLE:
       case EN_ARTIST:
       case EN_ALBUM:
       case EN_TRACK:
       case EN_DATE:
       case CO_GENRE:
       case EN_COMMENT:
        if (SHORT2FROMMP(mp1) == EN_CHANGE)
        { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
          if (data->page_ready)
          { data->autowrite_tagv1 = FALSE;
            WinCheckButton(hwnd, RB_UPDATE, TRUE);
          }
        }
        break;

       case RB_UPDATE:
       case RB_UNCHANGED:
       case RB_DELETE:
        switch (SHORT2FROMMP(mp1))
        { case BN_CLICKED:
          case BN_DBLCLICKED:
          { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
            data->autowrite_tagv1 = FALSE;
          }
        }
        break;
      };
      break;

    case UM_LOAD:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const ID3V1_TAG* tagv1;
      ULONG encv1;
      data->page_ready = FALSE;
      // clear old values to have a clean start
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TITLE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ARTIST,  ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ALBUM,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TRACK,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_DATE,    ""));
      PMRASSERT(lb_select(hwnd, CO_GENRE, 0));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_COMMENT, ""));
      switch (SHORT1FROMMP(mp1)) // mode
      {case 0: // load current
        WinCheckButton(hwnd, RB_UPDATE+data->write_tagv1, TRUE);
        tagv1 = &data->tagv1;
        encv1 = data->encoding_tagv1;
        dlg_set_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING), encv1);
        break;
       case 1: // load original
        WinCheckButton(hwnd, data->tagv1_old ? RB_UPDATE : RB_DELETE, TRUE);
        data->autowrite_tagv1 = FALSE;
        tagv1 = data->tagv1_old;
        // Do NOT use the old value in this case.
        encv1 = dlg_query_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING))->codepage;
        break;
       case 2: // reset
        WinCheckButton(hwnd, RB_DELETE, TRUE);
        data->autowrite_tagv1 = FALSE;
        tagv1 = NULL;
        encv1 = tag_id3v1_charset;
        dlg_set_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING), encv1);
        break;
       case 3: // copy
        WinCheckButton(hwnd, RB_UPDATE, TRUE);
        data->autowrite_tagv1 = FALSE;
        id3v2_load( hwnd, data->tagv2, data->encoding_tagv2 );
        { // Restrict track to Version 1
          HWND ctrl = WinWindowFromID(hwnd, EN_TRACK);
          char buf[4];
          char* cp;
          WinQueryWindowText(ctrl, sizeof buf, buf);
          cp = strchr(buf, '/');
          if (cp)
          { *cp = 0;
            WinSetWindowText(ctrl, buf);
          }
        }
        return 0;
      }
      id3v1_load( hwnd, tagv1, encv1 );
      data->page_ready = TRUE;
      return 0;
    }
    case UM_STORE:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const char* msg;
      unsigned track;
      SHORT s;
      data->page_ready = FALSE;
      id3_strip_all( hwnd );
      // validate
      msg = id3_validatetrack( hwnd, &track, NULL );
      if (msg)
        return MRFROMP(msg);
      msg = id3_validatedate( hwnd );
      if (msg)
        return MRFROMP(msg);
      if (track && WinQueryDlgItemTextLength(hwnd, EN_COMMENT) > 28)
        return "The Comment field of ID3v1.1 tags with a track number is restricted to 28 characters.";
      // store
      data->encoding_tagv1 = dlg_query_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING))->codepage;
      s = SHORT1FROMMR(WinSendDlgItemMsg(hwnd, RB_UPDATE, BM_QUERYCHECKINDEX, 0, 0));
      PMASSERT(s != -1);
      data->write_tagv1 = (write_mode)s;
      id3v1_store( hwnd, &data->tagv1, ~0, data->encoding_tagv1 );
      return 0; // OK
    }
  }
  return id3_page_dlg_base_proc( hwnd, msg, mp1, mp2 );
}

static MRESULT EXPENTRY
id3v2_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ DEBUGLOG2(("mpg123:id3v1_page_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  { case WM_INITDLG:
      dlg_populate_charset_listbox( WinWindowFromID( hwnd, CO_ENCODING ), ch_list, ch_list_size - ch_list_dbcs, tag_read_id3v2_charset );
      break;

    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      {case EN_TITLE:
       case EN_ARTIST:
       case EN_ALBUM:
       case EN_TRACK:
       case EN_DATE:
       case CO_GENRE:
       case EN_COMMENT:
       case EN_COPYRIGHT:
        if (SHORT2FROMMP(mp1) == EN_CHANGE)
        { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
          if (data->page_ready)
          { data->autowrite_tagv2 = FALSE;
            WinCheckButton(hwnd, RB_UPDATE2, TRUE);
          }
        }
        break;

       case RB_UPDATE2:
       case RB_UNCHANGED2:
       case RB_CLEAN2:
       case RB_DELETE2:
        switch (SHORT2FROMMP(mp1))
        { case BN_CLICKED:
          case BN_DBLCLICKED:
          { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
            data->autowrite_tagv2 = FALSE;
          }
        }
        break;
      };
      break;

    case UM_LOAD:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const ID3V2_TAG* tagv2;
      ULONG encv2;
      data->page_ready = FALSE;
      // clear old values to have a clean start
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TITLE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ARTIST,  ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_ALBUM,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_TRACK,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_DATE,    ""));
      PMRASSERT(WinSetDlgItemText(hwnd, CO_GENRE,   ""));
      PMRASSERT(WinSetDlgItemText(hwnd, EN_COMMENT, ""));
      switch (SHORT1FROMMP(mp1)) // mode
      {case 0: // load current
        WinCheckButton(hwnd, RB_UPDATE2+data->write_tagv2, TRUE);
        tagv2 = data->tagv2;
        encv2 = data->encoding_tagv2;
        dlg_set_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING), encv2);
        break;
       case 1: // load original
        WinCheckButton(hwnd, data->tagv2_old ? RB_UPDATE2 : RB_DELETE2, TRUE);
        data->autowrite_tagv2 = FALSE;
        tagv2 = data->tagv2_old;
        // Do NOT use the old value in this case.
        encv2 = dlg_query_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING))->codepage;
        break;
       case 2: // reset
        WinCheckButton(hwnd, RB_DELETE2, TRUE);
        data->autowrite_tagv2 = FALSE;
        tagv2 = NULL;
        encv2 = tag_read_id3v2_charset;
        dlg_set_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING), encv2);
        break;
       case 3: // copy
        WinCheckButton(hwnd, RB_UPDATE2, TRUE);
        data->autowrite_tagv2 = FALSE;
        id3v1_load( hwnd, &data->tagv1, data->encoding_tagv1 );
        return 0;
      }
      id3v2_load( hwnd, tagv2, encv2 );
      data->page_ready = TRUE;
      return 0;
    }
    case UM_STORE:
    { ID3_EDIT_TAGINFO* data = (ID3_EDIT_TAGINFO*)WinQueryWindowPtr( hwnd, QWL_USER );
      const char* msg;
      unsigned track, total;
      SHORT s;
      data->page_ready = FALSE;
      id3_strip_all( hwnd );
      // validate
      msg = id3_validatetrack( hwnd, &track, &total );
      if (msg)
        return MRFROMP(msg);
      msg = id3_validatedate( hwnd );
      if (msg)
        return MRFROMP(msg);
      // store
      data->encoding_tagv2 = dlg_query_charset_listbox(WinWindowFromID(hwnd, CO_ENCODING))->codepage;
      s = SHORT1FROMMR(WinSendDlgItemMsg(hwnd, RB_UPDATE2, BM_QUERYCHECKINDEX, 0, 0));
      PMASSERT(s != -1);
      data->write_tagv2 = (write_mode)s;
      id3v2_store( hwnd, data->tagv2, ~0, tag_save_id3v2_encoding );
      return 0; // OK
    }
  }
  return id3_page_dlg_base_proc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the dialog of edition of ID3 tag. */
static MRESULT EXPENTRY
id3_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  DEBUGLOG2(("mpg123:id3_dlg_proc(%p, %d, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch( msg )
  {
    case WM_QUERYTRACKINFO:
    { MRESULT mr = WinDefDlgProc( hwnd, msg, mp1, mp2 );
      TRACKINFO* pti = (TRACKINFO*)mp2;
      if (pti->ptlMinTrackSize.x < 400)
        pti->ptlMinTrackSize.x = 400;
      if (pti->ptlMinTrackSize.y < 400)
        pti->ptlMinTrackSize.y = 400;
      return mr;
    }     
    case WM_WINDOWPOSCHANGED:
    {
      SWP* pswp = (SWP*)mp1;
      if ( (pswp[0].fl & SWP_SIZE) && pswp[1].cx )
        nb_adjust( hwnd, pswp );
      break;
    }
    case WM_CONTROL:
      switch (SHORT1FROMMP(mp1))
      { case NB_ID3TAG:
          if (SHORT2FROMMP(mp1) == BKN_PAGESELECTEDPENDING)
          { PAGESELECTNOTIFY* psn = (PAGESELECTNOTIFY*)PVOIDFROMMP(mp2);
            if (psn->ulPageIdCur)
            { HWND nb = (HWND)WinSendDlgItemMsg(hwnd, NB_ID3TAG, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(psn->ulPageIdCur), 0);
              const char* msg;
              PMASSERT(nb != NULLHANDLE && nb != BOOKERR_INVALID_PARAMETERS);
              msg = (const char*)WinSendMsg(nb, UM_STORE, 0, 0);
              if (msg)
              { psn->ulPageIdNew = NULLHANDLE;
                PMRASSERT(WinPostMsg(hwnd, UM_ERROR, MPFROMP(msg), MPFROMP(NULL)));
                break;
              }
              if (psn->ulPageIdNew) 
              { HWND nb = (HWND)WinSendDlgItemMsg(hwnd, NB_ID3TAG, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(psn->ulPageIdNew), 0);
                PMASSERT(nb != NULLHANDLE && nb != BOOKERR_INVALID_PARAMETERS);
                PMRASSERT(WinPostMsg(nb, UM_LOAD, MPFROMSHORT(0), 0));
              }
            }
          }
          break;
      }
      return 0;
      
    case WM_COMMAND:
      switch (SHORT1FROMMP(mp1))
      { case DID_OK:
        { // save and validate current page
          HWND nb = WinWindowFromID(hwnd, NB_ID3TAG);
          ULONG id = LONGFROMMR(WinSendMsg(nb, BKM_QUERYPAGEID, 0, MPFROM2SHORT(BKA_TOP, 0)));
          const char* msg;
          PMASSERT(id != NULLHANDLE && id != BOOKERR_INVALID_PARAMETERS);
          nb = (HWND)WinSendMsg(nb, BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(id), 0);
          PMASSERT(nb != NULLHANDLE && nb != BOOKERR_INVALID_PARAMETERS);
          msg = (const char*)WinSendMsg(nb, UM_STORE, 0, 0);
          if (msg)
          { PMRASSERT(WinPostMsg(hwnd, UM_ERROR, MPFROMP(msg), MPFROMP(NULL)));
            return 0; // Reject the command
          }
          break;
        }
      }
      break;
      
    case UM_ERROR:
      WinMessageBox( HWND_DESKTOP, hwnd, (PSZ)mp1, (PSZ)mp2, DLG_ID3TAG, MB_CANCEL|MB_ERROR|MB_MOVEABLE );
      return 0;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Edits a ID3 tag for the specified file. */
ULONG DLLENTRY decoder_editmeta( HWND owner, const char* filename )
{
  char    caption[_MAX_FNAME+20] = "ID3 Tag Editor - ";
  HWND    hwnd;
  HWND    book;
  HWND    page01, page02, page03;
  HMODULE module;
  ULONG   rc;
  DECODER_STRUCT* w;
  ID3_EDIT_TAGINFO workarea;
  int     i; 

  DEBUGLOG(("mpg123:decoder_editmeta(%p, %s)\n", owner, filename));

  // read tag
  if( decoder_init((void**)(void*)&w ) != PLUGIN_OK ) {
    return PLUGIN_NO_PLAY;
  }

cont_edit:
  if(( rc = plg_open_file( w, filename, "r+bU" )) != 0 ) {
    decoder_uninit( w );
    return rc == -1 ? PLUGIN_NO_PLAY : PLUGIN_NO_READ;
  }
  // We keep the file open while the dialog is visible to lock the file.

  // Don't know what this mutex is good for
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

  if (id3v1_is_tag_valid(&w->mpeg.tagv1))
  { workarea.tagv1_old = &w->mpeg.tagv1;
    workarea.tagv1 = *workarea.tagv1_old;
    workarea.write_tagv1 = WR_UPDATE;
    workarea.encoding_tagv1_old = tag_read_id3v1_autoch
      ? detect_id3v1_codepage(workarea.tagv1_old, tag_id3v1_charset)
      : tag_id3v1_charset;
  } else
  { workarea.tagv1_old = NULL;
    id3v1_clean_tag(&workarea.tagv1);
    workarea.write_tagv1 = WR_CLEAN;
    workarea.encoding_tagv1_old = tag_id3v1_charset;
  }
  workarea.encoding_tagv1 = workarea.encoding_tagv1_old;
  workarea.autowrite_tagv1 = TRUE;

  if (w->mpeg.tagv2)
  { ID3V2_TAG* tag = id3v2_new_tag();
    // ID3V2: the current tag gets the new one to preserve other information,
    // some of the required information is put into the old_tag for backup purposes.
    workarea.tagv2 = w->mpeg.tagv2;
    if (workarea.tagv2)
    { ID3V2_FRAME* frame;
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TIT2, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TPE1, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TALB, 1));
      for ( i = 1; (frame = id3v2_get_frame(workarea.tagv2, ID3V2_COMM, i)) != NULL ; i++)
        id3v2_copy_frame(tag, frame);
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TRCK, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TDRC, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TCON, 1)); 
      id3v2_copy_frame(tag, id3v2_get_frame(workarea.tagv2, ID3V2_TCOP, 1));
      workarea.write_tagv2 = WR_UPDATE;
    } else
      workarea.write_tagv2 = WR_CLEAN;
    workarea.tagv2_old = tag;
  } else
  { workarea.tagv2 = id3v2_new_tag();
    workarea.tagv2_old = NULL;
    workarea.write_tagv2 = WR_UNCHANGED;
  }
  workarea.encoding_tagv2_old = tag_read_id3v2_charset;
  workarea.encoding_tagv2 = workarea.encoding_tagv2_old;
  workarea.autowrite_tagv2 = TRUE;
  
  DosQueryModFromEIP( &module, &rc, 0, NULL, &rc, (ULONG)&decoder_editmeta ); 
  hwnd = WinLoadDlg( HWND_DESKTOP, owner, id3_dlg_proc, module, DLG_ID3TAG, 0 );
  DEBUGLOG(("mpg123:decoder_editmeta: WinLoadDlg: %p (%p) - %p\n", hwnd, WinGetLastError(0), module));
  sfnameext( caption + strlen( caption ), filename, sizeof( caption ) - strlen( caption ));
  WinSetWindowText( hwnd, caption );

  book = WinWindowFromID( hwnd, NB_ID3TAG );
  do_warpsans( book );

  //WinSendMsg( book, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_FIELDBACKGROUND),
  //            MPFROMSHORT(BKA_BACKGROUNDPAGECOLORINDEX));

  page01 = WinLoadDlg( book, book, id3all_page_dlg_proc, module, DLG_ID3ALL, 0 );
  do_warpsans( page01 );
  WinSetWindowPtr( page01, QWL_USER, &workarea );
  PMRASSERT( nb_append_tab( book, page01, "All Tags", NULL, 0 ));

  page02 = WinLoadDlg( book, book, id3v1_page_dlg_proc, module, DLG_ID3V1, 0 );
  do_warpsans( page02 );
  WinSetWindowPtr( page02, QWL_USER, &workarea );
  PMRASSERT( nb_append_tab( book, page02, "ID3v1.x", NULL, 0 ));

  page03 = WinLoadDlg( book, book, id3v2_page_dlg_proc, module, DLG_ID3V2, 0 );
  do_warpsans( page03 );
  WinSetWindowPtr( page03, QWL_USER, &workarea );
  PMRASSERT( nb_append_tab( book, page03, "ID3v2.x", NULL, 0 ));

  PMRASSERT(WinPostMsg( page01, UM_LOAD, MPFROMSHORT(0), 0));
  rc = WinProcessDlg( hwnd );
  DEBUGLOG(("mpg123:decoder_editmeta: dlg completed - %u, %p %p (%p)\n", rc, hwnd, page01, WinGetLastError(0)));

  if ( rc == DID_OK )
  { // WRITE TAG!
    ID3V1_TAG* tagv1 = &workarea.tagv1;
    ID3V2_TAG* tagv2 = workarea.tagv2;
    char savename[_MAX_PATH];
    const char* errmsg;

    switch (workarea.write_tagv2)
    { ID3V2_FRAME* frame;
      char buffer[128];
      case WR_CLEAN:
        if ((frame = id3v2_get_frame( tagv2, ID3V2_TIT2, 1 )) != NULL)
          id3v2_delete_frame( frame );
        if ((frame = id3v2_get_frame( tagv2, ID3V2_TPE1, 1 )) != NULL)
          id3v2_delete_frame( frame );
        if ((frame = id3v2_get_frame( tagv2, ID3V2_TALB, 1 )) != NULL)
          id3v2_delete_frame( frame );
        if ((frame = id3v2_get_frame( tagv2, ID3V2_TDRC, 1 )) != NULL)
          id3v2_delete_frame( frame );
        for( i = 1; ( frame = id3v2_get_frame( tagv2, ID3V2_COMM, i )) != NULL ; i++ )
        { id3v2_get_description( frame, buffer, sizeof( buffer ));
          // Skip iTunes specific comment tags.
          if( strnicmp( buffer, "iTun", 4 ) != 0 )
          { id3v2_delete_frame( frame );
            break;
          }
        }
        if ((frame = id3v2_get_frame( tagv2, ID3V2_TCON, 1 )) != NULL)
          id3v2_delete_frame( frame );
        if ((frame = id3v2_get_frame( tagv2, ID3V2_TCOP, 1 )) != NULL)
          id3v2_delete_frame( frame );
        if ((frame = id3v2_get_frame( tagv2, ID3V2_TRCK, 1 )) != NULL)
          id3v2_delete_frame( frame );
      case WR_UPDATE:
        if (tagv2->id3_frames_count)
        { if (!tagv2->id3_altered)
            tagv2 = NULL;
          break;
        }
      case WR_DELETE:
        if (workarea.tagv2_old)
        { tagv2 = PLG_DELETE_ID3V2;
          break;
        }
      default: // case WR_UNCHANGED:
        tagv2 = NULL;
    }
    
    switch (workarea.write_tagv1)
    { case WR_UPDATE:
        if (!id3v1_is_clean_tag(tagv1))
          break;
      default: // case WR_CLEAN:
        if (workarea.tagv1_old)
        { tagv1 = PLG_DELETE_ID3V1;
          break;
        }
      case WR_UNCHANGED:
        tagv1 = NULL;
    }
    
    // Now start the transaction.
  retry:
    rc = plg_update_tags( w, tagv1, tagv2, &savename, &errmsg );
    plg_close_file( w );
    DosReleaseMutexSem( w->mutex );
    
    if (rc == 0 && *savename)
    { // Must replace the file.
      // Preserve EAs.
      eacopy( filename, savename );
      rc = plg_replace_file( savename, filename, &errmsg );
    }
    
    if (rc != 0)
    { // Error message
      char buf[256];
      snprintf(buf, sizeof buf, "%s\nError code: %i - %s\n Retry?", errmsg, (int)rc, xio_strerror(rc));
      switch (WinMessageBox(HWND_DESKTOP, hwnd, buf, NULL, 0, MB_YESNOCANCEL|MB_ERROR|MB_MOVEABLE))
      { case MBID_CANCEL:
          goto cont_edit;

        case MBID_YES:
          // First reopen the plug-in
          rc = plg_open_file( w, filename, "r+bU" );
          if( rc == 0 )
          { // Don't know what this mutex is good for
            DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
            goto retry;
          }
          snprintf(buf, sizeof buf, "Cannot reopen original file.\nError code: %i - %s", (int)rc, xio_strerror(rc));
          WinMessageBox(HWND_DESKTOP, hwnd, buf, NULL, 0, MB_OK|MB_ERROR|MB_MOVEABLE);
          rc = rc == -1 ? PLUGIN_NO_PLAY : PLUGIN_NO_READ;
      }
    } else
      rc = PLUGIN_OK;
  }
  else // ! DID_OK 
  { plg_close_file( w );
    DosReleaseMutexSem( w->mutex );
    rc = PLUGIN_NO_OP;
  }

  WinDestroyWindow( hwnd );
  decoder_uninit( w );

  return rc;
}

