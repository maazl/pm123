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

#include <decoder_plug.h>

#include <cpp/queue.h>
#include <cpp/xstring.h>
#include <cpp/event.h>
#include <cpp/cpputil.h>

#include <debuglog.h>


/* Class to iterate over a PlayableCollection recursively returning only Song items.
 * All non-const functions are not thread safe.
 */
class SongIterator
{public:
  struct Offsets
  { int                       Index;     // Item index of the current PlayableInstance counting from the top level.
    T_TIME                    Offset;    // Time offset of the current PlayableInstance counting from the top level or -1 if not available.
    Offsets(int index, T_TIME offset) : Index(index), Offset(offset) {}
  };
  struct CallstackEntry : public Offsets
  { int_ptr<PlayableInstance> Item;      // Current item in the PlayableCollection of the pervious Callstack entry if any or the root otherwise.
    CallstackEntry()          : Offsets(-1, Offset = -1) {}
  };
  typedef vector<CallstackEntry> CallstackType;
  
 private:
  int_ptr<PlayableCollection> Root;      // Root collection to enumerate. 
  CallstackType               Callstack; // Current callstack, excluding the top level playlist.
                                         // The Callstack has at least one element for Current.
                                         // This collection contains only enumerable objects except for the last entry.
  PlayableSet                 Exclude;   // List of playable objects to exclude.
                                         // This are in fact the enumerable objects from Callstack and the Root.
  static const Offsets        ZeroOffsets; // Offsets for root entry.

 protected: // internal functions, not thread-safe.
  // Push the current item to the call stack.
  // Precondition: Current is Enumerable.
  void                        Enter();
  // Pop the last object from the stack and make it to Current.
  void                        Leave();
  // Check wether to skip the current item.
  // I.e. if the current item is already in the call stack.
  bool                        SkipQ();
  // Fetch length and number of subitems from a Playable object with respect to Exclude.
  Offsets                     TechFromPlayable(PlayableCollection* pc);
  Offsets                     TechFromPlayable(Playable* pp);
  // Implementation of Prev and Next.
  // dir must be -1 for backward direction and +1 for forward.
  bool                        PrevNextCore(int dir);

 public:
  // Create a SongIterator for iteration over a PlayableCollection.
                              SongIterator();
  // Copy constructor                              
                              SongIterator(const SongIterator& r);
  // Cleanup (removes all strong references)
                              ~SongIterator();
  // swap two instances (fast)
  void                        Swap(SongIterator& r);
  /*#ifdef DEBUG
  void                        DebugDump() const;
  #endif*/  
  
  // Replace the attached root object. This resets the iterator.
  void                        SetRoot(PlayableCollection* pc);
  // Gets the current root.
  PlayableCollection*         GetRoot() const          { return Root; }
  // Gets the deepest playlist. This may be the root if the callstack is empty.
  PlayableCollection*         GetList() const;
  // Get Offsets of current item
  Offsets                     GetOffset() const;
  // Gets the call stack. Not thread-safe!
  const CallstackType&        GetCallstack() const     { return Callstack; }
  // Gets the excluded entries. Not thread-safe!
  const PlayableSet&          GetExclude() const       { return Exclude; }
  // Check whether a Playable object is in the call stack. Not thread-safe!
  // This will return always false if the Playable object is not enumerable.
  bool                        IsInCallstack(const Playable* pp) const { return Exclude.find(pp) != NULL; }

  // Get the current song. This may be NULL.  
  PlayableInstance*           GetCurrent() const;
  PlayableInstance*           operator*() const        { return GetCurrent(); }
  PlayableInstance*           operator->() const       { return GetCurrent(); }

  // Go to the previous Song.
  bool                        Prev()                   { return PrevNextCore(-1); }
  // Go to the next Song.
  bool                        Next()                   { return PrevNextCore(+1); }
  // reset the Iterator to it's initial state. 
  void                        Reset();
  
  // comparsion
  friend bool                 operator==(const SongIterator& l, const SongIterator& r);
};


/* PM123 controller class.
 * All playback activities are controlled by this class.
 * This class is static.

 * PM123 commands

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
DecStop                                                                The current decoder finished it's work.
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
    Cmd_Load = 1,
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
    Cmd_Status,
    // internal events
    Cmd_DecStop
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
    RC_NoList,
    RC_EndOfList,
    RC_NotPlaying,
    RC_OutPlugErr,
    RC_DecPlugErr
  };

  struct EQ_Data
  { float bandgain[2][10];
  };

  struct PlayStatus //: public PlayEnumerator::Status
  { int    CurrentItem;     // index of the currently played song
    int    TotalItems;      // total number of items in the queue
    T_TIME CurrentTime;     // current time index from the start of the queue excluding the currently loaded one
    T_TIME TotalTime;       // total time of all items in the queue
    T_TIME CurrentSongTime; // current time index in the current song
    T_TIME TotalSongTime;   // total time of the current song
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
  static int_ptr<Song>        CurrentSong;   // Current Song if a non-enumerable item is loaded.
  static vector<PrefetchEntry> PrefetchList; // List of prefetched iterators. The first entry is always the current iterator if a enumerable object is loaded.

  static T_TIME               Location;
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
  
  static delegate<void, const dec_event_args> DecEventDelegate;
  static delegate<void, const OUTEVENTTYPE> OutEventDelegate;

  static const SongIterator::CallstackType EmptyStack;

 private: // internal functions, not thread safe
  // Returns the current PrefetchEntry. I.e. the currently playing (or not playing) iterator.
  // Precondition: a enumerable object must have been loaded.
  static PrefetchEntry* Current() { return PrefetchList[0]; }
  // Applies the operator op to flag and returns true if the flag has been changed.
  static bool  SetFlag(bool& flag, Op op);
  // Sets the volume according to this->Volume and the scan mode.
  static void  SetVolume();
  // Initializes the decoder engine and starts playback of the song pp with a time offset for the output.
  // The function returns the result of dec_play.
  // Precondition: The output must have been initialized.
  // The function does not return unless the decoder is decoding or an error occured.
  static ULONG DecoderStart(Song* pp, T_TIME offset = 0);
  // Stops decoding and deinitializes the decoder plug-in.
  static void  DecoderStop();
  // Initializes the output for playing pp.
  // The playable object is needed for naming purposes.
  static ULONG OutputStart(Playable* pp);
  // Stops playback and clears the prefetchlist.
  static void  OutputStop();
  static void  UpdateStackUsage(const SongIterator::CallstackType& oldstack, const SongIterator::CallstackType& newstack);
  static bool  SkipCore(SongIterator& si, int count, bool relative);
  static void  PrefetchClear(bool keep);
  friend void  TFNENTRY ControllerWorkerStub(void*);
  static void  Worker();
  static void  DecEventHandler(void*, const dec_event_args& args);
  static void  OutEventHandler(void*, const OUTEVENTTYPE& event);
 private: // messages handlers, not thread safe  
  static RC    MsgPause(Op op);
  static RC    MsgScan(Op op);
  static RC    MsgVolume(double volume, bool relative);
  static RC    MsgNavigate(const xstring& iter, T_TIME loc, int flags);
  static RC    MsgPlayStop(Op op);
  static RC    MsgSkip(int count, bool relative);
  static RC    MsgLoad(const xstring& url);
  static RC    MsgSave(const xstring& filename);
  static RC    MsgEqualize(const EQ_Data* data);
  static RC    MsgStatus();
  static RC    MsgDecStop();
 
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
  static int_ptr<Song> GetCurrentSong();
  static int_ptr<Playable> GetRoot()
  { CritSect cs(); return PrefetchList.size() ? PrefetchList[0]->Iter.GetRoot() : (Playable*)CurrentSong; }

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

  // short-cuts
  static ControlCommand* MkLoad(const xstring& url)
  { return new ControlCommand(Cmd_Load, url, 0., 0); }
  static ControlCommand* MkSkip(int count, bool relative)
  { return new ControlCommand(Cmd_Skip, xstring(), count, relative); }
  static ControlCommand* MkNavigate(const xstring& iter, T_TIME start, bool relative, bool global)
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
  static ControlCommand* MkDecStop()
  { return new ControlCommand(Cmd_DecStop, xstring(), (void*)NULL, 0); }

 public: // notifications
  static event<const EventFlags> ChangeEvent;
};

FLAGSATTRIBUTE(Ctrl::EventFlags);

#endif
