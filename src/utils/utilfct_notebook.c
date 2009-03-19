/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com> *
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
#include <stdio.h>
#include <string.h>

#include "debuglog.h"


/* Append a tabbed dialog page */
ULONG
nb_append_tab(HWND book, HWND page, const char* major, char* minor, MPARAM index)
{ USHORT style = BKA_AUTOPAGESIZE | (SHORT2FROMMP(index) > 1) * (BKA_MINOR|BKA_STATUSTEXTON) | (SHORT1FROMMP(index) <= 1) * BKA_MAJOR;
  BOOKPAGEINFO bi = { sizeof bi };
  ULONG id = LONGFROMMR(WinSendMsg(book, BKM_INSERTPAGE, 0, MPFROM2SHORT(style, BKA_LAST)));
  if (id == 0)
    return 0;
  bi.fl = BFA_PAGEFROMHWND;
  bi.hwndPage = page;
  // set major text if supplied
  if (major)
  { bi.fl |= BFA_MAJORTABTEXT;
    bi.cbMajorTab = strlen(major);
    bi.pszMajorTab = (PSZ)major;
  }
  // set minor text if supplied
  if (minor)
  { bi.fl |= BFA_MINORTABTEXT;
    bi.cbMinorTab = strlen(minor);
    bi.pszMinorTab = (PSZ)minor;
  }
  // set status text if multiple pages
  if (style & BKA_STATUSTEXTON)
  { char buf[32];
    bi.fl |= BFA_STATUSLINE;
    bi.cbStatusLine = sprintf(buf, "Page %u of %u", SHORT1FROMMP(index), SHORT2FROMMP(index));
    bi.pszStatusLine = buf;
  }
  if (!WinSendMsg(book, BKM_SETPAGEINFO, MPFROMLONG(id), MPFROMP(&bi)))
    return 0;
  return id;
}

/* Adjusting the position and size of a notebook window. */
BOOL
nb_adjust( HWND hwnd, SWP* pswp )
{
  LONG dx = pswp[0].cx - pswp[1].cx;
  LONG dy = pswp[0].cy - pswp[1].cy;
  HWND hnext;
  char classname[64];
  SWP swp;
  BOOL rc = TRUE;
  LONG dx2 = (dx + (pswp[0].cx&1)) >> 1; // stable formula for rounding at the division by 2
  HENUM henum;

  if ((dx | dy) == 0)
    return TRUE;

  henum = WinBeginEnumWindows( hwnd );
  while(( hnext = WinGetNextWindow( henum )) != NULLHANDLE ) {
    if( WinQueryClassName( hnext, sizeof( classname ), classname ) > 0 ) {
      if( strcmp( classname, "#40" ) == 0 )
      { // Notebook => apply dx and dy to cx, cy
        PMRASSERT( WinQueryWindowPos( hnext, &swp ));
        swp.cx += dx;
        swp.cy += dy;
        if (swp.cx < 1 || swp.cy < 1)
        { rc = FALSE;
          // TODO: undo
          break;
        }
        PMRASSERT( WinSetWindowPos( hnext, 0, 0, 0, swp.cx, swp.cy, SWP_SIZE ));
      } else if( dx2 && strcmp( classname, "#3" ) == 0 )
      { // Button => apply dx/2 to x only
        // TODO: if above notebook: swp.y + dy;
        PMRASSERT( WinQueryWindowPos( hnext, &swp ));
        PMRASSERT( WinSetWindowPos( hnext, 0, swp.x + dx2, swp.y, 0, 0, SWP_MOVE ));
        PMRASSERT( WinInvalidateRect( hnext, NULL, FALSE ));
      }
    }
  }
  WinEndEnumWindows( henum );
  return rc;
}
