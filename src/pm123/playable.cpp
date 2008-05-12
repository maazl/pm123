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

const Playable::DecoderInfo Playable::DecoderInfo::InitialInfo;

void Playable::DecoderInfo::Init()
{ Format.size   = sizeof Format;
  TechInfo.size = sizeof TechInfo;
  MetaInfo.size = sizeof MetaInfo;
  size          = sizeof(DECODER_INFO2);
  format        = &Format;
  tech          = &TechInfo;
  meta          = &MetaInfo;
}

void Playable::DecoderInfo::Reset()
{ memset(&TechInfo, 0, sizeof TechInfo);
  memset(&MetaInfo, 0, sizeof MetaInfo);
  memset((DECODER_INFO2*)this, 0, sizeof(DECODER_INFO2));
  Init();
  Format  .samplerate = -1;
  Format  .channels   = -1;
  TechInfo.songlength = -1;
  TechInfo.bitrate    = -1;
  TechInfo.filesize   = -1;
  TechInfo.num_items  = -1;
  MetaInfo.track      = -1;
}

void Playable::DecoderInfo::operator=(const DECODER_INFO2& r)
{ Reset();
  // version stable copy
  if (r.format->size < sizeof Format)
    memcpy(&Format, r.format, r.format->size);
   else
    Format = *r.format;
  if (r.tech->size < sizeof TechInfo)
    memcpy(&TechInfo, r.tech, r.tech->size);
   else
    TechInfo = *r.tech;
  if (r.meta->size < sizeof MetaInfo)
    memcpy(&MetaInfo, r.meta, r.meta->size);
   else
    MetaInfo = *r.meta;
  memcpy((DECODER_INFO2*)this, &r, sizeof(DECODER_INFO2));
  Init();
}


Playable::Playable(const url123& url, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: URL(url),
  Stat(STA_Unknown),
  InfoValid(IF_None),
  InfoChangeFlags(IF_None),
  InfoRequest(IF_None),
  InfoRequestLow(IF_None)
{ DEBUGLOG(("Playable(%p)::Playable(%s, %p, %p, %p)\n", this, url.cdata(), ca_format, ca_tech, ca_meta));
  Info.Reset();

  if (ca_format)
  { *Info.format = *ca_format;
    InfoValid |= IF_Format;
  }
  if (ca_tech)
  { *Info.tech = *ca_tech;
    InfoValid |= IF_Tech;
  }
  if (ca_meta)
  { *Info.meta = *ca_meta;
    InfoValid |= IF_Meta;
  }
  Decoder[0] = 0;
}

Playable::~Playable()
{ DEBUGLOG(("Playable(%p{%s})::~Playable()\n", this, URL.cdata()));
  // Deregister from repository automatically
  { Mutex::Lock lock(RPMutex);
    XASSERT(RPInst.erase(URL), != NULL);
  }
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
  Mutex::Lock lock(Mtx);
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
  InfoChangeFlags |= (Playable::InfoFlags)(memcmpcpy(Info.format, info, sizeof *Info.format) * IF_Format);
  DEBUGLOG(("Playable::UpdateInfo: %x %u {,%u,%u}\n", InfoValid, InfoChangeFlags, Info.format->samplerate, Info.format->channels));
}
void Playable::UpdateInfo(const TECH_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(TECH_INFO* %p) - %p\n", info, Info.tech));
  if (!info)
    info = DecoderInfo::InitialInfo.tech;
  InfoValid |= IF_Tech;
  InfoChangeFlags |= (Playable::InfoFlags)(memcmpcpy(Info.tech, info, sizeof *Info.tech) * IF_Tech);
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const META_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(META_INFO* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.meta;
  InfoValid |= IF_Meta;
  InfoChangeFlags |= (Playable::InfoFlags)(memcmpcpy(Info.meta, info, sizeof *Info.meta) * IF_Meta);
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const DECODER_INFO2* info)
{ DEBUGLOG(("Playable::UpdateInfo(DECODER_INFO2* %p)\n", info));
  if (!info)
    info = &DecoderInfo::InitialInfo;
  InfoValid |= IF_Other;
  InfoChangeFlags |= (Playable::InfoFlags)(memcmpcpy(&Info.meta_write, &info->meta_write, sizeof(DECODER_INFO2) - offsetof(DECODER_INFO2, meta_write)) * IF_Other);
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
  UpdateInfo(info->format);
  UpdateInfo(info->tech);
  UpdateInfo(info->meta);
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
{ ASSERT(Mtx.GetStatus() == Mutex::Mine);
  if (InfoChangeFlags)
    InfoChange(change_args(*this, InfoChangeFlags));
  InfoChangeFlags = IF_None;
}

void Playable::SetMetaInfo(const META_INFO* meta)
{ Mutex::Lock lock(Mtx);
  UpdateInfo(meta);
  RaiseInfoChange();
}

void Playable::SetTechInfo(const TECH_INFO* tech)
{ Mutex::Lock lock(Mtx);
  UpdateInfo(tech);
  RaiseInfoChange();
}

Playable::InfoFlags Playable::LoadInfo(InfoFlags what)
{ // We always reset the flags of both priorities.
  InterlockedAnd(InfoRequest, ~what);
  InterlockedAnd(InfoRequestLow, ~what);
  // Well we might remove an outstanding request in the worker queue here,
  // but it is simply faster to wait for the request to arrive and ignore it then.
  return what;
}

void Playable::LoadInfoAsync(InfoFlags what, bool lowpri)
{ DEBUGLOG(("Playable(%p{%s})::LoadInfoAsync(%x, %u) - %x %x %x\n", this, GetURL().getShortName().cdata(), what, lowpri, InfoValid, InfoRequest, InfoRequestLow));
  if (what == 0)
    return;
  // schedule request
  unsigned& rq = lowpri ? InfoRequestLow : InfoRequest;
  InfoFlags oldrq = (InfoFlags)rq; // This read is non-atomic. But on the worst case an empty request is scheduled to the worker thread.
  InterlockedOr(rq, what);
  // Only write the Queue if there was no request before.
  if (!oldrq)
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
void Playable::worker_queue::CommitRead()
{ DEBUGLOG(("Playable::worker_queue::CommitRead() - %p %p %p\n", Head, HPTail, Tail));
  Mutex::Lock lock(Mtx);
  if (HPTail == Head)
    HPTail = NULL;
  queue<QEntry>::CommitRead();
}

void Playable::worker_queue::Write(const QEntry& data, bool lowpri)
{ DEBUGLOG(("Playable::worker_queue::Write(%p, %u) - %p %p %p\n", data.get(), lowpri, Head, HPTail, Tail));
  Mutex::Lock lock(Mtx);
  // if low priority or high priority with insert at the end
  if (lowpri || HPTail == Tail || (Head == Tail && EvRead.IsSet()))
    queue<QEntry>::Write(data);
  else
  { // high priority insert in the middle
    // If no high priority item currently in the queue and the head is in use.
    // => insert after head, else insert behind HPTail
    EntryBase* after = HPTail == NULL && EvRead.IsSet() ? Head : HPTail;
    EntryBase* newitem = new Entry(data);
    queue_base::Write(newitem, after);
    HPTail = newitem->Next;
  }
}

Playable::worker_queue Playable::WQueue;
int Playable::WTid = -1;
bool Playable::WTermRq = false;

static void PlayableWorker(void*)
{ for (;;)
  { DEBUGLOG(("PlayableWorker() looking for work\n"));
    Playable::WQueue.RequestRead();
    Playable::QEntry& qp = *Playable::WQueue.Read();
    DEBUGLOG(("PlayableWorker received message %p\n", qp.get()));

    if (Playable::WTermRq || !qp) // stop
    { Playable::WQueue.CommitRead();
      break;
    }

    // Do the work
    // Handle low priority requests correctly if no other requests are on the way.
    if (qp->InfoRequest == 0)
      InterlockedOr(qp->InfoRequest, qp->InfoRequestLow);
    if (qp->InfoRequest)
      Playable::InfoFlags what = qp->LoadInfo((Playable::InfoFlags)qp->InfoRequest);
    // finished
    Playable::WQueue.CommitRead();
  }
}

void Playable::Init()
{ DEBUGLOG(("Playable::Init()\n"));
  // start the worker
  ASSERT(WTid == -1);
  WTermRq = false;
  WTid = _beginthread(&PlayableWorker, NULL, 65536, NULL);
  ASSERT(WTid != -1);
}

void Playable::Uninit()
{ DEBUGLOG(("Playable::Uninit()\n"));
  WTermRq = true;
  WQueue.Write(NULL, false); // deadly pill
  if (WTid != -1)
    wait_thread_pm(amp_player_hab(), WTid, 60000);
  DEBUGLOG(("Playable::Uninit - complete\n"));
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
  Playable* pp = RPInst.find(url);
  CritSect cs;
  return pp && !pp->RefCountIsUnmanaged() ? pp : NULL;
}

int_ptr<Playable> Playable::GetByURL(const url123& URL, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
{ DEBUGLOG(("Playable::GetByURL(%s)\n", URL.cdata()));
  Mutex::Lock lock(RPMutex);
  #ifdef DEBUG
  //RPDebugDump();
  #endif
  Playable*& pp = RPInst.get(URL);
  { CritSect cs;
    if (pp && pp->RefCountIsUnmanaged())
      pp = NULL; // Do not return itmes that are about to die.
  }
  if (pp == NULL)
  { // factory
    if (URL.isScheme("file:") && URL.getObjectName().length() == 0)
      pp = new PlayFolder(URL, ca_tech, ca_meta);
     else if (IsPlaylist(URL))
      pp = new Playlist(URL, ca_tech, ca_meta);
     else // Song
      pp = new Song(URL, ca_format, ca_tech, ca_meta);
    DEBUGLOG(("Playable::GetByURL: factory &%p{%p}\n", &pp, pp));
  } else if ( (ca_format && !(pp->InfoValid & IF_Format)) // Double check
           || (ca_tech && !(pp->InfoValid & IF_Tech))
           || (ca_meta && !(pp->InfoValid & IF_Meta)) )
  { // Merge meta info, but do this outside the above mutex
    lock.Release();
    Mutex::Lock lock2(pp->Mtx);
    if (ca_format && !(pp->InfoValid & IF_Format))
      pp->UpdateInfo(ca_format);
    if (ca_tech && !(pp->InfoValid & IF_Tech))
      pp->UpdateInfo(ca_tech);
    if (ca_meta && !(pp->InfoValid & IF_Meta))
      pp->UpdateInfo(ca_meta);
    pp->RaiseInfoChange();
  }
  return pp;
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
{ Playable*const* ppp1 = begin();
  Playable*const* ppp2 = r.begin();
  for (;;)
  { // termination condition
    if (ppp2 == r.end())
      return ppp1 != end();
    else if (ppp1 == end())
      return -1;
    // compare content
    int ret = (*ppp1)->compareTo(**ppp2);
    if (ret)
      return ret;
    ++ppp1;
    ++ppp2;
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

Playable::InfoFlags Song::LoadInfo(InfoFlags what)
{ DEBUGLOG(("Song(%p)::LoadInfo() - %s\n", this, Decoder));
  // get information
  sco_ptr<DecoderInfo> info(new DecoderInfo()); // a bit much for the stack
  Mutex::Lock lock(Mtx);
  int rc = dec_fileinfo(GetURL(), info.get(), Decoder, sizeof Decoder);
  PlayableStatus stat;
  DEBUGLOG(("Song::LoadInfo - rc = %i\n", rc));
  // update information
  if (rc != 0)
  { stat = STA_Invalid;
    info = NULL; // free structure
  } else
    stat = Stat == STA_Used ? STA_Used : STA_Normal;
  UpdateStatus(stat);
  UpdateInfo(info.get());
  InfoValid |= IF_All;
  RaiseInfoChange();
  return Playable::LoadInfo(IF_All);
}

