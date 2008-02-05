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

#define INCL_WIN
#define INCL_BASE
#include "plugman.h"
#include "controller.h"
#include "properties.h"
#include "pm123.h"

#include <cpp/mutex.h>
#include <cpp/xstring.h>


/****************************************************************************
*
*  class SongIterator
*
****************************************************************************/

const SongIterator::Offsets SongIterator::ZeroOffsets(1, 0);


SongIterator::SongIterator()
: Callstack(8)
{ DEBUGLOG(("SongIterator(%p)::SongIterator()\n", this));
  // Always create a top level entry for Current.
  Callstack.append() = new CallstackEntry();
}

SongIterator::SongIterator(const SongIterator& r)
: Root(r.Root),
  Callstack(r.Callstack.size() > 4 ? r.Callstack.size() + 4 : 8),
  Exclude(r.Exclude)
{ DEBUGLOG(("SongIterator(%p)::SongIterator(&%p)\n", this, &r));
  // copy callstack
  for (const CallstackEntry*const* ppce = r.Callstack.begin(); ppce != r.Callstack.end(); ++ppce)
    Callstack.append() = new CallstackEntry(**ppce);
}

SongIterator::~SongIterator()
{ DEBUGLOG(("SongIterator(%p)::~SongIterator()\n", this));
  // destroy callstack entries
  for (CallstackEntry** p = Callstack.begin(); p != Callstack.end(); ++p)
    delete *p;
}

void SongIterator::Swap(SongIterator& r)
{ Root.swap(r.Root);
  Callstack.swap(r.Callstack);
  Exclude.swap(r.Exclude);
}

/*#ifdef DEBUG
void SongIterator::DebugDump() const
{ DEBUGLOG(("SongIterator(%p{%p, {%u }, {%u }})::DebugDump()\n", this, &*Root, Callstack.size(), Exclude.size()));
  for (const CallstackEntry*const* ppce = Callstack.begin(); ppce != Callstack.end(); ++ppce)
    DEBUGLOG(("SongIterator::DebugDump %p{%i, %g, %p}\n", *ppce, (*ppce)->Index, (*ppce)->Offset, &*(*ppce)->Item));
  Exclude.DebugDump();
}
#endif*/

void SongIterator::SetRoot(PlayableCollection* pc)
{ Root = pc;
  Reset();
}

PlayableInstance* SongIterator::GetCurrent() const
{ return Callstack[Callstack.size()-1]->Item;
}

SongIterator::Offsets SongIterator::GetOffset() const
{ return *Callstack[Callstack.size()-1];
}

void SongIterator::Enter()
{ DEBUGLOG(("SongIterator::Enter()\n"));
  ASSERT(GetCurrent() != NULL);
  ASSERT(GetCurrent()->GetPlayable()->GetFlags() & Playable::Enumerable);
  Playable* pp = GetCurrent()->GetPlayable();
  Exclude.get(pp) = pp;
  Callstack.append() = new CallstackEntry();
}

void SongIterator::Leave()
{ DEBUGLOG(("SongIterator::Leave()\n"));
  ASSERT(Callstack.size() > 1); // Can't remove the last item.
  delete Callstack.erase(Callstack.end()-1);
  RASSERT(Exclude.erase(GetCurrent()->GetPlayable()));
}

bool SongIterator::SkipQ()
{ return IsInCallstack(GetCurrent()->GetPlayable());
}

PlayableCollection* SongIterator::GetList() const
{ return Callstack.size() > 1 ? (PlayableCollection*)Callstack[Callstack.size()-2]->Item->GetPlayable() : &*Root;
}

SongIterator::Offsets SongIterator::TechFromPlayable(PlayableCollection* pc)
{ Mutex::Lock lck(pc->Mtx);
  const PlayableCollection::CollectionInfo& ci = pc->GetCollectionInfo(Exclude);
  return Offsets(ci.Items, ci.Songlength);
} 
SongIterator::Offsets SongIterator::TechFromPlayable(Playable* pp)
{ if (pp->GetFlags() & Playable::Enumerable)
    return TechFromPlayable((PlayableCollection*)pp);
  // Song => use tech info
  return Offsets(1, pp->GetInfo().tech->songlength);
} 

bool SongIterator::PrevNextCore(int dir)
{ DEBUGLOG(("SongIterator(%p)::PrevNextCore(%i) - %u\n", this, dir, Callstack.size()));
  ASSERT(dir == -1 || dir == 1);
  for (;;)
  { CallstackEntry* pce = Callstack[Callstack.size()-1];
    int_ptr<PlayableInstance> pi = dir < 0 ? GetList()->GetPrev(pce->Item) : GetList()->GetNext(pce->Item);
    DEBUGLOG(("SongIterator::PrevNextCore - {%i, %g, %p} @%p\n", pce->Index, pce->Offset, &*pce->Item, &*pi));
    if (pi == NULL)
    { // store new item     
      pce->Item = pi;
      // end of list => leave if there is something to leave
      if (Callstack.size() == 1)
      { DEBUGLOG(("SongIterator::PrevNextCore - NULL\n"));
        pce->Index = -1;
        pce->Offset = -1;
        return false;
      }
      Leave();
    } else
    { // update offsets
      if (pce->Item != NULL)
      { // relative offsets
        const Offsets& info = TechFromPlayable((dir < 0 ? pi : pce->Item)->GetPlayable());
        DEBUGLOG(("SongIterator::PrevNextCore - relative offset - {%i, %g}\n", info.Index, info.Offset));
        pce->Index += dir * info.Index;
        if (pce->Offset != -1)
        { // apply change
          if (pce->Offset >= 0)
          { if (info.Offset >= 0)
              pce->Offset += dir * info.Offset;
            else
              pce->Offset = -1;
          }
        } 
      } else
      { // calculate offsets from parent
        DEBUGLOG(("SongIterator::PrevNextCore - parent offset\n"));
        // parent offsets, slicing!
        (Offsets&)*pce = Callstack.size() > 1 ? (const Offsets&)*Callstack[Callstack.size()-2] : ZeroOffsets;
        // calc from the end?
        if (dir < 0)
        { // += length(parent) - length(new)
          DEBUGLOG(("SongIterator::PrevNextCore - reverse parent offset\n"));
          const Offsets& pinfo = TechFromPlayable(GetList());
          const Offsets& iinfo = TechFromPlayable(pi->GetPlayable());
          pce->Index += pinfo.Index - iinfo.Index;
          if (pce->Offset >= 0)
          { if (pinfo.Offset >= 0 && iinfo.Offset >= 0)
              pce->Offset += pinfo.Offset - iinfo.Offset;
            else
              pce->Offset = -1;
          }
        }
      }
 
      // store new item     
      pce->Item = pi;
      // item found => check wether it is enumerable
      if (!(pi->GetPlayable()->GetFlags() & Playable::Enumerable))
      { DEBUGLOG(("SongIterator::PrevNextCore - {%i, %g, %p{%p{%s}}}\n", pce->Index, pce->Offset, &*pi, pi->GetPlayable(), pi->GetPlayable()->GetURL().getShortName().cdata()));
        return true;
      } else if (!SkipQ()) // skip objects in the call stack
      { pi->GetPlayable()->EnsureInfo(Playable::IF_Other);
        Enter();
      }
    }
  }
}

void SongIterator::Reset()
{ DEBUGLOG(("SongIterator(%p)::Reset()\n", this));
  Exclude.clear();
  if (Root)
    Exclude.append() = Root;
  // destroy callstack entries.
  for (CallstackEntry** p = Callstack.begin(); p != Callstack.end(); ++p)
    delete *p;
  Callstack.clear();
  // Always create a top level entry for Current.
  Callstack.append() = new CallstackEntry();
}

bool operator==(const SongIterator& l, const SongIterator& r)
{ if (l.Root != r.Root)
    return false;
  if (l.Root == NULL)
    return true;
  // compare callstack
  if (l.Callstack.size() != r.Callstack.size())
    return false;
  const SongIterator::CallstackEntry*const* lcpp = l.Callstack.begin();
  const SongIterator::CallstackEntry*const* rcpp = r.Callstack.begin();
  while (lcpp != l.Callstack.end())
  { if ((*lcpp)->Item != (*rcpp)->Item) // compare by instance
      return false;
    ++lcpp;
    ++rcpp;
  }
  return true; 
}


/****************************************************************************
*
*  class Ctrl
*
****************************************************************************/

int_ptr<Song>        Ctrl::CurrentSong;
vector<Ctrl::PrefetchEntry> Ctrl::PrefetchList(10);
T_TIME               Ctrl::Location  = 0.;
bool                 Ctrl::Playing   = false;
bool                 Ctrl::Paused    = false;
DECFASTMODE          Ctrl::Scan      = DECFAST_NORMAL_PLAY;
double               Ctrl::Volume    = 1.;
xstring              Ctrl::Savename;
bool                 Ctrl::Shuffle   = false;
bool                 Ctrl::Repeat    = false;
bool                 Ctrl::EqEnabled = false;
Ctrl::EQ_Data        Ctrl::EqData    = { { {1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1} } };

queue<Ctrl::ControlCommand*> Ctrl::Queue;
TID                  Ctrl::WorkerTID = 0;

Ctrl::PlayStatus     Ctrl::Status;

volatile unsigned    Ctrl::Pending   = Ctrl::EV_None;
event<const Ctrl::EventFlags> Ctrl::ChangeEvent;

delegate<void, const dec_event_args>        Ctrl::DecEventDelegate(dec_event, &Ctrl::DecEventHandler);
delegate<void, const OUTEVENTTYPE>          Ctrl::OutEventDelegate(out_event, &Ctrl::OutEventHandler);
delegate<void, const Playable::change_args> Ctrl::CurrentSongDelegate(&Ctrl::CurrentSongEventHandler);

const SongIterator::CallstackType Ctrl::EmptyStack(1);


int_ptr<Song> Ctrl::GetCurrentSong()
{ DEBUGLOG(("Ctrl::GetCurrentSong() - %p, %u\n", &*CurrentSong, PrefetchList.size()));
  CritSect cs();
  if (PrefetchList.size() == 0)
    return CurrentSong;
  PlayableInstance* pi = *Current()->Iter;
  return pi ? (Song*)pi->GetPlayable() : NULL;
}


bool Ctrl::SetFlag(bool& flag, Op op)
{ switch (op)
  {default:
    return false;
    
   case Op_Set:
    if (flag)
      return false;
    break;
   case Op_Clear:
   case Op_Reset:
    if (!flag)
      return false;
   case Op_Toggle:;
  }
  flag = !flag;
  return true;
}

void Ctrl::SetVolume()
{ double volume = Volume;
  if (Scan)
    volume *= 3./5.;
  out_set_volume(volume);
}

ULONG Ctrl::DecoderStart(Song* pp, T_TIME offset)
{ DEBUGLOG(("Ctrl::DecoderStart(%p)\n", pp));
  pp->EnsureInfo(Playable::IF_Other);
  SetVolume();
  dec_eq(EqEnabled ? EqData.bandgain[0] : NULL);
  dec_save(Savename);
  dec_fast(Scan);

  // TODO: offset
  ULONG rc = dec_play( pp->GetURL(), pp->GetDecoder(), offset, Location );
  if (rc != 0)
    return rc;

  // TODO: CRAP!
  while (dec_status() == DECODER_STARTING)
  { DEBUGLOG(("Ctrl::DecoderStart - waiting for Spinlock\n"));
    DosSleep( 1 );
  }
  DEBUGLOG(("Ctrl::DecoderStart - completed\n"));
  return 0;
}

void Ctrl::DecoderStop()
{ DEBUGLOG(("Ctrl::DecoderStop()\n"));
  // stop decoder
  dec_stop();

  // TODO: CRAP! => we should disconnect decoder instead 
  while (dec_status() == DECODER_PLAYING)
  { DEBUGLOG(("Ctrl::DecoderStop - waiting for Spinlock\n"));
    DosSleep( 1 );
  }
  DEBUGLOG(("Ctrl::DecoderStop - completed\n"));
}

ULONG Ctrl::OutputStart(Playable* pp)
{ DEBUGLOG(("Ctrl::OutputStart(%p)\n", pp));
  pp->EnsureInfo(Playable::IF_Format|Playable::IF_Other);
  FORMAT_INFO2 format = *pp->GetInfo().format;
  ULONG rc = out_setup( &format, pp->GetURL() );
  DEBUGLOG(("Ctrl::OutputStart: after setup %d %d - %d\n", format.samplerate, format.channels, rc));
  return rc;
}

void Ctrl::OutputStop()
{ DEBUGLOG(("Ctrl::OutputStop()\n"));
  // Clear prefetch list
  PrefetchClear(true);
  // close output
  out_close();
  // reset offset
  if (PrefetchList.size())
    Current()->Offset = 0;
}

void Ctrl::UpdateStackUsage(const SongIterator::CallstackType& oldstack, const SongIterator::CallstackType& newstack)
{ DEBUGLOG(("Ctrl::UpdateStackUsage({%u }, {%u })\n", oldstack.size(), newstack.size()));
  SongIterator::CallstackEntry*const* oldppi = oldstack.begin();
  SongIterator::CallstackEntry*const* newppi = newstack.begin();
  // skip identical part
  // TODO? a more optimized approach may work on the sorted exclude lists.
  while (oldppi != oldstack.end() && newppi != newstack.end() && &*(*oldppi)->Item == &*(*newppi)->Item)
  { DEBUGLOG(("Ctrl::UpdateStackUsage identical - %p == %p\n", &*(*oldppi)->Item, &*(*newppi)->Item));
    ++oldppi;
    ++newppi;
  }
  // reset usage flags of part of old stack
  SongIterator::CallstackEntry*const* ppi = oldstack.end();
  while (ppi != oldppi)
    if ((*--ppi)->Item)
      (*ppi)->Item->SetInUse(false); 
  // set usage flags of part of the new stack
  ppi = newstack.end();
  while (ppi != newppi)
    if ((*--ppi)->Item)
      (*ppi)->Item->SetInUse(true); 
}

bool Ctrl::SkipCore(SongIterator& si, int count, bool relative)
{ DEBUGLOG(("Ctrl::SkipCore(%i, %u)\n", count, relative));
  if (relative)
  { if (count < 0)
    { // previous    
      do
      { if (!si.Prev())
          return false;
      } while (++count);
    } else
    { // next
      do
      { if (!si.Next())
          return false;
      } while (--count);
    }
  } else
  { si.Reset();
    do
    { if (!si.Next())
        return false;
    } while (--count);
  }
  return true;
}

void Ctrl::PrefetchClear(bool keep)
{ DEBUGLOG(("Ctrl::PrefetchClear(%u)\n", keep));
  CritSect cs();
  while (PrefetchList.size() > keep) // Hack: keep = false deletes all items while keep = true kepp the first item.
    delete PrefetchList.erase(PrefetchList.end()-1);
}

void Ctrl::CheckPrefetch(double pos)
{ DEBUGLOG(("Ctrl::CheckPrefetch(%g)\n", pos));
  if (PrefetchList.size()) 
  { size_t n = 1;
    // Since the item #1 is likely to compare less than CurrentSongTime a linear search is faster than a binary search.
    while (n < PrefetchList.size() && pos >= PrefetchList[n]->Offset)
      ++n;
    --n;
    DEBUGLOG(("Ctrl::CheckPrefetch %g, %g -> %u\n", pos, Current()->Offset, n));
    if (n)
    { // At least one prefetched item has been played completely.
      CurrentSongDelegate.detach();
      UpdateStackUsage(Current()->Iter.GetCallstack(), PrefetchList[n]->Iter.GetCallstack());
      Current()->Iter->GetPlayable()->InfoChange += CurrentSongDelegate;
      // Set events
      InterlockedOr(Pending, EV_Song|EV_Tech|EV_Meta);
      // Cleanup prefetch list
      CritSect cs;
      do
        delete PrefetchList.erase(--n);
      while (n);
    }
  }
}

void Ctrl::DecEventHandler(void*, const dec_event_args& args)
{ DEBUGLOG(("Ctrl::DecEventHandler(, {%i, %p})\n", args.type, args.param));
  switch (args.type)
  {case DECEVENT_PLAYSTOP:
    // Well, same as on play error.
   case DECEVENT_PLAYERROR:
    // Decoder error => next, please (if any)
    PostCommand(MkDecStop());
    break;
   /* currently unused
   case DECEVENT_SEEKSTOP:
    break;
   case DEVEVENT_CHANGETECH:
    break;    
   case DECEVENT_CHANGEMETA:
    break; */
   default: // avoid warnings
    break;
  }
}

void Ctrl::OutEventHandler(void*, const OUTEVENTTYPE& event)
{ DEBUGLOG(("Ctrl::OutEventHandler(, %i)\n", event));
  switch (event)
  {case OUTEVENT_END_OF_DATA:
    // Well, same as on play error.
   case OUTEVENT_PLAY_ERROR:
    // output error => full stop
    PostCommand(MkPlayStop(Ctrl::Op_Clear));
    break;
   default: // avoid warnings
    break;
  }
}

void Ctrl::CurrentSongEventHandler(void*, const Playable::change_args& args)
{ DEBUGLOG(("Ctrl::CurrentSongEventHandler(, {%p{%s}, %x})\n", &args.Instance, args.Instance.GetURL().cdata(), args.Flags));
  if (GetCurrentSong() != &args.Instance)
    return; // too late...
  EventFlags events = EV_Tech * ((args.Flags & Playable::IF_Tech) != 0)
                    | EV_Meta * ((args.Flags & Playable::IF_Meta) != 0);
  if (events)
  { InterlockedOr(Pending, events);
    PostCommand(MkNop());
  }  
}

/* Suspends or resumes playback of the currently played file. */
Ctrl::RC Ctrl::MsgPause(Op op)
{ DEBUGLOG(("Ctrl::MsgPause(%x) - %u\n", op, Scan)); 
  if (!decoder_playing())
    return op & Op_Set ? RC_NotPlaying : RC_OK;

  if (SetFlag(Paused, op))
  { out_pause(Paused);
    InterlockedOr(Pending, EV_Pause);
  }
  return RC_OK;
}

/* change scan mode logically */
Ctrl::RC Ctrl::MsgScan(Op op)
{ DEBUGLOG(("Ctrl::MsgScan(%x) - %u\n", op, Scan)); 
  if (op & ~7)
    return RC_BadArg;
  if (!decoder_playing())
    return op & Op_Set ? RC_NotPlaying : RC_OK;

  static const DECFASTMODE opmatrix[8][3] =
  { {DECFAST_NORMAL_PLAY, DECFAST_NORMAL_PLAY, DECFAST_NORMAL_PLAY},
    {DECFAST_FORWARD,     DECFAST_FORWARD,     DECFAST_FORWARD    },
    {DECFAST_NORMAL_PLAY, DECFAST_NORMAL_PLAY, DECFAST_REWIND     },
    {DECFAST_FORWARD,     DECFAST_NORMAL_PLAY, DECFAST_FORWARD    },
    {DECFAST_NORMAL_PLAY, DECFAST_FORWARD,     DECFAST_REWIND     },
    {DECFAST_REWIND,      DECFAST_REWIND,      DECFAST_REWIND     },
    {DECFAST_NORMAL_PLAY, DECFAST_FORWARD,     DECFAST_NORMAL_PLAY},
    {DECFAST_REWIND,      DECFAST_REWIND,      DECFAST_NORMAL_PLAY}
  };
  DECFASTMODE newscan = opmatrix[op][Scan];
  // Check for NOP.
  if (Scan == newscan)
    return RC_OK;
  // => Decoder
  if (dec_fast(newscan) != 0)
    return RC_DecPlugErr;
  else if (cfg.trash)
    // Going back in the stream to what is currently playing.
    // TODO: discard prefetch buffer.
    dec_jump(out_playing_pos());
  // Update event flags
  if ((Scan & DECFAST_FORWARD) != (newscan & DECFAST_FORWARD))
    InterlockedOr(Pending, EV_Forward);
  if ((Scan & DECFAST_REWIND) != (newscan & DECFAST_REWIND))
    InterlockedOr(Pending, EV_Rewind);
  Scan = newscan;
  SetVolume();
  return RC_OK;
}

Ctrl::RC Ctrl::MsgVolume(double volume, bool relative)
{ DEBUGLOG(("Ctrl::MsgVolume(%g, %u) - %g\n", volume, relative, Volume));
  volume += Volume * relative;
  // Limits
  if (volume < 0)
    volume = 0;
  else if (volume > 1)
    volume = 1;

  if (volume != Volume)
  { Volume = volume;
    SetVolume();
    InterlockedOr(Pending, EV_Volume);
  }
  return RC_OK;
}

/* change play/stop status */
Ctrl::RC Ctrl::MsgPlayStop(Op op)
{ DEBUGLOG(("Ctrl::MsgPlayStop(%x) - %u\n", op, Playing));
  if (!SetFlag(Playing, op))
    return RC_OK;

  if (Playing)
  { // start playback
    Song* pp = GetCurrentSong();
    if (pp == NULL)
      return RC_NoSong;
    if (OutputStart(pp) != 0)
      return RC_OutPlugErr;

    if (DecoderStart(pp) != 0)
    { OutputStop();
      return RC_DecPlugErr;
    }
    Playing = true;
  
  } else
  { // stop playback
    MsgPause(Op_Clear);
    MsgScan(Op_Reset);
    Location = 0;

    DecoderStop();
    OutputStop();
    
    while (out_playing_data())
    { DEBUGLOG(("Ctrl::MsgPlayStop - Spinlock\n"));
      DosSleep(1);
    }

    Playing = false;
  }
  InterlockedOr(Pending, EV_PlayStop);
  
  return RC_OK;
}

Ctrl::RC Ctrl::MsgNavigate(const xstring& iter, T_TIME loc, int flags)
{ DEBUGLOG(("Ctrl::MsgNavigate(%s, %g, %x)\n", iter.cdata(), loc, flags));
  if (!GetCurrentSong())
    return RC_NoSong;
  // TODO: the whole iterator stuff is missing
  // TODO: prefetch list?
  Location = loc;
  if (Playing)
  { if ( dec_jump( loc ) != 0 )
      return RC_DecPlugErr; 
  }
  return RC_OK;
}

Ctrl::RC Ctrl::MsgSkip(int count, bool relative)
{ DEBUGLOG(("Ctrl::MsgSkip(%i, %u) - %u\n", count, relative, decoder_playing()));
  if (PrefetchList.size() == 0)
    return RC_NoList;
  BOOL decoder_was_playing = decoder_playing();
  // some checks
  if (relative)
  { if (count == 0)
      return RC_OK;
  } else
  { // absolute mode
    if (count < 0)
      return RC_BadArg;
    /* TODO: ...
    if (Current.GetStatus().CurrentItem == count)
      return RC_OK;*/
  }

  if (decoder_was_playing)
  { DecoderStop();
    out_trash(); // discard buffers
    PrefetchClear(true);
  }
  Location = 0;
  // Navigation
  SongIterator si = Current()->Iter; // work on a temporary object => copy constructor
  if (!SkipCore(si, count, relative))
    return RC_EndOfList;
  // commit
  { CritSect cs;
    // deregister current song delegate
    CurrentSongDelegate.detach();
    // swap iterators
    Current()->Offset = 0;
    Current()->Iter.Swap(si);
  }
  // Events
  UpdateStackUsage(si.GetCallstack(), Current()->Iter.GetCallstack());
  InterlockedOr(Pending, EV_Song|EV_Tech|EV_Meta);
  // track updates
  if (*Current()->Iter)
    Current()->Iter->GetPlayable()->InfoChange += CurrentSongDelegate;

  // restart decoder immediately?
  if (decoder_was_playing)
  { Song* pp = GetCurrentSong();
    if (pp != NULL && DecoderStart(pp) != 0)
    { OutputStop();
      Playing = false;
      InterlockedOr(Pending, EV_PlayStop);
      return RC_DecPlugErr;
    }
  }
  return RC_OK;
}

/* Loads Playable object to player. */
Ctrl::RC Ctrl::MsgLoad(const xstring& url)
{ DEBUGLOG(("Ctrl::MsgLoad(%s)\n", url.cdata()));

  // always stop
  MsgPlayStop(Op_Reset);

  // detach
  CurrentSongDelegate.detach();
  if (CurrentSong)
  { CurrentSong->SetInUse(false);
    CurrentSong = NULL;
  }
  if (PrefetchList.size())
  { UpdateStackUsage(Current()->Iter.GetCallstack(), EmptyStack);
    Current()->Iter.GetRoot()->SetInUse(false);
    PrefetchClear(false);
  }
  
  if (url)
  { int_ptr<Playable> play = Playable::GetByURL(url);
    // Only load items that have a minimum of well known properties.
    // In case of enumerable items the content is required, in case of songs the decoder.
    // Both is related to IF_Other. The other informations are prefetched too.
    play->EnsureInfo(Playable::IF_Other);
    play->EnsureInfoAsync(Playable::IF_All);
    if (play->GetFlags() & Playable::Enumerable)
    { { CritSect cs();
        PrefetchList.append() = new PrefetchEntry();
        Current()->Iter.SetRoot((PlayableCollection*)&*play);
      }
      play->SetInUse(true);
      // Move always to the first element.
      if (Current()->Iter.Next())
      { UpdateStackUsage(EmptyStack, Current()->Iter.GetCallstack());
        // track changes
        Current()->Iter->GetPlayable()->InfoChange += CurrentSongDelegate;
      }
    } else
    { CurrentSong = (Song*)&*play;
      play->SetInUse(true);
      // track changes
      CurrentSong->InfoChange += CurrentSongDelegate;
    }
  }
  InterlockedOr(Pending, EV_Root|EV_Song|EV_Tech|EV_Meta);
  DEBUGLOG(("Ctrl::MsgLoad - attached\n"));

  return RC_OK;
}

/* saving the currently played stream. */
Ctrl::RC Ctrl::MsgSave(const xstring& filename)
{ DEBUGLOG(("Ctrl::MsgSave(%s)\n", filename.cdata()));

  if (Savename == filename)
    return RC_OK;
  InterlockedOr(Pending, EV_Savename);
  Savename = filename;

  if (decoder_playing())
    dec_save(Savename);

  return RC_OK;
}

Ctrl::RC Ctrl::MsgEqualize(const EQ_Data* data)
{ DEBUGLOG(("Ctrl::MsgEqualize(%p)\n", data));
  if (data)
  { // set EQ
    if (EqEnabled && memcmp(data, &EqData, sizeof(EQ_Data)) == 0)
      return RC_OK;
    if (decoder_playing() && dec_eq(data->bandgain[0]))
      return RC_DecPlugErr;
    EqData = *data;
    EqEnabled = true;
  } else
  { // disable EQ
    if (!EqEnabled)
      return RC_OK;
    if (decoder_playing() && dec_eq(NULL))
      return RC_DecPlugErr;
    EqEnabled = false;
  }
  InterlockedOr(Pending, EV_Equalize);
  return RC_OK;
}

Ctrl::RC Ctrl::MsgStatus()
{ DEBUGLOG(("Ctrl::MsgStatus()\n"));
  Status.CurrentSongTime = Location; // implicit else of the if below
  if (Playing)
  { Status.CurrentSongTime = out_playing_pos();
    // Check whether the output played a prefetched item completely.
    CheckPrefetch(Status.CurrentSongTime);
  }
  
  if (CurrentSong)
  { // root is a song
    CurrentSong->EnsureInfo(Playable::IF_Tech);
    Status.TotalSongTime   = CurrentSong->GetInfo().tech->songlength;
    Status.CurrentItem     = -1;
    Status.TotalItems      = -1;
    Status.CurrentTime     = -1;
    Status.TotalTime       = -1;
  } else if (PrefetchList.size())
  { // root is enumerable
    SongIterator& si = Current()->Iter;
    if (Playing)
      Status.CurrentSongTime -= Current()->Offset; // relocate playing position
    Status.TotalSongTime   = *si ? si->GetPlayable()->GetInfo().tech->songlength : -1;
    SongIterator::Offsets off = si.GetOffset();
    Status.CurrentItem     = off.Index;
    Status.TotalItems      = si.GetRoot()->GetInfo().tech->num_items;
    Status.CurrentTime     = off.Offset;
    Status.TotalTime       = si.GetRoot()->GetInfo().tech->songlength;
  } else
    // no root
    return RC_NoSong;
  return RC_OK; 
}

// The decoder completed decoding...
Ctrl::RC Ctrl::MsgDecStop()
{ DEBUGLOG(("Ctrl::MsgDecStop()\n"));
  if (!Playing)
    return RC_NotPlaying;

  if (PrefetchList.size() == 0)
  { // Song => Stop
   eol:
    out_flush();
    // Continue at OUTEVENT_END_OF_DATA
    return RC_OK;
  }

  int dir = Scan == DECFAST_REWIND ? -1 : 1;

  PrefetchEntry* pep = new PrefetchEntry(Current()->Offset + dec_maxpos(), Current()->Iter);
  pep->Offset += dec_maxpos();
  DecoderStop();
  Location = 0;

  // Navigation
  Song* pp = NULL;
  { do
    { if (!SkipCore(pep->Iter, dir, true))
      { delete pep;
        goto eol; // end of list => same as end of song
      }

      pp = (Song*)pep->Iter.GetCurrent()->GetPlayable();
      pp->EnsureInfo(Playable::IF_Other);
      // skip invalid
    } while (pp->GetStatus() == STA_Invalid);
    // store result
    CritSect cs;
    PrefetchList.append() = pep;
  }

  // start decoder for the prefetched item
  DEBUGLOG(("Ctrl::MsgDecStop playing %s with offset %g\n", pp->GetURL().cdata(), pep->Offset));
  if (DecoderStart(pp, pep->Offset) != 0)
  { // TODO: we should continue with the next song, and remove the current one from the prefetch list.
    OutputStop();
    Playing = false;
    InterlockedOr(Pending, EV_PlayStop);
    return RC_DecPlugErr;
  }
  
  // In rewind mode we continue to rewind from the end of the prevois song.
  // TODO: Location problem.
  return RC_OK; 
}


void TFNENTRY ControllerWorkerStub(void*)
{ Ctrl::Worker();
}

void Ctrl::Worker()
{ HAB hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(hab, 0);
  for(;;)
  { DEBUGLOG(("Ctrl::Worker() looking for work\n"));
    queue<ControlCommand*>::Reader rdr(Queue);
    ControlCommand* qp = rdr;
    if (qp == NULL)
      break; // deadly pill

    bool fail = false;
    do
    { DEBUGLOG(("Ctrl::Worker received message: %p{%i, %s, %g, %x, %p, %p} - %u\n",
      qp, qp->Cmd, qp->StrArg ? qp->StrArg.cdata() : "<null>", qp->NumArg, qp->Flags, qp->Callback, qp->Link, fail));
      if (fail)
        qp->Flags = RC_SubseqError;
      else
        // Do the work
        switch (qp->Cmd)
        {default:
          qp->Flags = RC_BadArg;
          break;
          
         case Cmd_Load:
          qp->Flags = MsgLoad(qp->StrArg);
          break;
          
         case Cmd_Skip:
          qp->Flags = MsgSkip((int)qp->NumArg, qp->Flags & 0x01);
          break;
          
         case Cmd_Navigate:
          qp->Flags = MsgNavigate(qp->StrArg, qp->NumArg, qp->Flags);
          break;
         
         case Cmd_PlayStop:
          qp->Flags = MsgPlayStop((Op)qp->Flags);
          break;
         
         case Cmd_Pause:
          qp->Flags = MsgPause((Op)qp->Flags);
          break;
          
         case Cmd_Scan:
          qp->Flags = MsgScan((Op)qp->Flags);
          break;
          
         case Cmd_Volume:
          qp->Flags = MsgVolume(qp->NumArg, qp->Flags & 0x01);
          break;
          
         case Cmd_Save:
          qp->Flags = MsgSave(qp->StrArg);
          break;
          
         case Cmd_Equalize:
          qp->Flags = MsgEqualize((EQ_Data*)qp->PtrArg);
          break;
          
         case Cmd_Status:
          qp->Flags = MsgStatus();
          qp->PtrArg = &Status;
          break;
         
         case Cmd_DecStop:
          qp->Flags = MsgDecStop();
          break;
         
         case Cmd_Nop:
          break;
        }
      DEBUGLOG(("Ctrl::Worker message %i completed, rc = %i\n", qp->Cmd, qp->Flags));
      fail = qp->Flags != RC_OK;

      // link
      ControlCommand* qpl = qp->Link;
      // cleanup
      if (qp->Callback)
        (*qp->Callback)(qp);
      else
        delete qp;

      qp = qpl;
    } while (qp);
      
    // raise control event
    EventFlags events = (EventFlags)InterlockedXch(Pending, EV_None);
    DEBUGLOG(("Ctrl::Worker raising events %x\n", events));
    if (events)
      ChangeEvent(events);
  }
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}


// Public interface

void Ctrl::Init()
{ DEBUGLOG(("Ctrl::Init()\n"));
  WorkerTID = _beginthread(&ControllerWorkerStub, NULL, 262144, NULL);
  ASSERT((int)WorkerTID != -1);
}

void Ctrl::Uninit()
{ DEBUGLOG(("Ctrl::Uninit()\n"));
  { Mutex::Lock l(Queue.Mtx);
    Queue.Purge();
    PostCommand(MkLoad(xstring()));
    PostCommand(NULL);
  }
  if (WorkerTID != 0)
    wait_thread_pm(amp_player_hab(), WorkerTID, 30000);
  // Now delete everything
  PrefetchClear(false);
  DEBUGLOG(("CtrlUninit complete\n"));
}

static void SendCallbackFunc(Ctrl::ControlCommand* cmd)
{ ((Event*)cmd->User)->Set();
}

Ctrl::ControlCommand* Ctrl::SendCommand(ControlCommand* cmd)
{ DEBUGLOG(("Ctrl::SendCommand(%p{%i, ...})\n", cmd, cmd ? cmd->Cmd : -1));
  Event callback;
  cmd->User = &callback;
  cmd->Callback = &SendCallbackFunc;
  PostCommand(cmd);
  callback.Wait();
  return cmd;
}

