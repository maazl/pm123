/*
 * Copyright 2020 Marcel Mueller
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

#define USE_TAGGING

#include "mp4decoder.h"

#include <cpp/interlockedptr.h>
#include <strutils.h>
#include <charset.h>
#include <eautils.h>
#include <utilfct.h>
#include <process.h>
#include <fileutil.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

#include <neaacdec.h>

/// Length of the chunks played during scan of a file.
const double seek_window = .2;


const char* Mp4Decoder::Sniffer(struct _XFILE* stream)
{ // https://mimesniff.spec.whatwg.org/#signature-for-mp4
  unsigned char buffer[16];
  if (xio_fread(buffer, sizeof(buffer), 1, stream) != 1)
    return NULL;
  if (buffer[0] & 3)
    return NULL;
  uint32_t box = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[0];
  int64_t size = xio_fsize(stream);
  if (size != -1 && size < box)
    return NULL;
  if (*(uint32_t*)(buffer + 4) != 0x70797466)
    return NULL;
  if (buffer[8] == 'm' && buffer[9] == 'p' && buffer[10] == '4')
    return "mp4";
  if (*(uint32_t*)(buffer + 8) != 0x41344d20)
    return "M4A";
  box -= 12;
  while (box -= 4)
  { if (xio_fread(buffer, 4, 1, stream) != 1)
      return NULL;
    if (buffer[0] == 'm' && buffer[1] == 'p' && buffer[2] == '4')
      return "mp4";
  }
  return NULL;
}


static uint32_t vio_read(void* w, void* ptr, uint32_t size)
{ DEBUGLOG2(("mp4play:vio_read(%p, %p, %i)\n", w, ptr, size));
  return xio_fread(ptr, 1, size, (XFILE*)w);
}

static uint32_t vio_write(void* w, void* ptr, uint32_t size)
{ DEBUGLOG2(("mp4play:vio_write(%p, %p, %i)\n", w, ptr, size));
  return xio_fwrite((const void*)ptr, 1, size, (XFILE*)w);
}

static uint32_t vio_seek(void* w, uint64_t offset)
{ DEBUGLOG2(("mp4play:vio_seek(%p, %llu)\n", w, offset));
  if (xio_fseekl((XFILE*)w, offset, XIO_SEEK_SET) >= 0)
    return 0;
  else
    return ~0;
}

static uint32_t vio_truncate(void *w)
{ DEBUGLOG2(("mp4play:vio_truncate(%p)\n", w));
  int64_t pos = xio_ftelll((XFILE*)w);
  if (pos == -1)
    return ~0;
  return xio_ftruncatel((XFILE*)w, pos);
}

Mp4Decoder::Mp4Decoder()
: MP4stream(NULL)
{ Callbacks.read = &vio_read;
  Callbacks.write = &vio_write;
  Callbacks.seek = &vio_seek;
  Callbacks.truncate = &vio_truncate;
  Callbacks.user_data = NULL;
}

Mp4Decoder::~Mp4Decoder()
{ Close();
}

PROXYFUNCIMP(void DLLENTRY, Mp4Decoder) Observer(XIO_META type, const char* metabuff, long pos, void* arg)
{
  // TODO:
}

int Mp4Decoder::Init(mp4ff_t* mp4)
{ DEBUGLOG(("Mp4Decoder(%p)::Init(%p)\n", this, mp4));

  MP4stream = mp4;
  if (MP4stream == NULL)
    return -1;

  int num_tracks = mp4ff_total_tracks(MP4stream);
  for (Track = 0; ; ++Track)
  { if (Track >= num_tracks)
      return -1; // no audio track
    if (mp4ff_get_track_type(MP4stream, Track) == 1) // audio track?
      break;
  }

  SongLength = -1;
  SongOffset = 0;
  FileTime = mp4ff_get_track_duration_use_offsets(MP4stream, Track);
  TimeScale = mp4ff_time_scale(MP4stream, Track);
  if (TimeScale < 0)
    return -1;
  if (FileTime != -1)
    SongLength = (double)FileTime / TimeScale;

  // honor edit list for gapless playback
  const mp4ff_elst_t* edit_list;
  int entries = mp4ff_get_edit_list(MP4stream, Track, &edit_list);
  if (entries && edit_list->media_time) // some M4A contain trash edit list entries => ignore them
  { SongOffset = (double)edit_list->media_time / TimeScale;
    SongLength = edit_list->track_duration;
    while (--entries)
      SongLength += (++edit_list)->track_duration;
    SongLength /= TimeScale;
  }

  return 0;
}

int Mp4Decoder::Open(XFILE* file)
{ SetFile(file);
  Samplerate = -1;
  Channels = -1;
  int r = Init(mp4ff_open_read_metaonly(&Callbacks));
  if (r == 0)
  { unsigned char* decoder_info;
    unsigned decoder_info_size;
    if (mp4ff_get_decoder_config(MP4stream, Track, &decoder_info, &decoder_info_size))
      return -1;
    mp4AudioSpecificConfig mp4ASC;
    if (!NeAACDecAudioSpecificConfig(decoder_info, decoder_info_size, &mp4ASC))
    { Samplerate = (int)mp4ASC.samplingFrequency;
      Channels = mp4ASC.channelsConfiguration ? mp4ASC.channelsConfiguration : mp4ff_get_channel_count(MP4stream, Track);
    } else
      r = -1;
    free(decoder_info);
  }
  return r;
}


xstring Mp4Decoder::ConvertString(const char* tag)
{
  if (!tag || !*tag)
    return xstring();
  char buffer[1024];
  return ch_convert(1208, tag, CH_CP_NONE, buffer, sizeof buffer, 0);
}

void Mp4Decoder::GetMeta(META_INFO& meta)
{
  for (const mp4ff_tag_t *tag, *tage = tag + mp4ff_meta_get(MP4stream, &tag); tag != tage; ++tag)
  { if (!stricmp(tag->item, "title"))
      meta.title = ConvertString(tag->value);
    else if(!stricmp(tag->item, "artist"))
      meta.artist = ConvertString(tag->value);
    else if(!stricmp(tag->item, "album"))
      meta.album = ConvertString(tag->value);
    else if(!stricmp(tag->item, "date"))
      meta.year = ConvertString(tag->value);
    else if(!stricmp(tag->item, "comment"))
      meta.comment = ConvertString(tag->value);
    else if(!stricmp(tag->item, "genre"))
      meta.genre = ConvertString(tag->value);
    else if(!stricmp(tag->item, "track"))
      meta.track = ConvertString(tag->value);
    else if(!stricmp(tag->item, "replaygain_track_gain"))
      sscanf(tag->value, "%f", &meta.track_gain);
    else if(!stricmp(tag->item, "replaygain_track_peak"))
      sscanf(tag->value, "%f", &meta.track_peak);
    else if(!stricmp(tag->item, "replaygain_album_gain"))
      sscanf(tag->value, "%f", &meta.album_gain);
    else if(!stricmp(tag->item, "replaygain_album_peak"))
      sscanf(tag->value, "%f", &meta.album_peak);
  }
}


vector<Mp4DecoderThread> Mp4DecoderThread::Instances;
Mutex Mp4DecoderThread::InstMtx;

Mp4DecoderThread::Mp4DecoderThread(const DECODER_PARAMS2* params)
: DecoderTID(-1)
, JumpToPos(-1)
, PlayedSecs(0)
, FAAD(NULL)
, NumFrames(0)
, CurrentFrame(0)
, ResumePcms(0)
{
  Mutex::Lock lock(InstMtx);
  Instances.append() = this;

  URL = params->URL;

  NextSkip  = JumpToPos = params->JumpTo;
  SkipSecs  = seek_window * params->SkipSpeed;
  State    = DECODER_STARTING;
  StopRq    = false;
  DecoderTID = _beginthread(PROXYFUNCREF(Mp4DecoderThread)DecoderThreadStub, 0, 65535, this);
  if (DecoderTID == -1)
    State = DECODER_STOPPED;
  else
    Play.Set(); // Go!
}

Mp4DecoderThread::~Mp4DecoderThread()
{
  DecoderCommand(DECODER_STOP, NULL);

  { Mutex::Lock lock(InstMtx);
    foreach (Mp4DecoderThread,*const*, dpp, Instances)
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

  if (FAAD)
    NeAACDecClose(FAAD);
}

/* Decoding thread. */
void Mp4DecoderThread::DecoderThread()
{
  sco_arr<unsigned char> buffer;

  // Open stream
  { Mutex::Lock lock(Mtx);
    SetFile(xio_fopen(URL, "rbXU"));
    if (GetFile() == NULL)
    { (*DecEvent)(A, DECEVENT_PLAYERROR, xstring().sprintf("Unable to open file '%s'\n%s", URL.cdata(), xio_strerror(xio_errno())).cdata());
      goto end;
    }

    int rc = Init(mp4ff_open_read(&Callbacks));
    if (rc != 0)
    { (*DecEvent)(A, DECEVENT_PLAYERROR, xstring().sprintf("Unable to open file '%s'\nUnsupported file format", URL.cdata()).cdata());
      goto end;
    }

    FAAD = NeAACDecOpen();

    NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(FAAD);
    config->outputFormat = FAAD_FMT_FLOAT;
    //config->downMatrix = 1; should be optional by config dialog

    unsigned char* decoder_info;
    unsigned decoder_info_size;
    if (mp4ff_get_decoder_config(MP4stream, Track, &decoder_info, &decoder_info_size))
    { (*DecEvent)(A, DECEVENT_PLAYERROR, xstring().sprintf("File '%s'\ndoes not contain AAC decoder information.", URL.cdata()).cdata());
      goto end;
    }

    unsigned long samplerate;
    unsigned char channels;
    rc = NeAACDecInit2(FAAD, decoder_info, decoder_info_size, &samplerate, &channels);
    free(decoder_info);
    if (rc)
    { (*DecEvent)(A, DECEVENT_PLAYERROR, xstring().sprintf("Failed to initialize AAC decoder: %i", rc).cdata());
      goto end;
    }

    Samplerate = (int)samplerate;
    Channels = channels;
  }

  NumFrames = mp4ff_num_samples(MP4stream, Track);
  CurrentFrame = 0;

  // After opening a new file we so are in its beginning, unless gapless
  if (JumpToPos == 0 && !SongOffset)
    JumpToPos = -1;

  // Decoder frame buffer
  buffer.reset(mp4ff_get_max_samples_size(MP4stream, Track));

  // always decode frame 0 to keep neaacdec happy
  int32_t frame_size;
  { Mutex::Lock lock(Mtx);
    frame_size = mp4ff_read_sample_v2(MP4stream, Track, CurrentFrame, buffer.get());
  }
  NeAACDecFrameInfo frame_info;
  NeAACDecDecode(FAAD, &frame_info, buffer.get(), frame_size);

  for(;;)
  { Play.Wait();

    for (;; ++CurrentFrame)
    { Play.Reset();
      if (StopRq)
        goto end;
      State = DECODER_PLAYING;

      double newpos = JumpToPos;
      if (SkipSecs && PlayedSecs >= NextSkip)
      { if (newpos < 0)
          newpos = PlayedSecs;
        newpos += SkipSecs;
        if (newpos < 0)
          break; // Begin of song
      }
      int32_t discardSamples = 0; // Discard the first samples of the current frame (in units of TimeScale)
      if (newpos >= 0)
      { DEBUGLOG(("Mp4DecoderThread:seek to %f\n", newpos));
        NextSkip = newpos + seek_window;
        { CurrentFrame = mp4ff_find_sample_use_offsets(MP4stream, Track, (int64_t)((newpos + SongOffset) * TimeScale + .5), &discardSamples);
          NeAACDecPostSeekReset(FAAD, CurrentFrame);
          newpos = JumpToPos;
          JumpToPos = -1;
        }
        DEBUGLOG(("Mp4DecoderThread:seek %i, %i\n", CurrentFrame, discardSamples));
        if (newpos >= 0)
          (*DecEvent)(A, DECEVENT_SEEKSTOP, NULL);
        if (CurrentFrame < 0 || CurrentFrame >= NumFrames)
          break; // Seek out of range => begin/end of song
        PlayedSecs = (double)(mp4ff_get_sample_position(MP4stream, Track, CurrentFrame) + discardSamples) / TimeScale + SongOffset;
      }

      // Beyond end?
      if (PlayedSecs >= SongLength)
        break;

      int32_t frame_duration;
      if (CurrentFrame == 0)
        frame_duration = 0;
      else if (CurrentFrame >= NumFrames)
        break; // end of song
      else
      { frame_duration = mp4ff_get_sample_duration(MP4stream, Track, CurrentFrame);
        if (frame_duration < 0)
        { (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
          goto end;
        }
      }
      frame_duration -= discardSamples;
      int32_t frame_size;
      { Mutex::Lock lock(Mtx);
        frame_size = mp4ff_read_sample_v2(MP4stream, Track, CurrentFrame, buffer.get());
      }
      if (frame_size <= 0)
      { (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
        goto end;
      }

      // AAC decode
      DEBUGLOG(("Mp4DecoderThread:NeAACDecDecode: %u {%08x,%08x,...}[%i]\n", CurrentFrame, ((int32_t*)buffer.get())[0], ((int32_t*)buffer.get())[1], frame_size));
      float* source = (float*)NeAACDecDecode(FAAD, &frame_info, buffer.get(), frame_size);

      if (frame_info.error > 0)
      { (*DecEvent)(A, DECEVENT_PLAYERROR, NeAACDecGetErrorMessage(frame_info.error));
        goto end;
      }
      if (!source || frame_info.samples == 0)
        continue; // error or empty => skip this frame

      discardSamples *= frame_info.samplerate / TimeScale; // SBR rescale
      source += frame_info.channels * discardSamples; // skip some data?
      frame_duration = frame_info.samples / frame_info.channels - discardSamples;
      if (frame_duration <= 0)
        continue; // empty frame?

      // cut if beyond SongLength
      if (PlayedSecs + frame_duration / frame_info.samplerate > SongLength)
      { frame_duration = (int)floor((SongLength - PlayedSecs * frame_info.samplerate + .5));
        if (frame_duration <= 0)
          break;
      }

      for (;;)
      { FORMAT_INFO2 output_format;
        output_format.channels = frame_info.channels;
        output_format.samplerate = frame_info.samplerate;
        float* target;
        int count = (*OutRequestBuffer)(A, &output_format, &target);
        if (count <= 0)
        { (*DecEvent)(A, DECEVENT_PLAYERROR, NULL);
          goto end;
        }

        if (count > frame_duration)
          count = frame_duration;
        memcpy(target, source, sizeof(float) * count * frame_info.channels);
        DEBUGLOG(("Mp4DecoderThread:OutCommitBuffer(, %i, %f)\n", count, PlayedSecs));
        (*OutCommitBuffer)(A, count, PlayedSecs);
        PlayedSecs += (double)count / frame_info.samplerate;

        if (frame_duration == count)
          break;
        frame_duration -= count;
        source += count * frame_info.channels;
      }
    }
    if (StopRq)
      goto end;
    DEBUGLOG(("Mp4DecoderThread::DecoderThread - playstop event - %.3f, %i\n", PlayedSecs, StopRq));
    (*DecEvent)(A, DECEVENT_PLAYSTOP, NULL);
  }

end:
  XFILE* file = InterlockedXch(&GetFile(), (XFILE*)NULL);
  if (file)
    xio_fclose(file);
  State = DECODER_STOPPED;
  DecoderTID = -1;
}

PROXYFUNCIMP(void, Mp4DecoderThread) TFNENTRY DecoderThreadStub(void* arg)
{ ((Mp4DecoderThread*)arg)->DecoderThread();
}

ULONG Mp4DecoderThread::DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params)
{ switch (msg)
  {
  case DECODER_PLAY:
    if (DecoderTID != -1)
    { if (State == DECODER_STOPPED)
        DecoderCommand(DECODER_STOP, NULL);
      else
        return PLUGIN_GO_ALREADY;
    }
  case DECODER_SETUP:
    // continue in base class
    return DECODER_STRUCT::DecoderCommand(msg, params);

  case DECODER_STOP:
  { if (DecoderTID == -1)
      return PLUGIN_GO_ALREADY;

    State = DECODER_STOPPING;
    StopRq = true;

    Play.Set();
    return PLUGIN_OK;
  }

  case DECODER_FFWD:
    /*if (params->Fast && File && xio_can_seek(File) != XIO_CAN_SEEK_FAST)
      return PLUGIN_UNSUPPORTED;*/
    SkipSecs = seek_window * params->SkipSpeed;
    JumpToPos = NextSkip = PlayedSecs;
    Play.Set();
    return PLUGIN_OK;

  case DECODER_JUMPTO:
    JumpToPos = NextSkip = params->JumpTo;
    Play.Set();
    return PLUGIN_OK;

  default:
    return PLUGIN_UNSUPPORTED;
  }
}

int Mp4DecoderThread::ReplaceStream(const char* sourcename, const char* destname)
{
/*  // Suspend all active instances of the updated file.
  Mutex::Lock lock(InstMtx);

  foreach(Mp4DecoderThread,*const*, dpp, Instances)
  { Mp4DecoderThread& dec = **dpp;
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
  foreach(Mp4DecoderThread,*const*, dpp, Instances)
  { Mp4DecoderThread& dec = **dpp;
    if (dec.ResumePcms != -1)
    { DEBUGLOG(("oggplay: resumes currently used file: %s\n", destname));
      dec.File = xio_fopen(destname, "rbXU");
      if (dec.File && dec.OggOpen() == PLUGIN_OK)
        ov_pcm_seek(&dec.VrbFile, dec.ResumePcms);
    }
    dec.Mtx.Release();
  }

  return rc;*/
  return -1;
}
