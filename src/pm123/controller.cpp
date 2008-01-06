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


StatusPlayEnumerator Ctrl::Current;
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



bool Ctrl::SetFlag(bool& flag, Op op)
{ switch (op)
  {default:
    return false;

    return true;
    
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
    Song* pp = Current.GetCurrentSong();
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
  if (!Current.GetRoot())
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
  if (!Current.GetRoot())
    return RC_NoSong;
  BOOL decoder_was_playing = decoder_playing();
  if (relative)
  { if (count == 0)
      return RC_OK;
    if (decoder_was_playing)
      MsgPlayStop(Op_Reset);
    Pending |= EV_Song|EV_Tech|EV_Meta;

    Location = 0;
    if (count < 0)
    { // previous    
      do
      { if (!Current.Prev())
        { Current.Next();
          return RC_EndOfList;
        }
      } while (++count);
    } else
    { // next
      do
      { if (!Current.Next())
        { Current.Prev();
          return RC_EndOfList;
        }
      } while (--count);
    }
  } else
  { // absolute mode
    if (count < 0)
      return RC_BadArg;
    if (Current.GetStatus().CurrentItem == count)
      return RC_OK;
    if (decoder_was_playing)
      MsgPlayStop(Op_Reset);
    Pending |= EV_Song|EV_Tech|EV_Meta;

    Location = 0;
    Current.Reset();
    do
    { if (!Current.Next())
      { Current.Prev();
        return RC_EndOfList;
      }
    } while (--count);
  }    
  if (decoder_was_playing)
    MsgPlayStop(Op_Set);
  return RC_OK;
}

/* Loads Playable object to player. */
Ctrl::RC Ctrl::MsgLoad(const xstring& url)
{ DEBUGLOG(("Ctrl::MsgLoad(%s)\n", url.cdata()));

  // always stop
  MsgPlayStop(Op_Reset);
    
  if (url)
  { int_ptr<Playable> play = Playable::GetByURL(url);
    Current.Attach(play);
    // Move always to the first element.
    Current.Next();
  } else
    Current.Attach(NULL);

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
  (PlayEnumerator::Status&)Status = Current.GetStatus();
  // TODO: in case of seeking -> Location too...
  Status.CurrentSongTime = Playing ? out_playing_pos() : Location;
  int_ptr<Song> song = Current.GetCurrentSong();
  Status.TotalSongTime = song ? song->GetInfo().tech->songlength : -1;
  return RC_OK; 
}

void TFNENTRY ControllerWorkerStub(void*)
{ Ctrl::Worker();
}

void Ctrl::Worker()
{ bool term = false;
  for(;;)
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
