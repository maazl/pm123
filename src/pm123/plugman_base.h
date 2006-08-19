/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
 * Copyright 2004 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef _PM123_PLUGMAN_BASE_H
#define _PM123_PLUGMAN_BASE_H

#include "plugman.h"


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
*    +- CL_PLUGIN_LIST1 (exactly one active plugin)
*
****************************************************************************/

/****************************************************************************
*
* CL_PLUGIN_BASE - C++ version of PLUGIN_BASE
*
****************************************************************************/

class CL_PLUGIN_BASE : public PLUGIN_BASE
{protected:
  BOOL load_function(void* function, const char* function_name);
};

/****************************************************************************
*
* CL_MODULE - object representing a plugin-DLL
*
****************************************************************************/

class CL_PLUGIN;

class CL_MODULE : public CL_PLUGIN_BASE
{private:
  int  refcount;
  void (PM123_ENTRYP plugin_configure)(HWND hwnd, HMODULE module);
 private:
  CL_MODULE(const CL_MODULE&); // non-existent, avoid copy construction
  BOOL load_module();
  BOOL unload_module();
 public:
  CL_MODULE(const char* name);
  ~CL_MODULE();
  BOOL load();
  BOOL unload_request()              { return !refcount && unload_module(); }
  void config(HWND hwnd) const       { (*plugin_configure)(hwnd, module); }
  void attach(const CL_PLUGIN* inst) { ++refcount; }
  BOOL detach(const CL_PLUGIN* inst) { return --refcount == 0; }
};


/****************************************************************************
*
* CL_PLUGIN - abstract object representing a plugin instance
*
****************************************************************************/

class CL_PLUGIN : public CL_PLUGIN_BASE
{protected:
  CL_MODULE& modref;
  BOOL enabled;
 private:
  CL_PLUGIN(const CL_PLUGIN&); // non-existent, avoid copy construction
 public:
  CL_PLUGIN(CL_MODULE& mod);
  virtual ~CL_PLUGIN();
  virtual BOOL load_plugin() = 0;
  virtual BOOL init_plugin() = 0;
  virtual BOOL uninit_plugin() = 0;
  virtual BOOL is_initialized() const = 0;
  CL_MODULE&   get_module() const  { return modref; }
          BOOL get_enabled() const { return enabled; }
  virtual void set_enabled(BOOL enabled);
};


/****************************************************************************
*
* decoder interface
*
****************************************************************************/

struct DECODER_PROCS
{ void* w;
  int   (PM123_ENTRYP decoder_init     )( void** w );
  BOOL  (PM123_ENTRYP decoder_uninit   )( void*  w );
  ULONG (PM123_ENTRYP decoder_command  )( void*  w, ULONG msg, DECODER_PARAMS2* params );
  void  (PM123_ENTRYP decoder_event    )( void*  w, OUTEVENTTYPE event );
  ULONG (PM123_ENTRYP decoder_status   )( void*  w );
  ULONG (PM123_ENTRYP decoder_length   )( void*  w );
  ULONG (PM123_ENTRYP decoder_fileinfo )( char*  filename, DECODER_INFO *info );
  ULONG (PM123_ENTRYP decoder_cdinfo   )( char*  drive, DECODER_CDINFO* info );
  ULONG (PM123_ENTRYP decoder_support  )( char*  ext[], int* size );
  int    type; 
  char** support;
};

// specialized class for decoders
class CL_DECODER : public CL_PLUGIN, protected DECODER_PROCS
{protected:
  CL_DECODER(CL_MODULE& mod) : CL_PLUGIN(mod) { w = NULL; support = NULL; }
  BOOL  after_load();
 public:
  virtual ~CL_DECODER();
  virtual BOOL load_plugin();
  virtual BOOL init_plugin();
  virtual BOOL uninit_plugin();
  virtual BOOL is_initialized() const    { return w != NULL; }
  const DECODER_PROCS& get_procs() const { return *this; }
  
  static CL_PLUGIN* factory(CL_MODULE& mod);
};


/****************************************************************************
*
* output interface
*
****************************************************************************/

struct OUTPUT_PROCS
{ void* a;
  ULONG (PM123_ENTRYP output_init           )( void** a );
  ULONG (PM123_ENTRYP output_uninit         )( void*  a );
  ULONG (PM123_ENTRYP output_command        )( void*  a, ULONG msg, OUTPUT_PARAMS2* info );
  int   (PM123_ENTRYP output_request_buffer )( void*  a, const FORMAT_INFO* format, char** buf, int posmarker );
  void  (PM123_ENTRYP output_commit_buffer  )( void*  a, int len );
  ULONG (PM123_ENTRYP output_playing_samples)( void*  a, FORMAT_INFO* info, char* buf, int len );
  int   (PM123_ENTRYP output_playing_pos    )( void*  a );
  BOOL  (PM123_ENTRYP output_playing_data   )( void*  a );
};

// specialized class for output plug-ins
class CL_OUTPUT : public CL_PLUGIN, protected OUTPUT_PROCS
{protected:
  CL_OUTPUT(CL_MODULE& mod) : CL_PLUGIN(mod) { a = NULL; }
 public:
  virtual BOOL load_plugin();
  virtual BOOL init_plugin();
  virtual BOOL uninit_plugin();
  virtual BOOL is_initialized() const        { return a != NULL; }
  const OUTPUT_PROCS& get_procs() const      { return *this; }

  static CL_PLUGIN* factory(CL_MODULE& mod);
};


/****************************************************************************
*
* filter interface
*
****************************************************************************/

struct FILTER_PROCS
{ void  *f;
  ULONG (PM123_ENTRYP filter_init        )( void** f, FILTER_PARAMS2* params );
  BOOL  (PM123_ENTRYP filter_uninit      )( void*  f );
};

// specialized class for filter plug-ins
class CL_FILTER : public CL_PLUGIN, protected FILTER_PROCS
{protected:
  CL_FILTER(CL_MODULE& mod) : CL_PLUGIN(mod) { f = NULL; }
 public:
  virtual BOOL load_plugin();
  virtual BOOL init_plugin();
  virtual BOOL uninit_plugin();
  virtual BOOL is_initialized() const     { return f != NULL; }
  const FILTER_PROCS& get_procs() const   { return *this; }
  
  static CL_PLUGIN* factory(CL_MODULE& mod);
};


/****************************************************************************
*
* visualization interface
*
****************************************************************************/

struct VISUAL_PROCS
{ HWND  (PM123_ENTRYP plugin_init     )( VISPLUGININIT* init );
  BOOL  (PM123_ENTRYP plugin_deinit   )( void* f );
};

// specialized class for visual plug-ins
class CL_VISUAL : public CL_PLUGIN, protected VISUAL_PROCS
{protected:
  VISUAL_PROPERTIES props;
  HWND              hwnd;

 protected:
  CL_VISUAL(CL_MODULE& mod) : CL_PLUGIN(mod), hwnd(NULLHANDLE) {}
 public:
  virtual BOOL load_plugin();
  virtual BOOL init_plugin(); // don't work for visual plug-ins
  virtual BOOL uninit_plugin();
  virtual BOOL is_initialized() const   { return hwnd != NULLHANDLE; }
  virtual BOOL initialize(VISPLUGININIT* visinit);
  const VISUAL_PROPERTIES& get_properties() const { return props; }
  virtual void set_properties(const VISUAL_PROPERTIES* data);
  const VISUAL_PROCS& get_procs() const { return *this; }
  HWND         get_hwnd() const         { return hwnd; }
  
  static CL_PLUGIN* factory(CL_MODULE& mod);
};


/****************************************************************************
*
* PLUGIN_BASE_LIST collection
*
****************************************************************************/

class CL_PLUGIN_BASE_LIST
{private:
  CL_PLUGIN_BASE** list;
  int             num;
  int             size;
 public:
  CL_PLUGIN_BASE_LIST() : list(NULL), num(0), size(0) {}
  ~CL_PLUGIN_BASE_LIST() { free(list); }
  BOOL            append(CL_PLUGIN_BASE* plugin);
  CL_PLUGIN_BASE* detach(int i);
  int             find(const PLUGIN_BASE& plugin) const;
  int             find(const char* name) const;
  int             count() const     { return num; }
  const CL_PLUGIN_BASE* const* get_list() const { return list; }
  CL_PLUGIN_BASE& operator[](int i) { return *list[i]; }
};


/****************************************************************************
*
* MODULE_LIST collection
*
****************************************************************************/

class CL_MODULE_LIST : public CL_PLUGIN_BASE_LIST
{public:
  BOOL            append(CL_MODULE* plugin) { return CL_PLUGIN_BASE_LIST::append(plugin); }
  CL_MODULE*      detach_request(int i);
  CL_MODULE&      operator[](int i) { return (CL_MODULE&)CL_PLUGIN_BASE_LIST::operator[](i); }
};


/****************************************************************************
*
* PLUGIN_LIST collection
*
****************************************************************************/

class CL_PLUGIN_LIST : public CL_PLUGIN_BASE_LIST
{public:
  BOOL            append(CL_PLUGIN* plugin) { return CL_PLUGIN_BASE_LIST::append(plugin); }
  CL_PLUGIN*      detach(int i);
  BOOL            remove(int i);
  void            clear();
  CL_PLUGIN&      operator[](int i)  { return (CL_PLUGIN&)CL_PLUGIN_BASE_LIST::operator[](i); }
};


/****************************************************************************
*
* PLUGIN_LIST collection with one active plugin
*
****************************************************************************/

class CL_PLUGIN_LIST1 : public CL_PLUGIN_LIST
{private:
  CL_PLUGIN*      active;
 public:
  CL_PLUGIN_LIST1() : active(NULL)   {}
  CL_PLUGIN*      detach(int i);
  int             get_active() const { return active ? find(*active) : -1; }
  int             set_active(int i);
  CL_PLUGIN*      current() const    { return active; }
};


/****************************************************************************
*
* global lists
*
****************************************************************************/

extern CL_MODULE_LIST plugins;  // all plug-ins


#endif
