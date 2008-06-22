/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 Marcel Mueller
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


#define INCL_BASE
#include "infodialog.h"
#include "pm123.h"
#include "pm123.rc.h"
#include <decoder_plug.h>

#include <stdio.h>


#define UM_UPDATE (WM_USER+1)


InfoDialog::PageBase::PageBase(InfoDialog& parent, ULONG rid)
: DialogBase(rid, NULLHANDLE),
  Parent(parent)
{}

MRESULT InfoDialog::PageBase::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    do_warpsans(GetHwnd());
    WinPostMsg(GetHwnd(), UM_UPDATE, 0, 0);
    break;

   case WM_WINDOWPOSCHANGED:
    if(((SWP*)mp1)[0].fl & SWP_SIZE)
      nb_adjust(GetHwnd());
    break;
  }
  return DialogBase::DlgProc(msg, mp1, mp2);
}

MRESULT InfoDialog::Page1Window::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case UM_UPDATE:
    { // update info values
      DEBUGLOG(("InfoDialog(%p)::Page1Window::DlgProc: UM_UPDATE\n", &Parent));
      char buffer[32];
      HWND ctrl;
      { const PHYS_INFO& phys = *Parent.Content->GetInfo().phys;
        const bool enabled = Parent.Content->CheckInfo(Playable::IF_Phys) == 0;
        // filesize
        ctrl = WinWindowFromID(GetHwnd(), EF_FILESIZE);
        PMRASSERT(WinSetWindowText(ctrl, phys.filesize < 0 ? "n/a" : (sprintf(buffer, "%.3f kiB", phys.filesize/1024.), buffer)));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
        // number of items
        ctrl = WinWindowFromID(GetHwnd(), EF_NUMITEMS);
        PMRASSERT(WinSetWindowText(ctrl, phys.num_items <= 0 ? "n/a" : (sprintf(buffer, "%i", phys.num_items), buffer)));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
      }
      { const TECH_INFO& tech = *Parent.Content->GetInfo().tech;
        const bool enabled = Parent.Content->CheckInfo(Playable::IF_Tech) == 0;
        // total filesize
        ctrl = WinWindowFromID(GetHwnd(), EF_TOTALSIZE);
        PMRASSERT(WinSetWindowText(ctrl, tech.totalsize < 0 ? "n/a" : (sprintf(buffer, "%.3f MiB", tech.totalsize/(1024.*1024.)), buffer)));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
        // playing time
        if (tech.songlength < 0)
          strcpy(buffer, "n/a");
        else
        { unsigned long s = (unsigned long)tech.songlength;
          if (s < 60)
            sprintf(buffer, "%lu s", s);
           else if (s < 3600)
            sprintf(buffer, "%lu:%02lu", s/60, s%60);
           else
            sprintf(buffer, "%lu:%02lu:%02lu", s/3600, s/60%60, s%60);
        }
        ctrl = WinWindowFromID(GetHwnd(), EF_TOTALTIME);
        PMRASSERT(WinSetWindowText(ctrl, buffer));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
        // bitrate
        ctrl = WinWindowFromID(GetHwnd(), EF_BITRATE);
        PMRASSERT(WinSetWindowText(ctrl, tech.bitrate < 0 ? "n/a" : (sprintf(buffer, "%i kbps", tech.bitrate), buffer)));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
        // info string
        ctrl = WinWindowFromID(GetHwnd(), EF_INFOSTRINGS);
        PMRASSERT(WinSetWindowText(ctrl, tech.info));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
      }
      { const RPL_INFO& rpl = *Parent.Content->GetInfo().rpl;
        const bool enabled = (Parent.Content->GetFlags() & Playable::Enumerable) && Parent.Content->CheckInfo(Playable::IF_Rpl) == 0;
        // number of songs
        ctrl = WinWindowFromID(GetHwnd(), EF_SONGITEMS);
        PMRASSERT(WinSetWindowText(ctrl, rpl.total_items <= 0 ? "n/a" : (sprintf(buffer, "%i", rpl.total_items), buffer)));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
        // recursion flag
        ctrl = WinWindowFromID(GetHwnd(), CB_ITEMSRECURSIVE);
        WinSendMsg(ctrl, BM_SETCHECK, MPFROMSHORT(!!rpl.recursive), 0);
        PMRASSERT(WinEnableWindow(ctrl, enabled));
      }
      { const FORMAT_INFO2& format = *Parent.Content->GetInfo().format;
        const bool enabled = Parent.Content->CheckInfo(Playable::IF_Format) == 0;
        // sampling rate
        ctrl = WinWindowFromID(GetHwnd(), EF_SAMPLERATE);
        PMRASSERT(WinSetWindowText(ctrl, format.samplerate < 0 ? "n/a" : (sprintf(buffer, "%i Hz", format.samplerate), buffer)));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
        // channels
        ctrl = WinWindowFromID(GetHwnd(), EF_NUMCHANNELS);
        PMRASSERT(WinSetDlgItemText(GetHwnd(), EF_NUMCHANNELS, format.channels < 0 ? "n/a" : (sprintf(buffer, "%i", format.channels), buffer)));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
      }
      { // Decoder
        const char* dec = Parent.Content->GetDecoder();
        const bool enabled = Parent.Content->CheckInfo(Playable::IF_Other) == 0;
        ctrl = WinWindowFromID(GetHwnd(), EF_DECODER);
        PMRASSERT(WinSetWindowText(ctrl, dec ? dec : "n/a"));
        PMRASSERT(WinEnableWindow(ctrl, enabled));
      }
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

MRESULT InfoDialog::Page2Window::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case UM_UPDATE:
    { // update info values
      DEBUGLOG(("InfoDialog(%p)::Page2Window::DlgProc: UM_UPDATE\n", &Parent));
      char buffer[32];
      HWND ctrl;
      const META_INFO& meta = *Parent.Content->GetInfo().meta;
      const bool enabled = Parent.Content->CheckInfo(Playable::IF_Meta) == 0;
      ctrl = WinWindowFromID(GetHwnd(), EF_METATITLE);
      PMRASSERT(WinSetWindowText(ctrl, meta.title));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METAARTIST);
      PMRASSERT(WinSetWindowText(ctrl, meta.artist));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METAALBUM);
      PMRASSERT(WinSetWindowText(ctrl, meta.album));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METATRACK);
      PMRASSERT(WinSetWindowText(ctrl, meta.track < 0 ? "" : (sprintf(buffer, "%i", meta.track), buffer)));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METADATE);
      PMRASSERT(WinSetWindowText(ctrl, meta.year));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METAGENRE);
      PMRASSERT(WinSetWindowText(ctrl, meta.genre));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METARPGAINT);
      PMRASSERT(WinSetWindowText(ctrl, meta.track_gain != 0 ? "" : (sprintf(buffer, "%.1f", meta.track_gain), buffer)));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METARPPEAKT);
      PMRASSERT(WinSetWindowText(ctrl, meta.track_peak != 0 ? "" : (sprintf(buffer, "%.1f", meta.track_peak), buffer)));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METARPGAINA);
      PMRASSERT(WinSetWindowText(ctrl, meta.album_gain != 0 ? "" : (sprintf(buffer, "%.1f", meta.album_gain), buffer)));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      ctrl = WinWindowFromID(GetHwnd(), EF_METARPPEAKA);
      PMRASSERT(WinSetWindowText(ctrl, meta.album_peak != 0 ? "" : (sprintf(buffer, "%.1f", meta.album_peak), buffer)));
      PMRASSERT(WinEnableWindow(ctrl, enabled));
      return 0;
    }
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

InfoDialog::InfoDialog(Playable* p)
: ManagedDialogBase(DLG_INFO, NULLHANDLE),
  inst_index<InfoDialog, Playable>(p),
  Content(p),
  Page1(*this, CFG_TECHINFO),
  Page2(*this, CFG_METAINFO),
  ContentChangeDeleg(*this, &InfoDialog::ContentChangeEvent)
{ DEBUGLOG(("InfoDialog(%p)::InfoDialog(%p{%s})\n", this, p, p->GetURL().cdata()));
  StartDialog();
}


InfoDialog::Factory InfoDialog::Factory::Instance;

InfoDialog* InfoDialog::Factory::operator()(Playable* key)
{ return new InfoDialog(key);
}

void InfoDialog::StartDialog()
{ DEBUGLOG(("InfoDialog(%p)::StartDialog()\n", this));
  ManagedDialogBase::StartDialog(HWND_DESKTOP/*amp_player_window()*/);
  // setup notebook windows
  Page1.StartDialog();
  Page2.StartDialog();
  HWND book = WinWindowFromID(GetHwnd(), FID_CLIENT);
  PMRASSERT(nb_append_tab(book, Page1.GetHwnd(), "Tech. info", 0));
  PMRASSERT(nb_append_tab(book, Page2.GetHwnd(), "Meta info", 0));
}

MRESULT InfoDialog::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case WM_INITDLG:
    { // initial position
      SWP swp;
      PMXASSERT(WinQueryTaskSizePos(amp_player_hab(), 0, &swp), == 0);
      PMRASSERT(WinSetWindowPos(GetHwnd(), NULLHANDLE, swp.x,swp.y, 0,0, SWP_MOVE));
    }
    SetTitle(xstring::sprintf("PM123 Object info: %s", Content->GetURL().cdata()));
    do_warpsans(GetHwnd());
    // register for change event
    Content->InfoChange += ContentChangeDeleg;
    break;

   case WM_DESTROY:
    ContentChangeDeleg.detach();
    break;

   case WM_SYSCOMMAND:
    if( SHORT1FROMMP(mp1) == SC_CLOSE )
    { Destroy();
      return 0;
    }
    break;
  }
  return ManagedDialogBase::DlgProc(msg, mp1, mp2);
}

void InfoDialog::ContentChangeEvent(const Playable::change_args& args)
{ if (args.Flags & (Playable::IF_Format|Playable::IF_Tech|Playable::IF_Phys|Playable::IF_Rpl|Playable::IF_Other))
    WinPostMsg(Page1.GetHwnd(), UM_UPDATE, MPFROMLONG(args.Flags), 0);
  if (args.Flags & Playable::IF_Meta)
    WinPostMsg(Page2.GetHwnd(), UM_UPDATE, MPFROMLONG(args.Flags), 0);
}

/*void InfoDialog::SetVisible(bool show)
{
  if (show && !GetVisible())

}*/

int_ptr<InfoDialog> InfoDialog::GetByKey(Playable* obj)
{ // provide factory
  return inst_index<InfoDialog, Playable>::GetByKey(obj, Factory::Instance);
}

int InfoDialog::compareTo(const Playable*const& r) const
{ return Content->compareTo(*r);
}

/*void InfoDialog::GetKey(const Playable*& key) const
{ key = Content;
} */

