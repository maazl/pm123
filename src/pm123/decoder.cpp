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
 */

#define  INCL_PM

#include "decoder.h"
#include "controller.h" // for starting position work around
#include "glue.h" // out_playing_pos
#include "configuration.h"
#include "eventhandler.h"
#include "proxyhelper.h"
#include <cpp/vdelegate.h>
#include <charset.h>
#include <fileutil.h>
#include <wildcards.h>
#include <cpp/url123.h>
#include "pm123.h" // for hab
#include <sys/stat.h> // TODO: replace by xio
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <os2.h>

#include <debuglog.h>


/****************************************************************************
*
* decoder interface
*
****************************************************************************/

Decoder::Decoder(Module& module)
: Plugin(module, PLUGIN_DECODER)
, TryOthers(false)
{ memset(&DFT_Add, 0, sizeof DFT_Add);
}
  
void Decoder::FillFileTypeCache()
{
  FileTypeCache = AddFileTypes;
  FileExtensionCache = NULL;
  FileTypeList.clear();

  const DECODER_FILETYPE* ft = FileTypes + FileTypesCount;
  while (ft-- != FileTypes)
  { FileTypeList.append() = ft;
    if (ft->eatype && *ft->eatype)
    { if (FileTypeCache)
        FileTypeCache = FileTypeCache+";"+ft->eatype;
      else
        FileTypeCache = ft->eatype;
    }
    if (ft->extension && *ft->extension)
    { if (FileExtensionCache)
        FileExtensionCache = FileExtensionCache+";"+ft->extension;
      else
        FileExtensionCache = ft->extension;
    }
  }

  if (AddFileTypes)
  { DFT_Add.eatype = AddFileTypes;
    FileTypeList.append() = &DFT_Add;
  }
}

void Decoder::LoadPlugin()
{ DEBUGLOG(("Decoder(%p{%s})::LoadPlugin()\n", this, ModRef.Key.cdata()));
  W = NULL;
  const Module& mod = ModRef;
  if (mod.GetParams().type & PLUGIN_DECODER)
  { mod.LoadMandatoryFunction(&decoder_fileinfo, "decoder_fileinfo");
    mod.LoadOptionalFunction(&decoder_init,     "decoder_init"    );
    mod.LoadOptionalFunction(&decoder_uninit,   "decoder_uninit"  );
    mod.LoadOptionalFunction(&decoder_command,  "decoder_command" );
    mod.LoadOptionalFunction(&decoder_status,   "decoder_status"  );
    mod.LoadOptionalFunction(&decoder_length,   "decoder_length"  );
    mod.LoadOptionalFunction(&decoder_event,    "decoder_event"   );
  }

  mod.LoadOptionalFunction(&decoder_saveinfo, "decoder_saveinfo");
  mod.LoadOptionalFunction(&decoder_editmeta, "decoder_editmeta");
  mod.LoadOptionalFunction(&decoder_getwizard,"decoder_getwizard");
  mod.LoadOptionalFunction(&decoder_savefile, "decoder_savefile");

  ULONG DLLENTRYP(decoder_support)(const DECODER_FILETYPE** types, int* count);
  ModRef.LoadMandatoryFunction(&decoder_support, "decoder_support");

  Type = (DECODER_TYPE)decoder_support(&FileTypes, &FileTypesCount);
  if ( (Type & DECODER_SONG)
    && ( !decoder_init || !decoder_uninit || !decoder_command
      || !decoder_status || !decoder_length || !decoder_event ))
    throw ModuleException("Could not load decoder %s\nThe plug-in does not export the playback interface completly.",
      ModRef.ModuleName.cdata());

  FillFileTypeCache();
}

void Decoder::InitPlugin()
{ DEBUGLOG(("Decoder(%p{%s})::InitPlugin()\n", this, ModRef.Key.cdata()));
  int rc = (*decoder_init)(&W);
  DEBUGLOG(("Decoder::InitPlugin: %i - %p\n", rc, W));
  if (rc)
  { W = NULL;
    throw ModuleException("The decoder %s failed to initialize. Error code %i", ModRef.Key.cdata(), rc);
  }
  DEBUGLOG(("Decoder::InitPlugin: %p\n", W));
  RaisePluginChange(PluginEventArgs::Init);
}

bool Decoder::UninitPlugin()
{ DEBUGLOG(("Decoder(%p{%s})::UninitPlugin()\n", this, ModRef.Key.cdata()));
  if (IsInitialized())
  { (*decoder_uninit)(xchg(W, (void*)NULL));
    RaisePluginChange(PluginEventArgs::Uninit);
  }
  return true;
}

bool Decoder::IsFileSupported(const char* file, const char* eatype) const
{ DEBUGLOG(("Decoder(%p{%s})::IsFileSupported(%s, %p) - %s, %s\n", this, ModRef.Key.cdata(),
    file, eatype, FileTypeCache.cdata(), FileExtensionCache.cdata()));

  // Try file name match
  if (FileExtensionCache)
  { // extract filename
    char fname[_MAX_PATH];
    sfnameext(fname, file, sizeof fname);
    if (wildcardfit(FileExtensionCache, fname))
    { DEBUGLOG(("Decoder::IsFileSupported: wildcard match of %s with %s\n", fname, FileExtensionCache.cdata()));
      return true;
  } }

  // Try file type match
  if (FileTypeCache && eatype)
  { const USHORT* data = (USHORT*)eatype;
    USHORT type = *data++;
    return DoFileTypeMatch(FileTypeCache, type, data);
  }

  // no match
  return false;
}

bool Decoder::DoFileTypeMatch(const char* filetypes, USHORT type, const USHORT*& eadata)
{ DEBUGLOG(("Decoder::DoFileTypeMatch(%04x, %04x...)\n", type, eadata[0]));
  switch (type)
  {case EAT_ASCII:
    if (wildcardfit(filetypes, xstring((const char*)(eadata + 1), eadata[0])))
      return true;
   default:
    (const char*&)eadata += sizeof(USHORT) + eadata[0];
    break;

   case EAT_MVST:
    { size_t count = eadata[1];
      USHORT type = eadata[2];
      eadata += 3;
      while (count--)
        if (DoFileTypeMatch(filetypes, type, eadata))
          return true;
    }
    break;

   case EAT_MVMT:
    { size_t count = eadata[1];
      eadata += 2;
      while (count--)
        if (DoFileTypeMatch(filetypes, *eadata++, eadata))
          return true;
    }
   case EAT_ASN1: // not supported
    break;
  }
  return false;
}

void Decoder::GetParams(stringmap_own& params) const
{ Plugin::GetParams(params);
  static const xstring troparam = "tryothers";
  params.get(troparam) = new stringmapentry(troparam, TryOthers ? "yes" : "no");
  static const xstring eftparam = "filetypes";
  params.get(eftparam) = new stringmapentry(eftparam, AddFileTypes);
}

bool Decoder::SetParam(const char* param, const xstring& value)
{ if (stricmp(param, "tryothers") == 0)
  { bool* b = url123::parseBoolean(value);
    DEBUGLOG(("Decoder::SetParam: tryothers = %u\n", b ? *b : -1));
    if (b)
    { TryOthers = *b;
      return true;
    }
    return false;
  } else if (stricmp(param, "filetypes") == 0)
  { AddFileTypes = value;
    FillFileTypeCache();
    return true;
  }
  return Plugin::SetParam(param, value);
}

ULONG Decoder::SaveInfo(const char* url, const META_INFO* info, int haveinfo)
{ if (!decoder_saveinfo)
  { EventHandler::PostFormat(MSG_ERROR, "The plug-in %s cannot save meta information.", ModRef.Key.cdata());
    return PLUGIN_NO_USABLE;
  }
  xstring errortxt;
  ULONG rc = decoder_saveinfo(url, info, haveinfo, &errortxt);
  if (errortxt)
    EventHandler::Post(rc ? MSG_ERROR : MSG_WARNING, errortxt);
  return rc;
}


// Proxy for level 1 decoder interface
class DecoderProxy1 : public Decoder, protected ProxyHelper
{protected:
  bool         SerializeInfo;

 private:
  int    Magic;
  ULONG  DLLENTRYP(vdecoder_command      )(void* w, ULONG msg, DECODER_PARAMS* params);
  int    DLLENTRYP(voutput_request_buffer)(void* a, const FORMAT_INFO2* format, float** buf);
  void   DLLENTRYP(voutput_commit_buffer )(void* a, int len, PM123_TIME posmarker);
  void   DLLENTRYP(voutput_event         )(void* a, DECEVENTTYPE event, void* param);
  void*  a;
  ULONG  DLLENTRYP(vdecoder_fileinfo )(const char* filename, DECODER_INFO* info);
  ULONG  DLLENTRYP(vdecoder_trackinfo)(const char* drive, int track, DECODER_INFO* info);
  ULONG  DLLENTRYP(vdecoder_saveinfo )(const char* filename, const DECODER_INFO* info);
  ULONG  DLLENTRYP(vdecoder_cdinfo   )(const char* drive, DECODER_CDINFO* info);
  ULONG  DLLENTRYP(vdecoder_editmeta )(HWND owner, const char* url);
  ULONG  DLLENTRYP(vdecoder_length   )(void* w);
  //void   DLLENTRYP(error_display)(char*);
  HWND   hwnd; // Window handle for catching event messages
  ULONG  tid;  // decoder thread id
  xstring url; // currently playing song
  PM123_TIME temppos;
  int    juststarted; // Status whether the first samples after DECODER_PLAY did not yet arrive. 2 = no data arrived, 1 = no valid data arrived, 0 = OK
  DECFASTMODE lastfast;
  DECODER_FILETYPE* filetypebuffer;
  char*  filetypestringbuffer;
  char   metadata_buffer[128]; // Loaded in curtun on decoder's demand WM_METADATA.
  Mutex  info_mtx; // Mutex to serialize access to the decoder_*info functions.
  VDELEGATE vd_decoder_command, vd_decoder_event, vd_decoder_fileinfo, vd_decoder_saveinfo, vd_decoder_editmeta, vd_decoder_length;

 private:
  PROXYFUNCDEF ULONG      DLLENTRY proxy_1_decoder_command     ( DecoderProxy1* op, void* w, ULONG msg, const DECODER_PARAMS2* params );
  PROXYFUNCDEF void       DLLENTRY proxy_1_decoder_event       ( DecoderProxy1* op, void* w, OUTEVENTTYPE event );
  PROXYFUNCDEF ULONG      DLLENTRY proxy_1_decoder_fileinfo    ( DecoderProxy1* op, const char* url, int* what, const INFO_BUNDLE* info,
                                                                                DECODER_INFO_ENUMERATION_CB cb, void* param );
  PROXYFUNCDEF ULONG      DLLENTRY proxy_1_decoder_saveinfo    ( DecoderProxy1* op, const char* url, const META_INFO* info, int haveinfo, xstring* errortxt );
  PROXYFUNCDEF int        DLLENTRY proxy_1_decoder_play_samples( DecoderProxy1* op, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
  PROXYFUNCDEF PM123_TIME DLLENTRY proxy_1_decoder_length      ( DecoderProxy1* op, void* w );
  PROXYFUNCDEF ULONG      DLLENTRY proxy_1_decoder_editmeta    ( DecoderProxy1* op, HWND owner, const char* url );
  friend MRESULT EXPENTRY proxy_1_decoder_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  // Callback for proxy induced commands
  static  void CommandCallback(Ctrl::ControlCommand* cmd);
 
 private:
  virtual void LoadPlugin();
          bool IsValid()     { return Magic == 0x714afb12; }
 public:
  DecoderProxy1(Module& module);
  virtual ~DecoderProxy1();
  virtual void GetParams(stringmap_own& map) const;
  virtual bool SetParam(const char* param, const xstring& value);

};

DecoderProxy1::DecoderProxy1(Module& module)
: Decoder(module),
  SerializeInfo(false),
  Magic(0x714afb12),
  hwnd(NULLHANDLE),
  juststarted(0),
  filetypebuffer(NULL),
  filetypestringbuffer(NULL)
{}

DecoderProxy1::~DecoderProxy1()
{ if (hwnd != NULLHANDLE)
    WinDestroyWindow(hwnd);
  Magic = 0;
  delete[] filetypebuffer;
  delete[] filetypestringbuffer;
}

/* Assigns the addresses of the decoder plug-in procedures. */
void DecoderProxy1::LoadPlugin()
{ DEBUGLOG(("DecoderProxy1(%p{%s})::load()\n", this, ModRef.Key.cdata()));
  W = NULL;
  const Module& mod = ModRef;
  ASSERT(mod.GetParams().type & PLUGIN_DECODER);
  ULONG DLLENTRYP(decoder_support)( char* fileext[], int* size );
  mod.LoadMandatoryFunction(&decoder_init,      "decoder_init");
  mod.LoadMandatoryFunction(&decoder_uninit,    "decoder_uninit");
  mod.LoadMandatoryFunction(&decoder_status,    "decoder_status");
  mod.LoadMandatoryFunction(&vdecoder_length,   "decoder_length");
  mod.LoadMandatoryFunction(&vdecoder_fileinfo, "decoder_fileinfo");
  mod.LoadMandatoryFunction(&vdecoder_command,  "decoder_command");
  mod.LoadMandatoryFunction(&decoder_support,   "decoder_support");

  mod.LoadOptionalFunction(&vdecoder_saveinfo, "decoder_saveinfo");
  mod.LoadOptionalFunction(&vdecoder_editmeta, "decoder_editmeta");
  mod.LoadOptionalFunction(&decoder_getwizard, "decoder_getwizard");
  decoder_command   = vdelegate(&vd_decoder_command,  &proxy_1_decoder_command,  this);
  decoder_event     = vdelegate(&vd_decoder_event,    &proxy_1_decoder_event,    this);
  decoder_fileinfo  = vdelegate(&vd_decoder_fileinfo, &proxy_1_decoder_fileinfo, this);
  if (vdecoder_saveinfo)
    decoder_saveinfo= vdelegate(&vd_decoder_saveinfo, &proxy_1_decoder_saveinfo, this);
  if (vdecoder_editmeta)
    decoder_editmeta= vdelegate(&vd_decoder_editmeta, &proxy_1_decoder_editmeta, this);
  decoder_length    = vdelegate(&vd_decoder_length,   &proxy_1_decoder_length,   this);
  tid = (ULONG)-1;

  FileTypesCount     = 0;
  FileTypes          = NULL;
  delete[] filetypebuffer;
  delete[] filetypestringbuffer;

  char**  my_support = NULL;
  int     i;

  // determine size (What a crappy interface!)
  Type = (DECODER_TYPE)decoder_support(my_support, &FileTypesCount);

  if (FileTypesCount)
  { my_support = (char**)alloca(sizeof *my_support * FileTypesCount);
    filetypestringbuffer = new char[_MAX_EXT * FileTypesCount];
    // initialize array
    my_support[0] = filetypestringbuffer;
    for( i = 1; i < FileTypesCount; i++ )
      my_support[i] = my_support[i-1] + _MAX_EXT;

    Type = (DECODER_TYPE)decoder_support(my_support, &FileTypesCount);

    // Destination array
    FileTypes = filetypebuffer = new DECODER_FILETYPE[FileTypesCount];
    memset(filetypebuffer, 0, sizeof *filetypebuffer * FileTypesCount);
    // Copy content
    for(i = 0; i < FileTypesCount; ++i)
    { filetypebuffer[i].category  = "Digital Audio";
      filetypebuffer[i].extension = strlwr(my_support[i]);
    }
  }

  FillFileTypeCache();

  if (Type & DECODER_TRACK)
  { mod.LoadMandatoryFunction(&vdecoder_cdinfo,    "decoder_cdinfo");
    mod.LoadMandatoryFunction(&vdecoder_trackinfo, "decoder_trackinfo");
  }
}

void DecoderProxy1::GetParams(stringmap_own& params) const
{ Decoder::GetParams(params);
  static const xstring siparam = "serializeinfo";
  params.get(siparam) = new stringmapentry(siparam, SerializeInfo ? "yes" : "no");
}

bool DecoderProxy1::SetParam(const char* param, const xstring& value)
{ if (stricmp(param, "serializeinfo") == 0)
  { bool* b = url123::parseBoolean(value);
    DEBUGLOG(("DecoderProxy1::SetParam: serializeinfo = %u\n", b ? *b : -1));
    if (b)
    { SerializeInfo = *b;
      return true;
    }
    return false;
  }
  return Decoder::SetParam(param, value);
}


/* proxy for the output callback of decoder interface level 0/1 */
PROXYFUNCIMP(int DLLENTRY, DecoderProxy1)
proxy_1_decoder_play_samples( DecoderProxy1* op, const FORMAT_INFO* format, const char* buf, int len, int posmarker )
{ DEBUGLOG(("proxy_1_decoder_play_samples(%p{%s}, %p{%u,%u,%u}, %p, %i, %i) - %f\n",
    op, op->ModRef.Key.cdata(), format, format->size, format->samplerate, format->channels, buf, len, posmarker, op->temppos));

  if (format->format != WAVE_FORMAT_PCM || (format->bits != 16 && format->bits != 8))
  { EventHandler::Post(MSG_ERROR, "PM123 does only accept PCM data with 8 or 16 bits per sample when using old-style decoder plug-ins.");
    return 0; // error
  }

  // prepare counters for output loop
  const int bps = (format->bits >> 3) * format->channels;
  len /= bps;
  int rem = len;

  // Work-around for decoders that do not support jumpto at DECODER_PLAY.
  if (op->juststarted)
  { if (posmarker < (int)(op->temppos*1000 + .5)) // before the desired start position?
    { // => send navigate message the first time and eat the samples
      // Eat all samples?
      if (posmarker/1000. + (double)rem/format->samplerate <= op->temppos)
      { DEBUGLOG(("proxy_1_decoder_play_samples juststarted = %i -> eating all samples.\n", op->juststarted));
        if (op->juststarted == 2)
        { // Set the status before the navigate command, because the callback may immediately reset the stats to 0.
          op->juststarted = 1;
          Ctrl::PostCommand(Ctrl::MkNavigate(xstring(), op->temppos, false, false), &DecoderProxy1::CommandCallback, op);
        }
        return len * bps;
      } else
      { // calculate remaining part
        rem = (int)floor(((op->temppos - posmarker/1000.) * format->samplerate));
        DEBUGLOG(("proxy_1_decoder_play_samples juststarted = %i -> eating %i samples.\n", op->juststarted, rem));
        buf += rem * bps;
        rem = len - rem;
    } }
    op->juststarted = 0;
  }

  op->temppos = -1; // buffer is empty now

  if (op->tid == (ULONG)-1)
  { PTIB ptib;
    DosGetInfoBlocks(&ptib, NULL);
    op->tid = ptib->tib_ordinal;
  }

  while (rem)
  { float* dest;
    FORMAT_INFO2 fi = { format->samplerate, format->channels };
    int l = (*op->voutput_request_buffer)(op->a, &fi, &dest);
    DEBUGLOG(("proxy_1_decoder_play_samples: now at %p %i %i %g\n", buf, l, rem, op->temppos));
    if (op->temppos != -1)
    { (*op->voutput_commit_buffer)(op->a, 0, op->temppos); // no-op
      break;
    }
    if (l <= 0)
      return (len - rem) * bps; // error
    if (l > rem)
      l = rem;
    if (format->bits == 8)
    { const unsigned char* sp = (const unsigned char*)buf;
      const unsigned char* ep = sp + (l*format->channels & -8);
      float* dp = dest;
      while (sp != ep)
      { DO_8(p, dp[p] = (sp[p] - 128) / 256.);
        sp += 8;
        dp += 8;
      }
      ep = sp + (l*format->channels & ~-8);
      while (sp != ep)
        *dp++ = (*sp++ - 128) / 256.;
    } else // 16 bit
      ProxyHelper::Short2Float(dest, (short*)buf, l*format->channels);

    DEBUGLOG(("proxy_1_decoder_play_samples: commit: %i %g\n", posmarker, posmarker/1000. + (double)(len-rem)/format->samplerate));
    (*op->voutput_commit_buffer)(op->a, l, posmarker/1000. + (double)(len-rem)/format->samplerate);
    rem -= l;
    buf += l * bps;
  }
  return len * bps;
}

/* Proxy for loading interface level 0/1 */
PROXYFUNCIMP(ULONG DLLENTRY, DecoderProxy1)
proxy_1_decoder_command( DecoderProxy1* op, void* w, ULONG msg, const DECODER_PARAMS2* params )
{ DEBUGLOG(("proxy_1_decoder_command(%p {%s}, %p, %d, %p)\n",
    op, op->ModRef.Key.cdata(), w, msg, params));

  if (params == NULL) // well, sometimes wired things may happen
    return (*op->vdecoder_command)(w, msg, NULL);

  static DECODER_PARAMS par1;
  memset(&par1, 0, sizeof par1);
  par1.size = sizeof par1;
  // preprocessing
  xstring proxy; // keep strong references
  xstring auth;
  switch (msg)
  {case DECODER_PLAY:
    // decompose URL for old interface
    { CDDA_REGION_INFO cd_info;
      if (strnicmp(params->URL, "file:///", 8) == 0)
      { size_t len = strlen(params->URL) + 1;
        char* cp = (char*)alloca(len);
        memcpy(cp, params->URL, len);
        par1.filename = ConvertUrl2File(cp);
        DEBUGLOG2(("proxy_1_decoder_command: filename=%s\n", par1.filename));
      } else if (scdparams(&cd_info, params->URL))
      { par1.drive = cd_info.drive;
        par1.track = cd_info.track;
        par1.sectors[0] = cd_info.sectors[0];
        par1.sectors[1] = cd_info.sectors[1];
      } else if (is_url(params->URL)) // URL
      { par1.URL = params->URL;
        par1.filename = params->URL; // Well, the URL parameter has never been used...
      } else
        par1.other = params->URL;

      par1.jumpto = (int)floor(params->JumpTo*1000 + .5);
      op->temppos = params->JumpTo;
      op->juststarted = 2;
      op->url = params->URL; // keep URL
    }
    break;

   case DECODER_SETUP:
    op->hwnd = DecoderProxy1::CreateProxyWindow("DecoderProxy1", op);

    par1.output_play_samples = (int DLLENTRYPF()(void*, const FORMAT_INFO*, const char*, int, int))&PROXYFUNCREF(DecoderProxy1)proxy_1_decoder_play_samples;
    par1.a                   = op;
    par1.proxyurl            = proxy = Cfg::Get().proxy;
    par1.httpauth            = auth  = Cfg::Get().auth;
    par1.hwnd                = op->hwnd;
    par1.buffersize          = Cfg::Get().buff_size;
    par1.bufferwait          = Cfg::Get().buff_wait;
    par1.metadata_buffer     = op->metadata_buffer;
    par1.metadata_size       = sizeof(op->metadata_buffer);
    par1.audio_buffersize    = BUFSIZE;
    par1.error_display       = &PROXYFUNCREF(ProxyHelper)PluginDisplayError;
    par1.info_display        = &PROXYFUNCREF(ProxyHelper)PluginDisplayInfo;

    op->voutput_request_buffer = params->OutRequestBuffer;
    op->voutput_commit_buffer  = params->OutCommitBuffer;
    op->voutput_event          = params->DecEvent;
    op->a                      = params->A;

    op->temppos  = -1;
    op->lastfast = DECFAST_NORMAL_PLAY;
    break;

   case DECODER_STOP:
    op->url = NULL;
    op->juststarted = 0;
    DecoderProxy1::DestroyProxyWindow(op->hwnd);
    op->hwnd = NULLHANDLE;
    op->temppos = out_playing_pos();
    break;

   case DECODER_FFWD:
   case DECODER_REW:
    DEBUGLOG(("proxy_1_decoder_command:DECODER_FFWD: %u\n", params->Fast));
    if (op->lastfast && params->Fast)
    { // changing direction requires two commands
      msg = op->lastfast == DECFAST_REWIND ? DECODER_REW : DECODER_FFWD;
      par1.ffwd              = FALSE;
      par1.rew               = FALSE;
      (*op->vdecoder_command)(w, msg, &par1);
      op->lastfast = params->Fast;
    }
    par1.ffwd                = params->Fast == DECFAST_FORWARD;
    par1.rew                 = params->Fast == DECFAST_REWIND;
    msg = (op->lastfast|params->Fast) == DECFAST_REWIND ? DECODER_REW : DECODER_FFWD;
    op->temppos  = out_playing_pos();
    op->lastfast = params->Fast;
    break;

   case DECODER_JUMPTO:
    DEBUGLOG(("proxy_1_decoder_command:DECODER_JUMPTO: %g\n", params->JumpTo));
    par1.jumpto              = (int)floor(params->JumpTo*1000 +.5);
    op->temppos = params->JumpTo;
    break;

   case DECODER_SAVEDATA:
    par1.save_filename       = params->SaveFilename;
    break;
  }

  ULONG rc = (*op->vdecoder_command)(w, msg, &par1);

  switch (msg)
  {case DECODER_JUMPTO:
    op->juststarted = 0;
  }

  return rc;
}

/* Proxy for loading interface level 0/1 */
PROXYFUNCIMP(void DLLENTRY, DecoderProxy1)
proxy_1_decoder_event( DecoderProxy1* op, void* w, OUTEVENTTYPE event )
{ DEBUGLOG(("proxy_1_decoder_event(%p {%s}, %p, %d)\n", op, op->ModRef.Key.cdata(), w, event));

  switch (event)
  {case OUTEVENT_LOW_WATER:
    DosSetPriority(PRTYS_THREAD, DECODER_HIGH_PRIORITY_CLASS, DECODER_HIGH_PRIORITY_DELTA, op->tid);
    break;
   case OUTEVENT_HIGH_WATER:
    DosSetPriority(PRTYS_THREAD, DECODER_LOW_PRIORITY_CLASS, DECODER_LOW_PRIORITY_DELTA, op->tid);
    break;
   default:; // explicit no-op
  }
}

PROXYFUNCIMP(ULONG DLLENTRY, DecoderProxy1)
proxy_1_decoder_fileinfo( DecoderProxy1* op, const char* filename, int* what, const INFO_BUNDLE *info,
                          DECODER_INFO_ENUMERATION_CB cb, void* param )
{ DEBUGLOG(("proxy_1_decoder_fileinfo(%p, %s, *%x, %p{...}, %p, %p)\n", op,
    filename, *what, info, cb, param));

  DECODER_INFO old_info = { sizeof old_info };
  // Work around for missing undef value of the replay gain fields in level 1 plug-ins.
  old_info.album_peak = old_info.album_gain = old_info.track_peak = old_info.track_gain = -1000;
  CDDA_REGION_INFO cd_info;
  ULONG rc;

  if (scdparams(&cd_info, filename))
  { // CD URL
    if ( cd_info.sectors[1] != 0           // can't handle sectors
      || (op->Type & DECODER_TRACK) == 0 ) // plug-in does not support CD
      return PLUGIN_NO_PLAY;

    if (cd_info.track != 0)
    { // Get track info
      // Serialize access to the info functions of old plug-ins.
      // In theory the must be thread safe for older PM123 releases too. In practice they are not always.
      sco_ptr<Mutex::Lock> lock(op->SerializeInfo ? new Mutex::Lock(op->info_mtx) : NULL);
      rc = (*op->vdecoder_trackinfo)(cd_info.drive, cd_info.track, &old_info);
    } else
    { // Get TOC info
      DECODER_CDINFO dec_cdinfo;
      { // Serialize access to the info functions of old plug-ins.
        // In theory the must be thread safe for older PM123 releases too. In practice they are not always.
        sco_ptr<Mutex::Lock> lock(op->SerializeInfo ? new Mutex::Lock(op->info_mtx) : NULL);
        rc = (*op->vdecoder_cdinfo)(cd_info.drive, &dec_cdinfo);
      }
      if (rc == PLUGIN_OK)
      { // emulate TOC playlist
        //                   0123456789012345678
        char trackurl[18] = "cdda:///x:/Track";
        trackurl[8] = cd_info.drive[0];
        //static const PHYS_INFO pi = { sizeof(PHYS_INFO), -1, (unsigned)-1, PATTR_NONE };
        static const TECH_INFO ti = { 44100, 2, TATTR_SONG, xstring_NULL, "CDXA" };
        static const ATTR_INFO ai = { PLO_NONE };
        static const INFO_BUNDLE trackinfo = { NULL, &(TECH_INFO&)ti, NULL, NULL, &(ATTR_INFO&)ai, NULL, NULL };
        for (int track = dec_cdinfo.firsttrack; track <= dec_cdinfo.lasttrack; ++track)
        { sprintf(trackurl+16, "%02i", track);
          (*cb)(param, trackurl, &trackinfo, INFO_NONE, INFO_TECH|INFO_ATTR|INFO_CHILD);
        }
        // Info of the whole CD
        info->phys->filesize   = dec_cdinfo.sectors * 2352.;
        info->tech->samplerate = 44100;
        info->tech->channels   = 2;
        info->tech->attributes = TATTR_PLAYLIST;
        info->tech->format     = "CD-TOC";
        info->obj->songlength  = dec_cdinfo.sectors / 75.;
        info->obj->bitrate     = 1411200;
        info->obj->num_items   = dec_cdinfo.lasttrack - dec_cdinfo.firsttrack +1;
        info->attr->ploptions  = PLO_NO_SHUFFLE;
        // old decoders always load all kind of information
        *what |= IF_Decoder;
      }
      return rc;
    }  
  } else
  { rc = 0;
    if (strnicmp(filename, "file:", 5) == 0)
    { char* fname = (char*)alloca(strlen(filename)+1);
      strcpy(fname, filename);
      filename = ConvertUrl2File(fname);
      if (*what & IF_Phys)
      { // get file size
        // TODO: large file support
        struct stat fi;
        rc = PLUGIN_NO_READ;
        if ( stat( filename, &fi ) == 0 && (fi.st_mode & S_IFDIR) == 0 )
        { // All level 1 plug-ins only support songs
          info->phys->filesize = fi.st_size;
          info->phys->tstmp    = fi.st_mtime;
          info->phys->attributes = PATTR_WRITABLE * ((fi.st_mode & S_IRUSR) != 0);
          rc = 0;
        }
      }
    }
    // DEBUGLOG(("proxy_1_decoder_fileinfo - %s\n", filename));
    if (*what & (IF_Tech|IF_Obj|IF_Meta))
    { // Serialize access to the info functions of old plug-ins.
      // In theory the must be thread safe for older PM123 releases too. In practice they are not.
      sco_ptr<Mutex::Lock> lock(op->SerializeInfo ? new Mutex::Lock(op->info_mtx) : NULL);
      rc = (*op->vdecoder_fileinfo)(filename, &old_info);
      *what |= IF_Tech|IF_Obj|IF_Meta;
    }
  }
  DEBUGLOG(("proxy_1_decoder_fileinfo: %lu\n", rc));

  // convert information to new format
  if (rc == 0)
  { ConvertDECODER_INFO(info, &old_info);
    // Always supply AttrInfo and RPL/DRPL
    *what |= IF_Attr|IF_Rpl|IF_Drpl;
  }
  return rc;
}

PROXYFUNCIMP(ULONG DLLENTRY, DecoderProxy1)
proxy_1_decoder_saveinfo(DecoderProxy1* op, const char* url, const META_INFO* info, int haveinfo, xstring* errortxt)
{ DEBUGLOG(("proxy_1_decoder_saveinfo(%p, %s, {%s,%s,%s,%s,%s,%s,%i,%s}, %x, )\n", op, url,
    info->title.cdata(),info->artist.cdata(),info->album.cdata(),info->year.cdata(),info->comment.cdata(),info->genre.cdata(),info->track.cdata(),info->copyright.cdata(),
    haveinfo));

  DECODER_INFO dinfo = { sizeof dinfo };
  ConvertMETA_INFO(&dinfo, info);

  dinfo.haveinfo   = haveinfo;

  // Decode file URLs
  if (strnicmp(url, "file:", 5) == 0)
  { char* fname = (char*)alloca(strlen(url)+1);
    strcpy(fname, url);
    url = ConvertUrl2File(fname);
  }
  // Call decoder's function
  return (*op->vdecoder_saveinfo)(url, &dinfo);
}

PROXYFUNCIMP(ULONG DLLENTRY, DecoderProxy1)
proxy_1_decoder_editmeta( DecoderProxy1* op, HWND owner, const char* url )
{ DEBUGLOG(("proxy_1_decoder_editmeta(%p, %p, %s)\n", op, owner, url));
  // Decode file URLs
  if (strnicmp(url, "file:", 5) == 0)
  { char* fname = (char*)alloca(strlen(url)+1);
    strcpy(fname, url);
    url = ConvertUrl2File(fname);
  }
  return (*op->vdecoder_editmeta)(owner, url);
}

PROXYFUNCIMP(PM123_TIME DLLENTRY, DecoderProxy1)
proxy_1_decoder_length( DecoderProxy1* op, void* a )
{ DEBUGLOG(("proxy_1_decoder_length(%p, %p)\n", op, a));
  int i = (*op->vdecoder_length)(a);
  return i < 0 ? -1 : i / 1000.;
}

MRESULT EXPENTRY proxy_1_decoder_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DecoderProxy1* op = (DecoderProxy1*)WinQueryWindowPtr(hwnd, QWL_USER);
  DEBUGLOG(("proxy_1_decoder_winfn(%p, %u, %p, %p) - %p {%s}\n",
    hwnd, msg, mp1, mp2, op, op == NULL ? NULL : op->ModRef.Key.cdata()));
  switch (msg)
  {case WM_PLAYSTOP:
    (*op->voutput_event)(op->a, DECEVENT_PLAYSTOP, NULL);
    return 0;
   //case WM_PLAYERROR: // ignored because the error_display function implies the event.
   case WM_SEEKSTOP:
    (*op->voutput_event)(op->a, DECEVENT_SEEKSTOP, NULL);
    return 0;
   case WM_CHANGEBR:
    /* It does not make sense to update the metadata as fast.
    if (op->url)
    { // Level 1 plug-ins can only handle Shoutcast stream infos.
      // Keep anything but the title field at the old values.
      int_ptr<Playable> pp = Playable::FindByURL(op->url);
      if (pp)
      { // Make this operation atomic
        Mutex::Lock lck(pp->Mtx);
        TECH_INFO ti = *pp->GetInfo().tech;
        ti.bitrate = LONGFROMMP(mp1);
        (*op->voutput_event)(op->a, DEVEVENT_CHANGETECH, &ti);
      }
    }
    */
    return 0;
   case WM_METADATA:
    if (op->url)
    { // Level 1 plug-ins can only handle Shoutcast stream infos.
      // Keep anything but the title field at the old values.
      int_ptr<Playable> pp = Playable::FindByURL(op->url);
      if (pp)
      { const char* metadata = (const char*)PVOIDFROMMP(mp1);
        // extract stream title
        if( metadata ) {
          /* @@@ TODO should be applied to the current playing reference.
          // Make this operation atomic
          Mutex::Lock lck(*pp);
          MetaInfo meta = *pp->GetInfo().meta;
          const char* titlepos = strstr( metadata, "StreamTitle='" );
          if ( titlepos )
          { titlepos += 13;
            const char* titleend = strstr( titlepos, "';" );
            if (titleend)
              memcpy( xstring_alloc(&meta.title, titleend-titlepos), titlepos, titleend-titlepos);
            else
              xstring_assign(&meta.title, titlepos);
            // Raise DECEVENT_CHANGEMETA
            (*op->voutput_event)(op->a, DECEVENT_CHANGEMETA, &meta);
          }*/
        }
      }
    }
    return 0;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

void DecoderProxy1::CommandCallback(Ctrl::ControlCommand* cmd)
{ DEBUGLOG(("DecoderProxy1::CommandCallback(%p{%u,...%i,%p})\n", cmd, cmd->Cmd, cmd->Flags, cmd->User));
  DecoderProxy1* op = (DecoderProxy1*)cmd->User;
  // Be careful, multithreaded access!
  { CritSect cs;
    if (op->IsValid())
    { switch (cmd->Cmd)
      {case Ctrl::Cmd_Navigate:
        // Whatever happens, after a seek command juststarted should be cleared,
        // because even if the decoder did not execute the seek we should start playback.
        op->juststarted = 0;
       default:; // avoid warnings
      }
    }
  }
  delete cmd;
}



Decoder::~Decoder()
{ Mutex::Lock lock(Module::Mtx);
  ASSERT(ModRef.Dec == this);
  ModRef.Dec = NULL;
}

int_ptr<Decoder> Decoder::FindInstance(const Module& module)
{ Mutex::Lock lock(Module::Mtx);
  Decoder* dec = module.Dec;
  return dec && !dec->RefCountIsUnmanaged() ? dec : NULL;
}

int_ptr<Decoder> Decoder::GetInstance(Module& module)
{ if ((module.GetParams().type & PLUGIN_DECODER) == 0)
    throw ModuleException("Cannot load plug-in %s as decoder.", module.Key.cdata());
  Mutex::Lock lock(Module::Mtx);
  Decoder* dec = module.Dec;
  if (dec && !dec->RefCountIsUnmanaged())
    return dec;
  if (module.GetParams().interface == 2)
    throw ModuleException("The docoder %s is not supported. It is intended for PM123 1.40.", module.Key.cdata());
  dec = module.GetParams().interface <= 1 ? new DecoderProxy1(module) : new Decoder(module);
  try
  { dec->LoadPlugin();
  } catch (...)
  { delete dec;
    throw;
  }
  return module.Dec = dec;
}

int_ptr<Decoder> Decoder::GetDecoder(const char* name)
{ int_ptr<Module> pm(Module::FindByKey(name));
  if (pm == NULL)
    throw ModuleException("Tried to invoke the decoder %s that does no longer exist.", name);
  int_ptr<Decoder> pd(Decoder::FindInstance(*pm));
  if (pd == NULL)
    throw ModuleException("Tried to invoke the decoder %s that is no longer loaded.", name);
  if (!pd->GetEnabled())
    throw ModuleException("Tried to invoke the decoder %s that is now disabled.", name);
  return pd;
}

void Decoder::Init()
{ PMRASSERT(WinRegisterClass(amp_player_hab, "DecoderProxy1", &proxy_1_decoder_winfn, 0, sizeof(DecoderProxy1*)));
}

void Decoder::AppendLoadMenu(HWND hMenu, ULONG id_base, SHORT where, DECODER_WIZARD_FUNC* callbacks, size_t size)
{ DEBUGLOG(("Decoder::AppendLoadMenu(%p, %d, %d, %p, %d)\n", hMenu, id_base, callbacks, size));
  size_t i;
  // cleanup
  SHORT lastcount = -1;
  for (i = 0; i < size; ++i)
  { SHORT newcount = SHORT1FROMMP(WinSendMsg(hMenu, MM_DELETEITEM, MPFROM2SHORT(id_base+i, FALSE), 0));
    DEBUGLOG(("Decoder::AppendLoadMenu - %i %i\n", i, newcount));
    if (newcount == lastcount)
      break;
    lastcount = newcount;
  }
  // for all decoder plug-ins...
  PluginList decoders(PLUGIN_DECODER);
  GetPlugins(decoders);

  for (i = 0; i < decoders.size(); ++i)
  { const Decoder& dec = (Decoder&)*decoders[i];
    if (dec.GetEnabled() && dec.decoder_getwizard)
    { const DECODER_WIZARD* da = (*dec.decoder_getwizard)();
      DEBUGLOG(("Decoder::AppendLoadMenu: %s - %p\n", dec.ModRef.Key.cdata(), da));
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
        DEBUGLOG(("Decoder::AppendLoadMenu: add %u: %s -> %p => %u\n", id_base, da->prompt, da->wizard, pos));
        // Add callback function
        *callbacks++ = da->wizard;
        if (mi.iPosition != MIT_END)
          ++mi.iPosition;
      }
    }
  }
}

void Decoder::AppendAccelTable(HACCEL& haccel, ULONG id_base, LONG offset, DECODER_WIZARD_FUNC* callbacks, size_t size)
{ DEBUGLOG(("Decoder::AppendAccelTable(%p, %u, %u, %p, %u)\n", haccel, id_base, offset, callbacks, size));
  // Fetch content
  ULONG accelsize = WinCopyAccelTable(haccel, NULL, 0);
  PMASSERT(accelsize);
  accelsize += (size << (offset != 0)) * sizeof(ACCEL); // space for plug-in entries
  ACCELTABLE* paccel = (ACCELTABLE*)alloca(accelsize);
  PMRASSERT(WinCopyAccelTable(haccel, paccel, accelsize));
  DEBUGLOG(("Decoder::AppendAccelTable: %i\n", paccel->cAccel));
  bool modified = false;
  // Append plug-in accelerators
  PluginList decoders(PLUGIN_DECODER);
  GetPlugins(decoders);
  for (size_t i = 0; i < decoders.size(); ++i)
  { Decoder& dec = (Decoder&)*decoders[i];
    if (dec.GetEnabled() && dec.decoder_getwizard)
    { const DECODER_WIZARD* da = (*dec.decoder_getwizard)();
      DEBUGLOG(("Decoder::AppendAccelTable: %s - %p\n", dec.ModRef.Key.cdata(), da));
      for (; da != NULL; da = da->link, ++id_base)
      { if (size-- == 0)
          goto nomore; // too many entries, can't help
        DEBUGLOG(("Decoder::AppendAccelTable: at %u: %s -> %x -> %p\n", id_base, da->prompt, da->accel_key, da->wizard));
        *callbacks++ = da->wizard;
        if (da->accel_key)
        { // Add table entry
          ACCEL& accel = paccel->aaccel[paccel->cAccel++];
          accel.fs  = da->accel_opt;
          accel.key = da->accel_key;
          accel.cmd = id_base;
          DEBUGLOG(("Decoder::AppendAccelTable: add {%x, %x, %i}\n", accel.fs, accel.key, accel.cmd));
          modified = true;
        }
        if (da->accel_key2 && offset)
        { // Add table entry
          ACCEL& accel = paccel->aaccel[paccel->cAccel++];
          accel.fs  = da->accel_opt2;
          accel.key = da->accel_key2;
          accel.cmd = id_base + offset;
          DEBUGLOG(("Decoder::AppendAccelTable: add {%x, %x, %i}\n", accel.fs, accel.key, accel.cmd));
          modified = true;
        }
      }
    }
  }
 nomore:
  if (modified)
  { PMRASSERT(WinDestroyAccelTable(haccel));
    haccel = WinCreateAccelTable(amp_player_hab, paccel);
    PMASSERT(haccel != NULLHANDLE);
  }
}

