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

#define INCL_BASE
#include "plugman.h"
#include "controller.h"
#include "properties.h"

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
    DEBUGLOG(("SongIterator::DebugDump %p{%i, %f, %p}\n", *ppce, (*ppce)->Index, (*ppce)->Offset, &*(*ppce)->Item));
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
    DEBUGLOG(("SongIterator::PrevNextCore - @%p\n", &*pi));
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
        DEBUGLOG(("SongIterator::PrevNextCore - relative offset\n"));
        pce->Index += dir;
        if (pce->Offset != -1)
        { Offsets info = TechFromPlayable((dir < 0 ? pi : pce->Item)->GetPlayable());
          // apply change
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
          Offsets pinfo = TechFromPlayable(GetList());
          Offsets iinfo = TechFromPlayable(pi->GetPlayable());
          pce->Index += pinfo.Index;
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
      { DEBUGLOG(("SongIterator::PrevNextCore - %p{%p{%s}}\n", &*pi, pi->GetPlayable(), pi->GetPlayable()->GetURL().getShortName().cdata()));
        return true;
      } else if (!SkipQ()) // skip objects in the call stack
        Enter();
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


/****************************************************************************
*
*  class Ctrl
*
****************************************************************************/

int_ptr<Song>        Ctrl::CurrentSong;
SongIterator         Ctrl::CurrentIter;
double               Ctrl::Location  = 0.;
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

Ctrl::EventFlags     Ctrl::Pending = Ctrl::EV_None;
event<const Ctrl::EventFlags> Ctrl::ChangeEvent;

const SongIterator::CallstackType Ctrl::EmptyStack(1);


int_ptr<Song> Ctrl::GetCurrentSong()
{ DEBUGLOG(("Ctrl::GetCurrentSong() - %p, %p\n", &*CurrentSong, *CurrentIter));
  CritSect cs();
  if (CurrentSong)
    return CurrentSong;
  PlayableInstance* pi = *CurrentIter;
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

/* Suspends or resumes playback of the currently played file. */
Ctrl::RC Ctrl::MsgPause(Op op)
{ DEBUGLOG(("Ctrl::MsgPause(%x) - %u\n", op, Scan)); 
  if (!decoder_playing())
    return op & Op_Set ? RC_NotPlaying : RC_OK;

  if (SetFlag(Paused, op))
  { out_pause(Paused);
    Pending |= EV_Pause;
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
    dec_jump(out_playing_pos());
  // Update event flags
  if ((Scan & DECFAST_FORWARD) != (newscan & DECFAST_FORWARD))
    Pending |= EV_Forward;
  if ((Scan & DECFAST_REWIND) != (newscan & DECFAST_REWIND))
    Pending |= EV_Rewind;
  Scan = newscan;
  SetVolume();
  return RC_OK;
}

Ctrl::RC Ctrl::MsgVolume(double volume, bool relative)
{ DEBUGLOG(("Ctrl::MsgVolume(%f, %u) - %f\n", volume, relative, Volume));
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
Ctrl::RC Ctrl::MsgPlayStop(Op op)
{ DEBUGLOG(("Ctrl::MsgPlayStop(%x) - %u\n", op, Playing));
  if (!SetFlag(Playing, op))
    return RC_OK;

  if (Playing)
  { // start playback
    Song* pp = GetCurrentSong();
    if (pp == NULL)
      return RC_NoSong;
    pp->EnsureInfo(Playable::IF_Format|Playable::IF_Other);
    FORMAT_INFO2 format = *pp->GetInfo().format;
    ULONG rc = out_setup( &format, pp->GetURL() );
    DEBUGLOG(("msg_playstop: after setup %d %d - %d\n", format.samplerate, format.channels, rc));
    if (rc != 0)
      return RC_OutPlugErr;

    SetVolume();
    dec_eq(EqEnabled ? EqData.bandgain[0] : NULL);
    dec_save(Savename);
    dec_fast(Scan);

    rc = dec_play( pp->GetURL(), pp->GetDecoder(), Location );
    if (rc != 0)
    { out_close();
      return RC_DecPlugErr;
    }

    // TODO: CRAP!
    while( dec_status() == DECODER_STARTING )
      DosSleep( 1 );

    Playing = true;
  
  } else
  { // stop playback
    MsgPause(Op_Clear);
    MsgScan(Op_Reset);
    Location = 0;

    // stop decoder
    dec_stop();
    // close output
    out_close();

    // TODO: CRAP! => we should disconnect decoder instead 
    while( decoder_playing())
      DosSleep( 1 );
    
    Playing = false;
  }
  Pending |= EV_PlayStop;
  
  return RC_OK;
}

Ctrl::RC Ctrl::MsgNavigate(const xstring& iter, double loc, int flags)
{ DEBUGLOG(("Ctrl::MsgNavigate(%s, %f, %x)\n", iter.cdata(), loc, flags));
  if (!GetCurrentSong())
    return RC_NoSong;
  // TODO: the whole iterator stuff is missing
  Location = loc;
  if (Playing)
  { if ( dec_jump( loc ) != 0 )
      return RC_DecPlugErr; 
  }
  return RC_OK;
}

Ctrl::RC Ctrl::MsgSkip(int count, bool relative)
{ DEBUGLOG(("Ctrl::MsgSkip(%i, %u) - %u\n", count, relative, decoder_playing()));
  if (!CurrentIter.GetRoot())
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
    MsgPlayStop(Op_Reset);
  Location = 0;
  SongIterator si = CurrentIter;
  if (relative)
  { if (count < 0)
    { // previous    
      do
      { if (!si.Prev())
          return RC_EndOfList;
      } while (++count);
    } else
    { // next
      do
      { if (!si.Next())
          return RC_EndOfList;
      } while (--count);
    }
  } else
  { si.Reset();
    do
    { if (!si.Next())
        return RC_EndOfList;
    } while (--count);
  }
  // commit
  /*CurrentIter.DebugDump();
  si.DebugDump();*/
  { CritSect cs;
    CurrentIter.Swap(si);
  }
  /*CurrentIter.DebugDump();
  si.DebugDump();*/
  UpdateStackUsage(si.GetCallstack(), CurrentIter.GetCallstack());

  Pending |= EV_Song|EV_Tech|EV_Meta;
  if (decoder_was_playing)
    MsgPlayStop(Op_Set);
  return RC_OK;
}

/* Loads Playable object to player. */
Ctrl::RC Ctrl::MsgLoad(const xstring& url)
{ DEBUGLOG(("Ctrl::MsgLoad(%s)\n", url.cdata()));

  // always stop
  MsgPlayStop(Op_Reset);

  // detach
  if (CurrentSong)
  { CurrentSong->SetInUse(false);
    CurrentSong = NULL;
  }
  if (CurrentIter.GetRoot())
  { UpdateStackUsage(CurrentIter.GetCallstack(), EmptyStack);
    CurrentIter.GetRoot()->SetInUse(false);
    CurrentIter.SetRoot(NULL); 
  }
  
  if (url)
  { int_ptr<Playable> play = Playable::GetByURL(url);
    // Only load items that have a minimum of well known properties.
    // In case of enumerable items the content is required, in case of songs the decoder.
    // Both is related to IF_Other. The other informations are prefetched too.
    play->EnsureInfo(Playable::IF_All);
    if (play->GetFlags() & Playable::Enumerable)
    { CurrentIter.SetRoot((PlayableCollection*)&*play);
      play->SetInUse(true);
      // Move always to the first element.
      if (CurrentIter.Next())
        UpdateStackUsage(EmptyStack, CurrentIter.GetCallstack());
    } else
    { CurrentSong = (Song*)&*play;
      play->SetInUse(true);
    }
  }
  Pending |= EV_Root|EV_Song|EV_Tech|EV_Meta;
  DEBUGLOG(("Ctrl::MsgLoad - attached\n"));

  return RC_OK;
}

/* saving the currently played stream. */
Ctrl::RC Ctrl::MsgSave(const xstring& filename)
{ DEBUGLOG(("Ctrl::MsgSave(%s)\n", filename.cdata()));

  if (Savename == filename)
    return RC_OK;
  Pending |= EV_Savename;
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
  Pending |= EV_Equalize;
  return RC_OK;
}

Ctrl::RC Ctrl::MsgStatus()
{ DEBUGLOG(("Ctrl::MsgStatus()\n"));
  Status.CurrentSongTime = Playing ? out_playing_pos() : Location;
  if (CurrentSong)
  { CurrentSong->EnsureInfo(Playable::IF_Tech);
    Status.TotalSongTime   = CurrentSong->GetInfo().tech->songlength;
    Status.CurrentItem     = -1;
    Status.TotalItems      = -1;
    Status.CurrentTime     = -1;
    Status.TotalTime       = -1;
  } else if (CurrentIter.GetRoot())
  { Status.TotalSongTime   = *CurrentIter ? CurrentIter->GetPlayable()->GetInfo().tech->songlength : -1;
    SongIterator::Offsets off = CurrentIter.GetOffset();
    Status.CurrentItem     = off.Index;
    Status.TotalItems      = CurrentIter.GetRoot()->GetInfo().tech->num_items;
    Status.CurrentTime     = off.Offset >= 0 ? off.Offset + Status.CurrentSongTime : -1;
    Status.TotalTime       = CurrentIter.GetRoot()->GetInfo().tech->songlength;
  } else
    return RC_NoSong;
  return RC_OK; 
}

void TFNENTRY ControllerWorkerStub(void*)
{ Ctrl::Worker();
}

void Ctrl::Worker()
{ for(;;)
  { DEBUGLOG(("Ctrl::Worker() looking for work\n"));
    queue<ControlCommand*>::Reader rdr(Queue);
    ControlCommand* qp = rdr;
    if (qp == NULL)
      break; // deadly pill
    DEBUGLOG(("Ctrl::Worker received message: %p{%i, %s, %f, %x, %p, %p}\n",
      qp, qp->Cmd, qp->StrArg, qp->NumArg, qp->Flags, qp->Callback, qp->Link));

    bool fail = false;
    do
    { if (fail)
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
    if (Pending)
    { ChangeEvent(Pending);
      Pending = EV_None;
    }
  }
}


// Public interface

void Ctrl::Init()
{ DEBUGLOG(("Ctrl::Init()\n"));
  WorkerTID = _beginthread(&ControllerWorkerStub, NULL, 65536, NULL);
  ASSERT((int)WorkerTID != -1);
}

void Ctrl::Uninit()
{ DEBUGLOG(("Ctrl::Uninit()\n"));
  { Mutex::Lock l(Queue.Mtx);
    Queue.Purge();
    PostCommand(MkPlayStop(Op_Reset));
    PostCommand(NULL);
  }
  if (WorkerTID != 0)
    DosWaitThread(&WorkerTID, DCWW_WAIT);
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

// TODO:
/*

AT msg_stop:
  
  while( WinPeekMsg( hab, &qms, hplayer, WM_PLAYSTOP, WM_PLAYSTOP, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_PLAYSTOP message.\n" ));
  }
  while( WinPeekMsg( hab, &qms, hplayer, WM_OUTPUT_OUTOFDATA, WM_OUTPUT_OUTOFDATA, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_OUTPUT_OUTOFDATA message.\n" ));
  }
  while( WinPeekMsg( hab, &qms, hplayer, WM_PLAYERROR, WM_PLAYERROR, PM_REMOVE )) {
    DEBUGLOG(( "pm123: discards WM_PLAYERROR message.\n" ));
  }


*/
