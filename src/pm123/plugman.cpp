/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
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

#define  INCL_PM
#define  INCL_ERRORS
#include "plugman.h"
#include "plugman_base.h"
#include "123_util.h"
#include "pm123.h" // for startpath
#include "dialog.h"
#include "properties.h"
#include "pm123.rc.h"
#include <utilfct.h>
#include <fileutil.h>
#include <cpp/mutex.h>
#include <cpp/url123.h>
#include <cpp/stringmap.h>
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

//#define DEBUG_LOG 1
#include <debuglog.h>

/* thread priorities for decoder thread */
#define  DECODER_HIGH_PRIORITY_CLASS PRTYC_FOREGROUNDSERVER // sorry, we should not lockup the system with time-critical priority
#define  DECODER_HIGH_PRIORITY_DELTA 20
#define  DECODER_LOW_PRIORITY_CLASS  PRTYC_REGULAR
#define  DECODER_LOW_PRIORITY_DELTA  1


/****************************************************************************
*
* global lists
*
****************************************************************************/

PluginList1 Decoders(PLUGIN_DECODER, // only decoders
  "oggplay.dll?enabled=true\n"
  "mpg123.dll?enabled=true\n"
  "wavplay.dll?enabled=true&filetypes=Digital Audio\n"
  "cddaplay.dll?enabled=true\n"
  "os2rec.dll?enabled=true\n");
PluginList1 Outputs(PLUGIN_OUTPUT,   // only outputs
  "os2audio.dll?enabled=true\n"
  "wavout.dll?enabled=true\n"
  "os2audio.dll"); // active output
PluginList  Filters(PLUGIN_FILTER,   // only filters
  "realeq.dll?enabled=true\n"
  "logvolum.dll\n");
PluginList  Visuals(PLUGIN_VISUAL, ""); // only visuals
PluginList  VisualsSkinned(PLUGIN_VISUAL, ""); // visual plug-ins loaded by skin


/****************************************************************************
*
* Module - object representing a plugin-DLL
*
****************************************************************************/

Module* Module::ModuleFactory::operator()(const xstring& key)
{ Module* pm = new Module(key);
  if (!pm->Load())
  { delete pm;
    return NULL;
  } else
    return pm;
}

Module::Module(const xstring& name)
: inst_index<Module, const xstring>(name),
  HModule(NULLHANDLE),
  plugin_configure(NULL)
{ DEBUGLOG(("Module(%p)::Module(%s)\n", this, name.cdata()));
  memset(&QueryParam, 0, sizeof QueryParam);
}

Module::~Module()
{ DEBUGLOG(("Module(%p{%s})::~Module()\n", this, GetModuleName().cdata()));
  ASSERT(RefCountIsUnmanaged()); // Module must not be in use!
  UnloadModule();
  DEBUGLOG(("Module::~Module() completed\n"));
}

int Module::compareTo(const xstring& key) const
{ DEBUGLOG(("Module(%p{%s})::compareTo(&%s)\n", this, GetModuleName().cdata(), key.cdata()));
  return stricmp(sfnameext2(GetModuleName()), sfnameext2(key));
}

/* Assigns the address of the specified procedure within a plug-in. */
bool Module::LoadOptionalFunction(void* function, const char* function_name) const
{ return DosQueryProcAddr(HModule, 0L, (PSZ)function_name, (PFN*)function) == NO_ERROR;
}

/* Assigns the address of the specified procedure within a plug-in. */
bool Module::LoadFunction(void* function, const char* function_name) const
{ ULONG rc = DosQueryProcAddr(HModule, 0L, (PSZ)function_name, (PFN*)function);
  if (rc != NO_ERROR)
  { char error[1024];
    *((ULONG*)function) = 0;
    amp_player_error("Could not load \"%s\" from %s\n%s", function_name,
                     GetModuleName().cdata(), os2_strerror(rc, error, sizeof error));
    return FALSE;
  }
  return TRUE;
}

/* Loads a plug-in dynamic link module. */
bool Module::LoadModule()
{ char load_error[_MAX_PATH];
  *load_error = 0;
  DEBUGLOG(("Module(%p{%s})::LoadModule()\n", this, GetModuleName().cdata()));
  APIRET rc = DosLoadModule(load_error, sizeof load_error, GetModuleName().cdata(), &HModule);
  DEBUGLOG(("Module::LoadModule: %p - %u\n", HModule, rc));
  // For some reason the API sometimes returns ERROR_INVALID_PARAMETER when loading oggplay.dll.
  // However, the module is loaded successfully.
  if (rc != NO_ERROR && rc != ERROR_INVALID_PARAMETER)
  { char error[1024];
    amp_player_error("Could not load %s, %s. Error %d\n%s",
                     GetModuleName().cdata(), load_error, rc, os2_strerror(rc, error, sizeof error));
    return false;
  }
  DEBUGLOG(("Module({%p,%s})::LoadModule: TRUE\n", HModule, GetModuleName().cdata()));
  return true;
}

/* Unloads a plug-in dynamic link module. */
bool Module::UnloadModule()
{ DEBUGLOG(("Module(%p{%p,%s}))::UnloadModule()\n", this, HModule, GetModuleName().cdata()));
  if (HModule == NULLHANDLE)
    return true;
  APIRET rc = DosFreeModule(HModule);
  if (rc != NO_ERROR && rc != ERROR_INVALID_ACCESS)
  { char  error[1024];
    amp_player_error("Could not unload %s. Error %d\n%s",
                     GetModuleName().cdata(), rc, os2_strerror(rc, error, sizeof error));
    return false;
  }
  HModule = NULLHANDLE;
  return true;
}

/* Fills the basic properties of any plug-in.
         module:      input
         ModuleName: input
         query_param  output
         plugin_configure output
   return FALSE = error */
bool Module::Load()
{ DEBUGLOG(("Module(%p{%s})::Load()\n", this, GetModuleName().cdata()));

  void DLLENTRYP(plugin_query)(PLUGIN_QUERYPARAM *param);
  if (!LoadModule() || !LoadFunction(&plugin_query, "plugin_query"))
    return false;

  QueryParam.interface = 0; // unchanged by old plug-ins
  (*plugin_query)(&QueryParam);

  if (QueryParam.interface > MAX_PLUGIN_LEVEL)
  {
    #define toconststring(x) #x
    amp_player_error( "Could not load plug-in %s because it requires a newer version of the PM123 core\n"
                      "Requested interface revision: %d, max. supported: " toconststring(MAX_PLUGIN_LEVEL),
                      GetModuleName().cdata(), QueryParam.interface);
    #undef toconststring
    return FALSE;
  }

  return !QueryParam.configurable || LoadFunction(&plugin_configure, "plugin_configure");
}


/****************************************************************************
*
* Plugin - object representing a plugin instance
*
****************************************************************************/

// statics
event<const Plugin::EventArgs> Plugin::ChangeEvent;
VISUAL_PROPERTIES Plugin::VisualProps;
HWND Plugin::ServiceHwnd = NULLHANDLE;


Plugin::Plugin(Module* mod)
: ModRef(mod),
  Enabled(true)
{}

Plugin::~Plugin()
{ DEBUGLOG(("Plugin(%p{%s})::~Plugin\n", this, GetModuleName().cdata()));
}

const xstring& Plugin::GetModuleName() const
{ return GetModule().GetModuleName();
}

void Plugin::SetEnabled(bool enabled)
{ if (Enabled == enabled)
    return;
  Enabled = enabled;
  RaisePluginChange(enabled ? EventArgs::Enable : EventArgs::Disable);
}

void Plugin::RaisePluginChange(EventArgs::event ev)
{ const EventArgs ea = { *this, ev };
  ChangeEvent(ea);
}

MRESULT EXPENTRY cl_plugin_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DEBUGLOG(("cl_plugin_winfn(%p, %u, %p, %p)\n", hwnd, msg, mp1, mp2));
  switch (msg)
  {case Plugin::UM_CREATEPROXYWINDOW:
    { HWND proxyhwnd = WinCreateWindow(hwnd, (PSZ)PVOIDFROMMP(mp1), "", 0, 0,0, 0,0, NULLHANDLE, HWND_BOTTOM, 42, NULL, NULL);
      PMASSERT(proxyhwnd != NULLHANDLE);
      PMRASSERT(WinSetWindowPtr(proxyhwnd, QWL_USER, PVOIDFROMMP(mp2)));
      return (MRESULT)proxyhwnd;
    }
   case Plugin::UM_DESTROYPROXYWINDOW:
    WinDestroyWindow(HWNDFROMMP(mp1));
    return 0;
   case WM_DESTROY:
    Plugin::ServiceHwnd = NULLHANDLE;
    break;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

HWND Plugin::CreateProxyWindow(const char* cls, void* ptr)
{ return (HWND)WinSendMsg(ServiceHwnd, UM_CREATEPROXYWINDOW, MPFROMP(cls), MPFROMP(ptr));
}

void Plugin::DestroyProxyWindow(HWND hwnd)
{ WinSendMsg(ServiceHwnd, UM_DESTROYPROXYWINDOW, MPFROMHWND(hwnd), 0);
}

void Plugin::Init()
{ PMRASSERT(WinRegisterClass(amp_player_hab(), "CL_MODULE_SERVICE", &cl_plugin_winfn, 0, 0));
  ServiceHwnd = WinCreateWindow(HWND_OBJECT, "CL_MODULE_SERVICE", "", 0, 0,0, 0,0, amp_player_window(), HWND_BOTTOM, 42, NULL, NULL);
  PMASSERT(ServiceHwnd != NULLHANDLE);
}

void Plugin::Uninit()
{ WinDestroyWindow(ServiceHwnd);
}


Plugin* Plugin::Instantiate(Module* mod, Plugin* (*factory)(Module* mod), PluginList& list, const char* params)
{ DEBUGLOG(("Plugin::Instantiate(%p{%s}, %p, %p, %s)\n", mod, mod->GetModuleName().cdata(), factory, &list, params ? params : "<null>"));

  if (list.find(mod) != -1)
  { pm123_display_error( xstring::sprintf("Tried to load the plug-in %s twice.\n", mod->GetModuleName().cdata()) );
    return NULL;
  }
  Plugin* pp = (*factory)(mod);
  if (!pp || !pp->LoadPlugin())
  { delete pp;
    DEBUGLOG(("Plugin::Instantiate: failed.\n"));
    return NULL;
  }
  list.append(pp);
  pp->SetParams(params);
  return pp;
}

void Plugin::GetParams(stringmap& params) const
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
      delete params.erase(p);
    else
      ++p;
  }
}

void Plugin::SetParams(const char* params)
{ // TODO: a on the fly parser would be better.
  stringmap_own sm(20);
  url123::parseParameter(sm, params);
  SetParams(sm);
  #ifdef DEBUG_LOG
  for (stringmapentry** p = sm.begin(); p != sm.end(); ++p)
    DEBUGLOG(("Plugin::SetParams: invalid parameter %s -> %s\n", (*p)->Key.cdata(), (*p)->Value ? (*p)->Value.cdata() : "<null>"));
  #endif
}

xstring Plugin::Serialize() const
{ stringmap_own sm(20);
  GetParams(sm);
  const xstring& params = url123::makeParameter(sm);
  return params ? xstring::sprintf("%s?%s", GetModuleName().cdata(), params.cdata()) : GetModuleName();
}

int Plugin::Deserialize(const char* str, int mask, bool skinned)
{ DEBUGLOG(("Plugin::Deserialize(%s, %x)\n", str, mask));
  const char* params = strchr(str, '?');
  // make absolute path
  xstring modulename;
  char* cp = modulename.raw_init(params ? params-str : strlen(str), str);
  // replace '/'
  { for(;;)
    { cp = strchr(cp, '/');
      if (cp == NULL)
        break;
      *cp++ = '\\';
  } }
  if (strchr(modulename, ':') == NULL && modulename[0U] != '\\' && modulename[0U] != '/')
    // relative path
    modulename = startpath + modulename;
  // load module
  int_ptr<Module> pm = Module::GetByKey(modulename);
  if (pm == NULL)
    return 0;
  // load as plugin
  int result = 0;
  mask &= pm->GetParams().type;
  // decoder
  if ((mask & PLUGIN_DECODER) && Instantiate(pm, &Decoder::Factory, Decoders, params))
    result |= PLUGIN_DECODER;
  // output
  if ((mask & PLUGIN_OUTPUT) && Instantiate(pm, &Output::Factory, Outputs, params))
  { if (Outputs.Current() == NULL)
      Outputs.SetActive(Outputs.size() -1);
    result |= PLUGIN_OUTPUT;
  }
  // filter
  if ((mask & PLUGIN_FILTER) && Instantiate(pm, &Filter::Factory, Filters, params))
    result |= PLUGIN_FILTER;
  // visual
  if (mask & PLUGIN_VISUAL)
  { PluginList& list = skinned ? VisualsSkinned : Visuals;
    int q = list.find(pm.get());
    DEBUGLOG(("Plugin::Deserialize: visuals.find returned %i\n", q));
    if (q != -1)
    { // existing plugin. update some values
      ((Visual*)list[q])->SetProperties(&VisualProps);
    } else
    { Plugin* pp = Instantiate(pm, &Visual::Factory, list, params);
      if (pp)
      { ((Visual*)pp)->SetProperties(&VisualProps);
        result |= PLUGIN_VISUAL;
      }
    }
  }

  DEBUGLOG(("Plugin::Deserialize: %x\n", result));
  return result;
}


/****************************************************************************
*
* PluginList collection
*
****************************************************************************/

void PluginList::append(Plugin* plugin)
{ vector_own<Plugin>::append() = plugin;
  plugin->RaisePluginChange(Plugin::EventArgs::Load);
}

Plugin* PluginList::erase(size_t i)
{ DEBUGLOG(("PluginList(%p)::erase(%u)\n", this, i));

  if (i > size())
  { DEBUGLOG(("PluginList::erase: index out of range\n"));
    return NULL;
  }
  if ((*this)[i]->IsInitialized() && !(*this)[i]->UninitPlugin())
  { DEBUGLOG(("PluginList::erase: plugin %s failed to uninitialize.\n", (*this)[i]->GetModuleName().cdata()));
    return NULL;
  }
  Plugin* plugin = vector_own<Plugin>::erase(i);
  plugin->RaisePluginChange(Plugin::EventArgs::Unload);
  return plugin;
}

bool PluginList::remove(int i)
{ Plugin* pp = erase(i);
  delete pp;
  return pp != NULL;
}

int PluginList::find(const Plugin* plugin) const
{ DEBUGLOG(("PluginList(%p)::find(Plugin* %p)\n", this, plugin));
  for (size_t i = 0; i < size(); ++i)
    if ((*this)[i] == plugin)
      return i;
  return -1;
}
int PluginList::find(const Module* module) const
{ DEBUGLOG(("PluginList(%p)::find(Module* %p)\n", this, module));
  for (size_t i = 0; i < size(); ++i)
    if (&(*this)[i]->GetModule() == module)
      return i;
  return -1;
}
int PluginList::find(const char* module) const
{ DEBUGLOG(("PluginList(%p)::find(%s)\n", this, module));
  module = sfnameext2(module);
  for (size_t i = 0; i < size(); ++i)
    if (stricmp(sfnameext2((*this)[i]->GetModuleName()), module) == 0)
      return i;
  return -1;
}

xstring PluginList::Serialize() const
{ DEBUGLOG(("PluginList::Serialize() - %u\n", size()));
  xstring result = xstring::empty;
  for (Plugin*const* pp = begin(); pp < end(); ++pp)
    // TODO: again a strungbuilder would be nice
    result = result + (*pp)->Serialize() + "\n";
  return result;
}

PluginList::RC PluginList::Deserialize(const xstring& str)
{ // Check which plug-ins are no longer in the list.
  const char* sp = str;
  int i;
  { sco_arr<bool> keep(new bool[size()]);
    memset(keep.get(), false, size() * sizeof(bool));
    for(;;)
    { const char* cp = strchr(sp, '\n');
      if (cp == NULL)
        break;
      const char* cp2 = strnchr(sp, '?', cp-sp);
      if (cp2 == NULL)
        cp2 = cp;
      // now [sp..cp2) is the module name.
      i = find(xstring(sp, cp2-sp));
      if (i >= 0)
      { if (keep[i])
          return RC_Error;
        keep[i] = true;
      }
      sp = cp +1;
    }
    // Check if all plug-ins with keep = false are unused.
    // TODO: there is a threading issung for decoder plug-ins when working in playlist mode.
    for (i = 0; i < size(); ++i)
      if (!keep[i] && (*this)[i]->IsInitialized())
        return RC_InUse;
    // delete unused plug-ins
    // We do this back to front to avoid inconsistencies.
    for (i = size(); i-- != 0; )
      if (!keep[i])
        remove(i);
  }
  // parse new plug-ins
  sp = str;
  for(i = 0; ; ++i)
  { const char* cp = strchr(sp, '\n');
    if (cp == NULL)
      return RC_OK; // ignore incomplete lines
    const char* cp2 = strnchr(sp, '?', cp-sp);
    if (cp2 == NULL)
      cp2 = cp;
    // now [sp..cp2) is the module name.
    // search for existing ones
    int j = find(xstring(sp, cp2-sp));
    if (j >= 0)
    { // match! => move an set parameters
      move(j, i);
      if (cp != cp2)
        (*this)[i]->SetParams(xstring(cp2+1, cp-cp2-1));
    } else if (Plugin::Deserialize(xstring(sp, cp-sp), Type) == 0)
      return RC_Error;
    sp = cp +1;
  }
}

/****************************************************************************
*
* PluginList1 - collection with one active plugin
*
****************************************************************************/

Plugin* PluginList1::erase(size_t i)
{ Plugin* pp = PluginList::erase(i);
  if ( pp == Active )
  { pp->RaisePluginChange(Plugin::EventArgs::Inactive);
    Active = NULL;
  }
  return pp;
}

int PluginList1::SetActive(int i)
{ if ( i >= size() || i < -1 )
    return -1;

  Plugin* pp = i == -1 ? NULL : (*this)[i];
  if (pp == Active)
    return 0;

  if (Active != NULL)
  { Plugin* ppold = xchg(Active, (Plugin*)NULL);
    if (ppold)
    { ppold->RaisePluginChange(Plugin::EventArgs::Inactive);
      ppold->UninitPlugin();
    }
  }  

  if (pp != NULL)
  { if (!pp->GetEnabled())
      return -2;
    if (!pp->InitPlugin())
      return -1;
    pp->RaisePluginChange(Plugin::EventArgs::Active);
  }
  Active = pp;
  return 0;
}

xstring PluginList1::Serialize() const
{ DEBUGLOG(("PluginList1::Serialize() - %p\n", Active));
  const xstring& ret = PluginList::Serialize();
  return Active ? ret + Active->GetModuleName() : ret;
}

PluginList::RC PluginList1::Deserialize(const xstring& str)
{ const char* cp = strrchr(str, '\n');
  if (cp == NULL)
    return RC_Error;
  SetActive(-1);
  RC r = PluginList::Deserialize(str);
  if (r != RC_OK)
    return r;
  int i = find(cp+1);
  if (i >= 0)
    SetActive(i);
  return RC_OK;
}


/****************************************************************************
*
* GUI stuff
*
****************************************************************************/

/* Plug-in menu in the main pop-up menu */
typedef struct {
   xstring filename;
   int   type;
   int   i;
   int   configurable;
   int   enabled;
} PLUGIN_ENTRY;

static sco_arr<PLUGIN_ENTRY> entries;
static size_t num_entries;

void
load_plugin_menu( HWND hMenu )
{
  char     buffer[2048];
  char     file[_MAX_PATH];
  MENUITEM mi;
  size_t   i;
  DEBUGLOG(("load_plugin_menu(%p)\n", hMenu));

  // Delete all
  size_t count = LONGFROMMR( WinSendMsg( hMenu, MM_QUERYITEMCOUNT, 0, 0 ));
  for( i = 0; i < count; i++ ) {
    short item = LONGFROMMR( WinSendMsg( hMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT(0), 0 ));
    WinSendMsg( hMenu, MM_DELETEITEM, MPFROM2SHORT( item, FALSE ), 0);
  }
  { // Fetch list of plug-ins atomically
    const Module::IXAccess ix;
    num_entries = ix->size();
    entries = new PLUGIN_ENTRY[num_entries];

    for( i = 0; i < num_entries; i++ )
    { Module* plug = (*ix)[i];
      sprintf( buffer, "%s (%s)", plug->GetParams().desc,
               sfname( file, plug->GetModuleName(), sizeof( file )));

      entries[i].filename     = buffer;
      entries[i].type         = plug->GetParams().type;
      entries[i].i            = i;
      entries[i].configurable = plug->GetParams().configurable;
      entries[i].enabled      = FALSE;
      if (plug->GetParams().type & PLUGIN_VISUAL)
      { int p = Visuals.find(plug);
        if (p != -1 && Visuals[p]->GetEnabled())
        { entries[i].enabled = TRUE;
          continue;
        }
      }
      if (plug->GetParams().type & PLUGIN_DECODER)
      { int p = Decoders.find(plug);
        if (p != -1 && Decoders[p]->GetEnabled())
        { entries[i].enabled = TRUE;
          continue;
        }
      }
      if (plug->GetParams().type & PLUGIN_OUTPUT)
      { if (Outputs.Current() != NULL && &Outputs.Current()->GetModule() == plug)
        { entries[i].enabled = TRUE;
          continue;
        }
      }
      if (plug->GetParams().type & PLUGIN_FILTER)
      { int p = Filters.find(plug);
        if (p != -1 && Filters[p]->GetEnabled())
        { entries[i].enabled = TRUE;
          continue;
        }
      }
    }
  }

  for( i = 0; i < num_entries; i++ )
  {
    mi.iPosition = MIT_END;
    mi.afStyle = MIS_TEXT;
    mi.afAttribute = 0;

    if( !entries[i].configurable ) {
      mi.afAttribute |= MIA_DISABLED;
    }
    if( entries[i].enabled ) {
      mi.afAttribute |= MIA_CHECKED;
    }

    mi.id = IDM_M_PLUG + i + 1;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem = 0;
    WinSendMsg( hMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP( entries[i].filename.cdata() ));
    DEBUGLOG2(("load_plugin_menu: add \"%s\"\n", entries[i].filename.cdata()));
  }

  if( num_entries == 0 )
  { mi.iPosition   = MIT_END;
    mi.afStyle     = MIS_TEXT;
    mi.afAttribute = MIA_DISABLED;
    mi.id          = IDM_M_BOOKMARKS + 1;
    mi.hwndSubMenu = (HWND)NULLHANDLE;
    mi.hItem       = 0;
    WinSendMsg( hMenu, MM_INSERTITEM, MPFROMP( &mi ), MPFROMP( "No plug-ins" ));
    return;
  }

  return;
}

void
dec_append_load_menu( HWND hMenu, ULONG id_base, SHORT where, DECODER_WIZZARD_FUNC* callbacks, int size )
{ DEBUGLOG(("dec_append_load_menu(%p, %d, %d, %p, %d)\n", hMenu, id_base, callbacks, size));
  int i;
  // cleanup
  SHORT lastcount = -1;
  for (i = 0; i < size; ++i)
  { SHORT newcount = SHORT1FROMMP(WinSendMsg(hMenu, MM_DELETEITEM, MPFROM2SHORT(id_base+i, FALSE), 0));
    DEBUGLOG(("dec_append_load_menu - %i %i\n", i, newcount));
    if (newcount == lastcount)
      break;
    lastcount = newcount;
  }
  // for all decoder plug-ins...
  for (i = 0; i < Decoders.size(); ++i)
  { const Decoder* dec = (Decoder*)Decoders[i];
    if (dec->GetEnabled() && dec->GetProcs().decoder_getwizzard)
    { const DECODER_WIZZARD* da = (*dec->GetProcs().decoder_getwizzard)();
      DEBUGLOG(("dec_append_load_menu: %s - %p\n", dec->GetModuleName().cdata(), da));
      MENUITEM mi = {0};
      mi.iPosition   = where;
      mi.afStyle     = MIS_TEXT;
      //mi.afAttribute = 0;
      //mi.hwndSubMenu = NULLHANDLE;
      //mi.hItem       = 0;
      for (; da != NULL; da = da->link, ++id_base)
      { if (size-- == 0)
          return; // too many entries, can't help
        // Add menu item
        mi.id        = id_base;
        SHORT pos = SHORT1FROMMR(WinSendMsg(hMenu, MM_INSERTITEM, MPFROMP(&mi), MPFROMP(da->prompt)));
        DEBUGLOG(("dec_append_load_menu: add %u: %s -> %p => %u\n", id_base, da->prompt, da->wizzard, pos));
        // Add callback function
        *callbacks++ = da->wizzard;
        if (mi.iPosition != MIT_END)
          ++mi.iPosition;
      }
    }
  }
}

void dec_append_accel_table( HACCEL& haccel, ULONG id_base, LONG offset, DECODER_WIZZARD_FUNC* callbacks, int size )
{ DEBUGLOG(("dec_append_accel_table(%p, %u, %u, %p, %u)\n", haccel, id_base, offset, callbacks, size));
  // Fetch content
  ULONG accelsize = WinCopyAccelTable(haccel, NULL, 0);
  PMASSERT(accelsize);
  accelsize += (size << (offset != 0)) * sizeof(ACCEL); // space for plug-in entries 
  ACCELTABLE* paccel = (ACCELTABLE*)alloca(accelsize);
  PMRASSERT(WinCopyAccelTable(haccel, paccel, accelsize));
  DEBUGLOG(("dec_append_accel_table: %i\n", paccel->cAccel));
  bool modified = false;
  // Append plug-in accelerators
  for (int i = 0; i < Decoders.size(); ++i)
  { Decoder* dec = (Decoder*)Decoders[i];
    if (dec->GetEnabled() && dec->GetProcs().decoder_getwizzard)
    { const DECODER_WIZZARD* da = (*dec->GetProcs().decoder_getwizzard)();
      DEBUGLOG(("dec_append_accel_table: %s - %p\n", dec->GetModuleName().cdata(), da));
      for (; da != NULL; da = da->link, ++id_base)
      { if (size-- == 0)
          goto nomore; // too many entries, can't help
        DEBUGLOG(("dec_append_accel_table: at %u: %s -> %x -> %p\n", id_base, da->prompt, da->accel_key, da->wizzard));
        *callbacks++ = da->wizzard;
        if (da->accel_key)
        { // Add table entry
          ACCEL& accel = paccel->aaccel[paccel->cAccel++];
          accel.fs  = da->accel_opt;
          accel.key = da->accel_key;
          accel.cmd = id_base;
          DEBUGLOG(("dec_append_accel_table: add {%x, %x, %i}\n", accel.fs, accel.key, accel.cmd));
          modified = true;
        }
        if (da->accel_key2 && offset)
        { // Add table entry
          ACCEL& accel = paccel->aaccel[paccel->cAccel++];
          accel.fs  = da->accel_opt2;
          accel.key = da->accel_key2;
          accel.cmd = id_base + offset;
          DEBUGLOG(("dec_append_accel_table: add {%x, %x, %i}\n", accel.fs, accel.key, accel.cmd));
          modified = true;
        }
      }
    }
  }
 nomore:
  if (modified)
  { PMRASSERT(WinDestroyAccelTable(haccel));
    haccel = WinCreateAccelTable(amp_player_hab(), paccel);
    PMASSERT(haccel != NULLHANDLE);
  }
}


/****************************************************************************
*
*  Global initialization functions
*
****************************************************************************/

void plugman_init()
{ Plugin::Init();
  Decoder::Init();
  Output::Init();
}

void plugman_uninit()
{ // remove all plugins
  VisualsSkinned.clear();
  Visuals.clear();
  Decoders.clear();
  Filters.clear();
  Outputs.clear();
  // deinitialize framework 
  Output::Uninit();
  Decoder::Uninit();
  Plugin::Uninit();
}
