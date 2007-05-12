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

/*
 * Button '95 include file
 */

#ifndef PM123_BUTTON95_H
#define PM123_BUTTON95_H

#include <os2.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLASSNAME "Button95"

#define MYM_UPDATE   (WM_USER)
#define WM_DEPRESS   (WM_USER + 1)
#define WM_SETTEXT   (WM_USER + 2)
#define WM_PRESS     (WM_USER + 3)
#define WM_CHANGEBMP (WM_USER + 4)

#define BITMAP_SERVER   1667
#define BITMAP_SHUTDOWN 1668
#define BITMAP_RUN      1669
#define BITMAP_HELP     1700
#define BITMAP_FIND     1701
#define BITMAP_SETTINGS 1702
#define BITMAP_DOCS     1703
#define BITMAP_PROGRAMS 1704


typedef struct _BUTTONDATA {
  unsigned int cb;
  char Text[256];
  int Pressed;
  int no;
  HWND hwndOwner;
  ULONG id;
  char Help[256];
} BUTTONDATA;

typedef struct _DATA95 {
  unsigned int cb;
  int bmp1, bmp2;
  int stick;
  int *stickvar;
  int Pressed;
  int id;
  HWND hwndOwner;
  char Help[256];
  int bubbling, wmpaint_kludge, mouse_in_button;
  HWND Bubble, BubbleClient;
} DATA95, *PDATA95;

typedef BUTTONDATA *PBUTTONDATA;

void InitButton(HAB hab);

#ifdef __cplusplus
}
#endif
#endif
