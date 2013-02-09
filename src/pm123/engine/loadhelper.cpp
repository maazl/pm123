/*
 * Copyright 2007-2012 M.Mueller
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


#include "loadhelper.h"
#include "../configuration.h"
#include "../gui/gui.h" // for DefaultPl


/****************************************************************************
*
*  class LoadHelper
*
****************************************************************************/

LoadHelper::LoadHelper(Options opt)
: Opt(opt)
, Items(20)
{}

APlayable* LoadHelper::ToAPlayable()
{ switch (Items.size())
  {case 0:
    return NULL;
   case 1:
    if (Opt & LoadRecall)
      GUI::Add2MRU(GUI::GetLoadMRU(), Cfg::Get().max_recall, *Items[0]);
    if (!(Opt & LoadAppend))
      return Items[0];
   default:
    // Load multiple items => Use default playlist
    Playable& pl = GUI::GetDefaultPL();
    if (!(Opt & LoadAppend))
      pl.Clear();
    for (const int_ptr<APlayable>* ppps = Items.begin(); ppps != Items.end(); ++ppps)
      pl.InsertItem(**ppps);
    return &pl;
  }
}

Ctrl::ControlCommand* LoadHelper::ToCommand()
{ int_ptr<APlayable> ps(ToAPlayable());
  if (ps == NULL)
    return NULL;
  // TODO: LoadKeepPlaylist
  Ctrl::ControlCommand* cmd = Ctrl::MkLoad(ps, true);
  Ctrl::ControlCommand* tail = cmd;
  tail = tail->Link = Ctrl::MkSkip(0, true);
  if (Opt & LoadPlay)
    // Start playback immediately after loading has completed
    tail = tail->Link = Ctrl::MkPlayStop(Ctrl::Op_Set);
  return cmd;
}

Ctrl::RC LoadHelper::SendCommand()
{ Ctrl::ControlCommand* cmd = ToCommand();
  if (!cmd)
    return Ctrl::RC_OK;
  Items.clear();
  // Send command
  Ctrl::ControlCommand* cmde = Ctrl::SendCommand(cmd);
  // Calculate result
  Ctrl::RC rc;
  do
    rc = (Ctrl::RC)cmde->Flags;
  while (rc == Ctrl::RC_OK && (cmde = cmde->Link) != NULL);
  // cleanup
  cmd->Destroy();
  return rc;
}
