/*
 * Copyright 2012 Marcel Mueller
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
#define  INCL_ERRORS

#include <decoder_plug.h>
#include <plugin.h>
#include <utilfct.h>
#include <fileutil.h>
#include <charset.h>
#include <debuglog.h>

#include "decoder.h"

#include <os2.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


PLUGIN_CONTEXT Ctx = {0};


/* Init function is called when PM123 needs the specified decoder to play
   the stream demanded by the user. */
int DLLENTRY decoder_init(struct DECODER_STRUCT** returnw)
{ ThreadDecoder* dec = new ThreadDecoder;
  if (!dec->Init())
  { delete dec;
    return PLUGIN_FAILED;
  }
  *returnw = dec;
  return PLUGIN_OK;
}

/* Uninit function is called when another decoder than this is needed. */
BOOL DLLENTRY decoder_uninit(struct DECODER_STRUCT* arg)
{ delete arg;
  return TRUE;
}

/* There is a lot of commands to implement for this function. Parameters
   needed for each of the are described in the definition of the structure
   in the decoder_plug.h file. */
ULONG DLLENTRY decoder_command(struct DECODER_STRUCT* w, DECMSGTYPE msg, const DECODER_PARAMS2* params)
{ DEBUGLOG(("flac123:decoder_command(%p, %u, )\n", w, msg));

  switch (msg)
  {case DECODER_SETUP:
    w->Setup(params->A, params->OutRequestBuffer, params->OutCommitBuffer, params->DecEvent);
    return PLUGIN_OK;

   case DECODER_PLAY:
    return w->Play(params->URL, params->JumpTo);

   case DECODER_STOP:
    return w->Stop();

   case DECODER_FFWD:
    return w->SetFast(params->Fast);

   case DECODER_JUMPTO:
    return w->Seek(params->JumpTo);

   /*case DECODER_SAVEDATA:
    return w->SaveStream(params->SaveFilename);*/

   default:
    return PLUGIN_UNSUPPORTED;
  }
}

/* currently not required
void DLLENTRY decoder_event(void* w, OUTEVENTTYPE event)
{
}*/

/* Returns current status of the decoder. */
ULONG DLLENTRY decoder_status(struct DECODER_STRUCT* arg)
{ return arg->GetState();
}

/* Returns number of milliseconds the stream lasts. */
PM123_TIME DLLENTRY decoder_length(struct DECODER_STRUCT* arg)
{ return arg->GetLength();
}

/* Returns information about specified file. */
ULONG DLLENTRY decoder_fileinfo(const char* url, struct _XFILE* handle, int* what, const INFO_BUNDLE* info,
                                DECODER_INFO_ENUMERATION_CB cb, void* param)
{
  if (handle == NULL)
    return PLUGIN_NO_PLAY;

  MetaDecoder dec;
  if (!dec.Init())
    return PLUGIN_ERROR;
  if (!dec.Analyze(info, handle, url))
    return PLUGIN_NO_PLAY;

  *what |= INFO_TECH|INFO_OBJ|INFO_META|INFO_ATTR;
  return PLUGIN_OK;
}

/*static size_t
vce_read( void* ptr, size_t size, size_t count, void* file ) {
  return xio_fread( ptr, size, count, (XFILE*)file );
}
static size_t
vce_write( void* ptr, size_t size, size_t count, void* file ) {
  return xio_fwrite( ptr, size, count, (XFILE*)file );
}

static void
ogg_set_string( vorbis_comment* comment, const char* source, char* type )
{
  char string[128*4];

  vorbis_comment_clear_tag(comment, type);

  if (!source || !*source) // Do not create empty comment tags
    return;

  ch_convert( CH_CP_NONE, source, 1208, string, sizeof( string ), 0 );
  vorbis_comment_add_tag( comment, type, string );
}*/

ULONG DLLENTRY decoder_saveinfo(const char* url, const META_INFO* info, int haveinfo, xstring* errortext)
{ DEBUGLOG(("oggplay:decoder_saveinfo(%s, )\n", url));

  return PLUGIN_ERROR;
/*
  XFILE* file = NULL;
  XFILE* save = NULL;
  vcedit_state* state = NULL;
  int    rc = PLUGIN_OK;
  xstring savename;

  { size_t len = strlen(url);
    char* cp = savename.allocate(len);
    memcpy(cp, url, len);
    cp[len-1] = '~';
  }

  do
  {
    if(( file = xio_fopen( url, "rbU" )) == NULL ) {
      rc = xio_errno();
      break;
    }
    if(( save = xio_fopen( savename, "wbU" )) == NULL ) {
      rc = xio_errno();
      break;
    }
    if(( state = vcedit_new_state()) == NULL ) {
      rc = errno;
      break;
    }

    if( vcedit_open_callbacks( state, file, vce_read, vce_write ) < 0 )
    { rc = EINVAL;
      *errortext = "File failed to open as Vorbis.";
      break;
    }

    vorbis_comment* vc = vcedit_comments( state );

    if (haveinfo & DECODER_HAVE_ARTIST)
      ogg_set_string( vc, info->artist,    "ARTIST"      );
    if (haveinfo & DECODER_HAVE_ALBUM)
      ogg_set_string( vc, info->album,     "ALBUM"       );
    if (haveinfo & DECODER_HAVE_TITLE)
      ogg_set_string( vc, info->title,     "TITLE"       );
    if (haveinfo & DECODER_HAVE_GENRE)
      ogg_set_string( vc, info->genre,     "GENRE"       );
    if (haveinfo & DECODER_HAVE_YEAR)
      ogg_set_string( vc, info->year,      "DATE"        );
    if (haveinfo & DECODER_HAVE_TRACK)
      ogg_set_string( vc, info->track,     "TRACKNUMBER" );
    if (haveinfo & DECODER_HAVE_COPYRIGHT)
      ogg_set_string( vc, info->copyright, "COPYRIGHT"   );
    if (haveinfo & DECODER_HAVE_COMMENT)
      ogg_set_string( vc, info->comment,   "COMMENT"     );

    char   buffer[64];
    if (haveinfo & DECODER_HAVE_ALBUM_GAIN)
    { sprintf( buffer, "%.2f dB", info->album_gain );
      ogg_set_string( vc, buffer, "replaygain_album_gain" );
    }
    if (haveinfo & DECODER_HAVE_ALBUM_PEAK)
    { sprintf( buffer, "%.6f", info->album_peak );
      ogg_set_string( vc, buffer, "replaygain_album_peak" );
    }
    if (haveinfo & DECODER_HAVE_TRACK_GAIN)
    { sprintf( buffer, "%.2f dB", info->track_gain );
      ogg_set_string( vc, buffer, "replaygain_track_gain" );
    }
    if (haveinfo & DECODER_HAVE_TRACK_PEAK)
    { sprintf( buffer, "%.6f", info->track_peak );
      ogg_set_string( vc, buffer, "replaygain_track_peak" );
    }

    if( vcedit_write( state, save ) < 0 )
      rc = errno;
  } while (0);

  if( state )
    vcedit_clear( state );
  if( file  )
    xio_fclose( file );
  if( save  )
    xio_fclose( save );

  if (rc == PLUGIN_OK)
    rc = OggDecoderThread::ReplaceStream(savename, url);
  if (rc)
  { if (!*errortext)
      *errortext = xio_strerror(rc);
    remove(savename);
  }
  return rc;*/
}


static const DECODER_FILETYPE filetypes[] =
{ { "Digital Audio", "FLAC", "*.flac", DECODER_FILENAME|DECODER_URL|DECODER_SONG|DECODER_METAINFO }
};

ULONG DLLENTRY decoder_support(const DECODER_FILETYPE** types, int* count)
{ *types = filetypes;
  *count = sizeof filetypes / sizeof *filetypes;
  return DECODER_FILENAME|DECODER_URL|DECODER_SONG|DECODER_METAINFO;
}


/* Returns information about plug-in. */
int DLLENTRY plugin_query(PLUGIN_QUERYPARAM* param)
{
  param->type         = PLUGIN_DECODER;
  param->author       = "Marcel Mueller";
  param->desc         = "FLAC123 1.0";
  param->configurable = FALSE;
  param->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{ Ctx = *ctx;
  return PLUGIN_OK;
}
