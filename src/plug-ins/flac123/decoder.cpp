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


#include "decoder.h"

#include <cpp/container/stringmap.h>
#include <strutils.h>
#include <charset.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <debuglog.h>


const double seek_window = .2;
const double seek_speed  = 4;


DecoderBase::DecoderBase()
: File(NULL)
, Decoder(NULL)
{}

DecoderBase::~DecoderBase()
{ if (Decoder)
    FLAC__stream_decoder_delete(Decoder);
}

bool DecoderBase::Init()
{ Decoder = FLAC__stream_decoder_new();
  return Decoder != NULL;
}

bool DecoderBase::DecoderInitStream(
  FLAC__StreamDecoderWriteCallback    write_callback,
  FLAC__StreamDecoderMetadataCallback metadata_callback,
  FLAC__StreamDecoderErrorCallback    error_callback)
{
  char header[4];
  if (xio_fread(header, sizeof header, 1, File) != 1)
    return false;
  xio_rewind(File);

  FLAC__StreamDecoderInitStatus rc = memcmp(header, "OggS", sizeof header) == 0
    ? FLAC__stream_decoder_init_ogg_stream(Decoder,
      &DecoderBase::ReadCB,
      &DecoderBase::SeekCB,
      &DecoderBase::TellCB,
      &DecoderBase::LengthCB,
      &DecoderBase::EofCB,
      write_callback, metadata_callback, error_callback, this)
    : FLAC__stream_decoder_init_stream(Decoder,
      &DecoderBase::ReadCB,
      &DecoderBase::SeekCB,
      &DecoderBase::TellCB,
      &DecoderBase::LengthCB,
      &DecoderBase::EofCB,
      write_callback, metadata_callback, error_callback, this);
  return rc == FLAC__STREAM_DECODER_INIT_STATUS_OK;
}

FLAC__StreamDecoderReadStatus DecoderBase::ReadCB(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *data)
{ DEBUGLOG(("DecoderBase(%p)::ReadCB(, %p, *%u, %p)\n", buffer, *bytes, data));
  if ((*bytes = xio_fread(buffer, 1, *bytes, ((DecoderBase*)data)->File)) != 0)
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
  return xio_ferror(((DecoderBase*)data)->File)
    ? FLAC__STREAM_DECODER_READ_STATUS_ABORT
    : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}

FLAC__StreamDecoderSeekStatus DecoderBase::SeekCB(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *data)
{ DEBUGLOG(("DecoderBase(%p)::SeekCB(, %Lu)\n", data, absolute_byte_offset));
  if (absolute_byte_offset & ~(FLAC__uint64)0xFFFFFFFFU) // No 64 bit support so far
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
  if (xio_fseek(((DecoderBase*)data)->File, (long)absolute_byte_offset, XIO_SEEK_SET) != -1L)
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
  return xio_can_seek(((DecoderBase*)data)->File)
    ? FLAC__STREAM_DECODER_SEEK_STATUS_ERROR
    : FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
}

FLAC__StreamDecoderTellStatus DecoderBase::TellCB(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *data)
{ long ret = xio_ftell(((DecoderBase*)data)->File);
  if (ret != -1L)
  { *absolute_byte_offset = ret;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
  }
  return xio_can_seek(((DecoderBase*)data)->File)
    ? FLAC__STREAM_DECODER_TELL_STATUS_ERROR
    : FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
}

FLAC__StreamDecoderLengthStatus DecoderBase::LengthCB(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *data)
{ long ret = xio_fsize(((DecoderBase*)data)->File);
  if (ret != -1L)
  { *stream_length = ret;
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
  }
  return xio_ferror(((DecoderBase*)data)->File) == EBADF
    ? FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR
    : FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
}

FLAC__bool DecoderBase::EofCB(const FLAC__StreamDecoder *decoder, void *data)
{ return xio_feof(((DecoderBase*)data)->File);
}


FLAC__StreamDecoderWriteStatus MetaDecoder::WriteCB(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *data)
{ DEBUGLOG(("MetaDecoder(%p)::write_callback(, %p, %p)\n", data, frame, buffer));
  // Dummy to access meta data
  return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

void DecoderBase::ErrorCB(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *data)
{ DEBUGLOG(("DecoderBase(%p)::error_callback(, %i)\n", data, status));
}


bool MetaDecoder::Init()
{ if (!DecoderBase::Init())
    return false;
  FLAC__stream_decoder_set_metadata_respond(Decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
  FLAC__stream_decoder_set_metadata_respond(Decoder, FLAC__METADATA_TYPE_CUESHEET);
  return true;
}

bool MetaDecoder::Analyze(const INFO_BUNDLE* target, XFILE* file, const xstring& url)
{ Target = target;
  File = file;
  URL = url;

  if ( !DecoderInitStream(
    &MetaDecoder::WriteCB,
    &MetaDecoder::MetadataCB,
    &MetaDecoder::ErrorCB) )
    return false;

  Target->tech->attributes = TATTR_NONE;
  if (!FLAC__stream_decoder_process_until_end_of_metadata(Decoder))
    return false;
  return Target->tech->attributes != TATTR_NONE;
}

// vorbis comment lookup table
static const strmap<16,xstring META_INFO::*> VorbisCommentMap[] =
{ { "album=",       &META_INFO::album     }
, { "artist=",      &META_INFO::artist    }
, { "comment=",     &META_INFO::comment   }
, { "copyright=",   &META_INFO::copyright }
, { "date=",        &META_INFO::year      }
, { "genre=",       &META_INFO::genre     }
, { "replaygain_",  NULL } // special marker for ReplayGain
, { "title=",       &META_INFO::title     }
, { "tracknumber=", &META_INFO::track     }
, { "year=",        &META_INFO::year      }
};
// vorbis comment lookup table
static const strmap<12,float META_INFO::*> ReplayGainCommentMap[] =
{ { "album_gain=",  &META_INFO::album_gain }
, { "album_peak=",  &META_INFO::album_peak }
, { "track_gain=",  &META_INFO::track_gain }
, { "track_peak=",  &META_INFO::track_peak }
};


void MetaDecoder::MetadataCB(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *data)
{ DEBUGLOG(("DecoderBase(%p)::metadata_callback(, %p)\n", data, metadata));
  MetaDecoder& that = *(MetaDecoder*)(DecoderBase*)data;
  switch (metadata->type)
  {case FLAC__METADATA_TYPE_STREAMINFO:
    { TECH_INFO& tech = *that.Target->tech;
      tech.samplerate = metadata->data.stream_info.sample_rate;
      tech.channels   = metadata->data.stream_info.channels;
      tech.attributes = TATTR_SONG | TATTR_STORABLE | TATTR_WRITABLE*(xio_protocol(that.File) == XIO_PROTOCOL_FILE);
      char buf[14];
      tech.info.sprintf("%.1f kHz, %s",
                        tech.samplerate/1000.0,
                        tech.channels == 1 ? "mono" : tech.channels == 2 ? "stereo" : (sprintf(buf, "%i ch.", tech.channels), buf));
      OBJ_INFO& obj = *that.Target->obj;
      obj.songlength = (double)metadata->data.stream_info.total_samples / tech.samplerate;
      break;
    }
   case FLAC__METADATA_TYPE_VORBIS_COMMENT:
    { META_INFO& meta = *that.Target->meta;
      FLAC__StreamMetadata_VorbisComment_Entry* vcp = metadata->data.vorbis_comment.comments;
      FLAC__StreamMetadata_VorbisComment_Entry* vcpe = vcp + metadata->data.vorbis_comment.num_comments;
      for (; vcp != vcpe; ++vcp)
      { const strmap<16,xstring META_INFO::*>* mp = mapsearcha(VorbisCommentMap, (char*)vcp->entry);
        if (!mp)
          continue;
        if (mp->Val)
        { // meta string
          size_t len = strlen(mp->Str);
          const char* value = (char*)vcp->entry + len;
          len = vcp->length - len;
          // codepage conversion - TODO: static buffer!
          char buf[1024];
          if (!ch_convert(1208, value, CH_CP_NONE, buf, sizeof buf, 0))
            continue;
          xstring& ts = meta.*(mp->Val);
          if (ts)
          { // Already assigned => append
            xstring nts;
            char* dp = nts.allocate(ts.length() + 1 + len);
            memcpy(dp, ts, ts.length());
            dp += ts.length();
            *dp++ = '\n';
            memcpy(dp, value, len);
            ts.swap(nts);
          } else
            ts.assign(value, len);
        } else
        { // replay gain comment
          static const strmap<12,float META_INFO::*>* rp = mapsearcha(ReplayGainCommentMap, (char*)vcp->entry + 11);
          if (rp)
            meta.*(rp->Val) = atof((char*)vcp->entry + 11 + strlen(rp->Str));
        }
      }
      break;
    }
   case FLAC__METADATA_TYPE_CUESHEET:
    {

    }
   default:; // avoid warnings
  }
}


ThreadDecoder::DECODER_STRUCT()
: State(DECODER_STOPPED)
, Terminate(false)
{}

void ThreadDecoder::Reset()
{ DEBUGLOG(("ThreadDecoder(%p)::Reset()\n", this));
  Terminate = false;
  SeekTo = -1;
  //NewSaveTo = NULL;
  Format.channels = -1;
  Format.samplerate = -1;
  FastMode = DECFAST_NORMAL_PLAY;
  //SaveTo = NULL;
}

PLUGIN_RC ThreadDecoder::Play(const xstring& url, PM123_TIME start)
{ DEBUGLOG(("ThreadDecoder(%p)::Play(%s, %f) - %u\n", this, url.cdata(), start, State));

  if (State != DECODER_STOPPED)
    return PLUGIN_GO_ALREADY;

  URL = url;
  Reset();
  SeekTo = start;

  State = DECODER_STARTING;
  DecoderTID = _beginthread(PROXYFUNCREF(ThreadDecoder)DecoderThreadStub, 0, 65535, this);
  if (DecoderTID == -1)
  { State = DECODER_STOPPED;
    return PLUGIN_ERROR;
  }
  PlayEv.Set();

  return PLUGIN_OK;
}

PLUGIN_RC ThreadDecoder::Stop()
{ DEBUGLOG(("ThreadDecoder(%p)::Stop()\n", this));
  if (State == DECODER_STOPPED)
    return PLUGIN_GO_ALREADY;
  Terminate = true;
  PlayEv.Set();
  return PLUGIN_OK;
}

PLUGIN_RC ThreadDecoder::Seek(PM123_TIME location)
{ DEBUGLOG(("ThreadDecoder(%p)::Seek(%f) - %f\n", this, location, SeekTo));
  SeekTo = location;
  PlayEv.Set();
  return PLUGIN_OK;
}

PLUGIN_RC ThreadDecoder::SetFast(DECFASTMODE mode)
{ DEBUGLOG(("ThreadDecoder(%p)::SetFast(%u) - %f\n", this, mode, FastMode));
  FastMode = mode;
  NextSkip = 0;
  return PLUGIN_OK;
}

/*PLUGIN_RC ThreadDecoder::SaveStream(const xstring& target)
{ DEBUGLOG(("ThreadDecoder(%p)::SaveStream(%s) - %p, %p\n", this, target.cdata(), SaveTo, NewSaveTo));

  if (target)
  {
    NewSaveTo = xio_fopen(target, "ab");
    if (!file)
      return
  } else
    NewSaveTo = NULL;

}*/

PM123_TIME ThreadDecoder::GetLength() const
{ if (!Decoder || Format.samplerate <= 0)
    return -1;
  return (PM123_TIME)FLAC__stream_decoder_get_total_samples(Decoder) / Format.samplerate;
}

PROXYFUNCIMP(void, ThreadDecoder) TFNENTRY DecoderThreadStub(void* arg)
{ ((ThreadDecoder*)arg)->DecoderThread();
}

void ThreadDecoder::DecoderThread()
{ DEBUGLOG(("ThreadDecoder(%p{%s})::DecoderThread()\n", this, URL.cdata()));
  // open source
  File = xio_fopen(URL, "rbUX");
  if (File == NULL)
  { xstring errortext;
    errortext.sprintf("Unable to open %s\n%s", URL.cdata(), xio_strerror(xio_errno()));
    (*DecEvent)(A, DECEVENT_PLAYERROR, (void*)errortext.cdata());
    goto end;
  }

  if (!DecoderInitStream(&ThreadDecoder::WriteCB, &ThreadDecoder::MetadataCB, &ThreadDecoder::ErrorCB))
  { xstring errortext;
    errortext.sprintf("Unable to open %s\nUnsupported file format", URL.cdata());
    (*DecEvent)(A, DECEVENT_PLAYERROR, (void*)errortext.cdata());
    goto end;
  }

  if (!FLAC__stream_decoder_process_until_end_of_metadata(Decoder))
  { xstring errortext;
    errortext.sprintf("Unable to decode %s\nState %s", URL.cdata(), FLAC__stream_decoder_get_resolved_state_string(Decoder));
    (*DecEvent)(A, DECEVENT_PLAYERROR, (void*)errortext.cdata());
    goto end;
  }

  DEBUGLOG(("ThreadDecoder::DecoderThread before main loop - %f\n", SeekTo));
  // After opening a new file we are at the beginning anyway.
  if (SeekTo == 0)
    SeekTo = -1;

  for(;;)
  { PlayEv.Wait();

    if (Terminate)
      goto end;

    State = DECODER_PLAYING;
    for (;;)
    { DEBUGLOG(("ThreadDecoder(%p)::DecoderThread decode loop - %f\n", this, SeekTo));
      PlayEv.Reset();

      PM123_TIME newpos = SeekTo;
      PM123_TIME pos = GetTime();
      if (FastMode && pos >= NextSkip)
      { if (newpos < 0)
          newpos = pos;
        newpos += FastMode == DECFAST_FORWARD ? seek_window*(-1+seek_speed) : seek_window*(-1-seek_speed);
        if (newpos < 0)
          break; // Begin of song
      }
      if (newpos >= 0)
      { NextSkip = newpos + seek_window;
        bool rc;
        { //Mutex::Lock lock(Mtx);
          rc = FLAC__stream_decoder_seek_absolute(Decoder, (FLAC__uint64)(newpos * Format.samplerate));
          DEBUGLOG(("ThreadDecoder::DecoderThread seek to %f : %u\n", newpos, rc));
          newpos = SeekTo;
          SeekTo = -1;
        }
        if (newpos >= 0)
          (*DecEvent)(A, DECEVENT_SEEKSTOP, NULL);
        if (!rc)
          break; // Seek out of range => begin/end of song
      }

      bool more = FLAC__stream_decoder_process_single(Decoder);
      if (Terminate)
        goto end;
      if (!more)
        goto fail;
      // The return value of FLAC__stream_decoder_process_single is not reliable, so check stream state always.
      switch (FLAC__stream_decoder_get_state(Decoder))
      {default:
       fail:
        DEBUGLOG(("ThreadDecoder::DecoderThread fail: %s\n", FLAC__stream_decoder_get_resolved_state_string(Decoder)));
        { xstring errortext;
          errortext.sprintf("Unable to decode %s\nState %s", URL.cdata(), FLAC__stream_decoder_get_resolved_state_string(Decoder));
          (*DecEvent)(A, DECEVENT_PLAYERROR, (void*)errortext.cdata());
          goto end;
        }
       case FLAC__STREAM_DECODER_ABORTED:
        DEBUGLOG(("ThreadDecoder::DecoderThread abort:\n"));
        (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
        goto end;
       case FLAC__STREAM_DECODER_END_OF_STREAM:
        goto eos;
       case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
       case FLAC__STREAM_DECODER_READ_METADATA:
       case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
       case FLAC__STREAM_DECODER_READ_FRAME:;
      }

      if (Terminate)
        goto end;
    }
   eos:
    DEBUGLOG(("ThreadDecoder(%p)::DecoderThread end of stream - %.3f\n", this, GetTime()));
    if (Terminate)
      goto end;
    (*DecEvent)(A, DECEVENT_PLAYSTOP, NULL);
  }

 end:
  State = DECODER_STOPPED;
  Terminate = false;
  DecoderTID = -1;
  DEBUGLOG(("ThreadDecoder(%p)::DecoderThread terminate\n", this));
}

FLAC__StreamDecoderWriteStatus ThreadDecoder::WriteCB(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{ DEBUGLOG(("ThreadDecoder(%p)::WriteCB(%p, {%u, %u, %u,, %Lu, %u}, %p)\n", client_data,
    decoder, frame->header.blocksize, frame->header.channels, frame->header.sample_rate, frame->header.bits_per_sample, frame->header.number.sample_number, buffer));
  #define this ((ThreadDecoder*)client_data)

  this->Format.channels = frame->header.channels;
  this->Format.samplerate = frame->header.sample_rate;
  this->SamplePos = frame->header.number.sample_number;
  int exponent = 1 - frame->header.bits_per_sample;
  unsigned done = 0;

  while (done < frame->header.blocksize)
  { float* target;
    unsigned count = (*this->OutRequestBuffer)(this->A, &this->Format, &target);
    if ((int)count <= 0)
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    if (count > frame->header.blocksize - done)
      count = frame->header.blocksize - done;

    // Convert samples
    unsigned channels = frame->header.channels;
    switch (channels)
    {case 1: // mono
      { const FLAC__int32* sp = buffer[0] + done;
        float* dp = target;
        unsigned i;
        for (i = count / 8; i; --i)
        { DO_8(p, dp[p] = (float)ldexp(sp[p], exponent));
          dp += 8;
          sp += 8;
        }
        for (i = count % 8; i; --i)
          *dp++ = (float)ldexp(*sp++, exponent);
        break;
      }
     case 2: // stereo
     { const FLAC__int32* sp1 = buffer[0] + done;
       const FLAC__int32* sp2 = buffer[1] + done;
       float* dp = target;
       unsigned i;
       for (i = count / 8; i; --i)
       { DO_8(p,
           dp[2*p] = (float)ldexp(sp1[p], exponent);
           dp[2*p+1] = (float)ldexp(sp2[p], exponent));
         dp += 16;
         sp1 += 8;
         sp2 += 8;
       }
       for (i = count % 8; i; --i)
       { *dp++ = (float)ldexp(*sp1++, exponent);
         *dp++ = (float)ldexp(*sp2++, exponent);
       }
       break;
     }
     default: // multi
      for (unsigned ch = 0; ch < channels; ++ch)
      { const FLAC__int32* sp = buffer[ch] + done;
        float* dp = target + ch;
        unsigned i;
        for (i = count / 8; i; --i)
        { DO_8(p, dp[p*channels] = (float)ldexp(sp[p], exponent));
          dp += 8 * channels;
          sp += 8;
        }
        for (i = count % 8; i; --i)
        { *dp = (float)ldexp(*sp++, exponent);
          dp += channels;
        }
      }
    }

    (*this->OutCommitBuffer)(this->A, count, this->GetTime());
    this->SamplePos += count;
    done += count;
  }

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
  #undef this
}

void ThreadDecoder::MetadataCB(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{ DEBUGLOG(("ThreadDecoder(%p)::MetadataCB(%p, %p{%u})\n", client_data, decoder, metadata, metadata->type));
  #define this ((ThreadDecoder*)client_data)
  if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
  { this->Format.channels = metadata->data.stream_info.channels;
    this->Format.samplerate = metadata->data.stream_info.sample_rate;
  }
  #undef this
}
