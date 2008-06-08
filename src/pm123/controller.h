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


#include "playable.h"
#include "plugman.h"
#include "songiterator.h"

#include <decoder_plug.h>

#include <cpp/queue.h>
#include <cpp/xstring.h>
#include <cpp/event.h>
#include <cpp/mutex.h>
#include <cpp/cpputil.h>

#include <debuglog.h>


/* PM123 controller class.
 * All playback activities are controlled by this class.
 * This class is static.

 * PM123 commands

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
---------------------------------------------------------------------------------------------------------------
Jump                           in/out:                                 Jump to location
                               SongIterator*                           This will change the Song and/or the
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
Equalize                       EQ_Data*                                Save stream of current decoder.
---------------------------------------------------------------------------------------------------------------
Save       Filename                                                    Save stream of current decoder.
---------------------------------------------------------------------------------------------------------------
Status                         PlayStatus*                             Fill PlayStatus structure.
---------------------------------------------------------------------------------------------------------------
Location   out: Root URL       SongIterator*                           Set the iterator to the current location.
---------------------------------------------------------------------------------------------------------------
DecStop                                                                The current decoder finished it's work.
---------------------------------------------------------------------------------------------------------------
OutStop                                                                The output finished playback.
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
    //Cmd_Status,
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
  { RC_OK,                  // Evenrything OK.
    RC_SubseqError,         // The command is not processed because an earlier command in the current set of linked commands has failed.
    RC_BadArg,              // Invalid command.
    RC_NoSong,              // The command requires a current song.
    RC_NoList,              // The command is only valid for enumerable objects like playlists.
    RC_EndOfList,           // The navigation tried to move beyond the limits of the current playlist.
    RC_NotPlaying,          // The command is only allowed while playing.
    RC_OutPlugErr,          // The output plug-in returned an error.
    RC_DecPlugErr,          // The decoder plug-in returnd an error.
    RC_InvalidItem          // Cannot load or play invalid object.
  };

  /*struct PlayStatus
  { int_ptr<Song>    CurrentSong;     // Currently active song
    int              CurrentItem;     // index of the currently played song
    int              TotalItems;      // total number of items in the queue
    T_TIME           CurrentTime;     // current time index from the start of the queue excluding the currently loaded one
    T_TIME           TotalTime;       // total time of all items in the queue
    T_TIME           CurrentSongTime; // current time index in the current song
    T_TIME           TotalSongTime;   // total time of the current song
  };*/

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
  { EV_None     = 0x0000,   // nothing
    EV_PlayStop = 0x0001,   // The play/stop status has changed.
    EV_Pause    = 0x0002,   // The pause status has changed.
    EV_Forward  = 0x0004,   // The fast forward status has changed.
    EV_Rewind   = 0x0008,   // The rewind status has changed.
    EV_Shuffle  = 0x0010,   // The shuffle flag has changed.
    EV_Repeat   = 0x0020,   // The repeat flag has changed.
    EV_Volume   = 0x0040,   // The volume has changed.
    EV_Savename = 0x0080,   // The savename has changed.
    EV_Root     = 0x0100,   // The currently loaded root object has changed.
    EV_Song     = 0x0200,   // The current song has changed. This always includes EV_Phys, EV_Tech and EV_Meta.
    EV_Tech     = 0x0400,   // The technical information have changed (e.g. song length).
    EV_Meta     = 0x0800,   // The meta data have changed (i.e. song title).
    EV_Phys     = 0x1000,   // The physical file information has changed (e.g. number of items).
    EV_Rpl      = 0x2000    // The recursive playlist information has changed
  };
  
 private:
  struct PrefetchEntry
  { T_TIME                    Offset;        // Starting time index from the output's point of view.
    SongIterator              Iter;          // Iterator that points to the song that starts at this position.
    PrefetchEntry() : Offset(0) {}
    PrefetchEntry(T_TIME offset, const SongIterator& iter) : Offset(offset), Iter(iter) {}
  };

  /*struct ControlEventArgs
  { ControlEventFlags Flags;
  };*/

 private: // working set
  // List of prefetched iterators.
  // The first entry is always the current iterator if a enumerable object is loaded.
  // Write access to this list is only done by the controller thread.
  // But since read access is done by other threads too,
  // any write to PrefetchList must be protected by a critical section.
  static vector<PrefetchEntry> PrefetchList;
  static Mutex                PLMtx;                 // Mutex to protect the above list.

  static bool                 Playing;               // True if a song is currently playing (not decoding)
  static bool                 Paused;                // True if the current song is paused
  static DECFASTMODE          Scan;                  // Current scan mode
  static double               Volume;                // Current volume setting
  static xstring              Savename;              // Current save file name (for the decoder)
  static bool                 Shuffle;               // Shuffle flag
  static bool                 Repeat;                // Repeat flag

  static queue<ControlCommand*> Queue;               // Command queue of the controller (all messages pass this queue)
  static TID                  WorkerTID;             // Thread ID of the worker

  //static PlayStatus           Status;                // Temporary storage: result of MsgStatus
  static volatile unsigned    Pending;               // Pending events
  // These events are set atomically from any thread.
  // After each last message of a queued set of messages the events are raised and Pending is atomically reset.

  // Delegates for the tracked events of the decoder, the output and the current song.  
  static delegate<void, const dec_event_args>        DecEventDelegate;
  static delegate<void, const OUTEVENTTYPE>          OutEventDelegate;
  static delegate<void, const Playable::change_args> CurrentSongDelegate;

  // Occasionally used constant.
  static const SongIterator::CallstackType EmptyStack;

 private: // internal functions, not thread safe
  // Returns the current PrefetchEntry. I.e. the currently playing (or not playing) iterator.
  // Precondition: a enumerable object must have been loaded.
  static PrefetchEntry* Current() { return PrefetchList[0]; }
  // Returns true if the current root is set and enumerable (of type PlayableCollection).
  static bool  IsEnumerable();
  // Applies the operator op to flag and returns true if the flag has been changed.
  static bool  SetFlag(bool& flag, Op op);
  // Sets the volume according to this->Volume and the scan mode.
  static void  SetVolume();
  // Initializes the decoder engine and starts playback of the song pp with a time offset for the output.
  // The function returns the result of dec_play.
  // Precondition: The output must have been initialized.
  // The function does not return unless the decoder is decoding or an error occured.
  static ULONG DecoderStart(PlayableSlice* ps, T_TIME offset);
  // Stops decoding and deinitializes the decoder plug-in.
  static void  DecoderStop();
  // Initializes the output for playing pp.
  // The playable object is needed for naming purposes.
  static ULONG OutputStart(Song* pp);
  // Stops playback and clears the prefetchlist.
  static void  OutputStop();
  // Updates the in-use status of PlayableInstance objects in the callstack by moving from oldstack to newstack.
  // The status of common parts of the two stacks is not touched. 
  // To set the in-use status initially pass EmptyStack as oldstack.
  // To reset all in-use status pass EmptyStack as newstack.
  static void  UpdateStackUsage(const SongIterator::CallstackType& oldstack, const SongIterator::CallstackType& newstack);
  // Internal subfunction to UpdateStackUsage
  static void  SetStackUsage(SongIterator::CallstackEntry*const* rbegin, SongIterator::CallstackEntry*const* rend, bool set);
  // Core logic of MsgSkip.
  // Move the current song pointer by count items if relative is true or to item number 'count' if relative is false.
  // If we try to move the current song pointer out of the range of the PlayableCollection that si relies on,
  // the function returns false and si is in reset state.
  // The function is side effect free and only operates on si.
  static bool  SkipCore(SongIterator& si, int count, bool relative);
  // Ensure that a SongIterator really points to a valid song by moving the iterator forward as required.
  static void  AdjustNext(SongIterator& si);
  // Jump to the location si. The function will destroy si.
  static RC    NavigateCore(SongIterator& si);
  // Clears the prefetch list and keep the first element if keep is true.
  // The operation is atomic.
  static void  PrefetchClear(bool keep);
  // Check whether a prefetched item is completed.
  // This is the case when the offset of the next item in PrefetchList is less than or equal to pos.
  // In case a prefetch entry is removed from PrefetchList the in-use status is refreshed and the song
  // change event is set.
  static void  CheckPrefetch(T_TIME pos);
  // Get the time in the current Song. This may cleanup the prefetch list by calling CheckPrefetch.
  static T_TIME FetchCurrentSongTime();
  // Internal stub to provide the TFNENTRY calling convention for _beginthread.
  friend void  TFNENTRY ControllerWorkerStub(void*);
  // Worker thread that processes the message queue.
  static void  Worker();
  // Event handler for decoder events.
  static void  DecEventHandler(void*, const dec_event_args& args);
  // Event handler for output events.
  static void  OutEventHandler(void*, const OUTEVENTTYPE& event);
  // Event handler for tracking modifications of the currently playing song.
  static void  CurrentSongEventHandler(void*, const Playable::change_args& args);
 private: // messages handlers, not thread safe
  // The messages are descibed above before the class header.
  static RC    MsgPause(Op op);
  static RC    MsgScan(Op op);
  static RC    MsgVolume(double volume, bool relative);
  static RC    MsgNavigate(const xstring& iter, T_TIME loc, int flags);
  static RC    MsgJump(SongIterator& iter);
  static RC    MsgStopAt(const xstring& iter, T_TIME loc, int flags);
  static RC    MsgPlayStop(Op op);
  static RC    MsgSkip(int count, bool relative);
  static RC    MsgLoad(const xstring& url, int flags);
  static RC    MsgSave(const xstring& filename);
  static RC    MsgShuffle(Op op);
  static RC    MsgRepeat(Op op);
  //static RC    MsgStatus(PlayStatus* status);
  static RC    MsgLocation(SongIterator* sip);
  static RC    MsgDecStop();
  static RC    MsgOutStop();
 
 public: // management interface, not thread safe
  // initialize controller
  static void Init();
  // uninitialize controller
  static void Uninit();

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
  static int_ptr<Song> GetCurrentSong();
  // Return the currently loaded root object. This might be enumerable or not.
  static int_ptr<PlayableSlice> GetRoot();

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
  // The function must not be called from a thread which receives PM messages.
  static ControlCommand* SendCommand(ControlCommand* cmd);

  // Short cuts for message creation, side-effect free.
  // This is recommended over calling the ControllCommand constructor directly.
  static ControlCommand* MkNop()
  { return new ControlCommand(Cmd_Nop, xstring(), (void*)NULL, 0); }
  static ControlCommand* MkLoad(const xstring& url, bool keepplaying)
  { return new ControlCommand(Cmd_Load, url, 0., keepplaying); }
  static ControlCommand* MkSkip(int count, bool relative)
  { return new ControlCommand(Cmd_Skip, xstring(), count, relative); }
  static ControlCommand* MkNavigate(const xstring& iter, T_TIME start, bool relative, bool global)
  { return new ControlCommand(Cmd_Navigate, iter, start, relative | (global<<1)); }
  static ControlCommand* MkJump(SongIterator* iter)
  { return new ControlCommand(Cmd_Jump, xstring(), iter, 0); }
  static ControlCommand* MkStopAt(const xstring& iter, T_TIME start, bool global)
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
  /*static ControlCommand* MkStatus(PlayStatus* dest)
  { return new ControlCommand(Cmd_Status, xstring(), dest, 0); }*/
  static ControlCommand* MkLocation(SongIterator* sip)
  { return new ControlCommand(Cmd_Location, xstring(), sip, 0); }
 private: // internal messages
  static ControlCommand* MkDecStop()
  { return new ControlCommand(Cmd_DecStop, xstring(), (void*)NULL, 0); }
  static ControlCommand* MkOutStop()
  { return new ControlCommand(Cmd_OutStop, xstring(), (void*)NULL, 0); }

 public: // notifications
  // Notify about any changes. See EventFlags for details.
  // The events are not synchonuous, i.e. they are not called before the things happen.
  static event<const EventFlags> ChangeEvent;
};

FLAGSATTRIBUTE(Ctrl::EventFlags);

#endif
