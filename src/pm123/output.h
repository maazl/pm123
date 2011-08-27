/*
 * Copyright 2006-2011 M.Mueller
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

/* This is the core interface to the plug-ins. It loads the plug-ins and
 * virtualizes them if required to refect always the most recent interface
 * level to the application.
 */ 

#ifndef PM123_OUTPUT_H
#define PM123_OUTPUT_H

#include "plugman.h"
#include <output_plug.h>
#include <filter_plug.h>
#include <decoder_plug.h>
#include <visual_plug.h>
#include <vdelegate.h>

#include <cpp/smartptr.h>


/****************************************************************************
*
* output plug-in interface
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

// Set of entry points related to output plug-ins.
struct OutputProcs
{ void*  A;
  ULONG  DLLENTRYP(output_init           )(void** a);
  ULONG  DLLENTRYP(output_uninit         )(void*  a);
  ULONG  DLLENTRYP(output_command        )(void*  a, ULONG msg, OUTPUT_PARAMS2* info);
  int    DLLENTRYP(output_request_buffer )(void*  a, const TECH_INFO* format, short** buf);
  void   DLLENTRYP(output_commit_buffer  )(void*  a, int len, double posmarker);
  ULONG  DLLENTRYP(output_playing_samples)(void*  a, FORMAT_INFO* info, char* buf, int len);
  double DLLENTRYP(output_playing_pos    )(void*  a);
  BOOL   DLLENTRYP(output_playing_data   )(void*  a);
         OutputProcs()                    { memset(this, 0, sizeof *this); } // Uh, well, allowed for PODs
};

// specialized class for output plug-ins
class Output
: public    Plugin,
  protected OutputProcs
{private:
  static delegate<void, const PluginEventArgs> PluginDeleg;

 private:
  static void PluginNotification(void*, const PluginEventArgs& args);
 protected:
  // instances of this class are only created by the factory function below.
  Output(Module& mod)                 : Plugin(mod, PLUGIN_OUTPUT) {}
  /// Load the output plug-in.
  /// @exception ModuleException Something went wrong.
  virtual void LoadPlugin();
 public:
  // Initialize the output plug-in. Return TRUE on success.
  virtual bool InitPlugin();
  // Uninitialize the output plug-in. Return TRUE on success.
  virtual bool UninitPlugin();
  virtual bool IsInitialized() const  { return A != NULL; }
  /// Getter to the entry points of this plug-in.
  const OutputProcs& GetProcs() const { return *this; }

  /// Return the \c Output instance on \a Module if it exists.
  /// Otherwise return \c NULL.
  static int_ptr<Output> FindInstance(const Module& module);
  /// Return the \c Output instance on \a module. Create a new one if required.
  /// @return the plug-in or NULL if it cannot be instantiated.
  static int_ptr<Output> GetInstance(Module& module);

  // initialize global services
  static void  Init();
  static void  Uninit()               { PluginDeleg.detach(); }
};


#endif
