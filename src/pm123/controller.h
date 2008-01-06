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


#ifndef CONTROLLER_H
#define CONTROLLER_H


#include "playenumerator.h"

#include <decoder_plug.h>

#include <cpp/queue.h>
#include <cpp/xstring.h>
#include <cpp/event.h>
#include <cpp/cpputil.h>

#include <debuglog.h>


/* PM123 commands

Command    StrArg              NumArg/PtrArg       Flags               Meaning
===============================================================================================================
Load       URL                                                         Load an URL
                                                                       The URL must be well formed.
---------------------------------------------------------------------------------------------------------------
Skip                           Number of songs     0x01  relative      Move to song number or some songs forward
                                                         navigation    or backward. This makes only sense if the
                                                                       currently loaded object is enumerable.
                                                                       Absolute positioning is not recommended.
                                                                       Use Navigate instead.
---------------------------------------------------------------------------------------------------------------
Navigate   Serialized iterator Location in         0x01  relative      Jump to location
           optional            seconds                   location      This will change the Song and/or the
                                                   0x02  playlist      playing position.
                                                         scope
---------------------------------------------------------------------------------------------------------------
PlayStop                                           0x01  play          Start or stop playing.
                                                   0x02  stop
                                                   0x03  toggle
---------------------------------------------------------------------------------------------------------------
Pause                                              0x01  pause         Set or unset pause.
                                                   0x02  resume        Pause is reset on stop.
                                                   0x03  toggle
---------------------------------------------------------------------------------------------------------------
Scan                                               0x01  scan on       Set/reset forwad/rewind.
                                                   0x02  scan off      If flag 0x04 is not set fast forward is
                                                   0x03  toggle        controlled. Setting fast forward
                                                   0x04  rewind        automatically stops rewinding and vice
                                                                       versa.
---------------------------------------------------------------------------------------------------------------
Volume                         Volume [0..1]       0x01  relative      Set volume to level.
---------------------------------------------------------------------------------------------------------------
Shuffle                                            0x01  enable        Enable/disable random play.
                                                   0x02  disable
                                                   0x03  toggle
---------------------------------------------------------------------------------------------------------------
Repeat                                             0x01  on            Set/reset auto repeat.
                                                   0x02  off
                                                   0x03  toggle
---------------------------------------------------------------------------------------------------------------
Equalize                       EQ_Data*                                Save stream of current decoder.
---------------------------------------------------------------------------------------------------------------
Save       Filename                                                    Save stream of current decoder.
---------------------------------------------------------------------------------------------------------------
Status                         out: PlayStatus*                        Return PlayStatus
---------------------------------------------------------------------------------------------------------------

Commands can have an optional callback function which is called when the command is executed completely.
The callback function must at least delete the ControlCommand instance.
The callback function should not block.
If a ControlCommand has no callback function, it is deleted by the Queue processor.

Commands can be linked by the Link field. Linked commands are executed without interuption by other command sources.
If one of the commands fails, all further linked commands fail immediately with PM123RC_SubseqError too. 
*/


class Ctrl
{public:
  enum Command
  { // current object control
    Cmd_Load,
    Cmd_Skip,
    Cmd_Navigate,
    // play mode control
    Cmd_PlayStop,
    Cmd_Pause,
    Cmd_Scan,
    Cmd_Volume,
    // misc.
    Cmd_Shuffle,
    Cmd_Repeat,
    Cmd_Save,
    Cmd_Equalize,
    // queries
    Cmd_Status
  };

  // Flags for setter commands 
  enum Op
  { Op_Reset  = 0,
    Op_Set    = 1,
    Op_Clear  = 2,
    Op_Toggle = 3,
    Op_Rewind = 4  // PM123_Scan only
  };

  // return codes in Flags
  enum RC
  { RC_OK,
    RC_SubseqError,
    RC_BadArg,
    RC_NoSong,
    RC_EndOfList,
    RC_NotPlaying,
    RC_OutPlugErr,
    RC_DecPlugErr
  };

  struct EQ_Data
  { float bandgain[2][10];
  };

  struct PlayStatus : public PlayEnumerator::Status
  { double CurrentSongTime; // current time index in the current song
    double TotalSongTime;   // total time of the current song
  };

  struct ControlCommand;

  typedef void (*CbComplete)(ControlCommand* cmd);

  struct ControlCommand
  { Command         Cmd;    // Basic command. See PM123Command for details. 
    xstring         StrArg; // String argument (command dependant)
    union
    { double        NumArg; // Integer argument (command dependant)
      void*         PtrArg; // Pointer argument (command dependant)
    };
    int             Flags;  // Flags argument (command dependant)
    CbComplete      Callback;// Notification on completion
    void*           User;   // Unused, for user purposes only 
    ControlCommand* Link;   // Linked commands. They are executed atomically.
    ControlCommand(Command cmd, const xstring& str, double num, int flags, CbComplete cb = NULL)
                    : Cmd(cmd), StrArg(str), NumArg(num), Flags(flags), Callback(cb), Link(NULL) {}
    ControlCommand(Command cmd, const xstring& str, void* ptr, int flags, CbComplete cb = NULL)
                    : Cmd(cmd), StrArg(str), PtrArg(ptr), Flags(flags), Callback(cb), Link(NULL) {}
  };

  enum EventFlags
  { EV_None     = 0x0000,
    EV_PlayStop = 0x0001,
    EV_Pause    = 0x0002,
    EV_Forward  = 0x0004,
    EV_Rewind   = 0x0008,
    EV_Shuffle  = 0x0010,
    EV_Repeat   = 0x0020,
    EV_Volume   = 0x0040,
    EV_Savename = 0x0080,
    EV_Equalize = 0x0100,
    EV_Root     = 0x1000,
    EV_Song     = 0x2000,
    EV_Tech     = 0x4000,
    EV_Meta     = 0x8000
  };

  /*struct ControlEventArgs
  { ControlEventFlags Flags;
  };*/

 private: // working set
  static StatusPlayEnumerator Current;  // Currently loaded object - that's what it's all about!
  static double               Location;
  static bool                 Playing;
  static bool                 Paused;
  static DECFASTMODE          Scan;
  static double               Volume;
  static xstring              Savename;
  static bool                 Shuffle;
  static bool                 Repeat;
  static bool                 EqEnabled;
  static EQ_Data              EqData;

  static queue<ControlCommand*> Queue;
  static TID                  WorkerTID;

  static PlayStatus           Status;
  static EventFlags           Pending;

 private: // internal functions, not thread safe
  static bool SetFlag(bool& flag, Op op);
  static void SetVolume();
  friend void TFNENTRY ControllerWorkerStub(void*);
  static void Worker();
 private: // messages handlers, not thread safe  
  static RC   MsgPause(Op op);
  static RC   MsgScan(Op op);
  static RC   MsgVolume(double volume, bool relative);
  static RC   MsgNavigate(const xstring& iter, double loc, int flags);
  static RC   MsgPlayStop(Op op);
  static RC   MsgSkip(int count, bool relative);
  static RC   MsgLoad(const xstring& url);
  static RC   MsgSave(const xstring& filename);
  static RC   MsgEqualize(const EQ_Data* data);
  static RC   MsgStatus();
 
 public: // management interface, not thread safe
  // initialize controller
  static void Init();
  // uninitialize controller
  static void Uninit();

 public: // properties, thread safe
  static bool IsPlaying()     { return Playing; }
  static bool IsPaused()      { return Paused; }
  static DECFASTMODE GetScan(){ return Scan; }
  static double GetVolume()   { return Volume; }
  static bool IsShuffle()     { return Shuffle; }
  static bool IsRepeat()      { return Repeat; }
  static xstring GetSavename(){ return Savename; }
  /* TODO: need to synchronize to thead */
  static int_ptr<Song> GetCurrentSong()
  { return Current.GetCurrentSong(); }
  static int_ptr<Playable> GetRoot()
  { return Current.GetRoot(); }

 public: //message interface, thread safe
  // post a command to the controller Queue
  static void PostCommand(ControlCommand* cmd)
  { DEBUGLOG(("Ctrl::PostCommand(%p{%i, ...})\n", cmd, cmd ? cmd->Cmd : -1));
    Queue.Write(cmd);
  }
  static void PostCommand(ControlCommand* cmd, CbComplete callback)
  { cmd->Callback = callback;
    PostCommand(cmd);
  }
  static void PostCommand(ControlCommand* cmd, CbComplete callback, void* user)
  { cmd->User = user;
    PostCommand(cmd, callback);
  }
  // Post a command to the controller queue and wait for completion.
  // The function returns the control command which usually must be deleted.
  static ControlCommand* SendCommand(ControlCommand* cmd);

  // short-cuts
  static ControlCommand* MkLoad(const xstring& url)
  { return new ControlCommand(Cmd_Load, url, 0., 0); }
  static ControlCommand* MkSkip(int count, bool relative)
  { return new ControlCommand(Cmd_Skip, xstring(), count, relative); }
  static ControlCommand* MkNavigate(const xstring& iter, double start, bool relative, bool global)
  { return new ControlCommand(Cmd_Navigate, iter, start, relative | (global<<1)); }
  static ControlCommand* MkPlayStop(Op op)
  { return new ControlCommand(Cmd_PlayStop, xstring(), 0., op); }
  static ControlCommand* MkPause(Op op)
  { return new ControlCommand(Cmd_Pause, xstring(), 0., op); }
  static ControlCommand* MkScan(int op)
  { return new ControlCommand(Cmd_Scan, xstring(), 0., op); }
  static ControlCommand* MkVolume(double volume, bool relative)
  { return new ControlCommand(Cmd_Volume, xstring(), volume, relative); }
  static ControlCommand* MkSave(const xstring& filename)
  { return new ControlCommand(Cmd_Save, filename, 0., 0); }
  static ControlCommand* MkEqualize(const EQ_Data* data)
  { return new ControlCommand(Cmd_Equalize, xstring(), (void*)data, 0); }
  static ControlCommand* MkStatus()
  { return new ControlCommand(Cmd_Status, xstring(), (void*)NULL, 0); }

 public: // notifications
  static event<const EventFlags> ChangeEvent;
};

FLAGSATTRIBUTE(Ctrl::EventFlags);

#endif