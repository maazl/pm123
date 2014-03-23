/*
 * Copyright 2008-2012 M.Mueller
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
#include "../core/location.h"
#include "../core/job.h"
#include "../engine/plugman.h"
#include "../engine/loadhelper.h"
#include "../gui/dialog.h"
#include "../gui/gui.h"
#include "../eventhandler.h"
#include "../123_util.h"
#include "../pm123.h"
#include "../copyright.h"
#include <fileutil.h>
#include <cpp/xstring.h>
#include <cpp/container/stringmap.h>
#include <debuglog.h>
#include <os2.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>


bool URLTokenizer::Next(const char*& url)
{ if (!Args || !*Args)
    return false;
  if (Args[0] == '"')
  { // quoted item
    url = Args+1;
    Args = strchr(url, '"');
  } else
  { url = Args;
    Args += strcspn(url, " \t");
  }
  if (*Args)
    *Args++ = 0;
  // next argument
  if (Args && *Args)
    Args += strspn(Args, " \t");
  return true;
}


// list must be ordered!!!
const CommandProcessor::CmdEntry CommandProcessor::CmdList[] =
{ { "add",                &CommandProcessor::XPlAdd         }
, { "autouse",            &CommandProcessor::XAutouse       }
, { "cd",                 &CommandProcessor::XCd            }
, { "clear",              &CommandProcessor::XPlClear       }
, { "current",            &CommandProcessor::XCurrent       }
, { "default",            &CommandProcessor::XDefault       }
, { "dir",                &CommandProcessor::XDir           }
, { "enqueue",            &CommandProcessor::XEnqueue       }
, { "float",              &CommandProcessor::XFloat         }
, { "font",               &CommandProcessor::XFont          }
, { "forward",            &CommandProcessor::XForward       }
, { "getmessages",        &CommandProcessor::XGetMessages   }
, { "hide",               &CommandProcessor::XHide          }
, { "info format",        &CommandProcessor::XInfoFormat    }
, { "info invalidate",    &CommandProcessor::XInfoInvalidate}
, { "info meta",          &CommandProcessor::XInfoMeta      }
, { "info pl format",     &CommandProcessor::XPlInfoFormat  }
, { "info pl item",       &CommandProcessor::XPlInfoItem    }
, { "info pl meta",       &CommandProcessor::XPlInfoMeta    }
, { "info pl playlist",   &CommandProcessor::XPlInfoPlaylist}
, { "info playlist",      &CommandProcessor::XInfoPlaylist  }
, { "info refresh",       &CommandProcessor::XInfoRefresh   }
, { "invoke",             &CommandProcessor::XInvoke        }
, { "isvisible",          &CommandProcessor::XIsVisible     }
, { "jump",               &CommandProcessor::XJump          }
, { "load",               &CommandProcessor::XLoad          }
, { "location",           &CommandProcessor::XLocation      }
, { "navigate",           &CommandProcessor::XNavigate      }
, { "next",               &CommandProcessor::XNext          }
, { "open",               &CommandProcessor::XShow          }
, { "option",             &CommandProcessor::XOption        }
, { "pause",              &CommandProcessor::XPause         }
, { "pl add",             &CommandProcessor::XPlAdd         }
, { "pl callstack",       &CommandProcessor::XPlCallstack   }
, { "pl clear",           &CommandProcessor::XPlClear       }
, { "pl current",         &CommandProcessor::XPlCurrent     }
, { "pl depth",           &CommandProcessor::XPlDepth       }
, { "pl dir",             &CommandProcessor::XDir           }
, { "pl enqueue",         &CommandProcessor::XPlEnqueue     }
, { "pl enter",           &CommandProcessor::XPlEnter       }
, { "pl index",           &CommandProcessor::XPlIndex       }
, { "pl info format",     &CommandProcessor::XPlInfoFormat  }
, { "pl info item",       &CommandProcessor::XPlInfoItem    }
, { "pl info meta",       &CommandProcessor::XPlInfoMeta    }
, { "pl info playlist",   &CommandProcessor::XPlInfoPlaylist}
, { "pl item",            &CommandProcessor::XPlItem        }
, { "pl itemindex",       &CommandProcessor::XPlItemIndex   }
, { "pl leave",           &CommandProcessor::XPlLeave       }
, { "pl load",            &CommandProcessor::XPlLoad        }
, { "pl navigate",        &CommandProcessor::XPlNavigate    }
, { "pl navto",           &CommandProcessor::XPlNavTo       }
, { "pl next",            &CommandProcessor::XPlNext        }
, { "pl nextitem",        &CommandProcessor::XPlNextItem    }
, { "pl parent",          &CommandProcessor::XPlParent      }
, { "pl prev",            &CommandProcessor::XPlPrev        }
, { "pl previous",        &CommandProcessor::XPlPrev        }
, { "pl previtem",        &CommandProcessor::XPlPrevItem    }
, { "pl rdir",            &CommandProcessor::XRdir          }
, { "pl remove",          &CommandProcessor::XPlRemove      }
, { "pl reset",           &CommandProcessor::XPlReset       }
, { "pl save",            &CommandProcessor::XPlSave        }
, { "pl sort",            &CommandProcessor::XPlSort        }
, { "pl_add",             &CommandProcessor::XPlAdd         }
, { "pl_clear",           &CommandProcessor::XPlClear       }
, { "pl_current",         &CommandProcessor::XPlCurrent     }
, { "pl_index",           &CommandProcessor::XPlIndex       }
, { "pl_item",            &CommandProcessor::XPlItem        }
, { "pl_next",            &CommandProcessor::XPlNext        }
, { "pl_prev",            &CommandProcessor::XPlPrev        }
, { "pl_previous",        &CommandProcessor::XPlPrev        }
, { "pl_remove",          &CommandProcessor::XPlRemove      }
, { "pl_reset",           &CommandProcessor::XPlReset       }
, { "pl_save",            &CommandProcessor::XPlSave        }
, { "pl_sort",            &CommandProcessor::XPlSort        }
, { "play",               &CommandProcessor::XPlay          }
, { "playlist",           &CommandProcessor::XPlaylist      }
, { "playonload",         &CommandProcessor::XPlayonload    }
, { "plugin list",        &CommandProcessor::XPluginList    }
, { "plugin load",        &CommandProcessor::XPluginLoad    }
, { "plugin params",      &CommandProcessor::XPluginParams  }
, { "plugin unload",      &CommandProcessor::XPluginUnload  }
, { "prev",               &CommandProcessor::XPrev          }
, { "previous",           &CommandProcessor::XPrev          }
, { "query",              &CommandProcessor::XQuery         }
, { "rdir",               &CommandProcessor::XRdir          }
, { "remove",             &CommandProcessor::XPlRemove      }
, { "repeat",             &CommandProcessor::XRepeat        }
, { "reset",              &CommandProcessor::XReset         }
, { "rewind",             &CommandProcessor::XRewind        }
, { "save",               &CommandProcessor::XPlSave        }
, { "savestream",         &CommandProcessor::XSavestream    }
, { "show",               &CommandProcessor::XShow          }
, { "shuffle",            &CommandProcessor::XShuffle       }
, { "size",               &CommandProcessor::XSize          }
, { "skin",               &CommandProcessor::XSkin          }
, { "status",             &CommandProcessor::XStatus        }
, { "stop",               &CommandProcessor::XStop          }
, { "time",               &CommandProcessor::XTime          }
, { "use",                &CommandProcessor::XUse           }
, { "version",            &CommandProcessor::XVersion       }
, { "volume",             &CommandProcessor::XVolume        }
, { "write meta rst",     &CommandProcessor::XWriteMetaRst  }
, { "write meta set",     &CommandProcessor::XWriteMetaSet  }
, { "write meta to",      &CommandProcessor::XWriteMetaTo   }
};

char CommandProcessor::Cmd_QueryDefault[] = ""; // This object is identified by instance
char CommandProcessor::Cmd_SetDefault[] = ""; // This object is identified by instance

CommandProcessor::SyntaxException::SyntaxException(const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  Text.vsprintf(fmt, va);
  va_end(va);
}

CommandProcessor::CommandProcessor()
: CurSI(&GUI::GetDefaultPL())
, Command(NULL)
, vd_message(&CommandProcessor::MessageHandler, this)
, vd_message2(&CommandProcessor::MessageHandler, this)
, MetaFlags(DECODER_HAVE_NONE)
{}

void CommandProcessor::EscapeNEL(xstringbuilder& target, size_t start)
{ while (true)
  { start = target.find_any("\r\n\x1b", start);
    if (start >= target.length())
      break;
    switch(target[start])
    {case '\r':
      target[start] = 'r'; break;
     case '\n':
      target[start] = 'n'; break;
    }
    target.insert(start, '\x1b');
    start += 2;
} }

/*void CommandProcessor::PostMessage(MESSAGE_TYPE type, const char* fmt, ...)
{ Messages.append(type < 2 ? "IWE"[type] : '?');
  Messages.append(' ');
  size_t start = Messages.length();
  va_list va;
  va_start(va, fmt);
  Messages.vappendf(fmt, va);
  va_end(va);
  EscapeNEL(Messages, start);
  Messages.append('\n');
}*/

void DLLENTRY CommandProcessor::MessageHandler(CommandProcessor* that, MESSAGE_TYPE type, const xstring& msg)
{ that->Messages.append(type < 3 ? "IWE"[type] : '?');
  that->Messages.append(' ');
  size_t start = that->Messages.length();
  that->Messages.append(msg);
  EscapeNEL(that->Messages, start);
  that->Messages.append('\n');
}

void CommandProcessor::Exec()
{ DEBUGLOG(("CommandProcessor::Exec() %s\n", Request));
  // redirect error handler
  EventHandler::LocalRedirect ehr(vd_message);

  Request += strspn(Request, " \t"); // skip leading blanks
  // remove trailing blanks
  { char* ape = Request + strlen(Request);
    while (ape != Request && (ape[-1] == ' ' || ape[-1] == '\t'))
      --ape;
    *ape = 0;
  }
  try
  { Request += strspn(Request, " \t"); // skip leading blanks
    // check for plug-in specific commands
    size_t len = strcspn(Request, " \t:");
    if (Request[len] == ':')
    { // Plug-in option
      Request[len] = 0;
      int_ptr<Module> plugin(ParsePlugin(Request));
      if (!plugin)
        return; // Plug-in not found.
      Request += len+1;
      Request += strspn(Request, " \t"); // skip leading blanks
      // Call plug-in function
      xstring result;
      plugin->Command(Request, result);
      Reply.append(result);
    } else
    { // Search command handler ...
      const CmdEntry* cep = mapsearcha(CmdList, Request);
      DEBUGLOG(("CommandProcessor::Exec: %s -> %p(%s)\n", Request, cep, cep));
      if (cep)
      { char* cp;
        // Check for non-alpha
        while (isgraph(*(cp = Request + strlen(cep->Str))))
        { // Try next entry
          if ( ++cep == CmdList + sizeof(CmdList)/sizeof(*CmdList)
            || strabbrevicmp(Request, cep->Str) != 0 )
            goto fail;
        }
        if (*cp)
          *cp++ = 0;
        Command = Request;
        CommandType = false;
        Request = cp + strspn(cp, " \t");
        DEBUGLOG(("CommandProcessor::Exec: %s\n", Request));
        // ... and execute
        (this->*cep->Val)();
        Command = NULL;
      } else
      {fail:
        // Invalid command
        Messages.append("E Syntax error: unknown command \"");
        size_t start = Messages.length();
        Messages.append(Request, strnlen(Request, sizeof(cep->Str)));
        EscapeNEL(Messages, start);
        Messages.append("\"\n");
      }
    }
  } catch (const SyntaxException& ex)
  { Reply.clear();
    if (CommandType)
      Reply.appendd(Ctrl::RC_BadArg);
    Messages.append("E Syntax error: ");
    size_t start = Messages.length();
    Messages.append(ex.Text);
    EscapeNEL(Messages, start);
    Messages.append('\n');
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

/* parse operator with default toggle */
Ctrl::Op CommandProcessor::ParseBool(const char* arg)
{ static const strmap<7,Ctrl::Op> opmap[] =
  { { "0",      Ctrl::Op_Clear },
    { "1",      Ctrl::Op_Set },
    { "false",  Ctrl::Op_Clear },
    { "no",     Ctrl::Op_Clear },
    { "off",    Ctrl::Op_Clear },
    { "on",     Ctrl::Op_Set },
    { "toggle", Ctrl::Op_Toggle },
    { "true",   Ctrl::Op_Set },
    { "yes",    Ctrl::Op_Set }
  };
  const strmap<7,Ctrl::Op>* op = mapsearch(opmap, arg);
  if (!op)
    throw SyntaxException("Expected boolean value {0|1|on|off|true|false|yes|no|toggle} but found \"%s\".", arg);
  return op->Val;
}

int CommandProcessor::ParseInt(const char* arg)
{ int v;
  size_t n;
  if (sscanf(arg, "%i%n", &v, &n) != 1 || n != strlen(arg))
    throw SyntaxException("Argument \"%s\" is not an integer.", arg);
  return v;
}

double CommandProcessor::ParseDouble(const char* arg)
{ double v;
  size_t n;
  if (sscanf(arg, "%lf%n", &v, &n) == 1 && n == strlen(arg))
    return v;
  throw SyntaxException("Argument \"%s\" is not an floating point number.", arg);
}

PM123_TIME CommandProcessor::ParseTime(const char* arg)
{ double v[3];
  size_t n;
  int count = sscanf(arg, "%lf%n:%lf%n:%lf%n", v+0, &n, v+1, &n, v+2, &n);
  if (n == strlen(arg))
    switch (count)
    {case 3:
      return 3600*v[0] + 60*v[1] + v[2];
     case 2:
      return 60*v[0] + v[1];
     case 1:
      return v[0];
    }
  throw SyntaxException("Argument \"%s\" is not an time specification.", arg);
}

static const strmap<5,cfg_disp> dispmap[] =
{ { "file", CFG_DISP_FILENAME }
, { "info", CFG_DISP_FILEINFO }
, { "tag",  CFG_DISP_ID3TAG   }
, { "url",  CFG_DISP_FILENAME }
};
cfg_disp CommandProcessor::ParseDisp(const char* arg)
{ const strmap<5,cfg_disp>* mp = mapsearch(dispmap, arg);
  if (!mp)
    throw SyntaxException("Expected {file|info|tag|url} but found \"%s\".", arg);
  return mp->Val;
}

url123 CommandProcessor::ParseURL(const char* url)
{ url123 ret = CurDir ? CurDir.makeAbsolute(url) : url123::normalizeURL(url);
  if (!ret)
    throw SyntaxException("Argument \"%s\" is no valid URL or file name.", url);
  // Directory?
  if (ret[ret.length()-1] != '/' && is_dir(ret))
    ret = ret + "/";
  return ret;
}

int_ptr<APlayable> CommandProcessor::ParseAPlayable(const char* url)
{ if (*url == 0)
    return Ctrl::GetCurrentSong();
  return Playable::GetByURL(ParseURL(url)).get();
};

int_ptr<Module> CommandProcessor::ParsePlugin(const char* arg)
{ size_t len = strlen(arg);
  if (len <= 4 || stricmp(arg+len-4, ".dll") != 0)
  { char* cp = (char*)alloca(len+4);
    memcpy(cp, arg, len);
    strcpy(cp+len, ".dll");
    arg = cp;
  }
  int_ptr<Module> mp = Module::FindByKey(arg);
  if (!mp)
    MessageHandler(this, MSG_ERROR, xstring().sprintf("Plug-in module \"%s\" not found.", arg));
  return mp;
}

void CommandProcessor::DoOption(bool amp_cfg::* option)
{ Reply.append(ReadCfg().*option ? "on" : "off");
  if (Request == Cmd_SetDefault)
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { Ctrl::Op op = ParseBool(Request);
    Cfg::ChangeAccess cfg;
    switch (op)
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
  }
}
void CommandProcessor::DoOption(int amp_cfg::* option)
{ Reply.appendf("%i", ReadCfg().*option);
  if (Request == Cmd_SetDefault)
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { Cfg::ChangeAccess cfg;
    cfg.*option = ParseInt(Request);
  }
}
void CommandProcessor::DoOption(unsigned amp_cfg::* option)
{ Reply.appendf("%u", ReadCfg().*option);
  if (Request == Cmd_SetDefault)
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { int i = ParseInt(Request);
    if (i < 0)
      throw SyntaxException("Argument \"%s\" must not be negative.", i);
    Cfg::ChangeAccess cfg;
    cfg.*option = i;
  }
}
void CommandProcessor::DoOption(xstring amp_cfg::* option)
{ Reply.append(xstring(ReadCfg().*option));
  if (Request == Cmd_SetDefault)
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { Cfg::ChangeAccess cfg;
    cfg.*option = Request;
  }
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
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { const strmap<10,cfg_anav>* mp = mapsearch(map, Request);
    if (!mp)
      throw SyntaxException("Expected {song|song,time|time} but found \"%s\".", Request);
    Cfg::ChangeAccess cfg;
    cfg.*option = mp->Val;
  }
}
void CommandProcessor::DoOption(cfg_button amp_cfg::* option)
{ static const strmap<6,cfg_button> map[] =
  { { "alt",   CFG_BUT_ALT   }
  , { "ctrl",  CFG_BUT_CTRL  }
  , { "shift", CFG_BUT_SHIFT }
  };
  Reply.append(map[ReadCfg().*option].Str);
  if (Request == Cmd_SetDefault)
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { const strmap<6,cfg_button>* mp = mapsearch(map, Request);
    if (!mp)
      throw SyntaxException("Expected {alt|ctrl|shift} but found \"%s\".", Request);
    Cfg::ChangeAccess cfg;
    cfg.*option = mp->Val;
  }
}
void CommandProcessor::DoOption(cfg_action amp_cfg::* option)
{ static const strmap<10,cfg_action> map[] =
  { { "enqueue",  CFG_ACTION_QUEUE }
  , { "load",     CFG_ACTION_LOAD  }
  , { "navigate", CFG_ACTION_NAVTO }
  };
  Reply.append(map[ReadCfg().*option].Str);
  if (Request == Cmd_SetDefault)
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { const strmap<10,cfg_action>* mp = mapsearch(map, Request);
    if (!mp)
      throw SyntaxException("Expected {enqueue|load|navigate} but found \"%s\".", Request);
    Cfg::ChangeAccess cfg;
    cfg.*option = mp->Val;
  }
}
void CommandProcessor::DoOption(cfg_disp amp_cfg::* option)
{ Reply.append(dispmap[3 - ReadCfg().*option].Str);
  if (Request == Cmd_SetDefault)
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { Cfg::ChangeAccess cfg;
    cfg.*option = ParseDisp(Request);
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
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  }
  else if (Request)
  { const strmap<9,cfg_scroll>* mp = mapsearch(map, Request);
    if (!mp)
      throw SyntaxException("Expected {none|once|infinite} but found \"%s\".", Request);
    Cfg::ChangeAccess cfg;
    cfg.*option = mp->Val;
  }
}
void CommandProcessor::DoOption(cfg_mode amp_cfg::* option)
{ // old value
  Reply.appendd(ReadCfg().*option);
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
  { Cfg::ChangeAccess cfg;
    cfg.*option = Cfg::Default.*option;
  } else if (Request)
  { const strmap<8,cfg_mode>* mp = mapsearch(map, Request);
    if (!mp)
      throw SyntaxException("Expected {0|1|2|normal|regular|small|tiny} but found \"%s\".", Request);
    Cfg::ChangeAccess cfg;
    cfg.*option = mp->Val;
  }
};

void CommandProcessor::DoFontOption(void*)
{ // old value
  { Cfg::Access cfg;
    const amp_cfg& rcfg = Request == Cmd_QueryDefault ? Cfg::Default : *cfg;
    if (rcfg.font_skinned)
      Reply.appendd(rcfg.font+1);
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
      if ((Request[0] != '1' && Request[0] != '2') || Request[1])
        throw SyntaxException("Expected skinned font number {1|2} but found \"%s\".", Request);
      Cfg::ChangeAccess cfg;
      cfg.font_skinned = true;
      cfg.font         = Request[0] & 0xf;
    } else
    { // non skinned font
      FATTRS fattrs = { sizeof(FATTRS), 0, 0, "", 0, 0, 16, 7, 0, 0 };
      unsigned size;
      if (!amp_string_to_font_attrs(fattrs, size, Request))
        throw SyntaxException("Expected font in the format <size>.<fontname>[.bold][.italic] but found \"%s\".", Request);
      Cfg::ChangeAccess cfg;
      cfg.font_skinned = false;
      cfg.font_attrs   = fattrs;
      cfg.font_size    = size;
      // TODO: we should validate the font here.
    }
  }
}

void CommandProcessor::SendCtrlCommand(Ctrl::ControlCommand* cmd)
{ cmd = Ctrl::SendCommand(cmd);
  Reply.appendd(cmd->Flags);
  if (cmd->StrArg)
    MessageHandler(this, cmd->Flags ? MSG_ERROR : MSG_INFO, cmd->StrArg);
  cmd->Destroy();
}

bool CommandProcessor::FillLoadHelper(LoadHelper& lh, char* args)
{ URLTokenizer tok(args);
  const char* url;
  // Tokenize args
  while (tok.Next(url))
  { if (!url)
      return false; // Bad URL
    lh.AddItem(*Playable::GetByURL(ParseURL(url)));
  }
  return true;
}


/****************************************************************************
*
*  remote command handlers
*
****************************************************************************/
void CommandProcessor::XGetMessages()
{ Reply.append(Messages, Messages.length());
  Messages.clear();
}

void CommandProcessor::XCd()
{ // return current value
  if (CurDir)
    Reply.append(CurDir);
  // set new dir
  if (*Request)
    CurDir = ParseURL(Request);
}

void CommandProcessor::XReset()
{ CurSI.SetRoot((APlayable*)&GUI::GetDefaultPL());
  CurDir.reset();
  Messages.clear();
}

// PLAY CONTROL

void CommandProcessor::XLoad()
{ LoadHelper lh(LoadHelper::LoadDefault);
  Reply.appendd(!FillLoadHelper(lh, Request) ? Ctrl::RC_BadArg : lh.SendCommand());
}

void CommandProcessor::XPlay()
{ if (!*Request)
    SendCtrlCommand(Ctrl::MkPlayStop(Ctrl::Op_Set));
  else
  { LoadHelper lh(LoadHelper::LoadPlay);
    Reply.appendd(!FillLoadHelper(lh, Request) ? Ctrl::RC_BadArg : lh.SendCommand());
  }
}

void CommandProcessor::XEnqueue()
{ LoadHelper lh(Cfg::Get().playonload*LoadHelper::LoadPlay | LoadHelper::LoadAppend);
  Reply.appendd(!FillLoadHelper(lh, Request) ? Ctrl::RC_BadArg : lh.SendCommand());
}

void CommandProcessor::XInvoke()
{ const volatile amp_cfg& cfg = Cfg::Get();
  LoadHelper lh(cfg.playonload*LoadHelper::LoadPlay | cfg.keeproot*LoadHelper::LoadKeepItem);
  Reply.appendd(!FillLoadHelper(lh, Request) ? Ctrl::RC_BadArg : lh.SendCommand());
}

void CommandProcessor::XStop()
{ SendCtrlCommand(Ctrl::MkPlayStop(Ctrl::Op_Clear));
}

void CommandProcessor::XPause()
{ CommandType = true;
  Ctrl::Op op = *Request ? ParseBool(Request) : Ctrl::Op_Toggle;
  SendCtrlCommand(Ctrl::MkPause(op));
}

void CommandProcessor::XNext()
{ CommandType = true;
  int count = *Request ? ParseInt(Request) : 1;
  SendCtrlCommand(Ctrl::MkSkip(count, true));
}

void CommandProcessor::XPrev()
{ CommandType = true;
  int count = *Request ? ParseInt(Request) : 1;
  SendCtrlCommand(Ctrl::MkSkip(-count, true));
}

void CommandProcessor::XRewind()
{ CommandType = true;
  Ctrl::Op op = *Request ? ParseBool(Request) : Ctrl::Op_Toggle;
  SendCtrlCommand(Ctrl::MkScan(op|Ctrl::Op_Rewind));
}

void CommandProcessor::XForward()
{ CommandType = true;
  Ctrl::Op op = *Request ? ParseBool(Request) : Ctrl::Op_Toggle;
  SendCtrlCommand(Ctrl::MkScan(op));
}

void CommandProcessor::XJump()
{ CommandType = true;
  PM123_TIME pos = ParseTime(Request);
  SendCtrlCommand(Ctrl::MkNavigate(xstring(), pos, Ctrl::NT_None));
}

void CommandProcessor::XNavigate()
{ SendCtrlCommand(Ctrl::MkNavigate(Request, 0, Ctrl::NT_None));
}

void CommandProcessor::XSavestream()
{ SendCtrlCommand(Ctrl::MkSave(Request));
}

void CommandProcessor::XVolume()
{ Reply.appendf("%f", Ctrl::GetVolume());
  if (*Request)
  { bool   sign = *Request == '+' || *Request == '-';
    double vol  = ParseDouble(Request);
    Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkVolume(vol, sign));
    if (cmd->Flags != 0)
      Reply.clear(); //error
    cmd->Destroy();
  }
}

void CommandProcessor::XShuffle()
{ CommandType = true;
  Ctrl::Op op = *Request ? ParseBool(Request) : Ctrl::Op_Toggle;
  SendCtrlCommand(Ctrl::MkShuffle(op));
}

void CommandProcessor::XRepeat()
{ CommandType = true;
  Ctrl::Op op = *Request ? ParseBool(Request) : Ctrl::Op_Toggle;
  SendCtrlCommand(Ctrl::MkRepeat(op));
}

static bool IsRewind()
{ return Ctrl::GetScan() < 0;
}
static bool IsForward()
{ return Ctrl::GetScan() > 0;
}
void CommandProcessor::XQuery()
{ static const strmap<8, bool (*)()> map[] =
  { { "forward", &IsForward       },
    { "pause",   &Ctrl::IsPaused  },
    { "play",    &Ctrl::IsPlaying },
    { "repeat",  &Ctrl::IsRepeat  },
    { "rewind",  &IsRewind        },
    { "shuffle", &Ctrl::IsShuffle }
  };
  const strmap<8, bool (*)()>* op = mapsearch(map, Request);
  if (!op)
    throw SyntaxException("Expected {forward|pause|play|repeat|rewind|shuffle} but found \"%s\".", Request);
  Reply.appendd((*op->Val)());
}

void CommandProcessor::XCurrent()
{ static const strmap<5,char> map[] =
  { { "",     0 },
    { "root", 1 },
    { "song", 0 }
  };
  const strmap<5,char>* op = mapsearch(map, Request);
  if (!op)
    throw SyntaxException("Expected [root|song] but found \"%s\".", Request);
  int_ptr<APlayable> cur;
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

void CommandProcessor::XStatus()
{ int_ptr<APlayable> song = Ctrl::GetCurrentSong();
  if (song)
  { switch (ParseDisp(Request))
    {case CFG_DISP_ID3TAG:
      Reply.append(GUI::ConstructTagString(*song->GetInfo().meta));
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

void CommandProcessor::XTime()
{ int_ptr<SongIterator> loc;
  Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkLocation(Ctrl::LM_ReturnPos));
  if (cmd->Flags == Ctrl::RC_OK)
  { loc.fromCptr((SongIterator*)cmd->PtrArg);
    Reply.appendf("%f", loc->GetTime());
  }
  cmd->Destroy();
}

void CommandProcessor::XLocation()
{ int_ptr<SongIterator> loc;
  Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkLocation(Ctrl::LM_ReturnPos));
  if (cmd->Flags == Ctrl::RC_OK)
  { loc.fromCptr((SongIterator*)cmd->PtrArg);
    Reply.append(loc->Serialize());
  }
  cmd->Destroy();
}

// PLAYLIST
static const strmap<10,TECH_ATTRIBUTES> stopatmap[] =
{ { "invalid",  TATTR_INVALID }
, { "list",     TATTR_PLAYLIST }
, { "playlist", TATTR_PLAYLIST }
, { "song",     TATTR_SONG }
};
void CommandProcessor::ParsePrevNext(char* arg, NavParams& par)
{ const char* cp = strtok(arg, " \t,");
  if (!cp)
    return;
  par.Count = ParseInt(cp);
  cp = strtok(NULL, " \t,");
  if (!cp)
    return;
  par.StopAt = TATTR_NONE;
  do
  { const strmap<10,TECH_ATTRIBUTES>* mp = mapsearch(stopatmap, cp);
    if (!mp)
      throw SyntaxException("Expected {song|playlist|invalid}[,...] but found \"%s\".", cp);
    par.StopAt |= mp->Val;
    cp = strtok(NULL, " \t,");
  } while (cp);
}

void CommandProcessor::NavigationResult(Location::NavigationResult result)
{ if (result)
    Messages.appendf("E %s\n", result.cdata());
  else
    XPlItem();
}

void CommandProcessor::XPlaylist()
{ int_ptr<Playable> pp = Playable::GetByURL(ParseURL(Request));
  { CurSI.SetRoot((APlayable*)pp);
    Reply.append(pp->URL);
  }
};

void CommandProcessor::XPlNext()
{ NavParams par = { 1, TATTR_SONG };
  ParsePrevNext(Request, par);
  NavigationResult(CurSI.NavigateCount(Job::SyncJob, par.Count, par.StopAt));
}

void CommandProcessor::XPlPrev()
{ NavParams par = { 1, TATTR_SONG };
  ParsePrevNext(Request, par);
  NavigationResult(CurSI.NavigateCount(Job::SyncJob, -par.Count, par.StopAt));
}

void CommandProcessor::XPlNextItem()
{ NavParams par = { 1, TATTR_SONG|TATTR_PLAYLIST|TATTR_INVALID };
  ParsePrevNext(Request, par);
  const size_t depth = CurSI.GetLevel();
  NavigationResult(CurSI.NavigateCount(Job::SyncJob, par.Count, par.StopAt, depth, depth ? depth : 1));
}

void CommandProcessor::XPlPrevItem()
{ NavParams par = { 1, TATTR_SONG|TATTR_PLAYLIST|TATTR_INVALID };
  ParsePrevNext(Request, par);
  const size_t depth = CurSI.GetLevel();
  NavigationResult(CurSI.NavigateCount(Job::SyncJob, -par.Count, par.StopAt, depth, depth ? depth : 1));
}

void CommandProcessor::XPlEnter()
{ XPlItem();
  Location::NavigationResult ret = CurSI.NavigateInto(Job::SyncJob);
  if (ret)
  { Messages.appendf("E %s\n", ret.cdata());
    Reply.clear();
  }
}

void CommandProcessor::XPlLeave()
{ int count = *Request ? ParseInt(Request) : 1;
  NavigationResult(CurSI.NavigateUp(count));
}

void CommandProcessor::XPlNavigate()
{ const char* cp = Request;
  const Location::NavigationResult& result = CurSI.Deserialize(Job::SyncJob, cp);
  if (result)
    Messages.appendf("E %s at %.30s\n", result.cdata(), cp);
  else
    Reply.append(CurSI.Serialize(true));
}

void CommandProcessor::XPlReset()
{ XPlItem();
  CurSI.Reset();
}

void CommandProcessor::XPlCurrent()
{ APlayable* root = CurSI.GetRoot();
  if (root)
    Reply.append(root->GetPlayable().URL);
}

void CommandProcessor::XPlParent()
{ APlayable* parent;
  switch (CurSI.GetLevel())
  {case 0:
    return;
   case 1:
    parent = CurSI.GetRoot();
    break;
   default: // >= 2
    parent = CurSI.GetCallstack()[CurSI.GetLevel()-2];
  }
  Reply.append(parent->GetPlayable().URL);
}

void CommandProcessor::XPlItem()
{ APlayable* cur = CurSI.GetCurrent();
  if (cur)
    Reply.append(cur->GetPlayable().URL);
}

void CommandProcessor::XPlDepth()
{ if (CurSI.GetRoot())
    Reply.appendd(CurSI.GetCallstack().size());
}

void CommandProcessor::XPlCallstack()
{ Reply.append(CurSI.Serialize(false));
}

void CommandProcessor::XPlIndex()
{ if (CurSI.GetRoot())
  { AggregateInfo ai(PlayableSetBase::Empty);
    CurSI.AddFrontAggregate(ai, IF_Rpl, Job::SyncJob);
    Reply.appendd(ai.Rpl.songs + 1);
  }
}

void CommandProcessor::XPlItemIndex()
{ const vector<PlayableInstance>& cs = CurSI.GetCallstack();
  if (cs.size())
  { PlayableInstance* pi = cs[cs.size()-1];
    if (pi)
      Reply.appendd(pi->GetIndex());
  }
}

void CommandProcessor::XUse()
{ APlayable* root = CurSI.GetRoot();
  if (root)
  { LoadHelper lh(Cfg::Get().playonload*LoadHelper::LoadPlay | Cfg::Get().append_cmd*LoadHelper::LoadAppend);
    lh.AddItem(*root);
    Reply.appendd(lh.SendCommand());
  } else
    Reply.appendd(Ctrl::RC_NoList);
};

void CommandProcessor::XPlLoad()
{ APlayable* cur = CurSI.GetCurrent();
  if (cur)
  { LoadHelper lh(LoadHelper::LoadDefault);
    lh.AddItem(*cur);
    Reply.appendd(lh.SendCommand());
  } else
    Reply.appendd(Ctrl::RC_NoSong);
}

void CommandProcessor::XPlEnqueue()
{ APlayable* cur = CurSI.GetCurrent();
  if (cur)
  { LoadHelper lh(LoadHelper::LoadAppend);
    lh.AddItem(*cur);
    Reply.appendd(lh.SendCommand());
  } else
    Reply.appendd(Ctrl::RC_NoSong);
}

void CommandProcessor::XPlNavTo()
{ if (CurSI.GetRoot())
    SendCtrlCommand(Ctrl::MkJump(new Location(CurSI), true));
  else
    Reply.appendd(Ctrl::RC_NoSong);
}

// Playlist modification

Playable* CommandProcessor::PrepareEditPlaylist()
{ if (CurSI.GetLevel() > 1)
    Messages.append("E Cannot modify nested playlist.\n");
  else
  { APlayable* root = CurSI.GetRoot();
    if (!root)
      Messages.append("E No current playlist.\n");
    else
    { root->RequestInfo(IF_Obj|IF_Tech|IF_Child, PRI_Sync|PRI_Normal);
      if (root->GetInfo().tech->attributes & TATTR_SONG)
        Messages.append("E Cannot edit a song as playlist.\n");
      else
        return &root->GetPlayable();
    }
  }
  return NULL; // Error
}

inline PlayableInstance* CommandProcessor::CurrentPlaylistItem()
{ return CurSI.GetCallstack().size() ? CurSI.GetCallstack()[0] : NULL;
}

void CommandProcessor::XPlClear()
{ Playable* root = PrepareEditPlaylist();
  if (root)
  { CurSI.Reset();
    Reply.appendd(root->GetInfo().obj->num_items);
    if (!root->Clear())
      Reply.clear();
  }
}

void CommandProcessor::XPlAdd()
{ Playable* root = PrepareEditPlaylist();
  if (root)
  { PlayableInstance* cur = CurrentPlaylistItem();
    URLTokenizer tok(Request);
    const char* url;
    int count = 0;
    Mutex::Lock lck(root->Mtx); // Atomic
    while (tok.Next(url))
    { if (!url)
      { Reply.append(tok.Current());
        break; // Bad URL
      }
      root->InsertItem(*Playable::GetByURL(ParseURL(url)), cur);
      ++count;
    }
    Reply.appendd(count);
  }
}

void CommandProcessor::XPlRemove()
{ Playable* root = PrepareEditPlaylist();
  if (root)
  { PlayableInstance* cur = CurrentPlaylistItem();
    if (!cur)
      Messages.append("E No current item to remove.\n");
    else
    { int_ptr<PlayableInstance> next = root->RemoveItem(*cur);
      if (next != cur)
      { Reply.append(cur->GetPlayable().URL);
        CurSI.NavigateTo(next);
      }
    }
  }
}

void CommandProcessor::PlDir(bool recursive)
{ Playable* root = PrepareEditPlaylist();
  if (root)
  { // collect the content
    URLTokenizer tok(Request);
    const char* curl;
    vector_int<APlayable> list;
    while (tok.Next(curl))
    { url123 url = ParseURL(curl);
      if (strchr(url, '?'))
      { Reply.append(tok.Current());
        break; // Bad URL
      }
      url = amp_make_dir_url(url, recursive);
      int_ptr<Playable> dir = Playable::GetByURL(url);
      // Request simple RPL info to force all recursive playlists to load.
      dir->RequestInfo(IF_Rpl, PRI_Sync|PRI_Normal);
      { // get all songs
        class Location iter(dir);
        while (iter.NavigateCount(Job::SyncJob, 1, TATTR_SONG) == NULL)
        { APlayable* cur = iter.GetCurrent();
          if (&cur->GetPlayable() != root) // Do not create recursion
            list.append() = cur;
        }
      }
    }
    // do the insert
    { PlayableInstance* cur = CurrentPlaylistItem();
      Mutex::Lock lck(root->Mtx); // Atomic
      foreach (const int_ptr<APlayable>,*, app, list)
        root->InsertItem(**app, cur);
    }
    Reply.appendd(list.size());
  }
}

void CommandProcessor::XDir()
{ PlDir(false);
}

void CommandProcessor::XRdir()
{ PlDir(true);
}

void CommandProcessor::XPlSort()
{
  // TODO:
}

void CommandProcessor::XPlSave()
{ int rc = Ctrl::RC_NoList;
  Playable* root = PrepareEditPlaylist();
  if (root)
  { const char* savename = Request;
    if (!*savename)
    { /*if ((CurPlaylist->GetFlags() & Playable::Mutable) != Playable::Mutable)
      { rc = Ctrl::RC_InvalidItem;
        goto end;
      }*/
      savename = root->URL;
    }
    /* TODO !!!
    rc = CurPlaylist->Save(savename, cfg.save_relative*PlayableCollection::SaveRelativePath)
      ? Ctrl::RC_OK : -1;*/
  }
 end:
  Reply.appendd(rc);
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
    { size_t start = Reply.length();
      Reply.append(value);
      EscapeNEL(Reply, start);
    }
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

void CommandProcessor::DoFormatInfo(APlayable& item)
{ item.RequestInfo(IF_Phys|IF_Tech|IF_Obj, PRI_Sync|PRI_Normal);
  const INFO_BUNDLE_CV& info = item.GetInfo();
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

void CommandProcessor::DoMetaInfo(APlayable& item)
{ item.RequestInfo(IF_Meta, PRI_Sync|PRI_Normal);
  const volatile META_INFO& meta = *item.GetInfo().meta;
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

void CommandProcessor::DoPlaylistInfo(APlayable& item)
{ item.RequestInfo(IF_Rpl|IF_Drpl, PRI_Sync|PRI_Normal);
  { const volatile RPL_INFO& rpl = *item.GetInfo().rpl;
    AppendIntAttribute("SONGS=%u\n", rpl.songs);
    AppendIntAttribute("LISTS=%u\n", rpl.lists);
    AppendIntAttribute("INVALID=%u\n", rpl.invalid);
  }
  { const volatile DRPL_INFO& drpl = *item.GetInfo().drpl;
    AppendFloatAttribute("TOTALLENGTH=%.6f\n", drpl.totallength);
    AppendFloatAttribute("TOTALSIZE=%.0f\n", drpl.totalsize);
  }
}

void CommandProcessor::XInfoFormat()
{ // get song object
  int_ptr<APlayable> song(ParseAPlayable(Request));
  if (song)
    DoFormatInfo(*song);
}

void CommandProcessor::XInfoMeta()
{ // get the song object
  int_ptr<APlayable> song(ParseAPlayable(Request));
  if (song)
    DoMetaInfo(*song);
}

void CommandProcessor::XInfoPlaylist()
{ int_ptr<APlayable> list;
  if (*Request == 0)
    list = CurSI.GetRoot();
  else
    list = Playable::GetByURL(ParseURL(Request));
  if (list)
    DoPlaylistInfo(*list);
}

void CommandProcessor::XPlInfoItem()
{ APlayable* cur = CurSI.GetCurrent();
  if (cur == NULL)
    return;
  cur->RequestInfo(IF_Item|IF_Attr|IF_Slice, PRI_Sync|PRI_Normal);
  { const volatile ITEM_INFO& item = *cur->GetInfo().item;
    AppendStringAttribute("ALIAS", item.alias);
    int_ptr<class Location> loc = cur->GetStartLoc();
    AppendStringAttribute("START", loc ? loc->Serialize() : xstring());
    loc = cur->GetStopLoc();
    AppendStringAttribute("STOP", loc ? loc->Serialize() : xstring());
    AppendFloatAttribute("PREGAP=%.6f\n", item.pregap);
    AppendFloatAttribute("POSTGAP=%.6f\n", item.postgap);
    AppendFloatAttribute("GAIN=%.1f\n", item.gain);
  }
  { const volatile ATTR_INFO& attr = *cur->GetInfo().attr;
    AppendFlagsAttribute2("OPTIONS", attr.ploptions, PlOptionsMap);
    AppendStringAttribute("AT", attr.at);
  }
}

void CommandProcessor::XPlInfoFormat()
{ APlayable* cur = CurSI.GetCurrent();
  if (cur)
    DoFormatInfo(*cur);
}

void CommandProcessor::XPlInfoMeta()
{ APlayable* cur = CurSI.GetCurrent();
  if (cur)
    DoMetaInfo(*cur);
}

void CommandProcessor::XPlInfoPlaylist()
{ APlayable* cur = CurSI.GetCurrent();
  if (cur)
    DoPlaylistInfo(*cur);
}

void CommandProcessor::XInfoRefresh()
{ // get the song object
  int_ptr<APlayable> song(ParseAPlayable(Request));
  if (song)
    song->RequestInfo(~IF_None, PRI_Sync|PRI_Normal, REL_Reload);
}

void CommandProcessor::XInfoInvalidate()
{ // get the song object
  int_ptr<APlayable> song(ParseAPlayable(Request));
  if (song)
    song->Invalidate(~IF_None);
}

struct meta_val
{ xstring META_INFO::* Field;
  DECODERMETA          Flag;
};
void CommandProcessor::XWriteMetaSet()
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
    DEBUGLOG(("CommandProcessor::WriteMetaSet: %s = %s\n", op->Str, Request));
  }
}

void CommandProcessor::XWriteMetaTo()
{ if (*Request == 0)
    return;
  const url123& url = ParseURL(Request);
  int_ptr<Playable> song = Playable::GetByURL(url);
  // Write meta
  song->RequestInfo(IF_Tech, PRI_Sync|PRI_Normal);
  Reply.appendf("%i", song->SaveMetaInfo(Meta, MetaFlags));
}

void CommandProcessor::XWriteMetaRst()
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

static const strmap<12,int> windowmap[] =
{ { "",           -2                },
  { "detailed",   GUI::DLT_PLAYLIST },
  { "metainfo",   GUI::DLT_METAINFO },
  { "playlist",   GUI::DLT_PLAYLIST },
  { "properties", -1                },
  { "tagedit",    GUI::DLT_INFOEDIT },
  { "techinfo",   GUI::DLT_TECHINFO },
  { "tree",       GUI::DLT_PLAYLISTTREE }
};

void CommandProcessor::ShowHideCore(DialogBase::DialogAction action)
{ char* arg2 = Request + strcspn(Request, " \t");
  if (*arg2)
  { *arg2++ = 0;
    arg2 += strspn(arg2, " \t");
  }
  const strmap<12,int>* op = mapsearch(windowmap, Request);
  if (!op)
  { Messages.appendf("E Invalid window type %s.\n", Request);
    return;
  }
  bool rc;
  switch (op->Val)
  {case -2: // Player window
    rc = GUI::Show(action);
    break;
   case -1: // properties
    if (*arg2)
    { int_ptr<Module> mod = ParsePlugin(arg2);
      if (mod)
        rc = GUI::ShowConfig(*mod, action);
    } else
      rc = GUI::ShowConfig(action);
    break;
   default:
    if (*arg2 == 0)
    { Messages.append("E missing URL parameter.");
      return;
    }
    int_ptr<Playable> pp = Playable::GetByURL(ParseURL(arg2));
    rc = GUI::ShowDialog(*pp, (GUI::DialogType)op->Val, action);
  }
  Reply.append((char)('0' + rc));
}

void CommandProcessor::XShow()
{ ShowHideCore(DialogBase::DLA_SHOW);
}

void CommandProcessor::XHide()
{ ShowHideCore(DialogBase::DLA_CLOSE);
}

void CommandProcessor::XIsVisible()
{ ShowHideCore(DialogBase::DLA_QUERY);
}

void CommandProcessor::XQuit()
{ GUI::Quit();
  Reply.append('1');
}

void CommandProcessor::XSkin()
{ Reply.append(xstring(Cfg::Get().defskin));
  if (*Request)
    Cfg::ChangeAccess().defskin = Request;
}

// CONFIGURATION

void CommandProcessor::XVersion()
{ Reply.append(AMP_VERSION);
}

// PM123 option
const strmap<16,CommandProcessor::Option> CommandProcessor::OptionMap[] =
{ { "altslider",       &amp_cfg::altnavig       }
, { "altbutton",       &amp_cfg::altbutton      }
, { "autorestart",     &amp_cfg::restartonstart }
, { "autosave",        &amp_cfg::autosave       }
, { "autouse",         &amp_cfg::autouse        }
, { "bufferlevel",     &amp_cfg::buff_fill      }
, { "buffersize",      &amp_cfg::buff_size      }
, { "bufferwait",      &amp_cfg::buff_wait      }
, { "conntimeout",     &amp_cfg::conn_timeout   }
, { "discardseed",     &amp_cfg::discardseed    }
, { "dndrecurse",      &amp_cfg::recurse_dnd    }
, { "dockmargin",      &amp_cfg::dock_margin    }
, { "dockwindows",     &amp_cfg::dock_windows   }
, { "floatontop",      &amp_cfg::floatontop     }
, { "foldersfirst",    &amp_cfg::folders_first  }
, { "font",            &CommandProcessor::DoFontOption }
, { "keeproot",        &amp_cfg::keeproot       }
, { "pipe",            &amp_cfg::pipe_name      }
, { "playlistwrap",    &amp_cfg::autoturnaround }
, { "playonload",      &amp_cfg::playonload     }
, { "plitemaction",    &amp_cfg::itemaction     }
, { "proxyserver",     &amp_cfg::proxy          }
, { "proxyauth",       &amp_cfg::auth           }
, { "queueatcommand",  &amp_cfg::append_cmd     }
, { "queueatdnd",      &amp_cfg::append_dnd     }
, { "queuedelplayed",  &amp_cfg::queue_mode     }
, { "restrictlength",  &amp_cfg::restrict_length}
, { "restrictmeta",    &amp_cfg::restrict_meta  }
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

void CommandProcessor::XOption()
{ char* cp = strchr(Request, '=');
  if (cp)
    *cp++ = 0;

  const strmap<16,Option>* op = mapsearch(OptionMap, Request);
  if (op)
  { Request = cp;
    op->Val(*this);
  }
}

void CommandProcessor::XDefault()
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

void CommandProcessor::XSize()
{ if (!*Request)
    Request = NULL;
  DoOption(&amp_cfg::mode);
};

void CommandProcessor::XFont()
{ if (!*Request)
    Request = NULL;
  DoFontOption(NULL);
}

void CommandProcessor::XFloat()
{ if (!*Request)
    Request = NULL;
  DoOption(&amp_cfg::floatontop);
}

void CommandProcessor::XAutouse()
{ if (!*Request)
    Request = NULL;
  DoOption(&amp_cfg::autouse);
}

void CommandProcessor::XPlayonload()
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

PLUGIN_TYPE CommandProcessor::ParseTypeList()
{ PLUGIN_TYPE type = PLUGIN_NULL;
  for (const char* cp = strtok(Request, ",|"); cp; cp = strtok(NULL, ",|"))
  { const strmap<8,PLUGIN_TYPE>* op = mapsearch(PluginTypeMap, cp);
    if (op == NULL)
      throw SyntaxException("Unknown plug-in type %s.", cp);
    type |= op->Val;
  }
  return type;
}

PLUGIN_TYPE CommandProcessor::ParseType()
{ const strmap<8,PLUGIN_TYPE>* op = mapsearch2(PluginTypeMap+1, sizeof PluginTypeMap/sizeof *PluginTypeMap -1, Request);
  if (op == NULL)
    throw SyntaxException("Unknown plug-in type %s.", Request);
  return op->Val;
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

PLUGIN_TYPE CommandProcessor::InstantiatePlugin(Module& module, const char* params, PLUGIN_TYPE type)
{ if (type)
    try
    { int_ptr<Plugin> pp = Plugin::GetInstance(module, type);
      if (params)
      { stringmap_own sm(20);
        url123::parseParameter(sm, params);
        pp->SetParams(sm);
        for (stringmapentry** p = sm.begin(); p != sm.end(); ++p)
          MessageHandler(this, MSG_WARNING, xstring().sprintf("Ignored unknown plug-in parameter %s", (*p)->Key.cdata()));
      }
      Plugin::AppendPlugin(pp);
      return type;
    } catch (const ModuleException& ex)
    { MessageHandler(this, MSG_ERROR, ex.GetErrorText().cdata());
    }
  return PLUGIN_NULL;
}

PLUGIN_TYPE CommandProcessor::LoadPlugin(PLUGIN_TYPE type)
{ DEBUGLOG(("CommandProcessor(%p)::LoadPlugin(%x) - %s\n", this, type, Request));

  const char* params = strchr(Request, '?');
  if (params)
  { Request[params-Request] = 0;
    ++params;
  }
  // append .dll?
  size_t len = strlen(Request);
  if (len <= 4 || stricmp(Request+len-4, ".dll") != 0)
  { char* cp = (char*)alloca(len+4);
    memcpy(cp, Request, len);
    strcpy(cp+len, ".dll");
    Request = cp;
  }

  try
  { int_ptr<Module> pm = Module::GetByKey(Request);
    if (type == (PLUGIN_DECODER|PLUGIN_FILTER|PLUGIN_OUTPUT|PLUGIN_VISUAL))
      type &= (PLUGIN_TYPE)pm->GetParams().type;

    type = InstantiatePlugin(*pm, params, type & PLUGIN_DECODER)
         | InstantiatePlugin(*pm, params, type & PLUGIN_FILTER)
         | InstantiatePlugin(*pm, params, type & PLUGIN_OUTPUT)
         | InstantiatePlugin(*pm, params, type & PLUGIN_VISUAL);

    ReplyType(type);
  } catch (const ModuleException& ex)
  { MessageHandler(this, MSG_ERROR, ex.GetErrorText().cdata());
  }
  return type;
}

void CommandProcessor::XPluginLoad()
{
  size_t len = strcspn(Request, " \t");
  if (Request[len] == 0)
    throw SyntaxException("Plug-in name missing.");
  Request[len] = 0;

  PLUGIN_TYPE type = ParseTypeList();

  Request += len+1;
  Request += strspn(Request, " \t");
  // TODO: AppendPluginType(Plugin::Deserialize(Request, op->Val));

  // Continue in thread 1
  WinSendMsg(ServiceHwnd, UM_LOAD_PLUGIN, MPFROMP(this), MPFROMLONG(type));
}

static PLUGIN_TYPE DoUnload(Module& module, PLUGIN_TYPE type)
{ if (type)
  { Mutex::Lock lock(Module::Mtx);
    int_ptr<PluginList> list(new PluginList(*Plugin::GetPluginList(type)));
    const int_ptr<Plugin>* ppp = list->begin();
    while (ppp != list->end())
      if ((*ppp)->ModRef == &module)
      { list->erase(ppp);
        Plugin::SetPluginList(list);
        return type;
      } else
        ++ppp;
  }
  return PLUGIN_NULL;
}

PLUGIN_TYPE CommandProcessor::UnloadPlugin(PLUGIN_TYPE type)
{ DEBUGLOG(("CommandProcessor(%p)::UnloadPlugin(%x)\n", this, type));

  int_ptr<Module> module(ParsePlugin(Request));
  if (module == NULL)
    return PLUGIN_NULL;

  type = DoUnload(*module, type & PLUGIN_DECODER)
       | DoUnload(*module, type & PLUGIN_FILTER)
       | DoUnload(*module, type & PLUGIN_OUTPUT)
       | DoUnload(*module, type & PLUGIN_VISUAL);

  ReplyType(type);
  return type;
}

void CommandProcessor::XPluginUnload()
{ size_t len = strcspn(Request, " \t");
  if (Request[len] == 0)
    throw SyntaxException("Plug-in name missing.");
  Request[len] = 0;

  PLUGIN_TYPE type = ParseTypeList();
  Request += len+1;
  Request += strspn(Request, " \t");

  // Continue in thread 1
  WinSendMsg(ServiceHwnd, UM_UNLOAD_PLUGIN, MPFROMP(this), MPFROMLONG(type));
}

bool CommandProcessor::ReplacePluginList(PLUGIN_TYPE type)
{ DEBUGLOG(("CommandProcessor(%p)::ReplacePluginList(%x) - %s\n", this, type, Request));
  xstring err;
  int_ptr<PluginList> list(new PluginList(*Plugin::GetPluginList(type)));
  // set plug-in list
  if (stricmp(Request, "@default") == 0)
    err = list->LoadDefaults();
  else if (stricmp(Request, "@empty") == 0)
    list->clear();
  else
  { // Replace \t by \n
    char* cp = Request;
    while ((cp = strchr(cp, '\t')) != NULL)
    { *cp = '\n';
      while (*++cp == '\t');
    }
    err = list->Deserialize(Request);
  }
  if (err)
  { MessageHandler(this, MSG_ERROR, err);
    Reply.clear();
    return false;
  }

  Plugin::SetPluginList(list);
  return true;
}

void CommandProcessor::XPluginList()
{ size_t len = strcspn(Request, " \t");
  char* np = Request + len;
  if (*np)
  { *np++ = 0;
    np += strspn(np, " \t");
  }
  // requested plug-in type
  PLUGIN_TYPE type = ParseType();
  // Current state
  Reply.append(Plugin::GetPluginList(type)->Serialize());
  if (*np)
  { Request = np;
    // Continue in thread 1
    WinSendMsg(ServiceHwnd, UM_LOAD_PLUGIN_LIST, MPFROMP(this), MPFROMLONG(type));
  }
}

void CommandProcessor::XPluginParams()
{ size_t len = strcspn(Request, " \t");
  if (!Request[len])
    return; // missing plug-in name
  Request[len] = 0;
  // requested plug-in type
  PLUGIN_TYPE type = ParseType();
  Request += len+1;
  Request += strspn(Request, " \t");

  char* params = strchr(Request, '?');
  if (params)
    *params++ = 0;
  // find plug-in
  int_ptr<Module> pm = ParsePlugin(Request);
  if (!pm)
    return;
  int_ptr<Plugin> pp = Plugin::FindInstance(*pm, type);
  if (!pp)
  { MessageHandler(this, MSG_ERROR, xstring().sprintf("Plug-in module \"%s\" is not initialized as the requested type.", Request));
    return;
  }
  // return current parameters
  pp->Serialize(Reply);
  // Set new parameters
  if (params)
  { stringmap_own sm(20);
    url123::parseParameter(sm, params);
    try
    { pp->SetParams(sm);
    } catch (const ModuleException& ex)
    { MessageHandler(this, MSG_ERROR, ex.GetErrorText());
      Reply.clear();
    }
    #ifdef DEBUG_LOG
    for (stringmapentry** p = sm.begin(); p != sm.end(); ++p)
      DEBUGLOG(("CommandProcessor::PluginParams: invalid parameter %s -> %s\n", (*p)->Key.cdata(), (*p)->Value.cdata()));
    #endif
  }
}


HWND CommandProcessor::ServiceHwnd = NULLHANDLE;

MRESULT EXPENTRY ServiceWinFn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ switch (msg)
  {case CommandProcessor::UM_LOAD_PLUGIN:
    { CommandProcessor* cmd = (CommandProcessor*)PVOIDFROMMP(mp1);
      EventHandler::LocalRedirect red(cmd->vd_message2);
      return MRFROMLONG(cmd->LoadPlugin((PLUGIN_TYPE)LONGFROMMP(mp2)));
    }
   case CommandProcessor::UM_UNLOAD_PLUGIN:
    { CommandProcessor* cmd = (CommandProcessor*)PVOIDFROMMP(mp1);
      EventHandler::LocalRedirect red(cmd->vd_message2);
      return MRFROMLONG(cmd->UnloadPlugin((PLUGIN_TYPE)LONGFROMMP(mp2)));
    }
   case CommandProcessor::UM_LOAD_PLUGIN_LIST:
    { CommandProcessor* cmd = (CommandProcessor*)PVOIDFROMMP(mp1);
      EventHandler::LocalRedirect red(cmd->vd_message2);
      return MRFROMLONG(cmd->ReplacePluginList((PLUGIN_TYPE)LONGFROMMP(mp2)));
    }
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

void CommandProcessor::CreateServiceWindow()
{ PMRASSERT(WinRegisterClass(amp_player_hab, "PM123_CommandProcessor", &ServiceWinFn, 0, 0));
  ServiceHwnd = WinCreateWindow(HWND_OBJECT, "PM123_CommandProcessor", NULL, 0, 0,0, 0,0, NULLHANDLE, HWND_BOTTOM, 42, NULL, NULL);
  PMASSERT(ServiceHwnd != NULLHANDLE);
}
