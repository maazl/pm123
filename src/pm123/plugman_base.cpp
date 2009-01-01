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
 */

#define  INCL_PM
#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>

//#undef DEBUG
//#define DEBUG 2

#include <utilfct.h>
#include "plugman_base.h"
#include "pm123.h" // for hab
#include "dialog.h"
#include "playable.h"
#include "controller.h" // for starting position work around
#include <vdelegate.h>
#include <cpp/url123.h>

#include <limits.h>
#include <math.h>

#include <debuglog.h>


/* thread priorities for decoder thread */
/*#define  DECODER_HIGH_PRIORITY_CLASS PRTYC_TIMECRITICAL
#define  DECODER_HIGH_PRIORITY_DELTA 0*/
#define  DECODER_HIGH_PRIORITY_CLASS PRTYC_TIMECRITICAL
#define  DECODER_HIGH_PRIORITY_DELTA 0
#define  DECODER_LOW_PRIORITY_CLASS  PRTYC_FOREGROUNDSERVER
#define  DECODER_LOW_PRIORITY_DELTA  0


#define DO_8(p,x) \
{  { const int p = 0; x; } \
   { const int p = 1; x; } \
   { const int p = 2; x; } \
   { const int p = 3; x; } \
   { const int p = 4; x; } \
   { const int p = 5; x; } \
   { const int p = 6; x; } \
   { const int p = 7; x; } \
}


// Convert timestamp in seconds to an integer in milliseconds.
// Truncate leading bits in case of an overflow.
static int tstmp_f2i(double pos)
{ DEBUGLOG(("tstmp_f2i(%g)\n", pos));
  // We cast to unsigned first to ensure that the range of the int is fully used.
  return pos >= 0 ? (int)(unsigned)(fmod(pos*1000., UINT_MAX+1.) + .5) : -1;
}

// Convert possibly truncated time stamp in milliseconds to seconds.
// The missing bits are taken from a context time stamp which has to be sufficiently close
// to the original time stamp. Sufficient is about ñ24 days.
static double tstmp_i2f(int pos, double context)
{ DEBUGLOG(("tstmp_i2f(%i, %g)\n", pos, context));
  if (pos < 0)
    return -1;
  double r = pos / 1000.;
  return r + (UINT_MAX+1.) * floor((context - r + UINT_MAX/2) / (UINT_MAX+1.));
}


/****************************************************************************
*
* decoder interface
*
****************************************************************************/

/*Decoder::~Decoder()
{ DEBUGLOG(("Decoder(%p{%s})::~Decoder%p\n", this, ModuleName, Support));
}*/

bool Decoder::AfterLoad()
{ DEBUGLOG(("Decoder(%p{%s})::AfterLoad()\n", this, GetModuleName().cdata()));

  ULONG DLLENTRYP(decoder_support)( const char** fileext, const char** filetype );
  if (!GetModule().LoadFunction(&decoder_support,  "decoder_support" ))
    return false;

  const char* fileext;
  const char* filetype;

  Type = decoder_support( &fileext, &filetype );
  Extensions = fileext;
  FileTypes = filetype;

  return true;
}

PLUGIN_TYPE Decoder::GetType() const
{ return PLUGIN_DECODER;
}

/* Assigns the addresses of the decoder plug-in procedures. */
bool Decoder::LoadPlugin()
{ DEBUGLOG(("Decoder(%p{%s})::LoadPlugin()\n", this, GetModuleName().cdata()));
  const Module& mod = GetModule();
  
  if ( !(mod.GetParams().type & PLUGIN_DECODER)
    || !mod.LoadFunction(&decoder_init,     "decoder_init"    )
    || !mod.LoadFunction(&decoder_uninit,   "decoder_uninit"  )
    || !mod.LoadFunction(&decoder_command,  "decoder_command" )
    || !mod.LoadFunction(&decoder_status,   "decoder_status"  )
    || !mod.LoadFunction(&decoder_length,   "decoder_length"  )
    || !mod.LoadFunction(&decoder_fileinfo, "decoder_fileinfo")
    || !mod.LoadFunction(&decoder_event,    "decoder_event"   ) )
    return false;

  mod.LoadOptionalFunction(&decoder_saveinfo, "decoder_saveinfo");
  mod.LoadOptionalFunction(&decoder_editmeta, "decoder_editmeta");
  mod.LoadOptionalFunction(&decoder_getwizzard, "decoder_getwizzard");

  if (!AfterLoad())
    return false;
  
  W = NULL;

  if (Type & DECODER_TRACK)
  { if (!mod.LoadFunction(&decoder_cdinfo,   "decoder_cdinfo"))
      return false;
  } else
  { decoder_cdinfo = &stub_decoder_cdinfo;
  }
  return true;
}

bool Decoder::InitPlugin()
{ DEBUGLOG(("Decoder(%p{%s})::InitPlugin()\n", this, GetModuleName().cdata()));

  if ((*decoder_init)(&W) == -1)
  { W = NULL;
    return false;
  }
  RaisePluginChange(EventArgs::Init);
  return true;
}

bool Decoder::UninitPlugin()
{ DEBUGLOG(("Decoder(%p{%s})::UninitPlugin()\n", this, GetModuleName().cdata()));

  if (IsInitialized())
  { (*decoder_uninit)( xchg(W, (void*)NULL));
    RaisePluginChange(EventArgs::Uninit);
  }
  return true;
}

bool Decoder::IsFileSupported(const char* file, const char* eatype) const
{ DEBUGLOG(("Decoder(%p{%s})::IsFileSupported(%s, %p) - %s, %s, %s\n", this, GetModuleName().cdata(),
    file, eatype, Extensions ? Extensions.cdata() : "<null>", FileTypes ? FileTypes.cdata() : "<null>",
    AddFileTypes ? AddFileTypes.cdata() : "<null>"));

  // Try file name match
  if (Extensions)
  { // extract filename
    char fname[_MAX_PATH];
    sfnameext(fname, file, sizeof fname);
    if (wildcardfit(Extensions, fname))
    { DEBUGLOG(("Decoder::IsFileSupported: wildcard match of %s with %s\n", fname, Extensions.cdata()));
      return true;
  } }

  // Try file type match
  const xstring& filetypes = GetFileTypes();
  if (filetypes && eatype)
  { const USHORT* data = (USHORT*)eatype;
    return DoFileTypeMatch(filetypes, *data++, data);
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

xstring Decoder::GetFileTypes() const
{ if (FileTypes && AddFileTypes)
    return xstring::sprintf("%s;%s", FileTypes.cdata(), AddFileTypes.cdata());
  if (AddFileTypes)
    return AddFileTypes;
  return FileTypes;
}

void Decoder::GetParams(stringmap& params) const
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
    return true; 
  }
  return Plugin::SetParam(param, value);
}


PROXYFUNCIMP(ULONG DLLENTRY, Decoder)
stub_decoder_cdinfo(const char* drive, DECODER_CDINFO* info)
{ return 100; // can't play CD
}


// Proxy for level 1 decoder interface
class DecoderProxy1 : public Decoder
{protected:
  bool         SerializeInfo;

 private:
  ULONG  DLLENTRYP(vdecoder_command      )( void* w, ULONG msg, DECODER_PARAMS* params );
  int    DLLENTRYP(voutput_request_buffer)( void* a, const FORMAT_INFO2* format, short** buf );
  void   DLLENTRYP(voutput_commit_buffer )( void* a, int len, double posmarker );
  void   DLLENTRYP(voutput_event         )( void* a, DECEVENTTYPE event, void* param );
  void*  a;
  ULONG  DLLENTRYP(vdecoder_fileinfo )( const char* filename, DECODER_INFO* info );
  ULONG  DLLENTRYP(vdecoder_trackinfo)( const char* drive, int track, DECODER_INFO* info );
  ULONG  DLLENTRYP(vdecoder_saveinfo )( const char* filename, const DECODER_INFO* info );
  ULONG  DLLENTRYP(vdecoder_cdinfo   )( const char* drive, DECODER_CDINFO* info );
  ULONG  DLLENTRYP(vdecoder_length   )( void* w );
  void   DLLENTRYP(error_display)( char* );
  HWND   hwnd; // Window handle for catching event messages
  ULONG  tid;  // decoder thread id
  xstring url; // currently playing song
  double temppos;
  int    juststarted; // Status whether the first samples after DECODER_PLAY did not yet arrive. 2 = no data arrived, 1 = no valid data arrived, 0 = OK
  DECFASTMODE lastfast;
  char   metadata_buffer[128]; // Loaded in curtun on decoder's demand WM_METADATA.
  Mutex  info_mtx; // Mutex to serialize access to the decoder_*info functions.
  VDELEGATE vd_decoder_command, vd_decoder_event, vd_decoder_fileinfo, vd_decoder_saveinfo, vd_decoder_cdinfo, vd_decoder_length;

 private:
  PROXYFUNCDEF ULONG  DLLENTRY proxy_1_decoder_command     ( DecoderProxy1* op, void* w, ULONG msg, DECODER_PARAMS2* params );
  PROXYFUNCDEF void   DLLENTRY proxy_1_decoder_event       ( DecoderProxy1* op, void* w, OUTEVENTTYPE event );
  PROXYFUNCDEF ULONG  DLLENTRY proxy_1_decoder_fileinfo    ( DecoderProxy1* op, const char* filename, INFOTYPE* what, DECODER_INFO2* info );
  PROXYFUNCDEF ULONG  DLLENTRY proxy_1_decoder_saveinfo    ( DecoderProxy1* op, const char* filename, const META_INFO* info, int haveinfo );
  PROXYFUNCDEF ULONG  DLLENTRY proxy_1_decoder_cdinfo      ( DecoderProxy1* op, const char* drive, DECODER_CDINFO* info );
  PROXYFUNCDEF int    DLLENTRY proxy_1_decoder_play_samples( DecoderProxy1* op, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
  PROXYFUNCDEF double DLLENTRY proxy_1_decoder_length      ( DecoderProxy1* op, void* w );
  friend MRESULT EXPENTRY proxy_1_decoder_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 
 protected:
  virtual bool AfterLoad();
 public:
  DecoderProxy1(Module* mod) : Decoder(mod), SerializeInfo(false), hwnd(NULLHANDLE), juststarted(0) {}
  virtual ~DecoderProxy1();
  virtual bool LoadPlugin();
  virtual void GetParams(stringmap& map) const;
  virtual bool SetParam(const char* param, const xstring& value);
};

DecoderProxy1::~DecoderProxy1()
{ if (hwnd != NULLHANDLE)
    WinDestroyWindow(hwnd);
}

/* Assigns the addresses of the decoder plug-in procedures. */
bool DecoderProxy1::LoadPlugin()
{ DEBUGLOG(("DecoderProxy1(%p{%s})::load()\n", this, GetModuleName().cdata()));
  const Module& mod = GetModule();

  if ( !(mod.GetParams().type & PLUGIN_DECODER)
    || !mod.LoadFunction(&decoder_init,       "decoder_init")
    || !mod.LoadFunction(&decoder_uninit,     "decoder_uninit")
    || !mod.LoadFunction(&decoder_status,     "decoder_status")
    || !mod.LoadFunction(&vdecoder_length,    "decoder_length")
    || !mod.LoadFunction(&vdecoder_fileinfo,  "decoder_fileinfo")
    || !mod.LoadFunction(&vdecoder_command,   "decoder_command") )
    return false;

  mod.LoadOptionalFunction(&vdecoder_saveinfo,  "decoder_saveinfo");
  mod.LoadOptionalFunction(&decoder_editmeta,   "decoder_editmeta");
  mod.LoadOptionalFunction(&decoder_getwizzard, "decoder_getwizzard");
  decoder_command   = vdelegate(&vd_decoder_command,  &proxy_1_decoder_command,  this);
  decoder_event     = vdelegate(&vd_decoder_event,    &proxy_1_decoder_event,    this);
  decoder_fileinfo  = vdelegate(&vd_decoder_fileinfo, &proxy_1_decoder_fileinfo, this);
  if (vdecoder_saveinfo)
    decoder_saveinfo= vdelegate(&vd_decoder_saveinfo, &proxy_1_decoder_saveinfo, this);
  decoder_length    = vdelegate(&vd_decoder_length,   &proxy_1_decoder_length,   this);
  tid = (ULONG)-1;

  if (!AfterLoad())
    return false;

  W = NULL;

  if (Type & DECODER_TRACK)
  { if ( !mod.LoadFunction(&vdecoder_cdinfo,    "decoder_cdinfo")
      || !mod.LoadFunction(&vdecoder_trackinfo, "decoder_trackinfo") )
      return false;
    decoder_cdinfo  = vdelegate(&vd_decoder_cdinfo,   &proxy_1_decoder_cdinfo,   this);
  } else
  { decoder_cdinfo  = &stub_decoder_cdinfo;
    vdecoder_trackinfo = NULL;
  }
  return true;
}

bool DecoderProxy1::AfterLoad()
{ DEBUGLOG(("DecoderProxy1(%p{%s})::AfterLoad()\n", this, GetModuleName().cdata()));

  ULONG DLLENTRYP(decoder_support)( char* fileext[], int* size );
  if (!GetModule().LoadFunction(&decoder_support,  "decoder_support" ))
    return false;

  char**  my_support = NULL;
  int     size = 0;
  int     i;

  // determine size
  Type = decoder_support( my_support, &size );

  FileTypes = NULL;
  if (size)
  { my_support = (char**)malloc((_MAX_EXT + sizeof *my_support) * size);
    // initialize array
    my_support[0] = (char*)(my_support + size);
    for( i = 1; i < size; i++ ) {
      my_support[i] = my_support[i-1] + _MAX_EXT;
    }

    Type = decoder_support( my_support, &size );

    // calculate total result string size
    size_t len = size-1; // delimiters
    for( i = 0; i < size; i++ )
      len += strlen(my_support[i]);

    // copy content
    char* dst = Extensions.raw_init(len);
    i = 0;
    for(;;)
    { strcpy(dst, strlwr(my_support[i]));
      dst += strlen(dst);
      if (++i >= size)
        break;
      *dst++ = ';';
    }
    *dst = 0;
    
    free(my_support);
  } else
  { // no types supported
    Extensions = NULL;
  }
  return true;
} 

void DecoderProxy1::GetParams(stringmap& params) const
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
    op, op->GetModuleName().cdata(), format, format->size, format->samplerate, format->channels, buf, len, posmarker, op->temppos));

  if (format->format != WAVE_FORMAT_PCM || (format->bits != 16 && format->bits != 8))
  { (*op->error_display)("PM123 does only accept PCM data with 8 or 16 bits per sample when using old-style decoder plug-ins.");
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
        { Ctrl::PostCommand(Ctrl::MkNavigate(xstring(), op->temppos, false, false));
          op->juststarted = 1;
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
  { short* dest;
    int l = (*op->voutput_request_buffer)(op->a, (FORMAT_INFO2*)format, &dest);
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
    { // convert to 16 bit
      int i;
      const unsigned char* sp = (const unsigned char*)buf;
      short* dp = dest;
      for (i = (l*format->channels) >> 3; i; --i)
      { DO_8(p, dp[p] = (sp[p] + 128) << 8);
        sp += 8;
        dp += 8;
      }
      for (i = (l*format->channels) & 7; i; --i)
      { *dp++ = (*sp++ + 128) << 8;
      }
    } else
    { memcpy(dest, buf, l*bps);
    }
    DEBUGLOG(("proxy_1_decoder_play_samples: commit: %i %g\n", posmarker, posmarker/1000. + (double)(len-rem)/format->samplerate));
    (*op->voutput_commit_buffer)(op->a, l, posmarker/1000. + (double)(len-rem)/format->samplerate);
    rem -= l;
    buf += l * bps;
  }
  return len * bps;
}

/* Proxy for loading interface level 0/1 */
PROXYFUNCIMP(ULONG DLLENTRY, DecoderProxy1)
proxy_1_decoder_command( DecoderProxy1* op, void* w, ULONG msg, DECODER_PARAMS2* params )
{ DEBUGLOG(("proxy_1_decoder_command(%p {%s}, %p, %d, %p)\n",
    op, op->GetModuleName().cdata(), w, msg, params));

  if (params == NULL) // well, sometimes wired things may happen
    return (*op->vdecoder_command)(w, msg, NULL);

  static DECODER_PARAMS par1;
  memset(&par1, 0, sizeof par1);
  par1.size = sizeof par1;
  char filename[300];
  // preprocessing
  switch (msg)
  {case DECODER_PLAY:
    // decompose URL for old interface
    { char* cp = strchr(params->URL, ':') +2; // for URLs
      CDDA_REGION_INFO cd_info;
      if (is_file(params->URL))
      { if (strnicmp(params->URL, "file:///", 8) == 0)
        { strlcpy(filename, params->URL+8, sizeof filename);
          par1.filename = filename;
          cp = strchr(filename, '/');
          while (cp)
          { *cp = '\\';
            cp = strchr(cp+1, '/');
          }
        } else
          par1.filename = params->URL; // bare filename - normally this should not happen
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

      par1.jumpto = (int)floor(params->jumpto*1000 + .5);
      op->temppos = params->jumpto;
      op->juststarted = 2;
      op->url = params->URL; // keep URL
    }
    break;

   case DECODER_SETUP:
    op->hwnd = DecoderProxy1::CreateProxyWindow("DecoderProxy1", op);

    par1.output_play_samples = (int DLLENTRYPF()(void*, const FORMAT_INFO*, const char*, int, int))&PROXYFUNCREF(DecoderProxy1)proxy_1_decoder_play_samples;
    par1.a                   = op;
    par1.proxyurl            = params->proxyurl;
    par1.httpauth            = params->httpauth;
    par1.hwnd                = op->hwnd;
    par1.buffersize          = params->buffersize;
    par1.bufferwait          = params->bufferwait;
    par1.metadata_buffer     = op->metadata_buffer;
    par1.metadata_size       = sizeof(op->metadata_buffer);
    par1.audio_buffersize    = BUFSIZE;
    par1.error_display       = params->error_display;
    par1.info_display        = params->info_display;

    op->voutput_request_buffer = params->output_request_buffer;
    op->voutput_commit_buffer  = params->output_commit_buffer;
    op->voutput_event          = params->output_event;
    op->a                      = params->a;

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
    DEBUGLOG(("proxy_1_decoder_command:DECODER_FFWD: %u\n", params->fast));
    if (op->lastfast && params->fast)
    { // changing direction requires two commands
      msg = op->lastfast == DECFAST_REWIND ? DECODER_REW : DECODER_FFWD;
      par1.ffwd              = FALSE;
      par1.rew               = FALSE;
      (*op->vdecoder_command)(w, msg, &par1);
      op->lastfast = params->fast;
    }
    par1.ffwd                = params->fast == DECFAST_FORWARD;
    par1.rew                 = params->fast == DECFAST_REWIND;
    msg = (op->lastfast|params->fast) == DECFAST_REWIND ? DECODER_REW : DECODER_FFWD;
    op->temppos  = out_playing_pos();
    op->lastfast = params->fast;
    break;

   case DECODER_JUMPTO:
    DEBUGLOG(("proxy_1_decoder_command:DECODER_JUMPTO: %g\n", params->jumpto));
    par1.jumpto              = (int)floor(params->jumpto*1000 +.5);
    op->temppos = params->jumpto;
    op->juststarted = 0;
    break;

   case DECODER_EQ:
    par1.equalizer           = params->equalizer;
    par1.bandgain            = params->bandgain;
    break;

   case DECODER_SAVEDATA:
    par1.save_filename       = params->save_filename;
    break;
  }

  return (*op->vdecoder_command)(w, msg, &par1);
}

/* Proxy for loading interface level 0/1 */
PROXYFUNCIMP(void DLLENTRY, DecoderProxy1)
proxy_1_decoder_event( DecoderProxy1* op, void* w, OUTEVENTTYPE event )
{ DEBUGLOG(("proxy_1_decoder_event(%p {%s}, %p, %d)\n", op, op->GetModuleName().cdata(), w, event));

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
proxy_1_decoder_fileinfo( DecoderProxy1* op, const char* filename, INFOTYPE* what, DECODER_INFO2 *info )
{ DEBUGLOG(("proxy_1_decoder_fileinfo(%p, %s, %x, %p{%u,%p,%p,%p})\n", op, filename, what, info, info->size, info->format, info->tech, info->meta));

  DECODER_INFO old_info = { sizeof old_info };
  CDDA_REGION_INFO cd_info;
  ULONG rc;
  char buf[300];

  info->phys->filesize   = -1;

  if (scdparams(&cd_info, filename))
  { if ( cd_info.track == 0 ||            // can't handle sectors
         op->vdecoder_trackinfo == NULL ) // plug-in does not support trackinfo
      return 200;
    // Serialize access to the info functions of old plug-ins.
    // In theory the must be thread safe for older PM123 releases too. In practice they are not.
    sco_ptr<Mutex::Lock> lock(op->SerializeInfo ? new Mutex::Lock(op->info_mtx) : NULL);
    rc = (*op->vdecoder_trackinfo)(cd_info.drive, cd_info.track, &old_info);
  } else
  { if (strnicmp(filename, "file:///", 8) == 0)
    { strlcpy(buf, filename+8, sizeof buf);
      filename = buf;
      char* cp = strchr(buf, '/');
      while (cp)
      { *cp = '\\';
        cp = strchr(cp+1, '/');
      }
    }
    // DEBUGLOG(("proxy_1_decoder_fileinfo - %s\n", filename));
    { // Serialize access to the info functions of old plug-ins.
      // In theory the must be thread safe for older PM123 releases too. In practice they are not.
      sco_ptr<Mutex::Lock> lock(op->SerializeInfo ? new Mutex::Lock(op->info_mtx) : NULL);
      rc = (*op->vdecoder_fileinfo)(filename, &old_info);
    }
    // get file size
    // TODO: large file support
    struct stat fi;
    if ( rc == 0 && is_file(filename) && stat( filename, &fi ) == 0 )
      info->phys->filesize = fi.st_size;
  }
  info->phys->num_items = 1;
  info->tech->totalsize = info->phys->filesize;
  DEBUGLOG(("proxy_1_decoder_fileinfo: %lu\n", rc));

  // convert information to new format
  if (rc == 0)
  { // Slicing: the structure FORMAT_INFO2 is a subset of FORMAT_INFO.
    info->format->samplerate = old_info.format.samplerate;
    info->format->channels = old_info.format.channels;

    info->tech->songlength = old_info.songlength < 0 ? -1 : old_info.songlength/1000.;
    info->tech->bitrate    = old_info.bitrate;
    strlcpy(info->tech->info,    old_info.tech_info,sizeof info->tech->info);

    memcpy(info->meta->title,    old_info.title,    sizeof info->meta->title);
    memcpy(info->meta->artist,   old_info.artist,   sizeof info->meta->artist);
    memcpy(info->meta->album,    old_info.album,    sizeof info->meta->album);
    strlcpy(info->meta->year,    old_info.year,     sizeof info->meta->year);
    memcpy(info->meta->comment,  old_info.comment,  sizeof info->meta->comment);
    memcpy(info->meta->genre,    old_info.genre,    sizeof info->meta->genre);
    info->meta->track      = atoi(old_info.track);
    memcpy(info->meta->copyright,old_info.copyright,sizeof info->meta->copyright);
    info->meta->track_gain = old_info.track_gain; 
    info->meta->track_peak = old_info.track_peak; 
    info->meta->album_gain = old_info.album_gain; 
    info->meta->album_peak = old_info.album_peak; 
    info->meta_write       = old_info.saveinfo;
    // old decoders always load all kind of information
    *what = (INFOTYPE)(*what | INFO_ALL);
  }
  return rc;
}

PROXYFUNCIMP(ULONG DLLENTRY, DecoderProxy1)
proxy_1_decoder_saveinfo( DecoderProxy1* op, const char* filename, const META_INFO* info, int haveinfo )
{ DEBUGLOG(("proxy_1_decoder_saveinfo(%p, %s, {%u,%s,%s,%s,%s,%s,%s,%i,%s}, %x)\n", op, filename, 
    info->size,info->title,info->artist,info->album,info->year,info->comment,info->genre,info->track,info->copyright,
    haveinfo));
  DECODER_INFO dinfo = { sizeof dinfo };
  // this part of the structure is binary compatible
  memcpy(dinfo.title,    info->title,    sizeof dinfo.title);
  memcpy(dinfo.artist,   info->artist,   sizeof dinfo.artist);
  memcpy(dinfo.album,    info->album,    sizeof dinfo.album);
  memcpy(dinfo.year,     info->year,     sizeof dinfo.year);
  memcpy(dinfo.comment,  info->comment,  sizeof dinfo.comment);
  memcpy(dinfo.genre,    info->genre,    sizeof dinfo.genre);
  if (info->track > 0)
    sprintf(dinfo.track, "%i", info->track);
  memcpy(dinfo.copyright,info->copyright,sizeof dinfo.copyright);
  dinfo.track_gain = info->track_gain; 
  dinfo.track_peak = info->track_peak; 
  dinfo.album_gain = info->album_gain; 
  dinfo.album_peak = info->album_peak;
  //dinfo.codepage = 0; 
  dinfo.haveinfo   = haveinfo;
  // Decode file URLs
  if (strnicmp(filename, "file:", 5) == 0)
  { filename += 5;
    char* fname = (char*)alloca(strlen(filename)+1);
    strcpy(fname, filename);
    { char* cp = strchr(fname, '/');
      while (cp)
      { *cp = '\\';
        cp = strchr(cp+1, '/');
      }
    } 
    if (strncmp(fname, "\\\\\\", 3) == 0)
    { fname += 3;
      if (fname[1] == '|')
        fname[1] = ':';
    }
    filename = fname;
  }
  // Call decoder's function
  return (*op->vdecoder_saveinfo)(filename, &dinfo);
}

PROXYFUNCIMP(ULONG DLLENTRY, DecoderProxy1)
proxy_1_decoder_cdinfo( DecoderProxy1* op, const char* drive, DECODER_CDINFO* info )
{ // Serialize access to the info functions of old plug-ins.
  // In theory the must be thread safe for older PM123 releases too. In practice they are not.
  sco_ptr<Mutex::Lock> lock(op->SerializeInfo ? new Mutex::Lock(op->info_mtx) : NULL);
  return (*op->vdecoder_cdinfo)(drive, info);
}

PROXYFUNCIMP(double DLLENTRY, DecoderProxy1)
proxy_1_decoder_length( DecoderProxy1* op, void* a )
{ DEBUGLOG(("proxy_1_decoder_length(%p, %p)\n", op, a));
  int i = (*op->vdecoder_length)(a);
  return i < 0 ? -1 : i / 1000.;
}

MRESULT EXPENTRY proxy_1_decoder_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ DecoderProxy1* op = (DecoderProxy1*)WinQueryWindowPtr(hwnd, 0);
  DEBUGLOG(("proxy_1_decoder_winfn(%p, %u, %p, %p) - %p {%s}\n",
    hwnd, msg, mp1, mp2, op, op == NULL ? NULL : op->GetModuleName().cdata()));
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
      { // Make this operation atomic
        Playable::Lock lck(*pp);
        META_INFO meta = *pp->GetInfo().meta;
        const char* metadata = (const char*)PVOIDFROMMP(mp1);
        const char* titlepos;
        // extract stream title
        if( metadata ) {
          titlepos = strstr( metadata, "StreamTitle='" );
          if ( titlepos )
          { titlepos += 13;
            unsigned i;
            for( i = 0; i < sizeof( meta.title ) - 1 && *titlepos
                        && ( titlepos[0] != '\'' || titlepos[1] != ';' ); i++ )
              meta.title[i] = *titlepos++;
            meta.title[i] = 0;
            (*op->voutput_event)(op->a, DECEVENT_CHANGEMETA, &meta);
          }
        }
      }
    }
    return 0;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}

Plugin* Decoder::Factory(Module* mod)
{ return mod->GetParams().interface <= 1 ? new DecoderProxy1(mod) : new Decoder(mod);
}


void Decoder::Init()
{ PMRASSERT(WinRegisterClass(amp_player_hab(), "DecoderProxy1", &proxy_1_decoder_winfn, 0, sizeof(DecoderProxy1*)));
}


/****************************************************************************
*
* output interface
*
****************************************************************************/

PLUGIN_TYPE Output::GetType() const
{ return PLUGIN_OUTPUT;
}

/* Assigns the addresses of the out7put plug-in procedures. */
bool Output::LoadPlugin()
{ DEBUGLOG(("Output(%p{%s})::LoadPlugin()\n", this, GetModuleName().cdata()));
  const Module& mod = GetModule();

  if ( !(mod.GetParams().type & PLUGIN_OUTPUT)
    || !mod.LoadFunction(&output_init,            "output_init")
    || !mod.LoadFunction(&output_uninit,          "output_uninit")
    || !mod.LoadFunction(&output_playing_samples, "output_playing_samples")
    || !mod.LoadFunction(&output_playing_pos,     "output_playing_pos")
    || !mod.LoadFunction(&output_playing_data,    "output_playing_data")
    || !mod.LoadFunction(&output_command,         "output_command")
    || !mod.LoadFunction(&output_request_buffer,  "output_request_buffer")
    || !mod.LoadFunction(&output_commit_buffer,   "output_commit_buffer") )
    return false;

  A = NULL;
  return true;
}

bool Output::InitPlugin()
{ DEBUGLOG(("Output(%p{%s})::InitPlugin()\n", this, GetModuleName().cdata()));

  if ((*output_init)(&A) != 0)
  { A = NULL;
    return false;
  }
  RaisePluginChange(EventArgs::Init);
  return true;
}

bool Output::UninitPlugin()
{ DEBUGLOG(("Output(%p{%s})::UninitPlugin()\n", this, GetModuleName().cdata()));

  if (IsInitialized())
  { (*output_command)(A, OUTPUT_CLOSE, NULL);
    (*output_uninit)(A);
    A = NULL;
    RaisePluginChange(EventArgs::Uninit);
  }
  return true;
}


// Proxy for loading level 1 plug-ins
class OutputProxy1 : public Output
{private:
  int         DLLENTRYP(voutput_command     )( void* a, ULONG msg, OUTPUT_PARAMS* info );
  int         DLLENTRYP(voutput_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );
  ULONG       DLLENTRYP(voutput_playing_pos )( void* a );
  short       voutput_buffer[BUFSIZE/2];
  int         voutput_buffer_level;             // current level of voutput_buffer
  BOOL        voutput_trash_buffer;
  BOOL        voutput_flush_request;            // flush-request received, generate OUTEVENT_END_OF_DATA from WM_OUTPUT_OUTOFDATA
  HWND        voutput_hwnd;                     // Window handle for catching event messages
  double      voutput_posmarker;
  FORMAT_INFO voutput_format;
  int         voutput_bufsamples;
  BOOL        voutput_always_hungry;
  void        DLLENTRYP(voutput_event)(void* w, OUTEVENTTYPE event);
  void*       voutput_w;
  VDELEGATE   vd_output_command, vd_output_request_buffer, vd_output_commit_buffer, vd_output_playing_pos;

 private:
  PROXYFUNCDEF ULONG  DLLENTRY proxy_1_output_command       ( OutputProxy1* op, void* a, ULONG msg, OUTPUT_PARAMS2* info );
  PROXYFUNCDEF int    DLLENTRY proxy_1_output_request_buffer( OutputProxy1* op, void* a, const FORMAT_INFO2* format, short** buf );
  PROXYFUNCDEF void   DLLENTRY proxy_1_output_commit_buffer ( OutputProxy1* op, void* a, int len, double posmarker );
  PROXYFUNCDEF double DLLENTRY proxy_1_output_playing_pos   ( OutputProxy1* op, void* a );
  friend MRESULT EXPENTRY proxy_1_output_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
 public:
  OutputProxy1(Module* mod) : Output(mod), voutput_hwnd(NULLHANDLE), voutput_posmarker(0) {}
  virtual ~OutputProxy1();
  virtual bool LoadPlugin();
};

OutputProxy1::~OutputProxy1()
{ if (voutput_hwnd != NULLHANDLE)
    WinDestroyWindow(voutput_hwnd);
}

/* Assigns the addresses of the out7put plug-in procedures. */
bool OutputProxy1::LoadPlugin()
{ DEBUGLOG(("OutputProxy1(%p{%s})::LoadPlugin()\n", this, GetModuleName().cdata()));
  const Module& mod = GetModule();

  if ( !(mod.GetParams().type & PLUGIN_OUTPUT)
    || !mod.LoadFunction(&output_init,            "output_init")
    || !mod.LoadFunction(&output_uninit,          "output_uninit")
    || !mod.LoadFunction(&output_playing_samples, "output_playing_samples")
    || !mod.LoadFunction(&voutput_playing_pos,    "output_playing_pos")
    || !mod.LoadFunction(&output_playing_data,    "output_playing_data")
    || !mod.LoadFunction(&voutput_command,        "output_command")
    || !mod.LoadFunction(&voutput_play_samples,   "output_play_samples") )
    return false;

  output_command        = vdelegate(&vd_output_command,        &proxy_1_output_command,        this);
  output_request_buffer = vdelegate(&vd_output_request_buffer, &proxy_1_output_request_buffer, this);
  output_commit_buffer  = vdelegate(&vd_output_commit_buffer,  &proxy_1_output_commit_buffer,  this);
  output_playing_pos    = vdelegate(&vd_output_playing_pos,    &proxy_1_output_playing_pos,    this);

  A = NULL;
  return true;
}

/* virtualization of level 1 output plug-ins */
PROXYFUNCIMP(ULONG DLLENTRY, OutputProxy1)
proxy_1_output_command( OutputProxy1* op, void* a, ULONG msg, OUTPUT_PARAMS2* info )
{ DEBUGLOG(("proxy_1_output_command(%p {%s}, %p, %d, %p)\n", op, op->GetModuleName().cdata(), a, msg, info));

  if (info == NULL) // sometimes info is NULL
    return (*op->voutput_command)(a, msg, NULL);

  OUTPUT_PARAMS params = { sizeof params };

  // preprocessing
  switch (msg)
  {case OUTPUT_TRASH_BUFFERS:
    op->voutput_trash_buffer   = TRUE;
    break;

   case OUTPUT_SETUP:
    { op->voutput_hwnd = OutputProxy1::CreateProxyWindow("OutputProxy1", op);
      // convert format info
      params.formatinfo.size       = sizeof params.formatinfo;
      params.formatinfo.samplerate = info->formatinfo.samplerate;
      params.formatinfo.channels   = info->formatinfo.channels;
      params.formatinfo.bits       = 16;
      params.formatinfo.format     = WAVE_FORMAT_PCM;
      // convert DECODER_INFO2 to DECODER_INFO
      DECODER_INFO dinfo = { sizeof dinfo };
      const DECODER_INFO2& dinfo2 = *info->info;
      dinfo.format     = params.formatinfo;
      dinfo.songlength = (int)(dinfo2.tech->songlength*1000.);
      dinfo.junklength = -1;
      dinfo.bitrate    = dinfo2.tech->bitrate;
      strlcpy(dinfo.tech_info, dinfo2.tech->info, sizeof dinfo.tech_info);
      // this part of the structure is binary compatible
      memcpy(dinfo.title, dinfo2.meta->title, offsetof(META_INFO, track) - offsetof(META_INFO, title));
      if (dinfo2.meta->track)
        sprintf(dinfo.track, "%i", dinfo2.meta->track);
      dinfo.codepage   = ch_default();
      dinfo.filesize   = (int)dinfo2.phys->filesize;
      dinfo.track_gain = dinfo2.meta->track_gain;
      dinfo.track_peak = dinfo2.meta->track_peak;
      dinfo.album_gain = dinfo2.meta->album_gain;
      dinfo.album_peak = dinfo2.meta->album_peak;
      params.info = &dinfo;
      params.hwnd                  = op->voutput_hwnd;
      break;
    }
  }
  params.buffersize            = BUFSIZE;
  params.boostclass            = DECODER_HIGH_PRIORITY_CLASS;
  params.normalclass           = DECODER_LOW_PRIORITY_CLASS;
  params.boostdelta            = DECODER_HIGH_PRIORITY_DELTA;
  params.normaldelta           = DECODER_LOW_PRIORITY_DELTA;
  params.nobuffermode          = FALSE;
  params.error_display         = info->error_display;
  params.info_display          = info->info_display;
  params.volume                = (char)(info->volume*100+.5);
  params.amplifier             = info->amplifier;
  params.pause                 = info->pause;
  params.temp_playingpos       = tstmp_f2i(info->temp_playingpos);
  if (info->URI != NULL && strnicmp(info->URI, "file://", 7) == 0)
    params.filename            = info->URI + 7;
   else
    params.filename            = info->URI;

  // call plug-in
  int r = (*op->voutput_command)(a, msg, &params);

  // postprocessing
  switch (msg)
  {case OUTPUT_SETUP:
    op->voutput_buffer_level   = 0;
    op->voutput_trash_buffer   = FALSE;
    op->voutput_flush_request  = FALSE;
    op->voutput_always_hungry  = params.always_hungry;
    op->voutput_event          = info->output_event;
    op->voutput_w              = info->w;
    op->voutput_format.size    = sizeof op->voutput_format;
    op->voutput_format.bits    = 16;
    op->voutput_format.format  = WAVE_FORMAT_PCM;
    break;

   case OUTPUT_CLOSE:
    OutputProxy1::DestroyProxyWindow(op->voutput_hwnd);
    op->voutput_hwnd = NULLHANDLE;
  }
  DEBUGLOG(("proxy_1_output_command: %d\n", r));
  return r;
}

PROXYFUNCIMP(int DLLENTRY, OutputProxy1)
proxy_1_output_request_buffer( OutputProxy1* op, void* a, const FORMAT_INFO2* format, short** buf )
{
  #ifdef DEBUG
  if (format != NULL)
    DEBUGLOG(("proxy_1_output_request_buffer(%p, %p, {%i,%i,%i}, %p) - %d\n",
      op, a, format->size, format->samplerate, format->channels, buf, op->voutput_buffer_level));
   else
    DEBUGLOG(("proxy_1_output_request_buffer(%p, %p, %p, %p) - %d\n", op, a, format, buf, op->voutput_buffer_level));
  #endif

  if (op->voutput_trash_buffer)
  { op->voutput_buffer_level = 0;
    op->voutput_trash_buffer = FALSE;
  }

  if ( op->voutput_buffer_level != 0
    && ( buf == 0
      || (op->voutput_format.samplerate != format->samplerate || op->voutput_format.channels != format->channels) ))
  { // flush
    (*op->voutput_play_samples)(a, &op->voutput_format, (char*)op->voutput_buffer, op->voutput_buffer_level * op->voutput_format.channels * sizeof(short), tstmp_f2i(op->voutput_posmarker));
    op->voutput_buffer_level = 0;
  }
  if (buf == 0)
  { if (op->voutput_always_hungry)
      (*op->voutput_event)(op->voutput_w, OUTEVENT_END_OF_DATA);
     else
      op->voutput_flush_request = TRUE; // wait for WM_OUTPUT_OUTOFDATA
    return 0;
  }

  *buf = op->voutput_buffer + op->voutput_buffer_level * format->channels;
  op->voutput_format.samplerate = format->samplerate;
  op->voutput_format.channels   = format->channels;
  op->voutput_bufsamples        = sizeof op->voutput_buffer / sizeof *op->voutput_buffer / format->channels;
  DEBUGLOG(("proxy_1_output_request_buffer: %d\n", op->voutput_bufsamples - op->voutput_buffer_level));
  return op->voutput_bufsamples - op->voutput_buffer_level;
}

PROXYFUNCIMP(void DLLENTRY, OutputProxy1)
proxy_1_output_commit_buffer( OutputProxy1* op, void* a, int len, double posmarker )
{ DEBUGLOG(("proxy_1_output_commit_buffer(%p {%s}, %p, %i, %g) - %d\n",
    op, op->GetModuleName().cdata(), a, len, posmarker, op->voutput_buffer_level));

  if (op->voutput_buffer_level == 0)
    op->voutput_posmarker = posmarker;

  op->voutput_buffer_level += len;
  if (op->voutput_buffer_level == op->voutput_bufsamples)
  { (*op->voutput_play_samples)(a, &op->voutput_format, (char*)op->voutput_buffer, op->voutput_buffer_level * op->voutput_format.channels * sizeof(short), tstmp_f2i(op->voutput_posmarker));
    op->voutput_buffer_level = 0;
  }
}

PROXYFUNCIMP(double DLLENTRY, OutputProxy1)
proxy_1_output_playing_pos( OutputProxy1* op, void* a )
{ DEBUGLOG(("proxy_1_output_playing_pos(%p {%s}, %p)\n", op, op->GetModuleName().cdata(), a));
  return tstmp_i2f((*op->voutput_playing_pos)(a), op->voutput_posmarker);
}

MRESULT EXPENTRY proxy_1_output_winfn(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{ OutputProxy1* op = (OutputProxy1*)WinQueryWindowPtr(hwnd, 0);
  DEBUGLOG2(("proxy_1_output_winfn(%p, %u, %p, %p) - %p {%s}\n", hwnd, msg, mp1, mp2, op, op == NULL ? NULL : op->ModuleName.cdata()));
  switch (msg)
  {case WM_PLAYERROR:
    (*op->voutput_event)(op->voutput_w, OUTEVENT_PLAY_ERROR);
    return 0;
   case WM_OUTPUT_OUTOFDATA:
    if (op->voutput_flush_request) // don't care unless we have a flush_request condition
    { op->voutput_flush_request = FALSE;
      (*op->voutput_event)(op->A, OUTEVENT_END_OF_DATA);
    }
    return 0;
  }
  return WinDefWindowProc(hwnd, msg, mp1, mp2);
}


Plugin* Output::Factory(Module* mod)
{ return mod->GetParams().interface <= 1 ? new OutputProxy1(mod) : new Output(mod);
}


void Output::Init()
{ PMRASSERT(WinRegisterClass(amp_player_hab(), "OutputProxy1", &proxy_1_output_winfn, 0, sizeof(OutputProxy1*)));
}


/****************************************************************************
*
* filter interface
*
****************************************************************************/

PLUGIN_TYPE Filter::GetType() const
{ return PLUGIN_FILTER;
}

/* Assigns the addresses of the filter plug-in procedures. */
bool Filter::LoadPlugin()
{ DEBUGLOG(("Filter(%p{%s})::LoadPlugin\n", this, GetModuleName().cdata()));
  const Module& mod = GetModule();

  if ( !(mod.GetParams().type & PLUGIN_FILTER)
    || !mod.LoadFunction(&filter_init,   "filter_init")
    || !mod.LoadFunction(&filter_update, "filter_update")
    || !mod.LoadFunction(&filter_uninit, "filter_uninit") )
    return false;

  F = NULL;
  Enabled = true;
  return true;
}

bool Filter::InitPlugin()
{ return true; // filters are not initialized unless they are used
}

bool Filter::UninitPlugin()
{ DEBUGLOG(("Filter(%p{%s})::UninitPlugin\n", this, GetModuleName().cdata()));

  if (IsInitialized())
  { (*filter_uninit)(F);
    F = NULL;
    RaisePluginChange(EventArgs::Uninit);
  }
  return true;
}

bool Filter::Initialize(FILTER_PARAMS2* params)
{ DEBUGLOG(("Filter(%p{%s})::Initialize(%p)\n", this, GetModuleName().cdata(), params));

  FILTER_PARAMS2 par = *params;
  if (IsInitialized() || (*filter_init)(&F, params) != 0)
    return false;

  if (F == NULL)
  { // plug-in does not require local structures
    // => pass the pointer of the next stage and skip virtualization of untouched function
    F = par.a;
  } else
  { // virtualize untouched functions
    if (par.output_command          == params->output_command)
      params->output_command         = vreplace1(&VRStubs[0], par.output_command, par.a);
    if (par.output_playing_samples  == params->output_playing_samples)
      params->output_playing_samples = vreplace1(&VRStubs[1], par.output_playing_samples, par.a);
    if (par.output_request_buffer   == params->output_request_buffer)
      params->output_request_buffer  = vreplace1(&VRStubs[2], par.output_request_buffer, par.a);
    if (par.output_commit_buffer    == params->output_commit_buffer)
      params->output_commit_buffer   = vreplace1(&VRStubs[3], par.output_commit_buffer, par.a);
    if (par.output_playing_pos      == params->output_playing_pos)
      params->output_playing_pos     = vreplace1(&VRStubs[4], par.output_playing_pos, par.a);
    if (par.output_playing_data     == params->output_playing_data)
      params->output_playing_data    = vreplace1(&VRStubs[5], par.output_playing_data, par.a);
  }
  RaisePluginChange(EventArgs::Init);
  return true;
}


// proxy for level 1 filters
class FilterProxy1 : public Filter
{private:
  int   DLLENTRYP(vfilter_init         )( void** f, FILTER_PARAMS* params );
  BOOL  DLLENTRYP(vfilter_uninit       )( void*  f );
  int   DLLENTRYP(vfilter_play_samples )( void*  f, const FORMAT_INFO* format, const char *buf, int len, int posmarker );
  void* vf;
  int   DLLENTRYP(output_request_buffer)( void*  a, const FORMAT_INFO2* format, short** buf );
  void  DLLENTRYP(output_commit_buffer )( void*  a, int len, double posmarker );
  void* a;
  void  DLLENTRYP(error_display        )( const char* );
  FORMAT_INFO vformat;                      // format of the samples
  short       vbuffer[BUFSIZE/2];           // buffer to store incoming samples
  int         vbufsamples;                  // size of vbuffer in samples
  int         vbuflevel;                    // current filled to vbuflevel
  double      vposmarker;                   // starting point of the current buffer
  BOOL        trash_buffer;                 // TRUE: signal to discard any buffer content
  VDELEGATE   vd_filter_init;
  VREPLACE1   vr_filter_update;
  VREPLACE1   vr_filter_uninit;

 private:
  PROXYFUNCDEF ULONG DLLENTRY proxy_1_filter_init          ( FilterProxy1* pp, void** f, FILTER_PARAMS2* params );
  PROXYFUNCDEF void  DLLENTRY proxy_1_filter_update        ( FilterProxy1* pp, const FILTER_PARAMS2* params );
  PROXYFUNCDEF BOOL  DLLENTRY proxy_1_filter_uninit        ( void* f ); // empty stub
  PROXYFUNCDEF int   DLLENTRY proxy_1_filter_request_buffer( FilterProxy1* f, const FORMAT_INFO2* format, short** buf );
  PROXYFUNCDEF void  DLLENTRY proxy_1_filter_commit_buffer ( FilterProxy1* f, int len, double posmarker );
  PROXYFUNCDEF int   DLLENTRY proxy_1_filter_play_samples  ( FilterProxy1* f, const FORMAT_INFO* format, const char *buf, int len, int posmarker );
 public:
  FilterProxy1(Module* mod) : Filter(mod) {}
  virtual bool LoadPlugin();
};

bool FilterProxy1::LoadPlugin()
{ DEBUGLOG(("FilterProxy1(%p{%s})::LoadPlugin()\n", this, GetModuleName().cdata()));
  const Module& mod = GetModule();

  if ( !(mod.GetParams().type & PLUGIN_FILTER)
    || !mod.LoadFunction(&vfilter_init,         "filter_init")
    || !mod.LoadFunction(&vfilter_uninit,       "filter_uninit")
    || !mod.LoadFunction(&vfilter_play_samples, "filter_play_samples") )
    return false;

  filter_init   = vdelegate(&vd_filter_init,   &proxy_1_filter_init,   this);
  filter_update = (void DLLENTRYPF()(void*, const FILTER_PARAMS2*)) // type of parameter is replaced too
                  vreplace1(&vr_filter_update, &proxy_1_filter_update, this);
  // filter_uninit is initialized at the filter_init call to a non-no-op function
  // However, the returned pointer will stay the same.
  filter_uninit = vreplace1(&vr_filter_uninit, &proxy_1_filter_uninit, (void*)NULL);

  F = NULL;
  Enabled = true;
  return true;
}

PROXYFUNCIMP(ULONG DLLENTRY, FilterProxy1)
proxy_1_filter_init( FilterProxy1* pp, void** f, FILTER_PARAMS2* params )
{ DEBUGLOG(("proxy_1_filter_init(%p{%s}, %p, %p{a=%p})\n", pp, pp->GetModuleName().cdata(), f, params, params->a));

  FILTER_PARAMS par;
  par.size                = sizeof par;
  par.output_play_samples = (int DLLENTRYPF()(void*, const FORMAT_INFO*, const char*, int, int))
                            &PROXYFUNCREF(FilterProxy1)proxy_1_filter_play_samples;
  par.a                   = pp;
  par.audio_buffersize    = BUFSIZE;
  par.error_display       = params->error_display;
  par.info_display        = params->info_display;
  par.pm123_getstring     = params->pm123_getstring;
  par.pm123_control       = params->pm123_control;
  int r = (*pp->vfilter_init)(&pp->vf, &par);
  if (r != 0)
    return r;
  // save some values
  pp->output_request_buffer = params->output_request_buffer;
  pp->output_commit_buffer  = params->output_commit_buffer;
  pp->a                     = params->a;
  pp->error_display         = params->error_display;
  // setup internals
  pp->vbuflevel             = 0;
  pp->trash_buffer          = FALSE;
  pp->vformat.size          = sizeof pp->vformat.size;
  pp->vformat.bits          = 16;
  pp->vformat.format        = WAVE_FORMAT_PCM;
  // replace the unload function
  vreplace1(&pp->vr_filter_uninit, pp->vfilter_uninit, pp->vf);
  // now return some values
  *f = pp;
  params->output_request_buffer = (int  DLLENTRYPF()(void*, const FORMAT_INFO2*, short**))
                                  &PROXYFUNCREF(FilterProxy1)proxy_1_filter_request_buffer;
  params->output_commit_buffer  = (void DLLENTRYPF()(void*, int, double))
                                  &PROXYFUNCREF(FilterProxy1)proxy_1_filter_commit_buffer;
  return 0;
}

PROXYFUNCIMP(void DLLENTRY, FilterProxy1)
proxy_1_filter_update( FilterProxy1* pp, const FILTER_PARAMS2* params )
{ DEBUGLOG(("proxy_1_filter_update(%p{%s}, %p)\n", pp, pp->GetModuleName().cdata(), params));

  CritSect cs;
  // replace function pointers
  pp->output_request_buffer = params->output_request_buffer;
  pp->output_commit_buffer  = params->output_commit_buffer;
  pp->a                     = params->a;
}

PROXYFUNCIMP(BOOL DLLENTRY, FilterProxy1)
proxy_1_filter_uninit( void* )
{ return TRUE;
}

PROXYFUNCIMP(int DLLENTRY, FilterProxy1)
proxy_1_filter_request_buffer( FilterProxy1* pp, const FORMAT_INFO2* format, short** buf )
{ DEBUGLOG(("proxy_1_filter_request_buffer(%p, %p, %p)\n", pp, format, buf));

  if ( pp->trash_buffer )
  { pp->vbuflevel = 0;
    pp->trash_buffer = FALSE;
  }

  if ( buf == 0
    || ( pp->vbuflevel != 0 &&
         (pp->vformat.samplerate != format->samplerate || pp->vformat.channels != format->channels) ))
  { // local flush
    DEBUGLOG(("proxy_1_filter_request_buffer: local flush: %d\n", pp->vbuflevel));
    // Oh well, the old output plug-ins seem to play some more samples in doubt.
    // memset( pp->vbuffer + pp->vbuflevel * pp->vformat.channels, 0, (pp->vbufsamples - pp->vbuflevel) * pp->vformat.channels * sizeof(short) );
    (*pp->vfilter_play_samples)(pp->vf, &pp->vformat, (char*)pp->vbuffer, pp->vbuflevel * pp->vformat.channels * sizeof(short), tstmp_f2i(pp->vposmarker));
  }
  if ( buf == 0 )
  { return (*pp->output_request_buffer)( pp->a, format, NULL );
  }
  pp->vformat.samplerate = format->samplerate;
  pp->vformat.channels   = format->channels;
  pp->vbufsamples        = sizeof pp->vbuffer / sizeof *pp->vbuffer / format->channels;

  DEBUGLOG(("proxy_1_filter_request_buffer: %d\n", pp->vbufsamples - pp->vbuflevel));
  *buf = pp->vbuffer + pp->vbuflevel * format->channels;
  return pp->vbufsamples - pp->vbuflevel;
}

PROXYFUNCIMP(void DLLENTRY, FilterProxy1)
proxy_1_filter_commit_buffer( FilterProxy1* pp, int len, double posmarker )
{ DEBUGLOG(("proxy_1_filter_commit_buffer(%p, %d, %g)\n", pp, len, posmarker));

  if (len == 0)
    return;

  if (pp->vbuflevel == 0)
    pp->vposmarker = posmarker;

  pp->vbuflevel += len;
  if (pp->vbuflevel == pp->vbufsamples)
  { // buffer full
    DEBUGLOG(("proxy_1_filter_commit_buffer: full: %d\n", pp->vbuflevel));
    (*pp->vfilter_play_samples)(pp->vf, &pp->vformat, (char*)pp->vbuffer, BUFSIZE, tstmp_f2i(pp->vposmarker));
    pp->vbuflevel = 0;
  }
}

PROXYFUNCIMP(int DLLENTRY, FilterProxy1)
proxy_1_filter_play_samples(FilterProxy1* pp, const FORMAT_INFO* format, const char *buf, int len, int posmarker_i)
{ DEBUGLOG(("proxy_1_filter_play_samples(%p, %p{%d,%d,%d,%d}, %p, %d, %d)\n",
    pp, format, format->samplerate, format->channels, format->bits, format->format, buf, len, posmarker_i));

  if (format->format != WAVE_FORMAT_PCM || format->bits != 16)
  { (*pp->error_display)("The proxy for old style filter plug-ins can only handle 16 bit raw PCM data.");
    return 0;
  }
  double posmarker = tstmp_i2f(posmarker_i, pp->vposmarker);
  len /= pp->vformat.channels * sizeof(short);
  int rem = len;
  while (rem != 0)
  { // request new buffer
    short* dest;
    int dlen = (*pp->output_request_buffer)( pp->a, (FORMAT_INFO2*)format, &dest );
    DEBUGLOG(("proxy_1_filter_play_samples: now at %p %d, %p, %d\n", buf, rem, dest, dlen));
    if (dlen <= 0)
      return 0; // error
    if (dlen > rem)
      dlen = rem;
    // store data
    memcpy( dest, buf, dlen * pp->vformat.channels * sizeof(short) );
    // commit destination
    (*pp->output_commit_buffer)( pp->a, dlen, posmarker + (double)(len-rem)/format->samplerate );
    buf += dlen * pp->vformat.channels * sizeof(short);
    rem -= dlen;
  }
  return len * pp->vformat.channels * sizeof(short);
}


Plugin* Filter::Factory(Module* mod)
{ return mod->GetParams().interface <= 1 ? new FilterProxy1(mod) : new Filter(mod);
}


/****************************************************************************
*
* visualization interface
*
****************************************************************************/

PLUGIN_TYPE Visual::GetType() const
{ return PLUGIN_VISUAL;
}

/* Assigns the addresses of the visual plug-in procedures. */
bool Visual::LoadPlugin()
{ DEBUGLOG(("Visual(%p{%s})::LoadPlugin()\n", this, GetModuleName().cdata()));
  const Module& mod = GetModule();

  if ( !(mod.GetParams().type & PLUGIN_VISUAL)
    || !mod.LoadFunction(&plugin_deinit, "plugin_deinit")
    || !mod.LoadFunction(&plugin_init,   "vis_init") )
    return false;

  Enabled = true;
  Hwnd = NULLHANDLE;
  return true;
}

bool Visual::InitPlugin()
{ return true; // no automatic init
}

bool Visual::UninitPlugin()
{ if (IsInitialized())
  { (*plugin_deinit)(0);
    Hwnd = NULLHANDLE;
    RaisePluginChange(EventArgs::Uninit);
  }
  return true;
}

bool Visual::Initialize(HWND hwnd, PLUGIN_PROCS* procs, int id)
{ DEBUGLOG(("Visual(%p{%s})::initialize(%x, %p, %d) - %d %d, %d %d, %s\n",
    this, GetModuleName().cdata(), hwnd, procs, id, Props.x, Props.y, Props.cx, Props.cy, Props.param));

  VISPLUGININIT visinit;
  visinit.x       = Props.x;
  visinit.y       = Props.y;
  visinit.cx      = Props.cx;
  visinit.cy      = Props.cy;
  visinit.hwnd    = hwnd;
  visinit.procs   = procs;
  visinit.id      = id;
  visinit.param   = Props.param;

  Hwnd = (*plugin_init)(&visinit);
  if (Hwnd == NULLHANDLE)
    return false;
  RaisePluginChange(EventArgs::Init);
  return true;
}

void Visual::SetProperties(const VISUAL_PROPERTIES* data)
{
  #ifdef DEBUG
  if (data)
    DEBUGLOG(("Visual(%p{%s})::set_properties(%p{%d %d, %d %d, %s})\n",
      this, GetModuleName().cdata(), data, data->x, data->y, data->cx, data->cy, data->param));
  else
    DEBUGLOG(("Visual(%p{%s})::set_properties(%p)\n", this, GetModuleName().cdata(), data));
  #endif
  static const VISUAL_PROPERTIES def_visuals = {0,0,0,0, FALSE, ""};
  Props = data == NULL ? def_visuals : *data;
}


Plugin* Visual::Factory(Module* mod)
{ if (mod->GetParams().interface == 0)
  { amp_player_error( "Could not load visual plug-in %s because it is designed for PM123 before vesion 1.32\n"
                      "Please get a newer version of this plug-in which supports at least interface revision 1.",
      mod->GetModuleName().cdata());
    return NULL;
  }
  return new Visual(mod);
}

