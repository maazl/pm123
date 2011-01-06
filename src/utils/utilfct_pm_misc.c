/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com> *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2009 Marcel Mueller
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

#define  INCL_PM
#include "utilfct.h"
#include <os2.h>
#include <string.h>

#include "debuglog.h"


/* Returns TRUE if WarpSans is supported by operating system. */
BOOL
check_warpsans( void )
{
  static int have_warpsans = -1;

  if( have_warpsans == -1 )
  {
    LONG fontcounter = 0;
    HPS  hps;
    BOOL rc;

    hps = WinGetPS( HWND_DESKTOP );
    rc  = GpiQueryFonts( hps, QF_PUBLIC, "WarpSans", &fontcounter, 0, NULL );
    WinReleasePS( hps );

    have_warpsans = ( rc != 0 && rc != GPI_ALTERROR );
  }

  return have_warpsans;
}

/* Assigns the 9.WarpSans as default font for a specified window if it is supported by
   operating system. Otherwise assigns the 8.Helv as default font. */
void
do_warpsans( HWND hwnd )
{
  char *font = check_warpsans() ? "9.WarpSans" : "8.Helv";
  WinSetPresParam( hwnd, PP_FONTNAMESIZE, strlen( font ) + 1, font );
}

HWND dlg_addcontrol( HWND hwnd, PSZ cls, PSZ text, ULONG style,
                     LONG x, LONG y, LONG cx, LONG cy, SHORT after,
                     USHORT id, PVOID ctldata, PVOID presparams )
{ POINTL pos[2] = {{ x, y }, { x + cx, y + cy }};
  HWND behind;
  if (!WinMapDlgPoints( hwnd, pos, 2, TRUE ))
    return NULLHANDLE;
  behind = after == NULLHANDLE ? HWND_BOTTOM : WinWindowFromID( hwnd, after );
  return WinCreateWindow( hwnd, cls, text, style, pos[0].x, pos[0].y, pos[1].x-pos[0].x, pos[1].y-pos[0].y,
                          hwnd, behind, id, ctldata, presparams);
}


/* Return the ID of the selected radiobutton in a group. */
SHORT rb_selected(HWND hwnd, SHORT id)
{
  HWND cur = WinWindowFromID(hwnd, id);
  PMASSERT(cur);
  HWND first = cur;
  HWND prev = cur; // Work around for PM bug
  cur = WinEnumDlgItem(hwnd, cur, EDI_FIRSTGROUPITEM);
  do
  { if (WinSendMsg(cur, BM_QUERYCHECK, 0, 0))
      return WinQueryWindowUShort(cur, QWS_ID);
    prev = cur;
    cur = WinEnumDlgItem(hwnd, cur, EDI_NEXTGROUPITEM);
  } while (cur != first && cur != prev);
  return -1;
}

/* Sets the enable state of the entryfield in the dialog template to
   the enable flag. */
void
en_enable( HWND hwnd, SHORT id, BOOL enable )
{
  ULONG bg_color = enable ? SYSCLR_ENTRYFIELD : SYSCLR_DIALOGBACKGROUND;
  HWND  hcontrol = WinWindowFromID( hwnd, id );

  if( hcontrol ) {
    WinSendMsg( hcontrol, EM_SETREADONLY, MPFROMSHORT( !enable ), 0 );
    WinSetPresParam( hcontrol, PP_BACKGROUNDCOLORINDEX, sizeof( bg_color ), &bg_color );
  }
}

BOOL sb_setnumlimits( HWND hwnd, USHORT id, LONG low, LONG high, USHORT len)
{ HWND sb = WinWindowFromID( hwnd, id );
  return sb != NULLHANDLE
    && WinSendMsg( sb, SPBM_SETLIMITS, MPFROMLONG( high ), MPFROMLONG( low ))
    && WinSendMsg( sb, SPBM_SETTEXTLIMIT, MPFROMSHORT( len ), 0 );
}
