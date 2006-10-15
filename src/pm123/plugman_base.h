/*
 * Copyright 2006 M.Mueller
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
 * This interface is used only by plugman.cpp
 */ 

#ifndef _PM123_PLUGMAN_BASE_H
#define _PM123_PLUGMAN_BASE_H

#include "plugman.h"
#include "vdelegate.h"


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
* CL_PLUGIN_BASE - C++ version of PLUGIN_BASE
*
* This is still a POD and therefore binary compatible to PLUGIN_BASE.
*
****************************************************************************/

class CL_PLUGIN_BASE : public PLUGIN_BASE
{protected:
  // Load the address of a dll entry point. Return TRUE on success.
  BOOL load_optional_function(void* function, const char* function_name);
  // Same as abobe, but raise amp_player_error in case of an error.
  BOOL load_function(void* function, const char* function_name);
};

/****************************************************************************
*
* CL_MODULE - object representing a plugin-DLL
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

class CL_PLUGIN;

class CL_MODULE : public CL_PLUGIN_BASE
{private:
  // internal reference counter
  int  refcount;
  // Entry point of the configure dialog (if any).
  void (PM123_ENTRYP plugin_configure)(HWND hwnd, HMODULE module);
 private:
  CL_MODULE(const CL_MODULE&); // non-existent, avoid copy construction
  // Load the DLL
  BOOL load_module();
  // Unload the DLL
  BOOL unload_module();
 public:
  // Create a MODULE object from the modules file name.
  CL_MODULE(const char* name);
  ~CL_MODULE();
  // Load a new DLL as plug-in. The plug-in flavour is specialized later.
  // Returns TRUE on success, otherwise return FALSE and call amp_player_error.
  BOOL load();
  // Launch the configure dialog.
  void config(HWND hwnd) const       { (*plugin_configure)(hwnd, module); }
  // For internal use by CL_PUGIN only!
  // Attach a new plug-in to this module.
  void attach(const CL_PLUGIN* inst) { ++refcount; }
  // For internal use by CL_PUGIN only!
  // Detach a plug-in from this module.
  BOOL detach(const CL_PLUGIN* inst) { return --refcount == 0; }
};


/****************************************************************************
*
* CL_PLUGIN - abstract object representing a plugin instance
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

class CL_PLUGIN : public CL_PLUGIN_BASE
{protected:
  // reference to the underlying module.
  CL_MODULE& modref;
  // Enabled flag. TRUE in doubt.
  BOOL enabled;
 private:
  CL_PLUGIN(const CL_PLUGIN&); // non-existent, avoid copy construction
 protected:
  // instanciate a new plug-in.
  // The properties of the underlying POD PLUGIN_BASE are copied 
  CL_PLUGIN(CL_MODULE& mod);
 public:
  // Destroy the current plug-in. This will implicitely deregister it from
  // the plug-in list "plugins". If the module is not used by another plug-in
  // the module will be unloaded.
  virtual ~CL_PLUGIN();
  // Load the current plug-in. Return TRUE on success.
  // To be implemented by the particular plug-in flavour.
  virtual BOOL load_plugin() = 0;
  // Initialize the current plug-in. Return TRUE on success.
  // To be implemented by the particular plug-in flavour.
  virtual BOOL init_plugin() = 0;
  // Uninitialize the current plug-in. Return TRUE on success.
  // To be implemented by the particular plug-in flavour.
  virtual BOOL uninit_plugin() = 0;
  // Tell whether the plug-in is currently initialized.
  // To be implemented by the particular plug-in flavour.
  virtual BOOL is_initialized() const = 0;
  // Getter to the underlying module.
  CL_MODULE&   get_module() const  { return modref; }
  // Getter to the enabled state.
          BOOL get_enabled() const { return enabled; }
  // Setter to the enabled state.
  virtual void set_enabled(BOOL enabled);
};


/****************************************************************************
*
* Decoder plug-in interface
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

// Set of entry points related to decoder plug-ins.
struct DECODER_PROCS
{ void* w;
  int   (PM123_ENTRYP decoder_init     )( void** w );
  BOOL  (PM123_ENTRYP decoder_uninit   )( void*  w );
  ULONG (PM123_ENTRYP decoder_command  )( void*  w, ULONG msg, DECODER_PARAMS2* params );
  void  (PM123_ENTRYP decoder_event    )( void*  w, OUTEVENTTYPE event );
  ULONG (PM123_ENTRYP decoder_status   )( void*  w );
  ULONG (PM123_ENTRYP decoder_length   )( void*  w );
  ULONG (PM123_ENTRYP decoder_fileinfo )( const char* filename, DECODER_INFO2* info );
  ULONG (PM123_ENTRYP decoder_setmeta  )( const char* filename, const META_INFO* meta );
  ULONG (PM123_ENTRYP decoder_cdinfo   )( const char* drive, DECODER_CDINFO* info );
  ULONG (PM123_ENTRYP decoder_support  )( char*  ext[], int* size );
  ULONG (PM123_ENTRYP decoder_editmeta )( HWND owner, const char* url );
  const DECODER_ASSIST* (PM123_ENTRYP decoder_getassist)( BOOL multiselect );
  // Result from the decoder_support call. Supported data sources.
  int    type;
  // Result from the decoder_support call. Supported file types.
  char** support;
};

// specialized class for decoders
class CL_DECODER : public CL_PLUGIN, protected DECODER_PROCS
{private:
  PROXYFUNCDEF ULONG PM123_ENTRY stub_decoder_cdinfo( const char* drive, DECODER_CDINFO* info );
 protected:
  // instances of this class are only created by the factory function below.
  CL_DECODER(CL_MODULE& mod) : CL_PLUGIN(mod) { memset((DECODER_PROCS*)this, 0, sizeof(DECODER_PROCS)); }
  // Get some information about the decoder. (decoder_support)
  BOOL  after_load();
 public:
  virtual ~CL_DECODER();
  // Load the plug-in that is identified as a decoder. Return TRUE on success.
  virtual BOOL load_plugin();
  // Initialize the decoder. Return TRUE on success.
  virtual BOOL init_plugin();
  // Uninitialize the decoder. Return TRUE on success.
  virtual BOOL uninit_plugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual BOOL is_initialized() const    { return w != NULL; }
  // Getter to the decoder entry points.
  const DECODER_PROCS& get_procs() const { return *this; }
  
  // Factory function to create a new decoder instance from a module object.
  // This function will perform all necessary tasks to load a decoder.
  // It will especially virtualize decoders with an older interface level.
  // The returned object must be deleted if it is no longer used.
  static CL_PLUGIN* factory(CL_MODULE& mod);
};


/****************************************************************************
*
* output plug-in interface
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

// Set of entry points related to output plug-ins.
struct OUTPUT_PROCS
{ void* a;
  ULONG (PM123_ENTRYP output_init           )( void** a );
  ULONG (PM123_ENTRYP output_uninit         )( void*  a );
  ULONG (PM123_ENTRYP output_command        )( void*  a, ULONG msg, OUTPUT_PARAMS2* info );
  int   (PM123_ENTRYP output_request_buffer )( void*  a, const FORMAT_INFO2* format, short** buf );
  void  (PM123_ENTRYP output_commit_buffer  )( void*  a, int len, int posmarker );
  ULONG (PM123_ENTRYP output_playing_samples)( void*  a, FORMAT_INFO* info, char* buf, int len );
  int   (PM123_ENTRYP output_playing_pos    )( void*  a );
  BOOL  (PM123_ENTRYP output_playing_data   )( void*  a );
};

// specialized class for output plug-ins
class CL_OUTPUT : public CL_PLUGIN, protected OUTPUT_PROCS
{protected:
  // instances of this class are only created by the factory function below.
  CL_OUTPUT(CL_MODULE& mod) : CL_PLUGIN(mod) { a = NULL; }
 public:
  // Load the output plug-in. Return TRUE on success.
  virtual BOOL load_plugin();
  // Initialize the output plug-in. Return TRUE on success.
  virtual BOOL init_plugin();
  // Uninitialize the output plug-in. Return TRUE on success.
  virtual BOOL uninit_plugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual BOOL is_initialized() const        { return a != NULL; }
  // Getter to the entry points.
  const OUTPUT_PROCS& get_procs() const      { return *this; }

  // Factory function to create a new output plug-in instance from a module object.
  // This function will perform all necessary tasks to load the plug-in.
  // It will especially virtualize plug-ins with an older interface level.
  // The returned object must be deleted if it is no longer used.
  static CL_PLUGIN* factory(CL_MODULE& mod);
};


/****************************************************************************
*
* filter interface
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

// Set of entry points related to filter plug-ins.
struct FILTER_PROCS
{ void  *f;
  ULONG (PM123_ENTRYP filter_init        )( void** f, FILTER_PARAMS2* params );
  void  (PM123_ENTRYP filter_update      )( void*  f, const FILTER_PARAMS2* params );
  BOOL  (PM123_ENTRYP filter_uninit      )( void*  f );
};

// specialized class for filter plug-ins
class CL_FILTER : public CL_PLUGIN, protected FILTER_PROCS
{private:
  VREPLACE1    vrstubs[6];
 protected:
  // instances of this class are only created by the factory function below.
  CL_FILTER(CL_MODULE& mod) : CL_PLUGIN(mod) { f = NULL; }
 public:
  // Load the filter plug-in. Return TRUE on success.
  virtual BOOL load_plugin();
  // No-op. Filters are initialized explicitely by calling initialize. Return TRUE.
  virtual BOOL init_plugin();
  // Uninitialize the filter.
  virtual BOOL uninit_plugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual BOOL is_initialized() const     { return f != NULL; }
  // Initialize the filter.
  // params: pointer to a FILTER_PARAMS2 structure.
  //         [in] Entry points of the downstream filter or output.
  //         [out] Entry points of this filter instance.
  // Return TRUE on success.
  virtual BOOL initialize(FILTER_PARAMS2* params);
  // Getter to the entry points.
  const FILTER_PROCS& get_procs() const   { return *this; }

  // Factory function to create a new filter plug-in instance from a module object.
  // This function will perform all necessary tasks to load the plug-in.
  // It will especially virtualize plug-ins with an older interface level.
  // The returned object must be deleted if it is no longer used.
  static CL_PLUGIN* factory(CL_MODULE& mod);
};


/****************************************************************************
*
* visualization interface
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

// Set of entry points related to visual plug-ins.
struct VISUAL_PROCS
{ HWND  (PM123_ENTRYP plugin_init     )( VISPLUGININIT* init );
  BOOL  (PM123_ENTRYP plugin_deinit   )( int unload );
};

// specialized class for visual plug-ins
class CL_VISUAL : public CL_PLUGIN, protected VISUAL_PROCS
{protected:
  // Properties of this plug-in when it is part of a skin.
  VISUAL_PROPERTIES props;
  // Window handle of the visual plug-in.
  HWND              hwnd;

 protected:
  // instances of this class are only created by the factory function below.
  CL_VISUAL(CL_MODULE& mod) : CL_PLUGIN(mod), hwnd(NULLHANDLE) {}
 public:
  // Load the visual plug-in. Return TRUE on success.
  virtual BOOL load_plugin();
  // No-op. Filters are initialized explicitely by calling initialize. Return TRUE.
  virtual BOOL init_plugin();
  // Uninitialize the plug-in.
  virtual BOOL uninit_plugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual BOOL is_initialized() const   { return hwnd != NULLHANDLE; }
  // Initialize the plug-in.
  // hwnd:  PM123 window handle
  // procs: procedure entry points for the visual plug-in
  // id:    plug-in number
  // Return TRUE on success
  virtual BOOL initialize(HWND hwnd, PLUGIN_PROCS* procs, int id);
  // Getter to the properties of this plug-in.
  const VISUAL_PROPERTIES& get_properties() const { return props; }
  // Setter to the properties of this plug-in instance.
  virtual void set_properties(const VISUAL_PROPERTIES* data);
  // Getter to the entry points of this plug-in.
  const VISUAL_PROCS& get_procs() const { return *this; }
  // Get the plug-in's window handle.
  HWND         get_hwnd() const         { return hwnd; }
  
  // Factory function to create a new visual plug-in instance from a module object.
  // This function will perform all necessary tasks to load the plug-in.
  // It will especially virtualize plug-ins with an older interface level.
  // The returned object must be deleted if it is no longer used.
  static CL_PLUGIN* factory(CL_MODULE& mod);
};


/****************************************************************************
*
* PLUGIN_BASE_LIST collection
*
* Collection of plug-ins or modules
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

class CL_PLUGIN_BASE_LIST
{private:
  // List of the pointers to the plug-ins.
  CL_PLUGIN_BASE** list;
  // Number of plug-ins currently in the list.
  int             num;
  // Current size of the list. This may be lagreg or equal to num.
  int             size;
 protected:
  // Create an empty list
  CL_PLUGIN_BASE_LIST() : list(NULL), num(0), size(0) {}

  // The following functions are replaced by type-safe ones in the derived classes.
  
  // Append a new object to the list.
  BOOL            append(CL_PLUGIN_BASE* plugin);
  // Remove the i-th object from the list and return a pointer to it.
  CL_PLUGIN_BASE* detach(int i);
  // Return the i-th object in the list.
  CL_PLUGIN_BASE& operator[](int i) { return *list[i]; }
 public:
  // Destroy the current list.
  // we do not clean up the objects in the list here, since the underlying
  // object CL_PLUGIN_BASE has no virtual destructor.
  ~CL_PLUGIN_BASE_LIST()            { free(list); }
  // Search for a list entry by the module handle.
  // Return the index in the list or -1 if not found.
  int             find(const PLUGIN_BASE& plugin) const;
  // Search for a list entry by the module name.
  // Return the index in the list or -1 if not found.
  int             find(const char* name) const;
  // Search for a list entry by the short module name.
  // Return the index in the list or -1 if not found.
  int             find_short(const char* name) const;
  // Get the number of entries in the list.
  int             count() const     { return num; }
  // Get the root object of the list.
  const CL_PLUGIN_BASE* const* get_list() const { return list; }
};


/****************************************************************************
*
* MODULE_LIST collection
*
* List of loaded DLL modules.
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

class CL_MODULE_LIST : public CL_PLUGIN_BASE_LIST
{public:
  // Append a new module to the list.
  BOOL            append(CL_MODULE* plugin) { return CL_PLUGIN_BASE_LIST::append(plugin); }
  // Remove the i-th module from the list and return a pointer to it.
  CL_MODULE*      detach(int i)             { return (CL_MODULE*)CL_PLUGIN_BASE_LIST::detach(i); }
  // Get the i-th module in the list.
  CL_MODULE&      operator[](int i) { return (CL_MODULE&)CL_PLUGIN_BASE_LIST::operator[](i); }
};


/****************************************************************************
*
* PLUGIN_LIST collection
*
* Collection of plug-ins of any kind.
*
****************************************************************************/

class CL_PLUGIN_LIST : public CL_PLUGIN_BASE_LIST
{public:
  // append a new plug-in to the list.
  BOOL            append(CL_PLUGIN* plugin) { return CL_PLUGIN_BASE_LIST::append(plugin); }
  // Remove the i-th plug-in from the list and return a pointer to it.
  CL_PLUGIN*      detach(int i);
  // Destroy the i-th plug-in in the list. Return TRUE on success.
  BOOL            remove(int i);
  // Destroy all plug-ins in the list.
  void            clear();
  // Get the i-th plug-in in the list.
  CL_PLUGIN&      operator[](int i)  { return (CL_PLUGIN&)CL_PLUGIN_BASE_LIST::operator[](i); }
};


/****************************************************************************
*
* PLUGIN_LIST collection with one active plugin
*
* This specializes CL_PLUGIN_LIST for plug-in types where at most one plug-in
* of the list is active at a time.
* This class ist thred-safe on per instance basis.
*
****************************************************************************/

class CL_PLUGIN_LIST1 : public CL_PLUGIN_LIST
{private:
  // current active plug-in.
  CL_PLUGIN*      active;
 public:
  CL_PLUGIN_LIST1() : active(NULL)   {}
  // Remove the i-th plug-in from the list and return a pointer to it.
  // If it was the activated one it is deactivated (uninit) first.
  CL_PLUGIN*      detach(int i);
  // Return the index of the currently active plug-in.
  // Returns -1 if no plug-in is active.
  int             get_active() const { return active ? find(*active) : -1; }
  // Change the currently activated plug-in.
  // If there is another plug-in active it is deactivated (uninit) first.
  // If the desired plug-in is already active this is a no-op.
  // Return 0 on success.
  int             set_active(int i);
  // Returnthe currently active plug-in or NULL if none.
  CL_PLUGIN*      current() const    { return active; }
};


/****************************************************************************
*
* global lists
*
****************************************************************************/

// List of all modules
// Owned by plugman, but updated by the CL_PLUGIN constructor and destructor.
extern CL_MODULE_LIST plugins;


#endif
