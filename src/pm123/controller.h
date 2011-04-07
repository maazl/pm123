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


#ifndef CONTROLLER_H
#define CONTROLLER_H


#include "aplayable.h"
#include "location.h"
#include "glue.h"

#include <decoder_plug.h>

#include <cpp/queue.h>
#include <cpp/xstring.h>
#include <cpp/event.h>
#include <cpp/mutex.h>
#include <cpp/cpputil.h>

#include <debuglog.h>


/* PM123 controller class.
 * All playback activities are controlled by this class.
 * This class is static (singleton).

 * PM123 control commands

Command    StrArg              NumArg/PtrArg       Flags               Meaning
===============================================================================================================
Nop                                                                    No operation (used to raies events)
---------------------------------------------------------------------------------------------------------------
Load       URL                                     0x01  continue      Load an URL
                                                         playing       The URL must be well formed.
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
                                                   0x04  ignore
                                                         syntax error
---------------------------------------------------------------------------------------------------------------
Jump                           in/out:                                 Jump to location
                               Location*                               This will change the Song and/or the
                                                                       playing position.
                                                                       The old position is returned in place.
---------------------------------------------------------------------------------------------------------------
StopAt     Serialized iterator Location in         0x02  playlist      Stop at location
           optional            seconds                   scope         This will stop the playback at the
                                                                       specified location.
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
Save       Filename                                                    Save stream of current decoder.
---------------------------------------------------------------------------------------------------------------
Location                       out SongIterator*   0x01  stopat        Set the iterator to the current location.
---------------------------------------------------------------------------------------------------------------
DecStop                                                                The current decoder finished it's work.
---------------------------------------------------------------------------------------------------------------
OutStop                                                                The output finished playback.
---------------------------------------------------------------------------------------------------------------

Commands can have an optional callback function which is called when the command is executed completely.
The callback function must at least delete the ControlCommand instance.
The callback function should not block.
If a ControlCommand has no callback function, it is deleted by the Queue processor.

Commands can be linked by the Link field. Linked commands are executed without interruption by other command sources.
If one of the commands fails, all further linked commands fail immediately with PM123RC_SubseqError too. 
*/
class Ctrl
{public:
  enum Command // Control commands, see above.
  { Cmd_Nop,
    // current object control
    Cmd_Load,
    Cmd_Skip,
    Cmd_Navigate,
    Cmd_Jump,
    Cmd_StopAt,
    // play mode control
    Cmd_PlayStop,
    Cmd_Pause,
    Cmd_Scan,
    Cmd_Volume,
    // misc.
    Cmd_Shuffle,
    Cmd_Repeat,
    Cmd_Save,
    // queries
    Cmd_Location,
    // internal events
    Cmd_DecStop,
    Cmd_OutStop
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
  { RC_OK,                  // Everything OK.
    RC_SubseqError,         // The command is not processed because an earlier command in the current set of linked commands has failed.
    RC_BadArg,              // Invalid command.
    RC_NoSong,              // The command requires a current song.
    RC_NoList,              // The command is only valid for enumerable objects like playlists.
    RC_EndOfList,           // The navigation tried to move beyond the limits of the current playlist.
    RC_NotPlaying,          // The command is only allowed while playing.
    RC_OutPlugErr,          // The output plug-in returned an error.
    RC_DecPlugErr,          // The decoder plug-in returned an error.
    RC_InvalidItem,         // Cannot load or play invalid object.
    RC_BadIterator          // Bad location string.
  };

  struct ControlCommand;
  typedef void (*CbComplete)(ControlCommand* cmd);
  struct ControlCommand
  { Command         Cmd;    // Basic command. See PM123Command for details. 
    xstring         StrArg; // String argument (command dependent)
    union
    { double        NumArg; // Integer argument (command dependent)
      void*         PtrArg; // Pointer argument (command dependent)
    };
    int             Flags;  // Flags argument (command dependent)
    CbComplete      Callback;// Notification on completion, this function must ensure that the control command is deleted.
    void*           User;   // Unused, for user purposes only 
    ControlCommand* Link;   // Linked commands. They are executed atomically.
    ControlCommand(Command cmd, const xstring& str, double num, int flags, CbComplete cb = NULL, void* user = NULL)
                    : Cmd(cmd), StrArg(str), NumArg(num), Flags(flags), Callback(cb), User(user), Link(NULL) {}
    ControlCommand(Command cmd, const xstring& str, void* ptr, int flags, CbComplete cb = NULL, void* user = NULL)
                    : Cmd(cmd), StrArg(str), PtrArg(ptr), Flags(flags), Callback(cb), User(user), Link(NULL) {}
    void            Destroy();// Deletes the entire command queue including this.
  };

  enum EventFlags
  { EV_None     = 0x00000000, // nothing
    EV_PlayStop = 0x00000001, // The play/stop status has changed.
    EV_Pause    = 0x00000002, // The pause status has changed.
    EV_Forward  = 0x00000004, // The fast forward status has changed.
    EV_Rewind   = 0x00000008, // The rewind status has changed.
    EV_Shuffle  = 0x00000010, // The shuffle flag has changed.
    EV_Repeat   = 0x00000020, // The repeat flag has changed.
    EV_Volume   = 0x00000040, // The volume has changed.
    EV_Savename = 0x00000080, // The savename has changed.
    EV_Offset   = 0x00000100, // The current playing offset has changed
    EV_Root     = 0x00001000, // The currently loaded root object has changed. This Always implies EV_Song.
    EV_Song     = 0x00100000, // The current song has changed.
  };
  
 private:
  struct QEntry : qentry
  { ControlCommand*           Cmd;
    QEntry(ControlCommand* cmd) : Cmd(cmd) {}
  };
  struct PrefetchEntry
  { PM123_TIME                Offset;        // Starting time index from the output's point of view.
    SongIterator              Loc;           // Location that points to the song that starts at this position.
    //PrefetchEntry() : Offset(0) {}
    PrefetchEntry(PM123_TIME offset, const SongIterator& loc) : Offset(offset), Loc(loc) {}
  };

  /*struct ControlEventArgs
  { ControlEventFlags Flags;
  };*/

 private: // working set
  // List of prefetched iterators.
  // The first entry is always the current iterator if a enumerable object is loaded.
  // Write access to this list is only done by the controller thread.
  static vector<PrefetchEntry> PrefetchList;
  static Mutex                PLMtx;                 // Mutex to protect the above list.

  static bool                 Playing;               // True if a song is currently playing (not decoding)
  static bool                 Paused;                // True if the current song is paused
  static DECFASTMODE          Scan;                  // Current scan mode
  static double               Volume;                // Current volume setting
  static xstring              Savename;              // Current save file name (for the decoder)
  static bool                 Shuffle;               // Shuffle flag
  static bool                 Repeat;                // Repeat flag

  static queue<QEntry>        Queue;                 // Command queue of the controller (all messages pass this queue)
  static TID                  WorkerTID;             // Thread ID of the worker
  static ControlCommand*      CurCmd;                // Currently processed ControlCommand, protected by Queue.Mtx

  static AtomicUnsigned       Pending;               // Pending events
  // These events are set atomically from any thread.
  // After each last message of a queued set of messages the events are raised and Pending is atomically reset.

  static event<const EventFlags> ChangeEvent;

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
 
 public: // management interface, not thread safe
  // initialize controller
  static void  Init();
  // uninitialize controller
  static void  Uninit();

 public: // properties, thread safe
  // While the functions below are atomic their return values are not reliable because they can change everytime.
  // So be careful.

  // Check whether we are currently playing.
  static bool          IsPlaying()            { return Playing; }
  // Check whether the current play status is paused.
  static bool          IsPaused()             { return Paused; }
  // Return the current scanmode.
  static DECFASTMODE   GetScan()              { return Scan; }
  // Return the current volume. This does not return a decreased value in scan mode.
  static double        GetVolume()            { return Volume; }
  // Return the current shuffle status.
  static bool          IsShuffle()            { return Shuffle; }
  // Return the current repeat status.
  static bool          IsRepeat()             { return Repeat; }
  // Return the current savefile name.
  static xstring       GetSavename()          { return Savename; }
  // Return the current song (whether playing or not).
  // If nothing is attached or if a playlist recently completed the function returns NULL.
  static int_ptr<APlayable> GetCurrentSong();
  // Return the currently loaded root object. This might be enumerable or not.
  static int_ptr<APlayable> GetRoot();

 public: //message interface, thread safe
  // post a command to the controller Queue
  static void PostCommand(ControlCommand* cmd)
  { DEBUGLOG(("Ctrl::PostCommand(%p{%i, ...})\n", cmd, cmd ? cmd->Cmd : -1));
    Queue.Write(new QEntry(cmd));
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
  // The function returns the control command queue which usually must be deleted.
  // Calling cmd->Destroy() will do the job.
  // The function must not be called from a thread which receives PM messages.
  static ControlCommand* SendCommand(ControlCommand* cmd);
  // Empty control callback, may be used to avoid the deletion of a ControlCommand.
  static void            CbNop(ControlCommand* cmd);

  // Short cuts for message creation, side-effect free.
  // This is recommended over calling the ControllCommand constructor directly.
  static ControlCommand* MkNop()
  { return new ControlCommand(Cmd_Nop, xstring(), (void*)NULL, 0); }
  static ControlCommand* MkLoad(const xstring& url, bool keepplaying)
  { return new ControlCommand(Cmd_Load, url, 0., keepplaying); }
  static ControlCommand* MkSkip(int count, bool relative)
  { return new ControlCommand(Cmd_Skip, xstring(), count, relative); }
  static ControlCommand* MkNavigate(const xstring& iter, PM123_TIME start, bool relative, bool global)
  { return new ControlCommand(Cmd_Navigate, iter, start, relative | (global<<1)); }
  static ControlCommand* MkJump(Location* iter)
  { return new ControlCommand(Cmd_Jump, xstring(), iter, 0); }
  static ControlCommand* MkStopAt(const xstring& iter, PM123_TIME start, bool global)
  { return new ControlCommand(Cmd_StopAt, iter, start, global<<1); }
  static ControlCommand* MkPlayStop(Op op)
  { return new ControlCommand(Cmd_PlayStop, xstring(), 0., op); }
  static ControlCommand* MkPause(Op op)
  { return new ControlCommand(Cmd_Pause, xstring(), 0., op); }
  static ControlCommand* MkScan(int op)
  { return new ControlCommand(Cmd_Scan, xstring(), 0., op); }
  static ControlCommand* MkVolume(double volume, bool relative)
  { return new ControlCommand(Cmd_Volume, xstring(), volume, relative); }
  static ControlCommand* MkShuffle(Op op)
  { return new ControlCommand(Cmd_Shuffle, xstring(), 0., op); }
  static ControlCommand* MkRepeat(Op op)
  { return new ControlCommand(Cmd_Repeat, xstring(), 0., op); }
  static ControlCommand* MkSave(const xstring& filename)
  { return new ControlCommand(Cmd_Save, filename, 0., 0); }
  static ControlCommand* MkLocation(SongIterator* sip, char what)
  { return new ControlCommand(Cmd_Location, xstring(), sip, what); }
 private: // internal messages
  static ControlCommand* MkDecStop()
  { return new ControlCommand(Cmd_DecStop, xstring(), (void*)NULL, 0); }
  static ControlCommand* MkOutStop()
  { return new ControlCommand(Cmd_OutStop, xstring(), (void*)NULL, 0); }

 public: // notifications
  // Notify about any changes. See EventFlags for details.
  // The events are not synchronous, i.e. they are not called before the things happen.
  static event_pub<const EventFlags>& GetChangeEvent() { return ChangeEvent; }
  
 public: // debug interface
  static void QueueTraverse(void (*action)(const ControlCommand& cmd, void* arg), void* arg);
  //{ Queue.ForEach(action, arg); }
 private:
  struct QueueTraverseProxyData
  { void  (*Action)(const Ctrl::ControlCommand& cmd, void* arg);
    void* Arg;
  };
  static void QueueTraverseProxy(const Ctrl::QEntry& entry, void* arg);
};
FLAGSATTRIBUTE(Ctrl::EventFlags);


#endif
