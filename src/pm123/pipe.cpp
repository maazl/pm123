/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2008-2010 M.Mueller
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
#include "pipe.h"
#include "dialog.h"
#include "gui.h"
#include "loadhelper.h"
#include "properties.h"
#include "controller.h"
#include "123_util.h"
#include "plugman.h"
#include <fileutil.h>
#include <cpp/xstring.h>
#include <cpp/container/stringmap.h>
#include <debuglog.h>
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>


/****************************************************************************
*
*  Private implementation of ACommandProcessor
*
****************************************************************************/
class CommandProcessor : public ACommandProcessor
{private:
  typedef strmap<12, void (CommandProcessor::*)(xstring& ret, char* args)> CmdEntry;
  static const CmdEntry       CmdList[];

 private:
  static const xstring        RetBadArg;   // Return value for Ctrl::RC_BadArg
  int_ptr<Playable>           CurPlaylist; // playlist where we currently operate
  int_ptr<PlayableInstance>   CurItem;     // current item of the above playlist
 private:
  // PLAYBACK
  // Excecute a controller command and return the reply as string.
  const xstring SendCtrlCommand(Ctrl::ControlCommand* cmd);
  // Add a set of URLs to a loadhelper object.
  bool FillLoadHelper(LoadHelper& lh, char* args);
  void CmdLoad(xstring& ret, char* args);
  void CmdPlay(xstring& ret, char* args);
  void CmdStop(xstring& ret, char* args);
  void CmdPause(xstring& ret, char* args);
  void CmdNext(xstring& ret, char* args);
  void CmdPrev(xstring& ret, char* args);
  void CmdRewind(xstring& ret, char* args);
  void CmdForward(xstring& ret, char* args);
  void CmdJump(xstring& ret, char* args);
  void CmdSavestream(xstring& ret, char* args);
  void CmdVolume(xstring& ret, char* args);
  void CmdShuffle(xstring& ret, char* args);
  void CmdRepeat(xstring& ret, char* args);
  void CmdQuery(xstring& ret, char* args);
  void CmdCurrent(xstring& ret, char* args);
  void CmdStatus(xstring& ret, char* args);
  void CmdLocation(xstring& ret, char* args);
  // PLAYLIST
  // Move forward/backward in the current playlist
  void PlSkip(int count);
  void CmdPlaylist(xstring& ret, char* args);
  void CmdPlNext(xstring& ret, char* args);
  void CmdPlPrev(xstring& ret, char* args);
  void CmdPlReset(xstring& ret, char* args);
  void CmdPlCurrent(xstring& ret, char* args);
  void CmdPlItem(xstring& ret, char* args);
  void CmdPlIndex(xstring& ret, char* args);
  void CmdUse(xstring& ret, char* args);
  void CmdClear(xstring& ret, char* args);
  void CmdRemove(xstring& ret, char* args);
  void CmdAdd(xstring& ret, char* args);
  void CmdDir(xstring& ret, char* args);
  void CmdRdir(xstring& ret, char* args);
  void CmdSave(xstring& ret, char* args);
  // GUI
  void CmdShow(xstring& ret, char* args);
  void CmdHide(xstring& ret, char* args);
  void CmdQuit(xstring& ret, char* args);
  void CmdOpen(xstring& ret, char* args);
  void CmdSkin(xstring& ret, char* args);
  // CONFIGURATION
  void CmdSize(xstring& ret, char* args);
  void CmdFont(xstring& ret, char* args);
  void CmdFloat(xstring& ret, char* args);
  void CmdAutouse(xstring& ret, char* args);
  void CmdPlayonload(xstring& ret, char* args);
 public:
  CommandProcessor();
  // Executes the Command cmd and return a value in ret.
  // Note that cmd is mutable. The buffer content will be destroyed.
  virtual void Execute(xstring& ret, char* cmd);
};


void ACommandProcessor::Execute(xstring& ret, const char* cmd)
{ char* buffer = strdup(cmd);
  Execute(ret, buffer);
  free(buffer);
}

ACommandProcessor* ACommandProcessor::Create()
{ return new CommandProcessor();
}


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
  bool                      Next(url123& url);
  char*                     Current() { return Args; }
};

bool URLTokenizer::Next(url123& url)
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
  url = url123::normalizeURL(Args);
  // next argument
  if (narg && *narg)
    Args = narg + strspn(Args, " \t");
  // return normalized URL
  if (url && url[url.length()-1] != '/' && is_dir(url))
    url = url + "/";
  return true;
}


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

static const strmap<8, Ctrl::Op> opmap[] =
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
inline static const strmap<8, Ctrl::Op>* parse_op1(const char* arg)
{ return mapsearch(opmap, arg);
}
/* parse operator without default */
inline static const strmap<8, Ctrl::Op>* parse_op2(const char* arg)
{ return mapsearch2(opmap+1, (sizeof opmap / sizeof *opmap)-1, arg);
}

static void cmd_op_bool(xstring& ret, const char* arg, bool& val)
{ ret = val ? "on" : "off";
  if (*arg)
  { const strmap<8, Ctrl::Op>* op = parse_op2(arg);
    if (op)
    { switch (op->Val)
      {case Ctrl::Op_Set:
        val = true;
        break;
       case Ctrl::Op_Toggle:
        val = !val;
        break;
       default:
        val = false;
        break;
      }
    } else
      ret = xstring::empty; // error
  }
}


const xstring CommandProcessor::RetBadArg(xstring::sprintf("%i", Ctrl::RC_BadArg));


///// PLAY CONTROL

const xstring CommandProcessor::SendCtrlCommand(Ctrl::ControlCommand* cmd)
{ cmd = Ctrl::SendCommand(cmd);
  xstring ret = xstring::sprintf("%i", cmd->Flags);
  cmd->Destroy();
  return ret;
}

bool CommandProcessor::FillLoadHelper(LoadHelper& lh, char* args)
{ URLTokenizer tok(args);
  url123 url;
  // Tokenize args
  while (tok.Next(url))
  { if (!url)
      return false; // Bad URL
    lh.AddItem(Playable::GetByURL(url));
  }
  return true;
}

void CommandProcessor::CmdLoad(xstring& ret, char* args)
{ LoadHelper lh(cfg.playonload*LoadHelper::LoadPlay | cfg.append_cmd*LoadHelper::LoadAppend);
  if (!FillLoadHelper(lh, args))
  { ret = RetBadArg;
    return;
  }
  ret = xstring::sprintf("%i", lh.SendCommand());
}

void CommandProcessor::CmdPlay(xstring& ret, char* args)
{ ExtLoadHelper lh( cfg.playonload*LoadHelper::LoadPlay | cfg.append_cmd*LoadHelper::LoadAppend,
                    Ctrl::MkPlayStop(Ctrl::Op_Set) );
  if (!FillLoadHelper(lh, args))
  { ret = RetBadArg;
    return;
  }
  ret = xstring::sprintf("%i", lh.SendCommand());
}

void CommandProcessor::CmdStop(xstring& ret, char* args)
{ ret = SendCtrlCommand(Ctrl::MkPlayStop(Ctrl::Op_Clear));
}

void CommandProcessor::CmdPause(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op1(args);
  ret = op
    ? (const xstring&)SendCtrlCommand(Ctrl::MkPause(op->Val))
    : RetBadArg;
}

void CommandProcessor::CmdNext(xstring& ret, char* args)
{ int count = 1;
  ret = *args == 0 || parse_int(args, count)
    ? (const xstring&)SendCtrlCommand(Ctrl::MkSkip(count, true))
    : RetBadArg;
}

void CommandProcessor::CmdPrev(xstring& ret, char* args)
{ int count = 1;
  ret = *args == 0 || parse_int(args, count)
    ? (const xstring&)SendCtrlCommand(Ctrl::MkSkip(-count, true))
    : RetBadArg;
}

void CommandProcessor::CmdRewind(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op1(args);
  ret = op
    ? (const xstring&)SendCtrlCommand(Ctrl::MkScan(op->Val|Ctrl::Op_Rewind))
    : RetBadArg;
}

void CommandProcessor::CmdForward(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op1(args);
  ret = op
    ? (const xstring&)SendCtrlCommand(Ctrl::MkScan(op->Val))
    : RetBadArg;
}

void CommandProcessor::CmdJump(xstring& ret, char* args)
{ double pos;
  ret = parse_double(args, pos)
    ? (const xstring&)SendCtrlCommand(Ctrl::MkNavigate(xstring(), pos, false, false))
    : RetBadArg;
}

void CommandProcessor::CmdSavestream(xstring& ret, char* args)
{ ret = SendCtrlCommand(Ctrl::MkSave(args));
}

void CommandProcessor::CmdVolume(xstring& ret, char* args)
{ ret = xstring::sprintf("%f", Ctrl::GetVolume());
  if (*args)
  { double vol;
    bool sign = *args == '+' || *args == '-';
    if (parse_double(args, vol))
    { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkVolume(vol, sign));
      if (cmd->Flags != 0)
        ret = xstring::empty; //error
      cmd->Destroy();
    } else
      ret = xstring::empty; //error
  }
}

void CommandProcessor::CmdShuffle(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op1(args);
  ret = op
    ? (const xstring&)SendCtrlCommand(Ctrl::MkShuffle(op->Val))
    : RetBadArg;
}

void CommandProcessor::CmdRepeat(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op1(args);
  ret = op
    ? (const xstring&)SendCtrlCommand(Ctrl::MkRepeat(op->Val))
    : RetBadArg;
}

static bool IsRewind()
{ return Ctrl::GetScan() == DECFAST_REWIND;
}
static bool IsForward()
{ return Ctrl::GetScan() == DECFAST_FORWARD;
}
void CommandProcessor::CmdQuery(xstring& ret, char* args)
{ static const strmap<8, bool (*)()> map[] =
  { { "forward", &IsForward       },
    { "pause",   &Ctrl::IsPaused  },
    { "play",    &Ctrl::IsPlaying },
    { "repeat",  &Ctrl::IsRepeat  },
    { "rewind",  &IsRewind        },
    { "shuffle", &Ctrl::IsShuffle }
  };
  const strmap<8, bool (*)()>* op = mapsearch(map, args);
  if (op)
    ret = xstring::sprintf("%u", (*op->Val)());
}

void CommandProcessor::CmdCurrent(xstring& ret, char* args)
{ static const strmap<5, char> map[] =
  { { "",     0 },
    { "root", 1 },
    { "song", 0 }
  };
  const strmap<5, char>* op = mapsearch(map, args);
  if (op)
  { switch (op->Val)
    {case 0: // current song
      ret = (const xstring&)Ctrl::GetCurrentSong()->GetPlayable().URL;
      break;
     case 1:
      ret = (const xstring&)Ctrl::GetRoot()->GetPlayable().URL;
      break;
    }
  }
}

void CommandProcessor::CmdStatus(xstring& ret, char* args)
{ static const strmap<6, cfg_disp> map[] =
  { { "file", CFG_DISP_FILENAME },
    { "info", CFG_DISP_FILEINFO },
    { "tag",  CFG_DISP_ID3TAG }
  };
  const strmap<6, cfg_disp>* op = mapsearch(map, args);
  if (op)
  { int_ptr<APlayable> song = Ctrl::GetCurrentSong();
    if (song)
    { switch (op->Val)
      {case CFG_DISP_ID3TAG:
        ret = amp_construct_tag_string(&song->GetInfo());
        if (ret.length())
          break;
        // if tag is empty - use filename instead of it.
       case CFG_DISP_FILENAME:
        ret = song->GetPlayable().URL.getShortName();
        break;

       case CFG_DISP_FILEINFO:
        ret = song->GetInfo().tech->info;
        break;
      }
    }
  }
}

void CommandProcessor::CmdLocation(xstring& ret, char* args)
{ static const strmap<7, char> map[] =
  { { "",       0 },
    { "play",   0 },
    { "stopat", 1 }
  };
  const strmap<7, char>* op = mapsearch(map, args);
  if (op)
  { SongIterator loc;
    Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkLocation(&loc, op->Val));
    if (cmd->Flags == Ctrl::RC_OK)
      ret = loc.Serialize();
    cmd->Destroy();
  }
}

///// PLAYLIST

void CommandProcessor::CmdPlaylist(xstring& ret, char* args)
{ int_ptr<Playable> pp = Playable::GetByURL(url123::normalizeURL(args));
  //if (pp->GetFlags() & Playable::Enumerable)
  { CurPlaylist = pp;
    CurItem = NULL;
    ret = CurPlaylist->URL;
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

void CommandProcessor::CmdPlNext(xstring& ret, char* args)
{ int count = 1;
  if (CurPlaylist && (*args == 0 || parse_int(args, count)))
  { PlSkip(count);
    CmdPlItem(ret, args);
  }
}

void CommandProcessor::CmdPlPrev(xstring& ret, char* args)
{ int count = 1;
  if (CurPlaylist && (*args == 0 || parse_int(args, count)))
  { PlSkip(-count);
    CmdPlItem(ret, args);
  }
}

void CommandProcessor::CmdPlReset(xstring& ret, char* args)
{ CurItem = NULL;
}

void CommandProcessor::CmdPlCurrent(xstring& ret, char* args)
{ if (CurPlaylist)
    ret = CurPlaylist->URL;
}

void CommandProcessor::CmdPlItem(xstring& ret, char* args)
{ if (CurItem)
    ret = CurItem->GetPlayable().URL;
}

void CommandProcessor::CmdPlIndex(xstring& ret, char* args)
{ int ix = 0;
  int_ptr<PlayableInstance> cur = CurItem;
  while (cur)
  { ++ix;
    cur = CurPlaylist->GetPrev(CurItem);
  }
  ret = xstring::sprintf("%i", ix);
}

void CommandProcessor::CmdUse(xstring& ret, char* args)
{ if (CurPlaylist)
  { LoadHelper lh(cfg.playonload*LoadHelper::LoadPlay | cfg.append_cmd*LoadHelper::LoadAppend);
    lh.AddItem(CurPlaylist);
    ret = xstring::sprintf("%i", lh.SendCommand());
  } else
    ret = xstring::sprintf("%i", Ctrl::RC_NoList);
};

void CommandProcessor::CmdClear(xstring& ret, char* args)
{ CurPlaylist->Clear();
}

void CommandProcessor::CmdRemove(xstring& ret, char* args)
{ if (CurItem)
  { CurPlaylist->RemoveItem(CurItem);
    ret = (const xstring&)CurItem->GetPlayable().URL;
  }
}

void CommandProcessor::CmdAdd(xstring& ret, char* args)
{ if (CurPlaylist)
  { URLTokenizer tok(args);
    url123 url;
    Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    while (tok.Next(url))
    { if (!url)
      { ret = tok.Current();
        break; // Bad URL
      }
      CurPlaylist->InsertItem(*Playable::GetByURL(url), CurItem);
    }
  }
}

void CommandProcessor::CmdDir(xstring& ret, char* args)
{ if (CurPlaylist)
  { URLTokenizer tok(args);
    url123 url;
    Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    while (tok.Next(url))
    { if (!url || strchr(url, '?'))
      { ret = tok.Current();
        break; // Bad URL
      }
      if (url[url.length()-1] != '/')
        url = url + "/";
      CurPlaylist->InsertItem(*Playable::GetByURL(url), CurItem);
    }
  }
}

void CommandProcessor::CmdRdir(xstring& ret, char* args)
{ if (CurPlaylist)
  { URLTokenizer tok(args);
    url123 url;
    Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    while (tok.Next(url))
    { if (!url || strchr(url, '?'))
      { ret = tok.Current();
        break; // Bad URL
      }
      if (url[url.length()-1] != '/')
        url = url + "/";
      CurPlaylist->InsertItem(*Playable::GetByURL(url+"?recursive"), CurItem);
    }
  }
}

void CommandProcessor::CmdSave(xstring& ret, char* args)
{ int rc = Ctrl::RC_NoList;
  if (CurPlaylist)
  { const char* savename = args;
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
  ret = xstring::sprintf("%i", rc);
}


///// GUI

void CommandProcessor::CmdShow(xstring& ret, char* args)
{ GUI::Show(true);
}

void CommandProcessor::CmdHide(xstring& ret, char* args)
{ GUI::Show(false);
}

void CommandProcessor::CmdQuit(xstring& ret, char* args)
{ GUI::Quit();
}

void CommandProcessor::CmdOpen(xstring& ret, char* args)
{ char* arg2 = args + strcspn(args, " \t");
  if (*arg2)
  { *arg2++ = 0;
    arg2 += strspn(arg2, " \t");
  }
  static const strmap<12, int> map[] =
  { { "detailed",   GUI::DLT_PLAYLIST },
    { "metainfo",   GUI::DLT_METAINFO },
    { "playlist",   GUI::DLT_PLAYLIST },
    { "properties", -1           },
    { "techinfo",   GUI::DLT_TECHINFO },
    { "tagedit",    GUI::DLT_INFOEDIT },
    { "tree",       GUI::DLT_PLAYLISTTREE }
  };
  const strmap<12, int>* op = mapsearch(map, args);
  if (op)
  { if (op->Val >= 0)
    { if (*arg2 == 0)
        return; // Error
      int_ptr<Playable> pp = Playable::GetByURL(url123::normalizeURL(arg2));
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

void CommandProcessor::CmdSkin(xstring& ret, char* args)
{ ret = cfg.defskin;
  if (*args)
  { strlcpy(cfg.defskin, args, sizeof cfg.defskin);
    GUI::ReloadSkin();
  }
}

///// CONFIGURATION

void CommandProcessor::CmdSize(xstring& ret, char* args)
{ // old value
  ret = xstring::sprintf("%i", cfg.mode);
  static const strmap<8, cfg_mode> map[] =
  { { "0",       CFG_MODE_REGULAR },
    { "1",       CFG_MODE_SMALL },
    { "2",       CFG_MODE_TINY },
    { "normal",  CFG_MODE_REGULAR },
    { "regular", CFG_MODE_REGULAR },
    { "small",   CFG_MODE_SMALL },
    { "tiny",    CFG_MODE_TINY }
  };
  if (*args)
  { const strmap<8, cfg_mode>* mp = mapsearch(map, args);
    if (mp)
      GUI::SetWindowMode(mp->Val);
    else
      ret = xstring::empty; // error
  }
};

void CommandProcessor::CmdFont(xstring& ret, char* args)
{ // old value
  if (cfg.font_skinned)
    ret = xstring::sprintf("%i", cfg.font+1);
  else
    ret = xstring::sprintf("%i.%s", cfg.font_size, cfg.font_attrs.szFacename);

  if (*args)
  { // set new value
    int font = 0;
    char* cp = strchr(args, '.');
    if (cp)
      *cp++ = 0;
    if (!parse_int(args, font))
    { ret = xstring::empty; // error
      return;
    }
    if (cp)
    { // non skinned font
      FATTRS fattrs = { sizeof(FATTRS), 0, 0, "", 0, 0, 16, 7, 0, 0 };
      strlcpy(fattrs.szFacename, cp, sizeof fattrs.szFacename);
      cfg.font_skinned = false;
      cfg.font_attrs   = fattrs;
      cfg.font_size    = font;
      // TODO: we should validate the font here.
      //amp_invalidate(UPD_FILENAME);
    } else
    { switch (font)
      {case 1:
       case 2:
        cfg.font_skinned = true;
        cfg.font         = font;
        //amp_invalidate(UPD_FILENAME);
        break;
       default:
        ret = xstring::empty; // error
      }
    }
  }
}

void CommandProcessor::CmdFloat(xstring& ret, char* args)
{ // old value
  ret = cfg.floatontop ? "on" : "off";
  if (*args)
  { const strmap<8, Ctrl::Op>* op = parse_op2(args);
    if (op)
    { switch (op->Val)
      {case Ctrl::Op_Clear:
        if (!cfg.floatontop)
          return;
        break;
       case Ctrl::Op_Set:
        if (cfg.floatontop)
          return;
       default:;
      }
      GUI::SetWindowFloat(!cfg.floatontop);
    } else
      ret = xstring::empty;
  }
}

void CommandProcessor::CmdAutouse(xstring& ret, char* args)
{ cmd_op_bool(ret, args, cfg.autouse);
}

void CommandProcessor::CmdPlayonload(xstring& ret, char* args)
{ cmd_op_bool(ret, args, cfg.playonload);
}

const CommandProcessor::CmdEntry CommandProcessor::CmdList[] = // list must be sorted!!!
{ { "add",        &CommandProcessor::CmdAdd        },
  { "autouse",    &CommandProcessor::CmdAutouse    },
  { "clear",      &CommandProcessor::CmdClear      },
  { "current",    &CommandProcessor::CmdCurrent    },
  { "dir",        &CommandProcessor::CmdDir        },
  { "float",      &CommandProcessor::CmdFloat      },
  { "font",       &CommandProcessor::CmdFont       },
  { "forward",    &CommandProcessor::CmdForward    },
  { "hide",       &CommandProcessor::CmdHide       },
  { "jump",       &CommandProcessor::CmdJump       },
  { "load",       &CommandProcessor::CmdLoad       },
  { "location",   &CommandProcessor::CmdLocation   },
  { "next",       &CommandProcessor::CmdNext       },
  { "open",       &CommandProcessor::CmdOpen       },
  { "pause",      &CommandProcessor::CmdPause      },
  { "pl_current", &CommandProcessor::CmdPlCurrent  },
  { "pl_item",    &CommandProcessor::CmdPlItem     },
  { "pl_next",    &CommandProcessor::CmdPlNext     },
  { "pl_prev",    &CommandProcessor::CmdPlPrev     },
  { "pl_reset",   &CommandProcessor::CmdPlReset    },
  { "play",       &CommandProcessor::CmdPlay       },
  { "playlist",   &CommandProcessor::CmdPlaylist   },
  { "playonload", &CommandProcessor::CmdPlayonload },
  { "prev",       &CommandProcessor::CmdPrev       },
  { "previous",   &CommandProcessor::CmdPrev       },
  { "query",      &CommandProcessor::CmdQuery      },
  { "rdir",       &CommandProcessor::CmdRdir       },
  { "remove",     &CommandProcessor::CmdRemove     },
  { "repeat",     &CommandProcessor::CmdRepeat     },
  { "rewind",     &CommandProcessor::CmdRewind     },
  { "save",       &CommandProcessor::CmdSave       },
  { "savestream", &CommandProcessor::CmdSavestream },
  { "show",       &CommandProcessor::CmdShow       },
  { "shuffle",    &CommandProcessor::CmdShuffle    },
  { "size",       &CommandProcessor::CmdSize       },
  { "skin",       &CommandProcessor::CmdSkin       },
  { "status",     &CommandProcessor::CmdStatus     },
  { "stop",       &CommandProcessor::CmdStop       },
  { "use",        &CommandProcessor::CmdUse        },
  { "volume",     &CommandProcessor::CmdVolume     }
};

CommandProcessor::CommandProcessor()
: CurPlaylist(GUI::GetDefaultPL())
{}

void CommandProcessor::Execute(xstring& ret, char* buffer)
{ DEBUGLOG(("CommandProcessor::Execute(, %s)\n", buffer));
  if( buffer[0] != '*' )
    CmdLoad(ret, buffer);
  else
  { ++buffer;
    buffer += strspn(buffer, " \t"); // skip leading blanks
    // extract command
    size_t len = strcspn(buffer, " \t");
    char* ap = buffer + len;
    ap += strspn(ap, " \t"); // skip leading blanks
    buffer[len] = 0;
    DEBUGLOG(("execute_command: %s %u %s\n", buffer, len, ap));
    { // remove trailing blanks
      char* ape = ap + strlen(ap);
      while (ape != ap && (ape[-1] == ' ' || ape[-1] == '\t'))
        --ape;
      *ape = 0;
    }
    // Now buffer points to the command, ap to the arguments. Both are stripped.

    // Seach command handler ...
    const CmdEntry* cep = mapsearch(CmdList, buffer);
    DEBUGLOG(("execute_command: %s(%s) -> %p\n", buffer, ap, cep));
    if (cep)
      // ... and execute
      (this->*cep->Val)(ret, ap);
  }
}

/****************************************************************************
* Pipe interface
****************************************************************************/

static HPIPE HPipe        = NULLHANDLE;
static TID   TIDWorker    = (TID)-1;


/* Dispatches requests received from the pipe. */
static void TFNENTRY pipe_thread( void* )
{ DEBUGLOG(("pipe_thread()\n"));

  APIRET rc;
  HAB hab = WinInitialize( 0 );
  HMQ hmq = WinCreateMsgQueue( hab, 0 );

  CommandProcessor cmdproc;

  for(;; DosDisConnectNPipe( HPipe ))
  { // Connect the pipe
    rc = DosConnectNPipe( HPipe );
    if (rc != NO_ERROR)
    { DEBUGLOG(("pipe_thread: DosConnectNPipe failed with rc = %u\n", rc));
      break;
    }

    // read a command
    static char buffer[65536];
    ULONG bytesread;
   nextcommand:
    rc = DosRead(HPipe, buffer, sizeof buffer -1, &bytesread);
    if (rc != NO_ERROR)
    { DEBUGLOG(("pipe_thread: DosRead failed with rc = %u\n", rc));
      continue;
    }
    if (bytesread == 0)
      continue;
    buffer[bytesread] = 0; // ensure terminating zero

    // execute commands
    char* cp = buffer;
    for (;;)
    { size_t len = strcspn(cp, "\r\n");
      DEBUGLOG(("pipe_thread: at %s, %u, %i, %u\n", cp, bytesread, cp-buffer, len));
      if (cp + len == buffer + bytesread)
      { // Command string goes to the end of the buffer.
        // This implies that there was no terminating newline in the buffer.
        // => Try to read the remaining part of the command.
        // But firstly move the command to the beginning of the buffer if required.
        if (cp != buffer)
        { memmove(buffer, cp, len+1);
          bytesread -= cp - buffer;
          cp = buffer;
        }
        ULONG bytesread2;
        rc = DosRead(HPipe, buffer + bytesread, sizeof buffer - bytesread -1, &bytesread2);
        if (rc == NO_ERROR)
        { bytesread += bytesread2;
          buffer[bytesread] = 0; // ensure terminating zero
          continue;
        } else if (rc != ERROR_NO_DATA)
        { DEBUGLOG(("pipe_thread: DosRead failed with rc = %u\n", rc));
          break; // results in continue in the outer loop
        }
      }
      cp[len] = 0; // terminate command
      // strip leading spaces
      char* cp2 = cp + strspn(cp, " \t");
      // and execute (if non-blank)
      if (*cp2)
      { xstring ret = xstring::empty;
        cmdproc.Execute(ret, cp2);
        DEBUGLOG(("pipe_thread: command done: %s\n", ret.cdata()));
        // send reply
        ULONG actual;
        DosWrite(HPipe, (PVOID)ret.cdata(), ret.length(), &actual);
        DosWrite(HPipe, "\r\n", 2, &actual);
        DosResetBuffer(HPipe);
      }
      // next line
      cp += len +1; // and skip delimiter
      if (cp >= buffer + bytesread)
        goto nextcommand;
    }
  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
}


/****************************************************************************
* Public functions
****************************************************************************/

/* Create main pipe with only one instance possible since these pipe
   is almost all the time free, it wouldn't make sense having multiple
   intances. */
bool amp_pipe_create( void )
{ DEBUGLOG(("amp_pipe_create() - %p\n", HPipe));
  ULONG rc = DosCreateNPipe( cfg.pipe_name, &HPipe,
                             NP_ACCESS_DUPLEX, NP_WAIT|NP_TYPE_BYTE|NP_READMODE_BYTE | 1,
                             2048, 2048, 500 );

  if( rc != 0 && rc != ERROR_PIPE_BUSY ) {
    amp_player_error( "Could not create pipe %s, rc = %d.", cfg.pipe_name.cdata(), rc );
    return false;
  }

  TIDWorker = _beginthread(pipe_thread, NULL, 65536, NULL);
  CASSERT((int)TIDWorker != -1);
  return true;
}

/* Shutdown the player pipe. */
void amp_pipe_destroy()
{ DEBUGLOG(("amp_pipe_destroy() - %p\n", HPipe));
  ORASSERT(DosClose(HPipe));
  HPipe = NULLHANDLE;
}

/* Opens specified pipe and writes data to it. */
bool amp_pipe_open_and_write( const char* pipename, const char* data, size_t size )
{
  HPIPE  hpipe;
  ULONG  action;
  APIRET rc;

  rc = DosOpen((PSZ)pipename, &hpipe, &action, 0, FILE_NORMAL,
                OPEN_ACTION_FAIL_IF_NEW  | OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE | OPEN_FLAGS_FAIL_ON_ERROR,
                NULL );

  if( rc == NO_ERROR )
  {
    DosWrite( hpipe, (PVOID)data, size, &action );
    DosResetBuffer( hpipe );
    DosDisConnectNPipe( hpipe );
    DosClose( hpipe );
    return true;
  } else {
    return false;
  }
}
