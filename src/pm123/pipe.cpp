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
#include "pipe.h"
#include "acommandprocessor.h"
#include "configuration.h"
#include "eventhandler.h"


#define PIPE_BUFFER_SIZE 65536


/****************************************************************************
* Pipe interface
****************************************************************************/

static HPIPE HPipe        = NULLHANDLE;
static TID   TIDWorker    = (TID)-1;


/* Dispatches requests received from the pipe. */
static void TFNENTRY pipe_thread(void*)
{ DEBUGLOG(("pipe_thread()\n"));

  HAB hab = WinInitialize(0);
  HMQ hmq = WinCreateMsgQueue(hab, 0);

  sco_ptr<ACommandProcessor> cmdproc(ACommandProcessor::Create());
  sco_arr<char> buffer(PIPE_BUFFER_SIZE);

  for(;; DosDisConnectNPipe( HPipe ))
  { // Connect the pipe
    APIRET rc = DosConnectNPipe( HPipe );
    if (rc != NO_ERROR)
    { DEBUGLOG(("pipe_thread: DosConnectNPipe failed with rc = %u\n", rc));
      break;
    }

    // read a command
    ULONG bytesread;
   nextcommand:
    rc = DosRead(HPipe, buffer.get(), PIPE_BUFFER_SIZE-1, &bytesread);
    if (rc != NO_ERROR)
    { DEBUGLOG(("pipe_thread: DosRead failed with rc = %u\n", rc));
      continue;
    }
    if (bytesread == 0)
      continue;
    buffer[bytesread] = 0; // ensure terminating zero

    // execute commands
    char* cp = buffer.get();
    for (;;)
    { size_t len = strcspn(cp, "\r\n");
      DEBUGLOG(("pipe_thread: at %s, %u, %i, %u\n", cp, bytesread, cp-buffer.get(), len));
      if (cp + len == buffer.get() + bytesread)
      { // Command string goes to the end of the buffer.
        // This implies that there was no terminating newline in the buffer.
        // => Try to read the remaining part of the command.
        // But firstly move the command to the beginning of the buffer if required.
        if (cp != buffer.get())
        { memmove(buffer.get(), cp, len+1);
          bytesread -= cp - buffer.get();
          cp = buffer.get();
        }
        ULONG bytesread2;
        rc = DosRead(HPipe, buffer.get() + bytesread, PIPE_BUFFER_SIZE - bytesread -1, &bytesread2);
        if (rc == NO_ERROR)
        { if (bytesread2)
          { bytesread += bytesread2;
            buffer[bytesread] = 0; // ensure terminating zero
            continue;
          }
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
      { if (*cp2 != '*')
        { // immediate load command
          if (cp2 < buffer.get() + 7)
          { // make room for the command string
            if (bytesread > PIPE_BUFFER_SIZE - 8) // avoid buffer overflow
              buffer[bytesread = PIPE_BUFFER_SIZE - 8] = 0;
            memmove(buffer.get() + 7, cp2, bytesread + 1);
            cp2 = buffer.get();
          } else
            cp2 -= 7;
          memcpy(cp2, "invoke ", 7);
        } else
          ++cp2;
        const char* ret = cmdproc->Execute(cp2);
        DEBUGLOG(("pipe_thread: command done: %s\n", ret));
        // send reply
        ULONG actual = strlen(ret);
        DosWrite(HPipe, (PVOID)ret, actual, &actual);
        DosWrite(HPipe, "\n\n", ret[actual-1] != '\n' ? 2 : 1, &actual);
        DosResetBuffer(HPipe);
      }
      // next line
      cp += len +1; // and skip delimiter
      if (cp >= buffer.get() + bytesread)
        goto nextcommand;
    }
  }

  WinDestroyMsgQueue(hmq);
  WinTerminate(hab);
}


/****************************************************************************
* Public functions
****************************************************************************/

/* Create main pipe with only one instance possible since these pipe
   is almost all the time free, it wouldn't make sense having multiple
   intances. */
bool amp_pipe_create( void )
{ DEBUGLOG(("amp_pipe_create() - %p\n", HPipe));
  xstring pipe_name(Cfg::Get().pipe_name);
  ULONG rc = DosCreateNPipe( pipe_name, &HPipe,
                             NP_ACCESS_DUPLEX, NP_WAIT|NP_TYPE_BYTE|NP_READMODE_BYTE | 1,
                             2048, 2048, 500 );

  if (rc != 0 && rc != ERROR_PIPE_BUSY)
  { EventHandler::PostFormat(MSG_WARNING, "Could not create pipe %s, rc = %d.", pipe_name.cdata(), rc);
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

bool amp_pipe_check()
{ APIRET rc = DosWaitNPipe(xstring(Cfg::Get().pipe_name), 50);
  DEBUGLOG(("amp_pipe_check() : %lu\n", rc));
  return rc == NO_ERROR || rc == ERROR_PIPE_BUSY;
}

/* Opens specified pipe and writes data to it. */
bool amp_pipe_open_and_write(const char* pipename, const char* data, size_t size)
{
  HPIPE  hpipe;
  ULONG  action;
  APIRET rc;

  rc = DosOpen((PSZ)pipename, &hpipe, &action, 0, FILE_NORMAL,
               OPEN_ACTION_FAIL_IF_NEW |OPEN_ACTION_OPEN_IF_EXISTS,
               OPEN_SHARE_DENYREADWRITE|OPEN_ACCESS_READWRITE|OPEN_FLAGS_FAIL_ON_ERROR,
               NULL);
  if (rc == NO_ERROR)
  { DosWrite(hpipe, (PVOID)data, size, &action);
    DosResetBuffer(hpipe);
    DosDisConnectNPipe(hpipe);
    DosClose(hpipe);
    return true;
  } else {
    return false;
  }
}
