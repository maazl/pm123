/*
 * Copyright 2007-2010 M.Mueller
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

#ifndef  GUI_H
#define  GUI_H

#define INCL_WIN
#include <config.h>
#include <cpp/smartptr.h>
#include <cpp/event.h>

#include "controller.h"
#include "properties.h"
#include "plugman.h"
#include "loadhelper.h"
#include "playlistmenu.h"
#include "pm123.rc.h"
#include <os2.h>


class Module;

class GUI
{public:
  // Opens dialog for the specified object.
  enum DialogType
  { DLT_INFOEDIT,
    DLT_METAINFO,
    DLT_TECHINFO,
    DLT_PLAYLIST,
    DLT_PLAYLISTTREE
  };

 private:
  enum WMUser                             // MP1               MP2
  { WMP_REFRESH_CONTROLS = WM_USER + 1000 // 0,                0
  , WMP_PAINT                             // maske,            0
  , WMP_LOAD                              // LoadHelper* 
  , WMP_RELOADSKIN                        // 0,                0
  , WMP_DISPLAY_MESSAGE                   // message,          TRUE (info) or FALSE (error)
  , WMP_DISPLAY_MODE                      // 0,                0
  , WMP_QUERY_STRING                      // buffer,           size and type
  , WMP_SHOW_DIALOG                       // int_ptr<Playable> DialogType
  , WMP_PLAYABLE_EVENT                    // APlayable*        Changed, Loaded
  , WMP_CTRL_EVENT                        // EventFlags        0
  , WMP_CTRL_EVENT_CB                     // ControlCommand*   0
  , WMP_REFRESH_ACCEL      
  , WMP_SLIDERDRAG                        // pos(x,y),         TRUE: navigate and complete
  , WMP_ARRANGEDOCKING                    // 
  };
  enum UpdateFlags
  { UPD_NONE             = 0
  , UPD_TIMERS           = 0x0001         // Current time index, remaining time and slider
  , UPD_TOTALS           = 0x0002         // Total playing time, total number of songs
  , UPD_PLMODE           = 0x0004         // Playlist mode icons
  , UPD_PLINDEX          = 0x0008         // Playlist index
  , UPD_RATE             = 0x0010         // Bit rate
  , UPD_CHANNELS         = 0x0020         // Number of channels
  , UPD_VOLUME           = 0x0040         // Volume slider
  , UPD_TEXT             = 0x0080         // Text in scroller
  , UPD_ALL              = 0x00ff         // All the above
  , UPD_WINDOW           = 0x0100         // Whole window redraw
  };
  CLASSFLAGSATTRIBUTE(UpdateFlags);

  // Cache lifetime of unused playable objects in seconds
  // Objects are removed after [CLEANUP_INTERVALL, 2*CLEANUP_INTERVALL].
  // However, since the romoved objects may hold references to other objects,
  // only one generation is cleand up per time.
  static const int CLEANUP_INTERVAL = 10;

 private: // Internal playlist objects
  static int_ptr<Playable> DefaultPL;     // Default playlist, representing PM123.LST in the program folder.
  static int_ptr<Playable> DefaultPM;     // PlaylistManager window, representing PFREQ.LST in the program folder.
  static int_ptr<Playable> DefaultBM;     // Default instance of bookmark window, representing BOOKMARK.LST in the program folder.
  static int_ptr<Playable> LoadMRU;       // Most recent used entries in the load menu, representing LOADMRU.LST in the program folder.
  static int_ptr<Playable> UrlMRU;        // Most recent used entries in the load URL dialog, representing URLMRU.LST in the program folder.

 private: // Working set
  static HWND              HFrame;        // Frame window
  static HWND              HPlayer;       // Player window
  static HWND              HHelp;         // Help instance
  static bool              Terminate;     // True when WM_QUIT arrived
  static DECODER_WIZARD_FUNC LoadWizards[16]; // Current load wizards
  static HWND              ContextMenu;   // Window handle of the PM123 main context menu
  static PlaylistMenu*     MenuWorker;    // Instance of PlaylistMenu to handle events of the context menu.

  static SongIterator      IterBuffer[2]; // Two SongIterators. CurrentIter points to one of them.
  static SongIterator*     CurrentIter;   // current SongIterator. NOT NULL!
  static bool              IsLocMsg;      // true if a MsgLocation to the controller is on the way
  static UpdateFlags       UpdFlags;      // Pending window updates.
  static UpdateFlags       UpdAtLocMsg;   // Update this fields at the next location message.
  static bool              IsHaveFocus;   // PM123 main window currently has the focus.
  static int               IsSeeking;     // A number of seek operations is currently on the way.
  static bool              IsVolumeDrag;  // We currently drag the volume bar.
  static bool              IsSliderDrag;  // We currently drag the navigation slider.
  static bool              IsAltSlider;   // Alternate Slider currently active
  static bool              IsAccelChanged;// Flag whether the accelerators have changed since the last context menu invocation.
  static bool              IsPluginChanged;// Flag whether the plug-in list has changed since the last context menu invocation.
  
  static delegate<const void, const Plugin::EventArgs>  PluginDeleg;
  static delegate<const void, const Ctrl::EventFlags>   ControllerDeleg;
  static delegate<const void, const PlayableChangeArgs> RootDeleg;
  static delegate<const void, const PlayableChangeArgs> CurrentDeleg;
  
 private:
  // Static members must not use EXPENTRY linkage with IBM VACPP.
  friend MRESULT EXPENTRY GUI_DlgProcStub(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  static MRESULT   DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2);

  static void      Invalidate(UpdateFlags what, bool immediate);
  static void      SetAltSlider(bool alt);

  // Ensure that a MsgLocation message is processed by the controller
  static void      ForceLocationMsg();
  static void      ControllerEventCB(Ctrl::ControlCommand* cmd);
  static void      ControllerNotification(const void*, const Ctrl::EventFlags& flags);
  static void      PlayableNotification(const void*, const PlayableChangeArgs& args);
  static void      PluginNotification(const void*, const Plugin::EventArgs& args);

  static Playable* CurrentRoot()             { return CurrentIter->GetRoot(); }
  static APlayable& CurrentSong()            { return CurrentIter->GetCurrent(); }

  static void      SaveStream(HWND hwnd, BOOL enable);
  friend BOOL EXPENTRY GUI_HelpHook(HAB hab, ULONG usMode, ULONG idTopic, ULONG idSubTopic, PRECTL prcPosition);

  static void      LoadAccel();           // (Re-)Loads the accelerator table and modifies it by the plug-ins.
  static void      ShowContextMenu();     // View to context menu
  static void      RefreshTimers(HPS hps);// Refresh current playing time, remaining time and slider.
  static void      PrepareText();         // Constructs a information text for currently loaded file and selects it for displaying.

  friend void DLLENTRY GUI_LoadFileCallback(void* param, const char* url, const INFO_BUNDLE* info, int cached, int override);

  static void      AutoSave(Playable& list);

  // Drag & drop
  static MRESULT   DragOver(DRAGINFO* pdinfo);
  static MRESULT   DragDrop(PDRAGINFO pdinfo);
  static MRESULT   DragRenderDone(PDRAGTRANSFER pdtrans, USHORT rc);

 public: // Utility functions
  static HWND      GetFrameWindow()       { return HFrame; }
  static HWND      GetHelpMgr()           { return HHelp; }
  static Playable* GetDefaultPL()         { return DefaultPL; }
  static Playable* GetDefaultPM()         { return DefaultPM; }
  static Playable* GetDefaultBM()         { return DefaultBM; }
  static Playable* GetLoadMRU()           { return LoadMRU; }
  static Playable* GetUrlMRU()            { return UrlMRU; }

  // Update an MRU list with item ps and maximum size max
  static void      Add2MRU(Playable& list, size_t max, APlayable& ps);

 public: // Manipulating interface   
  static void      ViewMessage(xstring info, bool error);
  // Tells the help manager to display a specific help window.
  static bool      ShowHelp(SHORT resid)  { DEBUGLOG(("amp_show_help(%u)\n", resid));
                                            return WinSendMsg(HHelp, HM_DISPLAY_HELP, MPFROMSHORT(resid), MPFROMSHORT(HM_RESOURCEID)) == 0; }
  // Opens dialog for the specified object.
  static void      ShowDialog(Playable& item, DialogType dlg);
  static void      ShowConfig()           { PMRASSERT(WinPostMsg(HPlayer, WM_COMMAND, MPFROMSHORT(IDM_M_CFG), 0)); }
  static void      ShowConfig(Module& plugin);
  
  static void      Show(bool visible = true);
  static void      Quit()                 { WinSendMsg(HPlayer, WM_COMMAND, MPFROMSHORT(BMP_POWER), 0); }
  static void      SetWindowMode(cfg_mode mode);
  static void      SetWindowFloat(bool flag);
  static void      ReloadSkin()           { PMRASSERT(WinPostMsg(HPlayer, WMP_RELOADSKIN, 0, 0)); }
  static void      RefreshDisplay()       { PrepareText(); PMRASSERT(WinPostMsg(HPlayer, WMP_PAINT, MPFROMLONG(UPD_ALL), 0)); }
  static void      RearrangeDocking()     { PMRASSERT(WinPostMsg(HPlayer, WMP_ARRANGEDOCKING, 0, 0)); }

  // Load objects into the player.
  // Attension!!! Load takes the exclusive ownership of lhp and deletes it afterwards. 
  static void      Load(LoadHelper* lhp)  { WinPostMsg(HFrame, WMP_LOAD, MPFROMP(lhp), 0); }

 public: // Initialization interface
  static void      Init();
  static void      Uninit();  
  
};

#include "plugman.h"

inline void GUI::ShowConfig(Module& plugin)
{ plugin.Config(HFrame);
} 


#endif /* GUI_H */

