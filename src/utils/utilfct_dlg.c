/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com> *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2011 Marcel Mueller
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


typedef struct
{ PPResizeInfo   rs;
  PPResizeConstr cs;
} PPResizeParams;


/** Caluate the resize delta from old and new size.
 * @param oldsize Size of the owner before resizing
 * @param newsize New size of the owner
 * @param factor  Rational resize factor.
 * @return        Size change to be applied to the child control.
 */
INLINE int calc_delta(SHORT oldsize, SHORT newsize, PPResizeFactor factor)
{ return newsize * factor.Numerator / factor.Denominator - oldsize * factor.Numerator / factor.Denominator;
}
/** Caluate the resize delta from old and new size with min size constraint.
 * @param oldsize Size of the owner before resizing
 * @param delta   Maximum allowed shrinking. This should usually be negative,
 *                but it might be positive if the minimum size constraint is already violated.
 *                In this case the current size is kept.
 * @param factor  Rational resize factor.
 * @return        New size of the owner.
 */
INLINE int calc_newsize(SHORT oldsize, int* delta, PPResizeFactor factor)
{ int new_scaled;
  if (*delta > 0)
    *delta = 0;
  new_scaled = oldsize*factor.Numerator/factor.Denominator + *delta;
  return new_scaled*factor.Denominator/factor.Numerator;
}

/**
 * Applies a size change to a child control using the resize info presentation parameters.
 * @param pos    Child window position, is modified in place to meet the new size.
 * @param rsinfo Resizing information from presentation parameter \c PPU_RESIZEINFO.
 * @param resize Requested size change. The cxnew and cynew fields might be adjusted
 *               if the resizing would cause a minimum size constraint in the \a rsinfo
 *               to be violated otherwise.
 * @remarks Note that the fact that the resize information may be changed causes
 * the resizing to get inconsistent if some child controls already have been resized
 * without checking the constrains of all siblings. Because of that you must operate
 * with two passes. In the first the resize should be simulated and the constraints should
 * be applied, in the second pass the resize actually takes place. The first pass
 * should be done in \c WM_ADJUSTWINDOWPOS processing, while the second pass should be done
 * in \c WM_WINDOWPOSCHANGED processing.
 */
static void apply_resize(SWP* pos, const PPResizeParams* rsinfo, SWP* oldsize, SWP* newsize)
{ // x resize
  int delta = calc_delta(oldsize->cx, newsize->cx, rsinfo->rs.cx_resize);
  if (delta < 0 && pos->cx + delta < rsinfo->cs.cx_min)
  { // Violated min constraint => adjust cxnew
    delta = rsinfo->cs.cx_min - pos->cx;
    newsize->cx = calc_newsize(oldsize->cx, &delta, rsinfo->rs.cx_resize);
    if (newsize->x > oldsize->x)
      newsize->x = oldsize->x;
  }
  pos->cx += delta;
  pos->x += calc_delta(oldsize->cx, newsize->cx, rsinfo->rs.x_resize);
  // y resize
  delta = calc_delta(oldsize->cy, newsize->cy, rsinfo->rs.cy_resize);
  if (delta < 0 && pos->cy + delta < rsinfo->cs.cy_min)
  { // Violated min constraint => adjust cynew
    delta = rsinfo->cs.cy_min - pos->cy;
    newsize->cy = calc_newsize(oldsize->cy, &delta, rsinfo->rs.cy_resize);
    if (newsize->y > oldsize->y)
      newsize->y = oldsize->y;
  }
  pos->cy += delta;
  pos->y += calc_delta(oldsize->cy, newsize->cy, rsinfo->rs.y_resize);
  // identify move operation
  pos->fl |= SWP_MOVE|SWP_SIZE;
}

static void map_size(HWND hwnd, PPResizeConstr* cs)
{ POINTL points[2] = {{ cs->cx_min, cs->cy_min }, { cs->cx_max, cs->cy_max }};
  PMRASSERT(WinMapDlgPoints(hwnd, points, 2, TRUE));
  cs->cx_min = (USHORT)points[0].x;
  cs->cy_min = (USHORT)points[0].y;
  cs->cx_max = (USHORT)points[1].x;
  cs->cy_max = (USHORT)points[1].y;
}

void dlg_adjust_resize(HWND hwnd, SWP* pswp)
{ SWP cursize;
  DEBUGLOG(("dlg_adjust_resize(%p, {%x, %i,%i, %i,%i})\n", hwnd,
    pswp->fl, pswp->cx, pswp->cy, pswp->x, pswp->y));

  // query current size
  PMRASSERT(WinQueryWindowPos(hwnd, &cursize));
  if (cursize.cx == pswp->cx && cursize.cy == pswp->cy)
    return;

  // Apply size constraint of the frame
  if (pswp->cx < cursize.cx || pswp->cy < cursize.cy)
  { PPResizeConstr cstr;
    if (WinQueryPresParam(hwnd, PPU_RESIZECONSTR, 0, NULL, sizeof cstr, &cstr, QPF_NOINHERIT) != 0)
    { map_size(hwnd, &cstr);
      if (pswp->cx < cstr.cx_min && pswp->cx < cursize.cx)
        pswp->cx = cstr.cx_min;
      if (pswp->cy < cstr.cy_min && pswp->cy < cursize.cy)
        pswp->cy = cstr.cy_min;
      // A size constraint at the top level is assumed to be sufficient.
      goto done;
    }
  }

  // for all children
  { HENUM en = WinBeginEnumWindows(hwnd);
    HWND child;
    PMASSERT(en != NULLHANDLE);
    while ((child = WinGetNextWindow(en)) != NULLHANDLE)
    { // find PPU_RESIZEINFO
      PPResizeParams rsinfo = {{0},{1,1}};
      SWP childpos;
      if (WinQueryPresParam(child, PPU_RESIZEINFO, 0, NULL, sizeof rsinfo.rs, &rsinfo.rs, QPF_NOINHERIT) == 0)
        continue;
      WinQueryPresParam(child, PPU_RESIZECONSTR, 0, NULL, sizeof rsinfo.cs, &rsinfo.cs, QPF_NOINHERIT);
      map_size(hwnd, &rsinfo.cs);
      PMRASSERT(WinQueryWindowPos(child, &childpos));
      DEBUGLOG(("dlg_adjust_resize: {%i/%i,%i/%i, %i/%i,%i/%i, %i,%i} :> %p {%x, %i,%i, %i,%i}\n",
        rsinfo.rs.x_resize.Numerator, rsinfo.rs.x_resize.Denominator, rsinfo.rs.y_resize.Numerator, rsinfo.rs.y_resize.Denominator,
        rsinfo.rs.cx_resize.Numerator, rsinfo.rs.cx_resize.Denominator, rsinfo.rs.cy_resize.Numerator, rsinfo.rs.cy_resize.Denominator,
        rsinfo.cs.cx_min, rsinfo.cs.cy_min,
        child, childpos.fl, childpos.x, childpos.y, childpos.cx, childpos.cy));
      apply_resize(&childpos, &rsinfo, &cursize, pswp);
    }
    PMRASSERT(WinEndEnumWindows(en));
  }

 done:
  // Apply adjustments
  DEBUGLOG(("dlg_adjust_resize: -> %i,%i\n", pswp->cx, pswp->cy));
}

void dlg_do_resize(HWND hwnd, SWP* pswpnew, SWP* pswpold)
{ HENUM en;
  HWND child;
  size_t count;
  SWP* childpos_list;
  PPResizeParams rsinfo = {{0}};

  DEBUGLOG(("dlg_do_resize(%p, {%x, %i,%i, %i,%i}, {%x, %i,%i, %i,%i})\n", hwnd,
    pswpnew->fl, pswpnew->cx, pswpnew->cy, pswpnew->x, pswpnew->y,
    pswpold->fl, pswpold->cx, pswpold->cy, pswpold->x, pswpold->y));
  if (pswpold->cx == pswpnew->cx && pswpold->cy == pswpnew->cy)
    return;
  // for all children
  en = WinBeginEnumWindows(hwnd);
  PMASSERT(en != NULLHANDLE);
  // count the children with PPU_RESIZEINFO first.
  while ((child = WinGetNextWindow(en)) != NULLHANDLE)
  { ULONG found = 0;
    WinQueryPresParam(child, PPU_RESIZEINFO, 0, &found, 0, NULL, QPF_NOINHERIT);
    if (found)
      ++count;
  }
  if (count == 0)
  { PMRASSERT(WinEndEnumWindows(en));
    return;
  }
  // Allocate target structure.
  childpos_list = (SWP*)calloc(count, sizeof(SWP));
  count = 0;
  while ((child = WinGetNextWindow(en)) != NULLHANDLE)
  { SWP* childpos;
    // find PPU_RESIZEINFO, but do not load size constraints.
    if (WinQueryPresParam(child, PPU_RESIZEINFO, 0, NULL, sizeof rsinfo.rs, &rsinfo.rs, QPF_NOINHERIT) == 0)
      continue;
    childpos = childpos_list + count++;
    PMRASSERT(WinQueryWindowPos(child, childpos));
    DEBUGLOG(("dlg_do_resize: {%i/%i,%i/%i, %i/%i,%i/%i, } :> {%x, %i,%i, %i,%i}\n",
      rsinfo.rs.x_resize.Numerator, rsinfo.rs.x_resize.Denominator, rsinfo.rs.y_resize.Numerator, rsinfo.rs.y_resize.Denominator,
      rsinfo.rs.cx_resize.Numerator, rsinfo.rs.cx_resize.Denominator, rsinfo.rs.cy_resize.Numerator, rsinfo.rs.cy_resize.Denominator,
      childpos->fl, childpos->x, childpos->y, childpos->cx, childpos->cy));
    childpos->fl = SWP_NOADJUST; // Fix: entry fields otherwise grow unlimited. (Probably PM bug.)
    apply_resize(childpos, &rsinfo, pswpold, pswpnew);
    DEBUGLOG(("dlg_do_resize: -> {%x, %i,%i, %i,%i}\n",
      childpos->fl, childpos->x, childpos->y, childpos->cx, childpos->cy));
  }
  PMRASSERT(WinEndEnumWindows(en));
  // Resize children
  if (count)
    PMRASSERT(WinSetMultWindowPos(WinQueryAnchorBlock(hwnd), childpos_list, count));
  free(childpos_list);
}

HWND dlg_addcontrol( HWND hwnd, PSZ cls, PSZ text, ULONG style,
                     LONG x, LONG y, LONG cx, LONG cy, SHORT after,
                     USHORT id, PVOID ctldata, PVOID presparams )
{ POINTL pos[2];
  HWND behind;
  pos[0].x = x;
  pos[0].y = y;
  pos[1].x = x + cx;
  pos[1].y = y + cy;
  if (!WinMapDlgPoints( hwnd, pos, 2, TRUE ))
    return NULLHANDLE;
  behind = after == NULLHANDLE ? HWND_BOTTOM : WinWindowFromID( hwnd, after );
  return WinCreateWindow( hwnd, cls, text, style, pos[0].x, pos[0].y, pos[1].x-pos[0].x, pos[1].y-pos[0].y,
                          hwnd, behind, id, ctldata, presparams);
}
