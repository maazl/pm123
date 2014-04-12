/*
 * Copyright 2006-2012 Marcel Mueller
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
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
#include <cpp/event.h>
#include <cpp/xstring.h>
#include <cpp/container/vector.h>


/****************************************************************************
*
* Class tree:
*
* Module
*  <- Plugin              (n Plugins share the same Module instance)
*      +- Decoder
*      |   +- proxy classes ...
*      +- Output
*      |   +- proxy classes ...
*      +- Filter
*      |   +- proxy classes ...
*      +- Visual
*          +- proxy classes ...
*
****************************************************************************/

class Plugin;
class PluginList;
class stringmap_own;
class Decoder;
class Filter;
class Output;
class Visual;

FLAGSATTRIBUTE(PLUGIN_TYPE);

class ModuleException
{ xstring Error;
 public:
  ModuleException(const char* fmt, ...);
  const xstring& GetErrorText() const { return Error; }
};

// Hack to selectively grant access rights to the plug-in classes.
// Each plug-in class has its own slot in class \c Module.
class DecoderInstance
{ friend class Decoder;
  Decoder*                 Dec;
 public:
  DecoderInstance()        : Dec(NULL) {}
};
class FilterInstance
{ friend class Filter;
  Filter*                  Fil;
 public:
  FilterInstance()         : Fil(NULL) {}
};
class OutputInstance
{ friend class Output;
  Output*                  Out;
 public:
  OutputInstance()         : Out(NULL) {}
};
class VisualInstance
{ friend class Visual;
  Visual*                  Vis;
 public:
  VisualInstance()         : Vis(NULL) {}
};

/****************************************************************************
 *
 * @brief Object representing a plug-in-DLL
 *
 * This class is thread-safe on per instance basis.
 *
 ***************************************************************************/
class Module
: public Iref_count
, public DecoderInstance
, public FilterInstance
, public OutputInstance
, public VisualInstance
{public:
  const xstring            Key;              ///< Short name of the module.
  const xstring            ModuleName;       ///< Long name of the module.
 protected:
  HMODULE                  HModule;          ///< DLL handle of the module.
  PLUGIN_QUERYPARAM        QueryParam;       ///< Result of \c plugin_query.
  HWND                     ConfigWindow;     ///< Handle of the configuration window or -1, while existent.
  /// Entry point of the configure dialog (if any).
  HWND DLLENTRYP(plugin_configure)(HWND owner, HMODULE module);
  /// Entry point of the configure dialog (if any).
  void DLLENTRYP(plugin_command)(const char* command, xstring* result, xstring* messages);

 protected:
  /// Create a Module object from the module file name.
  Module(const xstring& key, const xstring& name);
 public:
  virtual ~Module()                          {}
  /// Return reply of \c plugin_query.
  const PLUGIN_QUERYPARAM& GetParams() const { return QueryParam; }
  /// Load the address of a DLL entry point.
  /// @param function Here is the entry point placed. On error this is set to \c NULL.
  /// @return 0 on success, OS/2 error code otherwise.
  APIRET  LoadOptionalFunction(void* function, const char* function_name) const
                                             { *(int(**)())function = NULL; return DosQueryProcAddr(HModule, 0L, (PSZ)function_name, (PFN*)function); }
  /// Load the address of a DLL entry point.
  /// @param function Here is the entry point placed. On error this is set to \c NULL.
  /// @exception ModuleException Something went wrong.
  void    LoadMandatoryFunction(void* function, const char* function_name) const;
  /// Launch the configure dialog.
  /// @param owner Parent window. \c NULLHANDLE will close a non-modal dialog.
  /// @remarks This function must be called from thread 1.
  void    Config(HWND owner);
  /// Check whether the configuration dialog is currently visible.
  bool    IsConfig() const                   { return ConfigWindow != NULLHANDLE; }
  /// Handle plug-in specific command if the plug-in exports plugin_command.
  /// Otherwise the function is a no-op, result is unchanged.
  bool    Command(const char* command, xstring& result, xstring& messages) const
                                             { if (!plugin_command) return false; (*plugin_command)(command, &result, &messages); return true; }

  /// Helper function to provide message callback for plug-in API.
  PROXYFUNCDEF void DLLENTRY PluginDisplayMessage(MESSAGE_TYPE type, const char* msg);

 private: // non-copyable
  Module(const Module&);
  void operator=(const Module&);
 public: // Repository
  static Mutex&          Mtx;
  /// Check whether a module is loaded and return a strong reference if so.
  /// @return The function returns \c NULL if the module is not yet loaded.
  static int_ptr<Module> FindByKey(const char* name);
  /// Ensure access to a Module.
  /// @exception ModuleException Something went wrong.
  /// @remarks This function must be called from thread 1.
  static int_ptr<Module> GetByKey(const char* name);
  /// Gets a snapshot of all currently active plug-in instances.
  static void            CopyAllInstancesTo(vector_int<Module>& target);
};


/// Arguments of PluginChange event
struct PluginEventArgs
{ PLUGIN_TYPE Type;
  enum event
  { Unload,              ///< The plug-in is unloaded by the user
    Load,                ///< The plug-in is loaded by the user
    Disable,             ///< The plug-in is disabled by the user
    Enable,              ///< The plug-in is enabled by the user
    Uninit,              ///< The plug-in instance is destroyed by the plug-in manager
    Init,                ///< The plug-in is instantiated by the plug-in manager
    Sequence             ///< The Sequence of the plug-ins has changed. Plug is \c NULL.
  } Operation;
  Plugin* Plug;
};

/****************************************************************************
*
* @brief Plug-in - abstract object representing a plug-in instance.
* A single module can have multiple plug-in instances of different type.
*
* This class is thread-safe on per instance basis.
*
****************************************************************************/
class Plugin : public Iref_count
{public:
  /// Strong reference to the underlying module.
  const int_ptr<Module> ModRef;
  /// Kind of plug-in handled by the class instance.
  const PLUGIN_TYPE PluginType;
 protected:
  /// Enabled flag. \c true in doubt.
  /// @remarks Strictly speaking this property is a property o a plug-in list entry.
  /// But since a \c Plugin instance can only exist in a single PluginList,
  /// it is stored here to prevent further complexity.
  bool         Enabled;
 protected:
  /// Instantiate a new plug-in.
               Plugin(Module& mod, PLUGIN_TYPE type);
  /// Raise plugin_event
  void         RaisePluginChange(PluginEventArgs::event ev);

public:
  /// @brief Destroy the current plug-in.
  /// @details This will implicitly deregister it from the plug-in list.
  /// If the module is not used by another plug-in it will be unloaded.
  virtual      ~Plugin();

  /// Getter to the enabled state.
  bool         GetEnabled() const { return Enabled; }
  /// Setter to the enabled state.
  virtual void SetEnabled(bool enabled);

  /// Retrieve plug-in configuration parameters
  virtual void GetParams(stringmap_own& map) const;
  /// @brief Set the parameter \a param to value.
  /// @return Return \c true if \a param is known and valid.
  /// @details By overloading this function specific plug-ins may take individual parameters.
  /// Note that value might be \c NULL.
  /// @remarks This function must be called from thread 1.
  virtual bool SetParam(const char* param, const xstring& value);
  /// @brief Set the entire plug-in configuration.
  /// @details All identified keys are removed from map.
  /// The remaining keys are unknown.
  /// @remarks This function must be called from thread 1.
  void         SetParams(stringmap_own& map);
  /// Serialize current configuration and name and append it to \a target.
  void         Serialize(xstringbuilder& target) const;

 protected:
  // This lists are protected by Module::Mtx.
  static volatile int_ptr<PluginList> Decoders;
  static volatile int_ptr<PluginList> Outputs;
  static volatile int_ptr<PluginList> Filters;
  static volatile int_ptr<PluginList> Visuals;
  /*static volatile int_ptr<PluginList<Decoder> > Decoders;
  static volatile int_ptr<PluginList<Output> > Outputs;
  static volatile int_ptr<PluginList<Filter> > Filters;
  static volatile int_ptr<PluginList<Visual> > Visuals;*/
 private:
  /// Notify changes to the plug-in lists.
  static event<const PluginEventArgs> ChangeEvent;
 protected:
  /// Generic access to the PluginLists.
  static volatile int_ptr<PluginList>& AccessPluginList(PLUGIN_TYPE type);
 public:
  /// Access plug-in change event.
  static event_pub<const PluginEventArgs>& GetChangeEvent() { return ChangeEvent; }

  /// Return the \c plug-in instance of the requested type on \a module
  /// or null if none.
  static int_ptr<Plugin> FindInstance(Module& module, PLUGIN_TYPE type);
  /// Return the \c plug-in instance of the requested type on \a module. Create a new one if required.
  /// @exception ModuleException Something went wrong.
  /// @remarks This function must be called from thread 1.
  static int_ptr<Plugin> GetInstance(Module& module, PLUGIN_TYPE type);
  /// @brief Create or update a plug-in from a string.
  /// @exception ModuleException Something went wrong.
  /// @remarks Note that the function does not add the plug-in
  /// to any list. But the parameters in the string are applied immediately,
  /// if the plug-in \e is already in a list.
  /// @remarks This function must be called from thread 1.
  static int_ptr<Plugin> Deserialize(const char* str, PLUGIN_TYPE type);

  /// Retrieve the list of plug-ins of a given type. (Thread-safe)
  /// @return strong reference to a snapshot of the plug-ins.
  /// You must never modify the list!
  static int_ptr<PluginList> GetPluginList(PLUGIN_TYPE type) { return AccessPluginList(type); }

  /// Append the plug-in to the appropriate list.
  /// @exception ModuleException Something went wrong.
  static void  AppendPlugin(Plugin* plugin);
  /// Replace a plug-in list. This activates changes.
  /// The plug-in type is detected automatically from the type of \a source.
  /// @param source New plug-in list. Assigned by the old value on return.
  static void  SetPluginList(PluginList* source);
};


/***************************************************************************
 *
 * Collection of plug-ins of any kind.
 *
 * This class is not thread-safe.
 *
 ***************************************************************************/
class PluginList : public Iref_count, public vector_int<Plugin>
{public:
  /// Type of the plug-ins in this list instance.
  const PLUGIN_TYPE Type;

 public:
  PluginList(PLUGIN_TYPE type) : Type(type) {}
  PluginList(const PluginList& r) : vector_int<Plugin>(r), Type(r.Type) {}
  PluginList&     operator=(const PluginList& r) { ASSERT(Type == r.Type); vector_int<Plugin>::operator=(r); return *this; }

  /// Check whether a plug-in is already in the list.
  bool            contains(const Plugin* plugin) const { return find(plugin) != NULL; }

  /// @brief Serialize plug-in list to a string.
  /// @details This implicitly calls Serialize for each plug-in in the list.
  const xstring   Serialize() const;
  /// Deserialize plug-in list from a string.
  /// @return \c NULL on success and an error text on error.
  /// @remarks This function must be called from thread 1.
  xstring         Deserialize(const char* str);

  /// Reset the list to the default of this plug-in flavor.
  /// @return \c NULL on success and an error text on error.
  /// @remarks This function must be called from thread 1.
  xstring         LoadDefaults();
};


/****************************************************************************
*
*  Global initialization functions
*
****************************************************************************/

/// Initialize plug-in manager.
void plugman_init();
/// Deinitialize plug-in manager.
void plugman_uninit();

#endif /* PM123_PLUGMAN_H */
