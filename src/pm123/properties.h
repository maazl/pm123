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

#ifndef PM123_PROPERTIES_H
#define PM123_PROPERTIES_H

#include <stdlib.h>

/* Number of items in the recall list. */
#define MAX_RECALL            9

/* Possible sizes of the player window. */
#define CFG_MODE_REGULAR      0
#define CFG_MODE_SMALL        1
#define CFG_MODE_TINY         2

/* Possible scroll modes. */
#define CFG_SCROLL_INFINITE   0
#define CFG_SCROLL_ONCE       1
#define CFG_SCROLL_NONE       2

/* Possible display modes. */
#define CFG_DISP_FILENAME     0
#define CFG_DISP_ID3TAG       1
#define CFG_DISP_FILEINFO     2

typedef struct _amp_cfg {

  // TODO: buffers too small for URLs!!!
  char   filedir[_MAX_PATH];  /* The last directory used for addition of files.    */
  char   listdir[_MAX_PATH];  /* The last directory used for access to a playlist. */
  char   savedir[_MAX_PATH];  /* The last directory used for saving a stream.      */
  char   lasteq [_MAX_PATH];  /* The last directory used for saving a state of an  */
                              /* equalizer.                                        */
  char   defskin[_MAX_PATH];  /* Default skin.                                     */

  BOOL   eq_enabled;          /* Is the equalizer enabled.              */
  int    defaultvol;          /* Current audio volume.                  */
  BOOL   playonload;          /* Start playing on file load.            */
  BOOL   autouse;             /* Auto use playlist on add.              */
  BOOL   playonuse;           /* Auto play on use playlist.             */
  BOOL   selectplayed;        /* Select played file.                    */
  int    mode;                /* See CFG_MODE_*                         */
  int    font;                /* Use font 1 or font 2.                  */
  BOOL   trash;               /* Trash buffers on seek.                 */
//  BOOL   shf;                 /* The state of the "Shuffle" button.     */
//  BOOL   rpt;                 /* The state of the "Repeat" button.      */
  BOOL   floatontop;          /* Float on top.                          */
  BOOL   show_playlist;       /* Show playlist.                         */
  BOOL   show_bmarks;         /* Show bookmarks.                        */
  BOOL   show_plman;          /* Show playlist manager.                 */
  int    scroll;              /* See CFG_SCROLL_*                       */
  int    viewmode;            /* See CFG_DISP_*                         */
  char   proxy[1024];         /* Proxy URL.                             */
  char   auth [1024];         /* HTTP authorization.                    */
  int    buff_wait;           /* Wait before playing.                   */
  int    buff_size;           /* Read ahead buffer size (KB).           */
  BOOL   dock_windows;        /* Dock windows?                          */
  int    dock_margin;         /* The marging for docking window.        */
  BOOL   add_recursive;       /* Enable recursive addition.             */
  BOOL   save_relative;       /* Use relative paths in saved playlists. */
  BOOL   font_skinned;        /* Use skinned font.                      */
  FATTRS font_attrs;          /* Font's attributes.                     */
  LONG   font_size;           /* Font's point size.                     */
  SWP    main;                /* Position of the player.                */

} amp_cfg;

extern amp_cfg cfg;

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize properties, called from main. */
void cfg_init( void );

/* Creates the properties dialog. */
void cfg_properties( HWND owner );

#ifdef __cplusplus
}
#endif
#endif /* PM123_PROPERTIES_H */

