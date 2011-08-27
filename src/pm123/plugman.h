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
*      |   +- DecoderImp  (private implementation)
*      |       +- proxy classes ...
*      +- Output
*      |   +- OutputImp   (private implementation)
*      |       +- proxy classes ...
*      +- Filter
*      |   +- FilterImp   (private implementation)
*      |       +- proxy classes ...
*      +- Visual
*          +- VisualImp   (private implementation)
*              +- proxy classes ...
*
****************************************************************************/

/* Buffer size for compatibility interface */
#define BUFSIZE 16384

/* thread priorities for decoder thread */
#define DECODER_HIGH_PRIORITY_CLASS PRTYC_TIMECRITICAL
#define DECODER_HIGH_PRIORITY_DELTA 0
#define DECODER_LOW_PRIORITY_CLASS  PRTYC_FOREGROUNDSERVER
#define DECODER_LOW_PRIORITY_DELTA  0


class Plugin;
class PluginList;
class stringmap_own;

FLAGSATTRIBUTE(PLUGIN_TYPE);


class ModuleException
{ xstring Error;
 public:
  ModuleException(const char* fmt, ...);
  const xstring GetErrorText() const { return Error; }
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
  const xstring            Key;
  const xstring            ModuleName;
 protected:
  HMODULE                  HModule;
  PLUGIN_QUERYPARAM        QueryParam;
  /// Entry point of the configure dialog (if any).
  void DLLENTRYP(plugin_configure)(HWND hwnd, HMODULE module);
  /// Entry point of the configure dialog (if any).
  void DLLENTRYP(plugin_command)(const char* command, xstring* result);

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
  /// @param hwnd Parent window.
  void    Config(HWND hwnd) const            { DEBUGLOG(("Module(%p{%s})::Config(%p) - %p\n", this, Key.cdata(), hwnd, plugin_configure));
                                               if (plugin_configure) (*plugin_configure)(hwnd, HModule); }
  /// Handle plug-in specific command if the plug-in exports plugin_command.
  /// Otherwise the function is a no-op, result is unchanged.
  void    Command(const char* command, xstring& result) const
                                             { if (plugin_command) (*plugin_command)(command, &result); }

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
  static int_ptr<Module> GetByKey(const char* name);
  /// Gets a snapshot of all currently active plug-in instances.
  static void            CopyAllInstancesTo(vector_int<Module>& target);
};


/// Arguments of PluginChange event
struct PluginEventArgs
{ Plugin& Plug;
  enum event
  { /// The plug-in is unloaded by the user
    Unload,
    /// The plug-in is loaded by the user
    Load,
    /// The plug-in is enabled by the user
    Disable,
    /// The plug-in is instantiated by the plug-in manager
    Enable,
    /// The plug-in instance is destroyed by the plug-in manager
    Uninit,
    /// The plug-in is disabled by the user
    Init
  } Operation;
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
  Module&      ModRef;
  /// Kind of plug-in handled by the class instance.
  const PLUGIN_TYPE PluginType;
 protected:
  /// Enabled flag. \c true in doubt.
  /// @remarks Strictly speaking this property is a property o a plug-in list entry.
  /// But since a \c Plugin instance can only exist in a single PluginList,
  /// it is stored here to prevent further complexity.
  bool         Enabled;
 private:
  //static delegate<void, const CfgChangeArgs> ConfigDelegate;
 private:
  //static bool  Instantiate(Plugin* plugin, PluginList& list, const char* params);
  //static void  ConfigNotification(void*, const CfgChangeArgs& args);
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
  virtual bool SetParam(const char* param, const xstring& value);
  /// @brief Set the entire plug-in configuration.
  /// @details All identified keys are removed from map.
  /// The remaining keys are unknown.
  void         SetParams(stringmap_own& map);
  /// Serialize current configuration and name and append it to \a target.
  void         Serialize(xstringbuilder& target) const;

 protected:
   // The lists are protected by Module::Mtx.
   static PluginList Decoders; // only decoders
   static PluginList Outputs;  // only outputs
   static PluginList Filters;  // only filters
   static PluginList Visuals;  // only visuals
 private:
   /// Notify changes to the plug-in lists.
   static event<const PluginEventArgs> ChangeEvent;

 protected:
  static PluginList& GetPluginList(PLUGIN_TYPE type);
 public:
  /// Access plug-in change event.
  static event_pub<const PluginEventArgs>& GetChangeEvent() { return ChangeEvent; }

  /// Return the \c plug-in instance of the requested type on \a module. Create a new one if required.
  /// @exception ModuleException Something went wrong.
  static int_ptr<Plugin> GetInstance(Module& module, PLUGIN_TYPE type);
  /// @brief Create or update a plug-in from a string.
  /// @exception ModuleException Something went wrong.
  /// @remarks Note that the function does not add the plug-in
  /// to any list. But the parameters in the string are applied immediately,
  /// if the plug-in \e is already in a list.
  static int_ptr<Plugin> Deserialize(const char* str, PLUGIN_TYPE type);

  /// Put the list of enabled plug-ins into \a target.
  /// The plug-in type is detected automatically from the type of \a target.
  /// @param enabled Return only enabled plug-ins (default).
  static void  GetPlugins(PluginList& target, bool enabled = true);

  /// Append the plugin to the appropriate list.
  /// @exception ModuleException Something went wrong.
  static void  AppendPlugin(Plugin* plugin);
  /// Replace a plug-in list. This activates changes.
  /// The plug-in type is detected automatically from the type of \a source.
  /// @param source New plug-in list. Assigned by the old value on return.
  static void  SetPluginList(PluginList& source);

/*  static void  Init();
  static void  Uninit();*/
};


/***************************************************************************
 *
 * Collection of plug-ins of any kind.
 *
 * This class is not thread-safe.
 *
 ***************************************************************************/
class PluginList : public vector_int<Plugin>
{public:
  /// Type of the plug-ins in this list instance.
  const PLUGIN_TYPE Type;

 public:
  PluginList(PLUGIN_TYPE type) : Type(type) {}
  PluginList(const PluginList& r) : vector_int<Plugin>(r), Type(r.Type) {}
  PluginList&     operator=(const PluginList& r) { ASSERT(Type == r.Type); vector_int<Plugin>::operator=(r); return *this; }

  /*/// Append a new plug-in to the list.
  void            append(Plugin* plugin);*/
  /// Check whether a plug-in is already in the list.
  bool            contains(const Plugin* plugin) const { return find(plugin) != NULL; }

  /// Remove the plug-in from the list.
  /// @return true on success.
  bool            remove(Plugin* plugin);

  /// @brief Serialize plug-in list to a string.
  /// @details This implicitly calls Serialize for each plug-in in the list.
  const xstring   Serialize() const;
  /// Deserialize plug-in list from a string.
  /// @return \c NULL on success and an error text on error.
  xstring         Deserialize(const char* str);

  /// Reset the list to the default of this plug-in flavor.
  void            LoadDefaults();
};


/****************************************************************************
*
*  Helper class for plug-in virtualization proxies
*
****************************************************************************/

struct ProxyHelper
{protected:
  /// Convert file URL
  /// - discard file: prefix,
  /// - replace '/' by '\'
  /// - replace "X|" by "X:"
  /// The string url is modified in place
  static const char* ConvertUrl2File(char* url);

  /// Convert time stamp in seconds to an integer in milliseconds.
  /// Truncate leading bits in case of an overflow.
  static int    TstmpF2I(double pos);
  /// Convert possibly truncated time stamp in milliseconds to seconds.
  /// The missing bits are taken from a context time stamp which has to be sufficiently close
  /// to the original time stamp. Sufficient is about 24 days.
  static double TstmpI2F(int pos, double context);

  /// convert META_INFO into DECODER_INFO
  static void ConvertMETA_INFO(DECODER_INFO* dinfo, const volatile META_INFO* meta);
  /// convert DECODER_INFO2 to DECODER_INFO
  static void ConvertINFO_BUNDLE(DECODER_INFO* dinfo, const INFO_BUNDLE_CV* info);
  static void ConvertDECODER_INFO(const INFO_BUNDLE* info, const DECODER_INFO* dinfo);

  // global services
 protected:
  enum
  { UM_CREATEPROXYWINDOW = WM_USER,
    UM_DESTROYPROXYWINDOW
  };
 private:
  static HWND ServiceHwnd;
  friend MRESULT EXPENTRY ProxyHelperWinFn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 protected:
  static HWND CreateProxyWindow(const char* cls, void* ptr);
  static void DestroyProxyWindow(HWND hwnd);
 public:
  static void Init();
  static void Uninit();
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
