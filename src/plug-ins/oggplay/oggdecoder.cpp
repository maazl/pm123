/*
 * Copyright 2009-2012 Marcel Mueller
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
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


#include "oggdecoder.h"

#include <cpp/interlockedptr.h>
#include <strutils.h>
#include <charset.h>
#include <eautils.h>
#include <utilfct.h>
#include <process.h>
#include <fileutil.h>


const double seek_window = .2;
const double seek_speed  = 4;

/* Changes the current file position to a new location within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
static int DLLENTRY vio_seek(void* w, ogg_int64_t offset, int whence)
{ DEBUGLOG2(("oggplay:vio_seek(%p, %li, %i)\n", w, offset, whence));

  XFILE* x   = (XFILE*)w;
  int    pos = 0;

  if( xio_can_seek(x) < XIO_CAN_SEEK_FAST )
    return -1;

  switch( whence )
  { case SEEK_SET: pos = offset; break;
    case SEEK_CUR: pos = xio_ftell(x) + offset; break;
    case SEEK_END: pos = xio_fsize(x) + offset; break;
    default:
      return -1;
  }

  if( xio_fseek( x, pos, XIO_SEEK_SET ) >= 0 ) {
    return pos;
  } else {
    return -1;
  }
}

/* Finds the current position of the file. Returns the current file
   position. On error, returns -1L and sets errno to a nonzero value. */
static long DLLENTRY vio_tell(void* w)
{ return xio_ftell((XFILE*)w);
}

/* Reads up to count items of size length from the input file and
   stores them in the given buffer. The position in the file increases
   by the number of bytes read. Returns the number of full items
   successfully read, which can be less than count if an error occurs
   or if the end-of-file is met before reaching count. */
static size_t DLLENTRY vio_read(void* ptr, size_t size, size_t count, void* w)
{ DEBUGLOG2(("oggplay:vio_read(%p, %i, %i, %p)\n", ptr, size, count, w));
  return xio_fread( ptr, size, count, (XFILE*)w);
}


OggDecoder::OggDecoder()
: File(NULL)
, VrbInfo(NULL)
, Comment(NULL)
, Songlength(-1)
, Bitrate(-1)
, PlayedSecs(-1)
{ memset(&VrbFile, 0, sizeof VrbFile);
}

OggDecoder::~OggDecoder()
{ OggClose();
}

PROXYFUNCIMP(void DLLENTRY, OggDecoder) Observer(XIO_META type, const char* metabuff, long pos, void* arg)
{
  // TODO:
}

int OggDecoder::OggOpen()
{ DEBUGLOG(("OggDecoder(%p)::OggOpen\n", this));

  ov_callbacks callbacks;
  callbacks.read_func  = vio_read;
  callbacks.seek_func  = vio_seek;
  callbacks.tell_func  = vio_tell;
  callbacks.close_func = NULL;
  // The ov_open_callbaks() function performs full stream detection and machine
  // initialization. If it returns 0, the stream *is* Vorbis and we're
  // fully ready to decode.
  if (ov_open_callbacks(File, &VrbFile, NULL, 0, callbacks) < 0)
    return -1;

  if ((VrbInfo = ov_info(&VrbFile, -1)) == NULL)
  { ov_clear(&VrbFile);
    return -2;
  }

  Songlength  = ov_time_total(&VrbFile, -1);
  Comment     = ov_comment(&VrbFile, -1);
  Bitrate     = ov_bitrate(&VrbFile, -1);
  PlayedSecs  = 0;

  /*OutputFormat.channels   = VrbInfo->channels;
  OutputFormat.samplerate = VrbInfo->rate;*/

  xio_set_metacallback(File, OggDecoder::Observer, this);
  return 0;
}

/* Reads up to count pcm bytes from the Ogg Vorbis stream and
   stores them in the given buffer. */
int OggDecoder::OggRead(float* buffer, int count)
{ DEBUGLOG(("OggDecoder(%p)::OggRead(%p, %i) - %.3f\n", this, buffer, count, PlayedSecs));
  int section = -1;
  int resync  =  0;
 retry:
  float** source;
  long done = ov_read_float(&VrbFile, &source, count, &section);
  if (done == OV_HOLE && resync++ < MAXRESYNC)
    goto retry;
  if (done > 0)
  { // multiplex samples into a single stream
    switch (VrbInfo->channels)
    {case 1:
      memcpy(buffer, source[0], done * sizeof(float));
      break;
     case 2:
      { float* l = source[0];
        float* r = source[1];
        long i = 0;
        while (i < (done & 7))
        { buffer[0] = l[i];
          buffer[1] = r[i];
          buffer += 2;
          ++i;
        }
        while (i < done)
        { DO_8(p,
            buffer[0+2*p] = l[i];
            buffer[1+2*p] = r[i] );
          buffer += 2*8;
          i += 8;
        }
        break;
      }
     default:
      { float** ptrs = (float**)alloca(VrbInfo->channels * sizeof(float*));
        memcpy(ptrs, source, VrbInfo->channels * sizeof(float*));
        for (long i = 0; i < done; ++i)
        { int j = VrbInfo->channels;
          do
          { *buffer++ = *(ptrs[j])++;
          } while (--j);
        }
      }
    }
  }

  PlayedSecs = ov_time_tell(&VrbFile);
  DEBUGLOG(("OggDecoder::OggRead: %i - %.3f\n", done, PlayedSecs));
  return done;
}

/* Changes the current file position to a new time within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
int OggDecoder::OggSeek(double secs)
{ DEBUGLOG(("OggDecoder(%p)::OggSeek(%.3f) - %.3f\n", this, secs, PlayedSecs));
  int rc = ov_time_seek(&VrbFile, secs);
  DEBUGLOG(("OggDecoder::OggSeek: %i\n", rc));
  if (rc == 0)
    PlayedSecs = secs;
  return rc;
}

xstring OggDecoder::VorbisGetString(const char* type) const
{ const char* string;
  if (!Comment || (string = vorbis_comment_query(Comment, type, 0)) == NULL)
    return xstring();
  // TODO: static buffer!
  char buf[1024];
  return ch_convert(1208, string, CH_CP_NONE, buf, sizeof buf, 0);
}

/* Sets a specified field of the given Ogg Vorbis comment.
void OggDecoder::VorbisSetString(const char* type, const char* value)
{
  char string[1024];
  vorbis_comment_clear_tag(Comment, type);

  if (!*value) // Do not create empty comment tags
    return;

  ch_convert(CH_CP_NONE, value, 1208, string, sizeof string, 0);
  vorbis_comment_add_tag(Comment, type, string);
}*/


vector<OggDecoderThread> OggDecoderThread::Instances;
Mutex OggDecoderThread::InstMtx;

OggDecoderThread::DECODER_STRUCT()
: DecoderTID(-1)
, Status(DECODER_STOPPED)
, JumpToPos(-1)
, FastMode(DECFAST_NORMAL_PLAY)
{
  Mutex::Lock lock(InstMtx);
  Instances.append() = this;
}

OggDecoderThread::~DECODER_STRUCT()
{
  DecoderCommand(DECODER_STOP, NULL);

  { Mutex::Lock lock(InstMtx);
    foreach(OggDecoderThread*const*, dpp, Instances)
    { if (*dpp == this)
      { Instances.erase(dpp);
        break;
      }
    }
  }

  // Wait for decoder to complete
  int tid = DecoderTID;
  if (tid != -1)
    wait_thread(tid, 5000);
}

/* Decoding thread. */
void OggDecoderThread::DecoderThread()
{
  int rc;
  /*char  last_title[512] = "";
  char  curr_title[512];*/

  // Open stream
  { Mutex::Lock lock(Mtx);
    if ((File = xio_fopen(URL, "rbXU")) == NULL)
    { xstring errortext;
      errortext.sprintf("Unable to open file %s\n%s", URL.cdata(), xio_strerror(xio_errno()));
      Ctx.plugin_api->message_display(MSG_ERROR, errortext);
      (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
      goto end;
    }

    if ((rc = OggOpen()) != 0)
    { xstring errortext;
      errortext.sprintf("Unable to open file %s\nUnsupported file format", URL.cdata());
      Ctx.plugin_api->message_display(MSG_ERROR, errortext);
      (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
      goto end;
    }
  }

  // After opening a new file we so are in its beginning.
  if (JumpToPos == 0)
    JumpToPos = -1;

  FORMAT_INFO2 output_format;
  output_format.channels = GetChannels();
  output_format.samplerate = GetSamplerate();

  for(;;)
  { Play.Wait();
    Play.Reset();

    if (StopRq)
      goto end;

    Status = DECODER_PLAYING;
    for (;;)
    { double newpos = JumpToPos;
      if (FastMode && GetPos() >= NextSkip)
      { if (newpos < 0)
          newpos = GetPos();
        newpos += FastMode == DECFAST_FORWARD ? seek_window*(-1+seek_speed) : seek_window*(-1-seek_speed);
        if (newpos < 0)
          break; // Begin of song
      }
      if (newpos >= 0)
      { NextSkip = newpos + seek_window;
        int rc;
        { Mutex::Lock lock(Mtx);
          rc = OggSeek(newpos);
          newpos = JumpToPos;
          JumpToPos = -1;
        }
        if (newpos >= 0)
          (*DecEvent)(A, DECEVENT_SEEKSTOP, NULL);
        if (rc != 0)
          break; // Seek out of range => begin/end of song
      }

      float* target;
      int count = (*OutRequestBuffer)(A, &output_format, &target);
      if (count <= 0)
      { (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
        goto end;
      }

      int read;
      { Mutex::Lock lock(Mtx);
        read = OggRead(target, count);
      }
      if (read <= 0)
        break; // End of song

      (*OutCommitBuffer)(A, read, GetPos() - (double)read / GetSamplerate());

      /* Don't work anyway
      if( w->metadata_buff && w->metadata_size && w->hwnd )
      {
        ogg_get_string( w, curr_title, "TITLE", sizeof( curr_title ));

        if( strcmp( curr_title, last_title ) != 0 ) {
          strlcpy ( last_title, curr_title, sizeof( last_title ));
          snprintf( w->metadata_buff, w->metadata_size, "StreamTitle='%s';", last_title );
          WinPostMsg( w->hwnd, WM_METADATA, MPFROMP( w->metadata_buff ), 0 );
        }
      }*/
      if (StopRq)
        goto end;
    }
    if (StopRq)
      goto end;
    DEBUGLOG(("OggDecoderThread::DecoderThread - playstop event - %.3f, %i\n", GetPos(), StopRq));
    (*DecEvent)(A, DECEVENT_PLAYSTOP, NULL);
  }

end:
  ov_clear(&VrbFile);
  XFILE* file = InterlockedXch(&File, (XFILE*)NULL);
  if (file)
    xio_fclose(file);
  Status = DECODER_STOPPED;
  DecoderTID = -1;
}

PROXYFUNCIMP(void, OggDecoderThread) TFNENTRY DecoderThreadStub(void* arg)
{ ((OggDecoderThread*)arg)->DecoderThread();
}

ULONG OggDecoderThread::DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params)
{ switch (msg)
  {
    case DECODER_SETUP:
      OutRequestBuffer = params->OutRequestBuffer;
      OutCommitBuffer  = params->OutCommitBuffer;
      DecEvent         = params->DecEvent;
      A                = params->A;
      break;

    case DECODER_PLAY:
    {
      if (DecoderTID != -1)
      { if (Status == DECODER_STOPPED)
          DecoderCommand(DECODER_STOP, NULL);
        else
          return PLUGIN_GO_ALREADY;
      }

      URL = params->URL;

      JumpToPos = params->JumpTo;
      Status    = DECODER_STARTING;
      StopRq    = false;
      DecoderTID = _beginthread(PROXYFUNCREF(OggDecoderThread)DecoderThreadStub, 0, 65535, this);
      if (DecoderTID == -1)
      { Status = DECODER_STOPPED;
        return PLUGIN_FAILED;
      }
      Play.Set(); // Go!
      break;
    }

    case DECODER_STOP:
    {
      if (DecoderTID == -1)
        return PLUGIN_GO_ALREADY;

      Status = DECODER_STOPPING;
      StopRq = true;

      if (File) xio_fabort(File);

      Play.Set();
      /*w->status = DECODER_STOPPED;
      w->decodertid = -1;*/
      break;
    }

    case DECODER_FFWD:
      /*if (params->Fast && File && xio_can_seek(File) != XIO_CAN_SEEK_FAST)
        return PLUGIN_UNSUPPORTED;*/
      FastMode = params->Fast;
      NextSkip = GetPos();
      Play.Set();
      break;

    case DECODER_JUMPTO:
      JumpToPos = params->JumpTo;
      NextSkip = JumpToPos;
      Play.Set();
      break;

    default:
      return PLUGIN_UNSUPPORTED;
   }

   return PLUGIN_OK;
}

int OggDecoderThread::ReplaceStream(const char* sourcename, const char* destname)
{
  // Suspend all active instances of the updated file.
  Mutex::Lock lock(InstMtx);

  foreach(OggDecoderThread*const*, dpp, Instances)
  { OggDecoderThread& dec = **dpp;
    dec.Mtx.Request();

    if (dec.File && stricmp(dec.URL, destname) == 0)
    { DEBUGLOG(("oggplay: suspend currently used file: %s\n", destname));
      dec.ResumePcms = ov_pcm_tell(&dec.VrbFile);
      dec.OggClose();
      xio_fclose(dec.File);
    } else
      dec.ResumePcms = -1;
  }

  const char* srcfile = surl2file(sourcename);
  const char* dstfile = surl2file(destname);
  // Preserve EAs.
  eacopy(dstfile, srcfile);

  // Replace file.
  int rc = 0;
  if (remove(dstfile) != 0 || rename(srcfile, dstfile) != 0)
    rc = errno;

  // Resume all suspended instances of the updated file.
  foreach(OggDecoderThread*const*, dpp, Instances)
  { OggDecoderThread& dec = **dpp;
    if (dec.ResumePcms != -1)
    { DEBUGLOG(("oggplay: resumes currently used file: %s\n", destname));
      dec.File = xio_fopen(destname, "rbXU");
      if (dec.File && dec.OggOpen() == PLUGIN_OK)
        ov_pcm_seek(&dec.VrbFile, dec.ResumePcms);
    }
    dec.Mtx.Release();
  }

  return rc;
}
