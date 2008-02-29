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
static TID   TIDWorker    = -1;

static char  Pipename[50] = "\\PIPE\\PM123";

// Instance vars
static int_ptr<PlayableCollection> CurPlaylist; // playlist where we currently operate
static int_ptr<PlayableInstance>   CurItem;     // current item of the above playlist


template <int LEN, class V>
struct strmap
{ char Str[LEN];
  V    Val;
};

#ifdef __IBMCPP__
// IBM C work around
// IBM C cannot deduce the array size from the template argument.
template <class T>
inline T* mapsearch2(T (&map)[], size_t count, const char* cmd)
{ return (T*)bsearch(cmd, map, count, sizeof(T), (int(TFNENTRY*)(const void*, const void*))&stricmp); 
}
#define mapsearch(map, arg) mapsearch2(map, sizeof map / sizeof *map, arg)
#else
template <size_t I, class T>
inline T* mapsearch(T (&map)[I], const char* cmd)
{ return (T*)bsearch(cmd, map, I, sizeof(T), (int(TFNENTRY*)(const void*, const void*))&stricmp); 
}
#endif

int parse_bool(const char* arg)
{ static const strmap<6, bool> map[] =
  { { "0",     false },
    { "1",     true },
    { "false", false },
    { "no",    false },
    { "off",   false },
    { "on",    true },
    { "true",  true },
    { "yes",   true }
  };
  const strmap<6, bool>* mp = mapsearch(map, arg);
  return mp ? mp->Val : -1;
}

bool parse_int(const char* arg, int& val)
{ int v;
  size_t n;
  if (sscanf(arg, "%i%n", &v, &n) == 1 && n == strlen(arg))
  { val = v;
    return true;
  } else
    return false;
}

bool parse_double(const char* arg, double& val)
{ double v;
  size_t n;
  if (sscanf(arg, "%lf%n", &v, &n) == 1 && n == strlen(arg))
  { val = v;
    return true;
  } else
    return false;
}

const strmap<8, Ctrl::Op>* parse_op(const char* arg)
{ static const strmap<8, Ctrl::Op> map[] =
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
  return mapsearch(map, arg);
}


///// PLAY CONTROL

static void cmd_load(xstring& ret, char* args)
{ if (is_dir(args))
    // TODO: buffer overrun!!!!
    strcat(args, "\\");
  amp_load_playable(url123::normalizeURL(args), 0, AMP_LOAD_NOT_RECALL);
  // TODO: reply and sync wait
}

static void cmd_play(xstring& ret, char* args)
{ if (*args)
    cmd_load(ret, args);
  // TODO: make atomic
  Ctrl::PostCommand(Ctrl::MkPlayStop(Ctrl::Op_Set));
  // TODO: reply and sync wait
}

static void cmd_stop(xstring& ret, char* args)
{ Ctrl::PostCommand(Ctrl::MkPlayStop(Ctrl::Op_Clear));
  // TODO: reply and sync wait
}

static void cmd_pause(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op(args);
  if (op)
  { Ctrl::PostCommand(Ctrl::MkPause(op->Val));
    // TODO: reply and sync wait
  }
}

static void cmd_next(xstring& ret, char* args)
{ int count = 1;
  if (*args == 0 || parse_int(args, count))
  { Ctrl::PostCommand(Ctrl::MkSkip(count, true));
    // TODO: reply and sync wait
  }
}

static void cmd_prev(xstring& ret, char* args)
{ int count = 1;
  if (*args == 0 || parse_int(args, count))
  { Ctrl::PostCommand(Ctrl::MkSkip(-count, true));
    // TODO: reply and sync wait
  }
}

static void cmd_rewind(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op(args);
  if (op)
  { Ctrl::PostCommand(Ctrl::MkScan(op->Val|Ctrl::Op_Rewind));
    // TODO: reply and sync wait
  }
}

static void cmd_forward(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op(args);
  if (op)
  { Ctrl::PostCommand(Ctrl::MkScan(op->Val));
    // TODO: reply and sync wait
  }
}

static void cmd_jump(xstring& ret, char* args)
{ double pos;
  if (parse_double(args, pos))
  { Ctrl::PostCommand(Ctrl::MkNavigate(xstring(), pos, false, false));
    // TODO: reply and sync wait
  }
}

static void cmd_volume(xstring& ret, char* args)
{ double vol;
  bool sign = *args == '+' || *args == '-';
  if (parse_double(args, vol))
  { Ctrl::PostCommand(Ctrl::MkVolume(vol, sign));
    // TODO: reply and sync wait
  }
}

static void cmd_shuffle(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op(args);
  if (op)
  { Ctrl::PostCommand(Ctrl::MkShuffle(op->Val));
    // TODO: reply and sync wait
  }
}

static void cmd_repeat(xstring& ret, char* args)
{ const strmap<8, Ctrl::Op>* op = parse_op(args);
  if (op)
  { Ctrl::PostCommand(Ctrl::MkRepeat(op->Val));
    // TODO: reply and sync wait
  }
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
  { amp_load_playable(CurPlaylist->GetURL(), 0, AMP_LOAD_NOT_RECALL);
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
        ((Playlist&)*CurPlaylist).InsertItem(url123::normalizeURL(args), xstring(), CurItem);
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
        ((Playlist&)*CurPlaylist).InsertItem(url, xstring(), CurItem);
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
        ((Playlist&)*CurPlaylist).InsertItem(url+"?recursive", xstring(), CurItem);
      }
      args = cp;
    } while (args); 
  }
  // TODO: reply
}

///// CONFIGURATION

static void cmd_size(xstring& ret, char* args)
{ static const strmap<10, int> map[] =
  { { "0",       IDM_M_NORMAL },
    { "1",       IDM_M_SMALL },
    { "2",       IDM_M_TINY },
    { "normal",  IDM_M_NORMAL }, 
    { "regular", IDM_M_NORMAL }, 
    { "small",   IDM_M_SMALL },
    { "tiny",    IDM_M_TINY }
  };
  const strmap<10, int>* mp = mapsearch(map, args);
  if (mp)
  { ret = xstring::sprintf("%i", cfg.mode);
    WinSendMsg(amp_player_window(), WM_COMMAND, MPFROMSHORT(mp->Val), 0);
  }
};


static const struct CmdEntry
{ char Prefix[12];
  void (*ExecFn)(xstring& ret, char* args);
} CmdList[] =
{ { "add",      &cmd_add },
  { "clear",    &cmd_clear },
  { "dir",      &cmd_dir },
  { "forward",  &cmd_forward },
  { "jump",     &cmd_jump },
  { "load",     &cmd_load },
  { "next",     &cmd_next },
  { "pause",    &cmd_pause },
  { "pl_next",  &cmd_pl_next },
  { "pl_prev",  &cmd_pl_prev },
  { "pl_reset", &cmd_pl_reset },
  { "play",     &cmd_play },
  { "playlist", &cmd_playlist },
  { "previous", &cmd_prev },
  { "remove",   &cmd_remove },
  { "repeat",   &cmd_repeat },
  { "rewind",   &cmd_rewind },
  { "rdir",     &cmd_rdir },
  { "size",     &cmd_size },
  { "shuffle",  &cmd_shuffle },
  { "status",   &cmd_status },
  { "stop",     &cmd_stop },
  { "use",      &cmd_use },
  { "volume",   &cmd_volume }
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
    ap += strspn(buffer, " \t"); // skip leading blanks
    buffer[len] = 0;
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

#if 0
    if( zork )
    {
/*      if( stricmp( zork, "status" ) == 0 ) {
        if( dork ) {
          // TODO: makes no more sense with playlist objects
          int_ptr<Playable> current = Ctrl::GetRoot();
          if( !current ) {
            amp_pipe_write( hpipe, "" );
          } else if( stricmp( dork, "file" ) == 0 ) {
            amp_pipe_write( hpipe, current->GetURL().cdata() );
          } else if( stricmp( dork, "tag"  ) == 0 ) {
            current->EnsureInfo(Playable::IF_All);
            amp_pipe_write( hpipe, amp_construct_tag_string( &current->GetInfo() ).cdata() );
          } else if( stricmp( dork, "info" ) == 0 ) {
            current->EnsureInfo(Playable::IF_Tech);
            amp_pipe_write( hpipe, current->GetInfo().tech->info );
          }
        }
      }*/
/*      if( stricmp( zork, "size" ) == 0 ) {
        if( dork ) {
          if( stricmp( dork, "regular" ) == 0 ||
              stricmp( dork, "0"       ) == 0 ||
              stricmp( dork, "normal"  ) == 0  )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_NORMAL ), 0 );
          }
          if( stricmp( dork, "small"   ) == 0 ||
              stricmp( dork, "1"       ) == 0  )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_SMALL  ), 0 );
          }
          if( stricmp( dork, "tiny"    ) == 0 ||
              stricmp( dork, "2"       ) == 0  )
          {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_TINY   ), 0 );
          }
        }
      }*/
/*      if( stricmp( zork, "rdir" ) == 0 ) {
        if( dork ) {
          // TODO: edit playlist
          //pl_add_directory( dork, PL_DIR_RECURSIVE );
        }
      }
      if( stricmp( zork, "dir"  ) == 0 ) {
        if( dork ) {
          // TODO: edit playlist
          //pl_add_directory( dork, 0 );
        }
      }*/
      if( stricmp( zork, "font" ) == 0 ) {
        if( dork ) {
          if( stricmp( dork, "1" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FONT1 ), 0 );
          }
          if( stricmp( dork, "2" ) == 0 ) {
            WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FONT2 ), 0 );
          }
        }
      }
/*      if( stricmp( zork, "add" ) == 0 )
      {
        char* file;

        if( dork ) {
          while( *dork ) {
            file = dork;
            while( *dork && *dork != ';' ) {
              ++dork;
            }
            if( *dork == ';' ) {
              *dork++ = 0;
            }
            // TODO: edit playlist
            //pl_add_file( file, NULL, 0 );
          }
        }
      }*/
/*      if( stricmp( zork, "load" ) == 0 ) {
        if( dork ) {
          // TODO: dir
          amp_load_playable( url123::normalizeURL(dork), 0, 0 );
        }
      }*/
      if( stricmp( zork, "hide"  ) == 0 ) {
        WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_MINIMIZE ), 0 );
      }
      if( stricmp( zork, "float" ) == 0 ) {
        if( dork ) {
          if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
            if( cfg.floatontop ) {
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FLOAT ), 0 );
            }
          }
          if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
            if( !cfg.floatontop ) {
              WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( IDM_M_FLOAT ), 0 );
            }
          }
        }
      }
/*      if( stricmp( zork, "use" ) == 0 ) {
//          TODO: makes no more sense
//            amp_pl_use();
      }*/
/*      if( stricmp( zork, "clear" ) == 0 ) {
        // TODO: edit playlist
        //pl_clear( PL_CLR_NEW );
      }*/
/*      if( stricmp( zork, "next" ) == 0 ) {
        WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_NEXT ), 0 );
      }
      if( stricmp( zork, "previous" ) == 0 ) {
        WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PREV ), 0 );
      }*/
      /*if( stricmp( zork, "remove" ) == 0 ) {
        // TODO: pipe interface have to be renewed
        if( current_record ) {
          PLRECORD* rec = current_record;
          pl_remove_record( &rec, 1 );
        }
      }*/
/*      if( stricmp( zork, "forward" ) == 0 ) {
        WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_FWD  ), 0 );
      }
      if( stricmp( zork, "rewind" ) == 0 ) {
        WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_REW  ), 0 );
      }*/
      /*if( stricmp( zork, "stop" ) == 0 ) {
        WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_STOP ), 0 );
      }*/
/*      if( stricmp( zork, "jump" ) == 0 ) {
        if( dork ) {
          Ctrl::PostCommand(Ctrl::MkNavigate(xstring(), atof(dork), false, false));
        }
      }*/
/*      if( stricmp( zork, "play" ) == 0 ) {
        if( dork ) {
          // TODO: dir
          amp_load_playable( url123::normalizeURL(dork), 0, AMP_LOAD_NOT_PLAY );
          WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
        } else if( !decoder_playing()) {
          WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_PLAY ), 0 );
        }
      }*/
/*      if( stricmp( zork, "pause" ) == 0 ) {
        if( dork ) {
          if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
            Ctrl::PostCommand(Ctrl::MkPause(Ctrl::Op_Clear));
          }
          if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
            Ctrl::PostCommand(Ctrl::MkPause(Ctrl::Op_Set));
          }
        } else {
          Ctrl::PostCommand(Ctrl::MkPause(Ctrl::Op_Toggle));
        }
      }*/
      if( stricmp( zork, "playonload" ) == 0 ) {
        if( dork ) {
          if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
            cfg.playonload = FALSE;
          }
          if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
            cfg.playonload = TRUE;
          }
        }
      }
      if( stricmp( zork, "autouse" ) == 0 ) {
        if( dork ) {
          if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
            cfg.autouse = FALSE;
          }
          if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
            cfg.autouse = TRUE;
          }
        }
      }
      if( stricmp( zork, "playonuse" ) == 0 ) {
        if( dork ) {
          if( stricmp( dork, "off" ) == 0 || stricmp( dork, "0" ) == 0 ) {
            cfg.playonuse = FALSE;
          }
          if( stricmp( dork, "on"  ) == 0 || stricmp( dork, "1" ) == 0 ) {
            cfg.playonuse = TRUE;
          }
        }
      }
/*      if( stricmp( zork, "repeat"  ) == 0 ) {
        WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_REPEAT  ), 0 );
      }
      if( stricmp( zork, "shuffle" ) == 0 ) {
        WinSendMsg( hplayer, WM_COMMAND, MPFROMSHORT( BMP_SHUFFLE ), 0 );
      }*/
/*      if( stricmp( zork, "volume" ) == 0 )
      {
        if( dork )
        {
          bool relative = false;
          switch (*dork)
          {case '+':
            ++dork;
           case '-':
            relative = true;
          }
          // wait for command completion
          delete Ctrl::SendCommand(Ctrl::MkVolume(atof(dork), relative));
        }
        char buf[64];
        sprintf(buf, "%f", Ctrl::GetVolume());
        amp_pipe_write(hpipe, buf);
      }*/
#endif

/* Dispatches requests received from the pipe. */
// TODO: start the pipe thread!
static void TFNENTRY pipe_thread( void* )
{ DEBUGLOG(("pipe_thread()\n"));

  APIRET rc;
  HAB hab = WinInitialize( 0 );
  WinCreateMsgQueue( hab, 0 );

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
}


/****************************************************************************
* Public functions
****************************************************************************/

/* Create main pipe with only one instance possible since these pipe
   is almost all the time free, it wouldn't make sense having multiple
   intances. */
bool amp_pipe_create( void )
{
  ULONG rc;
  int   i = 1;

  while(( rc = DosCreateNPipe( Pipename, &HPipe,
                               NP_ACCESS_DUPLEX,
                               NP_WAIT|NP_TYPE_BYTE|NP_READMODE_BYTE | 1,
                               2048, 2048,
                               500 )) == ERROR_PIPE_BUSY )
  {
    sprintf( Pipename,"\\PIPE\\PM123_%d", ++i );
  }

  if( rc != 0 ) {
    amp_player_error( "Could not create pipe %s, rc = %d.", Pipename, rc );
    return false;
  }
  
  const url123& path = url123::normalizeURL(startpath);
  CurPlaylist = (PlayableCollection*)&*Playable::FindByURL(path + "PM123.LST");
  
  TIDWorker = _beginthread(pipe_thread, NULL, 65536, NULL);
  CASSERT(TIDWorker != -1);
  return true;
}

/* Shutdown the player pipe. */
void amp_pipe_destroy()
{ DosClose(HPipe);
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


