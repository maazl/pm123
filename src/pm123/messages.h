/*
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef PM123_MESSAGES_H
#define PM123_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Returns TRUE if the player is paused. */
BOOL is_paused( void );
/* Returns TRUE if the player is fast forwarding. */
BOOL is_forward( void );
/* Returns TRUE if the player is rewinding. */
BOOL is_rewind( void );
/* Returns TRUE if the output is always hungry. */
BOOL is_always_hungry( void );

/* Begins playback of the specified file. */
BOOL msg_play( HWND hwnd, const MSG_PLAY_STRUCT* play, int pos );
/* Stops playback of the currently played file. */
BOOL msg_stop( void );
/* Suspends or resumes playback of the currently played file. */
BOOL msg_pause( void );
/* Toggles a fast forward of the currently played file. */
BOOL msg_forward( void );
/* Toggles a rewind of the currently played file. */
BOOL msg_rewind( void );
/* Changes the current playing position of the currently played file. */
BOOL msg_seek( int pos );
/* Toggles a saving of the currently played stream. */
BOOL msg_savestream( const char* filename );
/* Toggles a equalizing of the currently played file. */
BOOL msg_equalize( const float* gains, const BOOL* mute, float preamp, BOOL enabled );

#ifdef __cplusplus
}
#endif
#endif /* PM123_MESSAGES_H */


