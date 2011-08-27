/*
 * Copyright 2007-2011 M.Mueller
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
#include "controller.h"
#include "waitinfo.h"
#include "configuration.h"
#include "decoder.h"
#include "gui.h"
#include "glue.h"
#include "pm123.h"

#include <inimacro.h>
#include <minmax.h>
#include <utilfct.h>
#include <cpp/mutex.h>
#include <cpp/xstring.h>

#include <limits.h>


/* Ini file stuff, a bit dirty */

struct ctrl_state
{ double  volume;        // Position of the volume slider
  xstring current_root;  // The currently loaded root.
  xstring current_iter;  // The current location within the root.
  bool    shf;           // The state of the "Shuffle" button.
  bool    rpt;           // The state of the "Repeat" button.
  bool    was_playing;   // Restart playback on start-up
  ctrl_state() : volume(-1), shf(false), rpt(false), was_playing(false) {}
};


/****************************************************************************
*
*  class Ctrl
*
****************************************************************************/

/// Private implementation of class \c Ctrl.
class CtrlImp : public Ctrl
{ friend class Ctrl;
 private:
  struct PrefetchEntry
  { PM123_TIME                Offset;        // Starting time index from the output's point of view.
    SongIterator              Loc;           // Location that points to the song that starts at this position.
    //PrefetchEntry() : Offset(0) {}
    PrefetchEntry(PM123_TIME offset, const SongIterator& loc) : Offset(offset), Loc(loc) {}
  };

 private: // extended working set
  // List of prefetched iterators.
  // The first entry is always the current iterator if a enumerable object is loaded.
  // Write access to this list is only done by the controller thread.
  static vector<PrefetchEntry> PrefetchList;
  static Mutex                PLMtx;                 // Mutex to protect the above list.

  static TID                  WorkerTID;             // Thread ID of the worker
  static ControlCommand*      CurCmd;                // Currently processed ControlCommand, protected by Queue.Mtx

  // These events are set atomically from any thread.
  // After each last message of a queued set of messages the events are raised and Pending is atomically reset.
  static AtomicUnsigned       Pending;               // Pending events

  // Delegates for the tracked events of the decoder, the output and the current song.
  static delegate<void, const dec_event_args>        DecEventDelegate;
  static delegate<void, const OUTEVENTTYPE>          OutEventDelegate;
  //static delegate<void, const PlayableChangeArgs>    CurrentSongDelegate;
  static delegate<void, const PlayableChangeArgs>    CurrentRootDelegate;
  //static delegate<void, const CallstackEntry>        SongIteratorDelegate;

  // Occasionally used constants.
  static const vector_int<PlayableInstance> EmptyStack;

 private: // internal functions, not thread safe
  // Returns the current PrefetchEntry. I.e. the currently playing (or not playing) iterator.
  // Precondition: an object must have been loaded.
  static PrefetchEntry* Current() { return PrefetchList[0]; }
  /// Raise any pending controller events.
  static void  RaiseControlEvents();

  // Applies the operator op to flag and returns true if the flag has been changed.
  static bool  SetFlag(bool& flag, Op op);
  // Sets the volume according to this->Volume and the scan mode.
  static void  SetVolume();
  // Initializes the decoder engine and starts playback of the song pp with a time offset for the output.
  // The function returns the result of dec_play.
  // Precondition: The output must have been initialized.
  // The function does not return unless the decoder is decoding or an error occurred.
  static ULONG DecoderStart(APlayable& ps, PM123_TIME offset);
  // Stops decoding and deinitializes the decoder plug-in.
  static void  DecoderStop();
  // Initializes the output for playing pp.
  // The playable object is needed for naming purposes.
  static ULONG OutputStart(APlayable& pp);
  // Stops playback and clears the prefetch list.
  static void  OutputStop();
  // Updates the in-use status of PlayableInstance objects in the callstack by moving from oldstack to newstack.
  // The status of common parts of the two stacks is not touched.
  // To set the in-use status initially pass EmptyStack as oldstack.
  // To reset all in-use status pass EmptyStack as newstack.
  static void  UpdateStackUsage(const vector<PlayableInstance>& oldstack, const vector<PlayableInstance>& newstack);
  // Internal sub function to UpdateStackUsage
  static void  SetStackUsage(PlayableInstance*const* rbegin, PlayableInstance*const* rend, bool set);
  // Core logic of MsgSkip.
  // Move the current song pointer by count items if relative is true or to item number 'count' if relative is false.
  // If we try to move the current song pointer out of the range of the that that si relies on,
  // the function returns false and si is in reset state.
  // The function is side effect free and only operates on si.
  static bool  SkipCore(Location& si, int count, bool relative);
  // Ensure that a SongIterator really points to a valid song by moving the iterator forward as required.
  static bool  AdjustNext(Location& si);
  // Jump to the location si. The function will destroy the content of si.
  static RC    NavigateCore(Location& si);
  // Register events to a new current song and request some information if not yet known.
  //static void  AttachCurrentSong(APlayable& ps);
  // Clears the prefetch list and keep the first element if keep is true.
  // The operation is atomic.
  static void  PrefetchClear(bool keep);
  // Check whether a prefetched item is completed.
  // This is the case when the offset of the next item in PrefetchList is less than or equal to pos.
  // In case a prefetch entry is removed from PrefetchList the in-use status is refreshed and the song
  // change event is set.
  static void  CheckPrefetch(PM123_TIME pos);
  // Get the time in the current Song. This may cleanup the prefetch list by calling CheckPrefetch.
  static PM123_TIME FetchCurrentSongTime();
  // Internal stub to provide the TFNENTRY calling convention for _beginthread.
  friend void TFNENTRY ControllerWorkerStub(void*);
  // Worker thread that processes the message queue.
  static void  Worker();
  // Event handler for decoder events.
  static void  DecEventHandler(void*, const dec_event_args& args);
  // Event handler for output events.
  static void  OutEventHandler(void*, const OUTEVENTTYPE& event);
  // Event handler for tracking modifications of the currently playing song.
  static void  CurrentSongEventHandler(void*, const PlayableChangeArgs& args);
  // Event handler for tracking modifications of the currently loaded object.
  static void  CurrentRootEventHandler(void*, const PlayableChangeArgs& args);
  // Event handler for asynchronuous changes to the songiterator (not any prefetched one).
  //static void  SongIteratorEventHandler(void*, const CallstackEntry& ce);

 private: // messages handlers, not thread safe
  // The messages are descibed above before the class header.
  static RC    MsgPause(Op op);
  static RC    MsgScan(Op op);
  static RC    MsgVolume(double volume, bool relative);
  static RC    MsgNavigate(const xstring& iter, PM123_TIME loc, int flags);
  static RC    MsgJump(Location& iter);
  static RC    MsgStopAt(const xstring& iter, PM123_TIME loc, int flags);
  static RC    MsgPlayStop(Op op);
  static RC    MsgSkip(int count, bool relative);
  static RC    MsgLoad(const xstring& url, int flags);
  static RC    MsgSave(const xstring& filename);
  static RC    MsgShuffle(Op op);
  static RC    MsgRepeat(Op op);
  static RC    MsgLocation(SongIterator* sip, int flags);
  static RC    MsgDecStop();
  static RC    MsgOutStop();

 private:
  struct QueueTraverseProxyData
  { void  (*Action)(const Ctrl::ControlCommand& cmd, void* arg);
    void* Arg;
  };
  static void QueueTraverseProxy(const Ctrl::QEntry& entry, void* arg);
};


void Ctrl::ControlCommand::Destroy()
{ if (Link)
    Link->Destroy();
  delete this;
}

vector<CtrlImp::PrefetchEntry> CtrlImp::PrefetchList(10);
Mutex                 CtrlImp::PLMtx;
bool                  Ctrl::Playing   = false;
bool                  Ctrl::Paused    = false;
DECFASTMODE           Ctrl::Scan      = DECFAST_NORMAL_PLAY;
double                Ctrl::Volume    = 1.;
xstring               Ctrl::Savename;
bool                  Ctrl::Shuffle   = false;
bool                  Ctrl::Repeat    = false;

queue<Ctrl::QEntry>   Ctrl::Queue;
TID                   CtrlImp::WorkerTID = 0;
Ctrl::ControlCommand* CtrlImp::CurCmd    = NULL;

AtomicUnsigned        CtrlImp::Pending   = Ctrl::EV_None;
event<const Ctrl::EventFlags> Ctrl::ChangeEvent;

delegate<void, const dec_event_args>        CtrlImp::DecEventDelegate(&CtrlImp::DecEventHandler);
delegate<void, const OUTEVENTTYPE>          CtrlImp::OutEventDelegate(&CtrlImp::OutEventHandler);
delegate<void, const PlayableChangeArgs>    CtrlImp::CurrentRootDelegate(&CtrlImp::CurrentRootEventHandler);

const vector_int<PlayableInstance> CtrlImp::EmptyStack;


int_ptr<APlayable> Ctrl::GetCurrentSong()
{ DEBUGLOG(("Ctrl::GetCurrentSong() - %u\n", CtrlImp::PrefetchList.size()));
  int_ptr<APlayable> pp;
  { Mutex::Lock lock(CtrlImp::PLMtx);
    if (CtrlImp::PrefetchList.size())
      pp = &CtrlImp::Current()->Loc.GetCurrent();
  }
  return pp;
}

int_ptr<APlayable> Ctrl::GetRoot()
{ Mutex::Lock lock(CtrlImp::PLMtx);
  if (!CtrlImp::PrefetchList.size())
    return int_ptr<APlayable>();
  return CtrlImp::Current()->Loc.GetRoot();
}

/*bool Ctrl::IsEnumerable()
{ return PrefetchList.size() && (Current()->Iter.GetRoot()->GetPlayable()->GetFlags() & Playable::Enumerable);
}*/

void CtrlImp::RaiseControlEvents()
{ EventFlags events = (EventFlags)Pending.swap(EV_None);
  DEBUGLOG(("CtrlImp::RaiseControlEvents: %x\n", events));
  if (events)
    ChangeEvent(events);
}


bool CtrlImp::SetFlag(bool& flag, Op op)
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

void CtrlImp::SetVolume()
{ double volume = Volume;
  if (Scan)
    volume *= 3./5.;
  out_set_volume(volume);
}

ULONG CtrlImp::DecoderStart(APlayable& ps, PM123_TIME offset)
{ DEBUGLOG(("Ctrl::DecoderStart(&%p{%p})\n", &ps, &ps.GetPlayable()));
  SetVolume();
  dec_save(Savename);

  PM123_TIME start = 0;
  PM123_TIME stop  = 1E99;
  { int_ptr<Location> lp = ps.GetStartLoc();
    if (lp)
      start = lp->GetPosition();
    lp = ps.GetStopLoc();
    if (lp)
      stop = lp->GetPosition();
  }
  
  if (Scan == DECFAST_REWIND)
  { if (stop > 0)
    { start = stop - 1.; // do not seek to the end, because this will cause problems.
    } else if (ps.GetInfo().obj->songlength > 0)
    { start = ps.GetInfo().obj->songlength - 1.;
    } else
    { // no songlength => error => undo MsgScan
      Scan = DECFAST_NORMAL_PLAY;
      Pending |= EV_Rewind;
    }
    if (start < 0) // Do not hit negative values for very short songs.
      start = 0;
  }

  ULONG rc = dec_play(ps, offset, start, stop);
  if (rc != 0)
    return rc;

  if (Scan != DECFAST_NORMAL_PLAY)
    dec_fast(Scan);
  DEBUGLOG(("Ctrl::DecoderStart - completed\n"));
  return 0;
}

void CtrlImp::DecoderStop()
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

ULONG CtrlImp::OutputStart(APlayable& pp)
{ DEBUGLOG(("Ctrl::OutputStart(&%p)\n", &pp));
  ULONG rc = out_setup( pp );
  DEBUGLOG(("Ctrl::OutputStart: after setup - %d\n", rc));
  return rc;
}

void CtrlImp::OutputStop()
{ DEBUGLOG(("Ctrl::OutputStop()\n"));
  // Clear prefetch list
  PrefetchClear(true);
  // close output
  out_close();
  // reset offset
  if (PrefetchList.size())
    Current()->Offset = 0;
}

void CtrlImp::SetStackUsage(PlayableInstance*const* rbegin, PlayableInstance*const* rend, bool set)
{ while (rend != rbegin)
    // Depending on the object type this updates the status of a PlayableInstance
    // or only the status of the underlying Playable.
    (*--rend)->SetInUse(set);
}

void CtrlImp::UpdateStackUsage(const vector<PlayableInstance>& oldstack, const vector<PlayableInstance>& newstack)
{ DEBUGLOG(("Ctrl::UpdateStackUsage({%u }, {%u })\n", oldstack.size(), newstack.size()));
  PlayableInstance*const* oldppi = oldstack.begin();
  PlayableInstance*const* newppi = newstack.begin();
  // skip identical part
  // TODO? a more optimized approach may work on the sorted exclude lists.
  while (oldppi != oldstack.end() && newppi != newstack.end() && *oldppi == *newppi)
  { DEBUGLOG(("Ctrl::UpdateStackUsage identical - %p == %p\n", *oldppi, *newppi));
    ++oldppi;
    ++newppi;
  }
  // reset usage flags of part of old stack
  SetStackUsage(oldppi, oldstack.end(), false);
  // set usage flags of part of the new stack
  SetStackUsage(newppi, newstack.end(), true);
}

bool CtrlImp::SkipCore(Location& si, int count, bool relative)
{ DEBUGLOG(("Ctrl::SkipCore({%s}, %i, %u)\n", si.Serialize().cdata(), count, relative));
  if (!relative)
    si.Reset();
  const Location::NavigationResult& rc = si.NavigateCount(count, TATTR_SONG, JobSet::SyncJob);
  DEBUGLOG(("Ctrl::SkipCore: %s\n", rc.cdata()));
  return !rc;
}

bool CtrlImp::AdjustNext(Location& si)
{ DEBUGLOG(("Ctrl::AdjustNext({%s})\n", si.Serialize().cdata()));
  APlayable& ps = si.GetCurrent();
  ps.RequestInfo(IF_Tech, PRI_Sync);
  if (ps.GetInfo().tech->attributes & TATTR_SONG)
    return true;
  const Location::NavigationResult& rc = si.NavigateCount(1, TATTR_SONG, JobSet::SyncJob);
  DEBUGLOG(("Ctrl::AdjustNext: %s\n", rc.cdata()));
  return !rc;  
}

Ctrl::RC CtrlImp::NavigateCore(Location& si)
{ DEBUGLOG(("Ctrl::NavigateCore({%s}) - %s\n", si.Serialize().cdata(), Current()->Loc.Serialize().cdata()));
  // Check whether the current song has changed?
  int level = si.CompareTo(Current()->Loc);
  DEBUGLOG(("Ctrl::NavigateCore - %i\n", level));
  if (level == 0)
    return RC_OK; // song and location identical => no-op
  ASSERT(level != INT_MIN);
  if (abs(level) > max(si.GetLevel(), Current()->Loc.GetLevel()))
  { DEBUGLOG(("Ctrl::NavigateCore - seek to %f\n", Current()->Loc.GetPosition()));
    // only location is different => seek only
    if (Playing)
    { if (dec_jump(si.GetPosition()) != 0)
        return RC_DecPlugErr;
    }
    Mutex::Lock lock(PLMtx);
    Current()->Loc.Swap(si);
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
    //CurrentSongDelegate.detach();
    // swap iterators
    Current()->Offset = 0;
    Current()->Loc.Swap(si);
  }
  // Events
  UpdateStackUsage(si.GetCallstack(), Current()->Loc.GetCallstack());
  Pending |= EV_Song;
  // track updates
  APlayable& ps = Current()->Loc.GetCurrent();
  //AttachCurrentSong(ps);

  // restart decoder immediately?
  if (Playing)
  { if (DecoderStart(ps, 0) != 0)
    { OutputStop();
      Playing = false;
      Pending |= EV_PlayStop;
      return RC_DecPlugErr;
    }
  }
  return RC_OK;
}

/*void Ctrl::AttachCurrentSong(APlayable& ps)
{ DEBUGLOG(("Ctrl::AttachCurrentSong(&%p)\n", &ps));
  ps.RequestInfo(IF_Tech|IF_Obj|IF_Meta|IF_Rpl|IF_Drpl, PRI_Low);
  ps.GetInfoChange() += CurrentSongDelegate;
}*/

void CtrlImp::PrefetchClear(bool keep)
{ DEBUGLOG(("Ctrl::PrefetchClear(%u)\n", keep));
  Mutex::Lock lock(PLMtx);
  PrefetchEntry*const* where = PrefetchList.end();
  while (PrefetchList.size() > keep) // Hack: keep = false deletes all items while keep = true kepp the first item.
    delete PrefetchList.erase(--where);
}

void CtrlImp::CheckPrefetch(double pos)
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
      //CurrentSongDelegate.detach();
      UpdateStackUsage(Current()->Loc.GetCallstack(), PrefetchList[n]->Loc.GetCallstack());
      // Set events
      Pending |= EV_Song;
      // Cleanup prefetch list
      vector<PrefetchEntry> ped(n);
      { Mutex::Lock lock(PLMtx);
        // detach the songiterator event
        //SongIteratorDelegate.detach();
        do
          ped.append() = PrefetchList.erase(--n);
        while (n);
        // attach the songiterator delegate to the new head
        //Current()->Iter.Change += SongIteratorDelegate;
      }
      // Now keep track of the next entry
      //AttachCurrentSong(Current()->Loc.GetCurrent());

      // delete iterators and remove from play queue (if desired)
      Playable* plp = NULL;
      if (Cfg::Get().queue_mode)
      { plp = Current()->Loc.GetRoot();
        if (plp != &GUI::GetDefaultPL())
          plp = NULL;
      }
      DEBUGLOG(("Ctrl::CheckPrefetch: queue mode %p\n", plp));
      // plp != NULL -> remove items
      n = ped.size();
      do
      { PrefetchEntry& pe = *ped[--n]; 
        if (plp && pe.Loc.GetLevel() >= 1)
        { PlayableInstance* pip = pe.Loc.GetCallstack()[0];
          if (pe.Loc.NavigateCount(1, TATTR_SONG, JobSet::SyncJob, 1))// we played the last item of a top level entry
            plp->RemoveItem(pip);
        }
        delete &pe;
      } while (n);
    }
  }
}

PM123_TIME CtrlImp::FetchCurrentSongTime()
{ DEBUGLOG(("Ctrl::FetchCurrentSongTime() - %u\n", Playing));
  if (Playing)
  { PM123_TIME time = out_playing_pos();
    // Check whether the output played a prefetched item completely.
    CheckPrefetch(time);
    return time - Current()->Offset; // relocate playing position
  } else
    return Current()->Loc.GetPosition();
}

void CtrlImp::DecEventHandler(void*, const dec_event_args& args)
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

void CtrlImp::OutEventHandler(void*, const OUTEVENTTYPE& event)
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

/*void Ctrl::CurrentSongEventHandler(void*, const PlayableChangeArgs& args)
{ DEBUGLOG(("Ctrl::CurrentSongEventHandler(, {%p{%s}, %x, %x})\n",
    &args.Instance, args.Instance.GetPlayable().URL.cdata(), args.Changed, args.Loaded));
  if (GetCurrentSong() != &args.Instance)
    return; // too late...
  EventFlags events = (EventFlags)((unsigned)args.Changed / Playable::IF_Tech * (unsigned)EV_SongTech) & EV_SongAll & ~EV_Song; // Dirty hack to shift the bits to match EV_Song*
  if (events)
  { InterlockedOr(&Pending, events);
    PostCommand(MkNop());
  }
}*/

void CtrlImp::CurrentRootEventHandler(void*, const PlayableChangeArgs& args)
{ DEBUGLOG(("Ctrl::CurrentRootEventHandler(, {%p{%s}, %x, %x})\n",
    &args.Instance, args.Instance.GetPlayable().URL.cdata(), args.Changed, args.Loaded));
  { const int_ptr<APlayable>& ps = GetRoot();
    if (!ps || ps != &args.Instance)
      return; // too late...
  }
  /*EventFlags events = (EventFlags)((unsigned)args.Changed / Playable::IF_Tech * (unsigned)EV_RootTech) & EV_RootAll & ~EV_Root; // Dirty hack to shift the bits to match EV_Root*
  if (events)
  { InterlockedOr(&Pending, events);
    PostCommand(MkNop());
  }*/  
}

/*void Ctrl::SongIteratorEventHandler(void*, const SongIterator::CallstackEntry& ce)
{ DEBUGLOG(("Ctrl::SongIteratorEventHandler(,&%p)\n", &ce));
  // Currently there is no other event dispatched by the SongIterator.
  if (!(Pending & EV_Offset)) // Effectively a double-check
  { InterlockedOr(&Pending, EV_Offset);
    PostCommand(MkNop());
  }
}*/

/* Suspends or resumes playback of the currently played file. */
Ctrl::RC CtrlImp::MsgPause(Op op)
{ DEBUGLOG(("Ctrl::MsgPause(%x) - %u\n", op, Scan));
  if (!Playing)
    return op & Op_Set ? RC_NotPlaying : RC_OK;

  if (SetFlag(Paused, op))
  { out_pause(Paused);
    Pending |= EV_Pause;
  }
  return RC_OK;
}

/* change scan mode logically */
Ctrl::RC CtrlImp::MsgScan(Op op)
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
    Pending |= EV_Forward;
  if ((Scan & DECFAST_REWIND) != (newscan & DECFAST_REWIND))
    Pending |= EV_Rewind;
  Scan = newscan;
  SetVolume();
  return RC_OK;
}

Ctrl::RC CtrlImp::MsgVolume(double volume, bool relative)
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
    Pending |= EV_Volume;
  }
  return RC_OK;
}

/* change play/stop status */
Ctrl::RC CtrlImp::MsgPlayStop(Op op)
{ DEBUGLOG(("Ctrl::MsgPlayStop(%x) - %u\n", op, Playing));

  if (Playing)
  { // Set new playing position
    if ( Cfg::Get().retainonstop && op != Op_Reset
      && Current()->Loc.GetCurrent().GetInfo().obj->songlength > 0 )
    { PM123_TIME time = FetchCurrentSongTime();
      Current()->Loc.Navigate(time, JobSet::SyncJob);
    } else
    { int_ptr<Location> start = Current()->Loc.GetCurrent().GetStartLoc();
      Current()->Loc.Navigate(start ? start->GetPosition() : 0, JobSet::SyncJob);
    }
  }

  if (!SetFlag(Playing, op))
    return RC_OK;

  if (Playing)
  { // start playback
    APlayable* pp = GetCurrentSong();
    if (pp == NULL)
    { Playing = false;
      return RC_NoSong;
    }

    pp->RequestInfo(IF_Decoder|IF_Tech|IF_Obj|IF_Slice, PRI_Sync);
    if (!(pp->GetInfo().tech->attributes & TATTR_SONG))
    { Playing = false;
      return RC_NoSong;
    }

    if (OutputStart(*pp) != 0)
    { Playing = false;
      return RC_OutPlugErr;
    }

    Current()->Offset = 0;
    if (DecoderStart(Current()->Loc.GetCurrent(), 0) != 0)
    { OutputStop();
      Playing = false;
      return RC_DecPlugErr;
    }

  } else
  { // stop playback
    DecoderStop();
    OutputStop();

    if (SetFlag(Paused, Op_Clear))
      Pending |= EV_Pause;
    MsgScan(Op_Reset);

    while (out_playing_data())
    { DEBUGLOG(("Ctrl::MsgPlayStop - Spinlock\n"));
      DosSleep(1);
    }
  }
  Pending |= EV_PlayStop;

  return RC_OK;
}

Ctrl::RC CtrlImp::MsgNavigate(const xstring& iter, PM123_TIME loc, int flags)
{ DEBUGLOG(("Ctrl::MsgNavigate(%s, %g, %x)\n", iter ? iter.cdata() : "<null>", loc, flags));
  if (!GetCurrentSong())
    return RC_NoSong;
  sco_ptr<Location> sip;
  if (flags & 0x02)
  { // Reset location
    sip = new Location(&GetRoot()->GetPlayable());
  } else
  { // Start from current location
    // We must fetch the current playing time first, because this may change Current().
    PM123_TIME time = FetchCurrentSongTime();
    sip = new Location(Current()->Loc);
    sip->Navigate(time, JobSet::SyncJob);
  }
  if (iter && iter.length())
  { const char* cp = iter.cdata();
    if (sip->Deserialize(cp, JobSet::SyncJob) && !(flags & 0x04))
      return RC_BadIterator;
    // Move forward to the next Song, if the current item is a playlist.
    AdjustNext(*sip);
  } else
    sip->Navigate(flags & 0x01 ? sip->GetPosition() + loc : loc, JobSet::SyncJob);
  // TODO: extend total playing time when leaving bounds of parent iterator?

  // commit
  return NavigateCore(*sip);
}

Ctrl::RC CtrlImp::MsgJump(Location& iter)
{ DEBUGLOG(("Ctrl::MsgJump(...)\n"));
  APlayable* ps = GetRoot();
  if (!ps)
    return RC_NoSong;
  if (&ps->GetPlayable() != iter.GetRoot())
    return RC_InvalidItem;

  return NavigateCore(iter);
}

Ctrl::RC CtrlImp::MsgSkip(int count, bool relative)
{ DEBUGLOG(("Ctrl::MsgSkip(%i, %u) - %u\n", count, relative, IsPlaying));
  APlayable* pp = GetRoot();
  pp->RequestInfo(IF_Tech, PRI_Sync);
  if (!(pp->GetInfo().tech->attributes & TATTR_PLAYLIST))
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
  Location si = Current()->Loc; // work on a temporary object => copy constructor
  if (!SkipCore(si, count, relative))
  { if (Cfg::Get().autoturnaround)
    { si.Reset();
      switch (count)
      {case 1:
       case -1:
        if (!si.NavigateCount(count, TATTR_SONG, JobSet::SyncJob))
          goto ok;
      }
    }
    return RC_EndOfList;
  }
 ok:
  // commit
  return NavigateCore(si);
}

/* Loads Playable object to player. */
Ctrl::RC CtrlImp::MsgLoad(const xstring& url, int flags)
{ DEBUGLOG(("Ctrl::MsgLoad(%s, %x)\n", url.cdata(), flags));

  // always stop
  MsgPlayStop(Op_Reset);

  // detach
  //CurrentSongDelegate.detach();
  CurrentRootDelegate.detach();
  if (PrefetchList.size())
  { UpdateStackUsage(Current()->Loc.GetCallstack(), EmptyStack);
    Current()->Loc.GetRoot()->SetInUse(false);
    PrefetchClear(false);
  }

  if (url)
  { int_ptr<Playable> play = Playable::GetByURL(url);
    { Mutex::Lock lock(PLMtx);
      PrefetchList.append() = new PrefetchEntry(0, SongIterator(play));
      // assign change event handler
      //Current()->Iter.Change += SongIteratorDelegate;
      Pending |= EV_Root|EV_Song;
      // Raise events early, because load may take some time.
      RaiseControlEvents();
    }
    play->SetInUse(true);
    // Only load items that have a minimum of well known properties.
    // In case of enumerable items the content is required, in case of songs the decoder.
    // Both is related to IF_Other. The other informations are prefetched too.
    play->RequestInfo(IF_Tech, PRI_Sync);
    if (play->GetInfo().tech->attributes & TATTR_INVALID)
    { play->SetInUse(false);
      return RC_InvalidItem;
    }
    // Load the required information as fast as possible
    play->RequestInfo(IF_Tech|IF_Obj|IF_Meta|IF_Child, PRI_Normal);
    // Verify all information
    play->RequestInfo(IF_Tech|IF_Obj|IF_Meta|IF_Child|IF_Aggreg, PRI_Low, REL_Confirmed);
    // Track root changes
    play->GetInfoChange() += CurrentRootDelegate;
    // Move always to the first element if a playlist.
    if (AdjustNext(Current()->Loc))
    { // track changes
      UpdateStackUsage(EmptyStack, Current()->Loc.GetCallstack());
      //AttachCurrentSong(ps);
    }
  }
  Pending |= EV_Song;
  DEBUGLOG(("Ctrl::MsgLoad - attached\n"));

  return RC_OK;
}

Ctrl::RC CtrlImp::MsgStopAt(const xstring& iter, PM123_TIME loc, int flags)
{ // TODO: !!!
  return RC_OK;
}

/* saving the currently played stream. */
Ctrl::RC CtrlImp::MsgSave(const xstring& filename)
{ DEBUGLOG(("Ctrl::MsgSave(%s)\n", filename.cdata()));

  if (Savename == filename)
    return RC_OK;
  Pending |= EV_Savename;
  Savename = filename;

  if (Playing)
    dec_save(Savename);
  // TODO: is it really a good idea to save different streams into the same file???
  return RC_OK;
}

/* Adjusts shuffle flag. */
Ctrl::RC CtrlImp::MsgShuffle(Op op)
{ DEBUGLOG(("Ctrl::MsgShuffle(%x) - %u\n", op, Scan));
  if (SetFlag(Shuffle, op))
    Pending |= EV_Shuffle;
  return RC_OK;
}

/* Adjusts repeat flag. */
Ctrl::RC CtrlImp::MsgRepeat(Op op)
{ DEBUGLOG(("Ctrl::MsgRepeat(%x) - %u\n", op, Scan));
  if (SetFlag(Repeat, op))
    Pending |= EV_Repeat;
  return RC_OK;
}

Ctrl::RC CtrlImp::MsgLocation(SongIterator* sip, int flags)
{ DEBUGLOG(("Ctrl::MsgLocation(%p, %x)\n", sip, flags));
  if (!PrefetchList.size())
    return RC_NoSong; // no root
  if (flags & 1)
  { // stopat location
    // TODO: not yet implemented
  } else
  { // Fetch time first because that may change Current().
    PM123_TIME pos = FetchCurrentSongTime();
    *sip = Current()->Loc; // copy
    sip->Navigate(pos, JobSet::SyncJob);
  }
  return RC_OK;
}

// The decoder completed decoding...
Ctrl::RC CtrlImp::MsgDecStop()
{ DEBUGLOG(("Ctrl::MsgDecStop()\n"));
  if (!Playing)
    return RC_NotPlaying;

  if ((GetRoot()->GetInfo().tech->attributes & TATTR_SONG) && !Repeat)
  { // Song, no repeat => stop
   eol:
    DEBUGLOG(("Ctrl::MsgDecStop: flush\n"));
    DecoderStop();
    out_flush();
    // Continue at OUTEVENT_END_OF_DATA
    return RC_OK;
  }

  PrefetchEntry* pep = new PrefetchEntry(Current()->Offset + dec_maxpos(), Current()->Loc);
  pep->Offset += dec_maxpos();
  int dir = Scan == DECFAST_REWIND ? -1 : 1; // DecoderStop resets scan mode
  DecoderStop();

  // Navigation
  if (!(GetRoot()->GetInfo().tech->attributes & TATTR_SONG))
  { if ( ( !SkipCore(pep->Loc, dir, true)
        && (!Repeat || !SkipCore(pep->Loc, dir, true)) )
      || (Repeat && pep->Loc.CompareTo(Current()->Loc, 0, false) == 0) ) // no infinite loop
    { delete pep;
      goto eol; // end of list => same as end of song
    }
  }
  APlayable& ps = pep->Loc.GetCurrent();
  // store result
  Mutex::Lock lock(PLMtx);
  PrefetchList.append() = pep;

  // start decoder for the prefetched item
  DEBUGLOG(("Ctrl::MsgDecStop playing %s with offset %g\n", ps.GetPlayable().URL.cdata(), pep->Offset));
  if (DecoderStart(ps, pep->Offset) != 0)
  { // TODO: we should continue with the next song, and remove the current one from the prefetch list.
    OutputStop();
    Playing = false;
    Pending |= EV_PlayStop;
    return RC_DecPlugErr;
  }

  // Once the player arrives the prefetched item it requests some information.
  // Let's try to prefetch this information at low priority to avoid latencies.
  ps.RequestInfo(IF_Tech|IF_Meta|IF_Obj, PRI_Low, REL_Confirmed);

  // In rewind mode we continue to rewind from the end of the prevois song.
  // TODO: Location problem.
  return RC_OK;
}

// The output completed playing
Ctrl::RC CtrlImp::MsgOutStop()
{ DEBUGLOG(("Ctrl::MsgOutStop()\n"));
  // Check whether we have to remove items in queue mode
  Playable& plp = *Current()->Loc.GetRoot();
  if (Cfg::Get().queue_mode && PrefetchList.size() && &plp == &GUI::GetDefaultPL())
    plp.RemoveItem(Current()->Loc.GetCallstack()[0]);
  // In any case stop the engine
  return MsgPlayStop(Op_Reset);
}

void TFNENTRY ControllerWorkerStub(void*)
{ CtrlImp::Worker();
}

void CtrlImp::Worker()
{ HAB hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(hab, 0);
  for(;;)
  { DEBUGLOG(("Ctrl::Worker() looking for work\n"));
    { sco_ptr<QEntry> qp(Queue.Read());
      CurCmd = qp->Cmd;
      if (CurCmd == NULL)
        break; // deadly pill
    }

    bool fail = false;
    do
    { register ControlCommand* ccp = CurCmd; // Create a local copy
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
          ccp->Flags = MsgJump(*(Location*)ccp->PtrArg);
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
      CurCmd = ccp->Link;
      // cleanup
      if (ccp->Callback)
        (*ccp->Callback)(ccp);
      else
        delete ccp;

    } while (CurCmd);

    // done, raise control event
    RaiseControlEvents();
  }
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}

void CtrlImp::QueueTraverseProxy(const QEntry& entry, void* arg)
{ (*((QueueTraverseProxyData*)arg)->Action)(*entry.Cmd, ((QueueTraverseProxyData*)arg)->Arg);
}

void Ctrl::QueueTraverse(void (*action)(const ControlCommand& cmd, void* arg), void* arg)
{ if (CtrlImp::CurCmd)
    action(*CtrlImp::CurCmd, arg);
  CtrlImp::QueueTraverseProxyData args_proxy = { action, arg };
  Queue.ForEach(&CtrlImp::QueueTraverseProxy, &args_proxy);
}


// Public interface

void Ctrl::Init()
{ DEBUGLOG(("Ctrl::Init()\n"));
  dec_event += CtrlImp::DecEventDelegate;
  out_event += CtrlImp::OutEventDelegate;
  CtrlImp::WorkerTID = _beginthread(&ControllerWorkerStub, NULL, 262144, NULL);
  ASSERT((int)CtrlImp::WorkerTID != -1);
  // load the state
  ctrl_state state;
  load_ini_value(Cfg::GetHIni(), state.volume);
  load_ini_int(Cfg::GetHIni(), state.shf);
  load_ini_int(Cfg::GetHIni(), state.rpt);
  load_ini_xstring(Cfg::GetHIni(), state.current_root);
  load_ini_xstring(Cfg::GetHIni(), state.current_iter);
  load_ini_int(Cfg::GetHIni(), state.was_playing);
  PostCommand(MkShuffle(state.shf ? Op_Set : Op_Clear));
  PostCommand(MkRepeat(state.rpt ? Op_Set : Op_Clear));
  if (state.volume >= 0)
    PostCommand(MkVolume(state.volume, false));
  if (state.current_root)
  { ControlCommand* head = MkLoad(state.current_root, false);
    ControlCommand* tail = head;
    if (state.current_iter)
      tail = tail->Link = MkNavigate(state.current_iter, 0, true, true);
    if (Cfg::Get().restartonstart && state.was_playing)
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
  { Queue.Purge();
    PostCommand(MkLocation(&last, 0));
    PostCommand(MkLoad(xstring(), 0));
    PostCommand(NULL);
    CtrlImp::DecEventDelegate.detach();
    CtrlImp::OutEventDelegate.detach();
  }
  if (CtrlImp::WorkerTID != 0)
    wait_thread_pm(amp_player_hab, CtrlImp::WorkerTID, 30000);

  // save the state
  state.shf = IsShuffle();
  state.rpt = IsRepeat();
  if (!!last)
  { state.current_root = last.GetRoot()->URL;
    // save location only if the current item has definite length.
    state.current_iter = last.Serialize(Cfg::Get().retainonexit && last.GetCurrent().GetInfo().obj->songlength >= 0);
    DEBUGLOG(("last_loc: %s %s\n", state.current_root.cdata(), state.current_iter.cdata()));
  }
  save_ini_value(Cfg::GetHIni(), state.volume);
  save_ini_value(Cfg::GetHIni(), state.shf);
  save_ini_value(Cfg::GetHIni(), state.rpt);
  save_ini_xstring(Cfg::GetHIni(), state.current_root);
  save_ini_xstring(Cfg::GetHIni(), state.current_iter);
  save_ini_value(Cfg::GetHIni(), state.was_playing);

  // Now delete everything
  CtrlImp::PrefetchClear(false);

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


