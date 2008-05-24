/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef  PIPE_H
#define  PIPE_H

#include <config.h>
#include <stdlib.h>
#include <cpp/xstring.h>
#include "playablecollection.h"


/* Create main pipe with only one instance possible since these pipe
   is almost all the time free, it wouldn't make sense having multiple
   intances. */
bool amp_pipe_create();
/* Shutdown the player pipe. */
void amp_pipe_destroy();

/* Opens specified pipe and writes data to it. */
bool amp_pipe_open_and_write( const char* pipename, const char* data, size_t size );


/* Class to execute pipe commands with a local context. */
class CommandProcessor
{private:
  static const struct CmdEntry
  { char Prefix[12];
    void (CommandProcessor::*ExecFn)(xstring& ret, char* args);
  } CmdList[];

 private:
  int_ptr<PlayableCollection> CurPlaylist; // playlist where we currently operate
  int_ptr<PlayableInstance>   CurItem;     // current item of the above playlist
 private:
  void CmdLoad(xstring& ret, char* args);
  void CmdPlay(xstring& ret, char* args);
  void CmdStop(xstring& ret, char* args);
  void CmdPause(xstring& ret, char* args);
  void CmdNext(xstring& ret, char* args);
  void CmdPrev(xstring& ret, char* args);
  void CmdRewind(xstring& ret, char* args);
  void CmdForward(xstring& ret, char* args);
  void CmdJump(xstring& ret, char* args);
  void CmdVolume(xstring& ret, char* args);
  void CmdShuffle(xstring& ret, char* args);
  void CmdRepeat(xstring& ret, char* args);
  void CmdStatus(xstring& ret, char* args);
  void CmdHide(xstring& ret, char* args);
  // PLAYLIST
  void CmdPlaylist(xstring& ret, char* args);
  void CmdPlNext(xstring& ret, char* args);
  void CmdPlPrev(xstring& ret, char* args);
  void CmdPlReset(xstring& ret, char* args);
  void CmdUse(xstring& ret, char* args);
  void CmdClear(xstring& ret, char* args);
  void CmdRemove(xstring& ret, char* args);
  void CmdAdd(xstring& ret, char* args);
  void CmdDir(xstring& ret, char* args);
  void CmdRdir(xstring& ret, char* args);
  // CONFIGURATION
  void CmdSize(xstring& ret, char* args);
  void CmdFont(xstring& ret, char* args);
  void CmdFloat(xstring& ret, char* args);
  void CmdAutouse(xstring& ret, char* args);
  void CmdPlayonload(xstring& ret, char* args);
  void CmdPlayonuse(xstring& ret, char* args);
 public:
  CommandProcessor();
  // Executes the Command cmd and return a value in ret.
  // Note that cmd is mutable. The buffer content will be destroyed.
  void Execute(xstring& ret, char* cmd);
  // Same as above, but copies the command buffer first.
  void Execute(xstring& ret, const char* cmd);
};

#endif

