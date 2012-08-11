/*
 * Copyright 2007-2011 M.Mueller
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

#ifndef  LOADHELPER_H
#define  LOADHELPER_H

#define INCL_WIN
#include <cpp/smartptr.h>

#include "controller.h"
#include <os2.h>


/** Helper class to load one or more objects into PM123.
 */
class LoadHelper
{public:
  enum Options
  { LoadDefault      = 0,
    LoadPlay         = 0x01, // Start Playing when completed
    LoadRecall       = 0x02, // Add item to the MRU-List if it is only one
    LoadAppend       = 0x04, // Always append to the default playlist
    //LoadKeepPlaylist = 0x08, // Play a playable object. If A playlist containing this item is loaded, the item is activated only.
    //AutoPost         = 0x20, // Auto post command if the class instance is destroyed.
    //PostGUI          = 0x40  // Do the job by the main message queue. This implies that errors are shown to the user.
  };
 protected:
  const Options         Opt;
  vector_int<APlayable> Items;
 protected:
  /// Returns /one/ APlayable that represents all the objects in the list.
  APlayable*            ToAPlayable();
 public:
  /// Initialize LoadHelper
                        LoadHelper(Options opt);
  //                      ~LoadHelper();
  /// Add a item to play to the list of items.
  void                  AddItem(APlayable& ps) { Items.append() = &ps; }
  /// Create a sequence of controller commands from the current list.
  Ctrl::ControlCommand* ToCommand();
  /// Send command to the controller and wait for reply.
  Ctrl::RC              SendCommand();
};
FLAGSATTRIBUTE(LoadHelper::Options);

#endif

