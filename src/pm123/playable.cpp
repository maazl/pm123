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
#include "playlist.h"
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
#include "playable.private.h"

void PlayableCollectionEnumerator::InitNextLevel()
{ DEBUGLOG(("PlayableCollectionEnumerator(%p)::InitNextLevel()\n", this));
  SubIterator = new PlayableCollectionEnumerator(this);
}

RecursiveEnumerator* PlayableCollectionEnumerator::RecursionCheck(const Playable* item)
{ RecursiveEnumerator* r = PlayEnumerator::RecursionCheck(item);
  DEBUGLOG(("PlayableCollectionEnumerator(%p)::RecursionCheck(%p{%s}): %p\n", this, item, item->GetURL().getObjName().cdata(), r));
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

Playable::Flags Playable::GetFlags() const
{ return None;
}

void Playable::SetInUse(bool used)
{ DEBUGLOG(("Playable(%p{%s})::SetInUse(%u)\n", this, URL.cdata(), used));
  Mutex::Lock lock(Mtx);
  if (Stat == STA_Invalid || Stat == STA_Unknown)
    return; // can't help 
  PlayableStatus old_stat = Stat;
  Stat = used ? STA_Used : STA_Normal;
  if (Stat != old_stat)
  { lock.Release();
    InfoChange(change_args(*this, IF_Status));
  }
}

xstring Playable::GetDisplayName() const
{ DEBUGLOG2(("Playable::GetDisplayName()\n"));
  const META_INFO& meta = *GetInfo().meta;
  return meta.title[0] ? xstring(meta.title) : URL.getObjName();
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

bool Playable::SetMetaInfo(const META_INFO* meta)
{ Mutex::Lock lock(Mtx);
  UpdateInfo(meta);
  RaiseInfoChange();
  return true;
}

void Playable::LoadInfoAsync(InfoFlags what)
{ DEBUGLOG(("Playable(%p{%s})::LoadInfoAsync(%x)\n", this, GetURL().getObjName().cdata(), what));
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
{ for (;;)
  { DEBUGLOG(("PlayableWorker() looking for work\n"));
    queue<Playable::QEntry>::Reader rdr(Playable::WQueue);
    Playable::QEntry& qp = rdr;
    DEBUGLOG(("PlayableWorker received message %p\n", &*qp));
  
    if (Playable::WTermRq || !qp) // stop
      return;
 
    // Do the work
    qp->LoadInfo(qp.Request);
  }
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

int_ptr<Playable> Playable::GetByURL(const char* url, const FORMAT_INFO2* ca_format, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
{ DEBUGLOG(("Playable::GetByURL(%s)\n", url));
  Mutex::Lock lock(RPMutex);
  Playable*& pp = RPInst.get(url);
  if (pp)
    return pp;
  // factory
  DEBUGLOG(("Playable::GetByURL: factory &%p{%p}\n", &pp, pp));
  size_t p = strlen(url);
  if (strnicmp(url, "file:", 5) == 0 && url[p-1] == '/')
    pp = new PlayFolder(url, ca_tech, ca_meta);
   else if (is_playlist(url))
    pp = new Playlist(url, ca_tech, ca_meta);
   else // Song
    pp = new Song(url, ca_format, ca_tech, ca_meta);
  return pp;
}


/****************************************************************************
*
*  class PlayableInstance
*
****************************************************************************/

PlayableInstance::PlayableInstance(PlayableCollection& parent, Playable* playable)
: Parent(parent),
  RefTo(playable),
  Stat(STA_Normal),
  Pos(0)
{ DEBUGLOG(("PlayableInstance(%p)::PlayableInstance(%p{%s}, %p{%s})\n", this,
    &parent, parent.GetURL().getObjName().cdata(), playable, playable->GetURL().getObjName().cdata()));
}

PlayableInstance::~PlayableInstance()
{ DEBUGLOG(("PlayableInstance(%p)::~PlayableInstance() - %p\n", this, &*RefTo));
  // signal dependant objects
  StatusChange(change_args(*this, SF_Destroy));
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

void PlayableInstance::SetPlayPos(double pos)
{ DEBUGLOG(("PlayableInstance(%p)::SetPlayPos(%f)\n", this, pos));
  if (Pos != pos)
  { Pos = pos;
    StatusChange(change_args(*this, SF_PlayPos));
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
  DEBUGLOG(("Song::LoadInfo - rc = %i\n", rc));
  // update information
  Mutex::Lock lock(Mtx);
  if (rc != 0)
  { Stat = STA_Invalid;
    info = NULL; // free structure
  } else
  { if (Stat < STA_Normal)
      Stat = STA_Normal;
  }
  UpdateInfo(&*info);
  InfoValid = IF_All;
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
{ DEBUGLOG(("PlayableCollection::Enumerator(%p{%p,->{%s}})::Prev()\n", this, Current, Parent->GetURL().getObjName().cdata()));
  Current = Current ? ((Entry*)Current)->Prev : Parent->Tail;
  DEBUGLOG(("PlayableCollection::Enumerator(%p)::Prev() - %p\n", this, Current));
  return IsValid();
}

bool PlayableCollection::Enumerator::Next()
{ DEBUGLOG(("PlayableCollection::Enumerator(%p{%p,->{%s}})::Next()\n", this, Current, Parent->GetURL().getObjName().cdata()));
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

PlayableCollection::PlayableCollection(const char* url, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: Playable(url, &no_format, ca_tech, ca_meta),
  Head(NULL),
  Tail(NULL),
  Sort(Native)
{ DEBUGLOG(("PlayableCollection(%p)::PlayableCollection(%s, %p, %p)\n", this, url, ca_tech, ca_meta));
}

PlayableCollection::~PlayableCollection()
{ DEBUGLOG(("PlayableCollection(%p{%s})::~PlayableCollection()\n", this, GetURL().getObjName().cdata()));
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

PlayableCollection::Entry* PlayableCollection::CreateEntry(const char* url)
{ DEBUGLOG(("PlayableCollection(%p{%s})::CreateEntry(%s)\n", this, GetURL().getObjName().cdata(), url));
  // now create the entry with absolute path
  return new Entry(*this, GetByURL(GetURL().makeAbsolute(url)), &ChildInfoChange);
}

void PlayableCollection::AppendEntry(Entry* entry)
{ DEBUGLOG(("PlayableCollection(%p{%s})::AppendEntry(%p{%s,%p,%p})\n", this, GetURL().getObjName().cdata(), entry, entry->GetPlayable().GetURL().getObjName().cdata(), entry->Prev, entry->Next));
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
{ DEBUGLOG(("PlayableCollection(%p{%s})::InsertEntry(%p{%s,%p,%p}, %p{%s})\n", this, GetURL().getObjName().cdata(), entry, entry->GetPlayable().GetURL().getObjName().cdata(), entry->Prev, entry->Next, before, before->GetPlayable().GetURL().getObjName().cdata()));
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
{ DEBUGLOG(("PlayableCollection(%p{%s})::RemoveEntry(%p{%s,%p,%p})\n", this, GetURL().getObjName().cdata(), entry, entry->GetPlayable().GetURL().getObjName().cdata(), entry->Prev, entry->Next));
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
{ DEBUGLOG(("PlayableCollection(%p{%s})::ChildInfoChange({{%s},%x})\n", this, GetURL().getObjName().cdata(),
    args.Instance.GetURL().getObjName().cdata(), args.Flags));
  // update dependant tech info, but only if already available
  if (args.Flags & InfoValid & IF_Tech)
    LoadInfoAsync(IF_Tech);
}

void PlayableCollection::CalcTechInfo(TECH_INFO& dst)
{ DEBUGLOG(("PlayableCollection(%p{%s})::CalcTechInfo(%p{}) - %x\n", this, GetURL().getObjName().cdata(), &dst, InfoValid));

  memset(&dst, 0, sizeof dst);
  dst.size = sizeof dst;

  if (Stat == STA_Invalid)
    strcpy(dst.info, "invalid");
  else
  { PlayableCollectionEnumerator penum;
    penum.Attach(this);
    Song* song;
    while ((song = penum.Next()) != NULL)
    { song->EnsureInfo(Playable::IF_Tech);
      AddTechInfo(dst, *song->GetInfo().tech);
    }
    dst.recursive = penum.GetRecursive();

    dst.bitrate = dst.filesize >= 0 && dst.songlength > 0
      ? dst.filesize / dst.songlength / (1000/8)
      : -1;
    DEBUGLOG(("PlayableCollection::CalcTechInfo(): %s {%f, %i, %f}\n", GetURL().getObjName().cdata(), dst.songlength, dst.bitrate, dst.filesize));
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
{ DEBUGLOG(("PlayableCollection(%p{%s})::GetEnumerator()\n", this, GetURL().getObjName().cdata()));
  EnsureInfo(IF_Other);
  return new Enumerator(this);
}

void PlayableCollection::LoadInfo(InfoFlags what)
{ DEBUGLOG(("PlayableCollection(%p{%s})::LoadInfo() - %x %u\n", this, GetURL().getObjName().cdata(), InfoValid, Stat));
  Mutex::Lock lock(Mtx);
  if ((what & (IF_Other|IF_Status|IF_Tech)) == 0)
  { // default implementation to fullfill the minimum requirements of LoadInfo.
    InfoValid |= what;
    return;
  }

  if (what & (IF_Other|IF_Status))
  { // TODO: We should allw a forced reload
    if (Stat <= STA_Unknown)
    { // load playlist content
      if (LoadInfoCore())
      { InfoChangeFlags |= IF_Status;
        Stat = STA_Normal;
      } else if (Stat == STA_Unknown)
      { InfoChangeFlags |= IF_Status;
        Stat = STA_Invalid;
      }
    }
    InfoValid |= IF_Other|IF_Status;
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

/****************************************************************************
*
*  class Playlist
*
****************************************************************************/

Playable::Flags Playlist::GetFlags() const
{ return Mutable;
}

bool Playlist::LoadLST(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadLST(%p)\n", this, GetURL().getObjName().cdata(), x));
  
  // TODO: fixed buffers...
  char file [4096];

  while (xio_fgets(file, sizeof(file), x))
  {
    blank_strip( file );
    DEBUGLOG(("Playlist::LoadLST - %s\n", file));

    if( *file != 0 && *file != '#' && *file != '>' && *file != '<' ) {
      AppendEntry(CreateEntry(file));
    }
  }
  return true;
}

bool Playlist::LoadMPL(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadMPL(%p)\n", this, GetURL().getObjName().cdata(), x));
  
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
        AppendEntry(CreateEntry(file));
      }
    }
  }
  return true;
}

bool Playlist::LoadPLS(XFILE* x)
{ DEBUGLOG(("Playlist(%p{%s})::LoadPLS(%p)\n", this, GetURL().getObjName().cdata(), x));
  
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
            // TODO: title
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
    AppendEntry(CreateEntry(file));
    //pl_add_file( file, have_title ? title : NULL, 0 );
  }

  return true;
}

bool Playlist::LoadInfoCore()
{ DEBUGLOG(("Playlist(%p{%s})::LoadInfoCore()\n", this, GetURL().getObjName().cdata()));

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

  return rc;
}

void Playlist::InsertItem(const char* url, PlayableInstance* before)
{ DEBUGLOG(("Playlist(%p{%s})::InsertItem(%s, %p) - %u\n", this, GetURL().getObjName().cdata(), url, before, Stat));
  Entry* ep = CreateEntry(url);
  Mutex::Lock lock(Mtx);
  if (Stat <= STA_Invalid)
  { InfoChangeFlags |= IF_Status;
    Stat = STA_Normal;
  }

  if (before == NULL)
    AppendEntry(ep);
   else
    InsertEntry(ep, (Entry*)before);
  InfoChangeFlags |= IF_Other;
  // update info
  LoadInfoAsync(InfoValid & IF_Tech);
  // raise InfoChange event?
  RaiseInfoChange();
}

void Playlist::RemoveItem(PlayableInstance* item)
{ DEBUGLOG(("Playlist(%p{%s})::RemoveItem(%p{%s})\n", this, GetURL().getObjName().cdata(), item, item->GetPlayable().GetURL().getObjName().cdata()));
  Mutex::Lock lock(Mtx);
  RemoveEntry((Entry*)item);
  InfoChangeFlags |= IF_Other;
  // update tech info
  LoadInfoAsync(InfoValid & IF_Tech);
  // raise InfoChange event?
  DEBUGLOG(("Playlist::RemoveItem: before raiseinfochange\n"));
  RaiseInfoChange();
  DEBUGLOG(("Playlist::RemoveItem: after raiseinfochange\n"));
}


/****************************************************************************
*
*  class PlayFolder
*
****************************************************************************/

PlayFolder::PlayFolder(const char* url, const TECH_INFO* ca_tech, const META_INFO* ca_meta)
: PlayableCollection(url, ca_tech, ca_meta)
{ DEBUGLOG(("PlayFolder(%p)::PlayFolder(...)\n", this));
  ParseQueryParams();
}

void PlayFolder::ParseQueryParams()
{ DEBUGLOG(("PlayFolder(%p)::ParseQueryParams()\n", this));
  xstring params = GetURL().getParameter();
  if (params.length() == 0)
    return;
  const char* cp = params.cdata() +1;
  while (*cp)
  { const char* cp2 = strchr(cp, '&');
    if (cp2 == 0)
      cp2 = params.cdata() + params.length(); // = cp + strlen(cp);
    xstring arg(cp, cp2-cp);
    if (arg.startsWithI("pattern="))
      Pattern.assign(arg.cdata() +8);
    else if (arg.startsWithI("recursive"))
      Recursive = true;
    else
      DEBUGLOG(("PlayFolder::ParseQueryParams: invalid option %s\n", arg.cdata()));
   
    cp = cp2 +1;
  }
  DEBUGLOG(("PlayFolder::ParseQueryParams: %s %u\n", Pattern ? Pattern.cdata() : "<null>", Recursive));
}

bool PlayFolder::LoadInfoCore()
{ DEBUGLOG(("PlayFolder(%p{%s})::LoadInfoCore()\n", this, GetURL().getObjName().cdata()));
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
  APIRET rc = DosFindFirst(name, &hdir, Recursive ? FILE_NORMAL : FILE_ARCHIVED|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY,
    &result, sizeof result, &count, FIL_STANDARD);
  while (rc == 0)
  { // add files
    for (FILEFINDBUF3* fp = (FILEFINDBUF3*)result; count--; ((char*&)fp) += fp->oNextEntryOffset)
    { DEBUGLOG(("PlayFolder::LoadInfoCore - \n", name.cdata(), rc));
        AppendEntry(CreateEntry(fp->achName));
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

