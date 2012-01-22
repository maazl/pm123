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
 *
 * This interface is used only by plugman.cpp!
 */ 

#ifndef PM123_VISUAL_H
#define PM123_VISUAL_H

#include "plugman.h"
#include <visual_plug.h>

#include <cpp/smartptr.h>
#include <memory.h>


typedef struct
{ /// coordinates of the visual plug-in
  int     x, y, cx, cy;
  /// Plug-in is loaded by a skin
  bool    skin;
  /// Plug-in parameters
  xstring param;
} VISUAL_PROPERTIES;

/****************************************************************************
*
* visualization interface
*
* This class is thread-safe on per instance basis.
*
****************************************************************************/

/// Set of entry points related to visual plug-ins.
struct VisualProcs
{ HWND   DLLENTRYP(plugin_init  )(const VISPLUGININIT* init);
  BOOL   DLLENTRYP(plugin_deinit)(int unload);
         VisualProcs()              { memset(this, 0, sizeof *this); } // Uh, well, allowed for PODs
};

/// Specialized class for visual plug-ins
class Visual : public Plugin, protected VisualProcs
{protected:
  static const PLUGIN_PROCS VisualCallbacks;
  /// Properties of this plug-in when it is part of a skin.
  VISUAL_PROPERTIES Props;
  /// Window handle of the visual plug-in.
  HWND              Hwnd;

 protected:
  /// instances of this class are only created by the factory function below.
  Visual(Module& mod)                 : Plugin(mod, PLUGIN_VISUAL), Hwnd(NULLHANDLE) { memset(&Props, 0, sizeof Props); }
  /// Load the visual plug-in. Return TRUE on success.
  /// @exception ModuleException Something went wrong.
  void         LoadPlugin();
 public:
  virtual ~Visual();
  /// Initialize the plug-in.
  /// @param hwnd PM123 window handle
  /// @return true on success
  virtual bool InitPlugin(HWND owner);
  /// Uninitialize the plug-in.
  virtual bool UninitPlugin();
  /// Implementation of IsInitialized.
  virtual bool IsInitialized() const  { return Hwnd != NULLHANDLE; }
  /*/// Initialize all visual plug-ins
  static void InitAll(HWND owner);
  /// Uninitialize all visual plug-ins
  static void UninitAll();*/
  /// Send a window message to all initialized visual plug-ins.
  static void BroadcastMsg(ULONG msg, MPARAM mp1, MPARAM mp2);
  /// Getter to the properties of this plug-in.
  const VISUAL_PROPERTIES& GetProperties() const { return Props; }
  /// Setter to the properties of this plug-in instance.
  virtual void SetProperties(const VISUAL_PROPERTIES* data);
  
  /// Return the \c Visual instance on \a Module if it exists.
  /// Otherwise return \c NULL.
  static int_ptr<Visual> FindInstance(const Module& module);
  /// Factory. Return the \c Visual instance on \a module. Create a new one if required.
  /// @exception ModuleException Something went wrong.
  /// @remarks This function must be called from thread 1.
  static int_ptr<Visual> GetInstance(Module& module);
};


#endif
