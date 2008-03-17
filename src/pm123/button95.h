/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

#ifndef PM123_BUTTON95_H
#define PM123_BUTTON95_H

#ifdef __cplusplus
extern "C" {
#endif

#define CLASSNAME "Button95"

#define WM_DEPRESS   ( WM_USER + 1 )
#define WM_SETHELP   ( WM_USER + 2 )
#define WM_PRESS     ( WM_USER + 3 )
#define WM_CHANGEBMP ( WM_USER + 4 )

typedef struct _DATA95 {

  int   cb;
  HWND  hwnd_owner;
  HWND  hwnd_bubble;
  HWND  hwnd_bubble_frame;
  int   id;
  char  help[256];

  int   bubbling;
  int   mouse_in_button;
  int   pressed;
  int   stick;
  int*  stickvar;

  HBITMAP* bmp_release_id;
  HBITMAP* bmp_pressed_id;

} DATA95, *PDATA95;

void InitButton( HAB hab );

#ifdef __cplusplus
}
#endif
#endif
