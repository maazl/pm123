/*
 * Copyright 2006-2008 M.Mueller
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

#ifndef _PM123_PLUGMAN_BASE_H
#define _PM123_PLUGMAN_BASE_H

#include "plugman.h"
#include <vdelegate.h>

#include <cpp/smartptr.h>


/* Define the macro NOSYSTEMSTATICMEMBER to work around for the IBMVAC++ restriction
 * that static class functions may not have a defined calling convention.
 * However, this workaround relies on friend functions with static linkage. This is
 * invalid acording to the C++ standard, but IBMVAC++ does not care about that fact.
 */
#ifdef NOSYSTEMSTATICMEMBER
#define PROXYFUNCDEF friend static
#define PROXYFUNCIMP(ret, cls) static ret
#define PROXYFUNCREF(cls)
#else
#define PROXYFUNCDEF static
#define PROXYFUNCIMP(ret, cls) ret cls::
#define PROXYFUNCREF(cls) cls::
#endif


/****************************************************************************
*
* Class tree:
*
* PLUGIN_BASE (POD)
* +- CL_PLUGIN_BASE (POD)
*    +- CL_MODULE  
*    +- CL_PLUGIN (references CL_MODULE n:1)
*       +- CL_DECODER
*       |  +- proxy classes ...
*       +- CL_OUTPUT
*       |  +- proxy classes ...
*       +- CL_FILTER
*       |  +- proxy classes ...
*       +- CL_VISUAL
*          +- proxy classes ...
*
* CL_PLUGIN_BASE_LIST (contains CL_PLUGIN_BASE 1:n)
* +- CL_MODULE_LIST (contains CL_MODULE 1:n)
* +- CL_PLUGIN_LIST (contains CL_PLUGIN 1:n)
*    +- CL_PLUGIN_LIST1 (list with exactly one active plugin)
*
****************************************************************************/


/* Buffer size for compatibility interface */
#define  BUFSIZE 16384


/****************************************************************************
*
* Decoder plug-in interface
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

// Set of entry points related to decoder plug-ins.
struct DecoderProcs
{ void*  W;
  int    DLLENTRYP(decoder_init     )( void** w );
  BOOL   DLLENTRYP(decoder_uninit   )( void*  w );
  ULONG  DLLENTRYP(decoder_command  )( void*  w, ULONG msg, DECODER_PARAMS2* params );
  void   DLLENTRYP(decoder_event    )( void*  w, OUTEVENTTYPE event );
  ULONG  DLLENTRYP(decoder_status   )( void*  w );
  double DLLENTRYP(decoder_length   )( void*  w );
  ULONG  DLLENTRYP(decoder_fileinfo )( const char* filename, INFOTYPE* what, DECODER_INFO2* info );
  ULONG  DLLENTRYP(decoder_setmeta  )( const char* filename, const META_INFO* meta );
  ULONG  DLLENTRYP(decoder_cdinfo   )( const char* drive, DECODER_CDINFO* info );
  ULONG  DLLENTRYP(decoder_editmeta )( HWND owner, const char* url );
  const  DECODER_WIZZARD*
         DLLENTRYP(decoder_getwizzard)( );
  // Init structure
         DecoderProcs()              { memset(this, 0, sizeof *this); } // Uh, well, allowed for PODs
};

// specialized class for decoders
class Decoder
: public Plugin,
  protected DecoderProcs
{protected:
  // Result from the decoder_support call. Supported data sources.
  int          Type;
  // Result from the decoder_support call. Supported file extensions.
  xstring      Extensions;
  // Result from the decoder_support call. Supported file types.
  xstring      FileTypes;
  // Additional supported file types.
  xstring      AddFileTypes;
 public:
  // Try unsupported file extensions and types too.
  bool         TryOthers;
 private:
  PROXYFUNCDEF ULONG DLLENTRY stub_decoder_cdinfo( const char* drive, DECODER_CDINFO* info );
 protected:
  // instances of this class are only created by the factory function below.
               Decoder(Module* mod)  : Plugin(mod), TryOthers(false) {}
  // Get some information about the decoder. (decoder_support)
  virtual bool AfterLoad();
 public:
  //virtual      ~Decoder();
  // Return kind of Plugin handled by the class instance. (RTTI by the backdoor.)
  virtual PLUGIN_TYPE GetType() const;
  // Load the plug-in that is identified as a decoder. Return TRUE on success.
  virtual bool LoadPlugin();
  // Initialize the decoder. Return TRUE on success.
  virtual bool InitPlugin();
  // Uninitialize the decoder. Return TRUE on success.
  virtual bool UninitPlugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual bool IsInitialized() const   { return W != NULL; }
  // Return a combination of DECODER_* flags to determine what kind of objects are supported.
  int          GetObjectTypes() const  { return Type; }
  // Getter to the decoder entry points.
  const DecoderProcs& GetProcs() const { return *this; }
  // Get supported extensions or NULL
  const xstring& GetExtensions() const { return Extensions; }
  // Get Supported EA types or NULL
  xstring      GetFileTypes() const;
  // Checks wether a decoder claims to support a certain URL.
  bool         IsFileSupported(const char* url) const;
  // Overloaded for parameter recognition
  virtual void GetParams(stringmap& map) const;
  virtual bool SetParam(const char* param, const xstring& value);
 private:
  bool         DoFileTypeMatch(USHORT type, const USHORT*& eadata) const;

 public:  
  // Factory function to create a new decoder instance from a module object.
  // This function will perform all necessary tasks to load a decoder.
  // It will especially virtualize decoders with an older interface level.
  // The returned object must be deleted if it is no longer used.
  static Plugin* Factory(Module* mod);
  
  // initialize global services
  static void  Init();
  static void  Uninit()                {}
};


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
  ULONG  DLLENTRYP(output_init           )( void** a );
  ULONG  DLLENTRYP(output_uninit         )( void*  a );
  ULONG  DLLENTRYP(output_command        )( void*  a, ULONG msg, OUTPUT_PARAMS2* info );
  int    DLLENTRYP(output_request_buffer )( void*  a, const FORMAT_INFO2* format, short** buf );
  void   DLLENTRYP(output_commit_buffer  )( void*  a, int len, double posmarker );
  ULONG  DLLENTRYP(output_playing_samples)( void*  a, FORMAT_INFO* info, char* buf, int len );
  double DLLENTRYP(output_playing_pos    )( void*  a );
  BOOL   DLLENTRYP(output_playing_data   )( void*  a );
         OutputProcs()                    { memset(this, 0, sizeof *this); } // Uh, well, allowed for PODs
};

// specialized class for output plug-ins
class Output
: public    Plugin,
  protected OutputProcs
{protected:
  // instances of this class are only created by the factory function below.
               Output(Module* mod)        : Plugin(mod) { Enabled = true; }
 public:
  // Return kind of Plugin handled by the class instance. (RTTI by the backdoor.)
  virtual PLUGIN_TYPE GetType() const;
  // Load the output plug-in. Return TRUE on success.
  virtual bool LoadPlugin();
  // Initialize the output plug-in. Return TRUE on success.
  virtual bool InitPlugin();
  // Uninitialize the output plug-in. Return TRUE on success.
  virtual bool UninitPlugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual bool IsInitialized() const      { return A != NULL; }
  // Getter to the entry points.
  const OutputProcs& GetProcs() const     { return *this; }

  // Factory function to create a new output plug-in instance from a module object.
  // This function will perform all necessary tasks to load the plug-in.
  // It will especially virtualize plug-ins with an older interface level.
  // The returned object must be deleted if it is no longer used.
  static Plugin* Factory(Module* mod);

  // initialize global services
  static void  Init();
  static void  Uninit()                   {}
};


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
  ULONG  DLLENTRYP(filter_init        )( void** f, FILTER_PARAMS2* params );
  void   DLLENTRYP(filter_update      )( void*  f, const FILTER_PARAMS2* params );
  BOOL   DLLENTRYP(filter_uninit      )( void*  f );
         FilterProcs()                 { memset(this, 0, sizeof *this); } // Uh, well, allowed for PODs
};

// specialized class for filter plug-ins
class Filter : public Plugin, protected FilterProcs
{private:
  VREPLACE1    VRStubs[6];
 protected:
  // instances of this class are only created by the factory function below.
               Filter(Module* mod)     : Plugin(mod) {}
 public:
  // Return kind of Plugin handled by the class instance. (RTTI by the backdoor.)
  virtual PLUGIN_TYPE GetType() const;
  // Load the filter plug-in. Return TRUE on success.
  virtual bool LoadPlugin();
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
  // Getter to the entry points.
  const FilterProcs& GetProcs() const  { return *this; }

  // Factory function to create a new filter plug-in instance from a module object.
  // This function will perform all necessary tasks to load the plug-in.
  // It will especially virtualize plug-ins with an older interface level.
  // The returned object must be deleted if it is no longer used.
  static Plugin* Factory(Module* mod);
};


/****************************************************************************
*
* visualization interface
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

// Set of entry points related to visual plug-ins.
struct VisualProcs
{ HWND   DLLENTRYP(plugin_init     )( VISPLUGININIT* init );
  BOOL   DLLENTRYP(plugin_deinit   )( int unload );
         VisualProcs()              { memset(this, 0, sizeof *this); } // Uh, well, allowed for PODs
};

// specialized class for visual plug-ins
class Visual : public Plugin, protected VisualProcs
{protected:
  // Properties of this plug-in when it is part of a skin.
  VISUAL_PROPERTIES Props;
  // Window handle of the visual plug-in.
  HWND              Hwnd;

 protected:
  // instances of this class are only created by the factory function below.
               Visual(Module* mod)    : Plugin(mod), Hwnd(NULLHANDLE) {}
 public:
  // Return kind of Plugin handled by the class instance. (RTTI by the backdoor.)
  virtual PLUGIN_TYPE GetType() const;
  // Load the visual plug-in. Return TRUE on success.
  virtual bool LoadPlugin();
  // No-op. Filters are initialized explicitely by calling initialize. Return TRUE.
  virtual bool InitPlugin();
  // Uninitialize the plug-in.
  virtual bool UninitPlugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual bool IsInitialized() const  { return Hwnd != NULLHANDLE; }
  // Initialize the plug-in.
  // hwnd:  PM123 window handle
  // procs: procedure entry points for the visual plug-in
  // id:    plug-in number
  // Return TRUE on success
  virtual bool Initialize(HWND hwnd, PLUGIN_PROCS* procs, int id);
  // Getter to the properties of this plug-in.
  const VISUAL_PROPERTIES& GetProperties() const { return Props; }
  // Setter to the properties of this plug-in instance.
  virtual void SetProperties(const VISUAL_PROPERTIES* data);
  // Getter to the entry points of this plug-in.
  const VisualProcs& GetProcs() const { return *this; }
  // Get the plug-in's window handle.
  HWND         GetHwnd() const        { return Hwnd; }
  
  // Factory function to create a new visual plug-in instance from a module object.
  // This function will perform all necessary tasks to load the plug-in.
  // It will especially virtualize plug-ins with an older interface level.
  // The returned object must be deleted if it is no longer used.
  static Plugin* Factory(Module* mod);
};


#endif
