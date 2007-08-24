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


/* Code for the playlist manager */

#define  INCL_WIN

#include "playlist_base.h"
#include "pm123.h"
#include "docking.h"
#include <utilfct.h>
#include <os2.h>

#include <debuglog.h>


#ifdef DEBUG
xstring PlaylistBase::RecordBase::DebugName() const
{ if (IsRemoved())
    return xstring::sprintf("%p{<removed>}", this);
  return xstring::sprintf("%p{%p{%s}}", this, Content, Content->GetPlayable().GetURL().getObjName().cdata());
} 
xstring PlaylistBase::RecordBase::DebugName(const RecordBase* rec)
{ static const xstring nullstring = "<NULL>";
  if (!rec)
    return nullstring;
  return rec->DebugName();
}

xstring PlaylistBase::DebugName() const
{ return Content->GetURL().getObjName();
} 
#endif


/****************************************************************************
*
*  class PlaylistBase
*
****************************************************************************/

HPOINTER PlaylistBase::IcoSong[3];
HPOINTER PlaylistBase::IcoPlaylist[3][3];
HPOINTER PlaylistBase::IcoEmptyPlaylist;
HPOINTER PlaylistBase::IcoInvalid;

void PlaylistBase::InitIcons()
{ IcoSong    [IC_Normal]                = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3);
  IcoSong    [IC_Used]                  = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3USED);
  IcoSong    [IC_Play]                  = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3PLAY);
  IcoPlaylist[IC_Normal][ICP_Closed]    = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLCLOSE);
  IcoPlaylist[IC_Normal][ICP_Open]      = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLOPEN);
  IcoPlaylist[IC_Normal][ICP_Recursive] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLRECURSIVE);
  IcoPlaylist[IC_Used]  [ICP_Closed]    = IcoPlaylist[IC_Normal][ICP_Closed];
  IcoPlaylist[IC_Used]  [ICP_Open]      = IcoPlaylist[IC_Normal][ICP_Open];
  IcoPlaylist[IC_Used]  [ICP_Recursive] = IcoPlaylist[IC_Normal][ICP_Recursive];
  IcoPlaylist[IC_Play]  [ICP_Closed]    = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLCLOSEPLAY);
  IcoPlaylist[IC_Play]  [ICP_Open]      = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLOPENPLAY);
  IcoPlaylist[IC_Play]  [ICP_Recursive] = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLRECURSIVEPLAY);
  IcoEmptyPlaylist                      = WinLoadPointer(HWND_DESKTOP, 0, ICO_PLEMPTY);
  IcoInvalid                            = WinLoadPointer(HWND_DESKTOP, 0, ICO_MP3INVLD);
}

PlaylistBase::PlaylistBase(const char* url, const char* alias)
: Content(Playable::GetByURL(url)),
  Alias(alias),
  HwndFrame(NULLHANDLE),
  HwndContainer(NULLHANDLE),
  CmFocus(NULL),
  NoRefresh(false)
{ DEBUGLOG(("PlaylistBase(%p)::PlaylistBase(%s)\n", this, url));
  static bool first = true;
  if (first)
  { InitIcons();
    first = false;
  }
  /*// These two ones are constant
  LoadWizzards[0] = &amp_file_wizzard;
  LoadWizzards[1] = &amp_url_wizzard;*/
}

void PlaylistBase::PostRecordCommand(RecordBase* rec, RecordCommand cmd)
{ DEBUGLOG(("PlaylistBase(%p)::PostRecordCommand(%p, %u)\n", this, rec, cmd));
  Interlocked il(StateFromRec(rec).PostMsg);
  // Check whether the requested bit is already set or another event or there are other events pending.
  if (il.bitset(cmd) || il != (1U<<cmd))
  { DEBUGLOG(("PlaylistBase::PostRecordCommand - nope! %u\n", (unsigned)il));
    return; // requested command is already on the way or another unexecuted message is outstanding
  }
  // There is a little chance that we generate two messages for the same record.
  // The second one will be a no-op in the window procedure.  
  if (rec)
    InterlockedInc(rec->UseCount);
  if (!WinPostMsg(HwndFrame, UM_RECORDCOMMAND, MPFROMP(rec), MPFROMSHORT(TRUE)))
    FreeRecord(rec); // avoid memory leaks
}

void PlaylistBase::FreeRecord(RecordBase* rec)
{ DEBUGLOG(("PlaylistBase(%p)::FreeRecord(%p)\n", this, rec));
  if (rec && InterlockedDec(rec->UseCount) == 0)
    // we can safely post this message because the record is now no longer used anyway.
    WinPostMsg(HwndFrame, UM_DELETERECORD, MPFROMP(rec), 0);
}

int PlaylistBase::CompareTo(const char* str) const
{ return stricmp(Content->GetURL(), str);
}

void PlaylistBase::SetVisible(bool show)
{ DEBUGLOG(("PlaylistBase(%p{%s})::SetVisible(%u)\n", this, DebugName().cdata(), show));
  HSWITCH hswitch = WinQuerySwitchHandle( HwndFrame, 0 );
  SWCNTRL swcntrl;

  if( WinQuerySwitchEntry( hswitch, &swcntrl ) == 0 ) {
    swcntrl.uchVisibility = show ? SWL_VISIBLE : SWL_INVISIBLE;
    WinChangeSwitchEntry( hswitch, &swcntrl );
  }

  dk_set_state( HwndFrame, show ? 0 : DK_IS_GHOST );
  WinSetWindowPos( HwndFrame, HWND_TOP, 0, 0, 0, 0,
                   show ? SWP_SHOW | SWP_ZORDER | SWP_ACTIVATE : SWP_HIDE );
}


