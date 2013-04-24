/*
 * Copyright 2008-2012 M.Mueller
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

#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include "acommandprocessor.h"
#include "../configuration.h"
#include "../core/songiterator.h"
#include "../core/dependencyinfo.h"
#include "../engine/controller.h"
#include "../engine/plugman.h"
#include <cpp/cppvdelegate.h>
#include <cpp/windowbase.h>


class LoadHelper;

/****************************************************************************
*
*  helper class URLTokenizer
*
****************************************************************************/

class URLTokenizer
{ char* Args;
 public:
  URLTokenizer(char* args) : Args(args) {}
  bool                      Next(const char*& url);
  char*                     Current() { return Args; }
};


/****************************************************************************
*
*  Private implementation of ACommandProcessor
*
****************************************************************************/
class CommandProcessor : public ACommandProcessor
{private:
  typedef strmap<20,void (CommandProcessor::*)()> CmdEntry;
  /// Command dispatch table, sorted
  static const CmdEntry       CmdList[];

  class Option
  { void (CommandProcessor::*Handler)(void* arg);
    void* Arg;
   public:
    Option(void (CommandProcessor::*handler)(void*), void* arg = NULL)
    : Handler(handler)
    , Arg(arg)
    {}
    template <class T>
    Option(T amp_cfg::* option, void (CommandProcessor::*handler)(T amp_cfg::*))
    : Handler((void (CommandProcessor::*)(void*))handler)
    , Arg((void*)option)
    {}
    template <class T>        Option(T amp_cfg::* option);
    void operator()(CommandProcessor& cp) const
    { (cp.*Handler)(Arg); }
  };

  struct SyntaxException
  { xstring Text;
    SyntaxException(const char* text, ...);
  };

 private: // state
  /// Current Playlist and location within the playlist.
  SongIterator                CurSI;
  /*/// playlist where we currently operate
  int_ptr<Playable>           CurPlaylist;
  /// current item of the above playlist
  int_ptr<PlayableInstance>   CurItem;*/
  /// current base URL
  url123                      CurDir;
  /// Collected messages
  xstringbuilder              Messages;
  /// Current command in service
  const char*                 Command;
  bool                        CommandType;

 private: // Working set
  JobSet                      SyncJob;

 private: // Helper functions
  /// escape newline characters
  static void EscapeNEL(xstringbuilder& target, size_t start);
  //void PostMessage(MESSAGE_TYPE type, const char* fmt, ...);
  static void DLLENTRY MessageHandler(CommandProcessor* that, MESSAGE_TYPE type, const xstring& msg);
  vdelegate2<void,CommandProcessor,MESSAGE_TYPE,const xstring&> vd_message;
  vdelegate2<void,CommandProcessor,MESSAGE_TYPE,const xstring&> vd_message2; // for thread 1
  //void ThrowSyntaxException(const char* msg, ...);
  //static void ThrowArgumentException(const char* arg)
  //{ throw SyntaxException(xstring::sprintf("Invalid argument \"%s\".", arg)); }

  /// Tag for Request: Query the default value of an option
  static char Cmd_QueryDefault[];
  /// Tag for Request: Set the default value of an option
  static char Cmd_SetDefault[];
  static const strmap<16,Option> OptionMap[];

  /*// Capture and convert the next string at \c Request.
  /// The Parser will either stop at
  /// <ul><li>the next tab ('\t'),</li>
  /// <li>at the end of the string,</li>
  /// <li>if the string does not start with a quote, at the next whitespace or</li>
  /// <li>if the string starts with a quote, at the next quote that is not doubled</li></ul>
  /// Quoted strings are returned without the quotes.
  /// Doubled quotes in quoted strings are converted to a single quote char.
  /// When there is nothing to capture, The function returns NULL.
  /// When the function returned non NULL, \c Request is advanced behind the string.
  char* ParseQuotedString();*/
  /// Parse argument as integer.
  /// @param arg argument as string
  /// @return boolean operator
  /// @exception SyntaxException The argument is not an boolean operator.
  static Ctrl::Op ParseBool(const char* arg);
  /// Parse argument as integer.
  /// @param arg argument as string
  /// @return integer value
  /// @exception SyntaxException The argument is not an integer.
  static int ParseInt(const char* arg);
  /// Parse argument as double.
  /// @param arg argument as string
  /// @return value
  /// @exception SyntaxException The argument is not an number.
  static double ParseDouble(const char* arg);
  /// Parse argument as double.
  /// @param arg argument as string
  /// @return value
  /// @exception SyntaxException The argument is not an number.
  static PM123_TIME ParseTime(const char* arg);
  /// Parse argument as display type.
  /// @param arg argument as string
  /// @return CFG_DISP_...
  /// @exception SyntaxException The argument is not valid.
  static cfg_disp ParseDisp(const char* arg);
  /// Parse and normalize one URL according to CurDir.
  /// @exception SyntaxException The URL is invalid.
  url123 ParseURL(const char* url);
  /// Parse the optional string as URL and return the playable object.
  /// If \a url is empty the current song is returned.
  /// @exception SyntaxException The URL is invalid.
  int_ptr<APlayable> ParseAPlayable(const char* url);
  /// Find plug-in module matching arg.
  /// @return Module or NULL if not found.
  int_ptr<Module> ParsePlugin(const char* arg);

  const volatile amp_cfg& ReadCfg() { return Request == Cmd_QueryDefault ? Cfg::Default : Cfg::Get(); }
  void DoOption(bool amp_cfg::* option);
  void DoOption(int amp_cfg::* option);
  void DoOption(unsigned amp_cfg::* option);
  void DoOption(xstring amp_cfg::* option);
  void DoOption(cfg_anav amp_cfg::* option);
  void DoOption(cfg_button amp_cfg::* option);
  void DoOption(cfg_action amp_cfg::* option);
  void DoOption(cfg_disp amp_cfg::* option);
  void DoOption(cfg_scroll amp_cfg::* option);
  void DoOption(cfg_mode amp_cfg::* option);
  void DoFontOption(void*);

  /// Execute a controller command and return the reply as string.
  void SendCtrlCommand(Ctrl::ControlCommand* cmd);
  /// Add a set of URLs to a LoadHelper object.
  bool FillLoadHelper(LoadHelper& lh, char* args);

 private:
  void XGetMessages();

  void XCd();
  void XReset();

  // PLAYBACK
  void XLoad();
  void XPlay();
  void XEnqueue();
  void XInvoke();
  void XStop();
  void XPause();
  void XNext();
  void XPrev();
  void XRewind();
  void XForward();
  void XJump();
  void XNavigate();
  void XSavestream();
  void XVolume();
  void XShuffle();
  void XRepeat();
  void XQuery();
  void XCurrent();
  void XStatus();
  void XTime();
  void XLocation();

  // PLAYLIST
  /// Parse stopat of navigation commands.
  struct NavParams
  { int             Count;
    TECH_ATTRIBUTES StopAt;
  };
  static void ParsePrevNext(char* arg, NavParams& par);
  void NavigationResult(Location::NavigationResult result);

  void XPlaylist();
  void XPlNext();
  void XPlPrev();
  void XPlNextItem();
  void XPlPrevItem();
  void XPlEnter();
  void XPlLeave();
  void XPlReset();
  void XPlNavigate();
  void XPlCurrent();
  void XPlItem();
  void XPlDepth();
  void XPlCallstack();
  void XPlParent();
  void XPlIndex();
  void XPlItemIndex();
  void XUse();

  // PLAYLIST MODIFICATION
  /// Prepare playlist edit operation. Do some checks.
  /// @return current playlist or \c NULL on error.
  Playable* PrepareEditPlaylist();
  /// Get the current item in the playlist.
  /// Requires \c PrepareEditPlaylist to have succeeded.
  /// @return Current item or NULL if we are at the root.
  PlayableInstance* CurrentPlaylistItem();
  /// Common implementation of Dir and Rdir.
  void PlDir(bool recursive);

  void XPlClear();
  void XPlRemove();
  void XPlAdd();
  void XDir();
  void XRdir();
  void XPlSort();
  void XPlSave();

  // METADATA
  void AppendIntAttribute(const char* fmt, int value);
  void AppendFloatAttribute(const char* fmt, double value);
  void AppendStringAttribute(const char* name, xstring value);
  typedef strmap<10, unsigned> FlagsEntry;
  void AppendFlagsAttribute(const char* name, unsigned flags, const FlagsEntry* map, size_t map_size);
  static const FlagsEntry PhysFlagsMap[];
  static const FlagsEntry TechFlagsMap[];
  static const FlagsEntry PlOptionsMap[];
  void AppendReplayGain(float tg, float tp, float ag, float ap);
  void DoFormatInfo(APlayable& item);
  void DoMetaInfo(APlayable& item);
  void DoPlaylistInfo(APlayable& item);

  void XInfoFormat();
  void XInfoMeta();
  void XInfoPlaylist();
  void XInfoRefresh();
  void XInfoInvalidate();

  void XPlInfoItem();
  void XPlInfoFormat();
  void XPlInfoMeta();
  void XPlInfoPlaylist();

  MetaInfo    Meta;
  DECODERMETA MetaFlags;
  void XWriteMetaSet();
  void XWriteMetaTo();
  void XWriteMetaRst();

  // GUI
  void ShowHideCore(DialogBase::DialogAction action);
  void XShow();
  void XHide();
  void XIsVisible();
  void XQuit();
  void XSkin();

  // CONFIGURATION
  void XVersion();
  void XOption();
  void XDefault();
  void XSize();
  void XFont();
  void XFloat();
  void XAutouse();
  void XPlayonload();

  static const strmap<8,PLUGIN_TYPE> PluginTypeMap[];
  PLUGIN_TYPE ParseTypeList();
  PLUGIN_TYPE ParseType();
  void ReplyType(PLUGIN_TYPE type);
  void AppendPluginType(PLUGIN_TYPE type);
  PLUGIN_TYPE InstantiatePlugin(Module& module, const char* params, PLUGIN_TYPE type);
  PLUGIN_TYPE LoadPlugin(PLUGIN_TYPE type);
  PLUGIN_TYPE UnloadPlugin(PLUGIN_TYPE type);
  bool ReplacePluginList(PLUGIN_TYPE type);

  void XPluginLoad();
  void XPluginUnload();
  void XPluginParams();
  void XPluginList();

 protected:
  /// Executes the Command \c Request and return a value in \c Reply.
  virtual void Exec();

 public:
  CommandProcessor();

  // Plugin manager service window
 private:
  enum
  { /// Instantiate a plug-in.
    /// mp1  CommandProcessor*
    /// mp2  requested PLUGIN_TYPEs to instantiate
    /// mr   PLUGIN_TYPEs successfully instantiated
    UM_LOAD_PLUGIN = WM_USER,
    /// Unload a plug-in.
    /// mp1  CommandProcessor*
    /// mp2  requested PLUGIN_TYPEs to unload
    /// mr   PLUGIN_TYPEs successfully unloaded
    UM_UNLOAD_PLUGIN,
    /// Replace a list of plug-ins.
    /// mp1  CommandProcessor*
    /// mp2  PluginList*, the list implicitly contains the plug-in flavor.
    /// mr   true on success
    UM_LOAD_PLUGIN_LIST
  };
  static HWND ServiceHwnd;
  friend MRESULT EXPENTRY ServiceWinFn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 public:
  static void CreateServiceWindow();
  static void DestroyServiceWindow() { WinDestroyWindow(ServiceHwnd); }
};


template <class T>
inline CommandProcessor::Option::Option(T amp_cfg::* option)
: Handler((void (CommandProcessor::*)(void*))(void (CommandProcessor::*)(T amp_cfg::*))&CommandProcessor::DoOption)
, Arg((void*)option)
{}

#endif
