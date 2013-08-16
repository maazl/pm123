/*
 * Copyright 2007-2013 M.Mueller
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
#include "../core/job.h"
#include "../core/songiterator.h"
#include "../configuration.h"
#include "decoder.h"
#include "glue.h"
#include "../pm123.h"
#include "../gui/gui.h" // for DefaultPl

#include <inimacro.h>
#include <minmax.h>
#include <utilfct.h>
#include <cpp/mutex.h>
#include <cpp/xstring.h>

#include <limits.h>

#include <debuglog.h>


/* Ini file stuff, a bit dirty */
struct ctrl_state
{ double  volume;        ///< Position of the volume slider
  xstring current_root;  ///< The currently loaded root.
  xstring current_iter;  ///< The current location within the root.
  bool    shf;           ///< The state of the "Shuffle" button.
  bool    rpt;           ///< The state of the "Repeat" button.
  bool    was_playing;   ///< Restart playback on start-up
  ctrl_state() : volume(-1), shf(false), rpt(false), was_playing(false) {}
};


/// Private implementation of class \c Ctrl.
class CtrlImp
: private Ctrl::ControlCommand // Derive to access command arguments directly
, public Ctrl                 // Derive to access protected members only
{ friend class Ctrl;
 private:
  struct PrefetchEntry
  { PM123_TIME                Offset;        ///< Starting time index from the output's point of view.
    SongIterator              Loc;           ///< Location that points to the song that starts at this position.
    //PrefetchEntry() : Offset(0) {}
    PrefetchEntry(PM123_TIME offset, const SongIterator& loc) : Offset(offset), Loc(loc) {}
  };

 private: // extended working set
  /// List of prefetched iterators.
  /// @details The first entry is always the current iterator if a enumerable object is loaded.
  /// Write access to this list is only done by the controller thread.
  static vector<PrefetchEntry> PrefetchList;

  static TID                  WorkerTID;          ///< Thread ID of the worker
  static Ctrl::ControlCommand* CurCmd;            ///< Currently processed ControlCommand, protected by Queue.Mtx
  /// Pending Events. These events are set atomically from any thread.
  /// After each last message of a queued set of messages the events are raised and Pending is atomically reset.
  static AtomicUnsigned       Pending;

  static APlayable*           LastStart;          ///< Weak reference to the last item successfully played or the start item. Must not be dereferenced!
  static bool                 IsSeeking;          ///< True during seek operations. This prevents calls to \c OutPlayingPos().

  static int_ptr<SongIterator> CurLocShadow;      ///< Shadow of CurLoc to allow safe concurrent modifications w/o reallocations.

  // Delegates for the tracked events of the decoder, the output and the current song.
  static delegate<void, const Glue::DecEventArgs> DecEventDelegate;
  static delegate<void, const OUTEVENTTYPE>       OutEventDelegate;
  static delegate<void, const PlayableChangeArgs> CurrentRootDelegate;

  // Occasionally used constants.
  static const vector_int<PlayableInstance> EmptyStack;

 private: // internal functions, not thread safe
  /// Returns the current PrefetchEntry. I.e. the currently playing (or not playing) iterator.
  /// @pre an object must have been loaded.
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
  /// Sets the volume according to this->Volume and the scan mode.
  static void  SetVolume();
  /// Initializes the decoder engine and starts playback of the song pp with a time offset for the output.
  /// @param pe Start decoding of the entry \a pe at the location in \c pe.Loc.
  /// @param reverse in case of \c DECFAST_REWIND start playing from behind rather than Loc.
  /// @return The function returns the result of dec_play.
  /// The function does not return unless the decoder is decoding or an error occurred.
  /// @pre The output must have been initialized.
  static ULONG DecoderStart(PrefetchEntry& pe, bool reverse);
  /// Initializes the output for playing pp.
  /// The playable object is needed for naming purposes.
  static ULONG OutputStart(APlayable& pp, PM123_TIME offset);
  /// Stops playback and clears the prefetch list.
  /// This also stops the decoder.
  static void  OutputStop();
  /// Updates the in-use status of PlayableInstance objects in the callstack by moving from \a oldstack to \a newstack.
  /// The status of common parts of the two stacks is not touched.
  /// To set the in-use status initially pass EmptyStack as \a oldstack.
  /// To reset all in-use status pass EmptyStack as \a newstack.
  static void  UpdateStackUsage(const vector<PlayableInstance>& oldstack, const vector<PlayableInstance>& newstack);
  /// Core logic of MsgSkip.
  /// Move the current song pointer by count items.
  /// If we try to move the current song pointer out of the range of the that that \a si relies on,
  /// the function returns false and \a si is in reset state.
         bool  SkipCore(SongIterator& si, int count);
  /// Jump to the location si. The function will destroy the content of \a si.
  /// @param newscan The parameter \a newscan can be used to update the scan mode at the same time.
  /// If it is non-zero a the action takes place. Therefore some additional bit have to be set
  /// if the scan mode should be set to \c DECFAST_NORMAL_PLAY.
  enum NavigateResult
  { NR_OK      ///< Navigation succeeded.
  , NR_Failed  ///< Navigation error, Error info placed in ControlCommand.
  , NR_NoScan  ///< Navigation OK, but failed to set scan mode.
  }            NavigateCore(Location& si, signed char newscan = 2);
  /// Request some information on the current song.
  static void  AttachCurrentSong();
  /// Clears the prefetch list and keep the first element if keep is true.
  /// The operation is atomic.
  static void  PrefetchClear(bool keep);
  /// Check whether a prefetched item is completed.
  /// This is the case when the offset of the next item in PrefetchList is less than or equal to pos.
  /// In case a prefetch entry is removed from PrefetchList the in-use status is refreshed and the song
  /// change event is set.
  static void  CheckPrefetch(PM123_TIME pos);
  /// Get the time in the current Song. This may cleanup the prefetch list by calling CheckPrefetch.
  static PM123_TIME FetchCurrentSongTime();
  /// Internal stub to provide the TFNENTRY calling convention for _beginthread.
  friend void TFNENTRY ControllerWorkerStub(void*);
  /// Worker thread that processes the message queue.
  static void  Worker();
  /// Event handler for decoder events.
  static void  DecEventHandler(void*, const Glue::DecEventArgs& args);
  /// Event handler for output events.
  static void  OutEventHandler(void*, const OUTEVENTTYPE& event);
  /// Event handler for tracking modifications of the currently playing song.
  //static void  CurrentSongEventHandler(void*, const PlayableChangeArgs& args);
  /// Event handler for tracking modifications of the currently loaded object.
  static void  CurrentRootEventHandler(void*, const PlayableChangeArgs& args);

 private: // messages handlers, not thread safe
  // The messages are described in controller.h.
  void  MsgNop     ();
  void  MsgPause   ();
  void  MsgScan    ();
  void  MsgVolume  ();
  void  MsgNavigate();
  void  MsgJump    ();
  void  MsgPlayStop();
  void  MsgSkip    ();
  void  MsgLoad    ();
  void  MsgSave    ();
  void  MsgShuffle ();
  void  MsgRepeat  ();
  void  MsgLocation();
  void  MsgDecStop ();
  void  MsgSeekStop();
  void  MsgOutStop ();

 private: // non-constructible, non-copyable
  CtrlImp();
  CtrlImp(const CtrlImp&);
  void  operator=(const CtrlImp&);
 public:
  static void Execute(Ctrl::ControlCommand& cc);

 private:
  struct QueueTraverseProxyData
  { bool  (*Action)(const Ctrl::ControlCommand& cmd, void* arg);
    void* Arg;
  };
  static bool QueueTraverseProxy(const Ctrl::QEntry& entry, void* arg);
};


void Ctrl::ControlCommand::Destroy()
{ if (Link)
    Link->Destroy();
  delete this;
}


vector<CtrlImp::PrefetchEntry> CtrlImp::PrefetchList(10);
double                Ctrl::Volume    = 1.;
xstring               Ctrl::Savename;
bool                  Ctrl::Paused    = false;
signed char           Ctrl::Scan      = 0;
bool                  Ctrl::Shuffle   = false;
bool                  Ctrl::Repeat    = false;

queue<Ctrl::QEntry>   Ctrl::Queue;
TID                   CtrlImp::WorkerTID = 0;
Ctrl::ControlCommand* CtrlImp::CurCmd    = NULL;
AtomicUnsigned        CtrlImp::Pending   = Ctrl::EV_None;
APlayable*            CtrlImp::LastStart = NULL;
bool                  CtrlImp::IsSeeking = false;

volatile int_ptr<SongIterator> Ctrl::CurLoc(new SongIterator());
int_ptr<SongIterator> CtrlImp::CurLocShadow(new SongIterator());

event<const Ctrl::EventFlags> Ctrl::ChangeEvent;

delegate<void, const Glue::DecEventArgs> CtrlImp::DecEventDelegate(&CtrlImp::DecEventHandler);
delegate<void, const OUTEVENTTYPE>       CtrlImp::OutEventDelegate(&CtrlImp::OutEventHandler);
delegate<void, const PlayableChangeArgs> CtrlImp::CurrentRootDelegate(&CtrlImp::CurrentRootEventHandler);

const vector_int<PlayableInstance> CtrlImp::EmptyStack;

int_ptr<APlayable> Ctrl::GetCurrentSong()
{ return int_ptr<SongIterator>(CurLoc)->GetCurrent();
}

int_ptr<APlayable> Ctrl::GetRoot()
{ return int_ptr<SongIterator>(CurLoc)->GetRoot();
}

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
  StrArg.vsprintf(fmt, va);
  va_end(va);
}
void CtrlImp::ReplyDecoderError(ULONG err)
{ Flags = RC_DecPlugErr;
  NumArg = err;
  StrArg.sprintf("Decoder plug-in failed with error code %li.", err);
}
void CtrlImp::ReplyOutputError(ULONG err)
{ Flags = RC_DecPlugErr;
  NumArg = err;
  StrArg.sprintf("Output plug-in failed with error code %li.", err);
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
  Glue::OutSetVolume(volume);
}

ULONG CtrlImp::DecoderStart(PrefetchEntry& pe, bool reverse)
{ DEBUGLOG(("Ctrl::DecoderStart(&%p{%f, {%p, %s}}, %u)\n", &pe, pe.Offset, pe.Loc.GetRoot(), pe.Loc.Serialize().cdata(), reverse));

  APlayable& song = *pe.Loc.GetCurrent();
  ASSERT(&song);
  song.RequestInfo(IF_Tech|IF_Slice|IF_Obj, PRI_Sync|PRI_Normal);

  PM123_TIME start = 0;
  { int_ptr<Location> lp = song.GetStartLoc();
    if (lp && lp->GetPosition() > 0)
      start = lp->GetPosition();
  }
  PM123_TIME stop = 0;
  { int_ptr<Location> lp = song.GetStopLoc();
    if (lp && lp->GetPosition() >= 0)
      stop = lp->GetPosition();
  }
  PM123_TIME at = start;
  if (pe.Loc.GetPosition() >= 0)
    at = pe.Loc.GetPosition();
  
  if (Scan < 0 && reverse)
  { if (stop > 0)
    { at = stop - 1.; // do not seek to the end, because this will cause problems.
    } else if (song.GetInfo().obj->songlength > 0)
    { at = song.GetInfo().obj->songlength - 1.;
    } else
    { // no songlength => error => undo MsgScan
      Scan = 0;
      Pending |= EV_Scan;
    }
    if (at < start) // Do not hit negative values for very short songs.
      at = start;
  }

  IsSeeking = at > 0;
  ULONG rc = Glue::DecPlay(song, pe.Offset-start, at, stop);
  if (rc != 0)
    return rc;

  if (Scan)
  { if (Glue::DecFast(Scan * Cfg::Get().scan_speed - 1))
    { Scan = 0;
      Pending |= EV_Scan;
    }
  }

  if (LastStart == NULL)
    LastStart = &song;
  DEBUGLOG(("Ctrl::DecoderStart - completed\n"));

  // Once the player arrives the prefetched item it requests some information.
  // Let's try to prefetch this information at low priority to avoid latencies.
  song.RequestInfo(IF_Meta|IF_Obj, PRI_Low, REL_Confirmed);
  return 0;
}

ULONG CtrlImp::OutputStart(APlayable& pp, PM123_TIME offset)
{ DEBUGLOG(("Ctrl::OutputStart(&%p)\n", &pp));
  ULONG rc = Glue::OutSetup(pp, offset);
  DEBUGLOG(("Ctrl::OutputStart: after setup - %d\n", rc));
  if (rc != PLUGIN_OK)
    Glue::OutClose();
  SetVolume();
  return rc;
}

void CtrlImp::OutputStop()
{ DEBUGLOG(("Ctrl::OutputStop()\n"));
  // Clear prefetch list
  PrefetchClear(true);
  // close output
  Glue::OutClose();
  // reset offset
  if (PrefetchList.size())
    Current()->Offset = 0;
}

void CtrlImp::UpdateStackUsage(const vector<PlayableInstance>& oldstack, const vector<PlayableInstance>& newstack)
{ DEBUGLOG(("Ctrl::UpdateStackUsage({%u }, {%u })\n", oldstack.size(), newstack.size()));
  const size_t limit = min(oldstack.size(), newstack.size());
  size_t level;
  // skip identical part
  for (level = 0; level < limit && oldstack[level] == newstack[level]; ++level)
    DEBUGLOG(("Ctrl::UpdateStackUsage identical - %p == %p\n", oldstack[level], newstack[level]));
  size_t i;
  // reset usage flags of part of old stack
  for (i = oldstack.size(); i > level;)
    oldstack[--i]->SetInUse(0);
  // set usage flags of part of the new stack
  for (i = level; i < newstack.size(); ++i)
    newstack[i]->SetInUse(i + 2); // Leave 0 and 1 for unused and current root.
}

bool CtrlImp::SkipCore(SongIterator& si, int count)
{ DEBUGLOG(("Ctrl::SkipCore({%s}, %i)\n", si.Serialize().cdata(), count));
  const SongIterator::NavigationResult& rc = si.NavigateCount(Job::SyncJob, count, TATTR_SONG);
  if (rc)
  { StrArg = rc;
    Flags = RC_BadIterator;
    return false;
  }
  return true;
}

CtrlImp::NavigateResult CtrlImp::NavigateCore(Location& si, signed char newscan)
{ DEBUGLOG(("Ctrl::NavigateCore({%s}, %u) - %s\n", si.Serialize().cdata(), newscan, Current()->Loc.Serialize().cdata()));
  SongIterator& curloc = PrefetchList[PrefetchList.size()-1]->Loc;
  // Move forward to the next Song, if the current item is a playlist.
  //Current()->Loc.NavigateCount(SyncJob, 0, TATTR_SONG);
  const bool was_playing = Glue::OutInitialized();

  // Check whether the current song has changed?
  int level = si.CompareTo(curloc);
  DEBUGLOG(("Ctrl::NavigateCore - %i\n", level));
  ASSERT(level != INT_MIN);
  level = abs(level);
  if (level > (int)si.GetCallstack().size() && level > (int)curloc.GetCallstack().size())
  { // only location is different => seek only
    DEBUGLOG(("Ctrl::NavigateCore - seek to %f\n", si.GetPosition()));
    Pending |= EV_Location;
    curloc.Swap(si);
    if (was_playing)
    { if (Glue::DecInitialized())
      { ULONG rc = Glue::DecJump(curloc.GetPosition());
        if (rc)
        { ReplyDecoderError(rc);
          return NR_Failed;
        }
        IsSeeking = true;
        if (newscan != 2)
        { int speed = newscan * Cfg::Get().scan_speed;
          if (speed)
            --speed;
          ULONG rc = Glue::DecFast(speed);
          if (rc)
          { ReplyDecoderError(rc);
            return NR_NoScan;
          }
          Scan = newscan;
          Pending |= EV_Scan;
        }
      } else
      { // The decoder has quit => reactivate it.
        Scan = newscan;
        Pending |= EV_Scan;
        ULONG rc = DecoderStart(*Current(), false);
        if (rc)
        { OutputStop();
          ReplyDecoderError(rc);
          return NR_Failed;
        }
      }
      Glue::OutTrash(); // discard buffers
    }
    Reply(RC_OK);
    return NR_OK;
  }
  // Navigate to another item
  DEBUGLOG(("Ctrl::NavigateCore - Navigate - %u\n", was_playing));
  if (was_playing)
  { if (Glue::DecInitialized())
      Glue::DecStop();
    Glue::OutTrash(); // discard buffers
    PrefetchClear(true);
  }
  // deregister current song delegate
  //CurrentSongDelegate.detach();
  // Events
  UpdateStackUsage(Current()->Loc.GetCallstack(), si.GetCallstack());
  Pending |= EV_Song|EV_Location;

  // restart decoder immediately?
  if (was_playing)
  { PrefetchEntry* pep = new PrefetchEntry(Current()->Offset + Glue::DecMaxPos(), Current()->Loc);
    pep->Loc.Swap(si);
    PrefetchList.append() = pep;
    AttachCurrentSong();
    ULONG rc = DecoderStart(*pep, false);
    if (rc)
    { OutputStop();
      Pending |= EV_PlayStop;
      ReplyDecoderError(rc);
      return NR_Failed;
    }
  } else
  { Current()->Offset = 0;
    Current()->Loc.Swap(si);
    AttachCurrentSong();
  }
  DEBUGLOG(("Ctrl::NavigateCore succeeded - %s -> %s\n", si.Serialize().cdata(), Current()->Loc.Serialize().cdata()));
  Reply(RC_OK);
  return NR_OK;
}

void CtrlImp::AttachCurrentSong()
{ DEBUGLOG(("Ctrl::AttachCurrentSong()\n"));
  if (!PrefetchList.size())
    return;
  APlayable* cur = Current()->Loc.GetCurrent();
  if (cur)
    cur->RequestInfo(IF_Tech|IF_Obj|IF_Meta|IF_Rpl|IF_Drpl, PRI_Normal);
  //ps.GetInfoChange() += CurrentSongDelegate;
  // Update CurLoc
  if (!CurLocShadow->RefCountIsUnique())
    CurLocShadow = new SongIterator();
  *CurLocShadow = Current()->Loc;
  CurLoc.swap(CurLocShadow);
}

void CtrlImp::PrefetchClear(bool keep)
{ DEBUGLOG(("Ctrl::PrefetchClear(%u)\n", keep));
  PrefetchEntry*const* where = PrefetchList.end();
  while (PrefetchList.size() > keep) // Hack: keep = false deletes all items while keep = true kepp the first item.
    delete PrefetchList.erase(--where);
}

void CtrlImp::CheckPrefetch(double pos)
{ DEBUGLOG(("Ctrl::CheckPrefetch(%g) - %u\n", pos, PrefetchList.size()));
  if (PrefetchList.size())
  { size_t n = 1;
    // Since the item #1 is likely to compare less than CurrentSongTime a linear search is faster than a binary search.
    while (n < PrefetchList.size() && pos >= PrefetchList[n]->Offset)
      ++n;
    --n;
    // now n contains the number of items to remove
    DEBUGLOG(("Ctrl::CheckPrefetch %g, %g -> %u\n", pos, Current()->Offset, n));
    if (n)
    { // At least one prefetched item has been played completely.
      PrefetchEntry* keep = PrefetchList[n];
      UpdateStackUsage(Current()->Loc.GetCallstack(), keep->Loc.GetCallstack());
      // Set events
      Pending |= EV_Song;

      // delete played items from default playlist in queue mode.
      Playable& default_pl = GUI::GetDefaultPL();
      if (Cfg::Get().queue_mode && Current()->Loc.GetRoot() == &default_pl)
      { ASSERT(Current()->Loc.GetCallstack().size());
        ASSERT(keep->Loc.GetCallstack().size());
        PlayableInstance* item = Current()->Loc.GetCallstack()[0];
        PlayableInstance* stop = keep->Loc.GetCallstack()[0];
        while (item != stop)
          item = default_pl.RemoveItem(*item);
      }

      // Cleanup prefetch list
      vector<PrefetchEntry> ped(n);
      do
        delete PrefetchList.erase(--n);
      while (n);

      // Now keep track of the next entry
      AttachCurrentSong();
    }
  }
}

PM123_TIME CtrlImp::FetchCurrentSongTime()
{ if (Glue::OutInitialized() && !IsSeeking)
  { PM123_TIME time = Glue::OutPlayingPos();
    // Check whether the output played a prefetched item completely.
    CheckPrefetch(time);
    DEBUGLOG(("Ctrl::FetchCurrentSongTime: %f (play)\n", time - Current()->Offset));
    return time - Current()->Offset; // relocate playing position
  } else
  { DEBUGLOG(("Ctrl::FetchCurrentSongTime: %f\n", Current()->Loc.GetPosition()));
    return Current()->Loc.GetPosition();
  }
}

void CtrlImp::DecEventHandler(void*, const Glue::DecEventArgs& args)
{ DEBUGLOG(("Ctrl::DecEventHandler(, {%i, %p})\n", args.Type, args.Param));
  switch (args.Type)
  {case DECEVENT_PLAYERROR:
    // Decoder error => next, please (if any)
   case DECEVENT_PLAYSTOP:
    // Well, same as on play error.
    PostCommand(MkDecStop());
    break;
   case DECEVENT_SEEKSTOP:
    PostCommand(MkSeekStop());
    break;
    /* currently unused
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

void CtrlImp::CurrentRootEventHandler(void*, const PlayableChangeArgs& args)
{ DEBUGLOG(("Ctrl::CurrentRootEventHandler(, {%p{%s}, %x, %x})\n",
    &args.Instance, args.Instance.DebugName().cdata(), args.Changed, args.Loaded));
  { const int_ptr<APlayable>& ps = GetRoot();
    if (!ps || ps != &args.Instance)
      return; // too late...
  }
  // Ensure invalidated infos to reload.
  if (args.Invalidated & (IF_Tech|IF_Obj|IF_Meta|IF_Child|IF_Aggreg))
    args.Instance.RequestInfo(args.Invalidated & (IF_Tech|IF_Obj|IF_Meta|IF_Child|IF_Aggreg), PRI_Low, REL_Confirmed);
}


void CtrlImp::MsgNop()
{ Reply(RC_OK);
}

/* Suspends or resumes playback of the currently played file. */
void CtrlImp::MsgPause()
{ DEBUGLOG(("Ctrl::MsgPause() {%x} - %u\n", Flags, Scan));
  if (!Glue::OutInitialized())
  { Reply(Flags & Op_Set ? RC_NotPlaying : RC_OK);
    return;
  }
  if (SetFlag(Paused))
  { Glue::OutPause(Paused);
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
  static const signed char opmatrix[8][3] =
  { { 0, 0, 0}
  , { 1, 1, 1}
  , {-1, 0, 0}
  , { 1, 1, 0}
  , {-1, 0, 1}
  , {-1,-1,-1}
  , { 0, 0, 1}
  , { 0,-1,-1}
  };
  signed char newscan = opmatrix[Flags][Scan+1];
  // Check for NOP.
  if (Scan != newscan)
  { if (Glue::OutInitialized())
    { // => Decoder
      // Going back in the stream to what is currently playing.
      SongIterator si(Current()->Loc);
      si.NavigateRewindSong();
      si.NavigateTime(Job::SyncJob, FetchCurrentSongTime(), si.GetCallstack().size());
      if (NavigateCore(si, newscan) == NR_Failed)
        return;
    } else
    { if (Flags & Op_Set)
      { Reply(RC_NotPlaying);
        return;
      }
      Scan = newscan;
      Pending |= EV_Scan;
    }
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
{ bool playing = Glue::OutInitialized();
  DEBUGLOG(("Ctrl::MsgPlayStop() {%x} - %u\n", Flags, playing));

  if (!SetFlag(playing))
    return;

  if (playing)
  { // start playback
    APlayable* pp = GetCurrentSong();
    if (pp == NULL)
    { Reply(RC_NoSong);
      return;
    }

    pp->RequestInfo(IF_Decoder|IF_Tech|IF_Obj|IF_Slice, PRI_Sync|PRI_Normal);
    if (!(pp->GetInfo().tech->attributes & TATTR_SONG))
    { Reply(RC_NoSong);
      return;
    }

    ULONG rc = OutputStart(*pp, 0);
    if (rc)
    { ReplyOutputError(rc);
      return;
    }

    Current()->Offset = 0;
    rc = DecoderStart(*Current(), true);
    if (rc)
    { OutputStop();
      ReplyDecoderError(rc);
      return;
    }

  } else
  { // Fetch current playling position
    SongIterator& si = Current()->Loc;
    si.NavigateRewindSong();
    // Set new playing position
    if ( Cfg::Get().retainonstop && Flags != Op_Reset
      && si.GetCurrent()->GetInfo().obj->songlength > 0 )
    { PM123_TIME time = FetchCurrentSongTime();
      if (time >= 0)
        si.NavigateTime(Job::SyncJob, time, si.GetCallstack().size(), true);
    }

    // stop playback
    Glue::DecStop();
    Glue::OutTrash();
    OutputStop();

    Flags = Op_Clear;
    if (SetFlag(Paused))
      Pending |= EV_Pause;

    Flags = Op_Reset;
    MsgScan();

    while (Glue::OutPlayingData())
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

  sco_ptr<SongIterator> sip;
  if (Flags & NT_Global)
  { // Reset location
    sip = new SongIterator(&GetRoot()->GetPlayable());
  } else if ((Flags & NT_Relative) == 0)
  { // Start from current location
    // We must fetch the current playing time first, because this may change Current().
    PM123_TIME time = FetchCurrentSongTime();
    sip = new SongIterator(Current()->Loc);
    sip->SetOptions(PLO_NONE);
    sip->NavigateRewindSong();
    if (time >= 0)
      sip->NavigateTime(Job::SyncJob, time);
  } else
  { sip = new SongIterator(Current()->Loc);
    sip->SetOptions(PLO_NONE);
  }

  if (StrArg && StrArg.length())
  { const char* cp = StrArg.cdata();
    const SongIterator::NavigationResult rc = sip->Deserialize(Job::SyncJob, cp);
    if (rc && !(Flags & NT_Partial))
    { StrArg = rc;
      NumArg = cp - StrArg.cdata();
      Flags = RC_BadIterator;
      return;
    }
  } else
  { if (!(Flags & 0x01))
      sip->NavigateRewindSong();
    const SongIterator::NavigationResult rc = sip->NavigateTime(Job::SyncJob, NumArg);
    if (rc && !(Flags & NT_Partial))
    { StrArg = rc;
      NumArg = -1;
      Flags = RC_BadIterator;
      return;
    }
  }

  // commit
  NavigateCore(*sip);
}

void CtrlImp::MsgJump()
{ DEBUGLOG(("Ctrl::MsgJump() {%p}\n", PtrArg));
  APlayable* ps = GetRoot();
  if (!ps)
  { Reply(RC_NoSong);
    return;
  }
  Location& loc = *(Location*)PtrArg;
  if (&ps->GetPlayable() != loc.GetRoot())
  { // try partial navigation
    Location loc2(Current()->Loc);
    StrArg = loc2.NavigateTo(loc);
    if (StrArg)
    { Flags = RC_InvalidItem;
      return;
    }
    loc.Swap(loc2);
  }
  NavigateCore(loc);
}

void CtrlImp::MsgSkip()
{ DEBUGLOG(("Ctrl::MsgSkip() {%g, %x} - %u\n", NumArg, Flags, IsPlaying()));
  APlayable* pp = GetRoot();
  if (!pp)
  { Reply(RC_NoSong);
    return;
  }
  pp->RequestInfo(IF_Tech|IF_Child, PRI_Sync|PRI_Normal);
  if (!(pp->GetInfo().tech->attributes & TATTR_PLAYLIST) && NumArg != 0)
  { Reply(RC_NoList);
    return;
  }

  // Navigation
  SongIterator si(Current()->Loc); // work on a temporary object => copy constructor
  if (!Flags)
    si.Reset();
  if (!SkipCore(si, (int)NumArg))
  { if (Flags && Cfg::Get().autoturnaround)
    { switch ((int)NumArg)
      {case 1:
       case -1:
        si.Reset();
        if (!si.NavigateCount(Job::SyncJob, (int)NumArg, TATTR_SONG))
          goto ok;
      }
    }
    Reply(RC_EndOfList);
    return;
  }
  // Detect no-op
  if (Current()->Loc.GetCurrent() == si.GetCurrent())
  { Reply(RC_OK);
    return;
  }
 ok:
  // commit
  NavigateCore(si);
}

/* Loads Playable object to player. */
void CtrlImp::MsgLoad()
{ DEBUGLOG(("Ctrl::MsgLoad() {%p, %x}\n", PtrArg, Flags));
  int_ptr<APlayable> play;
  play.fromCptr((APlayable*)PtrArg);

  // If keepitem is requested and we have a new root and an old root...
  if ((Flags & 1) && play && PrefetchList.size())
  { // ... then try to keep the current item alive.
    APlayable* oldroot = PrefetchList[0]->Loc.GetRoot();
    // Is it a null operation?
    if (oldroot == play)
      goto ok; // no-op

    // Now check whether the current root is a descendant of play.
    SongIterator loc(oldroot);
    if (!loc.Navigate(Job::SyncJob, play, -1, 0, INT_MAX))
    { // Got it! => relocate call stack
      unsigned level = loc.GetLevel();
      foreach (PrefetchEntry,**, pepp, PrefetchList)
      { SongIterator& si = (*pepp)->Loc;
        loc.NavigateTo(si);
        si = loc;
        // restore location for next prefetch entry.
        loc.NavigateUp(loc.GetLevel() - level);
      }
      Pending |= EV_Root;
      goto ok;
    }
    // Failed => do normal load
  }

  // always stop
  // TODO: continue flag
  Flags = Op_Reset;
  MsgPlayStop();

  // detach
  //CurrentSongDelegate.detach();
  CurrentRootDelegate.detach();
  if (PrefetchList.size())
  { UpdateStackUsage(Current()->Loc.GetCallstack(), EmptyStack);
    Current()->Loc.GetRoot()->SetInUse(0);
    PrefetchClear(false);
  }

  if (play)
  { { PrefetchEntry* pe = new PrefetchEntry(0, SongIterator(play));
      pe->Loc.Reshuffle();
      pe->Loc.SetOptions(Shuffle * PLO_SHUFFLE);
      // move to the first song. -> LoadHelper does the job
      //pe->Loc.NavigateCount(Job::SyncJob, 0, TATTR_SONG);
      PrefetchList.append() = pe;
      // assign change event handler
      //Current()->Iter.Change += SongIteratorDelegate;
      Pending |= EV_Root|EV_Song|EV_Location;
    }
    play->SetInUse(1);
    // Load the required information as fast as possible
    play->RequestInfo(IF_Tech|IF_Obj|IF_Meta|IF_Child, PRI_Normal);
    // Verify all information
    play->RequestInfo(IF_Aggreg, PRI_Low, REL_Confirmed);
    // Track root changes
    play->GetInfoChange() += CurrentRootDelegate;
    // track changes
    UpdateStackUsage(EmptyStack, Current()->Loc.GetCallstack());
  }
  AttachCurrentSong();
  Pending |= EV_Song|EV_Location;
  LastStart = NULL;
  DEBUGLOG(("Ctrl::MsgLoad - attached\n"));
 ok:
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

  if (Glue::DecInitialized())
  { ULONG rc = Glue::DecSave(Savename);
    if (rc)
    { ReplyDecoderError(rc);
      return;
  } }
  // TODO: is it really a good idea to save different streams into the same file???
  Reply(RC_OK);
}

void CtrlImp::MsgShuffle()
{ DEBUGLOG(("Ctrl::MsgShuffle() {%x} - %u\n", Flags, Scan));
  if (SetFlag(Shuffle))
    Pending |= EV_Shuffle;
  const PL_OPTIONS options = Shuffle * PLO_SHUFFLE;
  const bool reshuffle = !Shuffle && Cfg::Get().discardseed;
  foreach (PrefetchEntry,**, pepp, PrefetchList)
  { SongIterator& si = (*pepp)->Loc;
    si.SetOptions(options);
    if (reshuffle)
      si.Reshuffle();
  }
}

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
  // Ensure unique target
  if (!CurLocShadow->RefCountIsUnique())
    CurLocShadow = new SongIterator();
  // Fetch time first because that may change Current().
  PM123_TIME pos = -1;
  if (Flags != LM_ReturnSong)
    pos = FetchCurrentSongTime();
  *CurLocShadow = Current()->Loc; // copy
  if (pos >= 0)
  { CurLocShadow->NavigateRewindSong();
    CurLocShadow->NavigateTime(Job::SyncJob, pos, CurLocShadow->GetCallstack().size(), true);
  }
  // Return a value?
  if (Flags != LM_UpdatePos)
    PtrArg = int_ptr<SongIterator>(CurLocShadow).toCptr();
  // now *loc is frozen, update CurLoc
  CurLoc.swap(CurLocShadow);
  Reply(RC_OK);
}

// The decoder completed decoding...
void CtrlImp::MsgDecStop()
{ DEBUGLOG(("Ctrl::MsgDecStop()\n"));
  // TODO: we should check here whether the DecStop messages comes from the currently decodes stream.
  // It could be intercepted by a navigation message.
  if (!Glue::DecInitialized())
  { Reply(RC_NotPlaying);
    return;
  }

  // Acknowledge stop signal.
  Glue::DecStop();
  // wait for the decoder to complete
  Glue::DecClose();

  bool root_is_song = (GetRoot()->GetInfo().tech->attributes & TATTR_SONG) != 0;
  if (root_is_song && !Repeat)
  { // Song, no repeat => stop
   eol:
    DEBUGLOG(("Ctrl::MsgDecStop: flush\n"));
    // and then flush the output.
    Glue::OutFlush();
    // Continue at OUTEVENT_END_OF_DATA
    Reply(RC_OK);
    return;
  }

  PM123_TIME max = Glue::DecMaxPos();
  // Check whether the last decode action actually succeeded.
  if (max > Glue::DecMinPos())
    LastStart = NULL;

  PrefetchEntry* pep = new PrefetchEntry(Current()->Offset + max, Current()->Loc);
  int dir = Scan < 0 ? -1 : 1;

  // Navigation
  if (pep->Loc.NavigateCount(Job::SyncJob, dir, TATTR_SONG))
  { // End of current root
    if (!Repeat)
    { delete pep;
      goto eol;
    }
    pep->Loc.Reset();
    if ( (!root_is_song && pep->Loc.NavigateCount(Job::SyncJob, dir, TATTR_SONG))
      || pep->Loc.GetCurrent() == LastStart ) // Stop if there are no playable items
    { delete pep;
      goto eol;
    }
  }

  // store result
  PrefetchList.append() = pep;
  // Tell the output chain what we are going to play.
  OutputStart(*pep->Loc.GetCurrent(), pep->Offset);
  // start decoder for the prefetched item
  ULONG rc = DecoderStart(*pep, true);
  if (rc)
  { // TODO: we should continue with the next song, and remove the current one from the prefetch list.
    OutputStop();
    Pending |= EV_PlayStop;
    ReplyDecoderError(rc);
    return;
  }

  // In rewind mode we continue to rewind from the end of the previous song.
  // TODO: Location problem.
  Reply(RC_OK);
}

// A seek command completed
void CtrlImp::MsgSeekStop()
{ DEBUGLOG(("Ctrl::MsgSeekStop()\n"));
  IsSeeking = false;
}

// The output completed playing
void CtrlImp::MsgOutStop()
{ DEBUGLOG(("Ctrl::MsgOutStop()\n"));
  // Check whether we have to remove items in queue mode
  PrefetchEntry* pe = Current();
  ASSERT(pe);
  APlayable* plp = pe->Loc.GetRoot();
  if (Cfg::Get().queue_mode && PrefetchList.size() && plp == &GUI::GetDefaultPL())
  { PlayableInstance* pip = pe->Loc.GetCallstack()[0];
    if (pip)
      plp->GetPlayable().RemoveItem(*pip);
  }
  pe->Loc.Reshuffle();
  // Reset the iterator to the first song.
  pe->Loc.Reset();
  // move to the first song.
  pe->Loc.NavigateCount(Job::SyncJob, 0, TATTR_SONG);
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
      static void (CtrlImp::*const cmds[])() = // MUST match enum Command
      { &CtrlImp::MsgNop
      , &CtrlImp::MsgLoad
      , &CtrlImp::MsgSkip
      , &CtrlImp::MsgNavigate
      , &CtrlImp::MsgJump
      , &CtrlImp::MsgPlayStop
      , &CtrlImp::MsgPause
      , &CtrlImp::MsgScan
      , &CtrlImp::MsgVolume
      , &CtrlImp::MsgShuffle
      , &CtrlImp::MsgRepeat
      , &CtrlImp::MsgSave
      , &CtrlImp::MsgLocation
      , &CtrlImp::MsgDecStop
      , &CtrlImp::MsgSeekStop
      , &CtrlImp::MsgOutStop
      };
      if (fail)
      { ccp->StrArg.reset();
        ccp->Flags = RC_SubseqError;
      } else if ((unsigned)ccp->Cmd >= sizeof cmds / sizeof *cmds)
      { ccp->StrArg.sprintf("Invalid control command %i", ccp->Cmd);
        ccp->Flags = RC_BadArg;
      } else
      { // Do the work
        (((CtrlImp*)ccp)->*cmds[ccp->Cmd])();
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

      // done, raise control event
      RaiseControlEvents();
    } while (CurCmd);
  }
  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}

bool CtrlImp::QueueTraverseProxy(const QEntry& entry, void* arg)
{ return (*((QueueTraverseProxyData*)arg)->Action)(*entry.Cmd, ((QueueTraverseProxyData*)arg)->Arg);
}

bool Ctrl::QueueTraverse(bool (*action)(const ControlCommand& cmd, void* arg), void* arg)
{ if (CtrlImp::CurCmd)
    action(*CtrlImp::CurCmd, arg);
  CtrlImp::QueueTraverseProxyData args_proxy = { action, arg };
  return Queue.ForEach(&CtrlImp::QueueTraverseProxy, &args_proxy);
}


// Public interface

void Ctrl::Init()
{ DEBUGLOG(("Ctrl::Init()\n"));
  Glue::GetDecEvent() += CtrlImp::DecEventDelegate;
  Glue::GetOutEvent() += CtrlImp::OutEventDelegate;
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
  { // TODO: restore playlist item context also.
    ControlCommand* head = MkLoad(Playable::GetByURL(state.current_root), false);
    ControlCommand* tail = head;
    if (state.current_iter)
      tail = tail->Link = MkNavigate(state.current_iter, 0, NT_Relative|NT_Global);
    else
      tail = tail->Link = MkSkip(0, true);
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
  { Queue.Purge();
    SendCommand(MkLocation(LM_UpdatePos));
    int_ptr<SongIterator> last(CurLoc);
    if (last->GetRoot())
    { state.current_root = last->GetRoot()->GetPlayable().URL;
      // save location only if the current item has definite length.
      state.current_iter = last->Serialize(Cfg::Get().retainonexit && last->GetCurrent()->GetInfo().obj->songlength >= 0);
      DEBUGLOG(("last_loc: %s %s\n", state.current_root.cdata(), state.current_iter.cdata()));
    }
    PostCommand(MkLoad(NULL, false));
    PostCommand(NULL);
    CtrlImp::DecEventDelegate.detach();
    CtrlImp::OutEventDelegate.detach();
  }
  if (CtrlImp::WorkerTID != 0)
    wait_thread_pm(amp_player_hab, CtrlImp::WorkerTID, 30000);

  // save the state
  state.shf = IsShuffle();
  state.rpt = IsRepeat();
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

bool Ctrl::IsPlaying()
{ return Glue::OutInitialized();
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

