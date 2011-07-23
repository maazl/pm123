/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2010 M.Mueller
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

#ifndef  DIALOG_H
#define  DIALOG_H

#define INCL_WIN
#include <config.h>
#include <decoder_plug.h>
#include <cpp/url123.h>
#include <os2.h>

// Forward declarations
class APlayable;
class Playable;


/* Return help manager instance.
 * Not valid until dlg_init(). */
//HWND amp_help_mgr();

/// Creates and displays a error message window.
void amp_error( HWND owner, const char* format, ... );
//void amp_verror( HWND owner, const char* format, va_list va );
/// Creates and displays a error message window.
void amp_player_error( const char* format, ... );
/// Creates and displays a message window.
void amp_info ( HWND owner, const char* format, ... );
/// Requests the user about specified action.
BOOL amp_query( HWND owner, const char* format, ... );
/// Requests the user about specified action. With cancel button.
USHORT amp_query3( HWND owner, const char* format, ... );
/// Requests the user about overwriting a file.
BOOL amp_warn_if_overwrite( HWND owner, const char* filename );
/* Tells the help manager to display a specific help window. */
//bool  amp_show_help( SHORT resid );

/// Wizard function for the default entry "File..."
ULONG DLLENTRY amp_file_wizard( HWND owner, const char* title, DECODER_INFO_ENUMERATION_CB callback, void* param );
/// Wizard function for the default entry "URL..."
ULONG DLLENTRY amp_url_wizard( HWND owner, const char* title, DECODER_INFO_ENUMERATION_CB callback, void* param );
/// Open playlist file dialog.
url123 amp_playlist_select(HWND owner, const char* title);

/// Loads a skin selected by the user.
bool amp_loadskin();


/// Adds a user selected bookmark.
void amp_add_bookmark(HWND owner, APlayable& item);
/// File select dialog to save a playlist.
/// @param hwnd Parent window.
/// @param playlist Playlist to save.
/// @param format IN/OUT format to save. Preselected Format on input, selected format on output.
/// @return Path where the file has been saved. \c NULL in case the user did not press OK.
//url123 amp_save_playlist(HWND owner, Playable& playlist, xstring& format);
url123 amp_save_playlist(HWND owner, Playable& playlist, bool saveas);
/* Returns TRUE if the save stream feature has been enabled. */
//void  amp_save_stream( HWND hwnd, BOOL enable );


#endif /* DIALOG_H */

