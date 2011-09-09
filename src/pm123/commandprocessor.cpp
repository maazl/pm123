/*
 * Copyright 2008-2011 M.Mueller
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
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
#define INCL_PM
#include "commandprocessor.h"
#include "dialog.h"
#include "gui.h"
#include "loadhelper.h"
#include "configuration.h"
#include "controller.h"
#include "123_util.h"
#include "copyright.h"
#include "plugman.h"
#include <fileutil.h>
#include <cpp/xstring.h>
#include <cpp/container/stringmap.h>
#include <debuglog.h>
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#define PIPE_BUFFER_SIZE 65536


/****************************************************************************
*
*  helper class ExtLoadHelper
*
****************************************************************************/
class ExtLoadHelper : public LoadHelper
{private:
  Ctrl::ControlCommand* Ext;
 protected:
  // Create a sequence of controller commands from the current list.
  virtual Ctrl::ControlCommand* ToCommand();
 public:
  ExtLoadHelper(Options opt, Ctrl::ControlCommand* ext) : LoadHelper(opt), Ext(ext) {}
};

Ctrl::ControlCommand* ExtLoadHelper::ToCommand()
{ Ctrl::ControlCommand* cmd = LoadHelper::ToCommand();
  if (cmd)
  { Ctrl::ControlCommand* cmde = cmd;
    while (cmde->Link)
      cmde = cmde->Link;
    cmde->Link = Ext;
  } else
    cmd = Ext;
  Ext = NULL;
  return cmd;
}


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

bool URLTokenizer::Next(const char*& url)
{ if (!Args || !*Args)
    return false;
  char* narg;
  if (Args[0] == '"')
  { // quoted item
    narg = strchr(Args+1, '"');
    if (narg)
      *narg++ = 0;
  } else
  { narg = Args + strcspn(Args, " \t");
    if (*narg)
      *narg++ = 0;
  }
  // get url
  url = Args;
  // next argument
  if (narg && *narg)
    Args = narg + strspn(Args, " \t");
  return true;
}


/****************************************************************************
*
*  Private implementation of ACommandProcessor
*
****************************************************************************/
class CommandProcessor : public ACommandProcessor
{private:
  typedef strmap<16, void (CommandProcessor::*)()> CmdEntry;
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
    template <class T>
    Option(T amp_cfg::* option)
    : Handler((void (CommandProcessor::*)(void*))(void (CommandProcessor::*)(T amp_cfg::*))&CommandProcessor::DoOption)
    , Arg((void*)option)
    {}
    void operator()(CommandProcessor& cp) const
    { (cp.*Handler)(Arg);
    }
  };

 private: // state
  /// playlist where we currently operate
  int_ptr<Playable>           CurPlaylist;
  /// current item of the above playlist
  int_ptr<PlayableInstance>   CurItem;
  /// current base URL
  url123                      CurDir;

 private: // Helper functions
  /// Tag for Request: Query the default value of an option
  static char Cmd_QueryDefault[];
  /// Tag for Request: Set the default value of an option
  static char Cmd_SetDefault[];
  static const strmap<16,Option> OptionMap[];

  const volatile amp_cfg& ReadCfg() { return Request == Cmd_QueryDefault ? Cfg::Default : Cfg::Get(); }
  void DoOption(bool amp_cfg::* option);
  void DoOption(int amp_cfg::* option);
  void DoOption(xstring amp_cfg::* option);
  void DoOption(cfg_anav amp_cfg::* option);
  void DoOption(cfg_disp amp_cfg::* option);
  void DoOption(cfg_scroll amp_cfg::* option);
  void DoOption(cfg_mode amp_cfg::* option);
  void DoFontOption(void*);
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
  /// Parse and normalize one URL according to CurDir.
  url123 ParseURL(const char* url);
  /// Parse the optional string as URL and return the playable object.
  /// If \a url is empty the current song is returned.
  int_ptr<APlayable> ParseAPlayable(const char* url);
  /// Execute a controller command and return the reply as string.
  void SendCtrlCommand(Ctrl::ControlCommand* cmd);
  /// Add a set of URLs to a LoadHelper object.
  bool FillLoadHelper(LoadHelper& lh, char* args);
 private:
  // PLAYBACK

  void CmdCd();
  void CmdLoad();
  void CmdPlay();
  void CmdStop();
  void CmdPause();
  void CmdNext();
  void CmdPrev();
  void CmdRewind();
  void CmdForward();
  void CmdJump();
  void CmdSavestream();
  void CmdVolume();
  void CmdShuffle();
  void CmdRepeat();
  void CmdQuery();
  void CmdCurrent();
  void CmdStatus();
  void CmdLocation();

  // PLAYLIST
  /// Move forward/backward in the current playlist
  void PlSkip(int count);

  void CmdPlaylist();
  void CmdPlNext();
  void CmdPlPrev();
  void CmdPlReset();
  void CmdPlCurrent();
  void CmdPlItem();
  void CmdPlIndex();
  void CmdUse();
  void CmdClear();
  void CmdRemove();
  void CmdAdd();
  void CmdDir();
  void CmdRdir();
  void CmdSave();

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

  void CmdInfoFormat();
  void CmdInfoMeta();
  void CmdInfoPlaylist();
  void CmdInfoPlItem();
  void CmdInfoRefresh();
  void CmdInfoInvalidate();

  MetaInfo    Meta;
  DECODERMETA MetaFlags;
  void CmdWriteMetaSet();
  void CmdWriteMetaTo();
  void CmdWriteMetaRst();

  // GUI
  void CmdShow();
  void CmdHide();
  void CmdQuit();
  void CmdOpen();
  void CmdSkin();

  // CONFIGURATION
  void CmdVersion();
  void CmdOption();
  void CmdDefault();
  void CmdSize();
  void CmdFont();
  void CmdFloat();
  void CmdAutouse();
  void CmdPlayonload();

  static const strmap<8,PLUGIN_TYPE> PluginTypeMap[];
  bool ParseTypeList(PLUGIN_TYPE& type);
  void ReplyType(PLUGIN_TYPE type);
  void AppendPluginType(PLUGIN_TYPE type);

  void CmdPluginLoad();
  void CmdPluginUnload();
  void CmdPluginParams();
  void CmdPluginList();

 protected:
  /// Executes the Command \c Request and return a value in \c Reply.
  virtual void Exec();

 public:
  CommandProcessor();
};

const CommandProcessor::CmdEntry CommandProcessor::CmdList[] = // list must be sorted!!!
{ { "add",            &CommandProcessor::CmdAdd           }
, { "autouse",        &CommandProcessor::CmdAutouse       }
, { "cd",             &CommandProcessor::CmdCd            }
, { "clear",          &CommandProcessor::CmdClear         }
, { "current",        &CommandProcessor::CmdCurrent       }
, { "default",        &CommandProcessor::CmdDefault       }
, { "dir",            &CommandProcessor::CmdDir           }
, { "float",          &CommandProcessor::CmdFloat         }
, { "font",           &CommandProcessor::CmdFont          }
, { "forward",        &CommandProcessor::CmdForward       }
, { "hide",           &CommandProcessor::CmdHide          }
, { "info format",    &CommandProcessor::CmdInfoFormat    }
, { "info invalidate",&CommandProcessor::CmdInfoInvalidate}
, { "info meta",      &CommandProcessor::CmdInfoMeta      }
, { "info pl_item",   &CommandProcessor::CmdInfoPlItem    }
, { "info playlist",  &CommandProcessor::CmdInfoPlaylist  }
, { "info refresh" ,  &CommandProcessor::CmdInfoRefresh   }
, { "jump",           &CommandProcessor::CmdJump          }
, { "load",           &CommandProcessor::CmdLoad          }
, { "location",       &CommandProcessor::CmdLocation      }
, { "next",           &CommandProcessor::CmdNext          }
, { "open",           &CommandProcessor::CmdOpen          }
, { "option",         &CommandProcessor::CmdOption        }
, { "pause",          &CommandProcessor::CmdPause         }
, { "pl current",     &CommandProcessor::CmdPlCurrent     }
, { "pl item",        &CommandProcessor::CmdPlItem        }
, { "pl next",        &CommandProcessor::CmdPlNext        }
, { "pl prev",        &CommandProcessor::CmdPlPrev        }
, { "pl reset",       &CommandProcessor::CmdPlReset       }
, { "pl_current",     &CommandProcessor::CmdPlCurrent     }
, { "pl_item",        &CommandProcessor::CmdPlItem        }
, { "pl_next",        &CommandProcessor::CmdPlNext        }
, { "pl_prev",        &CommandProcessor::CmdPlPrev        }
, { "pl_reset",       &CommandProcessor::CmdPlReset       }
, { "play",           &CommandProcessor::CmdPlay          }
, { "playlist",       &CommandProcessor::CmdPlaylist      }
, { "playonload",     &CommandProcessor::CmdPlayonload    }
, { "plugin list",    &CommandProcessor::CmdPluginList    }
, { "plugin load",    &CommandProcessor::CmdPluginLoad    }
, { "plugin params",  &CommandProcessor::CmdPluginParams  }
, { "plugin unload",  &CommandProcessor::CmdPluginUnload  }
, { "prev",           &CommandProcessor::CmdPrev          }
, { "previous",       &CommandProcessor::CmdPrev          }
, { "query",          &CommandProcessor::CmdQuery         }
, { "rdir",           &CommandProcessor::CmdRdir          }
, { "remove",         &CommandProcessor::CmdRemove        }
, { "repeat",         &CommandProcessor::CmdRepeat        }
, { "rewind",         &CommandProcessor::CmdRewind        }
, { "save",           &CommandProcessor::CmdSave          }
, { "savestream",     &CommandProcessor::CmdSavestream    }
, { "show",           &CommandProcessor::CmdShow          }
, { "shuffle",        &CommandProcessor::CmdShuffle       }
, { "size",           &CommandProcessor::CmdSize          }
, { "skin",           &CommandProcessor::CmdSkin          }
, { "status",         &CommandProcessor::CmdStatus        }
, { "stop",           &CommandProcessor::CmdStop          }
, { "use",            &CommandProcessor::CmdUse           }
, { "version",        &CommandProcessor::CmdVersion       }
, { "volume",         &CommandProcessor::CmdVolume        }
, { "write meta rst", &CommandProcessor::CmdWriteMetaRst  }
, { "write meta set", &CommandProcessor::CmdWriteMetaSet  }
, { "write meta to",  &CommandProcessor::CmdWriteMetaTo   }
};

char CommandProcessor::Cmd_QueryDefault[] = ""; // This object is identified by instance
char CommandProcessor::Cmd_SetDefault[] = ""; // This object is identified by instance

static bool parse_int(const char* arg, int& val)
{ int v;
  size_t n;
  if (sscanf(arg, "%i%n", &v, &n) == 1 && n == strlen(arg))
  { val = v;
    return true;
  } else
    return false;
}

static bool parse_double(const char* arg, double& val)
{ double v;
  size_t n;
  if (sscanf(arg, "%lf%n", &v, &n) == 1 && n == strlen(arg))
  { val = v;
    return true;
  } else
    return false;
}

static const strmap<7,Ctrl::Op> opmap[] =
{ { "",       Ctrl::Op_Toggle },
  { "0",      Ctrl::Op_Clear },
  { "1",      Ctrl::Op_Set },
  { "false",  Ctrl::Op_Clear },
  { "no",     Ctrl::Op_Clear },
  { "off",    Ctrl::Op_Clear },
  { "on",     Ctrl::Op_Set },
  { "toggle", Ctrl::Op_Toggle },
  { "true",   Ctrl::Op_Set },
  { "yes",    Ctrl::Op_Set }
};

/* parse operator with default toggle */
inline static const strmap<7, Ctrl::Op>* parse_op1(const char* arg)
{ return mapsearch(opmap, arg);
}
/* parse operator without default */
inline static const strmap<7,Ctrl::Op>* parse_op2(const char* arg)
{ return mapsearch2(opmap+1, (sizeof opmap / sizeof *opmap)-1, arg);
}

static const strmap<5,cfg_disp> dispmap[] =
{ { "file", CFG_DISP_FILENAME }
, { "info", CFG_DISP_FILEINFO }
, { "tag",  CFG_DISP_ID3TAG   }
, { "url",  CFG_DISP_FILENAME }
};

CommandProcessor::CommandProcessor()
: CurPlaylist(&GUI::GetDefaultPL())
, MetaFlags(DECODER_HAVE_NONE)
{}

void CommandProcessor::DoOption(bool amp_cfg::* option)
{ Reply.append(ReadCfg().*option ? "on" : "off");
  if (Request == Cmd_SetDefault)
    Cfg::ChangeAccess().*option = Cfg::Default.*option;
  else if (Request)
  { const strmap<7,Ctrl::Op>* op = parse_op2(Request);
    if (op)
    { Cfg::ChangeAccess cfg;
      switch (op->Val)
      {case Ctrl::Op_Set:
        cfg.*option = true;
        break;
       case Ctrl::Op_Toggle:
        cfg.*option = !(cfg.*option);
        break;
       default: // Op_Clear
        cfg.*option = false;
        break;
      }
    } else
      Reply.clear(); // error
  }
}
void CommandProcessor::DoOption(int amp_cfg::* option)
{ Reply.appendf("%i", ReadCfg().*option);
  if (Request == Cmd_SetDefault)
    Cfg::ChangeAccess().*option = Cfg::Default.*option;
  else if (Request)
  { int val;
    size_t n = 0;
    sscanf(Request, "%i%n", &val, &n);
    if (n == strlen(Request))
      Cfg::ChangeAccess().*option = val;
    else
      Reply.clear();
  }
}
void CommandProcessor::DoOption(xstring amp_cfg::* option)
{ Reply.append(xstring(ReadCfg().*option));
  if (Request == Cmd_SetDefault)
    Cfg::ChangeAccess().*option = Cfg::Default.*option;
  else if (Request)
    Cfg::ChangeAccess().*option = Request;
}
void CommandProcessor::DoOption(cfg_anav amp_cfg::* option)
{ static const strmap<10,cfg_anav> map[] =
  { { "song",      CFG_ANAV_SONG     }
  , { "song,time", CFG_ANAV_SONGTIME }
  , { "time",      CFG_ANAV_TIME     }
  , { "time,song", CFG_ANAV_SONGTIME }
  };
  Reply.append(map[ReadCfg().*option].Str);
  if (Request == Cmd_SetDefault)
    Cfg::ChangeAccess().*option = Cfg::Default.*option;
  else if (Request)
  { const strmap<10,cfg_anav>* mp = mapsearch(map, Request);
    if (mp)
      Cfg::ChangeAccess().*option = mp->Val;
    else
      Reply.clear(); // Error
  }
}
void CommandProcessor::DoOption(cfg_disp amp_cfg::* option)
{ Reply.append(dispmap[3 - ReadCfg().*option].Str);
  if (Request == Cmd_SetDefault)
    Cfg::ChangeAccess().*option = Cfg::Default.*option;
  else if (Request)
  { const strmap<5,cfg_disp>* mp = mapsearch(dispmap, Request);
    if (mp)
      Cfg::ChangeAccess().*option = mp->Val;
    else
      Reply.clear(); // Error
  }
}
void CommandProcessor::DoOption(cfg_scroll amp_cfg::* option)
{ static const strmap<9,cfg_scroll> map[] =
  { { "infinite", CFG_SCROLL_INFINITE }
  , { "none",     CFG_SCROLL_NONE     }
  , { "once",     CFG_SCROLL_ONCE     }
  };
  Reply.append(map[ReadCfg().*option * 5 % 3].Str);
  if (Request == Cmd_SetDefault)
    Cfg::ChangeAccess().*option = Cfg::Default.*option;
  else if (Request)
  { const strmap<9,cfg_scroll>* mp = mapsearch(map, Request);
    if (mp)
      Cfg::ChangeAccess().*option = mp->Val;
    else
      Reply.clear(); // Error
  }
}
void CommandProcessor::DoOption(cfg_mode amp_cfg::* option)
{ // old value
  Reply.append(ReadCfg().*option);
  static const strmap<8,cfg_mode> map[] =
  { { "0",       CFG_MODE_REGULAR }
  , { "1",       CFG_MODE_SMALL }
  , { "2",       CFG_MODE_TINY }
  , { "normal",  CFG_MODE_REGULAR }
  , { "regular", CFG_MODE_REGULAR }
  , { "small",   CFG_MODE_SMALL }
  , { "tiny",    CFG_MODE_TINY }
  };
  if (Request == Cmd_SetDefault)
    Cfg::ChangeAccess().*option = Cfg::Default.*option;
  else if (Request)
  { const strmap<8,cfg_mode>* mp = mapsearch(map, Request);
    if (mp)
      Cfg::ChangeAccess().*option = mp->Val;
    else
      Reply.clear(); // error
  }
};

void CommandProcessor::DoFontOption(void*)
{ // old value
  { Cfg::Access cfg;
    const amp_cfg& rcfg = Request == Cmd_QueryDefault ? Cfg::Default : *cfg;
    if (rcfg.font_skinned)
      Reply.append(rcfg.font+1);
    else
      Reply.append(amp_font_attrs_to_string(rcfg.font_attrs, rcfg.font_size));
  }

  if (Request == Cmd_SetDefault)
  { Cfg::ChangeAccess cfg;
    cfg.font_skinned = Cfg::Default.font_skinned;
    cfg.font_attrs   = Cfg::Default.font_attrs;
    cfg.font_size    = Cfg::Default.font_size;
    cfg.font         = Cfg::Default.font;
  } else if (Request)
  { // set new value
    char* cp = strchr(Request, '.');
    if (!cp)
    { // Skinned font
      int font = 0;
      if (!parse_int(Request, font))
        Reply.clear(); // error
      else
        switch (font)
        {case 1:
         case 2:
          { Cfg::ChangeAccess cfg;
            cfg.font_skinned = true;
            cfg.font         = font;
            break;
          }
         default:
          Reply.clear(); // error
        }
    } else
    { // non skinned font
      FATTRS fattrs = { sizeof(FATTRS), 0, 0, "", 0, 0, 16, 7, 0, 0 };
      unsigned size;
      if (!amp_string_to_font_attrs(fattrs, size, Request))
        Reply.clear(); // error
      else
      { Cfg::ChangeAccess cfg;
        cfg.font_skinned = false;
        cfg.font_attrs   = fattrs;
        cfg.font_size    = size;
        // TODO: we should validate the font here.
      }
    }
  }
}

/*char* CommandProcessor::ParseQuotedString()
{ char* result;
  switch (*Request)
  {case 0:
    return NULL;

   default: // unquoted string
    result = Request;
    Request += strcspn(Request, " \t");
    *Request = 0;
    break;

   case '\'': // quoted string
   case '\"':
    char quotechar = *Request++;
    result = Request;

  }
  return result;
}*/

url123 CommandProcessor::ParseURL(const char* url)
{ url123 ret = CurDir ? CurDir.makeAbsolute(url) : url123::normalizeURL(url);
  // Directory?
  if (ret && ret[ret.length()-1] != '/' && is_dir(ret))
    ret = ret + "/";
  return ret;
}

int_ptr<APlayable> CommandProcessor::ParseAPlayable(const char* url)
{ if (*url == 0)
    return Ctrl::GetCurrentSong();
  const url123& parsed_url = ParseURL(url);
  if (url)
    return Playable::GetByURL(parsed_url).get();
  return NULL;
};

void CommandProcessor::SendCtrlCommand(Ctrl::ControlCommand* cmd)
{ cmd = Ctrl::SendCommand(cmd);
  Reply.append(cmd->Flags);
  cmd->Destroy();
}

bool CommandProcessor::FillLoadHelper(LoadHelper& lh, char* args)
{ URLTokenizer tok(args);
  const char* url;
  // Tokenize args
  while (tok.Next(url))
  { if (!url)
      return false; // Bad URL
    lh.AddItem(Playable::GetByURL(ParseURL(url)));
  }
  return true;
}


const char* ACommandProcessor::Execute(const char* cmd)
{ char* buffer = Request = strdup(cmd);
  Reply.clear();
  Exec();
  free(buffer);
  return Reply.cdata();
}

ACommandProcessor* ACommandProcessor::Create()
{ return new CommandProcessor();
}

void CommandProcessor::Exec()
{ DEBUGLOG(("CommandProcessor::Exec() %s\n", Request));
  Request += strspn(Request, " \t"); // skip leading blanks
  // remove trailing blanks
  { char* ape = Request + strlen(Request);
    while (ape != Request && (ape[-1] == ' ' || ape[-1] == '\t'))
      --ape;
    *ape = 0;
  }
  if (Request[0] != '*')
    CmdLoad();
  else
  { ++Request;
    Request += strspn(Request, " \t"); // skip leading blanks
    // check for plug-in specific commands
    size_t len = strcspn(Request, " \t:");
    if (Request[len] == ':')
    { // Plug-in option
      Request[len] = 0;
      int_ptr<Module> plugin(Module::FindByKey(Request));
      if (!plugin)
        return; // Plug-in not found.
      Request += len+1;
      Request += strspn(Request, " \t"); // skip leading blanks
      // Call plug-in function
      xstring result;
      plugin->Command(Request, result);
      Reply.append(result);
      return;
    }
    // Search command handler ...
    const CmdEntry* cep = mapsearcha(CmdList, Request);
    DEBUGLOG(("CommandProcessor::Exec: %s -> %p(%s)\n", Request, cep, cep));
    if (cep)
    { size_t len = strlen(cep->Str);
      len += strspn(Request + len, " \t");
      Request += len;
      DEBUGLOG(("CommandProcessor::Exec: %u %s\n", len, Request));
      if (len || *Request == 0)
        // ... and execute
        (this->*cep->Val)();
    }
  }
}


/****************************************************************************
*
*  remote command handlers
*
****************************************************************************/
// PLAY CONTROL

void CommandProcessor::CmdCd()
{ // return current value
  if (CurDir)
    Reply.append(CurDir);
  // set new dir
  if (*Request)
    CurDir = ParseURL(Request);
}


void CommandProcessor::CmdLoad()
{ LoadHelper lh(Cfg::Get().playonload*LoadHelper::LoadPlay | Cfg::Get().append_cmd*LoadHelper::LoadAppend);
  Reply.append(!FillLoadHelper(lh, Request) ? Ctrl::RC_BadArg : lh.SendCommand());
}

void CommandProcessor::CmdPlay()
{ ExtLoadHelper lh( Cfg::Get().playonload*LoadHelper::LoadPlay | Cfg::Get().append_cmd*LoadHelper::LoadAppend,
                    Ctrl::MkPlayStop(Ctrl::Op_Set) );
  Reply.append(!FillLoadHelper(lh, Request) ? Ctrl::RC_BadArg : lh.SendCommand());
}

void CommandProcessor::CmdStop()
{ SendCtrlCommand(Ctrl::MkPlayStop(Ctrl::Op_Clear));
}

void CommandProcessor::CmdPause()
{ const strmap<7,Ctrl::Op>* op = parse_op1(Request);
  if (!op)
    Reply.append(Ctrl::RC_BadArg);
  else
    SendCtrlCommand(Ctrl::MkPause(op->Val));
}

void CommandProcessor::CmdNext()
{ int count = 1;
  if (*Request && !parse_int(Request, count))
    Reply.append(Ctrl::RC_BadArg);
  else
    SendCtrlCommand(Ctrl::MkSkip(count, true));
}

void CommandProcessor::CmdPrev()
{ int count = 1;
  if (*Request && !parse_int(Request, count))
    Reply.append(Ctrl::RC_BadArg);
  else
    SendCtrlCommand(Ctrl::MkSkip(-count, true));
}

void CommandProcessor::CmdRewind()
{ const strmap<7,Ctrl::Op>* op = parse_op1(Request);
  if (!op)
    Reply.append(Ctrl::RC_BadArg);
  else
    SendCtrlCommand(Ctrl::MkScan(op->Val|Ctrl::Op_Rewind));
}

void CommandProcessor::CmdForward()
{ const strmap<7,Ctrl::Op>* op = parse_op1(Request);
  if (!op)
    Reply.append(Ctrl::RC_BadArg);
  else
    SendCtrlCommand(Ctrl::MkScan(op->Val));
}

void CommandProcessor::CmdJump()
{ double pos;
  if (!parse_double(Request, pos))
    Reply.append(Ctrl::RC_BadArg);
  else
    SendCtrlCommand(Ctrl::MkNavigate(xstring(), pos, false, false));
}

void CommandProcessor::CmdSavestream()
{ SendCtrlCommand(Ctrl::MkSave(Request));
}

void CommandProcessor::CmdVolume()
{ Reply.appendf("%f", Ctrl::GetVolume());
  if (*Request)
  { double vol;
    bool sign = *Request == '+' || *Request == '-';
    if (parse_double(Request, vol))
    { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkVolume(vol, sign));
      if (cmd->Flags != 0)
        Reply.clear(); //error
      cmd->Destroy();
    } else
      Reply.clear(); //error
  }
}

void CommandProcessor::CmdShuffle()
{ const strmap<7,Ctrl::Op>* op = parse_op1(Request);
  if (!op)
    Reply.append(Ctrl::RC_BadArg);
  else
    SendCtrlCommand(Ctrl::MkShuffle(op->Val));
}

void CommandProcessor::CmdRepeat()
{ const strmap<7,Ctrl::Op>* op = parse_op1(Request);
  if (!op)
    Reply.append(Ctrl::RC_BadArg);
  else
    SendCtrlCommand(Ctrl::MkRepeat(op->Val));
}

static bool IsRewind()
{ return Ctrl::GetScan() == DECFAST_REWIND;
}
static bool IsForward()
{ return Ctrl::GetScan() == DECFAST_FORWARD;
}
void CommandProcessor::CmdQuery()
{ static const strmap<8, bool (*)()> map[] =
  { { "forward", &IsForward       },
    { "pause",   &Ctrl::IsPaused  },
    { "play",    &Ctrl::IsPlaying },
    { "repeat",  &Ctrl::IsRepeat  },
    { "rewind",  &IsRewind        },
    { "shuffle", &Ctrl::IsShuffle }
  };
  const strmap<8, bool (*)()>* op = mapsearch(map, Request);
  if (op)
    Reply.append((*op->Val)());
}

void CommandProcessor::CmdCurrent()
{ static const strmap<5,char> map[] =
  { { "",     0 },
    { "root", 1 },
    { "song", 0 }
  };
  const strmap<5,char>* op = mapsearch(map, Request);
  if (op)
  { int_ptr<APlayable> cur;
    switch (op->Val)
    {case 0: // current song
      cur = Ctrl::GetCurrentSong();
      break;
     case 1:
      cur = Ctrl::GetRoot();
      break;
    }
    if (cur)
      Reply.append(cur->GetPlayable().URL);
  }
}

void CommandProcessor::CmdStatus()
{ const strmap<5,cfg_disp>* op = mapsearch(dispmap, Request);
  if (op)
  { int_ptr<APlayable> song = Ctrl::GetCurrentSong();
    if (song)
    { switch (op->Val)
      {case CFG_DISP_ID3TAG:
        Reply.append(amp_construct_tag_string(&song->GetInfo()));
        if (Reply.length())
          break;
        // if tag is empty - use filename instead of it.
       case CFG_DISP_FILENAME:
        Reply.append(song->GetPlayable().URL.getShortName());
        break;

       case CFG_DISP_FILEINFO:
        Reply.append(xstring(song->GetInfo().tech->info));
        break;
      }
    }
  }
}

void CommandProcessor::CmdLocation()
{ static const strmap<7, char> map[] =
  { { "",       0 },
    { "play",   0 },
    { "stopat", 1 }
  };
  const strmap<7, char>* op = mapsearch(map, Request);
  if (op)
  { SongIterator loc;
    Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkLocation(&loc, op->Val));
    if (cmd->Flags == Ctrl::RC_OK)
      Reply.append(loc.Serialize());
    cmd->Destroy();
  }
}

// PLAYLIST

void CommandProcessor::CmdPlaylist()
{ int_ptr<Playable> pp = Playable::GetByURL(ParseURL(Request));
  //if (pp->GetFlags() & Playable::Enumerable)
  { CurPlaylist = pp;
    CurItem = NULL;
    Reply.append(CurPlaylist->URL);
  }
};

void CommandProcessor::PlSkip(int count)
{ CurPlaylist->RequestInfo(IF_Child, PRI_Sync);
  int_ptr<PlayableInstance> (Playable::*dir)(const PlayableInstance*) const
    = &Playable::GetNext;
  if (count < 0)
  { count = -count;
    dir = &Playable::GetPrev;
  }
  while (count--)
  { CurItem = (CurPlaylist->*dir)(CurItem);
    if (!CurItem)
      break;
  }
}

void CommandProcessor::CmdPlNext()
{ int count = 1;
  if (CurPlaylist && (*Request == 0 || parse_int(Request, count)))
  { PlSkip(count);
    CmdPlItem();
  }
}

void CommandProcessor::CmdPlPrev()
{ int count = 1;
  if (CurPlaylist && (*Request == 0 || parse_int(Request, count)))
  { PlSkip(-count);
    CmdPlItem();
  }
}

void CommandProcessor::CmdPlReset()
{ CurItem = NULL;
}

void CommandProcessor::CmdPlCurrent()
{ if (CurPlaylist)
    Reply.append(CurPlaylist->URL);
}

void CommandProcessor::CmdPlItem()
{ if (CurItem)
    Reply.append(CurItem->GetPlayable().URL);
}

void CommandProcessor::CmdPlIndex()
{ int ix = 0;
  int_ptr<PlayableInstance> cur = CurItem;
  while (cur)
  { ++ix;
    cur = CurPlaylist->GetPrev(CurItem);
  }
  Reply.append(ix);
}

void CommandProcessor::CmdUse()
{ if (CurPlaylist)
  { LoadHelper lh(Cfg::Get().playonload*LoadHelper::LoadPlay | Cfg::Get().append_cmd*LoadHelper::LoadAppend);
    lh.AddItem(CurPlaylist);
    Reply.append(lh.SendCommand());
  } else
    Reply.append(Ctrl::RC_NoList);
};

void CommandProcessor::CmdClear()
{ if (CurPlaylist)
    CurPlaylist->Clear();
}

void CommandProcessor::CmdRemove()
{ if (CurItem)
  { CurPlaylist->RemoveItem(CurItem);
    Reply.append(CurItem->GetPlayable().URL);
  }
}

void CommandProcessor::CmdAdd()
{ if (CurPlaylist)
  { URLTokenizer tok(Request);
    const char* url;
    Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    while (tok.Next(url))
    { if (!url)
      { Reply.append(tok.Current());
        break; // Bad URL
      }
      CurPlaylist->InsertItem(*Playable::GetByURL(ParseURL(url)), CurItem);
    }
  }
}

void CommandProcessor::CmdDir()
{ if (CurPlaylist)
  { URLTokenizer tok(Request);
    const char* curl;
    Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    while (tok.Next(curl))
    { url123 url = ParseURL(curl);
      if (!url || strchr(url, '?'))
      { Reply.append(tok.Current());
        break; // Bad URL
      }
      if (url[url.length()-1] != '/')
        url = url + "/";
      CurPlaylist->InsertItem(*Playable::GetByURL(url), CurItem);
    }
  }
}

void CommandProcessor::CmdRdir()
{ if (CurPlaylist)
  { URLTokenizer tok(Request);
    const char* curl;
    Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    while (tok.Next(curl))
    { url123 url = ParseURL(curl);
      if (!url || strchr(url, '?'))
      { Reply.append(tok.Current());
        break; // Bad URL
      }
      if (url[url.length()-1] != '/')
        url = url + "/";
      CurPlaylist->InsertItem(*Playable::GetByURL(url+"?recursive"), CurItem);
    }
  }
}

void CommandProcessor::CmdSave()
{ int rc = Ctrl::RC_NoList;
  if (CurPlaylist)
  { const char* savename = Request;
    if (!*savename)
    { /*if ((CurPlaylist->GetFlags() & Playable::Mutable) != Playable::Mutable)
      { rc = Ctrl::RC_InvalidItem;
        goto end;
      }*/
      savename = CurPlaylist->URL;
    }
    /* TODO !!!
    rc = CurPlaylist->Save(savename, cfg.save_relative*PlayableCollection::SaveRelativePath)
      ? Ctrl::RC_OK : -1;*/
  }
 end:
  Reply.append(rc);
}

// METADATA

inline void CommandProcessor::AppendIntAttribute(const char* fmt, int value)
{ if (value >= 0)
    Reply.appendf(fmt, value);
}

inline void CommandProcessor::AppendFloatAttribute(const char* fmt, double value)
{ if (value >= 0)
    Reply.appendf(fmt, value);
}

void CommandProcessor::AppendStringAttribute(const char* name, xstring value)
{ Reply.append(name);
  if (value)
  { Reply.append('=');
    if (value.length() != 0)
    { size_t pos = Reply.length();
      Reply.append(value);
      // escape some special characters
      while (true)
      { pos = Reply.find_any("\r\n\x1b", pos);
        if (pos >= Reply.length())
          break;
        switch(Reply[pos])
        {case '\r':
          Reply[pos] = 'r';
          break;
         case '\n':
           Reply[pos] = 'n';
        }
        Reply.insert(pos, '\x1b');
        pos += 2;
    } }
  }
  Reply.append('\n');
}

void CommandProcessor::AppendFlagsAttribute(const char* name, unsigned flags, const FlagsEntry* map, size_t map_size)
{ size_t prevlen = Reply.length();
  Reply.append(name);
  Reply.append('=');
  size_t startlen = Reply.length();
  for (const FlagsEntry* ep = map + map_size; map != ep; ++map)
    if (flags & map->Val)
    { Reply.append(map->Str);
      Reply.append(' ');
    }
  if (Reply.length() == startlen)
    Reply.erase(prevlen); // Not even one flag set => revert
  else
    Reply[Reply.length()-1] = '\n'; // replace last blank by newline
}
#define AppendFlagsAttribute2(name, flags, map) AppendFlagsAttribute(name, flags, map, sizeof(map)/sizeof(*map))

const CommandProcessor::FlagsEntry CommandProcessor::PhysFlagsMap[] =
{ {"invalid",  PATTR_INVALID }
, {"writable", PATTR_WRITABLE}
};
const CommandProcessor::FlagsEntry CommandProcessor::TechFlagsMap[] =
{ {"invalid",  TATTR_INVALID }
, {"playlist", TATTR_PLAYLIST}
, {"song",     TATTR_SONG    }
, {"storable", TATTR_STORABLE}
, {"writable", TATTR_WRITABLE}
};
const CommandProcessor::FlagsEntry CommandProcessor::PlOptionsMap[] =
{ {"alt",      PLO_ALTERNATION}
, {"noshuffle",PLO_NO_SHUFFLE }
, {"shuffle",  PLO_SHUFFLE    }
};

void CommandProcessor::AppendReplayGain(float tg, float tp, float ag, float ap)
{ if (tg > -1000 || tp > -1000 || ag > -1000 || ap > -1000)
  { Reply.append("REPLAYGAIN=");
    if (tg > -1000)
      Reply.appendf("%.1f", tg);
    Reply.append(',');
    if (tp > -1000)
      Reply.appendf("%.1f", tp);
    Reply.append(',');
    if (ag > -1000)
      Reply.appendf("%.1f", ag);
    Reply.append(',');
    if (ap > -1000)
      Reply.appendf("%.1f", ap);
    Reply.append('\n');
  }
}

void CommandProcessor::CmdInfoFormat()
{ // get song object
  int_ptr<APlayable> song(ParseAPlayable(Request));
  if (song == NULL)
    return;
  // get info
  song->RequestInfo(IF_Phys|IF_Tech|IF_Obj, PRI_Sync);
  const INFO_BUNDLE_CV& info = song->GetInfo();
  // return info
  xstring tmpstr;
  { // Physical info
    const volatile PHYS_INFO& phys = *info.phys;
    AppendFloatAttribute("FILESIZE=%.0f\n", phys.filesize);
    time_t time = phys.tstmp;
    if (time != -1)
    { struct tm* tm = localtime(&time);
      char buffer[32];
      strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", tm);
      Reply.appendf("FILETIME=%s\n", buffer);
    }
    AppendFlagsAttribute2("FILEATTR", phys.attributes, PhysFlagsMap);
  }
  { // Technical info
    const volatile TECH_INFO& tech = *info.tech;
    AppendIntAttribute("SAMPLERATE=%i\n", tech.samplerate);
    AppendIntAttribute("CHANNELS=%i\n", tech.channels);
    AppendFlagsAttribute2("FLAGS", tech.attributes, TechFlagsMap);
    AppendStringAttribute("TECHINFO", tech.info);
    AppendStringAttribute("DECODER", tech.decoder);
    AppendStringAttribute("FORMAT", tech.format);
  }
  { // Object info
    const volatile OBJ_INFO& obj = *info.obj;
    AppendFloatAttribute("SONGLENGTH=%.6f\n", obj.songlength);
    AppendIntAttribute("BITRATE=%i\n", obj.bitrate);
    AppendIntAttribute("SUBITEMS=%i\n", obj.num_items);
  }
}

void CommandProcessor::CmdInfoMeta()
{ // get the song object
  int_ptr<APlayable> song(ParseAPlayable(Request));
  if (song == NULL)
    return;
  // get info
  song->RequestInfo(IF_Meta, PRI_Sync);
  const volatile META_INFO& meta = *song->GetInfo().meta;
  AppendStringAttribute("TITLE", meta.title);
  AppendStringAttribute("ARTIST", meta.artist);
  AppendStringAttribute("ALBUM", meta.album);
  AppendStringAttribute("YEAR", meta.year);
  AppendStringAttribute("COMMENT", meta.comment);
  AppendStringAttribute("GENRE", meta.genre);
  AppendStringAttribute("TRACK", meta.track);
  AppendStringAttribute("COPYRIGHT", meta.copyright);
  AppendReplayGain(meta.track_gain, meta.track_peak, meta.album_gain, meta.album_peak);
}

void CommandProcessor::CmdInfoPlaylist()
{
  int_ptr<Playable> list;
  if (*Request == 0)
    list = CurPlaylist;
  else
  { const url123& url = ParseURL(Request);
    if (url)
      list = Playable::GetByURL(url);
  }
  if (list == NULL)
    return;
  // get info
  list->RequestInfo(IF_Rpl|IF_Drpl, PRI_Sync);
  { const volatile RPL_INFO& rpl = *list->GetInfo().rpl;
    AppendIntAttribute("SONGS=%u\n", rpl.songs);
    AppendIntAttribute("LISTS=%u\n", rpl.lists);
    AppendIntAttribute("INVALID=%u\n", rpl.invalid);
  }
  { const volatile DRPL_INFO& drpl = *list->GetInfo().drpl;
    AppendFloatAttribute("TOTALLENGTH=%.6f\n", drpl.totallength);
    AppendFloatAttribute("TOTALSIZE=%.0f\n", drpl.totalsize);
  }
}

void CommandProcessor::CmdInfoPlItem()
{ if (CurItem == NULL)
    return;
  CurItem->RequestInfo(IF_Item|IF_Attr, PRI_Sync);
  { const volatile ITEM_INFO& item = *CurItem->GetInfo().item;
    AppendStringAttribute("ALIAS", item.alias);
    AppendStringAttribute("START", item.start);
    AppendStringAttribute("STOP", item.stop);
    AppendFloatAttribute("PREGAP=%.6f\n", item.pregap);
    AppendFloatAttribute("POSTGAP=%.6f\n", item.postgap);
    AppendFloatAttribute("GAIN=%.1f\n", item.gain);
  }
  { const volatile ATTR_INFO& attr = *CurItem->GetInfo().attr;
    AppendFlagsAttribute2("OPTIONS", attr.ploptions, PlOptionsMap);
    AppendStringAttribute("AT", attr.at);
  }
}

void CommandProcessor::CmdInfoRefresh()
{ // get the song object
  int_ptr<APlayable> song(ParseAPlayable(Request));
  if (song == NULL)
    return;
  song->RequestInfo(~IF_None, PRI_Sync, REL_Reload);
}

void CommandProcessor::CmdInfoInvalidate()
{ // get the song object
  int_ptr<APlayable> song(ParseAPlayable(Request));
  if (song == NULL)
    return;
  song->Invalidate(~IF_None);
}

struct meta_val
{ xstring META_INFO::* Field;
  DECODERMETA          Flag;
};
void CommandProcessor::CmdWriteMetaSet()
{
  static const strmap<12,meta_val> map[] =
  { { "ALBUM",     { &META_INFO::album    , DECODER_HAVE_ALBUM    } }
  , { "ARTIST",    { &META_INFO::artist   , DECODER_HAVE_ARTIST   } }
  , { "COMMENT",   { &META_INFO::comment  , DECODER_HAVE_COMMENT  } }
  , { "COPYRIGHT", { &META_INFO::copyright, DECODER_HAVE_COPYRIGHT} }
  , { "GENRE",     { &META_INFO::genre    , DECODER_HAVE_GENRE    } }
  , { "TITLE",     { &META_INFO::title    , DECODER_HAVE_TITLE    } }
  , { "TRACK",     { &META_INFO::track    , DECODER_HAVE_TRACK    } }
  , { "YEAR",      { &META_INFO::year     , DECODER_HAVE_YEAR     } }
  };

  char* cp = strchr(Request, '=');
  if (cp)
    *cp = 0;

  const strmap<12,meta_val>* op = mapsearch(map, Request);
  if (!op)
    return;

  xstring& val = Meta.*op->Val.Field;
  // return old value
  if ((MetaFlags & op->Val.Flag) && val)
    Reply.append(val);

  MetaFlags |= op->Val.Flag;
  if (!cp)
    val.reset();
  else
  { // Value given => replace escape sequences
    Request = ++cp;
    while ((cp = strchr(cp, 27)) != NULL)
    { strcpy(cp, cp+1); // remove <ESC>
      switch (*cp)
      {case 0:
        goto done;
       case 'r':
        *cp = '\r';
        break;
       case 'n':
        *cp = '\n';
      }
      ++cp;
    }
   done:
    val = Request;
    DEBUGLOG(("CommandProcessor::CmdWriteMetaSet: %s = %s\n", op->Str, Request));
  }
}

void CommandProcessor::CmdWriteMetaTo()
{ if (*Request == 0)
    return;
  const url123& url = ParseURL(Request);
  if (!url)
    return;
  int_ptr<Playable> song = Playable::GetByURL(url);
  // Write meta
  song->RequestInfo(IF_Tech, PRI_Sync);
  xstring errortxt;
  Reply.appendf("%i", song->SaveMetaInfo(Meta, MetaFlags, errortxt));
  if (errortxt)
  { Reply.append(' ');
    Reply.append(errortxt);
  }
}

void CommandProcessor::CmdWriteMetaRst()
{ // return old values
  if (MetaFlags & DECODER_HAVE_TITLE)
    AppendStringAttribute("TITLE", Meta.title);
  if (MetaFlags & DECODER_HAVE_ARTIST)
    AppendStringAttribute("ARTIST", Meta.artist);
  if (MetaFlags & DECODER_HAVE_ALBUM)
    AppendStringAttribute("ALBUM", Meta.album);
  if (MetaFlags & DECODER_HAVE_YEAR)
    AppendStringAttribute("YEAR", Meta.year);
  if (MetaFlags & DECODER_HAVE_COMMENT)
    AppendStringAttribute("COMMENT", Meta.comment);
  if (MetaFlags & DECODER_HAVE_GENRE)
    AppendStringAttribute("GENRE", Meta.genre);
  if (MetaFlags & DECODER_HAVE_TRACK)
    AppendStringAttribute("TRACK", Meta.track);
  if (MetaFlags & DECODER_HAVE_COPYRIGHT)
    AppendStringAttribute("COPYRIGHT", Meta.copyright);
  // and clear
  Meta.Reset();
  MetaFlags = stricmp(Request, "purge") != 0
    ? DECODER_HAVE_NONE
    : DECODER_HAVE_TITLE|DECODER_HAVE_ARTIST|DECODER_HAVE_ALBUM|DECODER_HAVE_YEAR|DECODER_HAVE_COMMENT|DECODER_HAVE_GENRE|DECODER_HAVE_TRACK|DECODER_HAVE_COPYRIGHT;
}

// GUI

void CommandProcessor::CmdShow()
{ GUI::Show(true);
}

void CommandProcessor::CmdHide()
{ GUI::Show(false);
}

void CommandProcessor::CmdQuit()
{ GUI::Quit();
}

void CommandProcessor::CmdOpen()
{ char* arg2 = Request + strcspn(Request, " \t");
  if (*arg2)
  { *arg2++ = 0;
    arg2 += strspn(arg2, " \t");
  }
  static const strmap<12,int> map[] =
  { { "detailed",   GUI::DLT_PLAYLIST },
    { "metainfo",   GUI::DLT_METAINFO },
    { "playlist",   GUI::DLT_PLAYLIST },
    { "properties", -1           },
    { "techinfo",   GUI::DLT_TECHINFO },
    { "tagedit",    GUI::DLT_INFOEDIT },
    { "tree",       GUI::DLT_PLAYLISTTREE }
  };
  const strmap<12,int>* op = mapsearch(map, Request);
  if (op)
  { if (op->Val >= 0)
    { if (*arg2 == 0)
        return; // Error
      int_ptr<Playable> pp = Playable::GetByURL(ParseURL(arg2));
      GUI::ShowDialog(*pp, (GUI::DialogType)op->Val);
    } else
    { // properties
      if (*arg2)
      { int_ptr<Module> mod = Module::FindByKey(arg2);
        if (mod)
          GUI::ShowConfig(*mod);
      } else
      { GUI::ShowConfig();
      }
    }
  }
}

void CommandProcessor::CmdSkin()
{ Reply.append(xstring(Cfg::Get().defskin));
  if (*Request)
    Cfg::ChangeAccess().defskin = Request;
}

// CONFIGURATION

void CommandProcessor::CmdVersion()
{ Reply.append(AMP_VERSION);
}

// PM123 option
const strmap<16,CommandProcessor::Option> CommandProcessor::OptionMap[] =
{ { "altslider",       &amp_cfg::altnavig       }
, { "autouse",         &amp_cfg::autouse        }
, { "bufferlevel",     &amp_cfg::buff_fill      }
, { "buffersize",      &amp_cfg::buff_size      }
, { "bufferwait",      &amp_cfg::buff_wait      }
, { "conntimeout",     &amp_cfg::conn_timeout   }
, { "dndrecurse",      &amp_cfg::recurse_dnd    }
, { "dockmargin",      &amp_cfg::dock_margin    }
, { "dockwindows",     &amp_cfg::dock_windows   }
, { "foldersautosort", &amp_cfg::sort_folders   }
, { "foldersfirst",    &amp_cfg::folders_first  }
, { "font",            &CommandProcessor::DoFontOption }
, { "pipe",            &amp_cfg::pipe_name      }
, { "playlistwrap",    &amp_cfg::autoturnaround }
, { "playonload",      &amp_cfg::playonload     }
, { "proxyserver",     &amp_cfg::proxy          }
, { "proxyauth",       &amp_cfg::auth           }
, { "queueatcommand",  &amp_cfg::append_cmd     }
, { "queueatdnd",      &amp_cfg::append_dnd     }
, { "queuedelplayed",  &amp_cfg::queue_mode     }
, { "resumeatstartup", &amp_cfg::restartonstart }
, { "retainposonexit", &amp_cfg::retainonexit   }
, { "retainposonstop", &amp_cfg::retainonstop   }
, { "scrollmode",      &amp_cfg::scroll         }
, { "scrollaround",    &amp_cfg::scroll_around  }
, { "skin",            &amp_cfg::defskin        }
, { "textdisplay",     &amp_cfg::viewmode       }
, { "windowposbyobj",  &amp_cfg::win_pos_by_obj }
, { "workersdlg",      &amp_cfg::num_dlg_workers}
, { "workersnorm",     &amp_cfg::num_workers    }
};

void CommandProcessor::CmdOption()
{
  char* cp = strchr(Request, '=');
  if (cp)
    *cp++ = 0;

  const strmap<16,Option>* op = mapsearch(OptionMap, Request);
  if (op)
  { Request = cp;
    op->Val(*this);
  }
}

void CommandProcessor::CmdDefault()
{
  size_t len = strcspn(Request, " \t");
  Request[len] = 0;

  const strmap<16,Option>* op = mapsearch(OptionMap, Request);
  if (op)
  { Request += len;
    Request += strspn(Request, " \t");

    if (stricmp(Request, "query") == 0)
      Request = Cmd_QueryDefault;
    else if (*Request && stricmp(Request, "reset") != 0)
      return;
    else
      Request = Cmd_SetDefault;

    op->Val(*this);
  }
}

void CommandProcessor::CmdSize()
{ if (!*Request)
    Request = NULL;
  DoOption(&amp_cfg::mode);
};

void CommandProcessor::CmdFont()
{ if (!*Request)
    Request = NULL;
  DoFontOption(NULL);
}

void CommandProcessor::CmdFloat()
{ if (!*Request)
    Request = NULL;
  DoOption(&amp_cfg::floatontop);
}

void CommandProcessor::CmdAutouse()
{ if (!*Request)
    Request = NULL;
  DoOption(&amp_cfg::autouse);
}

void CommandProcessor::CmdPlayonload()
{ if (!*Request)
    Request = NULL;
  DoOption(&amp_cfg::playonload);
}

const strmap<8,PLUGIN_TYPE> CommandProcessor::PluginTypeMap[] =
{ { "all",     PLUGIN_VISUAL|PLUGIN_FILTER|PLUGIN_DECODER|PLUGIN_OUTPUT }
, { "decoder", PLUGIN_DECODER }
, { "filter",  PLUGIN_FILTER }
, { "output",  PLUGIN_OUTPUT }
, { "visual",  PLUGIN_VISUAL }
};

bool CommandProcessor::ParseTypeList(PLUGIN_TYPE& type)
{ for (const char* cp = strtok(Request, ",|"); cp; cp = strtok(NULL, ",|"))
  { const strmap<8,PLUGIN_TYPE>* op = mapsearch(PluginTypeMap, cp);
    if (op == NULL)
      return false;
    type |= op->Val;
  }
  return true;
}

void CommandProcessor::ReplyType(PLUGIN_TYPE type)
{ size_t start = Reply.length();
  const strmap<8,PLUGIN_TYPE>* tpp = PluginTypeMap;
  const strmap<8,PLUGIN_TYPE>* tpe = tpp + sizeof PluginTypeMap / sizeof *PluginTypeMap;
  while (++tpp != tpe)
    if (type & tpp->Val)
    { Reply.append(tpp->Str);
      Reply.append(',');
    }
  if (Reply.length() != start)
    Reply.erase(Reply.length()-1);
}

void CommandProcessor::AppendPluginType(PLUGIN_TYPE type)
{ size_t pos = Reply.length();
  for (const strmap<8,PLUGIN_TYPE>* op = PluginTypeMap +1
    ; op < PluginTypeMap + sizeof PluginTypeMap/sizeof *PluginTypeMap
    ; ++op)
  { if (type & op->Val)
    { Reply.append(op->Str);
      Reply.append(',');
    }
  }
  if (pos == Reply.length())
    Reply.append('0');
  else
    Reply.erase(Reply.length()-1);
}

static PLUGIN_TYPE DoDeserialize(const char* str, PLUGIN_TYPE type)
{ if (type)
    try
    { Plugin::AppendPlugin(Plugin::Deserialize(str, type));
      return type;
    } catch (const ModuleException& ex)
    { // TODO: log exception
    }
  return PLUGIN_NULL;
}

void CommandProcessor::CmdPluginLoad()
{
  size_t len = strcspn(Request, " \t");
  if (Request[len] == 0)
    return; // Plug-in name missing
  Request[len] = 0;

  PLUGIN_TYPE type = PLUGIN_NULL;
  if (!ParseTypeList(type))
    return; // Syntax error

  Request += len;
  Request += strspn(Request, " \t");
  // TODO: AppendPluginType(Plugin::Deserialize(Request, op->Val));

  type = DoDeserialize(Request, type & PLUGIN_DECODER)
       | DoDeserialize(Request, type & PLUGIN_FILTER)
       | DoDeserialize(Request, type & PLUGIN_OUTPUT)
       | DoDeserialize(Request, type & PLUGIN_VISUAL);

  ReplyType(type);
}

static PLUGIN_TYPE DoUnload(Module& module, PLUGIN_TYPE type)
{ if (type)
  { PluginList list(type);
    Mutex::Lock lock(Module::Mtx);
    Plugin::GetPlugins(list, false);
    const int_ptr<Plugin>* ppp = list.begin();
    while (ppp != list.end())
      if (&(*ppp)->ModRef == &module)
      { list.erase(ppp);
        Plugin::SetPluginList(list);
        return type;
      } else
        ++ppp;
  }
  return PLUGIN_NULL;
}

void CommandProcessor::CmdPluginUnload()
{ size_t len = strcspn(Request, " \t");
  if (Request[len] == 0)
    return; // Plug-in name missing
  Request[len] = 0;

  PLUGIN_TYPE type = PLUGIN_NULL;
  if (!ParseTypeList(type))
    return; // Syntax error

  Request += len;
  Request += strspn(Request, " \t");
  int_ptr<Module> module(Module::FindByKey(Request));
  if (module == NULL)
    return;

  type = DoUnload(*module, type & PLUGIN_DECODER)
       | DoUnload(*module, type & PLUGIN_FILTER)
       | DoUnload(*module, type & PLUGIN_OUTPUT)
       | DoUnload(*module, type & PLUGIN_VISUAL);

  ReplyType(type);
}

void CommandProcessor::CmdPluginList()
{ size_t len = strcspn(Request, " \t");
  char* np = Request + len;
  if (*np)
  { *np++ = 0;
    np += strspn(np, " \t");
  }
  // requested plug-in type
  const strmap<8,PLUGIN_TYPE>* op = mapsearch2(PluginTypeMap+1, sizeof PluginTypeMap/sizeof *PluginTypeMap -1, Request);
  if (op == NULL)
    return;
  // Current state
  PluginList list(op->Val);
  if (*np)
  { // set plug-in list
    try
    { list.Deserialize(np);
      Plugin::SetPluginList(list);
    } catch (const ModuleException& ex)
    { // TODO: log error
      return;
    }
  } else
  { // get plug-in list
    Plugin::GetPlugins(list, false);
  }
  Reply.append(list.Serialize());
}

void CommandProcessor::CmdPluginParams()
{ size_t len = strcspn(Request, " \t");
  if (!Request[len])
    return; // missing plug-in name
  // requested plug-in type
  const strmap<8,PLUGIN_TYPE>* op = mapsearch2(PluginTypeMap+1, sizeof PluginTypeMap/sizeof *PluginTypeMap -1, Request);
  if (op == NULL)
    return; // invalid plug-in type
  Request += len;
  Request += strspn(Request, " \t");

  char* params = strchr(Request, '?');
  if (params)
    *params++ = 0;
  // find plug-in
  int_ptr<Module> pm = Module::FindByKey(Request);
  if (!pm)
    return;
  int_ptr<Plugin> pp = Plugin::FindInstance(*pm, op->Val);
  if (!pp)
    return;
  // return current parameters
  pp->Serialize(Reply);
  // Set new parameters
  if (params)
  { stringmap_own sm(20);
    url123::parseParameter(sm, params+1);
    try
    { pp->SetParams(sm);
    } catch (const ModuleException& ex)
    { // TODO: log exception
    }
    #ifdef DEBUG_LOG
    for (stringmapentry** p = sm.begin(); p != sm.end(); ++p)
      DEBUGLOG(("CommandProcessor::CmdPluginParams: invalid parameter %s -> %s\n", (*p)->Key.cdata(), (*p)->Value.cdata()));
    #endif
  }
}
