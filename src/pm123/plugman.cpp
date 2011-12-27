/*
 * Copyright 2006-2011 Marcel Mueller
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
 *
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

#define  INCL_PM
#define  INCL_ERRORS
#include "plugman.h"
#include "decoder.h"
#include "filter.h"
#include "output.h"
#include "visual.h"
#include "playable.h"
#include "commandprocessor.h"
#include "configuration.h"
#include "eventhandler.h"
#include "xstring_api.h"
#include "proxyhelper.h"
#include "pm123.h" // for startpath
#include <cpp/container/inst_index.h>
#include <cpp/url123.h>
#include <cpp/vdelegate.h>
#include <fileutil.h>

//#define DEBUG_LOG 1
#include <debuglog.h>


ModuleException::ModuleException(const char* fmt, ...)
{ va_list va;
  va_start(va, fmt);
  Error = xstring::vsprintf(fmt, va);
  va_end(va);
}


/****************************************************************************
*
* ModuleImp - Private implementation of class Module
*
****************************************************************************/
class ModuleImp : private Module
{private: // Static storage for plugin_init.
  PLUGIN_API               PluginApi;
  static const XSTRING_API XstringApi;
  PLUGIN_CONTEXT           Context;
 private:
  sco_ptr<ACommandProcessor> CommandInstance;
  VDELEGATE                vd_exec_command, vd_query_profile, vd_write_profile;

 private:
  // Proxy functions for PLUGIN_CONTEXT
  PROXYFUNCDEF int         DLLENTRY proxy_query_profile(ModuleImp* mp, const char* key, void* data, int maxlength);
  PROXYFUNCDEF int         DLLENTRY proxy_write_profile(ModuleImp* mp, const char* key, const void* data, int length);
  PROXYFUNCDEF const char* DLLENTRY proxy_exec_command (ModuleImp* mp, const char* cmd);
  PROXYFUNCDEF int         DLLENTRY proxy_obj_invalidate(const char* url, int what);
  PROXYFUNCDEF int         DLLENTRY proxy_obj_supported(const char* url, const char* eatype);
 private:
  ModuleImp(const xstring& key, const xstring& name) : Module(key, name) {}
  /// Load a new DLL as plug-in. The plug-in flavor is specialized later.
  /// @exception ModuleException Something went wrong.
  void           Load();
 public:
  virtual        ~ModuleImp();

 private: // Repository
  typedef char KeyType; // Hack! We use /references/ to const char as strings for optimum performance.
  static int     Comparer(const Module& module, const KeyType& key);
 public:
  static Module* Factory(const KeyType& key, const xstring& name);
  class Repository : public inst_index2<Module, const KeyType, &ModuleImp::Comparer, const xstring>
  {public:
    static Mutex& GetMtx()                  { return Mtx; }
    static const vector<Module>& GetIndex() { return Index; }
  };
};


const XSTRING_API ModuleImp::XstringApi =
{ (xstring DLLENTRYP()(const char*))&xstring_create,
  &xstring_free,
  &xstring_length,
  &xstring_equal,
  &xstring_compare,
  &xstring_copy,
  &xstring_copy_safe,
  &xstring_assign,
  &xstring_cmpassign,
  &xstring_append,
  &xstring_allocate,
  &xstring_sprintf,
  &xstring_vsprintf
};

Module* ModuleImp::Factory(const KeyType& key, const xstring& name)
{ ModuleImp* pm = new ModuleImp(&key, name);
  try
  { pm->Load();
  } catch (...)
  { delete pm;
    throw;
  }
  return pm;
}

int ModuleImp::Comparer(const Module& module, const KeyType& key)
{ return module.Key.compareToI(&key);
}

ModuleImp::~ModuleImp()
{ DEBUGLOG(("Module(%p{%s})::~Module()\n", this, Key.cdata()));
  if (HModule)
  {
    #ifdef DEBUG_LOG
    APIRET rc = DosFreeModule(HModule);
    if (rc != NO_ERROR && rc != ERROR_INVALID_ACCESS)
    { char error[1024];
      DEBUGLOG(("Module::~Module: Could not unload %s. Error %d\n%s\n",
        ModuleName.cdata(), rc, os2_strerror(rc, error, sizeof error)));
    }
    #else
    DosFreeModule(HModule);
    #endif
    HModule = NULLHANDLE;
  }
  ModuleImp::Repository::RemoveWithKey(*this, *Key.cdata());
  DEBUGLOG(("Module::~Module completed\n"));
}

PROXYFUNCIMP(int DLLENTRY, ModuleImp)
proxy_query_profile(ModuleImp* mp, const char* key, void* data, int maxlength)
{ ULONG len = maxlength;
  return PrfQueryProfileData(Cfg::GetHIni(), mp->Key, key, data, &len)
      && PrfQueryProfileSize(Cfg::GetHIni(), mp->Key, key, &len)
    ? (int)len : -1;
}

PROXYFUNCIMP(int DLLENTRY, ModuleImp)
proxy_write_profile(ModuleImp* mp, const char* key, const void* data, int length)
{ return PrfWriteProfileData(Cfg::GetHIni(), mp->Key, key, (PVOID)data, length);
}

PROXYFUNCIMP(const char* DLLENTRY, ModuleImp)
proxy_exec_command(ModuleImp* mp, const char* cmd)
{ // Initialize command processor on demand.
  if (mp->CommandInstance == NULL)
    mp->CommandInstance = ACommandProcessor::Create();
  return mp->CommandInstance->Execute(cmd);
}

PROXYFUNCIMP(int DLLENTRY, ModuleImp)
proxy_obj_invalidate(const char* url, int what)
{ int_ptr<Playable> pp = Playable::FindByURL(url);
  return pp ? pp->Invalidate((InfoFlags)what) : IF_None;
}

PROXYFUNCIMP(int DLLENTRY, ModuleImp)
proxy_obj_supported(const char* url, const char* eatype)
{ // Get list of decoders
  PluginList decoders(PLUGIN_DECODER);
  Plugin::GetPlugins(decoders);
  // Seek for match
  for (size_t i = 0; i < decoders.size(); i++)
  { Decoder* dp = (Decoder*)decoders[i].get();
    if (dp->IsFileSupported(url, eatype))
      return true;
  }
  return false;
}


void ModuleImp::Load()
{ DEBUGLOG(("Module(%p{%s})::Load()\n", this, Key.cdata()));

  char load_error[_MAX_PATH];
  *load_error = 0;
  APIRET rc = DosLoadModule(load_error, sizeof load_error, ModuleName, &HModule);
  DEBUGLOG(("Module::Load: %p - %u - %s\n", HModule, rc, load_error));
  // For some reason the API sometimes returns ERROR_INVALID_PARAMETER when loading oggplay.dll.
  // However, the module is loaded successfully.
  // Furthermore, once Firefox 3.5 has beed started at least once on the system loading os2audio.dll
  // or other MMOS2 related plug-ins fails with ERROR_INIT_ROUTINE_FAILED.
  if (rc != NO_ERROR && !(HModule != NULLHANDLE && (rc == ERROR_INVALID_PARAMETER || rc == ERROR_INIT_ROUTINE_FAILED)))
  { char error[1024];
    throw ModuleException("Could not load %s, %s. Error %d at %s",
      ModuleName.cdata(), load_error, rc, os2_strerror(rc, error, sizeof error));
  }

  int DLLENTRYP(pquery)(PLUGIN_QUERYPARAM *param);
  LoadMandatoryFunction(&pquery, "plugin_query");
  int DLLENTRYP(pinit)(const PLUGIN_CONTEXT* ctx);
  LoadOptionalFunction(&pinit, "plugin_init");

  QueryParam.interface = 0; // unchanged by old plug-ins
  (*pquery)(&QueryParam);

  if (QueryParam.interface > PLUGIN_INTERFACE_LEVEL)
  {
    #define toconststring(x) #x
    throw ModuleException("Could not load plug-in %s because it requires a newer version of the PM123 core\n"
                          "Requested interface revision: %d, max. supported: " toconststring(PLUGIN_INTERFACE_LEVEL),
                          ModuleName.cdata(), QueryParam.interface);
    #undef toconststring
  }

  if (pinit)
  { PluginApi.message_display = &PROXYFUNCREF(Module)PluginDisplayMessage;
    PluginApi.profile_query   = vdelegate(&((ModuleImp*)this)->vd_query_profile, &PROXYFUNCREF(ModuleImp)proxy_query_profile, (ModuleImp*)this);
    PluginApi.profile_write   = vdelegate(&((ModuleImp*)this)->vd_write_profile, &PROXYFUNCREF(ModuleImp)proxy_write_profile, (ModuleImp*)this);
    PluginApi.exec_command    = vdelegate(&((ModuleImp*)this)->vd_exec_command,  &PROXYFUNCREF(ModuleImp)proxy_exec_command,  (ModuleImp*)this);
    PluginApi.obj_invalidate  = &PROXYFUNCREF(ModuleImp)proxy_obj_invalidate;
    PluginApi.obj_supported   = &PROXYFUNCREF(ModuleImp)proxy_obj_supported;
    Context.plugin_api  = &PluginApi;
    Context.xstring_api = &XstringApi;
    
    int rc = (*pinit)(&Context);
    if (rc)
      throw ModuleException("Plugin %s returned an error at initialization. Code %d.",
                            ModuleName.cdata(), rc);
  }

  LoadOptionalFunction(&plugin_command, "plugin_command");
  if (QueryParam.configurable)
    LoadMandatoryFunction(&plugin_configure, "plugin_configure");
}


/****************************************************************************
*
* Module - object representing a plugin-DLL
*
****************************************************************************/

Mutex& Module::Mtx = ModuleImp::Repository::GetMtx();

Module::Module(const xstring& key, const xstring& name)
: Key(key)
, ModuleName(name)
, HModule(NULLHANDLE)
, plugin_configure(NULL)
, plugin_command(NULL)
{ DEBUGLOG(("Module(%p)::Module(%s)\n", this, name.cdata()));
  memset(&QueryParam, 0, sizeof QueryParam);
}

void Module::LoadMandatoryFunction(void* function, const char* function_name) const
{ APIRET rc = LoadOptionalFunction(function, function_name);
  if (rc != NO_ERROR)
  { char error[1024];
    throw ModuleException("Could not load \"%s\" from %s\n%s", function_name,
      ModuleName.cdata(), os2_strerror(rc, error, sizeof error));
  }
}

PROXYFUNCIMP(void DLLENTRY, Module) PluginDisplayMessage(MESSAGE_TYPE type, const char* msg)
{ EventHandler::Post(type, msg);
}


int_ptr<Module> Module::FindByKey(const char* name)
{ return ModuleImp::Repository::FindByKey(*sfnameext2(name));
}

int_ptr<Module> Module::GetByKey(const char* name)
{ xstring modulename;
  char* cp = modulename.allocate(strlen(name), name);
  // replace '/'
  for(;;)
  { cp = strchr(cp, '/');
    if (cp == NULL)
      break;
    *cp++ = '\\';
  }
  // make absolute path
  cp = strchr(modulename, '\\');
  if (cp == NULL || (cp != modulename.cdata() && cp[-1] != ':'))
    // relative path
    modulename = amp_startpath + modulename;
  // load module
  return ModuleImp::Repository::GetByKey(*sfnameext2(modulename), &ModuleImp::Factory, modulename);
}

void Module::CopyAllInstancesTo(vector_int<Module>& target)
{ target.clear();
  const vector<Module>& modules = ModuleImp::Repository::GetIndex();
  Mutex::Lock lock(ModuleImp::Repository::GetMtx());
  target.reserve(modules.size());
  Module*const* mpp = modules.begin();
  Module*const*const mpe = modules.end();
  while (mpp != mpe)
  { Module* mp = *mpp++;
    if (mp && !mp->RefCountIsUnmanaged())
      target.append() = mp;
  }
}


/****************************************************************************
*
* Plugin - object representing a plugin instance
*
****************************************************************************/

event<const PluginEventArgs> Plugin::ChangeEvent;

PluginList Plugin::Decoders(PLUGIN_DECODER); // only decoders
PluginList Plugin::Outputs (PLUGIN_OUTPUT);  // only outputs
PluginList Plugin::Filters (PLUGIN_FILTER);  // only filters
PluginList Plugin::Visuals (PLUGIN_VISUAL);  // only visuals


Plugin::Plugin(Module& mod, PLUGIN_TYPE type)
: ModRef(&mod)
, PluginType(type)
, Enabled(true)
{}

Plugin::~Plugin()
{ DEBUGLOG(("Plugin(%p{%s})::~Plugin\n", this, ModRef->Key.cdata()));
}

void Plugin::SetEnabled(bool enabled)
{ if (Enabled == enabled)
    return;
  Enabled = enabled;
  RaisePluginChange(enabled ? PluginEventArgs::Enable : PluginEventArgs::Disable);
}

void Plugin::RaisePluginChange(PluginEventArgs::event ev)
{ const PluginEventArgs ea = { *this, ev };
  ChangeEvent(ea);
}

void Plugin::GetParams(stringmap_own& params) const
{ static const xstring enparam = "enabled";
  params.get(enparam) = new stringmapentry(enparam, Enabled ? "yes" : "no");
}

bool Plugin::SetParam(const char* param, const xstring& value)
{ if (stricmp(param, "enabled") == 0)
  { bool* b = url123::parseBoolean(value);
    DEBUGLOG(("Plugin::SetParam: enabled = %u\n", b ? *b : -1));
    if (b)
    { SetEnabled(*b);
      return true;
    }
  }
  return false;
}

void Plugin::SetParams(stringmap_own& params)
{ stringmapentry*const* p = params.begin();
  while (p != params.end())
  { DEBUGLOG(("Plugin::SetParams: %s -> %s\n", (*p)->Key.cdata(), (*p)->Value ? (*p)->Value.cdata() : "<null>"));
    if (SetParam((*p)->Key, (*p)->Value))
      delete params.erase(p); // Implicitly updates p
    else
      ++p;
  }
}

void Plugin::Serialize(xstringbuilder& target) const
{ size_t start = target.length();
  const xstring& modulename = ModRef->ModuleName;
  target.append(modulename);
  if (modulename.startsWithI(amp_startpath))
    target.erase(start, amp_startpath.length());
  stringmap_own sm(20);
  GetParams(sm);
  if (sm.size())
  { target.append('?');
    start = target.length();
    url123::appendParameter(target, sm);
  }
}

PluginList& Plugin::GetPluginList(PLUGIN_TYPE type)
{ switch(type)
  {case PLUGIN_DECODER:
    return Decoders;
   case PLUGIN_FILTER:
    return Filters;
   case PLUGIN_OUTPUT:
    return Outputs;
   case PLUGIN_VISUAL:
    return Visuals;
   default:;
  }
  ASSERT(false);
  return *(PluginList*)NULL; // This must never happen!
}

int_ptr<Plugin> Plugin::FindInstance(Module& module, PLUGIN_TYPE type)
{ switch (type)
  {case PLUGIN_DECODER:
    return Decoder::FindInstance(module).get();
   case PLUGIN_FILTER:
    return Filter::FindInstance(module).get();
   case PLUGIN_OUTPUT:
    return Output::FindInstance(module).get();
   case PLUGIN_VISUAL:
    return Visual::FindInstance(module).get();
   default:;
  }
  ASSERT(false);
  return NULL;
}

int_ptr<Plugin> Plugin::GetInstance(Module& module, PLUGIN_TYPE type)
{ switch (type)
  {case PLUGIN_DECODER:
    return Decoder::GetInstance(module).get();
   case PLUGIN_FILTER:
    return Filter::GetInstance(module).get();
   case PLUGIN_OUTPUT:
    return Output::GetInstance(module).get();
   case PLUGIN_VISUAL:
    return Visual::GetInstance(module).get();
   default:;
  }
  ASSERT(false);
  return NULL;
}

int_ptr<Plugin> Plugin::Deserialize(const char* str, PLUGIN_TYPE type)
{ DEBUGLOG(("Plugin::Deserialize(%s, %x)\n", str, type));
  const char* params = strchr(str, '?');
  if (params)
  { const size_t len = params-str;
    char* const tmp = (char*)alloca(len+1);
    memcpy(tmp, str, len);
    tmp[len] = 0;
    str = tmp;
  }
  // load module
  int_ptr<Module> pm = Module::GetByKey(str);
  int_ptr<Plugin> pp = Plugin::GetInstance(*pm, type);
  if (params)
  { stringmap_own sm(20);
    url123::parseParameter(sm, params+1);
    pp->SetParams(sm);
    #ifdef DEBUG_LOG
    for (stringmapentry** p = sm.begin(); p != sm.end(); ++p)
      DEBUGLOG(("Plugin::SetParams: invalid parameter %s -> %s\n", (*p)->Key.cdata(), (*p)->Value.cdata()));
    #endif
  }
  return pp;
}

void Plugin::GetPlugins(PluginList& target, bool enabled)
{ const PluginList& source = GetPluginList(target.Type);
  Mutex::Lock lock(Module::Mtx);
  if (enabled)
  { target.clear();
    const int_ptr<Plugin>* ppp = source.begin();
    const int_ptr<Plugin>*const ppe = source.end();
    while (ppp != ppe)
    { if ((*ppp)->Enabled)
        target.append() = *ppp;
      ++ppp;
    }
  } else
    target = source;
}

void Plugin::AppendPlugin(Plugin* plugin)
{ PluginList& target = GetPluginList(plugin->PluginType);
  Mutex::Lock lock(Module::Mtx);
  if (target.contains(plugin))
    throw ModuleException("Tried to load the plug-in %s twice.", plugin->ModRef->Key.cdata());
  plugin->RaisePluginChange(PluginEventArgs::Load);
  target.append() = plugin;
}

void Plugin::SetPluginList(PluginList& source)
{ PluginList& target = GetPluginList(source.Type);
  Mutex::Lock lock(Module::Mtx);
  // check what's removed
  const int_ptr<Plugin>* ppp = target.begin();
  const int_ptr<Plugin>* ppe = target.end();
  while (ppp != ppe)
  { if (!source.contains(*ppp))
      (*ppp)->RaisePluginChange(PluginEventArgs::Unload);
    ++ppp;
  }
  // assign
  target.swap(source);
  // check what's new
  ppp = target.begin();
  ppe = target.end();
  while (ppp != ppe)
  { if (!source.contains(*ppp))
      (*ppp)->RaisePluginChange(PluginEventArgs::Load);
    ++ppp;
  }
}

/*void Plugin::Init()
{ Cfg::GetChange() += ConfigDelegate;
}

void Plugin::Uninit()
{ ConfigDelegate.detach();
}*/


/****************************************************************************
*
* PluginList collection
*
****************************************************************************/

/*bool PluginList::remove(Plugin* plugin)
{ DEBUGLOG(("PluginList(%p)::remove(%p)\n", this, plugin));
  const int_ptr<Plugin>* ppp = find(plugin);
  if (!ppp)
    return false;
  erase(ppp);
  return true;
}*/

const xstring PluginList::Serialize() const
{ DEBUGLOG(("PluginList::Serialize() - %u\n", size()));
  xstringbuilder result;
  const int_ptr<Plugin>* ppp = begin();
  const int_ptr<Plugin>* const ppe = end();
  while (ppp != ppe)
  { (*ppp)->Serialize(result);
    result += '\n';
    ++ppp;
  }
  return result;
}

xstring PluginList::Deserialize(const char* str)
{ clear();
  xstringbuilder err;
  // Now parse the string to create the new plug-in list.
  const char* sp = str;
  while (*sp)
  { const char* cp = strchr(sp, '\n');
    if (cp == NULL)
      cp = sp + strlen(sp);
    try
    { int_ptr<Plugin> pp = Plugin::Deserialize(xstring(sp, cp-sp), Type);
      if (contains(pp))
      { err.appendf("Tried to load plug-in %s twice.\n", pp->ModRef->Key.cdata());
        goto next;
      }
      append() = pp;
    } catch (const ModuleException& ex)
    { err.append(ex.GetErrorText());
      err.append('\n');
    }
   next:
    if (*cp == NULL)
      break;
    sp = cp +1;
  }
  return err.length() ? err : xstring();
}

void PluginList::LoadDefaults()
{ const char* def;
  switch (Type)
  {default:
    clear();
    return;
   case PLUGIN_DECODER:
    def = Cfg::Default.decoders_list;
    break;
   case PLUGIN_FILTER:
    def = Cfg::Default.filters_list;
    break;
   case PLUGIN_OUTPUT:
    def = Cfg::Default.outputs_list;
    break;
   case PLUGIN_VISUAL:
    def = Cfg::Default.visuals_list;
    break;
  }
  Deserialize(def);
}


/****************************************************************************
*
*  Global initialization functions
*
****************************************************************************/

void plugman_init()
{ ProxyHelper::Init();
  Decoder::Init();
  Output::Init();
  /* TODO!
   * if (Outputs.Current() == NULL && Outputs.size())
    Outputs.SetActive(1);*/
  //Plugin::Init();
}

void plugman_uninit()
{ // remove all plugins
  /* TODO!
   *
   Visuals.clear();
  Decoders.clear();
  Filters.clear();
  Outputs.clear();*/
  // deinitialize framework
  //Plugin::Uninit();
  Output::Uninit();
  Decoder::Uninit();
  ProxyHelper::Uninit();
}
