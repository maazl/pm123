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

#include <inimacro.h>
#include <cpp/mutex.h>
#include <cpp/xstring.h>

#include <limits.h>


/* Ini file stuff, a bit dirty */

struct ctrl_state
{ double  volume;        // Position of the volume slider
  BOOL    shf;           // The state of the "Shuffle" button.
  BOOL    rpt;           // The state of the "Repeat" button.
  xstring current_root;  // The currently loaded root.
  xstring current_iter;  // The current location within the root.
  BOOL    was_playing;   // Restart playback on start-up
  ctrl_state() : volume(-1), shf(FALSE), rpt(FALSE) {}
};


/****************************************************************************
*
*  class Ctrl
*
****************************************************************************/

void Ctrl::ControlCommand::Destroy()
{ if (Link)
    Link->Destroy();
  delete this;
}

vector<Ctrl::PrefetchEntry> Ctrl::PrefetchList(10);
Mutex                Ctrl::PLMtx;
bool                 Ctrl::Playing   = false;
bool                 Ctrl::Paused    = false;
DECFASTMODE          Ctrl::Scan      = DECFAST_NORMAL_PLAY;
double               Ctrl::Volume    = 1.;
xstring              Ctrl::Savename;
bool                 Ctrl::Shuffle   = false;
bool                 Ctrl::Repeat    = false;

queue<Ctrl::ControlCommand*> Ctrl::Queue;
TID                  Ctrl::WorkerTID = 0;

volatile unsigned    Ctrl::Pending   = Ctrl::EV_None;
event<const Ctrl::EventFlags> Ctrl::ChangeEvent;

delegate<void, const dec_event_args>        Ctrl::DecEventDelegate(&Ctrl::DecEventHandler);
delegate<void, const OUTEVENTTYPE>          Ctrl::OutEventDelegate(&Ctrl::OutEventHandler);
delegate<void, const Playable::change_args> Ctrl::CurrentSongDelegate(&Ctrl::CurrentSongEventHandler);
delegate<void, const Playable::change_args> Ctrl::CurrentRootDelegate(&Ctrl::CurrentRootEventHandler);
delegate<void, const SongIterator::CallstackEntry> Ctrl::SongIteratorDelegate(&Ctrl::SongIteratorEventHandler);

const SongIterator::CallstackType Ctrl::EmptyStack(1);


int_ptr<Song> Ctrl::GetCurrentSong()
{ DEBUGLOG(("Ctrl::GetCurrentSong() - %u\n", PrefetchList.size()));
  int_ptr<Playable> pp;
  { Mutex::Lock lock(PLMtx);
    if (PrefetchList.size())
      pp = Current()->Iter.GetCurrentItem();
  }
  return pp && !(pp->GetFlags() & Playable::Enumerable) ? (Song*)pp.get() : NULL;
}

int_ptr<PlayableSlice> Ctrl::GetRoot()
{ Mutex::Lock lock(PLMtx);
  return PrefetchList.size() ? Current()->Iter.GetRoot() : NULL;
}

bool Ctrl::IsEnumerable()
{ return PrefetchList.size() && (Current()->Iter.GetRoot()->GetPlayable()->GetFlags() & Playable::Enumerable);
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

ULONG Ctrl::DecoderStart(PlayableSlice* ps, T_TIME offset)
{ DEBUGLOG(("Ctrl::DecoderStart(%p{%p})\n", ps, ps->GetPlayable()));
  ps->GetPlayable()->EnsureInfo(Playable::IF_Other);
  ASSERT(!(ps->GetPlayable()->GetFlags() & Playable::Enumerable));
  SetVolume();
  dec_save(Savename);

  T_TIME start = ps->GetStart() ? ps->GetStart()->GetLocation() : 0;
  T_TIME stop  = ps->GetStop()  ? ps->GetStop() ->GetLocation() : -1;
  if (Scan == DECFAST_REWIND)
  { if (stop > 0)
    { start = stop - 1.; // do not seek to the end, because this will cause problems.
    } else if (ps->GetPlayable()->GetInfo().tech->songlength > 0)
    { start = ps->GetPlayable()->GetInfo().tech->songlength - 1.;
    } else
    { // no songlength => error => undo MsgScan
      Scan = DECFAST_NORMAL_PLAY;
      InterlockedOr(&Pending, EV_Rewind);
    }
    if (start < 0) // Do not hit negative values for very short songs.
      start = 0;
  }

  ULONG rc = dec_play((Song*)ps->GetPlayable(), offset, start, stop);
  if (rc != 0)
    return rc;

  // TODO: I hate this delay with a spinlock.
  int cnt = 0;
  while (dec_status() == DECODER_STARTING)
  { DEBUGLOG(("Ctrl::DecoderStart - waiting for Spinlock\n"));
    DosSleep( ++cnt > 10 );
  }

  if (Scan != DECFAST_NORMAL_PLAY)
    dec_fast(Scan);
  DEBUGLOG(("Ctrl::DecoderStart - completed\n"));
  return 0;
}

void Ctrl::DecoderStop()
{ DEBUGLOG(("Ctrl::DecoderStop()\n"));
  // stop decoder
  dec_stop();

  // TODO: CRAP? => we disconnect decoder instead
  /*int cnt = 0;
  while (dec_status() != DECODER_STOPPED && dec_status() != DECODER_ERROR)
  { DEBUGLOG(("Ctrl::DecoderStop - waiting for Spinlock\n"));
    DosSleep( ++cnt > 10 );
  }*/
  DEBUGLOG(("Ctrl::DecoderStop - completed\n"));
}

ULONG Ctrl::OutputStart(Song* pp)
{ DEBUGLOG(("Ctrl::OutputStart(%p)\n", pp));
  pp->EnsureInfo(Playable::IF_Format|Playable::IF_Other);
  ULONG rc = out_setup( pp );
  DEBUGLOG(("Ctrl::OutputStart: after setup - %d\n", rc));
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

void Ctrl::SetStackUsage(SongIterator::CallstackEntry*const* rbegin, SongIterator::CallstackEntry*const* rend, bool set)
{ while (rend != rbegin)
  { PlayableSlice* ps = (*--rend)->Item;
    if (ps)
      // Depending on the object type this updates the status of a PlayableInstance
      // or only the status of the underlying Playable.
      ps->SetInUse(set);
  }
}

void Ctrl::UpdateStackUsage(const SongIterator::CallstackType& oldstack, const SongIterator::CallstackType& newstack)
{ DEBUGLOG(("Ctrl::UpdateStackUsage({%u }, {%u })\n", oldstack.size(), newstack.size()));
  SongIterator::CallstackEntry*const* oldppi = oldstack.begin();
  SongIterator::CallstackEntry*const* newppi = newstack.begin();
  // skip identical part
  // TODO? a more optimized approach may work on the sorted exclude lists.
  while (oldppi != oldstack.end() && newppi != newstack.end() && (*oldppi)->Item == (*newppi)->Item)
  { DEBUGLOG(("Ctrl::UpdateStackUsage identical - %p == %p\n", (*oldppi)->Item.get(), (*newppi)->Item.get()));
    ++oldppi;
    ++newppi;
  }
  // reset usage flags of part of old stack
  SetStackUsage(oldppi, oldstack.end(), false);
  // set usage flags of part of the new stack
  SetStackUsage(newppi, newstack.end(), true);
}

bool Ctrl::SkipCore(SongIterator& si, int count, bool relative)
{ DEBUGLOG(("Ctrl::SkipCore({%s}, %i, %u)\n", si.Serialize().cdata(), count, relative));
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

void Ctrl::AdjustNext(SongIterator& si)
{ PlayableSlice* ps = si.GetCurrent();
  if (ps == NULL || (ps->GetPlayable()->GetFlags() & Playable::Enumerable))
  { do
    { si.Next();
      ps = si.GetCurrent();
    } while (ps != NULL && ps->GetPlayable()->GetStatus() == Playable::STA_Invalid);
  }
}

Ctrl::RC Ctrl::NavigateCore(SongIterator& si)
{ DEBUGLOG(("Ctrl::NavigateCore({%s}) - %s\n", si.Serialize().cdata(), Current()->Iter.Serialize().cdata()));
  // Check whether the current song has changed?
  int level = si.CompareTo(Current()->Iter);
  DEBUGLOG(("Ctrl::NavigateCore - %i\n", level));
  if (level == 0)
    return RC_OK; // song and location identical => no-op
  ASSERT(level != INT_MIN);
  if (abs(level) > max(si.GetLevel(), Current()->Iter.GetLevel()))
  { DEBUGLOG(("Ctrl::NavigateCore - seek to %f\n", Current()->Iter.GetLocation()));
    // only location is different => seek only
    if (Playing)
    { if (dec_jump(si.GetLocation()) != 0)
        return RC_DecPlugErr;
    }
    Mutex::Lock lock(PLMtx);
    Current()->Iter.Swap(si);
    return RC_OK;
  }
  DEBUGLOG(("Ctrl::NavigateCore - Navigate - %u\n", Playing));
  // Navigate to another item
  if (Playing)
  { DecoderStop();
    out_trash(); // discard buffers
    PrefetchClear(true);
  }
  { // Mutex because Current is modified.
    Mutex::Lock lock(PLMtx);
    // deregister current song delegate
    CurrentSongDelegate.detach();
    // swap iterators
    Current()->Offset = 0;
    Current()->Iter.Swap(si);
  }
  // Events
  UpdateStackUsage(si.GetCallstack(), Current()->Iter.GetCallstack());
  InterlockedOr(&Pending, EV_SongAll);
  // track updates
  PlayableSlice* ps = Current()->Iter.GetCurrent();
  AttachCurrentSong(ps);

  // restart decoder immediately?
  if (Playing && ps)
  { if (DecoderStart(ps, 0) != 0)
    { OutputStop();
      Playing = false;
      InterlockedOr(&Pending, EV_PlayStop);
      return RC_DecPlugErr;
    }
  }
  return RC_OK;
}

void Ctrl::AttachCurrentSong(PlayableSlice* ps)
{ DEBUGLOG(("Ctrl::AttachCurrentSong(%p)\n", ps));
  if (ps)
  { ps->GetPlayable()->EnsureInfoAsync(Playable::IF_Format|Playable::IF_Tech|Playable::IF_Meta|Playable::IF_Phys);
    ps->GetPlayable()->InfoChange += CurrentSongDelegate;
  }
}

void Ctrl::PrefetchClear(bool keep)
{ DEBUGLOG(("Ctrl::PrefetchClear(%u)\n", keep));
  Mutex::Lock lock(PLMtx);
  Ctrl::PrefetchEntry*const* where = PrefetchList.end();
  while (PrefetchList.size() > keep) // Hack: keep = false deletes all items while keep = true kepp the first item.
    delete PrefetchList.erase(--where);
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
      // Set events
      InterlockedOr(&Pending, EV_SongAll);
      // Cleanup prefetch list
      vector<PrefetchEntry> ped(n);
      { Mutex::Lock lock(PLMtx);
        // detach the songiterator event
        SongIteratorDelegate.detach();
        do
          ped.append() = PrefetchList.erase(--n);
        while (n);
        // attach the songiterator delegate to the new head
        Current()->Iter.Change += SongIteratorDelegate;
      }
      // Now keep track of the next entry
      AttachCurrentSong(Current()->Iter.GetCurrent());

      // delete iterators and remove from play queue (if desired)
      Playlist* plp = NULL;
      if (cfg.queue_mode && (Current()->Iter.GetRoot()->GetPlayable()->GetFlags() & Playable::Mutable) == Playable::Mutable)
      { plp = (Playlist*)Current()->Iter.GetRoot()->GetPlayable();
        if (plp->GetURL() != amp_get_default_pl()->GetURL())
          plp = NULL;
      }
      DEBUGLOG(("Ctrl::CheckPrefetch: queue mode %p\n", plp));
      // plp != NULL -> remove items
      n = ped.size();
      do
      { PrefetchEntry* pep = ped[--n];
        if (plp)
        { int_ptr<PlayableInstance> pip = (PlayableInstance*)pep->Iter.GetCallstack()[0]->Item.get();
          DEBUGLOG(("Ctrl::CheckPrefetch: %u\n", pep->Iter.GetCallstack().size()));
          if ( pep->Iter.GetCallstack().size() == 1       // we played an item at the top level
            || !pep->Iter.Next()                          // we played the last item
            || pep->Iter.GetCallstack()[0]->Item != pip ) // we played the last item of a top leven PlayableCollection
            plp->RemoveItem(pip);
        }
        delete pep;
      } while (n);
    }
  }
}

T_TIME Ctrl::FetchCurrentSongTime()
{ DEBUGLOG(("Ctrl::FetchCurrentSongTime() - %u\n", Playing));
  if (Playing)
  { T_TIME time = out_playing_pos();
    // Check whether the output played a prefetched item completely.
    CheckPrefetch(time);
    return time - Current()->Offset; // relocate playing position
  } else
    return Current()->Iter.GetLocation();
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
    PostCommand(MkOutStop());
    break;
   case OUTEVENT_PLAY_ERROR:
    // output error => full stop
    PostCommand(MkPlayStop(Ctrl::Op_Clear));
    break;
   default: // avoid warnings
    break;
  }
}

void Ctrl::CurrentSongEventHandler(void*, const Playable::change_args& args)
{ DEBUGLOG(("Ctrl::CurrentSongEventHandler(, {%p{%s}, %x, %x})\n",
    &args.Instance, args.Instance.GetURL().cdata(), args.Changed, args.Loaded));
  if (GetCurrentSong() != &args.Instance)
    return; // too late...
  EventFlags events = (EventFlags)((unsigned)args.Changed / Playable::IF_Tech * (unsigned)EV_SongTech) & EV_SongAll & ~EV_Song; // Dirty hack to shift the bits to match EV_Song*
  if (events)
  { InterlockedOr(&Pending, events);
    PostCommand(MkNop());
  }
}

void Ctrl::CurrentRootEventHandler(void*, const Playable::change_args& args)
{ DEBUGLOG(("Ctrl::CurrentRootEventHandler(, {%p{%s}, %x, %x})\n",
    &args.Instance, args.Instance.GetURL().cdata(), args.Changed, args.Loaded));
  { const int_ptr<PlayableSlice>& ps = GetRoot();
    if (!ps || ps->GetPlayable() != &args.Instance)
      return; // too late...
  }
  EventFlags events = (EventFlags)((unsigned)args.Changed / Playable::IF_Tech * (unsigned)EV_RootTech) & EV_RootAll & ~EV_Root; // Dirty hack to shift the bits to match EV_Root*
  if (events)
  { InterlockedOr(&Pending, events);
    PostCommand(MkNop());
  }
}

void Ctrl::SongIteratorEventHandler(void*, const SongIterator::CallstackEntry& ce)
{ DEBUGLOG(("Ctrl::SongIteratorEventHandler(,&%p)\n", &ce));
  // Currently there is no other event dispatched by the SongIterator.
  if (!(Pending & EV_Offset)) // Effectively a double-check
  { InterlockedOr(&Pending, EV_Offset);
    PostCommand(MkNop());
  }
}

/* Suspends or resumes playback of the currently played file. */
Ctrl::RC Ctrl::MsgPause(Op op)
{ DEBUGLOG(("Ctrl::MsgPause(%x) - %u\n", op, Scan));
  if (!Playing)
    return op & Op_Set ? RC_NotPlaying : RC_OK;

  if (SetFlag(Paused, op))
  { out_pause(Paused);
    InterlockedOr(&Pending, EV_Pause);
  }
  return RC_OK;
}

/* change scan mode logically */
Ctrl::RC Ctrl::MsgScan(Op op)
{ DEBUGLOG(("Ctrl::MsgScan(%x) - %u\n", op, Scan));
  if (op & ~7)
    return RC_BadArg;

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

  if (Playing)
  { // => Decoder
    // TODO: discard prefetch buffer.
    if (dec_fast(newscan) != 0)
      return RC_DecPlugErr;
    else // if (cfg.trash)
      // Going back in the stream to what is currently playing.
      dec_jump(FetchCurrentSongTime());

  } else if (op & Op_Set)
    return RC_NotPlaying;

  // Update event flags
  if ((Scan & DECFAST_FORWARD) != (newscan & DECFAST_FORWARD))
    InterlockedOr(&Pending, EV_Forward);
  if ((Scan & DECFAST_REWIND) != (newscan & DECFAST_REWIND))
    InterlockedOr(&Pending, EV_Rewind);
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
    InterlockedOr(&Pending, EV_Volume);
  }
  return RC_OK;
}

/* change play/stop status */
Ctrl::RC Ctrl::MsgPlayStop(Op op)
{ DEBUGLOG(("Ctrl::MsgPlayStop(%x) - %u\n", op, Playing));

  if (Playing)
  { // Set new playing position
    if ( cfg.retainonstop && op != Op_Reset
      && Current()->Iter.GetCurrentItem()->GetInfo().tech->songlength > 0 )
    { T_TIME time = FetchCurrentSongTime();
      Current()->Iter.SetLocation(time);
    } else
    { const SongIterator* start = Current()->Iter.GetCurrent()->GetStart();
      Current()->Iter.SetLocation(start ? start->GetLocation() : 0);
    }
  }

  if (!SetFlag(Playing, op))
    return RC_OK;

  if (Playing)
  { // start playback
    Song* pp = GetCurrentSong();
    if (pp == NULL)
    { Playing = false;
      return RC_NoSong;
    }
    if (OutputStart(pp) != 0)
    { Playing = false;
      return RC_OutPlugErr;
    }

    Current()->Offset = 0;
    if (DecoderStart(Current()->Iter.GetCurrent(), 0) != 0)
    { OutputStop();
      Playing = false;
      return RC_DecPlugErr;
    }

  } else
  { // stop playback
    OutputStop();
    DecoderStop();

    if (SetFlag(Paused, Op_Clear))
      InterlockedOr(&Pending, EV_Pause);
    MsgScan(Op_Reset);

    while (out_playing_data())
    { DEBUGLOG(("Ctrl::MsgPlayStop - Spinlock\n"));
      DosSleep(1);
    }
  }
  InterlockedOr(&Pending, EV_PlayStop);

  return RC_OK;
}

Ctrl::RC Ctrl::MsgNavigate(const xstring& iter, T_TIME loc, int flags)
{ DEBUGLOG(("Ctrl::MsgNavigate(%s, %g, %x)\n", iter ? iter.cdata() : "<null>", loc, flags));
  if (!GetCurrentSong())
    return RC_NoSong;
  sco_ptr<SongIterator> sip;
  if (flags & 0x02)
  { // Reset location
    sip = new SongIterator();
    sip->SetRoot(GetRoot());
  } else
  { // Start from current location
    // We must fetch the current playing time first, because this may change Current().
    T_TIME time = FetchCurrentSongTime();
    sip = new SongIterator(Current()->Iter);
    sip->SetLocation(time);
  }
  if (iter && iter.length())
  { const char* cp = iter.cdata();
    if (!sip->Deserialize(cp) && !(flags & 0x04))
      return RC_BadIterator;
    // Move forward to the next Song, if the current item is a playlist.
    AdjustNext(*sip);
  } else
    sip->SetLocation(flags & 0x01 ? sip->GetLocation() + loc : loc);
  // TODO: extend total playing time when leaving bounds of parent iterator?

  // commit
  return NavigateCore(*sip);
}

Ctrl::RC Ctrl::MsgJump(SongIterator& iter)
{ DEBUGLOG(("Ctrl::MsgJump(...)\n"));
  PlayableSlice* ps = GetRoot();
  if (!ps)
    return RC_NoSong;
  if (!iter.GetRoot() || ps->GetPlayable() != iter.GetRoot()->GetPlayable())
    return RC_InvalidItem;

  return NavigateCore(iter);
}

Ctrl::RC Ctrl::MsgSkip(int count, bool relative)
{ DEBUGLOG(("Ctrl::MsgSkip(%i, %u) - %u\n", count, relative, IsPlaying));
  if (!IsEnumerable())
    return RC_NoList;
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

  // Navigation
  SongIterator si = Current()->Iter; // work on a temporary object => copy constructor
  if (!SkipCore(si, count, relative))
  { if (cfg.autoturnaround)
    { si.Reset();
      switch (count)
      {case 1:
        if (si.Next())
          goto ok;
        break;
       case -1:
        if (si.Prev())
          goto ok;
        break;
      }
    }
    return RC_EndOfList;
  }
 ok:
  // commit
  return NavigateCore(si);
}

/* Loads Playable object to player. */
Ctrl::RC Ctrl::MsgLoad(const xstring& url, int flags)
{ DEBUGLOG(("Ctrl::MsgLoad(%s, %x)\n", url.cdata(), flags));

  // always stop
  MsgPlayStop(Op_Reset);

  // detach
  CurrentSongDelegate.detach();
  CurrentRootDelegate.detach();
  if (PrefetchList.size())
  { UpdateStackUsage(Current()->Iter.GetCallstack(), EmptyStack);
    Current()->Iter.GetRoot()->GetPlayable()->SetInUse(false);
    PrefetchClear(false);
  }

  if (url)
  { int_ptr<Playable> play = Playable::GetByURL(url);
    // Only load items that have a minimum of well known properties.
    // In case of enumerable items the content is required, in case of songs the decoder.
    // Both is related to IF_Other. The other informations are prefetched too.
    play->EnsureInfo(Playable::IF_Other);
    if (play->GetStatus() != Playable::STA_Valid)
      return RC_InvalidItem;
    // Load the required information as fast as possible
    play->EnsureInfoAsync(Playable::IF_Phys|Playable::IF_Rpl);
    // Verify all information
    play->EnsureInfoAsync(Playable::IF_All, true, true);
    { Mutex::Lock lock(PLMtx);
      PrefetchList.append() = new PrefetchEntry();
      Current()->Iter.SetRoot(new PlayableSlice(play));
      // assign change event handler
      Current()->Iter.Change += SongIteratorDelegate;
    }
    play->SetInUse(true);
    // Track root changes
    play->InfoChange += CurrentRootDelegate;
    // Move always to the first element if a playlist.
    AdjustNext(Current()->Iter);
    // track changes
    PlayableSlice* ps = Current()->Iter.GetCurrent();
    if (ps)
    { UpdateStackUsage(EmptyStack, Current()->Iter.GetCallstack());
      AttachCurrentSong(ps);
    }
  }
  InterlockedOr(&Pending, EV_RootAll|EV_SongAll);
  DEBUGLOG(("Ctrl::MsgLoad - attached\n"));

  return RC_OK;
}

Ctrl::RC Ctrl::MsgStopAt(const xstring& iter, T_TIME loc, int flags)
{ // TODO: !!!
  return RC_OK;
}

/* saving the currently played stream. */
Ctrl::RC Ctrl::MsgSave(const xstring& filename)
{ DEBUGLOG(("Ctrl::MsgSave(%s)\n", filename.cdata()));

  if (Savename == filename)
    return RC_OK;
  InterlockedOr(&Pending, EV_Savename);
  Savename = filename;

  if (Playing && dec_status() == DECODER_PLAYING)
    dec_save(Savename);
  // TODO: is it really a good idea to save different streams into the same file???
  return RC_OK;
}

/* Adjusts shuffle flag. */
Ctrl::RC Ctrl::MsgShuffle(Op op)
{ DEBUGLOG(("Ctrl::MsgShuffle(%x) - %u\n", op, Scan));
  if (SetFlag(Shuffle, op))
    InterlockedOr(&Pending, EV_Shuffle);
  return RC_OK;
}

/* Adjusts repeat flag. */
Ctrl::RC Ctrl::MsgRepeat(Op op)
{ DEBUGLOG(("Ctrl::MsgRepeat(%x) - %u\n", op, Scan));
  if (SetFlag(Repeat, op))
    InterlockedOr(&Pending, EV_Repeat);
  return RC_OK;
}

Ctrl::RC Ctrl::MsgLocation(SongIterator* sip, int flags)
{ DEBUGLOG(("Ctrl::MsgLocation(%p, %x)\n", sip, flags));
  if (!PrefetchList.size())
    return RC_NoSong; // no root
  if (flags & 1)
  { // stopat location
    // TODO: not yet implemented
  } else
  { // Fetch time first because that may change Current().
    T_TIME pos = FetchCurrentSongTime();
    *sip = Current()->Iter; // copy
    sip->SetLocation(pos);
  }
  return RC_OK;
}

// The decoder completed decoding...
Ctrl::RC Ctrl::MsgDecStop()
{ DEBUGLOG(("Ctrl::MsgDecStop()\n"));
  if (!Playing)
    return RC_NotPlaying;

  if (!IsEnumerable() && !Repeat)
  { // Song, no repeat => stop
   eol:
    DEBUGLOG(("Ctrl::MsgDecStop: flush\n"));
    DecoderStop();
    out_flush();
    // Continue at OUTEVENT_END_OF_DATA
    return RC_OK;
  }

  PrefetchEntry* pep = new PrefetchEntry(Current()->Offset + dec_maxpos(), Current()->Iter);
  pep->Offset += dec_maxpos();
  int dir = Scan == DECFAST_REWIND ? -1 : 1; // DecoderStop resets scan mode
  DecoderStop();

  // Navigation
  PlayableSlice* ps = NULL;
  if (IsEnumerable())
  { do
    { if ( ( !SkipCore(pep->Iter, dir, true)
          && (!Repeat || !SkipCore(pep->Iter, dir, true)) )
        || (Repeat && pep->Iter.CompareTo(Current()->Iter, 0, false) == 0) ) // no infinite loop
      { delete pep;
        goto eol; // end of list => same as end of song
      }

      ps = pep->Iter.GetCurrent();
      ps->GetPlayable()->EnsureInfo(Playable::IF_Other);
      // skip invalid
    } while (ps->GetPlayable()->GetStatus() == Playable::STA_Invalid);
  } else
  { ps = pep->Iter.GetCurrent();
  }
  // store result
  Mutex::Lock lock(PLMtx);
  PrefetchList.append() = pep;

  // start decoder for the prefetched item
  DEBUGLOG(("Ctrl::MsgDecStop playing %s with offset %g\n", ps->GetPlayable()->GetURL().cdata(), pep->Offset));
  if (DecoderStart(ps, pep->Offset) != 0)
  { // TODO: we should continue with the next song, and remove the current one from the prefetch list.
    OutputStop();
    Playing = false;
    InterlockedOr(&Pending, EV_PlayStop);
    return RC_DecPlugErr;
  }

  // Once the player arrives the prefetched item it requests some information.
  // Let's try to prefetch this information at low priority to avoid latencies.
  ps->GetPlayable()->EnsureInfoAsync(Playable::IF_Format|Playable::IF_Tech|Playable::IF_Meta|Playable::IF_Phys, true);

  // In rewind mode we continue to rewind from the end of the prevois song.
  // TODO: Location problem.
  return RC_OK;
}

// The output completed playing
Ctrl::RC Ctrl::MsgOutStop()
{ DEBUGLOG(("Ctrl::MsgOutStop()\n"));
  // Check whether we have to remove items in queue mode
  if ( cfg.queue_mode && PrefetchList.size()
    && (Current()->Iter.GetRoot()->GetPlayable()->GetFlags() & Playable::Mutable) == Playable::Mutable )
  { Playlist* plp = (Playlist*)Current()->Iter.GetRoot();
    if (plp->GetURL() == amp_get_default_pl()->GetURL())
    { DEBUGLOG(("Ctrl::MsgOutStop: queue mode %p\n", plp));
      plp->RemoveItem(&(PlayableInstance&)*Current()->Iter.GetCallstack()[0]->Item);
    }
  }
  // In any case stop the engine
  return MsgPlayStop(Op_Reset);
}

void TFNENTRY ControllerWorkerStub(void*)
{ Ctrl::Worker();
}

void Ctrl::Worker()
{ HAB hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(hab, 0);
  for(;;)
  { DEBUGLOG(("Ctrl::Worker() looking for work\n"));
    queue<ControlCommand*>::qentry* qp = Queue.RequestRead();
    if (qp->Data == NULL)
    { Queue.CommitRead(qp);
      break; // deadly pill
    }

    bool fail = false;
    do
    { ControlCommand* ccp = qp->Data;
      DEBUGLOG(("Ctrl::Worker received message: %p{%i, %s, %08x%08x, %x, %p, %p} - %u\n",
      ccp, ccp->Cmd, ccp->StrArg ? ccp->StrArg.cdata() : "<null>", ccp->PtrArg, (&ccp->PtrArg)[1], ccp->Flags, ccp->Callback, ccp->Link, fail));
      if (fail)
        ccp->Flags = RC_SubseqError;
      else
        // Do the work
        switch (ccp->Cmd)
        {default:
          ccp->Flags = RC_BadArg;
          break;

         case Cmd_Load:
          ccp->Flags = MsgLoad(ccp->StrArg, ccp->Flags);
          break;

         case Cmd_Skip:
          ccp->Flags = MsgSkip((int)ccp->NumArg, ccp->Flags & 0x01);
          break;

         case Cmd_Navigate:
          ccp->Flags = MsgNavigate(ccp->StrArg, ccp->NumArg, ccp->Flags);
          break;

         case Cmd_Jump:
          ccp->Flags = MsgJump(*(SongIterator*)ccp->PtrArg);
          break;

         case Cmd_StopAt:
          ccp->Flags = MsgStopAt(ccp->StrArg, ccp->NumArg, ccp->Flags);
          break;

         case Cmd_PlayStop:
          ccp->Flags = MsgPlayStop((Op)ccp->Flags);
          break;

         case Cmd_Pause:
          ccp->Flags = MsgPause((Op)ccp->Flags);
          break;

         case Cmd_Scan:
          ccp->Flags = MsgScan((Op)ccp->Flags);
          break;

         case Cmd_Volume:
          ccp->Flags = MsgVolume(ccp->NumArg, ccp->Flags & 0x01);
          break;

         case Cmd_Shuffle:
          ccp->Flags = MsgShuffle((Op)ccp->Flags);
          break;

         case Cmd_Repeat:
          ccp->Flags = MsgRepeat((Op)ccp->Flags);
          break;

         case Cmd_Save:
          ccp->Flags = MsgSave(ccp->StrArg);
          break;

         case Cmd_Location:
          ccp->Flags = MsgLocation((SongIterator*)ccp->PtrArg, ccp->Flags);
          break;

         case Cmd_DecStop:
          ccp->Flags = MsgDecStop();
          break;

         case Cmd_OutStop:
          ccp->Flags = MsgOutStop();
          break;

         case Cmd_Nop:
          break;
        }
      DEBUGLOG(("Ctrl::Worker message %i completed, rc = %i\n", ccp->Cmd, ccp->Flags));
      fail = ccp->Flags != RC_OK;

      // Link and store the link in the data element of the queue.
      { // Must be done in synchronized context to avoid inconsistecies in QueueTraverse.
        Mutex::Lock lock(Queue.Mtx);
        qp->Data = ccp->Link;
      }
      // cleanup
      if (ccp->Callback)
        (*ccp->Callback)(ccp);
      else
        delete ccp;

    } while (qp->Data);

    // done
    Queue.CommitRead(qp);
    // raise control event
    EventFlags events = (EventFlags)InterlockedXch(&Pending, EV_None);
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
  dec_event += DecEventDelegate;
  out_event += OutEventDelegate;
  WorkerTID = _beginthread(&ControllerWorkerStub, NULL, 262144, NULL);
  ASSERT((int)WorkerTID != -1);
  // load the state
  ctrl_state state;
  load_ini_value(amp_hini, state.volume);
  load_ini_value(amp_hini, state.shf);
  load_ini_value(amp_hini, state.rpt);
  load_ini_xstring(amp_hini, state.current_root);
  load_ini_xstring(amp_hini, state.current_iter);
  load_ini_value(amp_hini, state.was_playing);
  PostCommand(MkShuffle(state.shf ? Op_Set : Op_Clear));
  PostCommand(MkRepeat(state.rpt ? Op_Set : Op_Clear));
  if (state.volume >= 0)
    PostCommand(MkVolume(state.volume, false));
  if (state.current_root)
  { ControlCommand* head = MkLoad(state.current_root, false);
    ControlCommand* tail = head;
    if (state.current_iter)
      tail = tail->Link = MkNavigate(state.current_iter, 0, true, true);
    if (cfg.restartonstart && state.was_playing)
      tail = tail->Link = MkPlayStop(Op_Set);
    PostCommand(head);
  }
}

void Ctrl::Uninit()
{ DEBUGLOG(("Ctrl::Uninit()\n"));
  ctrl_state state;
  state.volume = GetVolume();
  state.was_playing = IsPlaying();
  SongIterator last; // last playing location
  { Mutex::Lock l(Queue.Mtx);
    Queue.Purge();
    PostCommand(MkLocation(&last, 0));
    PostCommand(MkLoad(xstring(), 0));
    PostCommand(NULL);
    DecEventDelegate.detach();
    OutEventDelegate.detach();
  }
  if (WorkerTID != 0)
    wait_thread_pm(amp_player_hab(), WorkerTID, 30000);

  // save the state
  state.shf = IsShuffle();
  state.rpt = IsRepeat();
  if (last.GetRoot())
  { state.current_root = (const xstring&)last.GetRoot()->GetPlayable()->GetURL();
    // save location only if the current item has definite length.
    Playable* pci = last.GetCurrentItem();
    state.current_iter = last.Serialize(cfg.retainonexit && pci && pci->GetInfo().tech->songlength >= 0);
    DEBUGLOG(("last_loc: %s %s\n", state.current_root.cdata(), state.current_iter.cdata()));
  }
  save_ini_value(amp_hini, state.volume);
  save_ini_value(amp_hini, state.shf);
  save_ini_value(amp_hini, state.rpt);
  save_ini_string(amp_hini, state.current_root);
  save_ini_string(amp_hini, state.current_iter);
  save_ini_value(amp_hini, state.was_playing);

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
  // find last command
  ControlCommand* cmde = cmd;
  while (cmde->Link)
  { cmde->Callback = &CbNop;
    cmde = cmde->Link;
  }
  cmde->User = &callback;
  cmde->Callback = &SendCallbackFunc;
  PostCommand(cmd);
  callback.Wait();
  return cmd;
}

void Ctrl::CbNop(ControlCommand* cmd)
{}

