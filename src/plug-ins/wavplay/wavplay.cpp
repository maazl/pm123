/*
 * Copyright 2013 Marcel Mueller
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#define  INCL_DOS
#define  INCL_WIN

#include "wavplay.h"
#include <plugin.h>
#include <utilfct.h>
#include <snprintf.h>
#include <os2.h>

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <process.h>

#include <debuglog.h>


PLUGIN_CONTEXT Ctx;

static const float SkipWindow = .1;

static sf_count_t DLLENTRY vio_fsize(void* x)
{ return xio_fsizel((XFILE*)x);
}

static sf_count_t DLLENTRY vio_seek(sf_count_t offset, int whence, void* x)
{ return xio_fseekl((XFILE*)x, offset, (XIO_SEEK)whence);
}

static sf_count_t DLLENTRY vio_read(void* ptr, sf_count_t count, void* x)
{ return xio_fread(ptr, 1, (int)count, (XFILE*)x);
}

static sf_count_t DLLENTRY vio_write(const void* ptr, sf_count_t count, void* x)
{ return xio_fwrite(ptr, 1, (int)count, (XFILE*)x);
}

static sf_count_t DLLENTRY vio_tell(void *x)
{ return xio_ftell((XFILE*)x);
}

static const SF_VIRTUAL_IO vio_procs = { &vio_fsize, &vio_seek, &vio_read, &vio_write, &vio_tell };

/* Decoding thread. */
static void TFNENTRY decoder_thread(void* arg)
{
  ULONG resetcount;
  DECODER_STRUCT* w = (DECODER_STRUCT*)arg;
  sf_count_t markerpos = 0;

  w->SkipSpeed = 0;
  w->Stop = false;

  if ((w->XFile = xio_fopen(w->URL, "rbXU")) == NULL)
  { xstring error;
    error.sprintf("Unable open file:\n%s\n%s", w->URL.cdata(), xio_strerror(xio_errno()));
    (*w->DecEvent)(w->A, DECEVENT_PLAYERROR, (void*)error.cdata());
    goto end;
  }

  if ((w->Sndfile = sf_open_virtual((SF_VIRTUAL_IO*)&vio_procs, SFM_READ, &w->SFInfo, w->XFile)) == NULL)
  { xstring error;
    error.sprintf("Unable open file:\n%s\nUnsupported format of the file.", w->URL.cdata());
    (*w->DecEvent)(w->A, DECEVENT_PLAYERROR, (void*)error.cdata());
    goto end;
  }

  // After opening a new file we so are in its beginning.
  if (w->JumpToPos == 0)
    w->JumpToPos = -1;

  w->Format.channels   = w->SFInfo.channels;
  w->Format.samplerate = w->SFInfo.samplerate;

  for(;;)
  {
    DosWaitEventSem(w->play, SEM_INDEFINITE_WAIT);

    if (w->Stop)
      break;

    w->status = DECODER_PLAYING;

    while (!w->Stop)
    {
      int read;
      DosResetEventSem(w->play, &resetcount);

      if (w->JumpToPos >= 0)
      { sf_count_t jumptoframe = (sf_count_t)(w->JumpToPos * w->SFInfo.samplerate +.5);
        markerpos = sf_seek(w->Sndfile, jumptoframe, SEEK_SET);
        if (markerpos < 0)
          break;
        w->JumpToPos = -1;
        (*w->DecEvent)(w->A, DECEVENT_SEEKSTOP, NULL);
      } else if (w->SkipSpeed && w->NextSkip <= 0)
      { sf_count_t jumptoframe =
          (sf_count_t)(w->SkipSpeed * (SkipWindow * w->SFInfo.samplerate) - w->NextSkip +.5);
        markerpos = sf_seek(w->Sndfile, jumptoframe, SEEK_CUR);
        if (markerpos < 0)
          break;
        w->NextSkip = (sf_count_t)(SkipWindow * w->SFInfo.samplerate +.5);
      }
      // Request target buffer
      float* buffer;
      int frames = (*w->OutRequestBuffer)(w->A, &w->Format, &buffer);
      if (frames <= 0)
      { (*w->DecEvent)(w->A, DECEVENT_PLAYERROR, NULL);
        goto end;
      }

      read = sf_readf_float(w->Sndfile, buffer, frames);
      if (read <= 0)
        break;

      (*w->OutCommitBuffer)(w->A, read, (PM123_TIME)markerpos / w->SFInfo.samplerate);

      w->NextSkip -= read;
      markerpos += read;
    }
    // File finished
    (*w->DecEvent)(w->A, DECEVENT_PLAYSTOP, NULL);
    w->status = DECODER_STOPPING;
  }

end:
  DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
  if (w->Sndfile)
  { sf_close(w->Sndfile);
    w->Sndfile = NULL;
  }
  if (w->XFile)
  { xio_fclose(w->XFile);
    w->XFile = NULL;
  }

  w->status = DECODER_STOPPED;
  w->decodertid = -1;
  DosReleaseMutexSem( w->mutex );
}

// libsndfile sub formats have no short name
static const char* sf_subformats[] =
{ NULL, "S8", "16", "24"
, "32", "U8", "float", "double"
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL

, "uLaw", "aLaw", "imaADPCM", "msADPCM"
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL

, "GSM610", "voxADPCM", NULL, NULL
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL

, "G721_32", "G723_24", "G723_40", NULL
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL

, "DWVW_12"
, "DWVW_16"
, "DWVW_24"
, "DWVW_N"
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL

, "DPCM_8", "DPCM_16", NULL, NULL
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL
, NULL, NULL, NULL, NULL

, "Vorbis"
};

/// Prefill format in case of RAW.
/// @param tech Technical info of the song.
/// @param sfinfo Place format info here.
static void decoder_inject_raw_format(const TECH_INFO& tech, SF_INFO& sfinfo)
{
  if (tech.samplerate > 0 && tech.channels > 0 && tech.format && tech.format.startsWithI("RAW "))
  { char subformat[6];
    char endianess[4];
    *endianess = 0;
    size_t len;
    if (sscanf(tech.format + 4, "%5s%n%3s%n", subformat, &len, endianess, &len) >= 1)
    { // search subformat
      const char** fp = sf_subformats;
      const char** fpe = fp + countof(sf_subformats);
      while (fp != fpe)
      { if (*fp && stricmp(subformat, *fp) == 0)
        { sfinfo.format = SF_FORMAT_RAW | (fp - sf_subformats);
          if (stricmp(endianess, "LE") == 0)
            sfinfo.format |= SF_ENDIAN_LITTLE;
          else if (stricmp(endianess, "BE") == 0)
            sfinfo.format |= SF_ENDIAN_BIG;
          else
            sfinfo.format |= SF_ENDIAN_CPU;
          sfinfo.samplerate = tech.samplerate;
          sfinfo.channels = tech.channels;
          break;
        }
        ++fp;
      }
    }
  }
}

/* Init function is called when PM123 needs the specified decoder to play
   the stream demanded by the user. */
int DLLENTRY decoder_init(DECODER_STRUCT** returnw)
{
  DECODER_STRUCT* w = (DECODER_STRUCT*)calloc(sizeof(*w), 1);
  *returnw = w;

  DosCreateEventSem( NULL, &w->play,  0, FALSE );
  DosCreateMutexSem( NULL, &w->mutex, 0, FALSE );

  w->decodertid = -1;
  w->status = DECODER_STOPPED;
  return PLUGIN_OK;
}

/* Uninit function is called when another decoder than yours is needed. */
BOOL DLLENTRY decoder_uninit(DECODER_STRUCT* w)
{
  decoder_command ( w, DECODER_STOP, NULL );
  wait_thread( w->decodertid, 5000 );
  DosCloseEventSem( w->play  );
  DosCloseMutexSem( w->mutex );
  free( w );
  return TRUE;
}

ULONG DLLENTRY
decoder_command(DECODER_STRUCT* w, DECMSGTYPE msg, const DECODER_PARAMS2* info)
{
  switch(msg)
  {
    case DECODER_SETUP:
      w->OutRequestBuffer = info->OutRequestBuffer;
      w->OutCommitBuffer  = info->OutCommitBuffer;
      w->DecEvent         = info->DecEvent;
      w->A                = info->A;
      break;

    case DECODER_PLAY:
    {
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

      if( w->decodertid != -1 ) {
        DosReleaseMutexSem( w->mutex );
        if( w->status == DECODER_STOPPED ) {
          decoder_command( w, DECODER_STOP, NULL );
          DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        } else {
          return PLUGIN_GO_ALREADY;
        }
      }

      w->URL = info->URL;
      TECH_INFO tech;
      // remove CV qualifiers, copy only required infos.
      { const volatile TECH_INFO& ti = *info->Info->tech;
        tech.samplerate = ti.samplerate;
        tech.channels   = ti.channels;
        tech.attributes = ti.attributes;
        tech.format     = ti.format;
      }
      decoder_inject_raw_format(tech, w->SFInfo);

      w->JumpToPos  = info->JumpTo;
      w->status     = DECODER_STARTING;
      w->decodertid = _beginthread(decoder_thread, 0, 65535, w);

      DosPostEventSem   ( w->play  );
      DosReleaseMutexSem( w->mutex );
      break;
    }

    case DECODER_STOP:
    {
      DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );

      if( w->decodertid == -1 ) {
        DosReleaseMutexSem( w->mutex );
        return PLUGIN_GO_ALREADY;
      }

      w->Stop = true;
      w->status = DECODER_STOPPING;

      if (w->XFile)
        xio_fabort(w->XFile);

      DosPostEventSem( w->play );
      DosReleaseMutexSem( w->mutex );
      break;
    }

    case DECODER_FFWD:
      if (info->SkipSpeed)
      { DosRequestMutexSem( w->mutex, SEM_INDEFINITE_WAIT );
        if (w->decodertid == -1 || xio_can_seek(w->XFile) != XIO_CAN_SEEK_FAST )
        { DosReleaseMutexSem( w->mutex );
          return PLUGIN_UNSUPPORTED;
        }
        DosReleaseMutexSem( w->mutex );
      }
      w->NextSkip = 0;
      w->SkipSpeed = info->SkipSpeed;
      break;

    case DECODER_JUMPTO:
      w->JumpToPos = info->JumpTo;
      DosPostEventSem( w->play );
      break;

    default:
      return PLUGIN_UNSUPPORTED;
   }

   return PLUGIN_OK;
}

/* Returns current status of the decoder. */
ULONG DLLENTRY decoder_status(DECODER_STRUCT* w)
{ return w->status;
}

/* Returns number of milliseconds the stream lasts. */
PM123_TIME DLLENTRY decoder_length(DECODER_STRUCT* w)
{
  if( w->SFInfo.samplerate ) {
    return w->SFInfo.frames / w->SFInfo.samplerate;
  }
  return 0;
}


ULONG DLLENTRY
decoder_fileinfo(const char* url, XFILE* handle, int* what, const INFO_BUNDLE* info,
  DECODER_INFO_ENUMERATION_CB cb, void* param)
{ DEBUGLOG(("wavplay:decoder_fileinfo(%s, %p, *%x ...)\n", url, handle, what));

  if (!handle)
    return PLUGIN_NO_PLAY;

  TECH_INFO& tech = *info->tech;

  SF_INFO sfinfo = {0};
  decoder_inject_raw_format(tech, sfinfo);
  SNDFILE* sndfile = sf_open_virtual((SF_VIRTUAL_IO*)&vio_procs, SFM_READ, &sfinfo, handle);
  if (sndfile == NULL)
    return PLUGIN_NO_PLAY;

  SF_FORMAT_INFO format_info;
  SF_FORMAT_INFO format_more;
  format_info.format = sfinfo.format & SF_FORMAT_TYPEMASK;
  if (sf_command(sndfile, SFC_GET_FORMAT_INFO, &format_info, sizeof format_info) != 0)
  { format_info.name = "?";
    format_info.extension = "?";
  }
  format_more.format = sfinfo.format & SF_FORMAT_SUBMASK;
  bool have_subformat = sf_command(sndfile, SFC_GET_FORMAT_INFO, &format_more, sizeof format_more) == 0;
  const char* endian = sfinfo.format & SF_ENDIAN_LITTLE ? " le" : sfinfo.format & SF_ENDIAN_BIG ? " be" : "";
  const char* chan = sfinfo.channels == 2 ? " stereo" : sfinfo.channels == 1 ? "mono" : "";

  tech.samplerate = sfinfo.samplerate;
  tech.channels = sfinfo.channels;
  tech.attributes = TATTR_SONG;
  tech.info.sprintf(have_subformat ? "%s %f kHz%s %s" : "%s %f kHz%s",
    format_info.name, sfinfo.samplerate/1000., chan, format_more.name);
  if (have_subformat)
    tech.format.sprintf("%s %s%s", format_info.extension, sf_subformats[sfinfo.format&SF_FORMAT_SUBMASK], endian);
  else
    tech.format = format_info.extension;

  info->obj->songlength = (PM123_TIME)sfinfo.frames / sfinfo.samplerate;
  info->obj->bitrate = sfinfo.samplerate * sfinfo.channels * 8;
  // Formats with exact bitrate
  switch (sfinfo.format & SF_FORMAT_SUBMASK)
  {default:
    info->obj->bitrate = -1; // don't know;
   case SF_FORMAT_PCM_S8:
   case SF_FORMAT_PCM_U8:
    break;
   case SF_FORMAT_PCM_16:
    info->obj->bitrate *= 2;
    break;
   case SF_FORMAT_PCM_24:
    info->obj->bitrate *= 3;
    break;
   case SF_FORMAT_PCM_32:
   case SF_FORMAT_FLOAT:
    info->obj->bitrate *= 4;
    break;
   case SF_FORMAT_DOUBLE:
    info->obj->bitrate *= 8;
  }

  META_INFO& meta = *info->meta;
  meta.title     = sf_get_string(sndfile, SF_STR_TITLE);
  meta.artist    = sf_get_string(sndfile, SF_STR_ARTIST);
  meta.album     = sf_get_string(sndfile, SF_STR_ALBUM);
  meta.year      = sf_get_string(sndfile, SF_STR_DATE);
  meta.comment   = sf_get_string(sndfile, SF_STR_COMMENT);
  meta.genre     = sf_get_string(sndfile, SF_STR_GENRE);
  meta.track     = sf_get_string(sndfile, SF_STR_TRACKNUMBER);
  meta.copyright = sf_get_string(sndfile, SF_STR_COPYRIGHT);

  *what |= INFO_TECH|INFO_OBJ|INFO_META|INFO_ATTR|INFO_CHILD;

  sf_close(sndfile);

  return PLUGIN_OK;
}

static const DECODER_FILETYPE filetypes[] =
{ { "Digital Audio", "WAV",  "*.wav", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "AIFF", "*.aif", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "AIFF", "*.aiff",DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "AU",   "*.au",  DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "AVR",  "*.avr", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "CAF",  "*.caf", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "IFF",  "*.iff", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "MAT",  "*.mat", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "PAF",  "*.paf", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "PVF",  "*.pvf", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "SD2",  "*.sd2", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "SDS",  "*.sds", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "SF",   "*.SF",  DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "VOC",  "*.voc", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "W64",  "*.w64", DECODER_FILENAME|DECODER_URL|DECODER_SONG }
, { "Digital Audio", "XI",   "*.xi",  DECODER_FILENAME|DECODER_URL|DECODER_SONG }
};

ULONG DLLENTRY decoder_support(const DECODER_FILETYPE** types, int* count)
{ *types = filetypes;
  *count = countof(filetypes);
  return DECODER_FILENAME|DECODER_URL|DECODER_SONG;
}

/* Returns information about plug-in. */
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* param)
{ param->type         = PLUGIN_DECODER;
  param->author       = "Dmitry A.Steklenev, M.Mueller";
  param->desc         = "WAV Play 3.00";
  param->configurable = FALSE;
  param->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{ Ctx = *ctx;
  return 0;
}


#if defined(__IBMC__) && defined(__DEBUG_ALLOC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return 1UL;
  } else {
    _dump_allocated( 0 );
    _CRT_term();
    return 1UL;
  }
}
#endif

