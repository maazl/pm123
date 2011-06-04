/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2011 M.Mueller
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

#include <cpp/xstring.h>
#include <os2.h>


/* Timers */
#define TID_UPDATE_TIMERS    ( TID_USERMAX - 1 )
#define TID_UPDATE_PLAYER    ( TID_USERMAX - 2 )
#define TID_ONTOP            ( TID_USERMAX - 3 )
#define TID_CLEANUP          ( TID_USERMAX - 4 )
#define TID_INSP_AUTOREFR    ( TID_USERMAX - 5 )


/*#define  AMP_REFRESH_CONTROLS   ( WM_USER + 1000 ) // 0,         0
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
*/


/// Abort PM123 with an error message.
void amp_fail(const char* fmt, ...);

/// Returns the anchor-block handle.
extern const HAB& amp_player_hab;

/// Contains the path of the PM123 executable.
extern const xstring& amp_startpath;
/// Contains the path of the configuration data. By default the executable's path.
extern const xstring& amp_basepath;


#endif /* PM123_H */

