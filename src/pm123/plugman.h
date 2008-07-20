/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2008 Marcel Mueller
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

#ifndef PM123_PLUGMAN_H
#define PM123_PLUGMAN_H

/* maximum supported and most recent plugin-levels */
#define MAX_PLUGIN_LEVEL     2
#define VISUAL_PLUGIN_LEVEL  1
#define FILTER_PLUGIN_LEVEL  2
#define DECODER_PLUGIN_LEVEL 2
#define OUTPUT_PLUGIN_LEVEL  2

#include "playable.h"
#include <plugin.h>
#include <format.h>
#include <output_plug.h>
#include <filter_plug.h>
#include <decoder_plug.h>
#include <visual_plug.h>
#include <utilfct.h>
#include <cpp/event.h>
#include <cpp/xstring.h>


typedef struct
{ int     x, y, cx, cy;
  BOOL    skin;
  char    param[256];
} VISUAL_PROPERTIES;


// Object representing a plugin-DLL
// This class ist thred-safe on per instance basis.
class Module
: public Iref_Count,
  public IComparableTo<xstring>,
  public inst_index<Module, const xstring>
{private:
  // New modules are always created by this factory
  class ModuleFactory : public inst_index<Module, const xstring>::IFactory
  { virtual Module* operator()(const xstring& key);
  };
 protected:
  HMODULE           HModule;
  PLUGIN_QUERYPARAM QueryParam;
 private:
  // Entry point of the configure dialog (if any).
  void DLLENTRYP(plugin_configure)(HWND hwnd, HMODULE module);
 private:
  Module(const Module&);         // avoid copy construction
  void operator=(const Module&); // ... and assignment
  // Load the DLL
  bool LoadModule();
  // Unload the DLL
  bool UnloadModule();
 public:
  // Create a Module object from the modules file name.
  Module(const xstring& name);
  ~Module();
  // Return full qualified module name.
  const xstring& GetModuleName() const       { return Key; }
  // Return reply of plugin_query. 
  const PLUGIN_QUERYPARAM& GetParams() const { return QueryParam; }
  // Load the address of a dll entry point. Return TRUE on success.
  bool    LoadOptionalFunction(void* function, const char* function_name) const;
  // Same as abobe, but raise amp_player_error in case of an error.
  bool    LoadFunction(void* function, const char* function_name) const;
  // Load a new DLL as plug-in. The plug-in flavour is specialized later.
  // Returns TRUE on success, otherwise return FALSE and call amp_player_error.
  bool    Load();
  // Launch the configure dialog.
  void    Config(HWND hwnd) const            { if (plugin_configure) (*plugin_configure)(hwnd, HModule); }
  // Comperator for repository
  virtual int compareTo(const xstring& key) const;
  // Ensure access to a Module.
  // The function returns NULL if the Module cannot be instantiated.
  static int_ptr<Module> GetByKey(const xstring& name) { return inst_index<Module, const xstring>::GetByKey(name, (ModuleFactory&)ModuleFactory()); }
};


/****************************************************************************
*
* Plugin - abstract object representing a plugin instance
*
* This class ist thred-safe on per instance basis.
*
****************************************************************************/
class PluginList;

class Plugin
{public:
  // Arguments of PluginChange event
  struct EventArgs
  { Plugin&     Plug;
    enum event
    { Load,
      Unload,
      Enable,
      Disable,
      Init,
      Uninit
    }           Operation;
  };

 public:
  static VISUAL_PROPERTIES VisualProps;
  // Notify changes to the plugin-lists.
  static event<const EventArgs> ChangeEvent;

 protected:
  // reference to the underlying module.
  const int_ptr<Module> ModRef;
  // Enabled flag. TRUE in doubt.
  bool         Enabled;
 private:
               Plugin(const Plugin&);    // avoid copy construction
  void         operator=(const Plugin&); // ... and assignment
 private:
  static Plugin* Instantiate(Module* mod, Plugin* (*factory)(Module*), PluginList& list, const char* params);

 protected:
  // instanciate a new plug-in.
               Plugin(Module* mod);
 public:
  // Destroy the current plug-in. This will implicitely deregister it from
  // the plug-in list "plugins". If the module is not used by another plug-in
  // the module will be unloaded.
  virtual      ~Plugin();
  // Getter to the underlying module.
  Module&      GetModule() const  { return *ModRef; }
  // Return full qualified module name. Same as GetModule().GetModuleName() for convenience.
  const xstring& GetModuleName() const;
  // Return kind of Plugin handled by the class instance. (RTTI by the backdoor.)
  virtual PLUGIN_TYPE GetType() const = 0;
  // Raise plugin_event
  void         RaisePluginChange(EventArgs::event ev);

  // Load the current plug-in. Return TRUE on success.
  // To be implemented by the particular plug-in flavour.
  virtual bool LoadPlugin() = 0;
  // Initialize the current plug-in. Return TRUE on success.
  // To be implemented by the particular plug-in flavour.
  virtual bool InitPlugin() = 0;
  // Uninitialize the current plug-in. Return TRUE on success.
  // To be implemented by the particular plug-in flavour.
  virtual bool UninitPlugin() = 0;
  // Tell whether the plug-in is currently initialized.
  // To be implemented by the particular plug-in flavour.
  virtual bool IsInitialized() const = 0;
  // Getter to the enabled state.
  bool         GetEnabled() const { return Enabled; }
  // Setter to the enabled state.
  virtual void SetEnabled(bool enabled);

  // Retrieve plug-in configuration parameters
  virtual void GetParams(stringmap& map) const;
  // Set the parameter param to value.
  // Return true if param is known and valid.
  // By overloading this function specific plug-ins may take individual parameters.
  // Note that value might be NULL.
  virtual bool SetParam(const char* param, const xstring& value);
  // Set configuration. All identified keys are removed from map.
  // The remaining keys are unknown.
  void         SetParams(stringmap_own& map);
  // Parse parameter string and call SetParam for each parameter.
  void         SetParams(const char* params);
  // Serialize current configuration and name to a string
  xstring      Serialize() const;
  // Create or update a plug-in from a string
  // Deserialize returns the real typess of the loaded plug-in.
  static int   Deserialize(const char* str, int type = PLUGIN_VISUAL|PLUGIN_FILTER|PLUGIN_DECODER|PLUGIN_OUTPUT, bool skinned = false);

  // global services
 protected:
  enum
  { UM_CREATEPROXYWINDOW = WM_USER,
    UM_DESTROYPROXYWINDOW
  };
 private:
  static HWND ServiceHwnd;
  friend MRESULT EXPENTRY cl_plugin_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 protected:
  static HWND CreateProxyWindow(const char* cls, void* ptr);
  static void DestroyProxyWindow(HWND hwnd);
 public: 
  static void Init();
  static void Uninit();
};


// Collection of plug-ins of any kind.
class PluginList : public vector_own<Plugin>
{public:
  enum RC
  { RC_OK,
    RC_InUse,
    RC_Error
  };
 public:
  // Type of the plug-ins in this list.
  const PLUGIN_TYPE Type;
 protected:
  const char* const Defaults;

 private: // non-copyable
  PluginList(const PluginList&);
  void operator=(const PluginList&);
 public:
                  PluginList(PLUGIN_TYPE type, const char* def) : vector_own<Plugin>(20), Type(type), Defaults(def) {}
  // append a new plug-in to the list.
  virtual void    append(Plugin* plugin);
  // Remove the i-th plug-in from the list and return a pointer to it.
  virtual Plugin* erase(int i);
  // Destroy the i-th plug-in in the list. Return TRUE on success.
  virtual bool    remove(int i);
  // Return index of Plugin or -1
  int             find(const Plugin* plugin) const;
  int             find(const Module* module) const;
  int             find(const char* module) const;
  // Serialize plug-in list to a string
  virtual xstring Serialize() const;
  // Deserialzie plug-in list from a string.
  // Return false on error.
  virtual RC      Deserialize(const xstring& str);
  // Load default plug-ins
  void            LoadDefaults() { Deserialize(Defaults); }
};

class PluginList1 : public PluginList
{private:
  Plugin*         Active; // current active plug-in.
 public:
                  PluginList1(PLUGIN_TYPE type, const char* def) : PluginList(type, def), Active(NULL) {}
  // Remove the i-th plug-in from the list and return a pointer to it.
  // If it was the activated one it is deactivated (uninit) first.
  virtual Plugin* erase(int i);
  // Return the index of the currently active plug-in.
  // Returns -1 if no plug-in is active.
  int             GetActive() const   { return Active ? find(Active) : -1; }
  // Change the currently activated plug-in.
  // If there is another plug-in active it is deactivated (uninit) first.
  // If the desired plug-in is already active this is a no-op.
  // Return 0 on success.
  int             SetActive(int i);
  // Returnthe currently active plug-in or NULL if none.
  Plugin*         Current() const     { return Active; }
  // Serialize plug-in list to a string
  virtual xstring Serialize() const;
  // Deserialzie plug-in list from a string.
  // Return false on error.
  virtual RC      Deserialize(const xstring& str);
};


/****************************************************************************
*
*  GUI extension interface of the plug-ins
*
****************************************************************************/
/* Plug-in menu in the main pop-up menu */
void  load_plugin_menu( HWND hmenu );
/* Add additional entries in load/add menu in the main and the playlist's pop-up menu */
void  dec_append_load_menu( HWND hMenu, ULONG id_base, SHORT where, DECODER_WIZZARD_FUNC* callbacks, int size );
/* Append accelerator table with plug-in specific entries */
void  dec_append_accel_table( HACCEL& haccel, ULONG id_base, LONG offset, DECODER_WIZZARD_FUNC* callbacks, int size );


/****************************************************************************
*
* global lists
*
****************************************************************************/

extern PluginList1 Decoders;       // only decoders
extern PluginList1 Outputs;        // only outputs
extern PluginList  Filters;        // only filters
extern PluginList  Visuals;        // only visuals
extern PluginList  VisualsSkinned; // visual plug-ins loaded by skin


/****************************************************************************
*
*  Global initialization functions
*
****************************************************************************/

/* Initialize plug-in manager */
void  plugman_init();
/* Deinitialize plug-in manager */
void  plugman_uninit();

#endif /* PM123_PLUGMAN_H */

