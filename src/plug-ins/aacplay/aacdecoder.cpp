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

#include "aacdecoder.h"
#include <memory.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <strutils.h>
#include <utilfct.h>
#include <cpp/smartptr.h>

const char* AacDecoder::Sniffer(XFILE* stream)
{ unsigned char buffer[10];
  if (xio_fread(buffer, sizeof(buffer), 1, stream) != 1)
    return NULL;
  if (*(uint32_t*)buffer == 0x46494441)
    return "ADIF";
  if (buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3')
  { // skip ID3 header
    unsigned len = buffer[6] << 21 | buffer[7] << 14 | buffer[8] << 7 | buffer[9];
    if (xio_fseek(stream, len, XIO_SEEK_CUR) || xio_fread(buffer, sizeof(buffer), 1, stream) != 1)
      return NULL;
  }
  if (buffer[0] == 0xff && (buffer[1] & 0xfe) == 0xf0)
    return "ADTS";
  return NULL;
}

AacDecoder::AacDecoder()
: FAAD(NULL)
, Tag(NULL)
, Samplerate(-1)
, Channels(-1)
, Songlength(-1)
{ }

AacDecoder::~AacDecoder()
{ if (FAAD)
    NeAACDecClose(FAAD);
  if (Tag)
    id3v2_free_tag(Tag);
}

const char* AacDecoder::Open(XFILE* stream)
{
  FAAD = NeAACDecOpen();
  if (!FAAD)
    return "Failed to init FAAD";
  Offset = 0;

  if (Buffer.size() < 4096)
    Buffer.reset(4096);
  Start = 0;
 retry:
  End = xio_fread(Buffer.get(), 1, Buffer.size(), stream);
  if ((int)End <= 0)
    return "Failed to read";

  if (Buffer[0] == 'I' && Buffer[1] == 'D' && Buffer[2] == '3')
  { // read ID3V2 tag
    Offset = Buffer[6] << 21 | Buffer[7] << 14 | Buffer[8] << 7 | Buffer[9] + 10;
    if (Offset > Buffer.size())
    { if (Offset > 64*1024)
      { DEBUGLOG(("AacDecoder::Open ID3V2 tag too long: %u\n", Start));
        if (xio_fseekl(stream, Offset, XIO_SEEK_SET) == -1)
          return "Cannot seek beyond too log ID3V2 tag.";
        goto retry;
      }
      sco_arr<unsigned char> buffer2(Offset);
      if (xio_fread(buffer2.get() + Buffer.size(), Offset - Buffer.size(), 1, stream) != 1)
        return "Failed to read ID3V2 tag";
      memcpy(buffer2.get(), Buffer.get(), Buffer.size());
      Buffer.swap(buffer2);
    } else if (Offset > End)
      return "Unexpected end of file";
    Tag = id3v2_load_tag((char*)Buffer.get(), Offset, ID3V2_GET_NONE);
    goto retry;
  }

  unsigned long samplerate;
  unsigned char channels;
  int rc = NeAACDecInit(FAAD, Buffer.get(), End, &samplerate, &channels);
  if (rc)
    return NeAACDecGetErrorMessage(rc);
  Samplerate = (int)samplerate;
  Channels = channels;
  Songlength = -1;

  return NULL;
}

static void copy_id3v2_string(ID3V2_FRAME* frame, xstring& result)
{
  char buffer[256];
  if (id3v2_get_string_ex(frame, buffer, sizeof buffer, 1004))
    result = buffer;
}

void AacDecoder::GetMeta(META_INFO& meta)
{
  if (!Tag)
    return;

  foreach (ID3V2_FRAME, *const*, pframe, Tag->id3_frames)
  { ID3V2_FRAME* frame = *pframe;
    if (frame->fr_desc)
      switch (frame->fr_desc->fd_id)
      {case ID3V2_TIT2:
        copy_id3v2_string(frame, meta.title);
        break;
       case ID3V2_TPE1:
        copy_id3v2_string(frame, meta.artist);
        break;
       case ID3V2_TALB:
        copy_id3v2_string(frame, meta.album);
        break;
       case ID3V2_TCON:
        copy_id3v2_string(frame, meta.genre);
        break;
       case ID3V2_TDRC:
        copy_id3v2_string(frame, meta.year);
        break;
       case ID3V2_COMM:
        { // Skip iTunes specific comment tags.
          char buffer[6];
          if (!id3v2_get_description(frame, buffer, sizeof buffer) || strnicmp(buffer, "iTun", 4) != 0)
            copy_id3v2_string(frame, meta.comment);
          break;
        }
       case ID3V2_TCOP:
        copy_id3v2_string(frame, meta.copyright);
        break;
       case ID3V2_TRCK:
        copy_id3v2_string(frame, meta.track);
        break;
       case ID3V2_RVA2:
        { // Format of RVA2 frame:
          // Identification          <text string> $00
          // Type of channel         $xx
          // Volume adjustment       $xx xx
          // Bits representing peak  $xx
          // Peak volume             $xx (xx ...)
          if (id3v2_decompress_frame(frame) == -1)
            break;
          const char* data = (char*)frame->fr_data;
          const char* dataE = data + frame->fr_size;
          bool isalbum = stricmp(data, "album") == 0;
          // Skip identification string.
          data += strnlen(data, frame->fr_size) + 1;
          // Search the master volume.
          while (data + 4 < dataE)
          { const unsigned char* rva2 = (const unsigned char*)data;
            data += 3 + ((rva2[3] + 7) >> 3);
            if (data > dataE)
              break;
            if (rva2[0] == 0x01)
            { float gain = ((signed char)rva2[1] << 8 | rva2[2]) / 512.f;
              (isalbum ? meta.album_gain : meta.track_gain) = gain;
              // decode peak
              float peak;
              switch ((data[3] + 7) >> 3)
              {case 3:
                peak += data[6] / 65536.f;
               case 2:
                peak += data[5] / 256.f;
               case 1:
                peak += data[4];
                peak = -20 * log10f(peak / (1 << ((data[3] - 1) & 7)));
                if (peak > gain)
                  peak = gain;
                (isalbum ? meta.album_peak : meta.track_peak) = peak;
              }
              break;
            } else
              data += 3 + ((data[3] + 7) >> 3);
          }
        }
       case ID3V2_TLEN:
        { char buffer[20];
          id3v2_get_string_ex(frame, buffer, sizeof buffer, 1004);
          size_t len = (size_t)-1;
          long millis;
          sscanf(buffer, "%lu%zn", &millis, &len);
          if (len == strnlen(buffer, sizeof buffer))
            Songlength = millis / 1000.;
        }
      }
  }
}


vector<AacDecoderThread> AacDecoderThread::Instances;
Mutex AacDecoderThread::InstMtx;

AacDecoderThread::AacDecoderThread(const DECODER_PARAMS2* params)
: File(NULL)
, DecoderTID(-1)
, JumpToPos(-1)
, PlayedSecs(0)
{
  Mutex::Lock lock(InstMtx);
  Instances.append() = this;

  URL = params->URL;

  NextSkip  = JumpToPos = params->JumpTo;
  //SkipSecs  = seek_window * params->SkipSpeed;
  State    = DECODER_STARTING;
  StopRq    = false;
  DecoderTID = _beginthread(PROXYFUNCREF(AacDecoderThread)DecoderThreadStub, 0, 65535, this);
  if (DecoderTID == -1)
    State = DECODER_STOPPED;
  else
    Play.Set(); // Go!
}

AacDecoderThread::~AacDecoderThread()
{
  DecoderCommand(DECODER_STOP, NULL);

  { Mutex::Lock lock(InstMtx);
    foreach (AacDecoderThread,*const*, dpp, Instances)
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

  if (File)
    xio_fclose(File);
}

/* Decoding thread. */
void AacDecoderThread::DecoderThread()
{
  Buffer.reset(65536);

  // Open stream
  { Mutex::Lock lock(Mtx);
    File = xio_fopen(URL, "rbXU");
    if (File == NULL)
    { (*DecEvent)(A, DECEVENT_PLAYERROR, xstring().sprintf("Unable to open file '%s'\n%s", URL.cdata(), xio_strerror(xio_errno())).cdata());
      goto end;
    }

    const char* err = Open(File);
    if (err)
    { (*DecEvent)(A, DECEVENT_PLAYERROR, xstring().sprintf("Unable to play file '%s'\n%s", URL.cdata(), err).cdata());
      goto end;
    }

    NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(FAAD);
    config->outputFormat = FAAD_FMT_FLOAT;
    //config->downMatrix = 1; should be optional by config dialog
  }

  // After opening a new file we so are in its beginning.
  if (JumpToPos == 0)
    JumpToPos = -1;

  for(;;)
  { Play.Wait();

    for (;;)
    { Play.Reset();
      if (StopRq)
        goto end;
      State = DECODER_PLAYING;

      double newpos = JumpToPos;
      /*if (SkipSecs && PlayedSecs >= NextSkip)
      { if (newpos < 0)
          newpos = PlayedSecs;
        newpos += SkipSecs;
        if (newpos < 0)
          break; // Begin of song
      }*/
      int32_t discardSamples = 0; // Discard the first samples of the current frame (in units of TimeScale)
      /*if (newpos >= 0)
      { DEBUGLOG(("mp4play:seek to %f\n", newpos));
        NextSkip = newpos + seek_window;
        { CurrentFrame = mp4ff_find_sample_use_offsets(MP4stream, Track, (int64_t)(newpos * TimeScale), &discardSamples);
          NeAACDecPostSeekReset(FAAD, CurrentFrame);
          newpos = JumpToPos;
          JumpToPos = -1;
        }
        DEBUGLOG(("mp4play:seek %i, %i\n", CurrentFrame, discardSamples));
        if (newpos >= 0)
          (*DecEvent)(A, DECEVENT_SEEKSTOP, NULL);
        if (CurrentFrame < 0 || CurrentFrame > NumFrames)
          break; // Seek out of range => begin/end of song
        PlayedSecs = (double)(mp4ff_get_sample_position(MP4stream, Track, CurrentFrame) + discardSamples) / TimeScale;
      }*/

      // read data
      if (End - Start < 500)
      {dataneeded:
        memmove(Buffer.get(), Buffer.get() + Start, End -= Start);
        Start = 0;
        int count = xio_fread(Buffer.get() + End, 1, Buffer.size() - End, File);
        DEBUGLOG(("AacDecoderThread:read: %i\n", count));
        if (count <= 0)
        { if (xio_feof(File))
          { (*DecEvent)(A, DECEVENT_PLAYSTOP, NULL);
            continue;
          }
          (*DecEvent)(A, DECEVENT_PLAYERROR, xio_strerror(xio_errno()));
          goto end;
        }
        End += count;
      }

      // AAC decode
      DEBUGLOG(("AacDecoderThread:NeAACDecDecode: {%08x,%08x,...}\n", ((int32_t*)(Buffer.get()+Start))[0], ((int32_t*)(Buffer.get()+Start))[1]));
      NeAACDecFrameInfo frame_info;
      float* source = (float*)NeAACDecDecode(FAAD, &frame_info, Buffer.get() + Start, End - Start);

      if (frame_info.error > 0)
      { // more data needed?
        // error 14 (Input data buffer too small) is not reliable, so retry if less than the maximum AAC frame length in Buffer.
        if (frame_info.error == 14 || End - Start < 6144)
          goto dataneeded;
        // ignore all errors in case of EOF
        if (xio_feof(File))
        { (*DecEvent)(A, DECEVENT_PLAYSTOP, NULL);
          continue;
        }
        (*DecEvent)(A, DECEVENT_PLAYERROR, NeAACDecGetErrorMessage(frame_info.error));
        goto end;
      }
      if (!source || frame_info.samples == 0)
        continue; // error or empty => skip this frame
      // frame successfully decoded
      Start += frame_info.bytesconsumed;
      if (Start > End)
        Start = End = 0;

      //discardSamples *= frame_info.samplerate / TimeScale; // SBR rescale
      source += frame_info.channels * discardSamples; // skip some data?
      long frame_duration = frame_info.samples / frame_info.channels - discardSamples;

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
        DEBUGLOG(("AacDecoderThread:OutCommitBuffer(, %i, %f)\n", count, PlayedSecs));
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
    DEBUGLOG(("AacDecoderThread::DecoderThread - playstop event - %.3f, %i\n", PlayedSecs, StopRq));
    (*DecEvent)(A, DECEVENT_PLAYSTOP, NULL);
  }

end:
  XFILE* file = InterlockedXch(&File, (XFILE*)NULL);
  if (file)
    xio_fclose(file);
  State = DECODER_STOPPED;
  DecoderTID = -1;
}

PROXYFUNCIMP(void, AacDecoderThread) TFNENTRY DecoderThreadStub(void* arg)
{ ((AacDecoderThread*)arg)->DecoderThread();
}

ULONG AacDecoderThread::DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params)
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
    //if (params->Fast && File && xio_can_seek(File) != XIO_CAN_SEEK_FAST)
      return PLUGIN_UNSUPPORTED;
    /*SkipSecs = seek_window * params->SkipSpeed;
    JumpToPos = NextSkip = PlayedSecs;
    Play.Set();
    return PLUGIN_OK;*/

  case DECODER_JUMPTO:
    JumpToPos = NextSkip = params->JumpTo;
    Play.Set();
    return PLUGIN_OK;

  default:
    return PLUGIN_UNSUPPORTED;
  }
}
