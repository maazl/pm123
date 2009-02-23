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
#include "pm123.h"
#include "properties.h"
#include <strutils.h>
#include <utilfct.h>
#include <string.h>
#include <stdio.h>

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
    P.OnReleaseMutex();
  P.Mtx.Release();
}

Playable::WaitInfo::WaitInfo(Playable& inst, InfoFlags filter)
: Filter(filter),
  Deleg(inst.InfoChange, *this, &Playable::WaitInfo::InfoChangeEvent)
{ DEBUGLOG(("Playable::WaitInfo(%p)::WaitInfo(&%p, %x)\n", this, &inst, filter));
}

Playable::WaitInfo::~WaitInfo()
{ DEBUGLOG(("Playable::WaitInfo(%p)::~WaitInfo()\n", this));
}

void Playable::WaitInfo::InfoChangeEvent(const change_args& args)
{ DEBUGLOG(("Playable::WaitInfo(%p)::InfoChangeEvent({&%p, %x, %x}) - %x\n",
    this, &args.Instance, args.Changed, args.Loaded, Filter));
  ASSERT(args.Loaded != IF_None); // Owner died! - This must not happen.
  Filter &= ~args.Loaded;
  if (!Filter)
  { Deleg.detach();
    EventSem.Set();
  }
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
  InUse(false),
  InfoChanged(IF_None),
  InfoLoaded(IF_None),
  InfoMask(IF_None),
  InfoValid(IF_Usage),
  InfoConfirmed(IF_Usage),
  InfoRequest(IF_None),
  InfoRequestLow(IF_None),
  InfoInService(IF_None),
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
  // Notify about dieing
  InfoChange(change_args(*this, IF_None, IF_None));
}

/* Returns true if the specified file is a playlist file. */
bool Playable::IsPlaylist(const url123& URL)
{ const xstring& ext = URL.getExtension();
  DEBUGLOG(("Playable::IsPlaylist(%s) - %s\n", URL.cdata(), ext.cdata()));
  // TODO: this is a very bad criterion
  return ext.compareToI(".lst", 4) == 0 ||
         ext.compareToI(".mpl", 4) == 0 ||
         ext.compareToI(".pls", 4) == 0 ||
         ext.compareToI(".m3u", 4) == 0 ||
         ext.compareToI(".m3u8", 5) == 0;
}

Playable::Flags Playable::GetFlags() const
{ return None;
}

void Playable::SetInUse(bool used)
{ DEBUGLOG(("Playable(%p{%s})::SetInUse(%u)\n", this, URL.cdata(), used));
  Lock lock(*this);
  InfoFlags what = BeginUpdate(IF_Usage);
  UpdateInUse(used);
  EndUpdate(what);
}

xstring Playable::GetDisplayName() const
{ //DEBUGLOG(("Playable(%p{%s})::GetDisplayName()\n", this, URL.cdata()));
  const META_INFO& meta = *GetInfo().meta;
  if (meta.title[0])
    return meta.title;
  else
    return URL.getShortName();
}

Playable::InfoFlags Playable::BeginUpdate(InfoFlags what)
{ DEBUGLOG(("Playable(%p)::BeginUpdate(%x) - %x\n", this, what, InfoInService));
  CritSect cs;
  what &= ~(InfoFlags)InfoInService;
  InfoInService |= what;
  return what;
}

void Playable::EndUpdate(InfoFlags what)
{ DEBUGLOG(("Playable(%p)::EndUpdate(%x) - IS=%x L=%x V=%x R=%x Rl=%x\n", this,
    what, InfoInService, InfoLoaded, InfoValid, InfoRequest, InfoRequestLow));
  ASSERT((InfoInService & what) == what);
  InfoFlags mask = ~what;
  Lock lock(*this);
  // We always reset the flags of both priorities.
  InterlockedAnd(InfoRequestLow, mask);
  InterlockedAnd(InfoRequest,    mask);
  // Well, we might remove an outstanding request in the worker queue here,
  // but it is simply faster to wait for the request to arrive and ignore it then.
  InterlockedAnd(InfoInService,  mask);
  // Prepare InfoChange event
  InfoMask |= what;
}

void Playable::ValidateInfo(InfoFlags what, bool changed, bool confirmed)
{ DEBUGLOG(("Playable::ValidateInfo(%x, %u, %u)\n", what, changed, confirmed));
  InterlockedOr((volatile unsigned&)InfoValid, what);
  if (confirmed)
    InterlockedOr((volatile unsigned&)InfoConfirmed, what);
  else
    InterlockedAnd((volatile unsigned&)InfoConfirmed, ~what);
  InfoLoaded |= what;
  InfoChanged |= what * changed;
}

void Playable::InvalidateInfo(InfoFlags what, bool reload)
{ DEBUGLOG(("Playable::InvalidateInfo(%x, %u)\n", what, reload));
  InterlockedAnd((volatile unsigned&)InfoConfirmed, ~what);
  if (reload && (InfoValid & what))
    LoadInfoAsync(InfoValid & what, true);
}

void Playable::UpdateFormat(const FORMAT_INFO2* info, bool confirmed)
{ DEBUGLOG(("Playable::UpdateFormat(%p, %u)\n", info, confirmed));
  ASSERT(RefCountIsUnmanaged() || ((InfoInService & IF_Format) && Mtx.GetStatus() > 0));
  if (!info)
    info = DecoderInfo::InitialInfo.format;
  ValidateInfo(IF_Format, ~memcmpcpy(Info.format, info, sizeof *Info.format) != 0, confirmed);
}

void Playable::UpdateTech(const TECH_INFO* info, bool confirmed)
{ DEBUGLOG(("Playable::UpdateTech(%p, %u) - %p\n", info, confirmed, Info.tech));
  ASSERT(RefCountIsUnmanaged() || ((InfoInService & IF_Tech) && Mtx.GetStatus() > 0));
  if (!info)
    info = DecoderInfo::InitialInfo.tech;
  ValidateInfo(IF_Tech, ~memcmpcpy(Info.tech, info, sizeof *Info.tech) != 0, confirmed);
}

void Playable::UpdateMeta(const META_INFO* info, bool confirmed)
{ DEBUGLOG(("Playable::UpdateMeta(%p, %u)\n", info, confirmed));
  ASSERT(RefCountIsUnmanaged() || ((InfoInService & IF_Meta) && Mtx.GetStatus() > 0));
  if (!info)
    info = DecoderInfo::InitialInfo.meta;
  ValidateInfo(IF_Meta, ~memcmpcpy(Info.meta, info, sizeof *Info.meta) != 0, confirmed);
}

void Playable::UpdatePhys(const PHYS_INFO* info, bool confirmed)
{ DEBUGLOG(("Playable::UpdatePhys(%p, %u)\n", info, confirmed));
  ASSERT(RefCountIsUnmanaged() || ((InfoInService & IF_Phys) && Mtx.GetStatus() > 0));
  if (!info)
    info = DecoderInfo::InitialInfo.phys;
  DEBUGLOG(("Playable::UpdatePhys {%f,%i} -> {%f,%i}\n", Info.phys->filesize, Info.phys->num_items, info->filesize, info->num_items));
  ValidateInfo(IF_Phys, ~memcmpcpy(Info.phys, info, sizeof *Info.phys) != 0, confirmed);
}

void Playable::UpdateRpl(const RPL_INFO* info, bool confirmed)
{ DEBUGLOG(("Playable::UpdateRpl(%p, %u)\n", info, confirmed));
  ASSERT(RefCountIsUnmanaged() || ((InfoInService & IF_Rpl) && Mtx.GetStatus() > 0));
  if (!info)
    info = DecoderInfo::InitialInfo.rpl;
  ValidateInfo(IF_Rpl, ~memcmpcpy(Info.rpl, info, sizeof *Info.rpl) != 0, confirmed);
}

void Playable::UpdateOther(Status stat, bool meta_write, const char* decoder, bool confirmed)
{ DEBUGLOG(("Playable::UpdateOther(%u, %u, %s, %u)\n", stat, meta_write, decoder, confirmed));
  ASSERT(RefCountIsUnmanaged() || ((InfoInService & IF_Other) && Mtx.GetStatus() > 0));
  bool changed = Stat != stat || Info.meta_write != meta_write || strcmp(decoder, Decoder) != 0;
  if (changed)
  { Stat = stat;
    changed = true;
    Info.meta_write = meta_write;
    strlcpy(Decoder, decoder, sizeof Decoder);
  }
  ValidateInfo(IF_Other, changed, confirmed);
}

void Playable::UpdateInfo(Status stat, const DECODER_INFO2* info, const char* decoder, InfoFlags what, bool confirmed)
{ DEBUGLOG(("Playable::UpdateInfo(DECODER_INFO2* %p, %s, %x, %u)\n", info, decoder, what, confirmed));
  if (!info)
    info = &DecoderInfo::InitialInfo;
  if (what & IF_Other)
    UpdateOther(stat, info->meta_write, decoder, confirmed);
  if (what & IF_Format)
    UpdateFormat(info->format, confirmed);
  if (what & IF_Tech)
    UpdateTech(info->tech, confirmed);
  if (what & IF_Meta)
    UpdateMeta(info->meta, confirmed);
  if (what & IF_Phys)
    UpdatePhys(info->phys, confirmed);
  if (what & IF_Rpl)
    UpdateRpl(info->rpl, confirmed);
}

void Playable::UpdateInUse(bool inuse)
{ DEBUGLOG(("Playable::UpdateInUse(%u) - %u\n", inuse, InUse));
  ASSERT(Mtx.GetStatus() > 0);
  bool changed = InUse != inuse;
  InUse = inuse;
  ValidateInfo(IF_Usage, changed, true);
}

void Playable::OnReleaseMutex()
{ DEBUGLOG(("Playable(%p)::OnReleaseMutex() - %x/%x %x\n", this, InfoChanged, InfoLoaded, InfoMask));
  // Raise InfoChange event
  if (InfoLoaded & InfoMask)
  { InfoChange(change_args(*this, InfoChanged & InfoMask, InfoLoaded & InfoMask));
    InfoChanged &= ~InfoMask;
    InfoLoaded  &= ~InfoMask;
  }
  InfoMask = IF_None;
}

void Playable::LoadInfo(InfoFlags what)
{ InfoFlags now = BeginUpdate(what);
  what &= ~now;
  now = DoLoadInfo(now);
  EndUpdate(now);
  // Is there still some information on the way by another Thread?
  if ((what &= (InfoFlags)InfoInService) != 0)
  { sco_ptr<WaitInfo> waitinfo;
    { Lock lock(*this);
      if ((what &= (InfoFlags)InfoInService) != 0); // Filter again, because some information may just have completed.
        waitinfo = new WaitInfo(*this, what);
    }
    // Wait for information currently in service.
    if (waitinfo != NULL)
      waitinfo->Wait();
  }
}

void Playable::EnsureInfo(InfoFlags what, bool confirmed)
{ DEBUGLOG(("Playable(%p{%s})::EnsureInfo(%x, %u) - %x, %x\n",
    this, GetURL().getShortName().cdata(), what, confirmed, InfoValid, InfoConfirmed));
  InfoFlags curinfo = confirmed ? InfoConfirmed : InfoValid;
  InfoFlags now = what & ~curinfo;
  if (!now)
    return;
  InfoFlags what2 = what & ~curinfo;
  now = BeginUpdate(what);
  what2 &= ~now;
  now = DoLoadInfo(now);
  EndUpdate(now);
  // Is there still some information on the way by another Thread?
  if ((what2 &= (InfoFlags)InfoInService) != 0)
  { sco_ptr<WaitInfo> waitinfo;
    { Lock lock(*this);
      if ((what2 &= (InfoFlags)InfoInService) != 0); // Filter again, because some information may just have completed.
        waitinfo = new WaitInfo(*this, what2);
    }
    // Wait for information currently in service.
    if (waitinfo != NULL)
      waitinfo->Wait();
  }
  ASSERT((InfoValid & what) == what);
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

Playable::InfoFlags Playable::EnsureInfoAsync(InfoFlags what, bool lowpri, bool confirmed)
{ InfoFlags avail = what & (confirmed ? InfoConfirmed : InfoValid);
  if ((what &= ~avail) != 0)
    // schedule request
    LoadInfoAsync(what, lowpri);
  return avail;
}

void Playable::SetMetaInfo(const META_INFO* meta)
{ Lock lock(*this);
  InfoFlags what = BeginUpdate(IF_Meta);
  if (what & IF_Meta)
    UpdateMeta(meta);
  // TODO: We cannot simply ignore update requests in case of concurrency issues.
  EndUpdate(what);
}

void Playable::SetTechInfo(const TECH_INFO* tech)
{ Lock lock(*this);
  InfoFlags what = BeginUpdate(IF_Tech);
  if (what & IF_Tech)
    UpdateTech(tech);
  // TODO: We cannot simply ignore update requests in case of concurrency issues.
  EndUpdate(what);
}


/* Worker Queue */
/*qentry* Playable::worker_queue::RequestRead(bool lowpri)
{ DEBUGLOG(("Playable::worker_queue::RequestRead(%u)\n", lowpri));
  //#ifdef DEBUG
  //DumpQ();
  //#endif
  return lowpri
    ? queue<QEntry>::RequestRead()
    : (qentry*)queue_base::RequestRead(HPEvEmpty, HPTail);
}*/

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
  // if low priority
  if (lowpri)
    queue<QEntry>::Write(data);
  else
  { // high priority insert in the middle
    Mutex::Lock lock(Mtx);
    EntryBase* newitem = new qentry(data);
    queue_base::Write(newitem, HPTail);
    HPTail = newitem;
    HPEvEmpty.Set();
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
int*    Playable::WTids          = NULL;
size_t  Playable::WNumWorkers    = 0;
size_t  Playable::WNumDlgWorkers = 0;
bool    Playable::WTermRq        = false;
clock_t Playable::LastCleanup    = 0;

void Playable::PlayableWorker(bool lowpri)
{ // Do the work! 
  for (;;)
  { DEBUGLOG(("PlayableWorker(%u) looking for work\n", lowpri));
    queue<Playable::QEntry>::qentry* qp = Playable::WQueue.RequestRead(lowpri);
    DEBUGLOG(("PlayableWorker received message %p %p\n", qp, qp->Data.get()));

    if (!qp->Data) // stop
    { // leave the deadly pill for the next thread
      WQueue.RollbackRead(qp);
      break;
    }
    
    // Do the work
    Playable* pp = qp->Data;
    DEBUGLOG(("PlayableWorker handle %x %x, %u\n", pp->InfoRequest, pp->InfoRequestLow, pp->RefCountIsUnique()));
    // Skip items that are no longer needed.
    // If their refcount is 1 we are the only users.
    if (pp->RefCountIsUnique())
    { CritSect cs;
      if (pp->RefCountIsUnique()) // double check
      { pp->InfoRequest    = 0;
        pp->InfoRequestLow = 0;
        goto skip;
      }
    }

    { // Handle low priority requests correctly if no other requests are on the way.
      volatile unsigned& request = 
        lowpri && pp->InfoRequest == 0
        ? (ORASSERT(DosSetPriority(PRTYS_THREAD, PRTYC_IDLETIME, 20, 0)), pp->InfoRequestLow)
        : pp->InfoRequest;

      // Handle the Request !!!
      while ((request & ~pp->InfoInService) && !WTermRq)
        pp->EndUpdate(
          pp->DoLoadInfo(
            pp->BeginUpdate((InfoFlags)request)));

      if (lowpri)
        ORASSERT(DosSetPriority(PRTYS_THREAD, PRTYC_REGULAR, 0, 0));
    }
   skip:
    // finished
    WQueue.CommitRead(qp);
  }
}

void TFNENTRY PlayableWorkerStub(void* arg)
{ Playable::PlayableWorker(!!arg);
}

void Playable::Init()
{ DEBUGLOG(("Playable::Init()\n"));
  // start the worker threads
  ASSERT(WTids == NULL);
  WTermRq = false;
  WNumWorkers = cfg.num_workers;     // sample this atomically
  WNumDlgWorkers = cfg.num_dlg_workers; // sample this atomically
  WTids = new int[WNumWorkers+WNumDlgWorkers];
  for (int* tidp = WTids + WNumWorkers+WNumDlgWorkers; tidp-- != WTids; )
  { *tidp = _beginthread(&PlayableWorkerStub, NULL, 65536, (void*)(tidp >= WTids + WNumDlgWorkers));
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
{ DEBUGLOG2(("Playable(%p{%s})::compareTo(%s)\n", this, URL.cdata(), str));
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
  { InfoFlags what = (bool)(ca->format != NULL) * IF_Format
                   | (bool)(ca->tech   != NULL) * IF_Tech
                   | (bool)(ca->meta   != NULL) * IF_Meta
                   | (bool)(ca->phys   != NULL) * IF_Phys
                   | (bool)(ca->rpl    != NULL) * IF_Rpl;
    DEBUGLOG(("Playable::GetByURL: merge {%p, %p, %p, %p, %p} %x %x\n",
      ca->format, ca->tech, ca->meta, ca->phys, ca->rpl, pp->InfoValid, what));
    what &= ~pp->InfoValid;               
    // Double check to avoid unneccessary mutex delays.
    if ( what )
    { // Merge meta info
      Lock lock(*pp);
      what = pp->BeginUpdate(what & ~pp->InfoValid);
      if (what & IF_Format)
        pp->UpdateFormat(ca->format, false);
      if (what & IF_Tech)
        pp->UpdateTech(ca->tech, false);
      if (what & IF_Meta)
        pp->UpdateMeta(ca->meta, false);
      if (what & IF_Phys)
        pp->UpdatePhys(ca->phys, false);
      if (what & IF_Rpl)
        pp->UpdateRpl(ca->rpl, false);
      pp->EndUpdate(what);
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

class EmptyPlayableSet : public PlayableSetBase
{ virtual size_t           size() const;
  virtual Playable*        operator[](size_t where) const;
  virtual bool             contains(const Playable& key) const;
};

size_t EmptyPlayableSet::size() const
{ return 0;
}

Playable* EmptyPlayableSet::operator[](size_t) const
{ // Must not call this function!
  ASSERT(1==0);
  return NULL;
}

bool EmptyPlayableSet::contains(const Playable&) const
{ return false;
}

static const EmptyPlayableSet EmptySet;
const PlayableSetBase& PlayableSetBase::Empty(EmptySet);

int PlayableSetBase::compareTo(const PlayableSetBase& r) const
{ DEBUGLOG(("PlayableSetBase(%p{%u,...})::compareTo(&%p{%u,...})\n", this, size(), &r, r.size()));
  if (&r == this)
    return 0; // Comparsion to itself
  size_t p1 = 0;
  size_t p2 = 0;
  for (;;)
  { // termination condition
    if (p2 == r.size())
      return p1 != size();
    else if (p1 == size())
      return -1;
    // compare content
    int ret = (*this)[p1]->compareTo(*r[p2]);
    DEBUGLOG2(("PlayableSetBase::compareTo %p <=> %p = %i\n", (*this)[p1], r[p2], ret));
    if (ret)
      return ret;
    ++p1;
    ++p2;
  }
}

bool PlayableSetBase::isSubsetOf(const PlayableSetBase& r) const
{ DEBUGLOG(("PlayableSetBase(%p{%u,...})::isSubsetOf(&%p{%u,...})\n", this, size(), &r, r.size()));
  if (&r == this)
    return true;
  if (size() > r.size())
    return false; // since PlayableSet is a unique container this condition is sufficient
  if (size() == 0)
    return true; // an empty set is always included
  size_t p1 = 0;
  size_t p2 = 0;
  for (;;)
  { // compare content
    int ret = (*this)[p1]->compareTo(*r[p2]);
    DEBUGLOG2(("PlayableSetBase::isSubsetOf %p <=> %p = %i\n", (*this)[p1], r[p2], ret));
    if (ret > 0)
      return false; // no match for **ppp1
    ++p2;
    if (p2 == r.size())
      return false; // no match for **ppp1 because no more elements in r
    if (ret < 0)
      continue; // only increment ppp2
    ++p1;
    if (p1 == size())
      return true; // all elements found
  }
}

#ifdef DEBUG
xstring PlayableSetBase::DebugDump() const
{ xstring r = xstring::empty;
  for (size_t i = 0; i != size(); ++i)
    if (r.length())
      r = xstring::sprintf("%s, %p", r.cdata(), (*this)[i]);
    else
      r = xstring::sprintf("%p", (*this)[i]);
  return r;
}
#endif


PlayableSet::PlayableSet()
: sorted_vector<Playable, Playable>(8)
{ DEBUGLOG(("PlayableSet(%p)::PlayableSet()\n", this));
}
PlayableSet::PlayableSet(const PlayableSetBase& r)
: sorted_vector<Playable, Playable>(r.size())
{ DEBUGLOG(("PlayableSet(%p)::PlayableSet(const PlayableSetBase&{%u...})\n", this, r.size()));
  for (int i = 0; i != r.size(); ++i)
    append() = r[i];
}
PlayableSet::PlayableSet(const PlayableSet& r)
: sorted_vector<Playable, Playable>(r)
{ DEBUGLOG(("PlayableSet(%p)::PlayableSet(const PlayableSet&{%u...})\n", this, r.size()));
}


OwnedPlayableSet::OwnedPlayableSet()
: sorted_vector_int<Playable, Playable>(8)
{ DEBUGLOG(("OwnedPlayableSet(%p)::OwnedPlayableSet()\n", this));
}
OwnedPlayableSet::OwnedPlayableSet(const PlayableSetBase& r)
: sorted_vector_int<Playable, Playable>(r.size())
{ DEBUGLOG(("OwnedPlayableSet(%p)::OwnedPlayableSet(const PlayableSetBase&{%u...})\n", this, r.size()));
  for (int i = 0; i != r.size(); ++i)
    append() = r[i];
}
OwnedPlayableSet::OwnedPlayableSet(const OwnedPlayableSet& r)
: sorted_vector_int<Playable, Playable>(r)
{ DEBUGLOG(("OwnedPlayableSet(%p)::OwnedPlayableSet(const OwnedPlayableSet&{%u...})\n", this, r.size()));
}

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
  UpdateRpl(&defrpl, true);
}

Playable::InfoFlags Song::DoLoadInfo(InfoFlags what)
{ DEBUGLOG(("Song(%p)::DoLoadInfo(%x) - %s\n", this, what, GetDecoder()));
  what |= BeginUpdate(IF_Other); // IF_Other always inclusive when we call dec_fileinfo
  // get information
  sco_ptr<DecoderInfo> info(new DecoderInfo()); // a bit much for the stack
  INFOTYPE what2 = (INFOTYPE)((int)what & INFO_ALL); // inclompatible types
  DecoderName decoder = "";
  int rc = dec_fileinfo(GetURL(), &what2, info.get(), decoder, sizeof decoder);
  InfoFlags done = (InfoFlags)what2 | IF_All;
  what |= BeginUpdate(done);
  DEBUGLOG(("Song::LoadInfo - rc = %i\n", rc));
  // update information
  Lock lock(*this);
  bool valid = rc == 0;
  if (!valid)
  { if (*info->tech->info == 0)
      sprintf(info->tech->info, "Decoder error %i", rc);
  }
  UpdateInfo(valid ? STA_Valid : STA_Invalid, info.get(), decoder, what, valid); // Reset fields
  return what;
}

ULONG Song::SaveMetaInfo(const META_INFO& info, int haveinfo)
{ DEBUGLOG(("Song(%p{%s})::SaveMetaInfo(, %x)\n", this, GetURL().cdata(), haveinfo));
  haveinfo &= DECODER_HAVE_TITLE|DECODER_HAVE_ARTIST|DECODER_HAVE_ALBUM  |DECODER_HAVE_TRACK
             |DECODER_HAVE_YEAR |DECODER_HAVE_GENRE |DECODER_HAVE_COMMENT|DECODER_HAVE_COPYRIGHT; 
  EnsureInfo(Playable::IF_Other);
  ULONG rc = dec_saveinfo(GetURL(), &info, haveinfo, GetDecoder());
  if (rc == 0)
  { Lock lock(*this);
    if (BeginUpdate(IF_Meta))
    { META_INFO new_info = *GetInfo().meta; // copy
      if (haveinfo & DECODER_HAVE_TITLE)
        strlcpy(new_info.title,    info.title,      sizeof new_info.title);
      if (haveinfo & DECODER_HAVE_ARTIST)
        strlcpy(new_info.artist,   info.artist,     sizeof new_info.artist);
      if (haveinfo & DECODER_HAVE_ALBUM)
        strlcpy(new_info.album,    info.album,      sizeof new_info.album);
      if (haveinfo & DECODER_HAVE_TRACK)
        new_info.track = info.track;
      if (haveinfo & DECODER_HAVE_YEAR)
        strlcpy(new_info.year,      info.year,      sizeof new_info.year);
      if (haveinfo & DECODER_HAVE_GENRE)
        strlcpy(new_info.genre,     info.genre,     sizeof new_info.genre);
      if (haveinfo & DECODER_HAVE_COMMENT)
        strlcpy(new_info.comment,   info.comment,   sizeof new_info.comment);
      if (haveinfo & DECODER_HAVE_COPYRIGHT)
        strlcpy(new_info.copyright, info.copyright, sizeof new_info.copyright);
      UpdateMeta(&new_info);
      EndUpdate(IF_Meta);
    }
  }
  return rc;
}
