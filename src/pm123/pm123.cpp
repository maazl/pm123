/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Leppä <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2011 Marcel Müller
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

#define  INCL_WIN
#define  INCL_GPI
#define  INCL_DOS
#define  INCL_DOSERRORS
#define  INCL_WINSTDDRAG
#include <os2.h>

//#undef DEBUG_LOG
//#define DEBUG_LOG 2

#include "pm123.h"
#include "123_util.h"
#include "plugman.h"
#include "button95.h"
#include "gui.h"
#include "properties.h"
#include "controller.h"
#include "loadhelper.h"
#include "pipe.h"

#include <fileutil.h>
#include <utilfct.h>

#include <stdio.h>

#include <debuglog.h>


// Cache lifetime of unused playable objects in seconds
// Objects are removed after [CLEANUP_INTERVALL, 2*CLEANUP_INTERVALL].
// However, since the romoved objects may hold references to other objects,
// only one generation is cleand up per time.
#define  CLEANUP_INTERVALL 10

/* file dialog additional flags */
#define  FDU_DIR_ENABLE   0x0001
#define  FDU_RECURSEBTN   0x0002
#define  FDU_RECURSE_ON   0x0004
#define  FDU_RELATIVBTN   0x0008
#define  FDU_RELATIV_ON   0x0010


static xstring StartPath;
const xstring& amp_startpath = StartPath;

static xstring BasePath;
const xstring& amp_basepath = BasePath;

static HAB Hab = NULLHANDLE;
const HAB& amp_player_hab = Hab;


void amp_fail(const char* fmt, ...)
{ // TODO: Write error to PM if initialized.
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
  exit(1);
}

static char* fetcharg(char**& argv, const char* opt)
{ if (!*++argv)
    amp_fail("The option -%s requires an argument.", opt);
  return *argv;
}

int main(int argc, char** argv)
{
  // used for debug printf()s
  setvbuf(stderr, NULL, _IONBF, 0);
  { // init startpath
    char exename[_MAX_PATH];
    getExeName(exename, sizeof exename);
    char* cp = strrchr(exename, '\\');
    ASSERT(cp);
    StartPath.assign(exename, cp-exename+1);
  }

  // parse command line
  vector<const char> files;
  const char* configdir = NULL;
  const char* pipename  = NULL;
  bool        shuffle   = false;
  bool        start     = false;
  bool        nogui     = false;
  bool        command   = false; // files are remote command
  bool        nomoreopt = false;
  while (*++argv) // The first item is always the executable itself
  { char* arg = *argv;

    if ((arg[0] == '-' || arg[0] == '/') && !nomoreopt)
    { // option
      ++arg;
      if (strcmp(arg, "-") == 0)
        nomoreopt = true;
      else if (stricmp(arg, "configdir") == 9)
        configdir = fetcharg(argv, "configdir");
      else if (stricmp(arg, "pipe") == 0)
        pipename = fetcharg(argv, "pipe");
      else if (stricmp(arg, "start") == 0)
        start = true;
      else if (stricmp(arg, "nugui") == 0)
        nogui = true;
      else if (stricmp(arg, "shuffle") == 0)
        shuffle = true;
      else if (stricmp(arg, "cmd") == 0)
      { if (files.size())
          amp_fail("The -cmd option could not be used after an URL: %s", files[0]);
        files.append() = fetcharg(argv, "cmd");
        while(*argv)
          files.append() = arg;
        command = true;
        break;
      } else if (stricmp(arg, "smooth") == 0)
        ;// Not supported since 1.32
      else
        amp_fail("invalid option: %s", arg-1);

    } else
      files.append() = arg;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Initialization of infrastructure
  ///////////////////////////////////////////////////////////////////////////
  srand((unsigned long)time( NULL ));

  if (configdir)
  { // TODO: make path absolute
    size_t len = strlen(configdir);
    if (configdir[len-1] != '\\')
    { // append '\'
      char* cp = BasePath.allocate(len+1, configdir);
      cp[len] = '\\';
    } else
      BasePath = configdir;
  } else
    BasePath = StartPath;

  // now init PM
  Hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(Hab, 0);
  PMASSERT(hmq != NULLHANDLE);

  // initialize properties
  cfg_init();
  if (pipename)
    cfg.pipe_name = pipename;

  // Command line args?
  if (files.size())
  { xstringbuilder cmd;
    if (command)
      cmd.append('*');
    const char*const*const epp = files.end();
    const char*const* spp = files.begin();
    cmd.append(*spp);
    while (++spp != epp)
    { cmd.append(' ');
      cmd.append(*spp);
    }
    // Pipe command
    if (amp_pipe_open_and_write(cfg.pipe_name, cmd.cdata(), cmd.length()))
      return 0;
    if (command && !start)
      amp_fail("Cannot write command to pipe %s.", cfg.pipe_name.cdata());
  }
  else if (start && amp_pipe_check())
    return 0;

  // prepare plug-in manager
  plugman_init();

  Playable::Init();

  // start controller
  Ctrl::Init();
  if (shuffle)
    Ctrl::PostCommand(Ctrl::MkShuffle(Ctrl::Op_Set));

  // start GUI
  InitButton(Hab);
  GUI::Init();

  if (!amp_pipe_create() && start)
    WinPostMsg(GUI::GetFrameWindow(), WM_QUIT, 0, 0);
  // Now it is time to show the window
  else if (!nogui)
    GUI::Show();

  DEBUGLOG(("main: init complete\n"));

  if (files.size())
  { // Files passed at command line => load them initially.
    // TODO: do not load last used file in this case!
    const url123& cwd = amp_get_cwd();
    LoadHelper* lhp = new LoadHelper(cfg.playonload*LoadHelper::LoadPlay | LoadHelper::LoadRecall);
    const char*const*const epp = files.end();
    const char*const* spp = files.begin();
    do
    { const url123& url = cwd.makeAbsolute(*spp);
      if (!url)
        GUI::ViewMessage(xstring::sprintf("Invalid file or URL: '%s'", *spp), true);
      else
        lhp->AddItem(Playable::GetByURL(url));
    } while (++spp != epp);
    GUI::Load(lhp);
  }

  ///////////////////////////////////////////////////////////////////////////
  // Main loop
  ///////////////////////////////////////////////////////////////////////////
  QMSG   qmsg;
  while (WinGetMsg(Hab, &qmsg, (HWND)0, 0, 0))
  { DEBUGLOG2(("main: after WinGetMsg - %p, %p\n", qmsg.hwnd, qmsg.msg));
    WinDispatchMsg(Hab, &qmsg);
    DEBUGLOG2(("main: after WinDispatchMsg - %p, %p\n", qmsg.hwnd, qmsg.msg));
  }
  DEBUGLOG(("main: dispatcher ended\n"));
  
  amp_pipe_destroy();

  ///////////////////////////////////////////////////////////////////////////
  // Stop and save configuration
  ///////////////////////////////////////////////////////////////////////////
  GUI::Uninit();

  // TODO: save volume
  Ctrl::Uninit();

  save_ini();

  ///////////////////////////////////////////////////////////////////////////
  // Uninitialize infrastructure
  ///////////////////////////////////////////////////////////////////////////
  Playable::Uninit();
  plugman_uninit();
  cfg_uninit();

  WinDestroyMsgQueue(hmq);
  WinTerminate(Hab);

  return 0;
}

#ifdef __DEBUG_ALLOC__
static struct MyTerm
{ ~MyTerm();
} MyTermInstance;

MyTerm::~MyTerm()
{ _dump_allocated( 0 );
}
#endif

