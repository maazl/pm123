/*
 * Copyright 2007-2008 M.Mueller
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


#define INCL_PM
#define INCL_BASE
#include "playable.h"
#include "plugman.h"
#include "pm123.h"
#include "properties.h"
#include <string.h>

#include <debuglog.h>

#ifdef DEBUG
#define RP_INITIAL_SIZE 5 // Errors come sooner...
#else
#define RP_INITIAL_SIZE 100
#endif


/****************************************************************************
*
*  class Playable
*
****************************************************************************/

Playable::Lock::~Lock()
{ if (P.Mtx.GetStatus() == 1)
    P.RaiseInfoChange();
  P.Mtx.Release();
}

const Playable::DecoderInfo Playable::DecoderInfo::InitialInfo;

void Playable::DecoderInfo::Init()
{ Format.size   = sizeof Format;
  TechInfo.size = sizeof TechInfo;
  MetaInfo.size = sizeof MetaInfo;
  PhysInfo.size = sizeof PhysInfo;
  RplInfo.size  = sizeof RplInfo;
  size          = sizeof(DECODER_INFO2);
  format        = &Format;
  tech          = &TechInfo;
  meta          = &MetaInfo;
  phys          = &PhysInfo;
  rpl           = &RplInfo;
}

void Playable::DecoderInfo::Reset()
{ memset(&MetaInfo, 0, sizeof MetaInfo);
  Init();
  Format  .samplerate = -1;
  Format  .channels   = -1;
  TechInfo.songlength = -1;
  TechInfo.bitrate    = -1;
  TechInfo.totalsize  = -1;
  memset(&TechInfo.info, 0, sizeof TechInfo.info);
  MetaInfo.track      = -1;
  PhysInfo.filesize   = -1;
  PhysInfo.num_items  = -1;
  RplInfo.total_items = -1;
  RplInfo.recursive   = 0;
  meta_write          = FALSE;
}

void Playable::DecoderInfo::operator=(const DECODER_INFO2& r)
{ Reset();
  // version stable copy
  if (r.format)
  { if (r.format->size < sizeof Format)
      memcpy(&Format, r.format, r.format->size);
     else
      Format = *r.format;
  }
  if (r.tech)
  { if (r.tech->size < sizeof TechInfo)
      memcpy(&TechInfo, r.tech, r.tech->size);
     else
      TechInfo = *r.tech;
  }
  if (r.meta)
  { if (r.meta->size < sizeof MetaInfo)
      memcpy(&MetaInfo, r.meta, r.meta->size);
     else
      MetaInfo = *r.meta;
  }
  if (r.phys)
  { if (r.phys->size < sizeof PhysInfo)
      memcpy(&PhysInfo, r.phys, r.phys->size);
     else
      PhysInfo = *r.phys;
  }
  if (r.rpl)
  { if (r.rpl->size < sizeof RplInfo)
      memcpy(&RplInfo, r.rpl, r.rpl->size);
     else
      RplInfo = *r.rpl;
  }
  memcpy((DECODER_INFO2*)this, &r, sizeof(DECODER_INFO2));
  Init();
}


Playable::Playable(const url123& url, const DECODER_INFO2* ca)
: URL(url),
  Stat(STA_Unknown),
  InfoValid(IF_None),
  InfoChangeFlags(IF_None),
  InfoRequest(IF_None),
  InfoRequestLow(IF_None),
  InService(1),
  LastAccess(clock())
{ DEBUGLOG(("Playable(%p)::Playable(%s, %p)\n", this, url.cdata(), ca));
  Info.Reset();
  if (ca)
  { DEBUGLOG(("Playable::Playable {%p,%p,%p,%p,%p}\n", ca->format, ca->tech, ca->meta, ca->phys, ca->rpl));
    if (ca->format)
    { *Info.format = *ca->format;
      InfoValid |= IF_Format;
    }
    if (ca->tech)
    { *Info.tech = *ca->tech;
      InfoValid |= IF_Tech;
    }
    if (ca->meta)
    { *Info.meta = *ca->meta;
      InfoValid |= IF_Meta;
    }
    if (ca->phys)
    { *Info.phys = *ca->phys;
      InfoValid |= IF_Phys;
    }
    if (ca->rpl)
    { *Info.rpl   = *ca->rpl;
      InfoValid |= IF_Rpl;
    }
  } 
  Decoder[0] = 0;
}

Playable::~Playable()
{ DEBUGLOG(("Playable(%p{%s})::~Playable()\n", this, URL.cdata()));
}

/* Returns true if the specified file is a playlist file. */
bool Playable::IsPlaylist(const url123& URL)
{ const xstring& ext = URL.getExtension();
  DEBUGLOG(("Playable::IsPlaylist(%s) - %s\n", URL.cdata(), ext.cdata()));
  // TODO: this is a very bad criterion
  return ext.compareToI(".lst", 4) == 0 ||
         ext.compareToI(".mpl", 4) == 0 ||
         ext.compareToI(".pls", 4) == 0 ||
         ext.compareToI(".m3u", 4) == 0;
}

Playable::Flags Playable::GetFlags() const
{ return None;
}

void Playable::SetInUse(bool used)
{ DEBUGLOG(("Playable(%p{%s})::SetInUse(%u)\n", this, URL.cdata(), used));
  Lock lock(*this);
  if (Stat <= STA_Invalid)
    return; // can't help
  UpdateStatus(used ? STA_Used : STA_Normal);
}

xstring Playable::GetDisplayName() const
{ //DEBUGLOG(("Playable(%p{%s})::GetDisplayName()\n", this, URL.cdata()));
  const META_INFO& meta = *GetInfo().meta;
  if (meta.title[0])
    return meta.title;
  else
    return URL.getShortName();
}

void Playable::UpdateInfo(const FORMAT_INFO2* info)
{ DEBUGLOG(("Playable::UpdateInfo(FORMAT_INFO2* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.format;
  InfoValid |= IF_Format;
  if (~memcmpcpy(Info.format, info, sizeof *Info.format))
    InfoChangeFlags |= IF_Format;
  DEBUGLOG(("Playable::UpdateInfo: %x %u {,%u,%u}\n", InfoValid, InfoChangeFlags, Info.format->samplerate, Info.format->channels));
}
void Playable::UpdateInfo(const TECH_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(TECH_INFO* %p) - %p\n", info, Info.tech));
  if (!info)
    info = DecoderInfo::InitialInfo.tech;
  InfoValid |= IF_Tech;
  if (~memcmpcpy(Info.tech, info, sizeof *Info.tech))
    InfoChangeFlags |= IF_Tech;
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const META_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(META_INFO* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.meta;
  InfoValid |= IF_Meta;
  if (~memcmpcpy(Info.meta, info, sizeof *Info.meta))
    InfoChangeFlags |= IF_Meta;
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const PHYS_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(PHYS_INFO* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.phys;
  InfoValid |= IF_Phys;
  if (~memcmpcpy(Info.phys, info, sizeof *Info.phys))
    InfoChangeFlags |= IF_Phys;
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const RPL_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(RPL_INFO* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.rpl;
  InfoValid |= IF_Rpl;
  if (~memcmpcpy(Info.rpl, info, sizeof *Info.rpl))
    InfoChangeFlags |= IF_Rpl;
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const DECODER_INFO2* info, InfoFlags what)
{ DEBUGLOG(("Playable::UpdateInfo(DECODER_INFO2* %p, %x)\n", info, what));
  if (!info)
    info = &DecoderInfo::InitialInfo;
  InfoValid |= IF_Other;
  if (Info.meta_write != info->meta_write)
  { Info.meta_write = info->meta_write;
    InfoChangeFlags |= IF_Other;
  }
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
  if (what & IF_Format)
    UpdateInfo(info->format);
  if (what & IF_Tech)
    UpdateInfo(info->tech);
  if (what & IF_Meta)
    UpdateInfo(info->meta);
  if (what & IF_Phys)
    UpdateInfo(info->phys);
  if (what & IF_Rpl)
    UpdateInfo(info->rpl);
}

void Playable::UpdateStatus(PlayableStatus stat)
{ DEBUGLOG(("Playable::UpdateStatus(%u) - %u\n", stat, Stat));
  InfoValid |= IF_Status;
  if (Stat != stat)
  { Stat = stat;
    InfoChangeFlags |= IF_Status;
  }
}

void Playable::RaiseInfoChange()
{ DEBUGLOG(("Playable(%p)::RaiseInfoChange()\n", this));
  if (InfoChangeFlags)
    InfoChange(change_args(*this, InfoChangeFlags));
  InfoChangeFlags = IF_None;
}

void Playable::SetMetaInfo(const META_INFO* meta)
{ Lock lock(*this);
  UpdateInfo(meta);
}

void Playable::SetTechInfo(const TECH_INFO* tech)
{ Lock lock(*this);
  UpdateInfo(tech);
}

Playable::InfoFlags Playable::LoadInfo(InfoFlags what)
{ DEBUGLOG(("Playable(%p)::LoadInfo(%x) - %x, %x\n", this, what, InfoRequest, InfoRequestLow));
  InfoValid |= what;
  // We always reset the flags of both priorities.
  InterlockedAnd(InfoRequest, ~what);
  InterlockedAnd(InfoRequestLow, ~what);
  // Well we might remove an outstanding request in the worker queue here,
  // but it is simply faster to wait for the request to arrive and ignore it then.
  return what;
}

void Playable::EnsureInfo(InfoFlags what)
{ InfoFlags i = CheckInfo(what);
  DEBUGLOG(("Playable(%p{%s})::EnsureInfo(%x) - %x, %x\n", this, GetURL().getShortName().cdata(), what, InfoValid, i));
  if (i)
  { { Lock lock(*this);
      i = CheckInfo(what);
      // We release the lock before we load the desired information
      // to avoid deadlocks with recursive playlists.
      // There is a small chance that the Informatione is requested by another thread
      // immediately after doing that. But this price we have to pay. In this case
      // the information is loaded twice. 
    }
    if (i) // double check
      LoadInfo(i);
  }
}

void Playable::LoadInfoAsync(InfoFlags what, bool lowpri)
{ DEBUGLOG(("Playable(%p{%s})::LoadInfoAsync(%x, %u) - %x %x %x\n", this, GetURL().getShortName().cdata(), what, lowpri, InfoValid, InfoRequest, InfoRequestLow));
  if (what == 0)
    return;
  // schedule request
  volatile unsigned& rq = lowpri ? InfoRequestLow : InfoRequest;
  InfoFlags oldrq = (InfoFlags)rq; // This read is non-atomic. But at the worst case an empty request is scheduled to the worker thread.
  InterlockedOr(rq, what);
  // Only write the Queue if there was no request before.
  if (!oldrq && !WTermRq)
    WQueue.Write(this, lowpri);
}

Playable::InfoFlags Playable::EnsureInfoAsync(InfoFlags what, bool lowpri)
{ InfoFlags avail = what & InfoValid;
  if ((what &= ~avail) != 0)
    // schedule request
    LoadInfoAsync(what, lowpri);
  return avail;
}

/* Worker Queue */
void Playable::worker_queue::CommitRead(qentry* qp)
{ DEBUGLOG(("Playable::worker_queue::CommitRead(%p) - %p %p %p\n", qp, Head, HPTail, Tail));
  Mutex::Lock lock(Mtx);
  if (HPTail == qp)
    HPTail = qp->Next;
  queue<QEntry>::CommitRead(qp);
  /*#ifdef DEBUG
  DumpQ();
  #endif*/
}

void Playable::worker_queue::Write(const QEntry& data, bool lowpri)
{ DEBUGLOG(("Playable::worker_queue::Write(%p, %u) - %p %p %p\n", data.get(), lowpri, Head, HPTail, Tail));
  // if low priority or high priority with insert at the end
  if (lowpri)
    queue<QEntry>::Write(data);
  else
  { // high priority insert in the middle
    Mutex::Lock lock(Mtx);
    EntryBase* newitem = new qentry(data);
    queue_base::Write(newitem, HPTail);
    HPTail = newitem;
  }
  /*#ifdef DEBUG
  DumpQ();
  #endif*/
}

#ifdef DEBUG
void Playable::worker_queue::DumpQ() const
{ DEBUGLOG(("Playable::worker_queue::DumpQ - %p, %p, %p\n", Head, HPTail, Tail));
  qentry* cur = (qentry*)Head;
  while (cur)
  { DEBUGLOG(("Playable::worker_queue::DumpQ %p{%p}\n", cur, cur->Data.get()));
    cur = (qentry*)cur->Next;
  }
}
#endif

Playable::worker_queue Playable::WQueue;
int*    Playable::WTids       = NULL;
size_t  Playable::WNumWorkers = 0;
bool    Playable::WTermRq     = false;
clock_t Playable::LastCleanup = 0;

static void PlayableWorker(void*)
{ for (;;)
  { DEBUGLOG(("PlayableWorker() looking for work\n"));
    queue<Playable::QEntry>::qentry* qp = Playable::WQueue.RequestRead();
    DEBUGLOG(("PlayableWorker received message %p %p\n", qp, qp->Data.get()));

    if (Playable::WTermRq || !qp->Data) // stop
    { // leave the deadly pill for the next thread
      Playable::WQueue.RollbackRead(qp);
      break;
    }
    
    // Do the work
    DEBUGLOG(("PlayableWorker handle %x %x\n", qp->Data->InfoRequest, qp->Data->InfoRequestLow));
    // Do not handle the same item by two threads.
    if (!InterlockedDec(qp->Data->InService))
    { // Handle low priority requests correctly if no other requests are on the way.
      if (qp->Data->InfoRequest == 0)
        InterlockedOr(qp->Data->InfoRequest, qp->Data->InfoRequestLow);
      while (qp->Data->InfoRequest)
        qp->Data->LoadInfo((Playable::InfoFlags)qp->Data->InfoRequest);
      // Free instance
      if (InterlockedXch(qp->Data->InService, 1))
      { // If the value wasn't zero, at least one queue item was skipped.
        // In this case we have to reschedule outstanding low priority requests.
        // There is a small chance that such a request is already scheduled.
        // But this will only be a no-op to the worker.
        if (qp->Data->InfoRequestLow)
          Playable::WQueue.Write(qp->Data, true);
      }
    }
    // finished
    Playable::WQueue.CommitRead(qp);
  }
}

void Playable::Init()
{ DEBUGLOG(("Playable::Init()\n"));
  // start the worker threads
  ASSERT(WTids == NULL);
  WTermRq = false;
  WNumWorkers = cfg.num_workers; // sample this atomically
  WTids = new int[WNumWorkers];
  for (int* tidp = WTids + WNumWorkers; tidp != WTids; )
  { *--tidp = _beginthread(&PlayableWorker, NULL, 65536, NULL);
    ASSERT(*tidp != -1);
  }
}

void Playable::Uninit()
{ DEBUGLOG(("Playable::Uninit()\n"));
  WTermRq = true;
  WQueue.Purge();
  WQueue.Write(NULL, false); // deadly pill
  // syncronize worker threads
  for (int* tidp = WTids + WNumWorkers; tidp != WTids; )
    wait_thread_pm(amp_player_hab(), *--tidp, 60000);
  // Now remove the deadly pill from the queue
  WQueue.Purge();
  // destroy all remaining instances immediately
  LastCleanup = clock();
  Cleanup();
  DEBUGLOG(("Playable::Uninit - complete - %u\n", RPInst.size()));
}

/* URL repository */
sorted_vector<Playable, const char*> Playable::RPInst(RP_INITIAL_SIZE);
Mutex Playable::RPMutex;

int Playable::compareTo(const char*const& str) const
{ DEBUGLOG(("Playable(%p{%s})::compareTo(%s)\n", this, URL.cdata(), str));
  return stricmp(URL, str);
}

#ifdef DEBUG
void Playable::RPDebugDump()
{ for (Playable*const* ppp = RPInst.begin(); ppp != RPInst.end(); ++ppp)
    DEBUGLOG(("Playable::RPDump: %p{%s}\n", *ppp, (*ppp)->GetURL().cdata()));
}
#endif

int_ptr<Playable> Playable::FindByURL(const char* url)
{ DEBUGLOG(("Playable::FindByURL(%s)\n", url));
  Mutex::Lock lock(RPMutex);
  return RPInst.find(url);
}

int_ptr<Playable> Playable::GetByURL(const url123& URL, const DECODER_INFO2* ca)
{ DEBUGLOG(("Playable::GetByURL(%s, %p)\n", URL.cdata(), ca));
  Playable* pp;
  // Repository lookup
  { Mutex::Lock lock(RPMutex);
    #ifdef DEBUG
    //RPDebugDump();
    #endif
    Playable*& ppn = RPInst.get(URL);
    if (ppn == NULL)
    { // no match => factory
      int_ptr<Playable> ppf;
      if (URL.isScheme("file:") && URL.getObjectName().length() == 0)
        ppf = new PlayFolder(URL, ca);
       else if (IsPlaylist(URL))
        ppf = new Playlist(URL, ca);
       else // Song
        ppf = new Song(URL, ca);
      DEBUGLOG(("Playable::GetByURL: factory &%p{%p}\n", &ppn, ppn));
      ppn = ppf.toCptr(); // keep reference count alive
      // The opposite function is at Cleanup().
      return ppn;
    }
    // else match
    pp = ppn;
    pp->LastAccess = clock();
  }
  
  if (ca)
  { DEBUGLOG(("Playable::GetByURL: merge {%p, %p, %p, %p, %p} %x\n",
      ca->format, ca->tech, ca->meta, ca->phys, ca->rpl, pp->InfoValid));
    // Double check to avoid unneccessary mutex delays.
    if ( (ca->format && !(pp->InfoValid & IF_Format))
      || (ca->tech   && !(pp->InfoValid & IF_Tech  ))
      || (ca->meta   && !(pp->InfoValid & IF_Meta  ))
      || (ca->phys   && !(pp->InfoValid & IF_Phys  ))
      || (ca->rpl    && !(pp->InfoValid & IF_Rpl   )) )
    { // Merge meta info
      Lock lock(*pp);
      if (ca->format && !(pp->InfoValid & IF_Format))
        pp->UpdateInfo(ca->format);
      if (ca->tech   && !(pp->InfoValid & IF_Tech  ))
        pp->UpdateInfo(ca->tech);
      if (ca->meta   && !(pp->InfoValid & IF_Meta  ))
        pp->UpdateInfo(ca->meta);
      if (ca->phys   && !(pp->InfoValid & IF_Phys  ))
        pp->UpdateInfo(ca->phys);
      if (ca->rpl    && !(pp->InfoValid & IF_Rpl   ))
        pp->UpdateInfo(ca->rpl);
    }
  }
  return pp;
}

void Playable::DetachObjects(const vector<Playable>& list)
{ DEBUGLOG(("Playable::DetachObjects({%u,})\n", list.size()));
  int_ptr<Playable> killer;
  for (Playable*const* ppp = list.begin(); ppp != list.end(); ++ppp)
  { DEBUGLOG(("Playable::DetachObjects - detaching %p{%s}\n", *ppp, (*ppp)->GetURL().cdata()));
    killer.fromCptr(*ppp);
  }
}

void Playable::Cleanup()
{ DEBUGLOG(("Playable::Cleanup() - %u\n", LastCleanup));
  // Keep destructor calls out of the mutex
  vector<Playable> todelete(32);
  // serach for unused items
  { Mutex::Lock lock(RPMutex);
    for (Playable*const* ppp = RPInst.end(); --ppp != RPInst.begin(); )
    { if ((*ppp)->RefCountIsUnique() && (long)((*ppp)->LastAccess - LastCleanup) <= 0)
        todelete.append() = RPInst.erase(ppp);
    }
  }
  // Destroy items
  DetachObjects(todelete);
  // prepare next run
  LastCleanup = clock();
}

void Playable::Clear()
{ DEBUGLOG(("Playable::Clear()\n"));
  // Keep destructor calls out of the mutex
  static sorted_vector<Playable, const char*> rp;
  { // detach all items
    Mutex::Lock lock(RPMutex);
    rp.swap(RPInst);
  }
  // Detach items
  DetachObjects(rp);
}


/****************************************************************************
*
*  class PlayableSet
*
****************************************************************************/

const PlayableSet PlayableSet::Empty;

PlayableSet::PlayableSet()
: sorted_vector<Playable, Playable>(8)
{}

int PlayableSet::compareTo(const PlayableSet& r) const
{ DEBUGLOG(("PlayableSet(%p{%u,...})::compareTo(&%p{%u,...})\n", this, size(), &r, r.size()));
  Playable*const* ppp1 = begin();
  Playable*const* ppp2 = r.begin();
  for (;;)
  { // termination condition
    if (ppp2 == r.end())
      return ppp1 != end();
    else if (ppp1 == end())
      return -1;
    // compare content
    int ret = (*ppp1)->compareTo(**ppp2);
    DEBUGLOG2(("PlayableSet::compareTo %p <=> %p = %i\n", **ppp1, **ppp2, ret));
    if (ret)
      return ret;
    ++ppp1;
    ++ppp2;
  }
}

bool PlayableSet::isSubsetOf(const PlayableSet& r) const
{ DEBUGLOG(("PlayableSet(%p{%u,...})::isSubsetOf(&%p{%u,...})\n", this, size(), &r, r.size()));
  if (size() > r.size())
    return false; // since PlayableSet is a unique container this condition is sufficient
  if (size() == 0)
    return true; // an empty set is always included
  Playable*const* ppp1 = begin();
  Playable*const* ppp2 = r.begin();
  for (;;)
  { // compare content
    int ret = (*ppp1)->compareTo(**ppp2);
    DEBUGLOG2(("PlayableSet::isSubsetOf %p <=> %p = %i\n", **ppp1, **ppp2, ret));
    if (ret > 0)
      return false; // no match for **ppp1
    ++ppp2;
    if (ppp2 == r.end())
      return false; // no match for **ppp1 because no more elements in r
    if (ret < 0)
      continue; // only increment ppp2
    ++ppp1;
    if (ppp1 == end())
      return true; // all elements found
  }
}

#ifdef DEBUG
xstring PlayableSet::DebugDump() const
{ xstring r = xstring::empty;
  for (Playable*const* ppp = begin(); ppp != end(); ++ppp)
    if (r.length())
      r = xstring::sprintf("%s, %p", r.cdata(), *ppp);
    else
      r = xstring::sprintf("%p", *ppp);
  return r;
}
#endif


/****************************************************************************
*
*  class Song
*
****************************************************************************/

Song::Song(const url123& URL, const DECODER_INFO2* ca)
: Playable(URL, ca)
{ DEBUGLOG(("Song(%p)::Song(%s, %p)\n", this, URL.cdata(), ca));
  // Always overwrite RplInfo
  static const RPL_INFO defrpl = { sizeof(RPL_INFO), 1, 0 };
  *Info.rpl = defrpl;
  InfoValid |= IF_Rpl;
}

Playable::InfoFlags Song::LoadInfo(InfoFlags what)
{ DEBUGLOG(("Song(%p)::LoadInfo(%x) - %s\n", this, what, Decoder));

  // get information
  sco_ptr<DecoderInfo> info(new DecoderInfo()); // a bit much for the stack
  INFOTYPE what2 = (INFOTYPE)((int)what & INFO_ALL); // inclompatible types
  Lock lock(*this);
  int rc = dec_fileinfo(GetURL(), &what2, info.get(), Decoder, sizeof Decoder);
  what = (InfoFlags)what2 | Playable::IF_Other|Playable::IF_Status; // That's always inclusive when we call dec_fileinfo

  PlayableStatus stat;
  DEBUGLOG(("Song::LoadInfo - rc = %i\n", rc));
  // update information
  if (rc != 0)
  { stat = STA_Invalid;
    info = NULL;
  } else
    stat = Stat == STA_Used ? STA_Used : STA_Normal;
  UpdateStatus(stat);
  UpdateInfo(info.get(), what); // Reset fields
  
  return Playable::LoadInfo(what);
}

