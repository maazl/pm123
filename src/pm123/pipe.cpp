/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2008-2008 M.Mueller
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
#include "pm123.h"
#include "pm123.rc.h"
#include "properties.h"
#include "controller.h"
#include "123_util.h"
#include <cpp/xstring.h>
#include <debuglog.h>
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>


static HPIPE HPipe        = NULLHANDLE;
static TID   TIDWorker    = (TID)-1;

// Instance vars
static int_ptr<PlayableCollection> CurPlaylist; // playlist where we currently operate
static int_ptr<PlayableInstance>   CurItem;     // current item of the above playlist


template <int LEN, class V>
struct strmap
{ char Str[LEN];
  V    Val;
};

template <class T>
inline T* mapsearch2(T* map, size_t count, const char* cmd)
{ return (T*)bsearch(cmd, map, count, sizeof(T), (int(TFNENTRY*)(const void*, const void*))&stricmp);
}
#ifdef __IBMCPP__
// IBM C work around
// IBM C cannot deduce the array size from the template argument.
#define mapsearch(map, arg) mapsearch2(map, sizeof map / sizeof *map, arg)
#else
template <size_t I, class T>
inline T* mapsearch(T (&map)[I], const char* cmd)
{ return (T*)bsearch(cmd, map, I, sizeof(T), (int(TFNENTRY*)(const void*, const void*))&stricmp);
}
#endif

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

void cmd_op_BOOL(xstring& ret, const char* arg, BOOL& val)
{ ret = val ? "on" : "off";
  if (*arg)
  { const strmap<8, Ctrl::Op>* op = parse_op2(arg);
    if (op)
    { switch (op->Val)
      {case Ctrl::Op_Set:
        val = TRUE;
        break;
       case Ctrl::Op_Toggle:
        val = !val;
        break;
       default:
        val = FALSE;
        break;
      }
    } else
      ret = xstring::empty; // error
  }
}


///// PLAY CONTROL

static void cmd_load(xstring& ret, char* args)
{ if (is_dir(args))
    // TODO: buffer may overrun???
    strcat(args, "\\");
  amp_load_playable(PlayableSlice(Playable::GetByURL(url123::normalizeURL(args))), AMP_LOAD_NOT_RECALL|(cfg.append_cmd*AMP_LOAD_APPEND));
  // TODO: reply and sync wait
}

static void cmd_play(xstring& ret, char* args)
{ if (*args)
    cmd_load(ret, args);
  // TODO: make atomic
  Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkPlayStop(Ctrl::Op_Set));
  ret = xstring::sprintf("%i", cmd->Flags);
  delete cmd;
}

static void cmd_stop(xstring& ret, char* args)
{ Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkPlayStop(Ctrl::Op_Clear));
  ret = xstring::sprintf("%i", cmd->Flags);
  delete cmd;
}

static void cmd_pause(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op1(args);
  if (op)
  { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkPause(op->Val));
    ret = xstring::sprintf("%i", cmd->Flags);
    delete cmd;
  } else
    ret = xstring::sprintf("%i", Ctrl::RC_BadArg);
}

static void cmd_next(xstring& ret, char* args)
{ int count = 1;
  if (*args == 0 || parse_int(args, count))
  { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkSkip(count, true));
    ret = xstring::sprintf("%i", cmd->Flags);
    delete cmd;
  } else
    ret = xstring::sprintf("%i", Ctrl::RC_BadArg);
}

static void cmd_prev(xstring& ret, char* args)
{ int count = 1;
  if (*args == 0 || parse_int(args, count))
  { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkSkip(-count, true));
    ret = xstring::sprintf("%i", cmd->Flags);
    delete cmd;
  } else
    ret = xstring::sprintf("%i", Ctrl::RC_BadArg);
}

static void cmd_rewind(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op1(args);
  if (op)
  { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkScan(op->Val|Ctrl::Op_Rewind));
    ret = xstring::sprintf("%i", cmd->Flags);
    delete cmd;
  } else
    ret = xstring::sprintf("%i", Ctrl::RC_BadArg);
}

static void cmd_forward(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op1(args);
  if (op)
  { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkScan(op->Val));
    ret = xstring::sprintf("%i", cmd->Flags);
    delete cmd;
  } else
    ret = xstring::sprintf("%i", Ctrl::RC_BadArg);
}

static void cmd_jump(xstring& ret, char* args)
{ double pos;
  if (parse_double(args, pos))
  { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkNavigate(xstring(), pos, false, false));
    ret = xstring::sprintf("%i", cmd->Flags);
    delete cmd;
  } else
    ret = xstring::sprintf("%i", Ctrl::RC_BadArg);
}

static void cmd_volume(xstring& ret, char* args)
{ ret = xstring::sprintf("%f", Ctrl::GetVolume());
  if (*args)
  { double vol;
    bool sign = *args == '+' || *args == '-';
    if (parse_double(args, vol))
    { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkVolume(vol, sign));
      if (cmd->Flags != 0)
        ret = xstring::empty; //error
      delete cmd;
    } else
      ret = xstring::empty; //error
  }
}

static void cmd_shuffle(xstring& ret, char* args)
{ ret = Ctrl::IsShuffle() ? "on" : "off";
  const strmap<8, Ctrl::Op>* op = parse_op1(args);
  if (op)
  { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkShuffle(op->Val));
    if (cmd->Flags != 0)
      ret = xstring::empty; //error
    delete cmd;
  } else
    ret = xstring::empty;
}

static void cmd_repeat(xstring& ret, char* args)
{ ret = Ctrl::IsRepeat() ? "on" : "off";
  const strmap<8, Ctrl::Op>* op = parse_op1(args);
  if (op)
  { Ctrl::ControlCommand* cmd = Ctrl::SendCommand(Ctrl::MkRepeat(op->Val));
    if (cmd->Flags != 0)
      ret = xstring::empty; //error
    delete cmd;
  } else
    ret = xstring::empty;
}

static void cmd_status(xstring& ret, char* args)
{ static const strmap<6, cfg_disp> map[] =
  { { "file", CFG_DISP_FILENAME },
    { "info", CFG_DISP_FILEINFO },
    { "tag",  CFG_DISP_ID3TAG }
  };
  const strmap<6, cfg_disp>* op = mapsearch(map, args);
  if (op)
  { int_ptr<Song> song = Ctrl::GetCurrentSong();
    if (song)
    { switch (op->Val)
      {case CFG_DISP_ID3TAG:
        ret = amp_construct_tag_string(&song->GetInfo());
        if (ret.length())
          break;
        // if tag is empty - use filename instead of it.
       case CFG_DISP_FILENAME:
        ret = song->GetURL().getShortName();
        break;

       case CFG_DISP_FILEINFO:
        ret = song->GetInfo().tech->info;
        break;
      }
    }
  }
}

static void cmd_hide(xstring& ret, char* args)
{ WinSendMsg( amp_player_window(), WM_COMMAND, MPFROMSHORT( IDM_M_MINIMIZE ), 0 );
}

///// PLAYLIST

static void cmd_playlist(xstring& ret, char* args)
{ int_ptr<Playable> pp = Playable::GetByURL(args);
  if (pp->GetFlags() & Playable::Enumerable)
  { CurPlaylist = (PlayableCollection*)&*pp;
    CurItem = NULL;
  }
  // TODO: result
};

static void cmd_pl_next(xstring& ret, char* args)
{ if (CurPlaylist)
  { CurItem = CurPlaylist->GetNext(CurItem);
    if (CurItem)
      ret = CurItem->GetPlayable()->GetURL();
  }
}

static void cmd_pl_prev(xstring& ret, char* args)
{ if (CurPlaylist)
  { CurItem = CurPlaylist->GetPrev(CurItem);
    if (CurItem)
      ret = CurItem->GetPlayable()->GetURL();
  }
}

static void cmd_pl_reset(xstring& ret, char* args)
{ CurItem = NULL;
}

static void cmd_use(xstring& ret, char* args)
{ if (CurPlaylist)
  { amp_load_playable(PlayableSlice(CurPlaylist), AMP_LOAD_NOT_RECALL);
    // TODO: reply and sync wait
  }
};

static void cmd_clear(xstring& ret, char* args)
{ if (CurPlaylist && (CurPlaylist->GetFlags() & Playable::Mutable) == Playable::Mutable)
  { ((Playlist&)*CurPlaylist).Clear();
  }
  // TODO: reply
}

static void cmd_remove(xstring& ret, char* args)
{ if (CurItem && (CurPlaylist->GetFlags() & Playable::Mutable) == Playable::Mutable)
  { ((Playlist&)*CurPlaylist).RemoveItem(CurItem);
  }
  // TODO: reply
}

static void cmd_add(xstring& ret, char* args)
{ if (CurPlaylist && (CurPlaylist->GetFlags() & Playable::Mutable) == Playable::Mutable)
  { Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    // parse args
    do
    { char* cp = *args == '"' ? strchr(++args, '"') : strchr(args, ';');
      if (cp)
        *cp++ = 0;
      if (*args)
        ((Playlist&)*CurPlaylist).InsertItem(PlayableSlice(Playable::GetByURL(url123::normalizeURL(args))), CurItem);
      args = cp;
    } while (args);
  }
  // TODO: reply
}

static void cmd_dir(xstring& ret, char* args)
{ if (CurPlaylist && (CurPlaylist->GetFlags() & Playable::Mutable) == Playable::Mutable)
  { Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    // parse args
    do
    { char* cp = *args == '"' ? strchr(++args, '"') : strchr(args, ';');
      if (cp)
        *cp++ = 0;
      if (*args)
      { xstring url = url123::normalizeURL(args);
        if (url[url.length()-1] != '/')
          url = url + "/";
        ((Playlist&)*CurPlaylist).InsertItem(PlayableSlice(Playable::GetByURL(url)), CurItem);
      }
      args = cp;
    } while (args);
  }
  // TODO: reply
}

static void cmd_rdir(xstring& ret, char* args)
{ if (CurPlaylist && (CurPlaylist->GetFlags() & Playable::Mutable) == Playable::Mutable)
  { Mutex::Lock lck(CurPlaylist->Mtx); // Atomic
    // parse args
    do
    { char* cp = *args == '"' ? strchr(++args, '"') : strchr(args, ';');
      if (cp)
        *cp++ = 0;
      if (*args)
      { xstring url = url123::normalizeURL(args);
        if (url[url.length()-1] != '/')
          url = url + "/";
        ((Playlist&)*CurPlaylist).InsertItem(PlayableSlice(Playable::GetByURL(url+"?recursive")), CurItem);
      }
      args = cp;
    } while (args);
  }
  // TODO: reply
}

///// CONFIGURATION

static void cmd_size(xstring& ret, char* args)
{ // old value
  ret = xstring::sprintf("%i", cfg.mode);
  static const strmap<10, int> map[] =
  { { "0",       IDM_M_NORMAL },
    { "1",       IDM_M_SMALL },
    { "2",       IDM_M_TINY },
    { "normal",  IDM_M_NORMAL },
    { "regular", IDM_M_NORMAL },
    { "small",   IDM_M_SMALL },
    { "tiny",    IDM_M_TINY }
  };
  if (*args)
  { const strmap<10, int>* mp = mapsearch(map, args);
    if (mp)
      WinSendMsg(amp_player_window(), WM_COMMAND, MPFROMSHORT(mp->Val), 0);
    else
      ret = xstring::empty; // error
  }
};

static void cmd_font(xstring& ret, char* args)
{ // old value
  if (cfg.font_skinned)
    ret = xstring::sprintf("%i", cfg.font+1);
  else
    ret = xstring::sprintf("%i.%s", cfg.font_size, cfg.font_attrs.szFacename);

  if (*args)
  { // set new value
    int font;
    if (parse_int(args, font))
    { switch (font)
      {case 1:
        WinSendMsg(amp_player_window(), WM_COMMAND, MPFROMSHORT(IDM_M_FONT1), 0);
        break;
       case 2:
        WinSendMsg(amp_player_window(), WM_COMMAND, MPFROMSHORT(IDM_M_FONT2), 0);
        break;
       default:
        ret = xstring::empty; // error
      }
    } else
    { // TODO: non-skinned font
      ret = xstring::empty; // error
    }
  }
}

static void cmd_float(xstring& ret, char* args)
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
      WinSendMsg(amp_player_window(), WM_COMMAND, MPFROMSHORT(IDM_M_FLOAT), 0);
    } else
      ret = xstring::empty;
  }
}

static void cmd_autouse(xstring& ret, char* args)
{ cmd_op_BOOL(ret, args, cfg.autouse);
}

static void cmd_playonload(xstring& ret, char* args)
{ cmd_op_BOOL(ret, args, cfg.playonload);
}

static void cmd_playonuse(xstring& ret, char* args)
{ cmd_op_BOOL(ret, args, cfg.playonuse);
}


static const struct CmdEntry
{ char Prefix[12];
  void (*ExecFn)(xstring& ret, char* args);
} CmdList[] = // list must be sorted!!!
{ { "add",        &cmd_add        },
  { "autouse",    &cmd_autouse    },
  { "clear",      &cmd_clear      },
  { "dir",        &cmd_dir        },
  { "float",      &cmd_float      },
  { "font",       &cmd_font       },
  { "forward",    &cmd_forward    },
  { "hide",       &cmd_hide       },
  { "jump",       &cmd_jump       },
  { "load",       &cmd_load       },
  { "next",       &cmd_next       },
  { "pause",      &cmd_pause      },
  { "pl_next",    &cmd_pl_next    },
  { "pl_prev",    &cmd_pl_prev    },
  { "pl_reset",   &cmd_pl_reset   },
  { "play",       &cmd_play       },
  { "playlist",   &cmd_playlist   },
  { "playonload", &cmd_playonload },
  { "playonuse",  &cmd_playonuse  },
  { "prev",       &cmd_prev       },
  { "previous",   &cmd_prev       },
  { "rdir",       &cmd_rdir       },
  { "remove",     &cmd_remove     },
  { "repeat",     &cmd_repeat     },
  { "rewind",     &cmd_rewind     },
  { "size",       &cmd_size       },
  { "shuffle",    &cmd_shuffle    },
  { "status",     &cmd_status     },
  { "stop",       &cmd_stop       },
  { "use",        &cmd_use        },
  { "volume",     &cmd_volume     }
};


static void execute_command(xstring& ret, char* buffer)
{ DEBUGLOG(("execute_command(, %s)\n", buffer));
  if( buffer[0] != '*' )
    cmd_load(ret, buffer);
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
      (*cep->ExecFn)(ret, ap);
  }
}

/* Dispatches requests received from the pipe. */
// TODO: start the pipe thread!
static void TFNENTRY pipe_thread( void* )
{ DEBUGLOG(("pipe_thread()\n"));

  APIRET rc;
  HAB hab = WinInitialize( 0 );
  HMQ hmq = WinCreateMsgQueue( hab, 0 );

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
        execute_command(ret, cp2);
        DEBUGLOG(("pipe_thread: command done: %s\n", ret.cdata()));
        // send reply
        ULONG actual;
        DosWrite(HPipe, (PVOID)ret.cdata(), ret.length(), &actual);
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
{
  ULONG rc = DosCreateNPipe( cfg.pipe_name, &HPipe,
                             NP_ACCESS_DUPLEX, NP_WAIT|NP_TYPE_BYTE|NP_READMODE_BYTE | 1,
                             2048, 2048, 500 );

  if( rc != 0 && rc != ERROR_PIPE_BUSY ) {
    amp_player_error( "Could not create pipe %s, rc = %d.", cfg.pipe_name, rc );
    return false;
  }

  CurPlaylist = amp_get_default_pl();

  TIDWorker = _beginthread(pipe_thread, NULL, 65536, NULL);
  CASSERT(TIDWorker != -1);
  return true;
}

/* Shutdown the player pipe. */
void amp_pipe_destroy()
{ DosClose(HPipe);
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
