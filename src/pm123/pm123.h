/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 M.Mueller
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

#ifndef  PM123_H
#define  PM123_H

#define INCL_WIN
#include <config.h>

#include "playablecollection.h"
#include "controller.h"
#include <os2.h>


#define  AMP_REFRESH_CONTROLS   ( WM_USER + 1000 ) // 0,         0
#define  AMP_PAINT              ( WM_USER + 1001 ) // maske,     0
#define  AMP_LOAD               ( WM_USER + 1002 ) // LoadHelper* 
#define  AMP_RELOADSKIN         ( WM_USER + 1003 ) // 0,         0
#define  AMP_DISPLAY_MESSAGE    ( WM_USER + 1013 ) // message,   TRUE (info) or FALSE (error)
#define  AMP_DISPLAY_MODE       ( WM_USER + 1014 ) // 0,         0
#define  AMP_QUERY_STRING       ( WM_USER + 1015 ) // buffer,    size and type
#define  AMP_SHOW_DIALOG        ( WM_USER + 1016 ) // iep,       0
#define  AMP_CTRL_EVENT         ( WM_USER + 1020 )
#define  AMP_CTRL_EVENT_CB      ( WM_USER + 1021 )
#define  AMP_REFRESH_ACCEL      ( WM_USER + 1022 )
#define  AMP_SLIDERDRAG         ( WM_USER + 1023 ) // pos(x,y),  TRUE: navigate and complete
#define  AMP_PIPERESTART        ( WM_USER + 1030 ) // 0,         0
#define  AMP_WORKERADJUST       ( WM_USER + 1031 ) // 0,         0


/* Constructs a information text for currently loaded file. */
void  amp_display_filename( void );
/* Switches to the next text displaying mode. */
void  amp_display_next_mode( void );

/* Helper class to load one or more objects into PM123.
 */
class LoadHelper : public Iref_count
{public:
  enum Options
  { OptDefault       = 0,
    LoadPlay         = 0x01, // Start Playing when completed
    LoadRecall       = 0x02, // Add item to the MRU-List if it is only one
    LoadAppend       = 0x04, // Always append to the default playlist
    LoadKeepPlaylist = 0x08, // Play a playable object. If A playlist containing this item is loaded, the item is activated only.
    ShowErrors       = 0x10, // Show errors on failure.
    AutoPost         = 0x20, // Auto post command if the class instance is destroyed.
    PostDelayed      = 0x40  // Do the job by the main message queue.
  };
 protected:
  const Options         Opt;
  vector_int<PlayableSlice> Items;
 protected:
  // Returns /one/ PlayableSlices that represents all the objects in the list.
  virtual PlayableSlice*        ToPlayableSlice();
  // Create a sequence of controller commands from the current list.
  virtual Ctrl::ControlCommand* ToCommand();
 public:
  // Initialize LoadHelper
                        LoadHelper(Options opt)    : Opt(opt), Items(20) {}
                        ~LoadHelper();
  // Add a item to play to the list of items.
  void                  AddItem(PlayableSlice* ps) { Items.append() = ps; }
  // Post the command above (if any)
  virtual void          PostCommand();
  // Send the command above (if any) and wait for the reply.
  Ctrl::RC              SendCommand();
};
FLAGSATTRIBUTE(LoadHelper::Options);


/* amp_load_playable options */
#define AMP_LOAD_NOT_PLAY      0x0001 // Load a playable object, but do not start playback automatically
#define AMP_LOAD_NOT_RECALL    0x0002 // Load a playable object, but do not add an entry into the list of recent files
#define AMP_LOAD_KEEP_PLAYLIST 0x0004 // Play a playable object. If A playlist containing this item is loaded, the item is activated only.
#define AMP_LOAD_APPEND        0x0008 // Take a playable object as part of multiple playable objects to load. 

/* Marks the player window as needed of redrawing. */
void  amp_invalidate( unsigned options );
/* amp_invalidate options - DO NOT CHANGE THESE CONSTANTS (dependencies) */
#define UPD_TIMERS           0x0001
#define UPD_FILEINFO         0x0002
#define UPD_VOLUME           0x0004
#define UPD_WINDOW           0x0008
#define UPD_FILENAME         0x0010
#define UPD_DELAYED          0x8000

/* Posts a command to the message queue associated with the player window. */
//BOOL  amp_post_command( USHORT id );
/* Returns the handle of the player frame window. */
HWND amp_player_window( void );
/* Returns the anchor-block handle. */
HAB  amp_player_hab( void );

/* Opens dialog for the specified object. */
enum dialog_type
{ DLT_INFOEDIT,
  DLT_METAINFO,
  DLT_TECHINFO,
  DLT_PLAYLIST,
  DLT_PLAYLISTTREE
};
void amp_show_dialog( HWND owner, Playable* item, dialog_type dlg );

void amp_control_event_callback(Ctrl::ControlCommand* cmd);

// Update an MRU list with item ps and maximum size max
void amp_AddMRU(Playlist* list, size_t max, const PlayableSlice& ps);


/* Global variables */

/* Contains startup path of the program without its name. */
extern const xstring& startpath;

Playlist* amp_get_default_pl();
Playlist* amp_get_default_bm();
Playlist* amp_get_url_mru();

#endif /* PM123_H */

