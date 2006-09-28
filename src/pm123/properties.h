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

#ifndef  RC_INVOKED
#include <stdlib.h>
#endif

#define DLG_CONFIG         2010
#define NB_CONFIG           100
#define PB_UNDO             101
#define PB_DEFAULT          102
#define PB_HELP             103

#define CFG_PAGE1          3000
#define GB_BEHAVIOUR       3001
#define CB_PLAYONLOAD      3010
#define CB_AUTOUSEPL       3020
#define CB_AUTOPLAYPL      3030
#define CB_SELECTPLAYED    3031
#define CB_TRASHONSEEK     3040
#define CB_DOCK            3150
#define EF_DOCK            3151
#define ST_PIXELS          3152
#define GB_STREAMING       3160
#define ST_PROXY_URL       3170
#define EF_PROXY_URL       3171
#define ST_HTTP_AUTH       3180
#define EF_HTTP_AUTH       3181
#define ST_BUFFERSIZE      3185
#define SB_BUFFERSIZE      3190
#define ST_KB              3195
#define CB_FILLBUFFER      3200

#define CFG_PAGE2          3500
#define GB_TITLE           3501
#define ST_SCROLL          3545
#define RB_SCROLL_INFINITE 3550
#define RB_SCROLL_ONCE     3551
#define RB_SCROLL_DISABLE  3552
#define ST_DISPLAY         3575
#define RB_DISP_FILENAME   3580
#define RB_DISP_ID3TAG     3581
#define RB_DISP_FILEINFO   3582
#define ST_CHARSET         3590
#define CB_CHARSET         3591
#define CB_AUTO_CHARSET    3592
#define GB_FONT            3600
#define CB_USE_SKIN_FONT   3610
#define ST_FONT_SAMPLE     3630
#define PB_FONT_SELECT     3640

#define CFG_PAGE3          4000
#define GB_VISUALPLUGINS   4001
#define LB_VISPLUG         4010
#define PB_VIS_ENABLE      4020
#define PB_VIS_UNLOAD      4030
#define ST_VIS_AUTHOR      4040
#define ST_VIS_DESC        4050
#define PB_VIS_CONFIG      4060
#define PB_VIS_ADD         4070
#define GB_DECODERPLUGINS  4071
#define LB_DECPLUG         4080
#define PB_DEC_ENABLE      4090
#define PB_DEC_UNLOAD      4100
#define ST_DEC_AUTHOR      4110
#define ST_DEC_DESC        4120
#define PB_DEC_CONFIG      4130
#define PB_DEC_ADD         4140

#define CFG_PAGE4          5000
#define GB_FILPLUG         5005
#define LB_FILPLUG         5010
#define PB_FIL_ENABLE      5020
#define PB_FIL_UNLOAD      5030
#define ST_FIL_AUTHOR      5040
#define ST_FIL_DESC        5050
#define PB_FIL_CONFIG      5060
#define PB_FIL_ADD         5070
#define GB_OUTPLUG         5075
#define LB_OUTPLUG         5080
#define PB_OUT_ACTIVATE    5090
#define PB_OUT_UNLOAD      5100
#define ST_OUT_AUTHOR      5110
#define ST_OUT_DESC        5120
#define PB_OUT_CONFIG      5130
#define PB_OUT_ADD         5140

#define CFG_ABOUT          2011
#define ST_TITLE1          2020
#define ST_TITLE2          2030
#define ST_BUILT           2040
#define GB_AUTHORS         2050
#define ST_AUTHORS         2060
#define GB_CREDITS         2070
#define ST_CREDITS         2080

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

  char   filedir[_MAX_PATH];  /* The last directory used for addition of files.    */
  char   listdir[_MAX_PATH];  /* The last directory used for access to a playlist. */
  char   savedir[_MAX_PATH];  /* The last directory used for saving a stream.      */
  char   lasteq [_MAX_PATH];  /* The last directory used for saving a state of an  */
                              /* equalizer.                                        */
  char   defskin[_MAX_PATH];  /* Default skin.                                     */

  char   last[MAX_RECALL][_MAX_PATH];
  char   list[MAX_RECALL][_MAX_PATH];

  char   cddrive[4];          /* Default CD drive.                      */
  BOOL   eq_enabled;          /* Is the equalizer enabled.              */
  int    defaultvol;          /* Current audio volume.                  */
  BOOL   playonload;          /* Start playing on file load.            */
  BOOL   autouse;             /* Auto use playlist on add.              */
  BOOL   playonuse;           /* Auto play on use playlist.             */
  BOOL   selectplayed;        /* Select played file.                    */
  int    mode;                /* See CFG_MODE_*                         */
  int    font;                /* Use font 1 or font 2.                  */
  BOOL   trash;               /* Trash buffers on seek.                 */
  BOOL   shf;                 /* The state of the "Shuffle" button.     */
  BOOL   rpt;                 /* The state of the "Repeat" button.      */
  BOOL   floatontop;          /* Float on top.                          */
  BOOL   show_playlist;       /* Show playlist.                         */
  BOOL   show_bmarks;         /* Show bookmarks.                        */
  BOOL   show_plman;          /* Show playlist manager.                 */
  int    scroll;              /* See CFG_SCROLL_*                       */
  int    viewmode;            /* See CFG_DISP_*                         */
  char   proxy[1024];         /* Proxy URL.                             */
  char   auth [ 256];         /* HTTP authorization.                    */
  int    bufwait;             /* Wait before fucking.                   */
  int    bufsize;             /* KB chunkz rewl.                        */
  BOOL   dock_windows;        /* Dock windows?                          */
  int    dock_margin;         /* The marging for docking window.        */
  BOOL   add_recursive;       /* Enable recursive addition.             */
  BOOL   save_relative;       /* Use relative paths in saved playlists. */
  int    codepage;            /* Codepage for ID3 tags.                 */
  BOOL   auto_codepage;       /* Autodetect codepage.                   */
  BOOL   font_skinned;        /* Use skinned font.                      */
  FATTRS font_attrs;          /* Font's attributes.                     */
  LONG   font_size;           /* Font's point size.                     */
  SWP    main;                /* Position of the player.                */

} amp_cfg;

extern amp_cfg cfg;

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the properties dialog. */
void cfg_properties( HWND owner );

#ifdef __cplusplus
}
#endif
#endif /* PM123_PROPERTIES_H */
