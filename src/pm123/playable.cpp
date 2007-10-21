/*
 * Copyright 2007-2007 M.Mueller
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
#include "plugman.h"
#include "playable.h"
#include <xio.h>
#include <assert.h>
#include <stdio.h>

#include <pm123.h>
#include <stddef.h>

#include <debuglog.h>

#ifdef DEBUG
#define RP_INITIAL_SIZE 5 // Errors come sooner...
#else
#define RP_INITIAL_SIZE 100
#endif

/* Private class implemenation headers */
#include "playable.i.h"

void PlayableCollectionEnumerator::InitNextLevel()
{ DEBUGLOG(("PlayableCollectionEnumerator(%p)::InitNextLevel()\n", this));
  SubIterator = new PlayableCollectionEnumerator(this);
}

RecursiveEnumerator* PlayableCollectionEnumerator::RecursionCheck(const Playable* item)
{ RecursiveEnumerator* r = PlayEnumerator::RecursionCheck(item);
  DEBUGLOG(("PlayableCollectionEnumerator(%p)::RecursionCheck(%p{%s}): %p\n", this, item, item->GetURL().getShortName().cdata(), r));
  if (r)
    ((PlayableCollectionEnumerator&)*r).Recursive = true;
  return r;
}

void PlayableCollectionEnumerator::Reset()
{ PlayEnumerator::Reset();
  Recursive = false;
}


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
{ memset(&Format,   0, sizeof Format);
  memset(&TechInfo, 0, sizeof TechInfo);
  memset(&MetaInfo, 0, sizeof MetaInfo);
  memset((DECODER_INFO2*)this, 0, sizeof(DECODER_INFO2));
  Init();
}

void Playable::DecoderInfo::operator=(const DECODER_INFO2& r)
{ // version stable copy
  if (r.format->size < sizeof Format)
  { memset(&Format, 0, sizeof Format);
    memcpy(&Format, r.format, r.format->size);
  } else
    Format = *r.format;
  if (r.tech->size < sizeof TechInfo)
  { memset(&TechInfo, 0, sizeof TechInfo);
    memcpy(&TechInfo, r.tech, r.tech->size);
  } else
    TechInfo = *r.tech;
  if (r.meta->size < sizeof MetaInfo)
  { memset(&MetaInfo, 0, sizeof MetaInfo);
    memcpy(&MetaInfo, r.meta, r.meta->size);
  } else
    MetaInfo = *r.meta;
  memcpy((DECODER_INFO2*)this, &r, sizeof(DECODER_INFO2));
  Init();
}


Playable::Playable(const url& url, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: URL(url),
  Stat(STA_Unknown),
  InfoValid(IF_None),
  InfoChangeFlags(IF_None)
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
    Playable* r = RPInst.erase(URL);
    assert(r != NULL);
  }
}

/* Returns true if the specified file is a playlist file. */
bool Playable::IsPlaylist(const url& URL)
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
  return meta.title[0] ? xstring(meta.title) : URL.getShortName();
}

void Playable::UpdateInfo(const FORMAT_INFO2* info)
{ DEBUGLOG(("Playable::UpdateInfo(FORMAT_INFO2* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.format;
  InfoValid |= IF_Format;
  InfoChangeFlags |= memcmpcpy(Info.format, info, sizeof *Info.format) * IF_Format;
  DEBUGLOG(("Playable::UpdateInfo: %x %u {,%u,%u}\n", InfoValid, InfoChangeFlags, Info.format->samplerate, Info.format->channels));
}
void Playable::UpdateInfo(const TECH_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(TECH_INFO* %p) - %p\n", info, Info.tech));
  if (!info)
    info = DecoderInfo::InitialInfo.tech; 
  InfoValid |= IF_Tech;
  InfoChangeFlags |= memcmpcpy(Info.tech, info, sizeof *Info.tech) * IF_Tech;
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const META_INFO* info)
{ DEBUGLOG(("Playable::UpdateInfo(META_INFO* %p)\n", info));
  if (!info)
    info = DecoderInfo::InitialInfo.meta; 
  InfoValid |= IF_Meta;
  InfoChangeFlags |= memcmpcpy(Info.meta, info, sizeof *Info.meta) * IF_Meta;
  DEBUGLOG(("Playable::UpdateInfo: %x %u\n", InfoValid, InfoChangeFlags));
}
void Playable::UpdateInfo(const DECODER_INFO2* info)
{ DEBUGLOG(("Playable::UpdateInfo(DECODER_INFO2* %p)\n", info));
  if (!info)
    info = &DecoderInfo::InitialInfo; 
  InfoValid |= IF_Other;
  InfoChangeFlags |= memcmpcpy(&Info.meta_write, &info->meta_write, sizeof(DECODER_INFO2) - offsetof(DECODER_INFO2, meta_write)) * IF_Other;
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

bool Playable::SetMetaInfo(const META_INFO* meta)
{ Mutex::Lock lock(Mtx);
  UpdateInfo(meta);
  RaiseInfoChange();
  return true;
}

void Playable::LoadInfoAsync(InfoFlags what)
{ DEBUGLOG(("Playable(%p{%s})::LoadInfoAsync(%x)\n", this, GetURL().getShortName().cdata(), what));
  if (what == 0)
    return;
  // schedule request
  QEntry entry(this, what);
  Mutex::Lock(WQueue.Mtx);
  // merge with another entry if any
  bool inuse;
  QEntry* qp = WQueue.Find(entry, inuse);
  if (qp)
  { if (!inuse)
    { // merge requests
      qp->Request |= what;
      return;
    }
    // The found item is currently processed
    // Look if the requested flags are on the way and enqueue only the missing ones.
    entry.Request &= ~qp->Request;
    if (entry.Request == 0)
      return; 
  }
  // enqueue the request
  WQueue.Write(entry);
}

Playable::InfoFlags Playable::EnsureInfoAsync(InfoFlags what)
{ InfoFlags avail = what & InfoValid;
  if ((what &= ~avail) != 0)
    // schedule request
    LoadInfoAsync(what);
  return avail;
}

/* Worker Queue */
queue<Playable::QEntry> Playable::WQueue;
int Playable::WTid = -1;
bool Playable::WTermRq = false;

static void PlayableWorker(void*)
{ HAB hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(hab, 0);
  for (;;)
  { DEBUGLOG(("PlayableWorker() looking for work\n"));
    queue<Playable::QEntry>::Reader rdr(Playable::WQueue);
    Playable::QEntry& qp = rdr;
    DEBUGLOG(("PlayableWorker received message %p\n", &*qp));
  
    if (Playable::WTermRq || !qp) // stop
      break;
 
    // Do the work
    qp->LoadInfo(qp.Request);
  }
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}

void Playable::Init()
{ DEBUGLOG(("Playable::Init()\n"));
  // start the worker
  assert(WTid == -1);
  WTermRq = false;
  WTid = _beginthread(&PlayableWorker, NULL, 65536, NULL);
  assert(WTid != -1);
}

void Playable::Uninit()
{ DEBUGLOG(("Playable::Uninit()\n"));
  WTermRq = true;
  WQueue.Write(QEntry(NULL, Playable::IF_None)); // deadly pill
  DosWaitThread(&(TID&)WTid, DCWW_WAIT);
  DEBUGLOG(("Playable::Uninit - complete\n"));
}


/* URL repository */
sorted_vector<Playable, char> Playable::RPInst(RP_INITIAL_SIZE);
Mutex Playable::RPMutex;

int Playable::CompareTo(const char* str) const
{ return stricmp(URL, str);
}

int_ptr<Playable> Playable::FindByURL(const char* url)
{ DEBUGLOG(("Playable::FindByURL(%s)\n", url));
  Mutex::Lock lock(RPMutex);
  return RPInst.find(url);
}

int_ptr<Playable> Playable::GetByURL(const url& URL, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
{ DEBUGLOG(("Playable::GetByURL(%s)\n", URL.cdata()));
  Mutex::Lock lock(RPMutex);
  Playable*& pp = RPInst.get(URL);
  if (pp)
    return pp;
  // factory
  DEBUGLOG(("Playable::GetByURL: factory &%p{%p}\n", &pp, pp));
  if (URL.isScheme("file:") && URL.getObjectName().length() == 0)
    pp = new PlayFolder(URL, ca_tech, ca_meta);
   else if (IsPlaylist(URL))
    pp = new Playlist(URL, ca_tech, ca_meta);
   else // Song
    pp = new Song(URL, ca_format, ca_tech, ca_meta);
  return pp;
}


/****************************************************************************
*
*  class PlayableInstance
*
****************************************************************************/

PlayableInstance::slice PlayableInstance::slice::Initial;


PlayableInstance::PlayableInstance(PlayableCollection& parent, int_ptr<Playable> playable)
: Parent(parent),
  RefTo(playable),
  Stat(STA_Normal)
{ DEBUGLOG(("PlayableInstance(%p)::PlayableInstance(%p{%s}, %p{%s})\n", this,
    &parent, parent.GetURL().getShortName().cdata(), &*playable, playable->GetURL().getShortName().cdata()));
}

PlayableInstance::~PlayableInstance()
{ DEBUGLOG(("PlayableInstance(%p)::~PlayableInstance() - %p\n", this, &*RefTo));
  // signal dependant objects
  StatusChange(change_args(*this, SF_Destroy));
  DEBUGLOG(("PlayableInstance::~PlayableInstance() - done\n"));
}

void PlayableInstance::SetInUse(bool used)
{ DEBUGLOG(("PlayableInstance(%p)::SetInUse(%u)\n", this, used));
  PlayableStatus old_stat = Stat;
  Stat = used ? STA_Used : STA_Normal;
  if (Stat != old_stat)
    StatusChange(change_args(*this, SF_InUse));
}

xstring PlayableInstance::GetDisplayName() const
{ DEBUGLOG2(("PlayableInstance(%p)::GetDisplayName()\n", this));
  return Alias ? Alias : RefTo->GetDisplayName();
}

void PlayableInstance::SetAlias(const xstring& alias)
{ DEBUGLOG(("PlayableInstance(%p)::SetAlias(%p{%s})\n", this, alias.cdata(), alias ? alias.cdata() : ""));
  if (Alias != alias)
  { Alias = alias;
    StatusChange(change_args(*this, SF_Alias));
  }
}

void PlayableInstance::SetSlice(const slice& sl)
{ DEBUGLOG(("PlayableInstance(%p)::SetSlice({%f,%f})\n", this, sl.Start, sl.Stop));
  if (Slice.Start != sl.Start || Slice.Stop != sl.Stop)
  { Slice = sl;
    StatusChange(change_args(*this, SF_Slice));
  }
}


/****************************************************************************
*
*  class Song
*
****************************************************************************/

void Song::LoadInfo(InfoFlags what)
{ DEBUGLOG(("Song(%p)::LoadInfo() - %s\n", this, Decoder));
  // get information
  sco_ptr<DecoderInfo> info = new DecoderInfo(); // a bit much for the stack
  int rc = dec_fileinfo(GetURL(), &*info, Decoder);
  PlayableStatus stat;
  DEBUGLOG(("Song::LoadInfo - rc = %i\n", rc));
  // update information
  Mutex::Lock lock(Mtx);
  if (rc != 0)
  { stat = STA_Invalid;
    info = NULL; // free structure
  } else
    stat = Stat == STA_Used ? STA_Used : STA_Normal;
  UpdateStatus(stat);
  UpdateInfo(&*info);
  RaiseInfoChange();
}


/****************************************************************************
*
*  class PlayableEnumerator
*
****************************************************************************/

void PlayableEnumerator::Reset()
{ DEBUGLOG(("PlayableEnumerator(%p{%p})::Reset()\n", this, Current));
  Current = NULL;
}


/****************************************************************************
*
*  class PlayableCollection
*
****************************************************************************/

bool PlayableCollection::Enumerator::Prev()
{ DEBUGLOG(("PlayableCollection::Enumerator(%p{%p,->{%s}})::Prev()\n", this, Current, Parent->GetURL().getShortName().cdata()));
  Current = Current ? ((Entry*)Current)->Prev : Parent->Tail;
  DEBUGLOG(("PlayableCollection::Enumerator(%p)::Prev() - %p\n", this, Current));
  return IsValid();
}

bool PlayableCollection::Enumerator::Next()
{ DEBUGLOG(("PlayableCollection::Enumerator(%p{%p,->{%s}})::Next()\n", this, Current, Parent->GetURL().getShortName().cdata()));
  Current = Current ? ((Entry*)Current)->Next : Parent->Head;
  DEBUGLOG(("PlayableCollection::Enumerator(%p)::Next() - %p\n", this, Current));
  return IsValid();
}

PlayableEnumerator* PlayableCollection::Enumerator::Clone() const
{ return new Enumerator(*this);
}

const FORMAT_INFO2 PlayableCollection::no_format =
{ sizeof(FORMAT_INFO2),
  -1,
  -1
};

PlayableCollection::PlayableCollection(const url& URL, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: Playable(URL, &no_format, ca_tech, ca_meta),
  Head(NULL),
  Tail(NULL),
  Sort(Native)
{ DEBUGLOG(("PlayableCollection(%p)::PlayableCollection(%s, %p, %p)\n", this, URL.cdata(), ca_tech, ca_meta));
}

PlayableCollection::~PlayableCollection()
{ DEBUGLOG(("PlayableCollection(%p{%s})::~PlayableCollection()\n", this, GetURL().getShortName().cdata()));
  // frist disable all events
  CollectionChange.reset();
  InfoChange.reset();
  // free nested entries
  Entry* ep = Head;
  Entry* ep2;
  while (ep != NULL)
  { ep2 = ep;
    ep = ep->Next;
    delete ep2;
  }
}

Playable::Flags PlayableCollection::GetFlags() const
{ return Enumerable;
}

bool PlayableCollection::IsModified() const
{ return false;
}

PlayableCollection::Entry* PlayableCollection::CreateEntry(const char* url, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
{ DEBUGLOG(("PlayableCollection(%p{%s})::CreateEntry(%s)\n", this, GetURL().getShortName().cdata(), url));
  // now create the entry with absolute path
  int_ptr<Playable> pp = GetByURL(GetURL().makeAbsolute(url), ca_format, ca_tech, ca_meta);
  // initiate prefetch of information
  pp->EnsureInfoAsync(IF_Status);
  return new Entry(*this, pp, &ChildInfoChange);
}

void PlayableCollection::AppendEntry(Entry* entry)
{ DEBUGLOG(("PlayableCollection(%p{%s})::AppendEntry(%p{%s,%p,%p})\n", this, GetURL().getShortName().cdata(), entry, entry->GetPlayable().GetURL().getShortName().cdata(), entry->Prev, entry->Next));
  if ((entry->Prev = Tail) != NULL)
    Tail->Next = entry;
   else
    Head = entry;
  Tail = entry;
  DEBUGLOG(("PlayableCollection::AppendEntry - before event\n"));
  CollectionChange(change_args(*this, *entry, Insert));
  DEBUGLOG(("PlayableCollection::AppendEntry - after event\n"));
}

void PlayableCollection::InsertEntry(Entry* entry, Entry* before)
{ DEBUGLOG(("PlayableCollection(%p{%s})::InsertEntry(%p{%s,%p,%p}, %p{%s})\n", this, GetURL().getShortName().cdata(), entry, entry->GetPlayable().GetURL().getShortName().cdata(), entry->Prev, entry->Next, before, before->GetPlayable().GetURL().getShortName().cdata()));
  entry->Next = before;
  if ((entry->Prev = before->Prev) != NULL)
    entry->Prev->Next = entry;
   else
    Head = entry;
  before->Prev = entry;
  DEBUGLOG(("PlayableCollection::InsertEntry - before event\n"));
  CollectionChange(change_args(*this, *entry, Insert));
  DEBUGLOG(("PlayableCollection::InsertEntry - after event\n"));
}

void PlayableCollection::RemoveEntry(Entry* entry)
{ DEBUGLOG(("PlayableCollection(%p{%s})::RemoveEntry(%p{%s,%p,%p})\n", this, GetURL().getShortName().cdata(), entry, entry->GetPlayable().GetURL().getShortName().cdata(), entry->Prev, entry->Next));
  CollectionChange(change_args(*this, *entry, Delete));
  DEBUGLOG(("PlayableCollection::RemoveEntry - after event\n"));
  if (entry->Prev)
    entry->Prev->Next = entry->Next;
   else
    Head = entry->Next;
  if (entry->Next)
    entry->Next->Prev = entry->Prev;
   else
    Tail = entry->Prev;
  delete entry;
}

void PlayableCollection::AddTechInfo(TECH_INFO& dst, const TECH_INFO& tech)
{ DEBUGLOG(("PlayableCollection::AddTechInfo({%f, %f}, {%f, %f})\n",
    dst.songlength, dst.filesize, tech.songlength, tech.filesize));
  
  // cummulate playing time
  if (dst.songlength >= 0)
  { if (tech.songlength >= 0)
      dst.songlength += tech.songlength;
     else
      dst.songlength = -1;
  }
  // cummulate file size
  if (dst.filesize >= 0)
  { if (tech.filesize >= 0)
      dst.filesize += tech.filesize;
     else
      dst.filesize = -1;
  }
  // Number of items
  if (dst.num_items >= 0)
  { if (tech.num_items >= 0)
      dst.num_items += tech.num_items;
     else
      dst.num_items = -1;
  }
}

void PlayableCollection::ChildInfoChange(const Playable::change_args& args)
{ DEBUGLOG(("PlayableCollection(%p{%s})::ChildInfoChange({{%s},%x})\n", this, GetURL().getShortName().cdata(),
    args.Instance.GetURL().getShortName().cdata(), args.Flags));
  // update dependant tech info, but only if already available
  if (args.Flags & InfoValid & IF_Tech)
    LoadInfoAsync(IF_Tech);
}

void PlayableCollection::CalcTechInfo(TECH_INFO& dst)
{ DEBUGLOG(("PlayableCollection(%p{%s})::CalcTechInfo(%p{}) - %x\n", this, GetURL().getShortName().cdata(), &dst, InfoValid));

  memset(&dst, 0, sizeof dst);
  dst.size = sizeof dst;

  if (Stat == STA_Invalid)
    strcpy(dst.info, "invalid");
  else
  { PlayableCollectionEnumerator penum;
    penum.Attach(this);
    Song* song;
    while ((song = penum.Next()) != NULL)
    { song->EnsureInfoAsync(Playable::IF_Tech);
      // Whether or not the information is yet available, we work with the current values to avoid deadlocks.
      AddTechInfo(dst, *song->GetInfo().tech);
    }
    dst.recursive = penum.GetRecursive();

    dst.bitrate = dst.filesize >= 0 && dst.songlength > 0
      ? dst.filesize / dst.songlength / (1000/8)
      : -1;
    DEBUGLOG(("PlayableCollection::CalcTechInfo(): %s {%f, %i, %f}\n", GetURL().getShortName().cdata(), dst.songlength, dst.bitrate, dst.filesize));
    // Info string
    // Since all strings are short, there may be no buffer overrun.
    if (dst.songlength < 0)
      strcpy(dst.info, "unknown length");
     else
    { int days = (int)(dst.songlength / 86400.);
      double rem = dst.songlength - 86400.*days;
      int hours = (int)(rem / 3600.);
      rem -= 3600*hours;
      int minutes = (int)(rem / 60);
      rem -= 60*minutes;
      if (days)
        sprintf(dst.info, "Total: %ud %u:%02u:%02u", days, hours, minutes, (int)rem);
       else 
        sprintf(dst.info, "Total: %u:%02u:%02u", hours, minutes, (int)rem);
    }
    if (dst.recursive)
      strcat(dst.info, ", recursion");
  }
}

PlayableEnumerator* PlayableCollection::GetEnumerator()
{ DEBUGLOG(("PlayableCollection(%p{%s})::GetEnumerator()\n", this, GetURL().getShortName().cdata()));
  EnsureInfo(IF_Other);
  return new Enumerator(this);
}

void PlayableCollection::LoadInfo(InfoFlags what)
{ DEBUGLOG(("PlayableCollection(%p{%s})::LoadInfo() - %x %u\n", this, GetURL().getShortName().cdata(), InfoValid, Stat));
  Mutex::Lock lock(Mtx);
  if ((what & (IF_Other|IF_Status|IF_Tech)) == 0)
  { // default implementation to fullfill the minimum requirements of LoadInfo.
    InfoValid |= what;
    return;
  }

  if (what & (IF_Other|IF_Status))
  { // load playlist content
    PlayableStatus stat;
    if (LoadInfoCore())
      stat = Stat == STA_Used ? STA_Used : STA_Normal;
    else
      stat = STA_Invalid;
    UpdateStatus(stat);
    InfoValid |= IF_Other;
  }

  if (what & IF_Tech) 
  { // update tech info
    TECH_INFO tech;
    CalcTechInfo(tech);
    UpdateInfo(&tech);
  }
  DEBUGLOG(("PlayableCollection::LoadInfo completed - %x\n", InfoChangeFlags));
  // raise InfoChange event?
  RaiseInfoChange();
}

bool PlayableCollection::SaveLST(XFILE* of, bool relative)
{ DEBUGLOG(("PlayableCollection(%p{%s})::SaveLST(%p, %u)\n", this, GetURL().cdata(), of, relative));
  // Header
  xio_fputs("#\n"
            "# Playlist created with " AMP_FULLNAME "\n"
            "# Do not modify!\n"
            "# Lines starting with '>' are used for optimization.\n"
            "#\n", of); 
  // Content
  Mutex::Lock lock(Mtx);
  sco_ptr<PlayableEnumerator> pe = GetEnumerator();
  while (pe->Next())
  { PlayableInstance* pi = *pe;
    Playable& play = pi->GetPlayable();
    // alias
    if (pi->GetAlias())
    { xio_fputs("#ALIAS ", of);
      xio_fputs(pi->GetAlias(), of);
      xio_fputs("\n", of);
    }
    // slice
    if (pi->GetSlice().Start != PlayableInstance::slice::Initial.Start || pi->GetSlice().Stop != PlayableInstance::slice::Initial.Stop)
    { char buf[50];
      sprintf(buf, "#SLICE %.3f,%.3f\n", pi->GetSlice().Start, pi->GetSlice().Stop);
      xio_fputs(buf, of);
    }
    // comment
    const TECH_INFO& tech = *play.GetInfo().tech;
    if (play.GetStatus() > STA_Invalid && !play.CheckInfo(IF_Tech)) // only if immediately available
    { char buf[50];
      xio_fputs("# ", of);
      if (tech.filesize < 0)
        xio_fputs("n/a", of);
      else
      { sprintf(buf, "%.0lfkiB", tech.filesize/1024.);
        xio_fputs(buf, of);
      }
      xio_fputs(", ", of);
      if (tech.songlength < 0)
        xio_fputs("n/a", of);
      else
      { unsigned long s = (unsigned long)tech.songlength;
        if (s < 60)
          sprintf(buf, "%lu s", s);
        else if (s < 3600)
          sprintf(buf, "%lu:%02lu", s/60, s%60);
        else
          sprintf(buf, "%lu:%02lu:%02lu", s/3600, s/60%60, s%60);
        xio_fputs(buf, of); 
      }
      xio_fputs(", ", of);
      xio_fputs(tech.info, of);
      xio_fputs("\n", of);
    }
    // Object name
    { xstring object = play.GetURL().makeRelative(GetURL(), relative);
      const char* cp = object;
      if (strnicmp(cp,"file://", 7) == 0)
      { cp += 5;
        if (cp[2] == '/')
          cp += 3;
      }
      xio_fputs(cp, of);
      xio_fputs("\n", of);
    }
    // tech info
    if (play.GetStatus() > STA_Invalid && play.CheckInfo(IF_Tech|IF_Format) != IF_Tech|IF_Format)
    { char buf[50];
      if (play.CheckInfo(IF_Tech) == IF_None)
      { sprintf(buf, ">%i", tech.bitrate);
        xio_fputs(buf, of); 
      } else
        xio_fputs(">", of);
      if (play.CheckInfo(IF_Format) == IF_None)
      { const FORMAT_INFO2& format = *play.GetInfo().format;
        sprintf(buf, ",%i,%i", format.samplerate, format.channels);
        xio_fputs(buf, of);
      } else
        xio_fputs(",,", of);
      if (play.CheckInfo(IF_Tech) == IF_None)
      { sprintf(buf, ",%.0f,%.3f", tech.filesize, tech.songlength);
        xio_fputs(buf, of);
        if (play.GetFlags() & Enumerable)
        sprintf(buf, ",%i,%i", tech.num_items, tech.recursive);
        xio_fputs(buf, of);
      }
      xio_fputs("\n", of);
    }
  }
  // Footer
  xio_fputs("# End of playlist\n", of);
  return true;
}

bool PlayableCollection::SaveM3U(XFILE* of, bool relative)
{ DEBUGLOG(("PlayableCollection(%p{%s})::SaveM3U(%p, %u)\n", this, GetURL().cdata(), of, relative));
  Mutex::Lock lock(Mtx);
  sco_ptr<PlayableEnumerator> pe = GetEnumerator();
  while (pe->Next())
  { Playable& play = (*pe)->GetPlayable();
    // Object name
    xstring object = play.GetURL().makeRelative(GetURL(), relative);
    const char* cp = object;
    if (strnicmp(cp,"file://",7) == 0)
    { cp += 5;
      if (cp[2] == '/')
        cp += 3;
    }
    xio_fputs(cp, of);
    xio_fputs("\n", of);
  }
  return true;
}

bool PlayableCollection::Save(const url& URL, save_options opt)
{ DEBUGLOG(("PlayableCollection(%p{%s})::Save(%s, %x)\n", this, GetURL().cdata(), URL.cdata(), opt));
  // verify URL
  if (!IsPlaylist(URL))
    return false;

  // create file
  XFILE* of = xio_fopen(URL, "w");
  if (of == NULL)
    return false;

  // now write the playlist
  bool rc;
  if (opt & SaveAsM3U)
    rc = SaveM3U(of, (opt & SaveRelativePath) != 0);
  else
    rc = SaveLST(of, (opt & SaveRelativePath) != 0);
  
  // done
  xio_fclose(of);
  
  if (!rc)
    return false;

  // notifications
  int_ptr<Playable> pp = FindByURL(URL);
  if (pp && pp != this)
  { assert(pp->GetFlags() & Enumerable);
    ((PlayableCollection&)*pp).NotifySourceChange();
  }
  return true;
}

void PlayableCollection::NotifySourceChange()
{}


/****************************************************************************
*
*  class Playlist
*
****************************************************************************/

void Playlist::LSTReader::Reset()
{ DEBUGLOG(("Playlist::LSTReader::Reset()\n"));
  has_format   = false;
  has_tech     = false;
  has_techinfo = false;
  Alias        = NULL;
  Slice        = PlayableInstance::slice::Initial;
  URL          = NULL;
}

void Playlist::LSTReader::Create()
{ DEBUGLOG(("Playlist::LSTReader::Create() - %p, %u, %u, %u\n", URL.cdata(), has_format, has_tech, has_techinfo));
  if (URL)
    List.AppendEntry(List.CreateEntry(URL, has_format ? &Format : NULL, has_tech && has_techinfo ? &Tech : NULL));
}

void Playlist::LSTReader::ParseLine(char* line)
{ DEBUGLOG(("Playlist::LSTReader::ParseLine(%s)\n", line));
  switch (line[0])
  {case '#':
    // prefix lines
    if (memcmp(line+1, "ALIAS ", 6) == 0) // alias name
    { Alias = line+7;
      break;
    } else if (memcmp(line+1, "SLICE ", 6) == 0) // slice
    { sscanf(line+7, "%lf,%lf", &Slice.Start, &Slice.Stop);
      break;
    } else if (line[1] == ' ')
    { const char* cp = strstr(line+2, ", ");
      if (cp)
      { cp = strstr(cp+2, ", ");
        if (cp)
        { strlcpy(Tech.info, cp+2, sizeof Tech.info);
          has_techinfo = true;
          break;
      } }
    }
    // comments close URL
    Create();
    Reset();
    break;

   default:
    // close last URL
    Create();
    if (URL)
      Reset();
    URL = List.GetURL().makeAbsolute(line);
    break;

   case '>':
    { // tokenize string
      const char* tokens[7] = {NULL};
      const char** tp = tokens;
      *tp = strtok(line+1, ",");
      while (*tp)
      { if (**tp == 0)
          *tp = NULL;
        *++tp = strtok(NULL, ",");
        if (tp == tokens + sizeof tokens/sizeof *tokens)
          break;
      }
      DEBUGLOG(("Playlist::LSTReader::ParseLine: tokens %s, %s, %s, %s, %s, %s, %s\n", tokens[0], tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6]));
      // make tech info
      if (tokens[0] && tokens[3] && tokens[4])
      { has_tech = true;
        // memset(Tech, 0, sizeof Tech);
        Tech.size       = sizeof Tech;
        Tech.bitrate    = atoi(tokens[0]);
        Tech.filesize   = atof(tokens[3]);
        Tech.songlength = atof(tokens[4]);
        Tech.num_items  = tokens[5] ? atoi(tokens[5]) : 1;
        Tech.recursive  = tokens[6] != NULL;
      }
      // make format info
      if (tokens[1] && tokens[2])
      { has_format = true;
        // memset(Format, 0, sizeof Format);
        Format.size       = sizeof Format;
        Format.samplerate = atoi(tokens[1]);
        Format.channels   = atoi(tokens[2]);
        switch (Format.channels) // convert stupid modes of old PM123 versions
        {case 0:
          Format.channels = 2;
          break;
         case 3:
          Format.channels = 1;
          break;
        }
      }
      break;
    }

   case '<':
    // close URL
    Create();
    Reset();
    break;
  }
}


Playable::Flags Playlist::GetFlags() const
{ return Mutable;
}

bool Playlist::IsModified() const
{ return Modified;
}

bool Playlist::LoadLST(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadLST(%p)\n", this, GetURL().getShortName().cdata(), x));
  
  // TODO: fixed buffers...
  char line[4096];
  LSTReader rdr(*this);

  while (xio_fgets(line, sizeof(line), x))
  { blank_strip(line);
    if (*line == 0)
      continue;
    rdr.ParseLine(line);
  }
  return true;
}

bool Playlist::LoadMPL(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadMPL(%p)\n", this, GetURL().getShortName().cdata(), x));
  
  // TODO: fixed buffers...
  char   file    [4096];
  char*  eq_pos;

  while (xio_fgets(file, sizeof(file), x))
  {
    blank_strip( file );

    if( *file != 0 && *file != '#' && *file != '[' && *file != '>' && *file != '<' )
    {
      eq_pos = strchr( file, '=' );

      if( eq_pos && strnicmp( file, "File", 4 ) == 0 )
      {
        strcpy( file, eq_pos + 1 );
        // TODO: fetch position
        AppendEntry(CreateEntry(file));
      }
    }
  }
  return true;
}

bool Playlist::LoadPLS(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadPLS(%p)\n", this, GetURL().getShortName().cdata(), x));
  
  char   file    [_MAX_PATH] = "";
  char   title   [_MAX_PATH] = "";
  char   line    [4096];
  int    last_idx = -1;
  BOOL   have_title = FALSE;
  char*  eq_pos;

  while( xio_fgets( line, sizeof(line), x))
  {
    blank_strip( line );

    if( *line != 0 && *line != '#' && *line != '[' && *line != '>' && *line != '<' )
    {
      eq_pos = strchr( line, '=' );

      if( eq_pos ) {
        if( strnicmp( line, "File", 4 ) == 0 )
        {
          if( *file ) {
            // TODO: title, position
            AppendEntry(CreateEntry(file));
            have_title = FALSE;
          }

          strcpy( file, eq_pos + 1 );
          last_idx = atoi( &line[4] );
        }
        else if( strnicmp( line, "Title", 5 ) == 0 )
        {
          // We hope the title field always follows the file field
          if( last_idx == atoi( &line[5] )) {
            strcpy( title, eq_pos + 1 );
            have_title = TRUE;
          }
        }
      }
    }
  }

  if( *file ) {
    // TODO: fetch position
    AppendEntry(CreateEntry(file));
    //pl_add_file( file, have_title ? title : NULL, 0 );
  }

  return true;
}

bool Playlist::LoadInfoCore()
{ DEBUGLOG(("Playlist(%p{%s})::LoadInfoCore()\n", this, GetURL().getShortName().cdata()));

  // clear content if any
  while (Head)
    RemoveEntry(Head);

  bool rc = false;

  xstring ext = GetURL().getExtension();
 
  // open file
  XFILE* x = xio_fopen(GetURL(), "r");
  if (x == NULL)
    amp_player_error("Unable open playlist file:\n%s\n%s", GetURL().cdata(), xio_strerror(xio_errno()));
   else
  { if ( stricmp( ext, ".lst" ) == 0 || stricmp( ext, ".m3u" ) == 0 )
      rc = LoadLST(x);
    else if ( stricmp( ext, ".mpl" ) == 0 )
      rc = LoadMPL(x);
    else if ( stricmp( ext, ".pls" ) == 0 )
      rc = LoadPLS(x);
    else
      amp_player_error("Cannot determine playlist format from file extension %s.\n", ext.cdata());
  
    xio_fclose( x );
  }
  InfoChangeFlags |= IF_Other;
  if (Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = false;
  }
  return rc;
}

void Playlist::InsertItem(const char* url, const xstring& alias, const PlayableInstance::slice& sl, PlayableInstance* before)
{ DEBUGLOG(("Playlist(%p{%s})::InsertItem(%s, %s, {%.1f,%.1f}, %p) - %u\n", this, GetURL().getShortName().cdata(), url, alias?alias.cdata():"<NULL>", sl.Start, sl.Stop, before, Stat));
  Entry* ep = CreateEntry(url);
  ep->SetAlias(alias);
  ep->SetSlice(sl);
  Mutex::Lock lock(Mtx);
  if (Stat <= STA_Invalid)
    UpdateStatus(STA_Normal);

  if (before == NULL)
    AppendEntry(ep);
   else
    InsertEntry(ep, (Entry*)before);
  InfoChangeFlags |= IF_Other;
  if (!Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = true;
  }
  // update info
  LoadInfoAsync(InfoValid & IF_Tech);
  // raise InfoChange event?
  RaiseInfoChange();
}

void Playlist::RemoveItem(PlayableInstance* item)
{ DEBUGLOG(("Playlist(%p{%s})::RemoveItem(%p{%s})\n", this, GetURL().getShortName().cdata(), item, item ? item->GetPlayable().GetURL().getShortName().cdata() : ""));
  Mutex::Lock lock(Mtx);
  if (item)
    RemoveEntry((Entry*)item);
  else
  { if (!Head)
      return; // early exit without change flag
    do
    { RemoveEntry(Head);
      DEBUGLOG(("Playlist::RemoveItem - %p\n", Head));
    } while (Head);
  }
  InfoChangeFlags |= IF_Other;
  if (!Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = true;
  }
  // update tech info
  LoadInfoAsync(InfoValid & IF_Tech);
  // raise InfoChange event?
  DEBUGLOG(("Playlist::RemoveItem: before raiseinfochange\n"));
  RaiseInfoChange();
  DEBUGLOG(("Playlist::RemoveItem: after raiseinfochange\n"));
}

bool Playlist::Save(const url& URL, save_options opt)
{ DEBUGLOG(("Playlist(%p)::Save(%s, %x)\n", this, URL.cdata(), opt));
  if (!PlayableCollection::Save(URL, opt))
    return false;
  if (URL == GetURL())
  { // TODO: this should be done atomic with the save.
    Mutex::Lock lock(Mtx);
    if (Modified)
    { InfoChangeFlags |= IF_Status;
      Modified = false;
      RaiseInfoChange();
    }
  }
  return true;
}

void Playlist::NotifySourceChange()
{ DEBUGLOG(("Playlist(%p{%s})::NotifySourceChange()", this, GetURL().cdata()));
  Mutex::Lock lock(Mtx);
  if (!Modified)
  { InfoChangeFlags |= IF_Status;
    Modified = true;
    RaiseInfoChange();
  }
}


/****************************************************************************
*
*  class PlayFolder
*
****************************************************************************/

PlayFolder::PlayFolder(const url& URL, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: PlayableCollection(URL, ca_tech, ca_meta)
{ DEBUGLOG(("PlayFolder(%p)::PlayFolder(%s, %p, %p)\n", this, URL.cdata(), ca_tech, ca_meta));
  ParseQueryParams();
}

void PlayFolder::ParseQueryParams()
{ xstring params = GetURL().getParameter();
  DEBUGLOG(("PlayFolder(%p)::ParseQueryParams() - %s\n", this, params.cdata()));
  if (params.length() == 0)
    return; // no params
  const char* cp = params.cdata() +1;
  if (*cp == 0)
    return; // empty params
  for(;;)
  { const char* cp2 = strchr(cp, '&');
    xstring arg(cp, cp2 ? cp2-cp : params.cdata()+params.length()-cp);
    DEBUGLOG(("PlayFolder::ParseQueryParams: arg=%s\n", arg.cdata()));
    if (arg.startsWithI("pattern="))
      Pattern.assign(arg.cdata() +8);
    else if (arg.startsWithI("recursive"))
      Recursive = true;
    else
      DEBUGLOG(("PlayFolder::ParseQueryParams: invalid option %s\n", arg.cdata()));
    if (cp2 == NULL)
      break;
    cp = cp2 +1;
  }
  DEBUGLOG(("PlayFolder::ParseQueryParams: %s %u\n", Pattern ? Pattern.cdata() : "<null>", Recursive));
}

bool PlayFolder::LoadInfoCore()
{ DEBUGLOG(("PlayFolder(%p{%s})::LoadInfoCore()\n", this, GetURL().getShortName().cdata()));
  if (!GetURL().isScheme("file:")) // Can't handle anything but filesystem folders so far.
    return false;
  xstring name = GetURL().getBasePath();
  // strinp file:[///]
  if (memcmp(name.cdata()+5, "///", 3) == 0) // local path?
    name.assign(name, 8);
   else
    name.assign(name, 5);
  // append pattern
  if (Pattern)
    name = name + Pattern;
   else
    name = name + "*";
  HDIR hdir = HDIR_CREATE;
  char result[2048];
  ULONG count = sizeof result / sizeof(FILEFINDBUF3);
  APIRET rc = DosFindFirst(name, &hdir, Recursive ? FILE_ARCHIVED|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY|FILE_DIRECTORY
                                                  : FILE_ARCHIVED|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY,
    &result, sizeof result, &count, FIL_STANDARD);
  while (rc == 0)
  { // add files
    for (FILEFINDBUF3* fp = (FILEFINDBUF3*)result; count--; ((char*&)fp) += fp->oNextEntryOffset)
    { DEBUGLOG(("PlayFolder::LoadInfoCore - %s, %x\n", fp->achName, fp->attrFile));
      // skip . and ..
      if (fp->achName[0] == '.' && (fp->achName[1] == 0 || (fp->achName[1] == '.' && fp->achName[2] == 0)))
        continue;
      Entry* ep;
      if (fp->attrFile & FILE_DIRECTORY)
      { // inherit parameters 
        xstring tmp = xstring::sprintf("%s/%s", fp->achName, GetURL().getParameter().cdata());
        ep = CreateEntry(tmp);
      } else
        ep = CreateEntry(fp->achName);
      AppendEntry(ep);
    }
    // next...
    count = sizeof result / sizeof(FILEFINDBUF3);
    rc = DosFindNext(hdir, &result, sizeof result, &count);
  }
  DosFindClose(hdir);
  DEBUGLOG(("PlayFolder::LoadInfoCore: %s, %u\n", name.cdata(), rc));
  switch (rc)
  {case NO_ERROR:
   case ERROR_FILE_NOT_FOUND:
   case ERROR_NO_MORE_FILES:
    return true;
   default:
    return false;
  }
}


