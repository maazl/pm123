/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
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

#ifndef PM123_PFREQ_H
#define PM123_PFREQ_H

#define DLG_PM          48
#define CNR_PM          FID_CLIENT
#define ACL_PM          49

#define MNU_MANAGER         1024
#define IDM_PM_ADD          1000
#define IDM_PM_CALC         1001

#define MNU_FILE            2048
#define IDM_PM_F_RENAME     2049
#define IDM_PM_F_LOAD       2050

#define MNU_LIST            3072
#define IDM_PM_L_RENAME     3073
#define IDM_PM_L_LOAD       3074
#define IDM_PM_L_REMOVE     3075
#define IDM_PM_L_DELETE     3076

#define IDM_PM_MENU         4000
#define IDM_PM_MENU_RECORD  4001

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HPOPULATE;

/* Begins population of the specified playlist. */
HPOPULATE pm_begin_populate( const char* filename );
/* Adds the specified file to the populated playlist. */
BOOL pm_add_file( HPOPULATE handle, const char* filename,
                  int bitrate, int samplerate, int mode, int filesize, int secs );
/* Ends population of the specified playlist. */
void pm_end_populate( HPOPULATE handle );

/* Sets the visibility state of the playlist manager
   presentation window. */
void pm_show( BOOL show );
/* Creates the playlist manager presentation window.
   Must be called from the main thread. */
HWND pm_create( void );
/* Destroys the playlist manager presentation window.
   Must be called from the main thread. */
void pm_destroy( void );

/* Changes the playlist manager colors. */
BOOL pm_set_colors( ULONG, ULONG, ULONG, ULONG );

#ifdef __cplusplus
}
#endif
#endif
