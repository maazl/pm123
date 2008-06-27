/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 Marcel Mueller
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

#define INCL_WIN
#include <stdlib.h>
#include <os2.h>

// Number of items in the recall lists.
#define MAX_RECALL            9

/* Possible sizes of the player window. */
enum cfg_mode
{ CFG_MODE_REGULAR,
  CFG_MODE_SMALL,
  CFG_MODE_TINY
};

/* Possible scroll modes. */
enum cfg_scroll
{ CFG_SCROLL_INFINITE,
  CFG_SCROLL_ONCE,
  CFG_SCROLL_NONE
};

/* Possible display modes. */
enum cfg_disp
{ CFG_DISP_FILENAME,
  CFG_DISP_ID3TAG,
  CFG_DISP_FILEINFO
};

// Alternate navigation methods
enum
{ CFG_ANAV_SONG,
  CFG_ANAV_SONGTIME,
  CFG_ANAV_TIME
};

typedef struct _amp_cfg {
  // TODO: buffers too small for URLs!!!
  char   defskin[_MAX_PATH];  // Default skin.
                                
  BOOL   playonload;          // Start playing on file load.
  BOOL   autouse;             // Auto use playlist on add.
  BOOL   playonuse;           // Auto play on use playlist.
  BOOL   retainonexit;        // Retain playing position on exit.
  BOOL   retainonstop;        // Retain playing position on stop.
  BOOL   restartonstart;      // Restart playing on startup.
  int    altnavig;            // Alternate navigation method 0=song only, 1=song&time, 2=time only
  BOOL   recurse_dnd;         // Drag and drop of folders recursive
  BOOL   sort_folders;        // Automatically sort filesystem folders (by name)
  BOOL   folders_first;       // Place subfolders before content
  BOOL   append_dnd;          // Drag and drop appends to default playlist
  BOOL   append_cmd;          // Commandline appends to default playlist
  BOOL   queue_mode;          // Delete played items from the default playlist
  int    num_workers;         // Number of worker threads for Playable objects
                                
  int    font;                // Use font 1 or font 2.
  BOOL   font_skinned;        // Use skinned font.
  FATTRS font_attrs;          // Font's attributes.
  LONG   font_size;           // Font's point size.
                                
  BOOL   trash;               // Trash buffers on fast forward
  BOOL   floatontop;          // Float on top.
  int    scroll;              // See CFG_SCROLL_*
  int    viewmode;            // See CFG_DISP_*
  char   proxy[1024];         // Proxy URL.
  char   auth [1024];         // HTTP authorization.
  int    buff_wait;           // Wait before playing.
  int    buff_size;           // Read ahead buffer size (KB).
  int    buff_fill;           // Percent of prefilling of the buffer.
  char   pipe_name[_MAX_PATH];// PM123 remote control pipe name
  BOOL   dock_windows;        // Dock windows?
  int    dock_margin;         // The marging for docking window.

// Player state
  char   filedir[_MAX_PATH];  /* The last directory used for addition of files.    */
  char   listdir[_MAX_PATH];  /* The last directory used for access to a playlist. */
  char   savedir[_MAX_PATH];  /* The last directory used for saving a stream.      */

  int    mode;                /* See CFG_MODE_*                         */

  BOOL   show_playlist;       /* Show playlist.                         */
  BOOL   show_bmarks;         /* Show bookmarks.                        */
  BOOL   show_plman;          /* Show playlist manager.                 */
  BOOL   add_recursive;       /* Enable recursive addition.             */
  BOOL   save_relative;       /* Use relative paths in saved playlists. */
  SWP    main;                /* Position of the player.                */

} amp_cfg;

extern amp_cfg cfg;
extern const amp_cfg cfg_default;

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

