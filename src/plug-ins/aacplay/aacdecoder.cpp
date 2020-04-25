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
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <strutils.h>
#include <cpp/smartptr.h>

const char* AacDecoder::Sniffer(XFILE* stream)
{
  unsigned char buffer[10];
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

  unsigned char buffer[1024];
 retry:
  int count = xio_fread(buffer, 1, sizeof buffer, stream);
  if (count <= 0)
    return "Failed to read";

  if (buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3')
  { // read ID3V2 tag
    Offset = buffer[6] << 21 | buffer[7] << 14 | buffer[8] << 7 | buffer[9] + 10;
    sco_arr<unsigned char> buffer2;
    unsigned char* tag = buffer;
    if (Offset > sizeof buffer)
    { if (Offset > 64*1024)
      { DEBUGLOG(("AacDecoder::Open ID3V2 tag too long: %u\n", Offset));
        xio_fseek(stream, Offset, XIO_SEEK_SET);
        goto retry;
      }
      buffer2.reset(Offset);
      tag = buffer2.get();
      if (xio_fread(tag + sizeof buffer, Offset - sizeof buffer, 1, stream) != 1)
        return "Failed to read ID3V2 tag";
      memcpy(tag, buffer, sizeof buffer);
    } else if (Offset > (unsigned)count)
      return "Unexpected end of file";
    Tag = id3v2_load_tag((char*)tag, Offset, ID3V2_GET_NONE);
    goto retry;
  }

  unsigned long samplerate;
  unsigned char channels;
  int rc = NeAACDecInit(FAAD, buffer, count, &samplerate, &channels);
  if (rc)
    return NeAACDecGetErrorMessage(rc);
  Samplerate = (int)samplerate;
  Channels = channels;

  return NULL;
}

static void copy_id3v2_string(ID3V2_FRAME* frame, xstring& result)
{
  char buffer[256];
  if (id3v2_get_string_ex(frame, buffer, sizeof buffer, 1004))
    result = buffer;
}

double AacDecoder::GetMeta(META_INFO& meta)
{
  if (!Tag)
    return -1;

  double songlength;
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
            songlength = millis / 1000.;
        }
      }
  }
  return songlength;
}
