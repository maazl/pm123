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

#ifndef _PM123_DOCKING_H
#define _PM123_DOCKING_H

#ifdef __cplusplus
extern "C" {
#endif

#define DK_MAX_DOCKED 32      /* The maximum of the used dockable windows.  */
#define DK_IS_MASTER  0x0001  /* Is the master window.                      */
#define DK_IS_GHOST   0x0002  /* Is the invisible window.                   */
#define DK_IS_DOCKED  0x0004  /* Is the docked window.                      */
#define DK_IS_PULLED  0x0008  /* Is the window already pulled by his owner. */

#define WM_DOCKWINDOW (WM_USER+737)

typedef struct _DK_DATA {

  HWND   hwnd;
  PFNWP  def_proc;
  int    state;

  struct _DK_DATA* childs[DK_MAX_DOCKED];
  struct _DK_DATA* owners[DK_MAX_DOCKED];

  int    childs_count;
  int    owners_count;

} DK_DATA, *PDK_DATA;

/** Adds a specified window to the docking subsystem. */
BOOL dk_add_window( HWND hwnd, int state );
/** Removes a specified window from the docking subsystem. */
BOOL dk_remove_window( HWND hwnd );
/** Returns a state of the specified window. */
int  dk_get_state( HWND hwnd );
/** Sets a state of the specified window. */
void dk_set_state( HWND hwnd, int state );

/* Rebuilds all relationships with the specified window. */
void dk_arrange( HWND hwnd );
/* Cleanups all relationships with the specified window. */
void dk_cleanup( HWND hwnd );

/** Initializes of the docking subsystem. */
void dk_init( void );
/** Terminates of the docking subsystem. */
void dk_term( void );

#ifdef __cplusplus
}
#endif
#endif /* _PM123_DOCKING_H */


