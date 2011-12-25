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
#include "dependencyinfo.h"
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
class CtrlImp
: public Ctrl::ControlCommand // Derive to access command arguments directly
, public Ctrl                 // Derive to access protected members only
{ friend class Ctrl;
 private:
  struct PrefetchEntry
  { PM123_TIME                Offset;        // Starting time index from the output's point of view.
    SongIterator              Loc;           // Location that points to the song that starts at this position.
    //PrefetchEntry() : Offset(0) {}
    PrefetchEntry(PM123_TIME offset, const SongIterator& loc) : Offset(offset), Loc(loc) {}
  };

 private: // extended working set
  /// List of prefetched iterators.
  /// The first entry is always the current iterator if a enumerable object is loaded.
  /// Write access to this list is only done by the controller thread.
  static vector<PrefetchEntry> PrefetchList;
  static Mutex                PLMtx;                 // Mutex to protect the above list.

  static TID                  WorkerTID;             // Thread ID of the worker
  static Ctrl::ControlCommand* CurCmd;               // Currently processed ControlCommand, protected by Queue.Mtx

  /// Pending Events. These events are set atomically from any thread.
  /// After each last message of a queued set of messages the events are raised and Pending is atomically reset.
  static AtomicUnsigned       Pending;

  // Delegates for the tracked events of the decoder, the output and the current song.
  static delegate<void, const dec_event_args>        DecEventDelegate;
  static delegate<void, const OUTEVENTTYPE>          OutEventDelegate;
  //static delegate<void, const PlayableChangeArgs>    CurrentSongDelegate;
  static delegate<void, const PlayableChangeArgs>    CurrentRootDelegate;
  //static delegate<void, const CallstackEntry>        SongIteratorDelegate;

  // Occasionally used constant.
  static const vector_int<PlayableInstance> EmptyStack;
  static JobSet SyncJob;

 private: // internal functions, not thread safe
  /// Returns the current PrefetchEntry. I.e. the currently playing (or not playing) iterator.
  /// Precondition: an object must have been loaded.
  static PrefetchEntry* Current() { return PrefetchList[0]; }
  /// Raise any pending controller events.
  static void  RaiseControlEvents();

 private: // Helper functions for command handlers
  /// Reply a result code and clear the error text.
         void  Reply(RC rc) { Flags = rc; StrArg.reset(); }
  /// Reply a argument error
         void  ReplyBadArg(const char* fmt, ...);
  /// Reply a decoder error
         void  ReplyDecoderError(ULONG err);
  /// Reply a decoder error
         void  ReplyOutputError(ULONG err);
  /// Applies the operator op to flag and returns true if the flag has been changed.
  /// Implicitly updates the reply fields \c Flags and \c StrVal.
         bool  SetFlag(bool& flag);
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
         bool  SkipCore(Location& si, int count);
  // Ensure that a SongIterator really points to a valid song by moving the iterator forward as required.
  static bool  AdjustNext(Location& si);
  // Jump to the location si. The function will destroy the content of si.
         void  NavigateCore(Location& si);
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
  // Event handler for asynchronous changes to the SongIterator (not any prefetched one).
  //static void  SongIteratorEventHandler(void*, const CallstackEntry& ce);

 private: // messages handlers, not thread safe
  // The messages are described before the class header.
  void  MsgNop     ();
  void  MsgPause   ();
  void  MsgScan    ();
  void  MsgVolume  ();
  void  MsgNavigate();
  void  MsgJump    ();
  void  MsgStopAt  ();
  void  MsgPlayStop();
  void  MsgSkip    ();
  void  MsgLoad    ();
  void  MsgSave    ();
  void  MsgShuffle ();
  void  MsgRepeat  ();
  void  MsgLocation();
  void  MsgDecStop ();
  void  MsgOutStop ();

 private: // non-constructable, non-copyable
  CtrlImp();
  CtrlImp(const CtrlImp&);
  void  operator=(const CtrlImp&);
 public:
  static void Execute(Ctrl::ControlCommand& cc);

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
JobSet CtrlImp::SyncJob(PRI_Sync);

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
  DEBUGLOG(("Ctrl::RaiseControlEvents: %x\n", events));
  if (events)
    ChangeEvent(events);
}


void CtrlImp::ReplyBadArg(const char* fmt, ...)
{ Flags = RC_BadArg;
  va_list va;
  va_start(va, fmt);
  StrArg = xstring::vsprintf(fmt, va);
  va_end(va);
}
void CtrlImp::ReplyDecoderError(ULONG err)
{ Flags = RC_DecPlugErr;
  NumArg = err;
  StrArg = xstring::sprintf("Decoder plug-in failed with error code %li.", err);
}
void CtrlImp::ReplyOutputError(ULONG err)
{ Flags = RC_DecPlugErr;
  NumArg = err;
  StrArg = xstring::sprintf("Output plug-in failed with error code %li.", err);
}

bool CtrlImp::SetFlag(bool& flag)
{ switch (Flags)
  {default:
    ReplyBadArg("Invalid boolean operator %x.", Flags);
    return false;

   case Op_Set:
    if (flag)
      break;
    goto toggle;
   case Op_Clear:
   case Op_Reset:
    if (!flag)
      break;
   case Op_Toggle:
   toggle:
    flag = !flag;
    Reply(RC_OK);
    return true;
  }
  Reply(RC_OK);
  return false;
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

bool CtrlImp::SkipCore(Location& si, int count)
{ DEBUGLOG(("Ctrl::SkipCore({%s}, %i)\n", si.Serialize().cdata(), count));
  const Location::NavigationResult& rc = si.NavigateCount(count, TATTR_SONG, SyncJob);
  if (rc)
  { StrArg = rc;
    Flags = RC_BadIterator;
    return false;
  }
  return true;
}

bool CtrlImp::AdjustNext(Location& si)
{ DEBUGLOG(("Ctrl::AdjustNext({%s})\n", si.Serialize().cdata()));
  APlayable& ps = si.GetCurrent();
  ps.RequestInfo(IF_Tech|IF_Child, PRI_Sync);
  if (ps.GetInfo().tech->attributes & TATTR_SONG)
    return true;
  const Location::NavigationResult& rc = si.NavigateCount(1, TATTR_SONG, SyncJob);
  DEBUGLOG(("Ctrl::AdjustNext: %s\n", rc.cdata()));
  return !rc;  
}

void CtrlImp::NavigateCore(Location& si)
{ DEBUGLOG(("Ctrl::NavigateCore({%s}) - %s\n", si.Serialize().cdata(), Current()->Loc.Serialize().cdata()));
  // Check whether the current song has changed?
  int level = si.CompareTo(Current()->Loc);
  DEBUGLOG(("Ctrl::NavigateCore - %i\n", level));
  if (level == 0)
  { Reply(RC_OK); // song and location identical => no-op
    return;
  }
  ASSERT(level != INT_MIN);
  if (abs(level) > max(si.GetLevel(), Current()->Loc.GetLevel()))
  { DEBUGLOG(("Ctrl::NavigateCore - seek to %f\n", Current()->Loc.GetPosition()));
    // only location is different => seek only
    if (Playing)
    { ULONG rc = dec_jump(si.GetPosition());
      if (rc)
      { ReplyDecoderError(rc);
        return;
    } }
    Mutex::Lock lock(PLMtx);
    Current()->Loc.Swap(si);
    Reply(RC_OK);
    return;
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
  { ULONG rc = DecoderStart(ps, 0);
    if (rc)
    { OutputStop();
      Playing = false;
      Pending |= EV_PlayStop;
      ReplyDecoderError(rc);
      return;
    }
  }
  Reply(RC_OK);
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
          if (pe.Loc.NavigateCount(1, TATTR_SONG, SyncJob, 1))// we played the last item of a top level entry
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


void CtrlImp::MsgNop()
{ Flags = RC_OK;
}

/* Suspends or resumes playback of the currently played file. */
void CtrlImp::MsgPause()
{ DEBUGLOG(("Ctrl::MsgPause() {%x} - %u\n", Flags, Scan));
  if (!Playing)
  { Reply(Flags & Op_Set ? RC_NotPlaying : RC_OK);
    return;
  }
  if (SetFlag(Paused))
  { out_pause(Paused);
    Pending |= EV_Pause;
  }
}

/* change scan mode logically */
void CtrlImp::MsgScan()
{ DEBUGLOG(("Ctrl::MsgScan() {%x} - %u\n", Flags, Scan));
  if (Flags & ~7)
  { ReplyBadArg("Invalid scan argument %x", Flags);
    return;
  }
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
  DECFASTMODE newscan = opmatrix[Flags][Scan];
  // Check for NOP.
  if (Scan != newscan)
  { if (Playing)
    { // => Decoder
      // TODO: discard prefetch buffer.
      ULONG rc = dec_fast(newscan);
      if (rc)
      { ReplyDecoderError(rc);
        return;
      } else // if (cfg.trash)
        // Going back in the stream to what is currently playing.
        dec_jump(FetchCurrentSongTime());

    } else if (Flags & Op_Set)
    { Reply(RC_NotPlaying);
      return;
    }
    // Update event flags
    if ((Scan & DECFAST_FORWARD) != (newscan & DECFAST_FORWARD))
      Pending |= EV_Forward;
    if ((Scan & DECFAST_REWIND) != (newscan & DECFAST_REWIND))
      Pending |= EV_Rewind;
    Scan = newscan;
    SetVolume();
  }
  Reply(RC_OK);
}

void CtrlImp::MsgVolume()
{ DEBUGLOG(("Ctrl::MsgVolume() {%g, %u} - %g\n", NumArg, Flags, Volume));
  if (Flags)
    NumArg += Volume;
  // Limits
  if (NumArg < 0)
    NumArg = 0;
  else if (NumArg > 1)
    NumArg = 1;

  if (NumArg != Volume)
  { Volume = NumArg;
    SetVolume();
    Pending |= EV_Volume;
  }
  Reply(RC_OK);
}

/* change play/stop status */
void CtrlImp::MsgPlayStop()
{ DEBUGLOG(("Ctrl::MsgPlayStop() {%x} - %u\n", Flags, Playing));

  if (Playing)
  { // Set new playing position
    if ( Cfg::Get().retainonstop && Flags != Op_Reset
      && Current()->Loc.GetCurrent().GetInfo().obj->songlength > 0 )
    { PM123_TIME time = FetchCurrentSongTime();
      Current()->Loc.Navigate(time, SyncJob);
    } else
    { int_ptr<Location> start = Current()->Loc.GetCurrent().GetStartLoc();
      Current()->Loc.Navigate(start ? start->GetPosition() : 0, SyncJob);
    }
  }

  if (!SetFlag(Playing))
    return;

  if (Playing)
  { // start playback
    APlayable* pp = GetCurrentSong();
    if (pp == NULL)
    { Playing = false;
      Reply(RC_NoSong);
      return;
    }

    pp->RequestInfo(IF_Decoder|IF_Tech|IF_Obj|IF_Slice, PRI_Sync);
    if (!(pp->GetInfo().tech->attributes & TATTR_SONG))
    { Playing = false;
      Reply(RC_NoSong);
      return;
    }

    ULONG rc = OutputStart(*pp);
    if (rc)
    { Playing = false;
      ReplyOutputError(rc);
      return;
    }

    Current()->Offset = 0;
    rc = DecoderStart(Current()->Loc.GetCurrent(), 0);
    if (rc)
    { OutputStop();
      Playing = false;
      ReplyOutputError(rc);
      return;
    }

  } else
  { // stop playback
    DecoderStop();
    OutputStop();

    Flags = Op_Clear;
    if (SetFlag(Paused))
      Pending |= EV_Pause;
    Flags = Op_Reset;
    MsgScan();

    while (out_playing_data())
    { DEBUGLOG(("Ctrl::MsgPlayStop - Spinlock\n"));
      DosSleep(1);
    }
  }
  Pending |= EV_PlayStop;

  Reply(RC_OK);
}

void CtrlImp::MsgNavigate()
{ DEBUGLOG(("Ctrl::MsgNavigate() {%s, %g, %x}\n", StrArg.cdata(), NumArg, Flags));
  if (!GetCurrentSong())
  { Reply(RC_NoSong);
    return;
  }
  sco_ptr<Location> sip;
  if (Flags & 0x02)
  { // Reset location
    sip = new Location(&GetRoot()->GetPlayable());
  } else
  { // Start from current location
    // We must fetch the current playing time first, because this may change Current().
    PM123_TIME time = FetchCurrentSongTime();
    sip = new Location(Current()->Loc);
    sip->Navigate(time, SyncJob);
  }
  if (StrArg && StrArg.length())
  { const char* cp = StrArg.cdata();
    const Location::NavigationResult rc = sip->Deserialize(cp, SyncJob);
    if (rc && !(Flags & 0x04))
    { StrArg = rc;
      NumArg = cp - StrArg.cdata();
      Flags = RC_BadIterator;
      return;
    }
    // Move forward to the next Song, if the current item is a playlist.
    AdjustNext(*sip);
  } else
  { if (Flags & 0x01)
      NumArg += sip->GetPosition();
    const Location::NavigationResult rc = sip->Navigate(NumArg, SyncJob);
    if (rc && !(Flags & 0x04))
    { StrArg = rc;
      NumArg = -1;
      Flags = RC_BadIterator;
      return;
    }
  }
  // TODO: extend total playing time when leaving bounds of parent iterator?

  // commit
  NavigateCore(*sip);
}

void CtrlImp::MsgJump()
{ DEBUGLOG(("Ctrl::MsgJump() {%p}\n", PtrArg));
  APlayable* ps = GetRoot();
  if (!ps)
    Reply(RC_NoSong);
  else if (&ps->GetPlayable() != ((Location*)PtrArg)->GetRoot())
    Reply(RC_InvalidItem);
  else
    NavigateCore(*(Location*)PtrArg);
}

void CtrlImp::MsgSkip()
{ DEBUGLOG(("Ctrl::MsgSkip() {%g, %x} - %u\n", NumArg, Flags, IsPlaying));
  APlayable* pp = GetRoot();
  if (!pp)
  { Reply(RC_NoSong);
    return;
  }
  pp->RequestInfo(IF_Tech|IF_Child, PRI_Sync);
  if (!(pp->GetInfo().tech->attributes & TATTR_PLAYLIST))
  { Reply(RC_NoList);
    return;
  }
  // some checks
  if (Flags)
  { if (NumArg == 0)
    { Reply(RC_OK);
      return;
    }
  } else
  { // absolute mode
    if (NumArg < 0)
    { ReplyBadArg("Negative index %g in playlist???", NumArg);
      return;
    }
    /* TODO: ...
    if (Current.GetStatus().CurrentItem == count)
      return RC_OK;*/
  }

  // Navigation
  Location si = Current()->Loc; // work on a temporary object => copy constructor
  if (!Flags)
    si.Reset();
  if (!SkipCore(si, (int)NumArg))
  { if (Cfg::Get().autoturnaround)
    { si.Reset();
      switch ((int)NumArg)
      {case 1:
       case -1:
        if (!si.NavigateCount((int)NumArg, TATTR_SONG, SyncJob))
          goto ok;
      }
    }
    Reply(RC_EndOfList);
    return;
  }
 ok:
  // commit
  NavigateCore(si);
}

/* Loads Playable object to player. */
void CtrlImp::MsgLoad()
{ DEBUGLOG(("Ctrl::MsgLoad() {%s, %x}\n", StrArg.cdata(), Flags));
  xstring url = StrArg;
  int flags = Flags;

  // always stop
  // TODO: continue flag
  Flags = Op_Reset;
  MsgPlayStop();

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
    play->RequestInfo(IF_Tech|IF_Child, PRI_Sync);
    if (play->GetInfo().tech->attributes & TATTR_INVALID)
    { play->SetInUse(false);
      Reply(RC_InvalidItem);
      return;
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

  Reply(RC_OK);
}

void CtrlImp::MsgStopAt()//const xstring& iter, PM123_TIME loc, int flags)
{ // TODO: !!!
  Reply(RC_OK);
}

/* saving the currently played stream. */
void CtrlImp::MsgSave()
{ DEBUGLOG(("Ctrl::MsgSave() {%s}\n", StrArg.cdata()));

  if (Savename == StrArg)
  { Reply(RC_OK);
    return;
  }
  Pending |= EV_Savename;
  Savename = StrArg;

  if (Playing)
  { ULONG rc = dec_save(Savename);
    if (rc)
    { ReplyDecoderError(rc);
      return;
  } }
  // TODO: is it really a good idea to save different streams into the same file???
  Reply(RC_OK);
}

/* Adjusts shuffle flag. */
void CtrlImp::MsgShuffle()
{ DEBUGLOG(("Ctrl::MsgShuffle() {%x} - %u\n", Flags, Scan));
  if (SetFlag(Shuffle))
    Pending |= EV_Shuffle;
}

/* Adjusts repeat flag. */
void CtrlImp::MsgRepeat()
{ DEBUGLOG(("Ctrl::MsgRepeat() {%x} - %u\n", Flags, Scan));
  if (SetFlag(Repeat))
    Pending |= EV_Repeat;
}

void CtrlImp::MsgLocation()
{ DEBUGLOG(("Ctrl::MsgLocation() {%p, %x}\n", PtrArg, Flags));
  if (!PrefetchList.size())
  { Reply(RC_NoSong); // no root
    return;
  }
  if (Flags & 1)
  { // stopat location
    // TODO: not yet implemented
  } else
  { // Fetch time first because that may change Current().
    SongIterator*& sip = (SongIterator*&)PtrArg;
    PM123_TIME pos = FetchCurrentSongTime();
    *sip = Current()->Loc; // copy
    sip->Navigate(pos, SyncJob);
  }
  Reply(RC_OK);
}

// The decoder completed decoding...
void CtrlImp::MsgDecStop()
{ DEBUGLOG(("Ctrl::MsgDecStop()\n"));
  if (!Playing)
  { Reply(RC_NotPlaying);
    return;
  }

  if ((GetRoot()->GetInfo().tech->attributes & TATTR_SONG) && !Repeat)
  { // Song, no repeat => stop
   eol:
    DEBUGLOG(("Ctrl::MsgDecStop: flush\n"));
    DecoderStop();
    out_flush();
    // Continue at OUTEVENT_END_OF_DATA
    Reply(RC_OK);
    return;
  }

  PrefetchEntry* pep = new PrefetchEntry(Current()->Offset + dec_maxpos(), Current()->Loc);
  pep->Offset += dec_maxpos();
  int dir = Scan == DECFAST_REWIND ? -1 : 1; // DecoderStop resets scan mode
  DecoderStop();

  // Navigation
  if (!(GetRoot()->GetInfo().tech->attributes & TATTR_SONG))
  { if ( ( !SkipCore(pep->Loc, dir)
        && (!Repeat || !SkipCore(pep->Loc, dir)) )
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
  ULONG rc = DecoderStart(ps, pep->Offset);
  if (rc)
  { // TODO: we should continue with the next song, and remove the current one from the prefetch list.
    OutputStop();
    Playing = false;
    Pending |= EV_PlayStop;
    ReplyDecoderError(rc);
    return;
  }

  // Once the player arrives the prefetched item it requests some information.
  // Let's try to prefetch this information at low priority to avoid latencies.
  ps.RequestInfo(IF_Tech|IF_Meta|IF_Obj, PRI_Low, REL_Confirmed);

  // In rewind mode we continue to rewind from the end of the previous song.
  // TODO: Location problem.
  Reply(RC_OK);
}

// The output completed playing
void CtrlImp::MsgOutStop()
{ DEBUGLOG(("Ctrl::MsgOutStop()\n"));
  // Check whether we have to remove items in queue mode
  Playable& plp = *Current()->Loc.GetRoot();
  if (Cfg::Get().queue_mode && PrefetchList.size() && &plp == &GUI::GetDefaultPL())
    plp.RemoveItem(Current()->Loc.GetCallstack()[0]);
  // In any case stop the engine
  Flags = Op_Reset;
  MsgPlayStop();
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
    { register Ctrl::ControlCommand* ccp = CurCmd; // Create a local copy
      DEBUGLOG(("Ctrl::Worker received message: %p{%i, %s, %08x%08x, %x, %p, %p} - %u\n",
        ccp, ccp->Cmd, ccp->StrArg ? ccp->StrArg.cdata() : "<null>", ccp->PtrArg, (&ccp->PtrArg)[1], ccp->Flags, ccp->Callback, ccp->Link, fail));
      static void (CtrlImp::*const cmds[])() =
      { &CtrlImp::MsgNop
      , &CtrlImp::MsgLoad
      , &CtrlImp::MsgSkip
      , &CtrlImp::MsgNavigate
      , &CtrlImp::MsgJump
      , &CtrlImp::MsgStopAt
      , &CtrlImp::MsgPlayStop
      , &CtrlImp::MsgPause
      , &CtrlImp::MsgScan
      , &CtrlImp::MsgVolume
      , &CtrlImp::MsgShuffle
      , &CtrlImp::MsgRepeat
      , &CtrlImp::MsgSave
      , &CtrlImp::MsgLocation
      , &CtrlImp::MsgDecStop
      , &CtrlImp::MsgOutStop
      };
      if (fail)
      { ccp->StrArg.reset();
        ccp->Flags = RC_SubseqError;
      } else if ((unsigned)ccp->Cmd >= sizeof cmds / sizeof *cmds)
      { ccp->StrArg = xstring::sprintf("Invalid control command %i", ccp->Cmd);
        ccp->Flags = RC_BadArg;
      } else
      { // Do the work
        (((CtrlImp*)ccp)->*cmds[ccp->Cmd])();
        // Always clear dependencies - they make no sense anyway.
        SyncJob.Rollback();
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
    PostCommand(MkPlayStop(Op_Reset));
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

