/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2011 Marcel Mueller
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

#include <plugin.h>
#include <decoder_plug.h>
#include <cpp/container/inst_index.h>
#include <cpp/event.h>
#include <cpp/xstring.h>
#include <cpp/container/stringmap.h>


typedef struct
{ int     x, y, cx, cy;
  BOOL    skin;
  char    param[256];
} VISUAL_PROPERTIES;


/****************************************************************************
 *
 * @brief Object representing a plug-in-DLL
 *
 * This class is thread-safe on per instance basis.
 *
 ***************************************************************************/
class Module
: public Iref_count
, public inst_index<Module, const xstring, &xstring::compare>
{protected:
  HMODULE                  HModule;
  xstring                  ModuleName;
  PLUGIN_QUERYPARAM        QueryParam;
 private: // Static storage for plugin_init.
  PLUGIN_API               PluginApi;
  static const DSTRING_API DstringApi;
  PLUGIN_CONTEXT           Context;
 private:
  /// Entry point of the configure dialog (if any).
  void DLLENTRYP(plugin_configure)(HWND hwnd, HMODULE module);
 private: // non-copyable
  Module(const Module&);
  void operator=(const Module&);
 private:
  /// Load the DLL.
  bool LoadModule();
  /// Unload the DLL.
  bool UnloadModule();
 protected:
  /// Create a Module object from the module file name.
  Module(const xstring& name);
 private:
  static Module* Factory(const xstring& key);
 public:
  ~Module();
  /// Return full qualified module name.
  const xstring& GetModuleName() const       { return ModuleName; }
  /// Return reply of \c plugin_query.
  const PLUGIN_QUERYPARAM& GetParams() const { return QueryParam; }
  /// Load the address of a DLL entry point.
  /// @return Return \c true on success.
  bool    LoadOptionalFunction(void* function, const char* function_name) const;
  /// Same as \c LoadOptionalFunction, but raise \c amp_player_error in case of an error.
  bool    LoadFunction(void* function, const char* function_name) const;
  /// Load a new DLL as plug-in. The plug-in flavor is specialized later.
  /// @return Returns \c true on success, otherwise return \c false
  /// and call \c amp_player_error.
  bool    Load();
  /// Launch the configure dialog.
  /// @param hwnd Parent window.
  void    Config(HWND hwnd) const            { DEBUGLOG(("Module(%p{%s})::Config(%p) - %p\n", this, Key.cdata(), hwnd, plugin_configure));
                                               if (plugin_configure != NULL) (*plugin_configure)(hwnd, HModule); }

  /// Ensure access to a Module.
  /// @return The function returns \c NULL if the module cannot be instantiated.
  static int_ptr<Module> GetByKey(const xstring& name) { return inst_index<Module, const xstring, &xstring::compare>::GetByKey(name, &Module::Factory); }
};


class PluginList;
/****************************************************************************
*
* @brief Plugin - abstract object representing a plug-in instance.
*
* This class is thread-safe on per instance basis.
*
****************************************************************************/
class Plugin
{public:
  /// Arguments of PluginChange event
  struct EventArgs
  { Plugin&     Plug;
    enum event
    { /// The plug-in is loaded by the user
      Load,
      /// The plug-in is unloaded by the user
      Unload,
      /// The plug-in is enabled by the user
      Enable,
      /// The plug-in is disabled by the user
      Disable,
      /// The plug-in is instantiated by the plug-in manager
      Init,
      /// The plug-in instance is destroyed by the plug-in manager
      Uninit,
      /// The plug-in is activated (outputs and decoders only)
      Active,
      /// The plug-in is deactivated (outputs and decoders only)
      Inactive
    }           Operation;
  };

 public:
  static VISUAL_PROPERTIES VisualProps;
  /// Notify changes to the plug-in lists.
  static event<const EventArgs> ChangeEvent;

 protected:
  /// Strong reference to the underlying module.
  const int_ptr<Module> ModRef;
  /// Enabled flag. \c true in doubt.
  bool         Enabled;
 private:
  const PLUGIN_TYPE PluginType;
 private: // non-copyable
               Plugin(const Plugin&);
  void         operator=(const Plugin&);
 private:
  static Plugin* Instantiate(Module* mod, Plugin* (*factory)(Module*), PluginList& list, const char* params);

 protected:
  /// Instantiate a new plug-in.
               Plugin(Module* mod, PLUGIN_TYPE type);
 public:
  /// @brief Destroy the current plug-in.
  /// @details This will implicitly deregister it from the plug-in list.
  /// If the module is not used by another plug-in it will be unloaded.
  virtual      ~Plugin();
  /// Getter to the underlying module.
  Module&      GetModule() const  { return *ModRef; }
  /// Return kind of plug-in handled by the class instance.
  PLUGIN_TYPE  GetType() const    { return PluginType; }
  /// Raise plugin_event
  void         RaisePluginChange(EventArgs::event ev);

  /// @brief Load the current plug-in.
  /// @return Return \c true on success.
  /// @details To be implemented by the particular plug-in flavor.
  virtual bool LoadPlugin() = 0;
  /// @brief Initialize the current plug-in.
  /// @return Return \c true on success.
  /// @details To be implemented by the particular plug-in flavor.
  virtual bool InitPlugin() = 0;
  /// Deinitialize the current plug-in.
  /// @return Return \c true on success.
  /// @details To be implemented by the particular plug-in flavor.
  virtual bool UninitPlugin() = 0;
  /// Tell whether the plug-in is currently initialized.
  /// @details To be implemented by the particular plug-in flavor.
  virtual bool IsInitialized() const = 0;

  /// Getter to the enabled state.
  bool         GetEnabled() const { return Enabled; }
  /// Setter to the enabled state.
  virtual void SetEnabled(bool enabled);

  /// Retrieve plug-in configuration parameters
  virtual void GetParams(stringmap& map) const;
  /// @brief Set the parameter \a param to value.
  /// @return Return \c true if \a param is known and valid.
  /// @details By overloading this function specific plug-ins may take individual parameters.
  /// Note that value might be \c NULL.
  virtual bool SetParam(const char* param, const xstring& value);
  /// @brief Set the entire plug-in configuration.
  /// @details All identified keys are removed from map.
  /// The remaining keys are unknown.
  void         SetParams(stringmap_own& map);
  /// Parse parameter string and call SetParam for each parameter.
  void         SetParams(const char* params);
  /// Serialize current configuration and name to a string.
  xstring      Serialize() const;
  /// @brief Create or update a plug-in from a string.
  /// @return Deserialize returns the real types of the loaded plug-in.
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


/****************************************************************************
 *
 * Collection of plug-ins of any kind.
 *
 ***************************************************************************/
class PluginList : public vector_own<Plugin>
{public:
  enum RC
  { RC_OK,
    RC_InUse,
    RC_Error
  };
 public:
  /// Type of the plug-ins in this list instance.
  const PLUGIN_TYPE Type;
 protected:
  const char* const Defaults;

 private: // non-copyable
  PluginList(const PluginList&);
  void operator=(const PluginList&);
 public:
                  PluginList(PLUGIN_TYPE type, const char* def) : vector_own<Plugin>(20), Type(type), Defaults(def) {}
  /// Append a new plug-in to the list.
  virtual void    append(Plugin* plugin);
  /// Remove the i-th plug-in from the list.
  /// @return Return a pointer to the removed plugin.
  virtual Plugin* erase(size_t i);
  /// Destroy the i-th plug-in in the list.
  /// @return Return \c true on success.
  virtual bool    remove(int i);
  /// @return Return index of plug-in or -1 if not found.
  int             find(const Plugin* plugin) const;
  /// @return Return index of plug-in or -1 if not found.
  int             find(const Module* module) const;
  /// @return Return index of plug-in or -1 if not found.
  int             find(const char* module) const;
  /// @brief Serialize plug-in list to a string.
  /// @details This implicitly calls Serialize for each plug-in in the list.
  virtual const xstring Serialize() const;
  /// Deserialzie plug-in list from a string.
  /// @return Return false on error.
  virtual RC      Deserialize(const xstring& str);
  /// Load default plug-ins
  void            LoadDefaults() { Deserialize(Defaults); }
};

/****************************************************************************
 *
 * Specialized version of PluinList that keep track of exactly one activated plug-in.
 *
 ***************************************************************************/
class PluginList1 : public PluginList
{private:
  /// Current active plug-in.
  Plugin*         Active;
 public:
                  PluginList1(PLUGIN_TYPE type, const char* def) : PluginList(type, def), Active(NULL) {}
  /// Remove the i-th plug-in from the list and return a pointer to it.
  /// If it was the activated one it is deactivated (uninit) first.
  virtual Plugin* erase(size_t i);
  /// Return the index of the currently active plug-in.
  /// @return Returns -1 if no plug-in is active.
  int             GetActive() const   { return Active ? find(Active) : -1; }
  /// @brief Change the currently activated plug-in.
  /// @details If there is another plug-in active it is deactivated (uninit) first.
  /// If the desired plug-in is already active this is a no-op.
  /// @return Return 0 on success.
  int             SetActive(int i);
  /// Get the currently active plug-in or \c NULL if none.
  Plugin*         Current() const     { return Active; }
  /// Serialize plug-in list to a string
  virtual const xstring Serialize() const;
  /// Deserialzie plug-in list from a string.
  /// @return Return \c false on error.
  virtual RC      Deserialize(const xstring& str);
};


/****************************************************************************
*
*  GUI extension interface of the plug-ins
*
****************************************************************************/

/// Plug-in menu in the main pop-up menu.
void  load_plugin_menu( HWND hmenu );
/// Add additional entries in load/add menu in the main and the playlist's pop-up menu.
void  dec_append_load_menu( HWND hMenu, ULONG id_base, SHORT where, DECODER_WIZARD_FUNC* callbacks, size_t size );
/// Append accelerator table with plug-in specific entries.
void  dec_append_accel_table( HACCEL& haccel, ULONG id_base, LONG offset, DECODER_WIZARD_FUNC* callbacks, size_t size );


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

/// Initialize plug-in manager.
void  plugman_init();
/// Deinitialize plug-in manager.
void  plugman_uninit();

#endif /* PM123_PLUGMAN_H */
