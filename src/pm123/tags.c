/*
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

#define  INCL_DOS
#define  INCL_WIN
#include <os2.h>
#include <process.h>
#include <snprintf.h>

#include <decoder_plug.h>
#include <queue.h>

#include "pm123.h"
#include "plugman.h"
#include "assertions.h"
#include "genre.h"
#include "playlist.h"
#include "tags.h"

#define  TAG_UPDATE    0
#define  TAG_TERMINATE 1

static   TID    broker_tid   = 0;
static   PQUEUE broker_queue = NULL;

/* Dispatches the tags management requests. */
static void TFNENTRY
tag_broker( void* dummy )
{
  ULONG request;
  PVOID reqdata;
  HAB   hab = WinInitialize( 0 );
  HMQ   hmq = WinCreateMsgQueue( hab, 0 );
  ULONG rc;

  while( qu_read( broker_queue, &request, &reqdata ))
  {
    AMP_FILEINFO* data = (AMP_FILEINFO*)reqdata;

    switch( request ) {
      case TAG_UPDATE:
        if( data->options & TAG_APPLY_ALL ) {
          pl_refresh_songname( data->filename, "Updating..." );
          rc = dec_saveinfo( data->filename, &data->info, data->decoder  );
        } else {
          DECODER_INFO info;

          #define TAG_COPY_FIELD( type, name )                           \
            if( data->info.haveinfo & type ) {                           \
              strlcpy( info.name, data->info.name, sizeof( info.name )); \
            }

          if(( rc = dec_fileinfo( data->filename, &info, data->decoder )) == PLUGIN_OK )
          {
            TAG_COPY_FIELD( DECODER_HAVE_TITLE,     title     );
            TAG_COPY_FIELD( DECODER_HAVE_ARTIST,    artist    );
            TAG_COPY_FIELD( DECODER_HAVE_ALBUM,     album     );
            TAG_COPY_FIELD( DECODER_HAVE_YEAR,      year      );
            TAG_COPY_FIELD( DECODER_HAVE_COMMENT,   comment   );
            TAG_COPY_FIELD( DECODER_HAVE_GENRE,     genre     );
            TAG_COPY_FIELD( DECODER_HAVE_COPYRIGHT, copyright );
            TAG_COPY_FIELD( DECODER_HAVE_TRACK,     track     );

            pl_refresh_songname( data->filename, "Updating..." );
            rc = dec_saveinfo( data->filename, &info, data->decoder );
          }
        }

        if( rc == PLUGIN_OK ) {
          dec_fileinfo( data->filename, &data->info, data->decoder );
          WinPostMsg( amp_player_window(), WM_123FILE_REFRESH, data, 0 );
          continue;
        } else {
          pl_refresh_songname( data->filename, "Can't update file info" );
          break;
        }
    }

    if( data ) {
      free( data->filename );
      free( data );
    }

    if( request == TAG_TERMINATE ) {
      break;
    }
  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
  _endthread();
}

/* Processes messages of the song information dialog.
   Must be called from the main thread. */
static MRESULT EXPENTRY
tag_song_info_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_INITDLG:
    {
      int  i;
      for( i = 0; i <= GENRE_LARGEST; i++) {
        WinSendDlgItemMsg( hwnd, CB_TAG_GENRE, LM_INSERTITEM,
                           MPFROMSHORT( LIT_END ), MPFROMP( genres[i] ));
      }
      break;
    }

    case WM_COMMAND:
      if( COMMANDMSG(&msg)->cmd == PB_TAG_UNDO )
      {
        DECODER_INFO* info = (DECODER_INFO*)WinQueryWindowPtr( hwnd, 0 );
        SHORT gennum;

        WinSetDlgItemText( hwnd, EN_TAG_TITLE,     info->title     );
        WinSetDlgItemText( hwnd, EN_TAG_ARTIST,    info->artist    );
        WinSetDlgItemText( hwnd, EN_TAG_ALBUM,     info->album     );
        WinSetDlgItemText( hwnd, EN_TAG_COMMENT,   info->comment   );
        WinSetDlgItemText( hwnd, EN_TAG_COPYRIGHT, info->copyright );
        WinSetDlgItemText( hwnd, EN_TAG_YEAR,      info->year      );
        WinSetDlgItemText( hwnd, EN_TAG_TRACK,     info->track     );

        gennum = lb_search( hwnd, CB_TAG_GENRE, LIT_FIRST, info->genre );

        if( gennum != LIT_ERROR && gennum != LIT_NONE ) {
          lb_select( hwnd, CB_TAG_GENRE, gennum );
        } else {
          WinSetDlgItemText( hwnd, CB_TAG_GENRE, info->genre );
        }
      }
      return 0;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the tech information dialog.
   Must be called from the main thread. */
static MRESULT EXPENTRY
tag_tech_info_page_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg ) {
    case WM_COMMAND:
      return 0;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Processes messages of the song information dialog.
   Must be called from the main thread. */
static MRESULT EXPENTRY
tag_edit_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  switch( msg )
  {
    case WM_WINDOWPOSCHANGED:
      if(((PSWP)mp1)[0].fl & SWP_SIZE ) {
        nb_adjust( hwnd );
      }
      break;

    case WM_COMMAND:
      switch( COMMANDMSG( &msg )->cmd ) {
        case PB_TAG_APPLYGROUP:
        case PB_TAG_APPLY:
          WinDismissDlg( hwnd, COMMANDMSG( &msg )->cmd );
          return MRFROMLONG(1L);

        case PB_TAG_UNDO:
        {
          LONG id;
          HWND page = NULLHANDLE;

          id = (LONG)WinSendDlgItemMsg( hwnd, NB_FILEINFO, BKM_QUERYPAGEID, 0,
                                              MPFROM2SHORT(BKA_TOP,BKA_MAJOR));

          if( id && id != BOOKERR_INVALID_PARAMETERS ) {
              page = (HWND)WinSendDlgItemMsg( hwnd, NB_FILEINFO,
                                              BKM_QUERYPAGEWINDOWHWND, MPFROMLONG(id), 0 );
          }

          if( page && page != BOOKERR_INVALID_PARAMETERS ) {
            WinSendMsg( page, WM_COMMAND, mp1, mp2 );
          }
          return MRFROMLONG(1L);
        }
      }
      break;
  }
  return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/* Initializes the song information dialog. */
static void
tag_song_info_init( HWND hwnd, HWND page01, DECODER_INFO* info, int options )
{
  if( options & TAG_GROUP_OPERATIONS ) {
    WinCheckButton( page01, CH_TAG_TITLE,     cfg.tags_choice & DECODER_HAVE_TITLE     ? TRUE : FALSE );
    WinCheckButton( page01, CH_TAG_ARTIST,    cfg.tags_choice & DECODER_HAVE_ARTIST    ? TRUE : FALSE );
    WinCheckButton( page01, CH_TAG_ALBUM,     cfg.tags_choice & DECODER_HAVE_ALBUM     ? TRUE : FALSE );
    WinCheckButton( page01, CH_TAG_COMMENT,   cfg.tags_choice & DECODER_HAVE_COMMENT   ? TRUE : FALSE );
    WinCheckButton( page01, CH_TAG_COPYRIGHT, cfg.tags_choice & DECODER_HAVE_COPYRIGHT ? TRUE : FALSE );
    WinCheckButton( page01, CH_TAG_YEAR,      cfg.tags_choice & DECODER_HAVE_YEAR      ? TRUE : FALSE );
    WinCheckButton( page01, CH_TAG_TRACK,     cfg.tags_choice & DECODER_HAVE_TRACK     ? TRUE : FALSE );
    WinCheckButton( page01, CH_TAG_GENRE,     cfg.tags_choice & DECODER_HAVE_GENRE     ? TRUE : FALSE );
  } else {
    if( !info->saveinfo )
    {
      en_enable( page01, EN_TAG_TITLE,     FALSE );
      en_enable( page01, EN_TAG_ARTIST,    FALSE );
      en_enable( page01, EN_TAG_ALBUM,     FALSE );
      en_enable( page01, EN_TAG_COMMENT,   FALSE );
      en_enable( page01, EN_TAG_COPYRIGHT, FALSE );
      en_enable( page01, EN_TAG_YEAR,      FALSE );
      en_enable( page01, EN_TAG_TRACK,     FALSE );
      en_enable( page01, CB_TAG_GENRE,     FALSE );

      WinEnableControl( page01, PB_TAG_UNDO, FALSE );
    }
    else if( info->haveinfo )
    {
      en_enable( page01, EN_TAG_TITLE,     info->haveinfo & DECODER_HAVE_TITLE     );
      en_enable( page01, EN_TAG_ARTIST,    info->haveinfo & DECODER_HAVE_ARTIST    );
      en_enable( page01, EN_TAG_ALBUM,     info->haveinfo & DECODER_HAVE_ALBUM     );
      en_enable( page01, EN_TAG_COMMENT,   info->haveinfo & DECODER_HAVE_COMMENT   );
      en_enable( page01, EN_TAG_COPYRIGHT, info->haveinfo & DECODER_HAVE_COPYRIGHT );
      en_enable( page01, EN_TAG_YEAR,      info->haveinfo & DECODER_HAVE_YEAR      );
      en_enable( page01, EN_TAG_TRACK,     info->haveinfo & DECODER_HAVE_TRACK     );
      en_enable( page01, CB_TAG_GENRE,     info->haveinfo & DECODER_HAVE_GENRE     );
    }

    WinShowDlgItem( page01, CH_TAG_TITLE,      FALSE );
    WinShowDlgItem( page01, CH_TAG_ARTIST,     FALSE );
    WinShowDlgItem( page01, CH_TAG_ALBUM,      FALSE );
    WinShowDlgItem( page01, CH_TAG_COMMENT,    FALSE );
    WinShowDlgItem( page01, CH_TAG_COPYRIGHT,  FALSE );
    WinShowDlgItem( page01, CH_TAG_YEAR,       FALSE );
    WinShowDlgItem( page01, CH_TAG_TRACK,      FALSE );
    WinShowDlgItem( page01, CH_TAG_GENRE,      FALSE );
    WinShowDlgItem( hwnd,   PB_TAG_APPLYGROUP, FALSE );
  }
}

/* Initializes the tech information dialog. */
static void
tag_tech_info_init( HWND hwnd, HWND page02, const char* filename, char* decoder, DECODER_INFO* info )
{
  char buffer[512];

  en_enable( page02, EN_FILENAME,   FALSE );
  en_enable( page02, EN_DECODER,    FALSE );
  en_enable( page02, EN_INFO,       FALSE );
  en_enable( page02, EN_MPEGINFO,   FALSE );
  en_enable( page02, EN_SONGLENGTH, FALSE );
  en_enable( page02, EN_FILESIZE,   FALSE );
  en_enable( page02, EN_STARTED,    FALSE );
  en_enable( page02, EN_ENDED,      FALSE );
  en_enable( page02, EN_TRACK_GAIN, FALSE );
  en_enable( page02, EN_TRACK_PEAK, FALSE );
  en_enable( page02, EN_ALBUM_GAIN, FALSE );
  en_enable( page02, EN_ALBUM_PEAK, FALSE );

  WinSetDlgItemText( page02, EN_FILENAME, (PSZ)filename );
  WinSetDlgItemText( page02, EN_DECODER,  dec_get_description( decoder, buffer, sizeof( buffer )));
  WinSetDlgItemText( page02, EN_INFO,     info->tech_info );

  if( info->mpeg && info->layer ) {
    snprintf( buffer, sizeof( buffer ), "mpeg %.1f layer %d", (float)info->mpeg/10, info->layer );
    WinSetDlgItemText( page02, EN_MPEGINFO, buffer );
  }
  if( info->songlength > 0 ) {
    snprintf( buffer, sizeof( buffer ), "%d:%d", info->songlength / 60000, info->songlength % 60000 / 1000 );
    WinSetDlgItemText( page02, EN_SONGLENGTH, buffer );
  }
  if( info->filesize > 0 ) {
    snprintf( buffer, sizeof( buffer ), "%d bytes", info->filesize );
    WinSetDlgItemText( page02, EN_FILESIZE, buffer );
  }
  if( info->startsector ) {
    snprintf( buffer, sizeof( buffer ), "%d sector", info->startsector );
    WinSetDlgItemText( page02, EN_STARTED, buffer );
  } else if( info->junklength ) {
    snprintf( buffer, sizeof( buffer ), "%d byte", info->junklength );
    WinSetDlgItemText( page02, EN_STARTED, buffer );
  }
  if( info->endsector ) {
    snprintf( buffer, sizeof( buffer ), "%d sector", info->endsector );
    WinSetDlgItemText( page02, EN_ENDED, buffer );
  }
  if( info->track_gain ) {
    snprintf( buffer, sizeof( buffer ), "%+.2f dB", info->track_gain );
    WinSetDlgItemText( page02, EN_TRACK_GAIN, buffer );
  }
  if( info->track_peak ) {
    snprintf( buffer, sizeof( buffer ), "%.6f", info->track_peak );
    WinSetDlgItemText( page02, EN_TRACK_PEAK, buffer );
  }
  if( info->album_gain ) {
    snprintf( buffer, sizeof( buffer ), "%+.2f dB", info->album_gain );
    WinSetDlgItemText( page02, EN_ALBUM_GAIN, buffer );
  }
  if( info->album_peak ) {
    snprintf( buffer, sizeof( buffer ), "%.6f", info->album_peak );
    WinSetDlgItemText( page02, EN_ALBUM_PEAK, buffer );
  }
}

/* Edits a meta information of the specified file.
   Must be called from the main thread. */
int
tag_edit( HWND owner, const char* filename, char* decoder, DECODER_INFO* info, int options )
{
  char    caption[_MAX_FNAME] = "File Info - ";
  HWND    hwnd;
  HWND    book;
  HWND    page01;
  HWND    page02;
  MRESULT id;
  ULONG   rc;

  DECODER_INFO  old_info;
  DECODER_INFO  new_info;

  ASSERT_IS_MAIN_THREAD;

  if( info ) {
    old_info = *info;
    new_info = *info;
  } else {
    dec_fileinfo((char*)filename, &old_info, decoder );
    new_info = old_info;
  }

  hwnd = WinLoadDlg( HWND_DESKTOP, owner,
                     tag_edit_dlg_proc, hmodule, DLG_FILEINFO, 0 );

  if( is_url( filename )) {
    char decoded[_MAX_PATH];
    sfnameext( caption + strlen( caption ),
               sdecode( decoded, filename, sizeof( decoded )), sizeof( caption ) - strlen( caption ));
  } else {
    sfnameext( caption + strlen( caption ),
               filename, sizeof( caption ) - strlen( caption ));
  }

  WinSetWindowText( hwnd, caption );

  book = WinWindowFromID( hwnd, NB_FILEINFO );
  do_warpsans( book );

  WinSendMsg( book, BKM_SETDIMENSIONS, MPFROM2SHORT(100,25), MPFROMSHORT(BKA_MAJORTAB));
  WinSendMsg( book, BKM_SETDIMENSIONS, MPFROMLONG(0), MPFROMSHORT(BKA_MINORTAB));
  WinSendMsg( book, BKM_SETNOTEBOOKCOLORS, MPFROMLONG(SYSCLR_FIELDBACKGROUND),
              MPFROMSHORT(BKA_BACKGROUNDPAGECOLORINDEX));

  page01 = WinLoadDlg( book, book, tag_song_info_page_dlg_proc, hmodule, DLG_SONGINFO, 0 );
  id     = WinSendMsg( book, BKM_INSERTPAGE, MPFROMLONG(0),
                       MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MAJOR, BKA_FIRST ));
  do_warpsans( page01 );
  WinSendMsg ( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG(id), MPFROMLONG( page01 ));
  WinSendMsg ( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "~Song Info" ));
  WinSetOwner( page01, book );

  page02 = WinLoadDlg( book, book, tag_tech_info_page_dlg_proc, hmodule, DLG_TECHINFO, 0 );
  id     = WinSendMsg( book, BKM_INSERTPAGE, MPFROMLONG(id),
                       MPFROM2SHORT( BKA_AUTOPAGESIZE | BKA_MAJOR, BKA_NEXT ));
  do_warpsans( page02 );
  WinSendMsg ( book, BKM_SETPAGEWINDOWHWND, MPFROMLONG(id), MPFROMLONG( page02 ));
  WinSendMsg ( book, BKM_SETTABTEXT, MPFROMLONG( id ), MPFROMP( "~Tech Info" ));
  WinSetOwner( page02, book );

  WinSetWindowPtr( page01, 0, &old_info );
  WinSetWindowPtr( page02, 0, &old_info );

  WinPostMsg( page01, WM_COMMAND,
              MPFROMSHORT( PB_TAG_UNDO ), MPFROM2SHORT( CMDSRC_OTHER, FALSE ));

  tag_song_info_init( hwnd, page01, &old_info, options );
  tag_tech_info_init( hwnd, page02, filename,  decoder, &old_info );

  nb_adjust( hwnd );
  WinSetFocus( HWND_DESKTOP, WinWindowFromID( page01, EN_TAG_TITLE ));
  rc = WinProcessDlg( hwnd );

  WinQueryDlgItemText( page01, EN_TAG_TITLE,     sizeof( new_info.title     ), new_info.title     );
  WinQueryDlgItemText( page01, EN_TAG_ARTIST,    sizeof( new_info.artist    ), new_info.artist    );
  WinQueryDlgItemText( page01, EN_TAG_ALBUM,     sizeof( new_info.album     ), new_info.album     );
  WinQueryDlgItemText( page01, EN_TAG_COMMENT,   sizeof( new_info.comment   ), new_info.comment   );
  WinQueryDlgItemText( page01, EN_TAG_COPYRIGHT, sizeof( new_info.copyright ), new_info.copyright );
  WinQueryDlgItemText( page01, EN_TAG_YEAR,      sizeof( new_info.year      ), new_info.year      );
  WinQueryDlgItemText( page01, EN_TAG_TRACK,     sizeof( new_info.track     ), new_info.track     );
  WinQueryDlgItemText( page01, CB_TAG_GENRE,     sizeof( new_info.genre     ), new_info.genre     );

  if( rc == PB_TAG_APPLY )
  {
    WinDestroyWindow( page01 );
    WinDestroyWindow( hwnd   );

    if( !old_info.saveinfo ) {
      return TAG_NO_CHANGES;
    }

    if( strcmp( old_info.title,     new_info.title     ) != 0 ||
        strcmp( old_info.artist,    new_info.artist    ) != 0 ||
        strcmp( old_info.album,     new_info.album     ) != 0 ||
        strcmp( old_info.comment,   new_info.comment   ) != 0 ||
        strcmp( old_info.copyright, new_info.copyright ) != 0 ||
        strcmp( old_info.year,      new_info.year      ) != 0 ||
        strcmp( old_info.track,     new_info.track     ) != 0 ||
        strcmp( old_info.genre,     new_info.genre     ) != 0 )
    {
      *info = new_info;
      return TAG_APPLY;
    }
  }
  else if( rc == PB_TAG_APPLYGROUP )
  {
    new_info.haveinfo =
      ( WinQueryButtonCheckstate( page01, CH_TAG_TITLE     ) ? DECODER_HAVE_TITLE     : 0 ) |
      ( WinQueryButtonCheckstate( page01, CH_TAG_ARTIST    ) ? DECODER_HAVE_ARTIST    : 0 ) |
      ( WinQueryButtonCheckstate( page01, CH_TAG_ALBUM     ) ? DECODER_HAVE_ALBUM     : 0 ) |
      ( WinQueryButtonCheckstate( page01, CH_TAG_COMMENT   ) ? DECODER_HAVE_COMMENT   : 0 ) |
      ( WinQueryButtonCheckstate( page01, CH_TAG_COPYRIGHT ) ? DECODER_HAVE_COPYRIGHT : 0 ) |
      ( WinQueryButtonCheckstate( page01, CH_TAG_YEAR      ) ? DECODER_HAVE_YEAR      : 0 ) |
      ( WinQueryButtonCheckstate( page01, CH_TAG_TRACK     ) ? DECODER_HAVE_TRACK     : 0 ) |
      ( WinQueryButtonCheckstate( page01, CH_TAG_GENRE     ) ? DECODER_HAVE_GENRE     : 0 );

    WinDestroyWindow( page01 );
    WinDestroyWindow( hwnd   );

    cfg.tags_choice = new_info.haveinfo;

    if( new_info.haveinfo ) {
      *info = new_info;
      return TAG_APPLY_TO_GROUP;
    }
  }

  return TAG_NO_CHANGES;
}

/* Sends request about applying a meta information to the specified file. */
BOOL
tag_apply( const char* filename, char* decoder, DECODER_INFO* info, int options )
{
  AMP_FILEINFO* data = malloc( sizeof( AMP_FILEINFO ));

  if( data ) {
    data->filename = strdup( filename );
    data->info     = *info;
    data->options  = options;

    if( data->filename ) {
      strlcpy( data->decoder, decoder, sizeof( data->decoder ));
      return qu_write( broker_queue, TAG_UPDATE, data );
    } else {
      free( data->filename );
    }
  }

  return FALSE;
}

/* Initializes of the tagging module. */
void
tag_init( void )
{
  ASSERT_IS_MAIN_THREAD;
  broker_queue = qu_create();

  if( !broker_queue ) {
    amp_player_error( "Unable create tags service queue." );
  } else {
    if(( broker_tid = _beginthread( tag_broker, NULL, 204800, NULL )) == -1 ) {
      amp_player_error( "Unable create the tags service thread." );
    }
  }
}

/* Terminates  of the tagging module. */
void
tag_term( void )
{
  ULONG request;
  PVOID data;

  ASSERT_IS_MAIN_THREAD;

  while( !qu_empty( broker_queue )) {
    qu_read( broker_queue, &request, &data );
    if( data ) {
      free(((AMP_FILEINFO*)data)->filename );
      free( data );
    }
  }

  qu_push( broker_queue, TAG_TERMINATE, NULL );
  wait_thread( broker_tid, 2000 );
  qu_close( broker_queue );
}

