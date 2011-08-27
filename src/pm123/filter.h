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

#ifndef PM123_FILTER_H
#define PM123_FILTER_H

#include "plugman.h"
#include <filter_plug.h>
#include <vdelegate.h>

#include <cpp/smartptr.h>


/****************************************************************************
*
* filter interface
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

// Set of entry points related to filter plug-ins.
struct FilterProcs
{ void*  F;
  ULONG  DLLENTRYP(filter_init        )(void** f, FILTER_PARAMS2* params);
  void   DLLENTRYP(filter_update      )(void*  f, const FILTER_PARAMS2* params);
  BOOL   DLLENTRYP(filter_uninit      )(void*  f);
         FilterProcs()                 { memset(this, 0, sizeof *this); } // Uh, well, allowed for PODs
};

// specialized class for filter plug-ins
class Filter : public Plugin, protected FilterProcs
{private:
  VREPLACE1    VRStubs[6];
 protected:
  // instances of this class are only created by the factory function below.
  Filter(Module& mod)                  : Plugin(mod, PLUGIN_FILTER) {}
  /// Load the filter plug-in.
  /// @exception ModuleException Something went wrong.
  virtual void LoadPlugin();
 public:
  // No-op. Filters are initialized explicitely by calling initialize. Return TRUE.
  virtual bool InitPlugin();
  // Uninitialize the filter.
  virtual bool UninitPlugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual bool IsInitialized() const   { return F != NULL; }
  // Initialize the filter.
  // params: pointer to a FILTER_PARAMS2 structure.
  //         [in] Entry points of the downstream filter or output.
  //         [out] Entry points of this filter instance.
  // Return TRUE on success.
  virtual bool Initialize(FILTER_PARAMS2* params);
  /// Getter to the entry points of this plug-in.
  const FilterProcs& GetProcs() const { return *this; }

  /// Return the \c Filter instance on \a Module if it exists.
  /// Otherwise return \c NULL.
  static int_ptr<Filter> FindInstance(const Module& module);
  /// Return the \c Filter instance on \a module. Create a new one if required.
  /// @return the plug-in or NULL if it cannot be instantiated.
  static int_ptr<Filter> GetInstance(Module& module);
};


#endif
