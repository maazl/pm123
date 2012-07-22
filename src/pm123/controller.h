/*
 * Copyright 2007-2012 M.Mueller
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

#include <cpp/queue.h>
#include <cpp/xstring.h>
#include <cpp/event.h>
#include <cpp/cpputil.h>

#include <debuglog.h>


class SongIterator;

/** PM123 controller class.
 * All playback activities are controlled by this class.
 * This class is static (singleton).
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
    Cmd_SeekStop,
    Cmd_OutStop
  };

  /// Flags for setter commands
  enum Op
  { Op_Reset  = 0,
    Op_Set    = 1,
    Op_Clear  = 2,
    Op_Toggle = 3,
    Op_Rewind = 4  // PM123_Scan only
  };

  /// return codes in Flags
  enum RC
  { RC_OK,                  ///< Everything OK.
    RC_SubseqError,         ///< The command is not processed because an earlier command in the current set of linked commands has failed.
    RC_BadArg,              ///< Invalid command.
    RC_NoSong,              ///< The command requires a current song.
    RC_NoList,              ///< The command is only valid for enumerable objects like playlists.
    RC_EndOfList,           ///< The navigation tried to move beyond the limits of the current playlist.
    RC_NotPlaying,          ///< The command is only allowed while playing.
    RC_OutPlugErr,          ///< The output plug-in returned an error.
    RC_DecPlugErr,          ///< The decoder plug-in returned an error.
    RC_InvalidItem,         ///< Cannot load or play invalid object.
    RC_BadIterator          ///< Bad location string.
  };

  struct ControlCommand;
  typedef void (*CbComplete)(ControlCommand* cmd);
  /// @brief Command to the controller.
  /// @details See Mk... functions for the description of the individual command arguments.
  /// All commands return a success code in Flags.
  /// If a command failed StrArg may point to a descriptive error text.
  ///
  /// Commands can have an optional callback function which is called when the command is executed completely.
  /// The callback function must at least delete the \c ControlCommand instance.
  /// The callback function should not block.
  /// If a \c ControlCommand has no callback function, it is deleted by the queue processor.
  ///
  /// Commands can be linked by the Link field. Linked commands are executed without interruption by other command sources.
  /// If one of the commands fails, all further linked commands fail immediately with \c PM123RC_SubseqError too.
  struct ControlCommand
  { Command         Cmd;    ///< Basic command. See PM123Command for details.
    xstring         StrArg; ///< String argument (command dependent)
    union
    { double        NumArg; ///< Integer argument (command dependent)
      void*         PtrArg; ///< Pointer argument (command dependent)
    };
    int             Flags;  ///< Flags argument (command dependent)
    CbComplete      Callback;///< Notification on completion, this function must ensure that the control command is deleted.
    void*           User;   ///< Unused, for user purposes only
    ControlCommand* Link;   ///< Linked commands. They are executed atomically.
    ControlCommand(Command cmd, const xstring& str, double num, int flags, CbComplete cb = NULL, void* user = NULL)
                    : Cmd(cmd), StrArg(str), NumArg(num), Flags(flags), Callback(cb), User(user), Link(NULL) {}
    ControlCommand(Command cmd, const xstring& str, void* ptr, int flags, CbComplete cb = NULL, void* user = NULL)
                    : Cmd(cmd), StrArg(str), PtrArg(ptr), Flags(flags), Callback(cb), User(user), Link(NULL) {}
    void            Destroy();///< Deletes the entire command queue including this.
  };

  enum EventFlags
  { EV_None     = 0x00000000, ///< nothing
    EV_PlayStop = 0x00000001, ///< The play/stop status has changed.
    EV_Pause    = 0x00000002, ///< The pause status has changed.
    EV_Forward  = 0x00000004, ///< The fast forward status has changed.
    EV_Rewind   = 0x00000008, ///< The rewind status has changed.
    EV_Shuffle  = 0x00000010, ///< The shuffle flag has changed.
    EV_Repeat   = 0x00000020, ///< The repeat flag has changed.
    EV_Volume   = 0x00000040, ///< The volume has changed.
    EV_Savename = 0x00000080, ///< The savename has changed.
    EV_Location = 0x00000100, ///< The current playing offset has changed
    EV_Root     = 0x00001000, ///< The currently loaded root object has changed. This Always implies EV_Song.
    EV_Song     = 0x00100000, ///< The current song has changed.
  };

 protected:
  struct QEntry : qentry
  { ControlCommand*           Cmd;
    QEntry(ControlCommand* cmd) : Cmd(cmd) {}
  };

 protected: // working set
  static bool          Playing;               ///< True if a song is currently playing (not decoding)
  static bool          Paused;                ///< True if the current song is paused
  static DECFASTMODE   Scan;                  ///< Current scan mode
  static double        Volume;                ///< Current volume setting
  static xstring       Savename;              ///< Current save file name (for the decoder)
  static bool          Shuffle;               ///< Shuffle flag
  static bool          Repeat;                ///< Repeat flag

  static queue<QEntry> Queue;                 ///< Command queue of the controller (all messages pass this queue)

  static event<const EventFlags> ChangeEvent;

 public: // management interface, not thread safe
  /// initialize controller
  static void          Init();
  /// uninitialize controller
  static void          Uninit();

 public: // properties, thread safe
  // While the functions below are atomic their return values are not reliable because they can change everytime.
  // So be careful.

  /// Check whether we are currently playing.
  static bool          IsPlaying()            { return Playing; }
  /// Check whether the current play status is paused.
  static bool          IsPaused()             { return Paused; }
  /// Return the current scanmode.
  static DECFASTMODE   GetScan()              { return Scan; }
  /// Return the current volume. This does not return a decreased value in scan mode.
  static double        GetVolume()            { return Volume; }
  /// Return the current shuffle status.
  static bool          IsShuffle()            { return Shuffle; }
  /// Return the current repeat status.
  static bool          IsRepeat()             { return Repeat; }
  /// Return the current savefile name.
  static xstring       GetSavename()          { return Savename; }
  /// Return the current song (whether playing or not).
  /// If nothing is attached or if a playlist recently completed the function returns NULL.
  static int_ptr<APlayable> GetCurrentSong();
  /// Return the currently loaded root object. This might be enumerable or not.
  static int_ptr<APlayable> GetRoot();

 public: //message interface, thread safe
  /// post a command to the controller Queue
  static void PostCommand(ControlCommand* cmd)
  { DEBUGLOG(("Ctrl::PostCommand(%p{%i, ...})\n", cmd, cmd ? cmd->Cmd : -1));
    Queue.Write(new QEntry(cmd));
  }
  /// post a command with a callback function to the controller Queue
  static void PostCommand(ControlCommand* cmd, CbComplete callback)
  { cmd->Callback = callback;
    PostCommand(cmd);
  }
  /// post a command with a callback function and a user parameter to the controller Queue
  static void PostCommand(ControlCommand* cmd, CbComplete callback, void* user)
  { cmd->User = user;
    PostCommand(cmd, callback);
  }
  /// Post a command to the controller queue and wait for completion.
  /// The function returns the control command queue which usually must be deleted.
  /// Calling \c cmd->Destroy() will do the job.
  /// The function must not be called from a thread which receives PM messages.
  static ControlCommand* SendCommand(ControlCommand* cmd);
  /// Empty control callback, may be used to avoid the deletion of a ControlCommand.
  static void            CbNop(ControlCommand* cmd);

  // Short cuts for message creation, side-effect free.
  // This is recommended over calling the ControllCommand constructor directly.
  /// No operation (used to raise events)
  static ControlCommand* MkNop()
  { return new ControlCommand(Cmd_Nop, xstring(), (void*)NULL, 0); }
  /// Load an URL
  /// @param root new current root
  /// @param keepplaying true: Start playback immediately if currently playing.
  static ControlCommand* MkLoad(APlayable* root, bool keepplaying)
  { return new ControlCommand(Cmd_Load, xstring(), int_ptr<APlayable>(root).toCptr(), keepplaying); }
  /// @brief Move to song number or some songs forward or backward.
  /// @details This makes only sense if the currently loaded object is enumerable.
  /// Absolute positioning is not recommended. Use \c MkNavigate instead.
  /// @param count Number of songs to move. If less than zero backward navigation is used.
  static ControlCommand* MkSkip(int count, bool relative)
  { return new ControlCommand(Cmd_Skip, xstring(), count, relative); }
  /// @brief Jump to location.
  /// @details This will change the Song and/or the playing position.
  /// @param iter Serialized iterator (optional)
  /// @param start Location in seconds
  /// @param relative Location is relative from the current location.
  /// Otherwise it starts from the outside the current song.
  /// @param global Navigate in global playlist scope.
  /// @param ignoreerror Ignore syntax errors. Parse as far as possible.
  static ControlCommand* MkNavigate(const xstring& iter, PM123_TIME start, bool relative, bool global, bool ignoreerror = false)
  { return new ControlCommand(Cmd_Navigate, iter, start, relative | (global<<1) | (ignoreerror<<2)); }
  /// @brief Jump to location
  /// @details This will change the Song and/or the playing position.
  /// The old position is returned in place.
  /// @param iter [in] New location. The new location need not to have
  /// the same root than the current location. It is sufficient if they are related,
  /// i.e. the root of one location should be in the call stack of the other one.
  /// [out after completion] new location, not necessarily the same than on input.
  static ControlCommand* MkJump(Location* iter)
  { return new ControlCommand(Cmd_Jump, xstring(), iter, 0); }
  /// Start or stop playing.
  /// @param op Flag operator. See \c Op. (Op_Rewind is not valid)
  static ControlCommand* MkPlayStop(Op op)
  { return new ControlCommand(Cmd_PlayStop, xstring(), 0., op); }
  /// Set or unset pause. Pause is reset on stop.
  /// @param op Flag operator. See \c Op.
  static ControlCommand* MkPause(Op op)
  { return new ControlCommand(Cmd_Pause, xstring(), 0., op); }
  /// Set/reset forward/rewind.
  /// @param op Flag operator. See \c Op.
  /// If Op_Rewind is not set fast forward is controlled.
  /// Setting fast forward automatically stops rewinding and vice versa.
  static ControlCommand* MkScan(int op)
  { return new ControlCommand(Cmd_Scan, xstring(), 0., op); }
  /// Set volume.
  /// @param volume [in] Volume in the range (0..+/-1)
  /// [out after completion] new volume
  /// @param relative Set value relative to the current value.
  /// Otherwise absolute (0..1)
  static ControlCommand* MkVolume(double volume, bool relative)
  { return new ControlCommand(Cmd_Volume, xstring(), volume, relative); }
  /// Set/reset shuffle.
  /// @param op Flag operator. See \c Op. (Op_Rewind is not valid)
  static ControlCommand* MkShuffle(Op op)
  { return new ControlCommand(Cmd_Shuffle, xstring(), 0., op); }
  /// Set/reset repeat mode.
  /// @param op Flag operator. See \c Op. (Op_Rewind is not valid)
  static ControlCommand* MkRepeat(Op op)
  { return new ControlCommand(Cmd_Repeat, xstring(), 0., op); }
  /// Save raw stream of current decoder.
  /// @param filename Target filename. NULL disables stream save.
  static ControlCommand* MkSave(const xstring& filename)
  { return new ControlCommand(Cmd_Save, filename, 0., 0); }
  /// Query the current location.
  /// @param loc [out after completion] Location object where to store the current location.
  /// @param notime Return only the current song without the time offset (faster).
  static ControlCommand* MkLocation(Location* loc, bool notime)
  { return new ControlCommand(Cmd_Location, xstring(), loc, notime); }
 protected: // internal messages
  /// The current decoder finished it's work.
  static ControlCommand* MkDecStop()
  { return new ControlCommand(Cmd_DecStop, xstring(), (void*)NULL, 0); }
  /// The current decoder finished it's work.
  static ControlCommand* MkSeekStop()
  { return new ControlCommand(Cmd_SeekStop, xstring(), (void*)NULL, 0); }
  /// The output finished playback.
  static ControlCommand* MkOutStop()
  { return new ControlCommand(Cmd_OutStop, xstring(), (void*)NULL, 0); }

 public: // notifications
  /// Notify about any changes. See EventFlags for details.
  /// The events are not synchronous, i.e. they are not called before the things happen.
  static event_pub<const EventFlags>& GetChangeEvent() { return ChangeEvent; }
  
 public: // debug interface
  static bool QueueTraverse(bool (*action)(const ControlCommand& cmd, void* arg), void* arg);
};
FLAGSATTRIBUTE(Ctrl::EventFlags);

#endif
